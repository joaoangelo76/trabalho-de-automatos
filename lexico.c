// lexer_int_scanf_main.c - analisador léxico para inteiros e + - * / ( )
// Compilar: gcc -std=c11 -O2 lexer_int_scanf_main.c -o lexer_int
// Testar:   echo "12*(3+4) - 5/2" | ./lexer_int

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ========================= Definições ========================= */

typedef enum {
    TOKEN_INTEIRO,
    TOKEN_STRING,
    TOKEN_MAIS,      // +
    TOKEN_MENOS,     // -
    TOKEN_MULT,      // *
    TOKEN_DIV,       // /
    TOKEN_ABRE_PAR,  // (
    TOKEN_FECHA_PAR, // )
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

Token coletar_string(Scanner *sc);
Token coletar_comentario(Scanner *sc);
Token coletar_comentario2(Scanner *sc);

char* nome_token(TipoToken t){
    switch(t){
        case TOKEN_INTEIRO:   return "INTEIRO";
        case TOKEN_STRING:    return "STRING";
        case TOKEN_MAIS:      return "MAIS";
        case TOKEN_MENOS:     return "MENOS";
        case TOKEN_MULT:      return "MULT";
        case TOKEN_DIV:       return "DIV";
        case TOKEN_ABRE_PAR:  return "ABRE_PAR";
        case TOKEN_FECHA_PAR: return "FECHA_PAR";
        case TOKEN_FIM:       return "FIM";
        case TOKEN_ERRO:      return "ERRO";
        default:              return "?";
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
    char ch=sc->c; avancar(sc);
    return criar_token_texto(sc, tipo, &ch, 1, lin, col);
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

    if(sc->c == '{') return coletar_comentario(sc);

    if(sc->c == '(' && sc->src[sc->i+1] == '*') return coletar_comentario2(sc);

    if(sc->c == '\'') return coletar_string(sc);

    switch(sc->c){
        case '+': return token_simples(sc, TOKEN_MAIS);
        case '-': return token_simples(sc, TOKEN_MENOS);
        case '*': return token_simples(sc, TOKEN_MULT);
        case '/': return token_simples(sc, TOKEN_DIV);
        case '(': return token_simples(sc, TOKEN_ABRE_PAR);
        case ')': return token_simples(sc, TOKEN_FECHA_PAR);
        default: {
            int lin_erro = sc->linha;
            int col_erro = sc->coluna;
    
            char msg[64];
            snprintf(msg, sizeof(msg), "Caractere inválido: '%c'", sc->c);
    
            avancar(sc); 
    
            return criar_token_texto(sc, TOKEN_ERRO, msg, strlen(msg), lin_erro, col_erro);
        }   
    }
}

Token coletar_string(Scanner *sc){
    int lin = sc->linha;
    int col = sc->coluna;       // posição onde a ' foi encontrada (início da string)
    size_t inicio_quote = sc->i; // índice do ' inicial

    avancar(sc); // consumir a aspas inicial '

    // percorre até encontrar a aspas de fechamento, EOF ou newline
    while(sc->c != '\0' && sc->c != '\n'){
        if(sc->c == '\''){
            // caso Pascal: '' dentro da string significa uma aspa simples literal
            if(sc->src[sc->i + 1] == '\''){
                // consome as duas aspas e continua
                avancar(sc); // primeira '
                avancar(sc); // segunda '
                continue;
            } else {
                // aspas de fechamento encontrada
                // lexema sem as aspas: começa em inicio_quote+1 e tem comprimento sc->i - (inicio_quote+1)
                size_t len = sc->i - (inicio_quote + 1);
                Token t = criar_token_texto(sc, TOKEN_STRING, sc->src + inicio_quote + 1, len, lin, col);
                avancar(sc); // consumir a aspas de fechamento
                return t;
            }
        } else {
            avancar(sc);
        }
    }

    // saiu do loop por encontrar '\n' ou '\0' => string não fechada
    const char *msg = "String nao-fechada antes da quebra de linha/EOF";
    return criar_token_texto(sc, TOKEN_ERRO, msg, strlen(msg), lin, col);
}

Token coletar_comentario(Scanner *sc){
    int lin = sc->linha, col = sc->coluna;
    avancar(sc); // consumir o {
    while(sc->c != '}' && sc->c != '\0'){
        avancar(sc);
    }
    if(sc->c == '}'){
        avancar(sc); // consumir o }
        return proximo_token(sc); // ignora comentário, busca próximo
    } else {
        return token_erro_msg(sc, "Comentario nao-fechado antes do fim do arquivo");
    }
}

Token coletar_comentario2(Scanner *sc){
    int lin = sc->linha, col = sc->coluna;
    avancar(sc); // consumir (
    if(sc->c == '*'){
        avancar(sc); // consumir *
        while(!(sc->c == '*' && sc->src[sc->i+1] == ')') && sc->c != '\0'){
            avancar(sc);
        }
        if(sc->c == '\0'){
            return token_erro_msg(sc, "Comentario nao-fechado antes do fim do arquivo");
        }
        avancar(sc); // consumir *
        avancar(sc); // consumir )
        return proximo_token(sc);
    } else {
        return token_erro_msg(sc, "Caractere inválido: '(' esperado '*'");
    }
}

/* ========================= main ========================= */

int main(int argc, char *argv[]){
    char Entrada[1024];

    if(argc >= 2){
        // Usa argumento da linha de comando
        strncpy(Entrada, argv[1], sizeof(Entrada)-1);
        Entrada[sizeof(Entrada)-1] = '\0';
    } else {
        // Lê do teclado
        if(!fgets(Entrada, sizeof(Entrada), stdin)){
            fprintf(stderr, "Erro ao ler entrada.\n");
            return 1;
        }
        // Remove o \n no final, se existir
        Entrada[strcspn(Entrada, "\n")] = '\0';
    }

    Scanner S; 
    iniciar(&S, Entrada);

    for(;;){
        Token t = proximo_token(&S);
        printf("(%s, \"%s\") @ linha %d, col %d\n",
               nome_token(t.tipo), t.lexema, t.linha, t.coluna);
        free(t.lexema);
        
        // O loop agora só para no fim do arquivo
        if(t.tipo==TOKEN_FIM) break;
    }
    return 0;
}
