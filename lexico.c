// lexico.c - analisador léxico com Tabela de Símbolos, saída em arquivo e correção para FIM
// Compilar: gcc -std=c11 -O2 lexico.c -o lexico
// Testar:   echo "program teste; begin write('Olá mundo); end." > test.in && ./lexico test.in

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "lexico.h"


FILE *output_file;

/* ========================= Definições ========================= */

#define MAX_TS 100
typedef struct {
    char *lexema;
    TipoToken tipo;
} EntradaTS;

EntradaTS tabela_simbolos[MAX_TS];
int ts_count = 0;

/* protótipos */
Token coletar_identificador(Scanner *sc);
Token coletar_string(Scanner *sc);
Token coletar_comentario(Scanner *sc);
Token coletar_comentario2(Scanner *sc);
Token proximo_token(Scanner *sc);
void iniciar_tabela_simbolos();
EntradaTS* consultar_ts(const char *lexema);
void inserir_ts(const char *lexema, TipoToken tipo);
void liberar_tabela_simbolos();

char* nome_token(TipoToken t){
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
        case TOKEN_PALAVRA_RESERVADA: return "PALAVRA_RESERVADA";
        case TOKEN_FIM:             return "FIM";
        case TOKEN_ERRO:            return "ERRO";
        default:                    return "?";
    }
}

char *str_ndup(const char *s, size_t n){
    char *p = (char*)malloc(n+1);
    if(!p){ fprintf(stderr,"Memória insuficiente\n"); exit(1); }
    memcpy(p, s, n); p[n]='\0'; return p;
}

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

Token coletar_inteiro(Scanner *sc){
    int lin=sc->linha, col=sc->coluna;
    size_t ini = sc->i;
    if(!isdigit((unsigned char)sc->c)) return token_erro_msg(sc, "Inteiro malformado");
    while(isdigit((unsigned char)sc->c)) avancar(sc);
    return criar_token_texto(sc, TOKEN_INTEIRO, sc->src+ini, sc->i-ini, lin, col);
}

Token proximo_token(Scanner *sc){
    pular_espacos(sc);
    if(sc->c=='\0') return criar_token_texto(sc, TOKEN_FIM, "", 0, sc->linha, sc->coluna);

    if(sc->c == '\'') return coletar_string(sc);
    if(isdigit((unsigned char)sc->c)) return coletar_inteiro(sc);
    if(isalpha((unsigned char)sc->c)) return coletar_identificador(sc);
    if(sc->c == '{') return coletar_comentario(sc);
    if(sc->c == '(' && sc->src[sc->i+1] == '*') return coletar_comentario2(sc);

    // Operadores relacionais
    if(sc->c == '<'){
        int lin = sc->linha, col = sc->coluna;
        const char *p = sc->src + sc->i;
        avancar(sc);
        if(sc->c == '='){          // <=
            avancar(sc);
            return criar_token_texto(sc, TOKEN_MENOR_IGUAL, "<=", 2, lin, col);
        } else if(sc->c == '>'){   // <>
            avancar(sc);
            return criar_token_texto(sc, TOKEN_DIFERENTE, "<>", 2, lin, col);
        } else {                    // <
            return criar_token_texto(sc, TOKEN_MENOR, "<", 1, lin, col);
        }
    }

    if(sc->c == '>'){
        int lin = sc->linha, col = sc->coluna;
        const char *p = sc->src + sc->i;
        avancar(sc);
        if(sc->c == '='){          // >=
            avancar(sc);
            return criar_token_texto(sc, TOKEN_MAIOR_IGUAL, ">=", 2, lin, col);
        } else {                    // >
            return criar_token_texto(sc, TOKEN_MAIOR, ">", 1, lin, col);
        }
    }

    if(sc->c == '='){
        int lin = sc->linha, col = sc->coluna;
        avancar(sc);
        return criar_token_texto(sc, TOKEN_IGUAL, "=", 1, lin, col);
    }

    if(sc->c == ':' && sc->src[sc->i+1] == '='){
        int lin = sc->linha, col = sc->coluna;
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
        case ',': return token_simples(sc, TOKEN_VIRGULA);  // agora é token válido
        case '.': return token_simples(sc, TOKEN_PONTO);          // fim do programa
        case ':': {
            int lin = sc->linha, col = sc->coluna;
            avancar(sc);
            if(sc->c == '='){  // verifica se é ':='
                avancar(sc);
                return criar_token_texto(sc, TOKEN_ATRIBUICAO, ":=", 2, lin, col);
            } else {
                return criar_token_texto(sc, TOKEN_DOIS_PONTOS, ":", 1, lin, col);
            }
        }
        default: {
            int lin_erro = sc->linha;
            int col_erro = sc->coluna;
            char msg[64];
            snprintf(msg, sizeof(msg), "Caractere inválido: '%c'", sc->c);
            avancar(sc); // **CORREÇÃO:** Avance o cursor para que o loop principal continue a partir do próximo caractere.
            return criar_token_texto(sc, TOKEN_ERRO, msg, strlen(msg), lin_erro, col_erro);
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

EntradaTS* consultar_ts(const char *lexema){
    for(int i = 0; i < ts_count; i++){
        if(strcmp(tabela_simbolos[i].lexema, lexema) == 0){
            return &tabela_simbolos[i];
        }
    }
    return NULL;
}

void inserir_ts(const char *lexema, TipoToken tipo){
    if(ts_count >= MAX_TS){
        fprintf(stderr, "Tabela de simbolos cheia!\n");
        return;
    }
    tabela_simbolos[ts_count].lexema = str_ndup(lexema, strlen(lexema));
    tabela_simbolos[ts_count].tipo = tipo;
    ts_count++;
}

/* --- Coleta --- */
Token coletar_string(Scanner *sc){
    int lin = sc->linha;
    int col = sc->coluna;
    size_t inicio_quote = sc->i;
    avancar(sc); // pula o '

    while(sc->c != '\0' && sc->c != '\n'){
        if(sc->c == '\''){
            if(sc->src[sc->i + 1] == '\''){
                // escape '' -> representa ' em Pascal-like
                avancar(sc); // primeiro '
                avancar(sc); // segundo '
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

    // String não fechada -> retorna token de erro
    Token erro_token = criar_token_texto(sc, TOKEN_ERRO, 
        "String nao-fechada antes da quebra de linha/EOF", 
        strlen("String nao-fechada antes da quebra de linha/EOF"), lin, col);

    // Avança apenas caracteres que não iniciam outro token, mantendo a linha ativa
    while(sc->c != '\0' && sc->c != '\n'){
        if(sc->c == ';' || sc->c == '.' || sc->c == ':' || sc->c == ',' || 
           sc->c == '(' || sc->c == ')' || sc->c == '+' || sc->c == '-' ||
           sc->c == '*' || sc->c == '/' || sc->c == '<' || sc->c == '>' || 
           sc->c == '=' || sc->c == '\''){
            break; // encontramos um possível token -> pare aqui
        }
        avancar(sc);
    }

    return erro_token;
}

Token coletar_identificador(Scanner *sc){
    int lin = sc->linha, col = sc->coluna;
    size_t ini = sc->i;
    if(!isalpha((unsigned char)sc->c)) return token_erro_msg(sc, "Identificador malformado");
    while(isalnum((unsigned char)sc->c)) avancar(sc);

    char *lexema_temp = str_ndup(sc->src + ini, sc->i - ini);
    EntradaTS* entrada_existente = consultar_ts(lexema_temp);

    TipoToken tipo_token;
    if(entrada_existente != NULL){
        tipo_token = entrada_existente->tipo;
        if (tipo_token == TOKEN_PALAVRA_RESERVADA) {
            fprintf(output_file, "AVISO: Palavra reservada '%s' encontrada novamente.\n", lexema_temp);
        } else {
            fprintf(output_file, "AVISO: Identificador '%s' ja foi cadastrado.\n", lexema_temp);
        }
        free(lexema_temp);
        // Retorna um novo token usando o lexema da tabela de símbolos para evitar duplicação.
        return criar_token_texto(sc, tipo_token, entrada_existente->lexema, strlen(entrada_existente->lexema), lin, col);
    } else {
        tipo_token = TOKEN_IDENTIFICADOR;
        inserir_ts(lexema_temp, tipo_token);
        fprintf(output_file, "INFO: Novo identificador '%s' cadastrado.\n", lexema_temp);
        return criar_token_texto(sc, tipo_token, lexema_temp, strlen(lexema_temp), lin, col);
    }
}

Token coletar_comentario(Scanner *sc){
    int lin = sc->linha, col = sc->coluna;
    avancar(sc); // pula '{'

    while(sc->c != '\0' && sc->c != '}'){
        avancar(sc);
    }

    if(sc->c == '}'){
        avancar(sc); // pula '}'
        return proximo_token(sc); // comentário fechado, continua normalmente
    }

    // Comentário não fechado -> token de erro
    Token erro_token = criar_token_texto(sc, TOKEN_ERRO, 
        "Comentario nao-fechado antes do fim do arquivo", 
        strlen("Comentario nao-fechado antes do fim do arquivo"), lin, col);

    // Avança até próximo caractere seguro, sem consumir tokens válidos
    while(sc->c != '\0' && sc->c != '\n'){
        if(sc->c == ';' || sc->c == '.' || sc->c == ':' || sc->c == ',' || 
           sc->c == '(' || sc->c == ')' || sc->c == '+' || sc->c == '-' ||
           sc->c == '*' || sc->c == '/' || sc->c == '<' || sc->c == '>' || 
           sc->c == '=' || sc->c == '\''){
            break; // possível token -> pare aqui
        }
        avancar(sc);
    }

    return erro_token;
}

Token coletar_comentario2(Scanner *sc){
    int lin = sc->linha, col = sc->coluna;
    avancar(sc); // pula '('
    if(sc->c == '*'){
        avancar(sc); // pula '*'
        while(!(sc->c == '*' && sc->src[sc->i+1] == ')') && sc->c != '\0'){
            avancar(sc);
        }
        if(sc->c == '\0'){
            Token erro_token = criar_token_texto(sc, TOKEN_ERRO, 
                "Comentario nao-fechado antes do fim do arquivo", 
                strlen("Comentario nao-fechado antes do fim do arquivo"), lin, col);
            
            // Avança até próximo caractere seguro
            while(sc->c != '\0' && sc->c != '\n'){
                if(sc->c == ';' || sc->c == '.' || sc->c == ':' || sc->c == ',' || 
                   sc->c == '(' || sc->c == ')' || sc->c == '+' || sc->c == '-' ||
                   sc->c == '*' || sc->c == '/' || sc->c == '<' || sc->c == '>' || 
                   sc->c == '=' || sc->c == '\''){
                    break;
                }
                avancar(sc);
            }

            return erro_token;
        }
        avancar(sc); // pula '*'
        avancar(sc); // pula ')'
        return proximo_token(sc);
    } else {
        return criar_token_texto(sc, TOKEN_ERRO, 
            "Caractere inválido: '(' esperado '*'", 
            strlen("Caractere inválido: '(' esperado '*'"), lin, col);
    }
}

void liberar_tabela_simbolos(){
    for(int i = 0; i < ts_count; i++){
        free(tabela_simbolos[i].lexema);
        tabela_simbolos[i].lexema = NULL;
    }
    ts_count = 0;
}