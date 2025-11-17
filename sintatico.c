#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexico.h"   // importa os tokens, structs e funções do seu léxico

Token tokenAtual;
Scanner S;

/* ========================= Tratamento de erro ========================= */

void erro_sintatico(const char *msg) {
    if (tokenAtual.tipo == TOKEN_FIM) {
        fprintf(stderr, "%d:fim de arquivo não esperado.\n", tokenAtual.linha);
    } else {
        fprintf(stderr, "%d:token nao esperado [%s]. %s\n",
                tokenAtual.linha, tokenAtual.lexema, msg);
    }
    exit(1);
}

void CasaToken(TipoToken esperado) {
    if (tokenAtual.tipo == esperado) {
        free(tokenAtual.lexema);              // libera memória do lexema anterior
        tokenAtual = proximo_token(&S);       // lê próximo token
    } else {
        erro_sintatico("Token diferente do esperado.");
    }
}

/* ====================== Declarações dos não-terminais ====================== */

void programa();
void bloco();
void parte_decl_variaveis();
void declaracao_variaveis();
void lista_identificadores();
void tipo();
void comando_composto();
void comando();
void atribuicao();
void comando_condicional();
void comando_repetitivo();
void expressao();
void expressao_simples();
void termo();
void fator();

/* ========================= Implementações ========================= */

void programa() {
    printf("Regra: programa -> program identificador ; bloco .\n");

    CasaToken(TOKEN_PALAVRA_RESERVADA); // program
    CasaToken(TOKEN_IDENTIFICADOR);
    CasaToken(TOKEN_PONTO_VIRGULA);
    bloco();
    CasaToken(TOKEN_PONTO);
}

void bloco() {
    printf("Regra: bloco -> parte_decl_variaveis comando_composto\n");

    parte_decl_variaveis();
    comando_composto();
}

void parte_decl_variaveis() {
    printf("Regra: parte_decl_variaveis\n");

    while (tokenAtual.tipo == TOKEN_PALAVRA_RESERVADA &&
           strcmp(tokenAtual.lexema, "var") == 0) {
        CasaToken(TOKEN_PALAVRA_RESERVADA); // var
        declaracao_variaveis();
        CasaToken(TOKEN_PONTO_VIRGULA);
    }
}

void declaracao_variaveis() {
    printf("Regra: declaracao_variaveis -> lista_identificadores : tipo\n");

    lista_identificadores();
    CasaToken(TOKEN_DOIS_PONTOS);
    tipo();
}

void lista_identificadores() {
    printf("Regra: lista_identificadores -> ident { , ident }\n");

    CasaToken(TOKEN_IDENTIFICADOR);

    while (tokenAtual.tipo == TOKEN_VIRGULA) {
        CasaToken(TOKEN_VIRGULA);
        CasaToken(TOKEN_IDENTIFICADOR);
    }
}

void tipo() {
    printf("Regra: tipo -> integer | real\n");

    if (strcmp(tokenAtual.lexema, "integer") == 0 ||
        strcmp(tokenAtual.lexema, "real") == 0) {
        CasaToken(TOKEN_PALAVRA_RESERVADA);
    } else {
        erro_sintatico("Esperado tipo integer ou real.");
    }
}

void comando_composto() {
    printf("Regra: comando_composto -> begin comando ; { comando ; } end\n");

    if (strcmp(tokenAtual.lexema, "begin") != 0)
        erro_sintatico("Esperado begin.");

    CasaToken(TOKEN_PALAVRA_RESERVADA); // begin

    comando();
    CasaToken(TOKEN_PONTO_VIRGULA);

    while (strcmp(tokenAtual.lexema, "end") != 0) {
        comando();
        CasaToken(TOKEN_PONTO_VIRGULA);
    }

    CasaToken(TOKEN_PALAVRA_RESERVADA); // end
}

void comando() {
    printf("Regra: comando\n");

    if (tokenAtual.tipo == TOKEN_IDENTIFICADOR) {
        atribuicao();
    }
    else if (strcmp(tokenAtual.lexema, "begin") == 0) {
        comando_composto();
    }
    else if (strcmp(tokenAtual.lexema, "if") == 0) {
        comando_condicional();
    }
    else if (strcmp(tokenAtual.lexema, "while") == 0) {
        comando_repetitivo();
    }
    else {
        erro_sintatico("Comando inválido.");
    }
}

void atribuicao() {
    printf("Regra: atribuicao -> identificador := expressao\n");

    CasaToken(TOKEN_IDENTIFICADOR);
    CasaToken(TOKEN_ATRIBUICAO);
    expressao();
}

void comando_condicional() {
    printf("Regra: condicional -> if expressao then comando [else comando]\n");

    CasaToken(TOKEN_PALAVRA_RESERVADA); // if
    expressao();
    CasaToken(TOKEN_PALAVRA_RESERVADA); // then
    comando();

    if (strcmp(tokenAtual.lexema, "else") == 0) {
        CasaToken(TOKEN_PALAVRA_RESERVADA);
        comando();
    }
}

void comando_repetitivo() {
    printf("Regra: repetitivo -> while expressao do comando\n");

    CasaToken(TOKEN_PALAVRA_RESERVADA); // while
    expressao();
    CasaToken(TOKEN_PALAVRA_RESERVADA); // do
    comando();
}

/* EXPRESSÕES */

void expressao() {
    printf("Regra: expressao\n");

    expressao_simples();

    if (tokenAtual.tipo == TOKEN_IGUAL ||
        tokenAtual.tipo == TOKEN_MENOR ||
        tokenAtual.tipo == TOKEN_MAIOR ||
        tokenAtual.tipo == TOKEN_MENOR_IGUAL ||
        tokenAtual.tipo == TOKEN_MAIOR_IGUAL ||
        tokenAtual.tipo == TOKEN_DIFERENTE) {

        CasaToken(tokenAtual.tipo);
        expressao_simples();
    }
}

void expressao_simples() {
    printf("Regra: expressao_simples\n");

    if (tokenAtual.tipo == TOKEN_MAIS || tokenAtual.tipo == TOKEN_MENOS) {
        CasaToken(tokenAtual.tipo);
    }

    termo();

    while (tokenAtual.tipo == TOKEN_MAIS || tokenAtual.tipo == TOKEN_MENOS) {
        CasaToken(tokenAtual.tipo);
        termo();
    }
}

void termo() {
    printf("Regra: termo\n");

    fator();

    while (tokenAtual.tipo == TOKEN_MULT || tokenAtual.tipo == TOKEN_DIV) {
        CasaToken(tokenAtual.tipo);
        fator();
    }
}

void fator() {
    printf("Regra: fator\n");

    if (tokenAtual.tipo == TOKEN_IDENTIFICADOR) {
        CasaToken(TOKEN_IDENTIFICADOR);
    }
    else if (tokenAtual.tipo == TOKEN_INTEIRO) {
        CasaToken(TOKEN_INTEIRO);
    }
    else if (tokenAtual.tipo == TOKEN_ABRE_PAR) {
        CasaToken(TOKEN_ABRE_PAR);
        expressao();
        CasaToken(TOKEN_FECHA_PAR);
    }
    else {
        erro_sintatico("Fator inválido.");
    }
}

/* ========================= MAIN ========================= */

int main(int argc, char *argv[]) {

    if (argc < 2) {
        fprintf(stderr, "Uso: %s arquivo.pas\n", argv[0]);
        return 1;
    }

    // carrega o arquivo fonte
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        fprintf(stderr, "Erro ao abrir o arquivo %s\n", argv[1]);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *codigo = malloc(size + 2);
    fread(codigo, 1, size, f);
    fclose(f);
    codigo[size] = '\0';

    // output_file vem do lexico.c (variável global)
    output_file = fopen("sintatico.lex", "w");
    if (!output_file) {
        fprintf(stderr, "Erro ao criar sintatico.lex\n");
        return 1;
    }

    // inicia léxico e scanner
    iniciar_tabela_simbolos();
    iniciar(&S, codigo);

    // pega o primeiro token
    tokenAtual = proximo_token(&S);

    // chama o símbolo inicial
    programa();

    printf("Analise sintatica concluida com sucesso!\n");

    free(codigo);
    liberar_tabela_simbolos();
    free(tokenAtual.lexema);

    if (output_file) fclose(output_file);

    return 0;
}