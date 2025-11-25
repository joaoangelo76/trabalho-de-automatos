#ifndef LEXICO_H
#define LEXICO_H

#include <stdio.h>

typedef enum {
    // Tipos Fundamentais de Literais
    TOKEN_NUMERO,       // Engloba INTEIRO e REAL (se o léxico não distinguir)
    TOKEN_IDENTIFICADOR,
    
    // Palavras Chave (Terminais da Gramática)
    TOKEN_PROGRAM, TOKEN_VAR, TOKEN_INTEGER, TOKEN_REAL,
    TOKEN_BEGIN, TOKEN_END, 
    TOKEN_IF, TOKEN_THEN, TOKEN_ELSE,
    TOKEN_WHILE, TOKEN_DO,
    
    // Símbolos e Operadores
    TOKEN_PONTO_VIRGULA,
    TOKEN_PONTO,
    TOKEN_DOIS_PONTOS,
    TOKEN_VIRGULA,
    TOKEN_ABRE_PAR,
    TOKEN_FECHA_PAR,

    TOKEN_ATRIBUICAO, // :=

    // Operadores Aritméticos e Relacionais (como terminais separados)
    TOKEN_MAIS,
    TOKEN_MENOS,
    TOKEN_MULT,
    TOKEN_DIV,
    
    // Operadores Relacionais explícitos (se o léxico consegue diferenciá-los de TOKEN_OP_REL)
    TOKEN_MENOR,
    TOKEN_MAIOR,
    TOKEN_MENOR_IGUAL,
    TOKEN_MAIOR_IGUAL,
    TOKEN_IGUAL,
    TOKEN_DIFERENTE,

    // Mantendo estes genéricos apenas para o léxico/parser se for estritamente necessário,
    // mas o parser deve preferir os explícitos acima.
    TOKEN_OP_REL, 
    
    TOKEN_FIM,
    TOKEN_ERRO
} TipoToken;

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

void iniciar(Scanner *sc, const char *texto);
Token proximo_token(Scanner *sc);

void iniciar_tabela_simbolos();
void liberar_tabela_simbolos();

const char *nome_token(TipoToken t);

extern FILE *output_file;

#endif