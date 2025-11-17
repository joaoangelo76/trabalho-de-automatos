#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexico.h"

extern FILE *output_file;

int main(int argc, char *argv[]) {
    char *Entrada = NULL;
    long total_lido = 0;

    if (argc >= 2) {
        FILE *input_file = fopen(argv[1], "rb");
        if (!input_file) {
            fprintf(stderr, "Erro ao abrir o arquivo de entrada: %s\n", argv[1]);
            return 1;
        }

        fseek(input_file, 0, SEEK_END);
        long file_size = ftell(input_file);
        fseek(input_file, 0, SEEK_SET);
        Entrada = malloc(file_size + 2);
        if (!Entrada){ fprintf(stderr, "Erro de alocacao\n"); fclose(input_file); return 1; }
        size_t lidos = fread(Entrada, 1, file_size, input_file);
        (void)lidos;
        fclose(input_file);

        while(file_size > 0 && (Entrada[file_size-1] == '\n' || Entrada[file_size-1] == '\r')){
            file_size--;
        }
        Entrada[file_size] = '\0';

        char output_filename[512];
        snprintf(output_filename, sizeof(output_filename), "%s.lex", argv[1]);
        output_file = fopen(output_filename, "w");
        if(!output_file){
            fprintf(stderr, "Erro ao criar o arquivo de saída: %s\n", output_filename);
            free(Entrada);
            return 1;
        }
    }

    iniciar_tabela_simbolos();

    Scanner S;
    iniciar(&S, Entrada);

    for(;;){
        Token t = proximo_token(&S);
        printf("<%s, \"%s\"> @ linha %d, col %d\n",
               nome_token(t.tipo), t.lexema, t.linha, t.coluna);
        free(t.lexema);
        if(t.tipo==TOKEN_FIM) break;
    }

    liberar_tabela_simbolos();
    free(Entrada);
    fclose(output_file);

    return 0;
}
