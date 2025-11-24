#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexico.h"

void inserir_ts(const char *lexema, TipoToken tipo);

FILE *output_file = NULL;

/* ========================= Definições ========================= */

#define MAX_TS 100
typedef struct {
    char *lexema;
    TipoToken tipo;
} EntradaTS;

static EntradaTS tabela_simbolos[MAX_TS];
static int ts_count = 0;

/* utilitários */
static char *str_ndup(const char *s, size_t n){
    char *p = (char*)malloc(n+1);
    if(!p){ fprintf(stderr,"Memória insuficiente\n"); exit(1); }
    memcpy(p, s, n); p[n]='\0'; return p;
}

static char *to_lower_copy(const char *s, size_t n){
    char *p = (char*)malloc(n+1);
    if(!p){ fprintf(stderr,"Memória insuficiente\n"); exit(1); }
    for(size_t i=0;i<n;i++) p[i] = (char)tolower((unsigned char)s[i]);
    p[n]='\0';
    return p;
}

/* ---------------- scanner helpers ---------------- */

void iniciar(Scanner *sc, const char *texto){
    sc->src = texto ? texto : "";
    sc->i = 0; sc->linha = 1; sc->coluna = 1;
    sc->c = sc->src[0];
}

void avancar(Scanner *sc){
    if(sc->c=='\0') return;
    if(sc->c=='\n'){ sc->linha++; sc->coluna=1; }
    else           { sc->coluna++; }
    sc->i++; sc->c = sc->src[sc->i];
}

void pular_espacos(Scanner *sc){
    while(isspace((unsigned char)sc->c)) avancar(sc);
}

Token criar_token_texto(Scanner *sc, TipoToken tipo, const char *ini, size_t n, int lin, int col){
    (void)sc;
    Token t; t.tipo=tipo; t.lexema=str_ndup(ini,n); t.linha=lin; t.coluna=col; return t;
}

Token token_simples(Scanner *sc, TipoToken tipo){
    int lin=sc->linha, col=sc->coluna;
    const char *p = sc->src + sc->i;
    Token t = criar_token_texto(sc, tipo, p, 1, lin, col);
    avancar(sc);
    return t;
}

Token token_erro_msg(Scanner *sc, const char *msg){
    return criar_token_texto(sc, TOKEN_ERRO, msg, strlen(msg), sc->linha, sc->coluna);
}

/* --- coletores --- */

Token coletar_inteiro(Scanner *sc){
    int lin=sc->linha, col=sc->coluna;
    size_t ini = sc->i;
    if(!isdigit((unsigned char)sc->c)) return token_erro_msg(sc, "Inteiro malformado");
    while(isdigit((unsigned char)sc->c)) avancar(sc);
    return criar_token_texto(sc, TOKEN_INTEIRO, sc->src+ini, sc->i-ini, lin, col);
}

Token coletar_string(Scanner *sc){
    int lin = sc->linha, col = sc->coluna;
    size_t inicio_quote = sc->i;
    avancar(sc); // pula o '

    while(sc->c != '\0' && sc->c != '\n'){
        if(sc->c == '\''){
            if(sc->src[sc->i + 1] == '\''){
                // escape '' -> '
                avancar(sc); avancar(sc);
                continue;
            } else {
                size_t len = sc->i - (inicio_quote + 1);
                Token t = criar_token_texto(sc, TOKEN_STRING, sc->src + inicio_quote + 1, len, lin, col);
                avancar(sc); // pula o fechamento '
                return t;
            }
        } else {
            avancar(sc);
        }
    }

    return token_erro_msg(sc, "String nao-fechada antes da quebra de linha/EOF");
}

Token coletar_identificador(Scanner *sc){
    int lin = sc->linha, col = sc->coluna;
    size_t ini = sc->i;
    if(!isalpha((unsigned char)sc->c)) return token_erro_msg(sc, "Identificador malformado");
    while(isalnum((unsigned char)sc->c)) avancar(sc);

    size_t len = sc->i - ini;
    char *lexema_orig = str_ndup(sc->src + ini, len);         // original
    char *lexema_lower = to_lower_copy(sc->src + ini, len);   // lookup

    // consultar TS
    for(int i=0;i<ts_count;i++){
        if(strcmp(tabela_simbolos[i].lexema, lexema_lower)==0){
            TipoToken tp = tabela_simbolos[i].tipo;
            if (output_file) {
                if(tp==TOKEN_PALAVRA_RESERVADA) fprintf(output_file, "AVISO: Palavra reservada '%s' encontrada.\n", lexema_lower);
                else fprintf(output_file, "AVISO: Identificador '%s' ja foi cadastrado.\n", lexema_lower);
            }
            Token t = criar_token_texto(sc, tp, tabela_simbolos[i].lexema, strlen(tabela_simbolos[i].lexema), lin, col);
            free(lexema_orig);
            free(lexema_lower);
            return t;
        }
    }

    // novo identificador
    if(ts_count < MAX_TS){
        tabela_simbolos[ts_count].lexema = str_ndup(lexema_lower, strlen(lexema_lower));
        tabela_simbolos[ts_count].tipo = TOKEN_IDENTIFICADOR;
        ts_count++;
        if(output_file) fprintf(output_file, "INFO: Novo identificador '%s' cadastrado.\n", lexema_lower);
    } else {
        if(output_file) fprintf(output_file, "ERRO: Tabela de simbolos cheia ao inserir '%s'\n", lexema_lower);
    }

    Token t = criar_token_texto(sc, TOKEN_IDENTIFICADOR, lexema_lower, strlen(lexema_lower), lin, col);
    free(lexema_orig);
    free(lexema_lower);
    return t;
}

Token coletar_comentario(Scanner *sc){
    /* comentários não geram token; pulamos e solicitamos próximo token */
    avancar(sc); // pula '{'
    while(sc->c != '\0' && sc->c != '}') avancar(sc);
    if(sc->c == '}'){ avancar(sc); return proximo_token(sc); }
    return token_erro_msg(sc, "Comentario nao-fechado antes do fim do arquivo");
}

Token coletar_comentario2(Scanner *sc){
    avancar(sc); // '('
    if(sc->c == '*'){
        avancar(sc);
        while(!(sc->c == '*' && sc->src[sc->i+1] == ')') && sc->c != '\0') avancar(sc);
        if(sc->c == '\0') return token_erro_msg(sc, "Comentario nao-fechado antes do fim do arquivo");
        avancar(sc); avancar(sc); // skip "*)"
        return proximo_token(sc);
    } else {
        return token_erro_msg(sc, "Caractere inválido: '(' esperado '*'");
    }
}

Token proximo_token(Scanner *sc){
    pular_espacos(sc);
    if(sc->c=='\0') return criar_token_texto(sc, TOKEN_FIM, "", 0, sc->linha, sc->coluna);

    if(sc->c == '\'') return coletar_string(sc);
    if(isdigit((unsigned char)sc->c)) return coletar_inteiro(sc);
    if(isalpha((unsigned char)sc->c)) return coletar_identificador(sc);
    if(sc->c == '{') return coletar_comentario(sc);
    if(sc->c == '(' && sc->src[sc->i+1] == '*') return coletar_comentario2(sc);

    /* relacionais */
    if(sc->c == '<'){
        int lin=sc->linha, col=sc->coluna;
        avancar(sc);
        if(sc->c == '='){ avancar(sc); return criar_token_texto(sc, TOKEN_MENOR_IGUAL, "<=", 2, lin, col); }
        else if(sc->c == '>'){ avancar(sc); return criar_token_texto(sc, TOKEN_DIFERENTE, "<>", 2, lin, col); }
        else return criar_token_texto(sc, TOKEN_MENOR, "<", 1, lin, col);
    }

    if(sc->c == '>'){
        int lin=sc->linha, col=sc->coluna;
        avancar(sc);
        if(sc->c == '='){ avancar(sc); return criar_token_texto(sc, TOKEN_MAIOR_IGUAL, ">=", 2, lin, col); }
        else return criar_token_texto(sc, TOKEN_MAIOR, ">", 1, lin, col);
    }

    if(sc->c == '='){
        int lin=sc->linha, col=sc->coluna;
        avancar(sc);
        return criar_token_texto(sc, TOKEN_IGUAL, "=", 1, lin, col);
    }

    if(sc->c == ':' && sc->src[sc->i+1] == '='){
        int lin=sc->linha, col=sc->coluna;
        const char *p = sc->src + sc->i;
        avancar(sc); avancar(sc);
        return criar_token_texto(sc, TOKEN_ATRIBUICAO, p, 2, lin, col);
    }

    switch(sc->c){
        case '+': return token_simples(sc, TOKEN_MAIS);
        case '-': return token_simples(sc, TOKEN_MENOS);
        case '*': return token_simples(sc, TOKEN_MULT);
        case '/': return token_simples(sc, TOKEN_DIV);
        case '(': return token_simples(sc, TOKEN_ABRE_PAR);
        case ')': return token_simples(sc, TOKEN_FECHA_PAR);
        case ';': return token_simples(sc, TOKEN_PONTO_VIRGULA);
        case ',': return token_simples(sc, TOKEN_VIRGULA);
        case '.': return token_simples(sc, TOKEN_PONTO);
        case ':': {
            int lin=sc->linha, col=sc->coluna;
            avancar(sc);
            if(sc->c == '='){ avancar(sc); return criar_token_texto(sc, TOKEN_ATRIBUICAO, ":=", 2, lin, col); }
            else return criar_token_texto(sc, TOKEN_DOIS_PONTOS, ":", 1, lin, col);
        }
        default: {
            int lin=sc->linha, col=sc->coluna;
            char msg[64];
            snprintf(msg, sizeof(msg), "Caractere inválido: '%c'", sc->c);
            avancar(sc);
            return criar_token_texto(sc, TOKEN_ERRO, msg, strlen(msg), lin, col);
        }
    }
}

/* --- Tabela de símbolos --- */
void iniciar_tabela_simbolos(){
    ts_count = 0;
    inserir_ts("program", TOKEN_PALAVRA_RESERVADA);
    inserir_ts("var", TOKEN_PALAVRA_RESERVADA);
    inserir_ts("integer", TOKEN_PALAVRA_RESERVADA);
    inserir_ts("real", TOKEN_PALAVRA_RESERVADA);
    inserir_ts("begin", TOKEN_PALAVRA_RESERVADA);
    inserir_ts("end", TOKEN_PALAVRA_RESERVADA);
    inserir_ts("if", TOKEN_PALAVRA_RESERVADA);
    inserir_ts("then", TOKEN_PALAVRA_RESERVADA);
    inserir_ts("else", TOKEN_PALAVRA_RESERVADA);
    inserir_ts("while", TOKEN_PALAVRA_RESERVADA);
    inserir_ts("do", TOKEN_PALAVRA_RESERVADA);
}

void inserir_ts(const char *lexema, TipoToken tipo){
    if(ts_count >= MAX_TS){
        if(output_file) fprintf(output_file, "ERRO: Tabela de simbolos cheia ao inserir '%s'\n", lexema);
        else fprintf(stderr, "Tabela de simbolos cheia ao inserir '%s'\n", lexema);
        return;
    }
    tabela_simbolos[ts_count].lexema = str_ndup(lexema, strlen(lexema));
    tabela_simbolos[ts_count].tipo = tipo;
    ts_count++;
}

EntradaTS* consultar_ts(const char *lexema){
    for(int i = 0; i < ts_count; i++){
        if(strcmp(tabela_simbolos[i].lexema, lexema) == 0){
            return &tabela_simbolos[i];
        }
    }
    return NULL;
}

void liberar_tabela_simbolos(){
    for(int i = 0; i < ts_count; i++){
        free(tabela_simbolos[i].lexema);
        tabela_simbolos[i].lexema = NULL;
    }
    ts_count = 0;
}

/* --- Nome de token (utilitário para impressão) --- */
char *nome_token(TipoToken t){
    switch(t){
        case TOKEN_INTEIRO:         return "INTEIRO";
        case TOKEN_STRING:          return "STRING";
        case TOKEN_MAIS:            return "MAIS";
        case TOKEN_MENOS:           return "MENOS";
        case TOKEN_MULT:            return "MULT";
        case TOKEN_DIV:             return "DIV";
        case TOKEN_PONTO_VIRGULA:   return "PONTO_VIRGULA";
        case TOKEN_MENOR:           return "MENOR";
        case TOKEN_MAIOR:           return "MAIOR";
        case TOKEN_MENOR_IGUAL:     return "MENOR_IGUAL";
        case TOKEN_MAIOR_IGUAL:     return "MAIOR_IGUAL";
        case TOKEN_IGUAL:           return "IGUAL";
        case TOKEN_DIFERENTE:       return "DIFERENTE";
        case TOKEN_VIRGULA:         return "VIRGULA";
        case TOKEN_PONTO:           return "PONTO";
        case TOKEN_DOIS_PONTOS:     return "DOIS_PONTO";
        case TOKEN_ABRE_PAR:        return "ABRE_PAR";
        case TOKEN_FECHA_PAR:       return "FECHA_PAR";
        case TOKEN_ATRIBUICAO:      return "ATRIBUICAO";
        case TOKEN_IDENTIFICADOR:   return "IDENTIFICADOR";
        case TOKEN_PALAVRA_RESERVADA:return "PALAVRA_RESERVADA";
        case TOKEN_FIM:             return "FIM";
        case TOKEN_ERRO:            return "ERRO";
        default:                    return "?";
    }
}