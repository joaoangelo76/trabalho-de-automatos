#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

/* ========================= Definições ========================= */

typedef enum {
    TOKEN_INTEIRO,
    TOKEN_STRING,
    TOKEN_MAIS,
    TOKEN_MENOS,
    TOKEN_MULT,
    TOKEN_DIV,
    TOKEN_ABRE_PAR,
    TOKEN_FECHA_PAR,
    TOKEN_ATRIBUICAO,
    TOKEN_IDENTIFICADOR,
    TOKEN_PALAVRA_RESERVADA,
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
    size_t i;
    int linha, coluna;
    char c;
} Scanner;

#define MAX_TS 100
typedef struct {
    char *lexema;
    TipoToken tipo;
} EntradaTS;

EntradaTS tabela_simbolos[MAX_TS];
int ts_count = 0;
FILE *output_file;

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

const char* nome_token(TipoToken t){
    switch(t){
        case TOKEN_INTEIRO:         return "INTEIRO";
        case TOKEN_STRING:          return "STRING";
        case TOKEN_MAIS:            return "MAIS";
        case TOKEN_MENOS:           return "MENOS";
        case TOKEN_MULT:            return "MULT";
        case TOKEN_DIV:             return "DIV";
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
        case ';': {
            int lin=sc->linha, col=sc->coluna;
            avancar(sc);
            return criar_token_texto(sc, TOKEN_ERRO, "Caractere inválido: ';'", strlen("Caractere inválido: ';'"), lin, col);
        }
        case '.': {
            int lin=sc->linha, col=sc->coluna;
            avancar(sc);
            return criar_token_texto(sc, TOKEN_ERRO, "Caractere inválido: '.'", strlen("Caractere inválido: '.'"), lin, col);
        }
        case ':': {
            int lin=sc->linha, col=sc->coluna;
            avancar(sc);
            return criar_token_texto(sc, TOKEN_ERRO, "Caractere inválido: ':'", strlen("Caractere inválido: ':'"), lin, col);
        }
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

/* --- Tabela de símbolos --- */
void iniciar_tabela_simbolos(){
    ts_count = 0;
    inserir_ts("program", TOKEN_PALAVRA_RESERVADA);
    inserir_ts("var", TOKEN_PALAVRA_RESERVADA);
    inserir_ts("begin", TOKEN_PALAVRA_RESERVADA);
    inserir_ts("end", TOKEN_PALAVRA_RESERVADA);
    inserir_ts("read", TOKEN_PALAVRA_RESERVADA);
    inserir_ts("write", TOKEN_PALAVRA_RESERVADA);
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
    return criar_token_texto(sc, TOKEN_ERRO, "String nao-fechada antes da quebra de linha/EOF", strlen("String nao-fechada antes da quebra de linha/EOF"), lin, col);
}

Token coletar_identificador(Scanner *sc){
    int lin = sc->linha, col = sc->coluna;
    size_t ini = sc->i;
    if(!isalpha((unsigned char)sc->c)) return token_erro_msg(sc, "Identificador malformado");
    while(isalnum((unsigned char)sc->c)) avancar(sc);

    char *lexema_token = str_ndup(sc->src + ini, sc->i - ini);
    EntradaTS* entrada_existente = consultar_ts(lexema_token);

    TipoToken tipo_token;
    if(entrada_existente != NULL){
        tipo_token = entrada_existente->tipo;
        if(tipo_token == TOKEN_PALAVRA_RESERVADA){
            fprintf(output_file, "AVISO: Palavra reservada '%s' encontrada novamente.\n", lexema_token);
        } else {
            fprintf(output_file, "AVISO: Identificador '%s' ja foi cadastrado.\n", lexema_token);
        }
    } else {
        tipo_token = TOKEN_IDENTIFICADOR;
        inserir_ts(lexema_token, tipo_token);
        fprintf(output_file, "INFO: Novo identificador '%s' cadastrado.\n", lexema_token);
    }
    free(lexema_token);

    return criar_token_texto(sc, tipo_token, sc->src + ini, sc->i - ini, lin, col);
}

Token coletar_comentario(Scanner *sc){
    int lin = sc->linha, col = sc->coluna;
    avancar(sc); // pula '{'
    while(sc->c != '}' && sc->c != '\0'){
        avancar(sc);
    }
    if(sc->c == '}'){
        avancar(sc); // pula '}'
        return proximo_token(sc);
    } else {
        return criar_token_texto(sc, TOKEN_ERRO, "Comentario nao-fechado antes do fim do arquivo", strlen("Comentario nao-fechado antes do fim do arquivo"), lin, col);
    }
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
            return criar_token_texto(sc, TOKEN_ERRO, "Comentario nao-fechado antes do fim do arquivo", strlen("Comentario nao-fechado antes do fim do arquivo"), lin, col);
        }
        avancar(sc); // pula '*'
        avancar(sc); // pula ')'
        return proximo_token(sc);
    } else {
        return criar_token_texto(sc, TOKEN_ERRO, "Caractere inválido: '(' esperado '*'", strlen("Caractere inválido: '(' esperado '*'"), lin, col);
    }
}

void liberar_tabela_simbolos(){
    for(int i = 0; i < ts_count; i++){
        free(tabela_simbolos[i].lexema);
        tabela_simbolos[i].lexema = NULL;
    }
    ts_count = 0;
}

/* ========================= main ========================= */

int main(int argc, char *argv[]){
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
    } else {
        size_t buffer_size = 1024;
        Entrada = malloc(buffer_size);
        if(!Entrada){ fprintf(stderr,"Erro de alocacao\n"); return 1; }
        Entrada[0] = '\0';

        if (isatty(STDIN_FILENO)) {
            printf("Digite a expressao (CTRL+D para terminar):\n");
        }

        while(!feof(stdin)){
            if(fgets(Entrada + total_lido, (int)(buffer_size - total_lido), stdin) == NULL) break;
            total_lido += strlen(Entrada + total_lido);
            if (total_lido + 1 >= buffer_size){
                buffer_size *= 2;
                Entrada = realloc(Entrada, buffer_size);
                if(!Entrada){ fprintf(stderr, "Erro de alocacao de memoria.\n"); return 1; }
            }
        }
        if (total_lido > 0 && (Entrada[total_lido-1] == '\n' || Entrada[total_lido-1] == '\r')){
            Entrada[total_lido-1] = '\0';
            total_lido--;
        }

        output_file = fopen("output.lex", "w");
        if(!output_file){
            fprintf(stderr, "Erro ao criar o arquivo de saída: output.lex\n");
            free(Entrada);
            return 1;
        }
    }

    iniciar_tabela_simbolos();

    Scanner S;
    iniciar(&S, Entrada);

    for(;;){
        Token t = proximo_token(&S);
        fprintf(output_file, "<%s, \"%s\"> @ linha %d, coluna %d\n",
                nome_token(t.tipo), t.lexema, t.linha, t.coluna);
        printf("<%s, \"%s\"> @ linha %d, coluna %d\n",
                nome_token(t.tipo), t.lexema, t.linha, t.coluna);
        free(t.lexema);
        if(t.tipo==TOKEN_FIM) break;
    }

    fprintf(output_file, "\n--- Tabela de Símbolos ---\n");
    for(int i = 0; i < ts_count; i++){
        fprintf(output_file, " [%d] -> <%s, \"%s\">\n", i, nome_token(tabela_simbolos[i].tipo), tabela_simbolos[i].lexema);
    }
    fprintf(output_file, "--- Fim da Tabela de Símbolos ---\n");

    fclose(output_file);
    free(Entrada);
    liberar_tabela_simbolos();

    return 0;
}
