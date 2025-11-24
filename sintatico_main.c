// sintatico_main.c
// Programa principal separado para rodar o analisador sintático individualmente.

#include <stdio.h>
#include <stdlib.h>
#include "lexico.h"   // declara proximo_token, iniciar, iniciar_tabela_simbolos, extern FILE *output_file

extern Token tokenAtual; // declarado no sintatico.c (se não estiver, remova esta linha)
extern Scanner S;        // idem

// Se no seu sintatico.c tokenAtual e S são 'static' ou não exportados, não use extern e declare locais aqui.
// Mas ideal é que sintatico.c declare tokenAtual e S como não-static (como estamos usando).

// Forward do símbolo inicial (se sintatico.c já tem a função 'programa', apenas chamamos)
void programa(void);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s arquivo.pas\n", argv[0]);
        return 1;
    }

    // abre arquivo fonte
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        fprintf(stderr, "Erro ao abrir o arquivo %s\n", argv[1]);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *codigo = malloc(size + 2);
    if (!codigo) {
        fprintf(stderr, "Erro de alocacao\n");
        fclose(f);
        return 1;
    }
    size_t lidos = fread(codigo, 1, size, f);
    (void)lidos;
    fclose(f);
    codigo[size] = '\0';

    // prepara arquivo de log para o sintático
    output_file = fopen("sintatico.lex", "w");
    if (!output_file) {
        fprintf(stderr, "Erro ao criar sintatico.lex\n");
        free(codigo);
        return 1;
    }

    // inicia léxico / tabela de símbolos / scanner
    iniciar_tabela_simbolos();
    iniciar(&S, codigo);

    // pega o primeiro token no tokenAtual (sintatico.c espera isso)
    tokenAtual = proximo_token(&S);

    // chama o processo sintático (símbolo inicial)
    programa();

    printf("Analise sintatica concluida com sucesso!\n");

    // cleanup
    liberar_tabela_simbolos();
    free(tokenAtual.lexema);
    free(codigo);
    if (output_file) fclose(output_file);

    return 0;
}