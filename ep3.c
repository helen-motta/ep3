#include "ep3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int memoria_fisica[NUM_QUADROS];
int memoria_virtual[NUM_PAGINAS];
int bit_r[NUM_PAGINAS];
int bit_m[NUM_PAGINAS];
int matriz_lru[NUM_PAGINAS][NUM_PAGINAS];
int fila_chance[NUM_PAGINAS];
int esta_na_fila[NUM_PAGINAS];

int inicio_fila = 0;
int fim_fila = 0;
int tamanho_fila = 0;
long page_faults = 0;
long page_hits = 0;
int acessos = 0;
int algoritmo;

char arquivo_fisica_caminho[256];
char arquivo_virtual_caminho[256];

int carregar_pgm_fisica(const char *caminho, int memoria_fisica[NUM_QUADROS]) {
    FILE *arq = fopen(caminho, "r");
    char magic[16];
    int largura, altura, maximo;

    if (arq == NULL) return 0;

    // le o cabecalho pgm esperado para a memoria fisica
    if (fscanf(arq, "%15s", magic) != 1 || strcmp(magic, "P2") != 0) {
        fclose(arq);
        return 0;
    }

    if (fscanf(arq, "%d %d %d", &largura, &altura, &maximo) != 3) {
        fclose(arq);
        return 0;
    }

    if (largura != 1 || altura != NUM_QUADROS || maximo != 255) {
        fclose(arq);
        return 0;
    }

    // carrega os quadros da memoria fisica
    for (int i = 0; i < NUM_QUADROS; i++) {
        if (fscanf(arq, "%d", &memoria_fisica[i]) != 1) {
            fclose(arq);
            return 0;
        }
    }

    fclose(arq);
    return 1;
}

int carregar_pgm_virtual(const char *caminho, int memoria_virtual[NUM_PAGINAS]) {
    FILE *arq = fopen(caminho, "r");
    char magic[16];
    int largura, altura, maximo;

    if (arq == NULL) return 0;

    // le o cabecalho pgm esperado para a memoria virtual
    if (fscanf(arq, "%15s", magic) != 1 || strcmp(magic, "P2") != 0) {
        fclose(arq);
        return 0;
    }

    if (fscanf(arq, "%d %d %d", &largura, &altura, &maximo) != 3) {
        fclose(arq);
        return 0;
    }

    if (largura != 1 || altura != NUM_PAGINAS || maximo != 255) {
        fclose(arq);
        return 0;
    }

    // carrega as paginas da memoria virtual
    for (int i = 0; i < NUM_PAGINAS; i++) {
        if (fscanf(arq, "%d", &memoria_virtual[i]) != 1) {
            fclose(arq);
            return 0;
        }
    }

    fclose(arq);
    return 1;
}

int salvar_pgm_fisica(const char *caminho, const int memoria_fisica[NUM_QUADROS]) {
    FILE *arq = fopen(caminho, "w");

    if (arq == NULL) return 0;

    // escreve o pgm da memoria fisica com um quadro por linha
    fprintf(arq, "P2\n1 %d\n255\n", NUM_QUADROS);
    for (int i = 0; i < NUM_QUADROS; i++) {
        fprintf(arq, "%d\n", memoria_fisica[i]);
    }

    fclose(arq);
    return 1;
}

int salvar_pgm_virtual(const char *caminho, const int memoria_virtual[NUM_PAGINAS]) {
    FILE *arq = fopen(caminho, "w");

    if (arq == NULL) return 0;

    // escreve o pgm da memoria virtual com uma pagina por linha
    fprintf(arq, "P2\n1 %d\n255\n", NUM_PAGINAS);
    for (int i = 0; i < NUM_PAGINAS; i++) {
        fprintf(arq, "%d\n", memoria_virtual[i]);
    }

    fclose(arq);
    return 1;
}

int pagina_dona_do_quadro(int quadro) {
    for (int i = 0; i < NUM_PAGINAS; i++) {
        if (memoria_virtual[i] == quadro) return i;
    }
    return -1;
}

// algoritmo de 2a chance
void enfileirar_chance(int pagina) {
    if (pagina < 0 || pagina >= NUM_PAGINAS || memoria_virtual[pagina] == VALOR_AUSENTE 
        || esta_na_fila[pagina] || tamanho_fila >= NUM_PAGINAS) 
        return;

    fila_chance[fim_fila] = pagina;
    fim_fila = (fim_fila + 1) % NUM_PAGINAS;
    tamanho_fila++;
    esta_na_fila[pagina] = 1;
}

int desenfileirar_chance() {
    if (tamanho_fila == 0) return -1;

    int pagina = fila_chance[inicio_fila];
    inicio_fila = (inicio_fila + 1) % NUM_PAGINAS;
    tamanho_fila--;
    esta_na_fila[pagina] = 0;
    return pagina;
}

void preencher_fila_se_vazia() {
    if (tamanho_fila != 0) return;

    for (int q = 0; q < NUM_QUADROS; q++) {
        int p = pagina_dona_do_quadro(q);
        if (p != -1) enfileirar_chance(p);
    }
}


// NRU
int escolher_vitima_nru() {
    int melhor_classe = 4;
    int melhor_quadro = 0;

    for (int q = 0; q < NUM_QUADROS; q++) {
        int p = pagina_dona_do_quadro(q);
        if (p == -1) return q;

        // Calcula a classe com base nos bits R e M
        int classe = (bit_r[p] ? 2 : 0) + (bit_m[p] ? 1 : 0);
        if (classe < melhor_classe) {
            melhor_classe = classe;
            melhor_quadro = q;
            if (classe == 0) break;
        }
    }
    return melhor_quadro;
}

int escolher_vitima_segunda_chance() {
    int tentativas = 0;
    preencher_fila_se_vazia();

    while (tentativas < NUM_PAGINAS * 4) {
        int p = desenfileirar_chance();

        if (p == -1 || memoria_virtual[p] == VALOR_AUSENTE) {
            preencher_fila_se_vazia();
            tentativas++;
            continue;
        }

        // se o bit de acesso for 0, é a vítima
        if (bit_r[p] == 0) {
            return memoria_virtual[p];
        }

        // Se for 1, ganha uma "segunda chance" (zera o bit e joga pro fim da fila)
        bit_r[p] = 0;
        enfileirar_chance(p);
        tentativas++;
    }
    return escolher_vitima_nru();
}

// LRU
int escolher_vitima_lru() {
    int melhor_quadro = 0;
    unsigned long long melhor_valor = ULLONG_MAX;

    for (int q = 0; q < NUM_QUADROS; q++) {
        int p = pagina_dona_do_quadro(q);
        if (p == -1) return q;

        // Converte a linha da matriz LRU em um número decimal legível
        unsigned long long valor_linha = 0;
        for (int i = 0; i < NUM_PAGINAS; i++) {
            valor_linha = (valor_linha * 2) + matriz_lru[p][i];
        }

        if (valor_linha < melhor_valor) {
            melhor_valor = valor_linha;
            melhor_quadro = q;
        }
    }
    return melhor_quadro;
}

// escolha da vítima
int escolher_vitima() {
    if (algoritmo == NRU) return escolher_vitima_nru();
    if (algoritmo == SEGUNDA_CHANCE) return escolher_vitima_segunda_chance();
    return escolher_vitima_lru();
}

int executar_acesso(int pid, char operacao, int endereco) {
    // página virtual que está sendo acessada
    int pagina = (pid / 64) * PAGINAS_POR_PROCESSO + (endereco / TAM_PAGINA);
    int hit = (memoria_virtual[pagina] != VALOR_AUSENTE);

    acessos++;

    if (hit) {
        page_hits++;
    } else {
        page_faults++;
        int quadro_vitima = escolher_vitima();
        int pagina_vitima = pagina_dona_do_quadro(quadro_vitima);

        // Se a página vítima estiver mapeada, precisa ser desalocada
        if (pagina_vitima != -1) {
            memoria_virtual[pagina_vitima] = VALOR_AUSENTE;
            bit_r[pagina_vitima] = 0;
            bit_m[pagina_vitima] = 0;
            esta_na_fila[pagina_vitima] = 0;
        }

        // Mapeia a nova página
        memoria_virtual[pagina] = quadro_vitima;
        memoria_fisica[quadro_vitima] = pid;
    }

    // Atualiza os bits de histórico
    bit_r[pagina] = 1;
    if (operacao == 'E') bit_m[pagina] = 1;

    // Atualiza estruturas internas dos algoritmos
    if (algoritmo == SEGUNDA_CHANCE) enfileirar_chance(pagina);
    if (algoritmo == LRU) {
        for (int i = 0; i < NUM_PAGINAS; i++) {
            matriz_lru[pagina][i] = 1;
            matriz_lru[i][pagina] = 0;
        }
        matriz_lru[pagina][pagina] = 0;
    }

    // reseta o bit R à cada 100 acessos
    if (acessos % 100 == 0) {
        for (int i = 0; i < NUM_PAGINAS; i++) bit_r[i] = 0;
    }

    return hit ? 1 : 0;
}

int main(int argc, char **argv) {
    char linha[256];

    if (argc != 4) return 1;

    algoritmo = atoi(argv[3]);

    strcpy(arquivo_fisica_caminho, argv[1]);
    strcpy(arquivo_virtual_caminho, argv[2]);

    if (!carregar_pgm_fisica(arquivo_fisica_caminho, memoria_fisica)) return 1;
    if (!carregar_pgm_virtual(arquivo_virtual_caminho, memoria_virtual)) return 1;

    // Loop de cmds do terminal
    while (1) {
        printf("> ");

        if (fgets(linha, sizeof(linha), stdin) == NULL) break;

        if (strncmp(linha, "imprime virtual", 15) == 0) {
            printf("P2\n1 %d\n255\n", NUM_PAGINAS);
            for (int i = 0; i < NUM_PAGINAS; i++) {
                printf("%d\n", memoria_virtual[i]);
            }
            
        } else if (strncmp(linha, "imprime fisica", 14) == 0) {
            printf("P2\n1 %d\n255\n", NUM_QUADROS);
            for (int i = 0; i < NUM_QUADROS; i++) {
                printf("%d\n", memoria_fisica[i]);
            }
            
        } else if (strncmp(linha, "sair", 4) == 0) {
            salvar_pgm_fisica(arquivo_fisica_caminho, memoria_fisica);
            salvar_pgm_virtual(arquivo_virtual_caminho, memoria_virtual);
            printf("%ld\n%ld\n", page_faults, page_hits);
            break;
            
        } else {
            int pid, endereco;
            char op;
            if (sscanf(linha, " %d %c %d", &pid, &op, &endereco) == 3) {
                int resultado = executar_acesso(pid, op, endereco);
                if (resultado == 1) printf("hit\n");
                else if (resultado == 0) printf("fault\n");
            }
        }
    }

    return 0;
}