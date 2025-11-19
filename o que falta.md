1. lexico.h — está incompleto (faltam definições centrais)

Caminho: lexico.h.
Faltas concretas:

Definição completa do enum de tokens (TipoToken) com todos os tokens que o sintático espera (identificador, número, palavras-chave: program, var, integer, real, begin, end, if, then, else, while, do, símbolos: ; , . : := + - \* / = <> < <= >= > ( ), TOKEN_FIM, TOKEN_ERRO etc.).

Definição do Token (struct com TipoToken tipo; char \*lexema; int linha; int coluna;).

Definição do Scanner (struct com buffer, índice, caractere atual, linha, coluna).

Declaração de EntradaTS (estrutura para tabela de símbolos) se aplicável.

Protótipos usados pelo sintático: void iniciar(Scanner *sc, const char *texto); Token proximo_token(Scanner *sc); void iniciar_tabela_simbolos(); void liberar_tabela_simbolos(); char *nome_token(TipoToken t); extern FILE \*output_file;

Ação recomendada — substituir o lexico.h atual por algo equivalente a este esqueleto (exemplo mínimo):

#ifndef LEXICO_H
#define LEXICO_H

#include <stdio.h>

typedef enum {
TOKEN_IDENT,
TOKEN_NUMERO,
TOKEN_INTEIRO, // se usar tipo literal
TOKEN_STRING,
TOKEN_PALAVRA_RESERVADA, // ou listar keywords específicas
TOKEN_MAIS,
TOKEN_MENOS,
TOKEN_MULT,
TOKEN_DIV,
TOKEN_PONTO_VIRGULA,
TOKEN_VIRGULA,
TOKEN_PONTO,
TOKEN_DOIS_PONTOS,
TOKEN_ATRIB, // ':='
TOKEN_IGUAL,
TOKEN_DIF, // '<>'
TOKEN_MENOR,
TOKEN_MAIOR,
TOKEN_MENOR_IGUAL,
TOKEN_MAIOR_IGUAL,
TOKEN_ABRE_PAREN,
TOKEN_FECHA_PAREN,
TOKEN_BEGIN, TOKEN_END, TOKEN_PROGRAM, TOKEN_VAR, TOKEN_IF, TOKEN_THEN, TOKEN_ELSE, TOKEN_WHILE, TOKEN_DO,
TOKEN_FIM,
TOKEN_ERRO
} TipoToken;

typedef struct {
TipoToken tipo;
char \*lexema;
int linha;
int coluna;
} Token;

typedef struct {
const char \*src;
int i;
char c;
int linha;
int coluna;
} Scanner;

/_ tabela de simbolos (exemplo) _/
typedef struct {
char \*lexema;
TipoToken tipo;
} EntradaTS;

/_ protótipos _/
void iniciar(Scanner *sc, const char *texto);
Token proximo_token(Scanner *sc);
void iniciar_tabela_simbolos();
void liberar_tabela_simbolos();
char *nome_token(TipoToken t);
extern FILE \*output_file;

#endif

2. lexico.c — precisa da implementação completa / revisar consistência com o header

Caminho: lexico.c. Observação: o arquivo existe mas, analisando o conteúdo no repositório, há trechos faltando ou que dependem das definições do header (ou seja, o código atual não está autonomamente utilizável). O sintático chama proximo_token, iniciar, iniciar_tabela_simbolos, consultar_ts, inserir_ts, liberar_tabela_simbolos, criar_token_texto e funções de coleta (coletar_string, coletar_comentario, coletar identificadores/números). Verifique se todas estão:

Implementadas com a mesma assinatura declarada no header.

Retornam Token com lexema alocado dinamicamente (o sintático faz free(tokenAtual.lexema) no main).

Geram TOKEN_FIM corretamente ao final do arquivo.

Convertem palavras-chave para tokens reservados (ou usam TOKEN_PALAVRA_RESERVADA com lexema indicando qual).

Ação recomendada:

Completar/retirar trechos que estejam comentados/omitidos no lexico.c.

Garantir que proximo_token() sempre retorne um Token válido (incluindo linha), e que token.lexema seja malloc/strdup e documentado para ser liberado pelo chamador.

Implementar/confirmar: criar_token_texto(), coletar_identificador(), coletar_numero(), coletar_string(), pular_espacos(), avancar() e funções de manipulação da tabela de símbolos (consultar_ts, inserir_ts).

3. Compatibilidade entre sintatico.c e o léxico

Caminho: sintatico.c. Estado: muito próximo do requerido — já existem funções para todos os não-terminais (programa, bloco, parte_decl_variaveis, declaracao_variaveis, lista_identificadores, tipo, comando_composto, comando, atribuicao, comando_condicional, comando_repetitivo, expressao, expressao_simples, termo, fator) e CasaToken() implementada. Ainda assim, para compilar/rodar corretamente:

O lexico.h deve definir exatamente os tipos usados em sintatico.c (TipoToken, Token, Scanner, nomes de tokens como TOKEN_PONTO_VIRGULA etc.) — caso contrário ocorrem erros de compilação.

O léxico precisa mapear lexemas das palavras-chave ("begin", "end", "if", etc.) para TOKEN_PALAVRA_RESERVADA ou tokens dedicados, e sintatico.c usa comparações via tokenAtual.lexema em vários pontos — garanta consistência (se preferir, converta sintatico.c para testar tokenAtual.tipo quando fizer sentido).

4. Saída exigida pelo enunciado (sequência de produções / árvore)

O enunciado exige que para cada fonte testado o analisador sintático emita a sequência de regras de produção ou a árvore sintática. Em sintatico.c há printf("Regra: ...") em várias regras — isso provavelmente já cumpre o requisito de sequência de regras. Verifique:

Se todas as regras imprimem exatamente a sequência (consistência formatada).

Se o output para programas corretos está presente e legível (3 certos) e se erros seguem o formato do enunciado (já há erro_sintatico() imprimindo nn:token nao esperado [lex]. e nn:fim de arquivo não esperado. — confirme que não há texto adicional no mesmo fprintf).

5. Testes / relatório

No repositório há exemplos em Códigos - Teste pascal/ e programa.pas. Contudo:

As saídas de teste e o relatório técnico pedido no enunciado (descrição das structs/funções e 3 testes corretos + 3 errados) não estão no repositório como relatório formal. Você precisa preparar o relatório e incluir os testes (entrada e saída esperada) para entregar.

6. Pequenas incongruências a conferir (lista técnica)

sintatico.c faz free(tokenAtual.lexema); no main — confirmar que todo Token vindo do léxico tem lexema alocado. Se nem todo token tiver lexema (ex.: TOKEN_FIM) isso pode gerar free(NULL) (ok) ou free de ponteiro inválido.

Formatos de nomes de tokens: sintatico.c usa TOKEN_PONTO_VIRGULA etc. Garanta que o enum no header use exatamente esses símbolos.

A identificação de palavras reservadas: decidir se o léxico devolve TOKEN_PALAVRA_RESERVADA com lexema == "begin" ou tokens separados TOKEN_BEGIN. Atualize sintatico.c conforme a escolha (atualmente o código faz comparações textuais em tokenAtual.lexema — manter ou alterar para testar tokenAtual.tipo).

Arquivo lexico.dSYM/... pode ser removido do repo (não necessário).

7. Resumo prático e imediato (o quê implementar já)

Substituir/completar lexico.h com os tipos e protótipos mínimos (exemplo fornecido acima).

Completar/validar lexico.c para que:

iniciar() inicialize Scanner.

proximo_token() retorne tokens corretos e TOKEN_FIM ao fim.

criar_token_texto() aloque e retorne Token.lexema.

Tabela de símbolos funcione (inserir keywords + consulta).

Compilar: gcc -std=c11 -O2 sintatico.c lexico.c -o analisador e corrigir warnings/erros.

Rodar os testes (3 corretos + 3 errados) e garantir mensagens conforme PDF.

Gerar relatório (explicar structs e funções e anexar testes e saídas).
