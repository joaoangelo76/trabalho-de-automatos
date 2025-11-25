// analisador.c - Lexer + Parser unificado para MicroPascal (Corrigido com Modo Pânico)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Inclua o cabeçalho (assumindo que você o salvou como lexico.h)
#include "lexico.h"

/* ==================== Variáveis globais ==================== */
Token tokenAtual;
Scanner S; // A variável Scanner global é 'S'

// Variáveis de controle de erro
int erro_encontrado = 0; // Flag: 1 se um erro sintático ocorreu em qualquer ponto
int erro_fatal = 0;     // Flag: 1 se o processo deve terminar com erro

const char* nome_token(TipoToken tipo) {
    switch (tipo) {
        case TOKEN_PROGRAM: return "program";
        case TOKEN_IDENTIFICADOR: return "identificador";
        case TOKEN_PONTO_VIRGULA: return ";";
        case TOKEN_PONTO: return ".";
        case TOKEN_FIM: return "fim de arquivo";
        case TOKEN_BEGIN: return "begin";
        case TOKEN_END: return "end";
        case TOKEN_ATRIBUICAO: return ":=";
        case TOKEN_VAR: return "var";
        case TOKEN_INTEGER: return "integer";
        case TOKEN_REAL: return "real";
        case TOKEN_IF: return "if";
        case TOKEN_THEN: return "then";
        case TOKEN_ELSE: return "else";
        case TOKEN_WHILE: return "while";
        case TOKEN_DO: return "do";
        case TOKEN_DOIS_PONTOS: return ":";
        case TOKEN_VIRGULA: return ",";
        case TOKEN_NUMERO: return "numero";
        // Adicione todos os seus tokens aqui
        default: return "TOKEN DESCONHECIDO"; 
    }
}

/* -------------------- Derivação -------------------- */
static char **derivacoes = NULL;
static int deriv_count = 0;
static int deriv_cap = 0;

static void record_rule(const char *s) {
    if (deriv_count + 1 > deriv_cap) {
        deriv_cap = deriv_cap == 0 ? 64 : deriv_cap * 2;
        derivacoes = realloc(derivacoes, deriv_cap * sizeof(char*));
        if (!derivacoes) { fprintf(stderr,"Memória insuficiente\n"); exit(1); }
    }
    derivacoes[deriv_count++] = strdup(s);
}

void imprimir_derivacao(void) {
    if (!erro_fatal) {
        for (int i = 0; i < deriv_count; ++i) {
            printf("Regra: %s\n", derivacoes[i]);
            free(derivacoes[i]);
        }
    } else {
        for (int i = 0; i < deriv_count; ++i) {
            free(derivacoes[i]);
        }
    }
    free(derivacoes);
    derivacoes = NULL;
    deriv_count = deriv_cap = 0;
}

/* ==================== Funções Auxiliares do Lexer (Corrigidas) ==================== */
// Corrigidas para usar APENAS o parâmetro 'sc' e os nomes de campo definidos em lexico.h (src, i)

void iniciar(Scanner *sc, const char *texto) {
    // CORREÇÃO: Usa 'sc' para inicializar o scanner (não toca na global 'S')
    sc->src = texto;
    sc->i = 0;
    sc->linha = 1;
    sc->coluna = 1;
    sc->c = texto[0];
}

void avancar(Scanner *sc) {
    // CORREÇÃO: Usa 'sc' para avançar
    if (sc->c == '\n') { 
        sc->linha++; 
        sc->coluna = 1; 
    }
    else sc->coluna++;
    sc->i++; // Usa 'i' para a posição, conforme sua struct
    sc->c = sc->src[sc->i]; // Usa 'src' para o código-fonte
}

void pular_espacos(Scanner *sc) {
    while (isspace(sc->c)) avancar(sc);
}

// Implementação de criar_token (crucial para o retorno correto)
Token criar_token(TipoToken tipo, const char *lexema, int linha, int coluna) {
    Token t;
    t.tipo = tipo;
    // Lógica para lidar com lexema de erro de 1 caractere
    if (tipo == TOKEN_ERRO && lexema != NULL && strlen(lexema) == 1) {
        char *temp_lexema = malloc(2);
        temp_lexema[0] = lexema[0];
        temp_lexema[1] = '\0';
        t.lexema = temp_lexema;
    } else {
        t.lexema = strdup(lexema);
    }
    t.linha = linha;
    t.coluna = coluna;
    return t;
}


/* ==================== Função Principal do Lexer (Corrigida) ==================== */
Token proximo_token(Scanner *sc) {
    pular_espacos(sc);

    int linha = sc->linha;
    int coluna = sc->coluna;
    
    // Se for FIM
    if (sc->c == '\0') return criar_token(TOKEN_FIM, "", linha, coluna);

    // Identificadores e palavras reservadas
    if (isalpha(sc->c)) {
        char buffer[128]; int i = 0;
        while (isalnum(sc->c)) buffer[i++] = sc->c, avancar(sc);
        buffer[i] = '\0';

        if (strcmp(buffer,"program")==0) return criar_token(TOKEN_PROGRAM, buffer, linha, coluna);
        if (strcmp(buffer,"begin")==0)   return criar_token(TOKEN_BEGIN, buffer, linha, coluna);
        if (strcmp(buffer,"end")==0)     return criar_token(TOKEN_END, buffer, linha, coluna);
        if (strcmp(buffer,"var")==0)     return criar_token(TOKEN_VAR, buffer, linha, coluna);
        if (strcmp(buffer,"integer")==0) return criar_token(TOKEN_INTEGER, buffer, linha, coluna);
        if (strcmp(buffer,"real")==0)    return criar_token(TOKEN_REAL, buffer, linha, coluna);
        if (strcmp(buffer,"if")==0)      return criar_token(TOKEN_IF, buffer, linha, coluna);
        if (strcmp(buffer,"then")==0)    return criar_token(TOKEN_THEN, buffer, linha, coluna);
        if (strcmp(buffer,"else")==0)    return criar_token(TOKEN_ELSE, buffer, linha, coluna);
        if (strcmp(buffer,"while")==0)   return criar_token(TOKEN_WHILE, buffer, linha, coluna);
        if (strcmp(buffer,"do")==0)      return criar_token(TOKEN_DO, buffer, linha, coluna);

        return criar_token(TOKEN_IDENTIFICADOR, buffer, linha, coluna);
    }

    // Números
    if (isdigit(sc->c)) {
        char buffer[64]; int i = 0;
        while (isdigit(sc->c) || sc->c == '.') buffer[i++] = sc->c, avancar(sc);
        buffer[i] = '\0';
        return criar_token(TOKEN_NUMERO, buffer, linha, coluna);
    }
    
    // Operadores Relacionais
    if (strchr("<>=", sc->c)) {
        char c1 = sc->c; 
        avancar(sc); // Consome o primeiro caractere

        // Verifica operadores de dois caracteres e retorna o de um caractere se não houver segundo
        if (c1 == '<') {
            if (sc->c == '=') { avancar(sc); return criar_token(TOKEN_MENOR_IGUAL, "<=", linha, coluna); }
            if (sc->c == '>') { avancar(sc); return criar_token(TOKEN_DIFERENTE, "<>", linha, coluna); }
            return criar_token(TOKEN_MENOR, "<", linha, coluna); // RETORNO para '<' simples
        } 
        if (c1 == '>') {
            if (sc->c == '=') { avancar(sc); return criar_token(TOKEN_MAIOR_IGUAL, ">=", linha, coluna); }
            return criar_token(TOKEN_MAIOR, ">", linha, coluna); // RETORNO para '>' simples
        } 
        if (c1 == '=') {
            return criar_token(TOKEN_IGUAL, "=", linha, coluna); // RETORNO para '='
        }
    }

    // Armazena o caractere para o switch/erro ANTES de avançar
    char c_atual = sc->c; 
    avancar(sc);

    // Operadores e símbolos simples
    switch(c_atual){
        case ';': return criar_token(TOKEN_PONTO_VIRGULA,";",linha,coluna);
        case '.': return criar_token(TOKEN_PONTO,".",linha,coluna);
        case ',': return criar_token(TOKEN_VIRGULA,",",linha,coluna);
        case ':':
            // O caracter ':' já foi consumido acima, apenas verifica o '='
            if(sc->c=='='){ avancar(sc); return criar_token(TOKEN_ATRIBUICAO,":=",linha,coluna);}
            return criar_token(TOKEN_DOIS_PONTOS,":",linha,coluna);
        case '(': return criar_token(TOKEN_ABRE_PAR,"(",linha,coluna);
        case ')': return criar_token(TOKEN_FECHA_PAR,")",linha,coluna);
        case '+': return criar_token(TOKEN_MAIS,"+",linha,coluna);
        case '-': return criar_token(TOKEN_MENOS,"-",linha,coluna);
        case '*': return criar_token(TOKEN_MULT,"*",linha,coluna);
        case '/': return criar_token(TOKEN_DIV,"/",linha,coluna);
    }

    // Se o switch falhou, criamos o TOKEN_ERRO.
    return criar_token(TOKEN_ERRO, &c_atual, linha, coluna); 
}

/* ==================== Parser / Sintático (Com Recuperação) ==================== */
void ConsomeLexemaAtual() { free(tokenAtual.lexema); tokenAtual = proximo_token(&S); }

void erro_sintatico(const char *msg) {
    // Seta o flag de erro (necessário para o Modo Pânico/recuperação)
    erro_encontrado = 1;

    // nn:token nao esperado [lex].
    if (tokenAtual.tipo != TOKEN_FIM) {
        fprintf(stderr, "%d:token nao esperado [%s].\n", 
                tokenAtual.linha, tokenAtual.lexema);
    } 
    // nn:fim de arquivo não esperado.
    else {
        fprintf(stderr,"%d:fim de arquivo não esperado.\n", 
                tokenAtual.linha);
    }
    
    // NOTA: A string 'msg' não é usada aqui para manter o formato exigido.
}

void CasaToken(TipoToken esperado) {
    if (tokenAtual.tipo == esperado) {
        ConsomeLexemaAtual();
    } else {
        // Usa nome_token para gerar uma mensagem de erro mais detalhada
        char erro_msg[100];
        snprintf(erro_msg, 100, "Esperado '%s' (encontrado '%s').", 
                 nome_token(esperado), nome_token(tokenAtual.tipo));
        
        erro_sintatico(erro_msg);
    }
}

// Conjunto de tokens que podem iniciar um comando: FIRST_COMANDO
#define IS_FIRST_COMANDO(t) (t == TOKEN_IDENTIFICADOR || t == TOKEN_BEGIN || t == TOKEN_IF || t == TOKEN_WHILE)

// Conjunto de tokens que podem delimitar um comando ou bloco: FOLLOW_COMANDO (Tokens de sincronização)
#define IS_FOLLOW_COMANDO(t) (t == TOKEN_PONTO_VIRGULA || t == TOKEN_END || t == TOKEN_ELSE || t == TOKEN_FIM)

// Função de sincronização (Modo Pânico)
void sincronizar(void) {
    while (tokenAtual.tipo != TOKEN_FIM && !IS_FOLLOW_COMANDO(tokenAtual.tipo) && !IS_FIRST_COMANDO(tokenAtual.tipo)) {
        ConsomeLexemaAtual(); // Descartando!
    }
}


/* -------------------- Protótipos dos não-terminais -------------------- */
void programa(void); void bloco(void); void parte_decl_variaveis(void);
void declaracao_variaveis(void); void lista_identificadores(void); void tipo(void);
void comando_composto(void); void comando(void); void atribuicao(void);
void comando_condicional(void); void comando_repetitivo(void);
void expressao(void); void expressao_simples(void); void termo(void); void fator(void);

/* ==================== Implementação dos não-terminais ==================== */
void programa() {
    record_rule("programa -> program identificador ; bloco .");
    CasaToken(TOKEN_PROGRAM);
    CasaToken(TOKEN_IDENTIFICADOR);
    CasaToken(TOKEN_PONTO_VIRGULA);
    bloco();
}

void bloco() {
    record_rule("bloco -> parte_decl_variaveis comando_composto");
    parte_decl_variaveis();
    comando_composto();
}

void parte_decl_variaveis() {
    record_rule("parte_decl_variaveis");
    
    if(tokenAtual.tipo == TOKEN_VAR) {
        record_rule("parte_decl_variaveis -> var declaracao_variaveis ; { declaracao_variaveis ; }");
        CasaToken(TOKEN_VAR);
        
        do {
            declaracao_variaveis();
            CasaToken(TOKEN_PONTO_VIRGULA); 
            if (tokenAtual.tipo != TOKEN_IDENTIFICADOR && tokenAtual.tipo != TOKEN_BEGIN) {
                sincronizar(); 
            }
        } while(tokenAtual.tipo == TOKEN_IDENTIFICADOR);
    }
}

void declaracao_variaveis() {
    record_rule("declaracao_variaveis -> lista_identificadores : tipo");
    lista_identificadores();
    CasaToken(TOKEN_DOIS_PONTOS);
    tipo();
}

void lista_identificadores() {
    record_rule("lista_identificadores -> identificador { , identificador }");
    CasaToken(TOKEN_IDENTIFICADOR);
    while(tokenAtual.tipo == TOKEN_VIRGULA) {
        CasaToken(TOKEN_VIRGULA);
        if (tokenAtual.tipo != TOKEN_IDENTIFICADOR) {
            erro_sintatico("Identificador esperado após vírgula.");
            sincronizar();
        }
        CasaToken(TOKEN_IDENTIFICADOR);
    }
}

void tipo() {
    if(tokenAtual.tipo == TOKEN_INTEGER) {
        record_rule("tipo -> integer");
        CasaToken(TOKEN_INTEGER);
    } else if(tokenAtual.tipo == TOKEN_REAL) {
        record_rule("tipo -> real");
        CasaToken(TOKEN_REAL);
    } else {
        erro_sintatico("Esperado tipo integer ou real");
    }
}

void comando_composto() {
    record_rule("comando_composto -> begin comando { ; comando } end");
    CasaToken(TOKEN_BEGIN);
    
    if(tokenAtual.tipo != TOKEN_END) {
        comando();
        
        while(tokenAtual.tipo == TOKEN_PONTO_VIRGULA || !IS_FOLLOW_COMANDO(tokenAtual.tipo)) {
            
            if (tokenAtual.tipo == TOKEN_PONTO_VIRGULA) {
                CasaToken(TOKEN_PONTO_VIRGULA);
            } else if (!IS_FOLLOW_COMANDO(tokenAtual.tipo) && tokenAtual.tipo != TOKEN_FIM) {
                erro_sintatico("Ponto e vírgula esperado para separar comandos.");
                sincronizar();
            }

            if(tokenAtual.tipo == TOKEN_END || tokenAtual.tipo == TOKEN_FIM) break; 

            if (IS_FIRST_COMANDO(tokenAtual.tipo)) {
                comando();
            } else if (tokenAtual.tipo != TOKEN_END && tokenAtual.tipo != TOKEN_FIM) {
                erro_sintatico("Comando ou END esperado.");
                sincronizar();
                if(tokenAtual.tipo == TOKEN_END || tokenAtual.tipo == TOKEN_FIM) break;
            }
        }
    }
    
    CasaToken(TOKEN_END);
}

void comando() {
    if(tokenAtual.tipo == TOKEN_IDENTIFICADOR) {
        record_rule("comando -> atribuicao");
        atribuicao();
    } else if(tokenAtual.tipo == TOKEN_BEGIN) {
        record_rule("comando -> comando_composto");
        comando_composto();
    } else if(tokenAtual.tipo == TOKEN_IF) {
        record_rule("comando -> comando_condicional");
        comando_condicional();
    } else if(tokenAtual.tipo == TOKEN_WHILE) {
        record_rule("comando -> comando_repetitivo");
        comando_repetitivo();
    } else if (IS_FOLLOW_COMANDO(tokenAtual.tipo)) {
        record_rule("comando -> ε");
    } else {
        erro_sintatico("Esperado início de comando (atribuicao, begin, if, while).");
        sincronizar();
    }
}

void atribuicao() {
    record_rule("atribuicao -> identificador := expressao");
    CasaToken(TOKEN_IDENTIFICADOR);
    CasaToken(TOKEN_ATRIBUICAO);
    expressao();
}

void comando_condicional() {
    record_rule("comando_condicional -> if expressao then comando [ else comando ]");
    CasaToken(TOKEN_IF);
    expressao();
    CasaToken(TOKEN_THEN);
    comando();
    if(tokenAtual.tipo == TOKEN_ELSE) {
        CasaToken(TOKEN_ELSE);
        comando();
    }
}

void comando_repetitivo() {
    record_rule("comando_repetitivo -> while expressao do comando");
    CasaToken(TOKEN_WHILE);
    expressao();
    CasaToken(TOKEN_DO);
    comando();
}

void expressao() {
    record_rule("expressao -> expressao_simples [ op_rel expressao_simples ]");
    
    expressao_simples();
    if(tokenAtual.tipo == TOKEN_MENOR || tokenAtual.tipo == TOKEN_MAIOR || 
       tokenAtual.tipo == TOKEN_MENOR_IGUAL || tokenAtual.tipo == TOKEN_MAIOR_IGUAL || 
       tokenAtual.tipo == TOKEN_IGUAL || tokenAtual.tipo == TOKEN_DIFERENTE) 
    {
        record_rule("op_rel -> < | > | <= | >= | = | <>"); 
        ConsomeLexemaAtual();
        expressao_simples();
    }
}

void expressao_simples() {
    record_rule("expressao_simples -> [ + | - ] termo { ( + | - ) termo }");
    
    if(tokenAtual.tipo == TOKEN_MAIS || tokenAtual.tipo == TOKEN_MENOS) {
        CasaToken(tokenAtual.tipo);
    }
    termo();
    while(tokenAtual.tipo == TOKEN_MAIS || tokenAtual.tipo == TOKEN_MENOS) {
        CasaToken(tokenAtual.tipo);
        termo();
    }
}

void termo() {
    record_rule("termo -> fator { ( * | / ) fator }");
    fator();
    while(tokenAtual.tipo == TOKEN_MULT || tokenAtual.tipo == TOKEN_DIV) {
        CasaToken(tokenAtual.tipo);
        termo();
    }
}

void fator() {
    if(tokenAtual.tipo == TOKEN_IDENTIFICADOR) {
        record_rule("fator -> identificador");
        CasaToken(TOKEN_IDENTIFICADOR);
    } else if(tokenAtual.tipo == TOKEN_NUMERO) {
        record_rule("fator -> numero");
        CasaToken(TOKEN_NUMERO);
    } else if(tokenAtual.tipo == TOKEN_ABRE_PAR) {
        record_rule("fator -> ( expressao )");
        CasaToken(TOKEN_ABRE_PAR);
        expressao();
        CasaToken(TOKEN_FECHA_PAR);
    } else {
        erro_sintatico("Esperado identificador, numero ou (expressao)");
        sincronizar();
    }
}


/* ==================== Main ==================== */
int main(int argc, char *argv[]){
    if(argc<2){ fprintf(stderr,"Uso: %s arquivo.pas\n",argv[0]); return 1; }

    FILE *f = fopen(argv[1],"rb");
    if(!f){ fprintf(stderr,"Erro ao abrir o arquivo %s\n",argv[1]); return 1; }

    fseek(f,0,SEEK_END);
    long size = ftell(f);
    fseek(f,0,SEEK_SET);

    char *codigo = malloc(size+2);
    if (!codigo) { fprintf(stderr, "Memória insuficiente\n"); return 1; }
    
    fread(codigo,1,size,f);
    fclose(f);
    codigo[size]='\0';

    iniciar(&S,codigo); // Inicializa a Scanner global 'S'
    tokenAtual = proximo_token(&S);

    // Início da análise
    // Início da análise
    programa();
    
    // ... dentro da main, após programa();

    // Verificação de Erro Final
    if (erro_encontrado) {
        // O erro já foi impresso no formato exigido por erro_sintatico()
        fprintf(stderr,"Analise sintatica concluida com erros.\n");
        erro_fatal = 1; 
    } else if (tokenAtual.tipo != TOKEN_FIM) {
        // Este caso cobre apenas lixo após o programa (ex: "end. lixo")
        fprintf(stderr,"%d:token nao esperado [%s].\n", tokenAtual.linha, tokenAtual.lexema);
        erro_fatal = 1;
    } else {
        printf("Analise sintatica concluida com sucesso!\n");
    }
    
    // ... restante da main

    if (!erro_fatal) {
        imprimir_derivacao();
    }
    
    free(tokenAtual.lexema);
    free(codigo);
    
    return erro_fatal ? 1 : 0; 
}