#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexico.h"

Token tokenAtual;
Scanner S;

/* Erro sintático com formato exigido */
void erro_sintatico(const char *msg) {
    if (tokenAtual.tipo == TOKEN_FIM) {
        fprintf(stderr, "%d:fim de arquivo não esperado.\n", tokenAtual.linha);
    } else {
        fprintf(stderr, "%d:token nao esperado [%s].\n", tokenAtual.linha, tokenAtual.lexema);
    }
    exit(1);
}

void CasaToken(TipoToken esperado) {
    if (tokenAtual.tipo == esperado) {
        free(tokenAtual.lexema);
        tokenAtual = proximo_token(&S);
    } else {
        erro_sintatico("Token diferente do esperado.");
    }
}

/* Declarações dos não-terminais (assinaturas) */
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

/* Implementações (imprimem regra usada) */

void programa() {
    printf("Regra: programa -> program identificador ; bloco .\n");
    CasaToken(TOKEN_PALAVRA_RESERVADA); // program
    CasaToken(TOKEN_IDENTIFICADOR);
    CasaToken(TOKEN_PONTO_VIRGULA);
    bloco();
    CasaToken(TOKEN_PONTO);
}

void bloco() {
    printf("Regra: bloco -> parte_de_declaracoes_de_variaveis comando_composto\n");
    parte_decl_variaveis();
    comando_composto();
}

void parte_decl_variaveis() {
    printf("Regra: parte_de_declaracoes_de_variaveis\n");
    while (tokenAtual.tipo == TOKEN_PALAVRA_RESERVADA &&
           strcmp(tokenAtual.lexema, "var") == 0) {
        printf("Regra: bloco_var -> var <lista_declaracoes> ;\n");
        CasaToken(TOKEN_PALAVRA_RESERVADA); // var
        declaracao_variaveis();
        while (tokenAtual.tipo == TOKEN_PONTO_VIRGULA) {
            CasaToken(TOKEN_PONTO_VIRGULA);
            if (tokenAtual.tipo == TOKEN_IDENTIFICADOR) {
                declaracao_variaveis();
            } else {
                break;
            }
        }
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
    if (tokenAtual.tipo == TOKEN_PALAVRA_RESERVADA &&
        (strcmp(tokenAtual.lexema, "integer")==0 || strcmp(tokenAtual.lexema, "real")==0)) {
        CasaToken(TOKEN_PALAVRA_RESERVADA);
    } else {
        erro_sintatico("Esperado tipo integer ou real.");
    }
}

void comando_composto() {
    printf("Regra: comando_composto -> begin comando { ; comando } end\n");
    if (!(tokenAtual.tipo == TOKEN_PALAVRA_RESERVADA && strcmp(tokenAtual.lexema,"begin")==0))
        erro_sintatico("Esperado begin.");
    CasaToken(TOKEN_PALAVRA_RESERVADA);

    comando();

    while (tokenAtual.tipo == TOKEN_PONTO_VIRGULA) {
        CasaToken(TOKEN_PONTO_VIRGULA);
        
        if (tokenAtual.tipo == TOKEN_PALAVRA_RESERVADA && strcmp(tokenAtual.lexema, "end") == 0) {
            break; 
        }

        comando();
    }
    
    if (!(tokenAtual.tipo == TOKEN_PALAVRA_RESERVADA && strcmp(tokenAtual.lexema,"end")==0))
        erro_sintatico("Esperado end.");
        
    CasaToken(TOKEN_PALAVRA_RESERVADA);
}

void comando() {
    printf("Regra: comando\n");
    if (tokenAtual.tipo == TOKEN_IDENTIFICADOR) {
        atribuicao();
    } else if (tokenAtual.tipo == TOKEN_PALAVRA_RESERVADA && strcmp(tokenAtual.lexema,"begin")==0) {
        comando_composto();
    } else if (tokenAtual.tipo == TOKEN_PALAVRA_RESERVADA && strcmp(tokenAtual.lexema,"if")==0) {
        comando_condicional();
    } else if (tokenAtual.tipo == TOKEN_PALAVRA_RESERVADA && strcmp(tokenAtual.lexema,"while")==0) {
        comando_repetitivo();
    } else {
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
    if (tokenAtual.tipo == TOKEN_PALAVRA_RESERVADA && strcmp(tokenAtual.lexema, "else")==0) {
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
    if (tokenAtual.tipo == TOKEN_IGUAL || tokenAtual.tipo == TOKEN_MENOR || tokenAtual.tipo == TOKEN_MAIOR ||
        tokenAtual.tipo == TOKEN_MENOR_IGUAL || tokenAtual.tipo == TOKEN_MAIOR_IGUAL || tokenAtual.tipo == TOKEN_DIFERENTE) {
        CasaToken(tokenAtual.tipo);
        expressao_simples();
    }
}

void expressao_simples() {
    printf("Regra: expressao_simples\n");
    if (tokenAtual.tipo == TOKEN_MAIS || tokenAtual.tipo == TOKEN_MENOS) CasaToken(tokenAtual.tipo);
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
    if (tokenAtual.tipo == TOKEN_IDENTIFICADOR) CasaToken(TOKEN_IDENTIFICADOR);
    else if (tokenAtual.tipo == TOKEN_INTEIRO) CasaToken(TOKEN_INTEIRO);
    else if (tokenAtual.tipo == TOKEN_ABRE_PAR) {
        CasaToken(TOKEN_ABRE_PAR);
        expressao();
        CasaToken(TOKEN_FECHA_PAR);
    } else {
        erro_sintatico("Fator inválido.");
    }
}