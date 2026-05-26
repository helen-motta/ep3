#ifndef EP3_H
#define EP3_H

#define NUM_QUADROS 32
#define NUM_PAGINAS 64
#define NUM_PROCESSOS 4
#define PAGINAS_POR_PROCESSO 16
#define TAM_PAGINA 4096
#define TAM_ESPACO_PROCESSO (PAGINAS_POR_PROCESSO * TAM_PAGINA)
#define VALOR_AUSENTE 255

#define NRU 1
#define SEGUNDA_CHANCE 2
#define LRU 3

typedef char Operacao;
#define OP_LEITURA 'L'
#define OP_ESCRITA 'E'

int carregar_pgm_fisica(const char *caminho, int memoria_fisica[NUM_QUADROS]);
int carregar_pgm_virtual(const char *caminho, int memoria_virtual[NUM_PAGINAS]);
int salvar_pgm_fisica(const char *caminho, const int memoria_fisica[NUM_QUADROS]);
int salvar_pgm_virtual(const char *caminho, const int memoria_virtual[NUM_PAGINAS]);

int executar_acesso(int pid, Operacao operacao, int endereco);

void imprimir_memoria_fisica(const int memoria_fisica[NUM_QUADROS]);
void imprimir_memoria_virtual(const int memoria_virtual[NUM_PAGINAS]);

#endif
