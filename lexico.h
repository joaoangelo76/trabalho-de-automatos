#ifndef LEXICO_H
#define LEXICO_H

#include <stdio.h>

/* ========================= Tipos de Tokens ========================= */

typedef enum {
    TOKEN_INTEIRO,
    TOKEN_STRING,
    TOKEN_MAIS,
    TOKEN_MENOS,
    TOKEN_MULT,
    TOKEN_PONTO_VIRGULA,
    TOKEN_PONTO,
    TOKEN_DOIS_PONTOS,
    TOKEN_MENOR,
    TOKEN_MAIOR,
    TOKEN_MENOR_IGUAL,
    TOKEN_MAIOR_IGUAL,
    TOKEN_IGUAL,
    TOKEN_DIFERENTE,
    TOKEN_VIRGULA,
    TOKEN_DIV,
    TOKEN_ABRE_PAR,
    TOKEN_FECHA_PAR,
    TOKEN_ATRIBUICAO,
    TOKEN_IDENTIFICADOR,
    TOKEN_PALAVRA_RESERVADA,
    TOKEN_FIM,
    TOKEN_ERRO
} TipoToken;

/* ========================= Estruturas ========================= */

typedef struct {
    TipoToken tipo;
    char *lexema;
    int linha;
    int coluna;
} Token;

typedef struct {
    const char *src;
    int i;
    int linha, coluna;
    char c;
} Scanner;

/* ========================= Protótipos usadas no Sintático ========================= */

// Funções principais que o sintático usa
void iniciar(Scanner *sc, const char *texto);
Token proximo_token(Scanner *sc);

// tabela de símbolos
void iniciar_tabela_simbolos();
void liberar_tabela_simbolos();

// utilidades
char *nome_token(TipoToken t);

extern FILE *output_file;

#endif
