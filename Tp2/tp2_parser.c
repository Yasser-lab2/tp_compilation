/*
 * TP2 - Analyseur syntaxique descendant recursif de MICRO + reels.
 * Version SIMPLE, d'apres les procedures photographiees (p.32-35),
 * avec l'extension du TP1 : prim accepte aussi REALLITERAL.
 *
 * Grammaire acceptee :
 *   <system_goal> ::= <program> SCANEOF
 *   <program>     ::= BEGIN <inst_list> END
 *   <inst_list>   ::= <inst> { <inst> }
 *   <inst>        ::= ID := <expr> ; | READ ( <id_list> ) ; | WRITE ( <expr_list> ) ;
 *   <id_list>     ::= ID { , ID }
 *   <expr>        ::= <prim> { <add_op> <prim> }
 *   <expr_list>   ::= <expr> { , <expr> }
 *   <add_op>      ::= PLUSOP | MINUSOP
 *   <prim>        ::= '(' <expr> ')' | ID | INTLITERAL | REALLITERAL
 *
 * Compilation :
 *   cc -std=c11 -Wall -Wextra -pedantic tp2_parser.c -o tp2_parser
 * Exemples :
 *   echo 'begin x := 5.2; end' | ./tp2_parser
 *   echo 'begin x := 5 +; end' | ./tp2_parser   (erreur syntaxique)
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============ 1) SCANNER (repris du TP1 + mots reserves) ============ */

typedef enum {
    SCANEOF = 0,
    ID,
    INTLITERAL,
    REALLITERAL, /* extension TP1 */
    LPAREN,
    RPAREN,
    SEMICOLON,
    COMMA,
    PLUSOP,
    ASSIGNOP,
    MINUSOP,
    BEGIN_,
    END_,
    READ_,
    WRITE_
} token;

static char token_buffer[256];
static int ligne = 1;

static void clear_buffer(void)
{
    token_buffer[0] = '\0';
}

static void buffer_char(char c)
{
    size_t n = strlen(token_buffer);
    if (n + 1 < sizeof(token_buffer)) {
        token_buffer[n] = c;
        token_buffer[n + 1] = '\0';
    }
}

/* begin/end/read/write (minuscules ou MAJUSCULES, comme souvent en MICRO) */
static token check_reserved(void)
{
    if (strcmp(token_buffer, "begin") == 0 || strcmp(token_buffer, "BEGIN") == 0)
        return BEGIN_;
    if (strcmp(token_buffer, "end") == 0 || strcmp(token_buffer, "END") == 0)
        return END_;
    if (strcmp(token_buffer, "read") == 0 || strcmp(token_buffer, "READ") == 0)
        return READ_;
    if (strcmp(token_buffer, "write") == 0 || strcmp(token_buffer, "WRITE") == 0)
        return WRITE_;
    return ID;
}

static token scan_nombre(char premier)
{
    int c;
    clear_buffer();
    buffer_char(premier);
    /* partie entiere : les chiffres ne contiennent jamais '\n', pas de compteur ici */
    while ((c = getchar()) != EOF && isdigit(c))
        buffer_char((char)c);
    if (c == '.') {
        int d = getchar();
        if (d == EOF || !isdigit(d)) {
            fprintf(stderr, "ligne %d : reel mal forme \"%s.\"\n", ligne, token_buffer);
            exit(1);
        }
        buffer_char('.');
        buffer_char((char)d);
        while ((c = getchar()) != EOF && isdigit(c))
            buffer_char((char)c);
        /* c est le premier caractere APRES le nombre : le rendre (y compris
           '\n', qui sera compte une seule fois par la boucle principale). */
        if (c != EOF)
            ungetc(c, stdin);
        return REALLITERAL;
    }
    if (c != EOF)
        ungetc(c, stdin);
    return INTLITERAL;
}

static token scanner(void)
{
    int in_char, c;
    clear_buffer();
    while ((in_char = getchar()) != EOF) {
        if (in_char == '\n') {
            ligne++;
            continue;
        }
        if (isspace(in_char))
            continue;
        if (isalpha(in_char)) {
            buffer_char((char)in_char);
            for (c = getchar(); c != EOF && (isalnum(c) || c == '_'); c = getchar())
                buffer_char((char)c);
            /* c est le separateur : on le rend tel quel ('\n' compris).
               Le comptage de ligne se fait uniquement en haut de scanner(). */
            if (c != EOF)
                ungetc(c, stdin);
            return check_reserved();
        }
        if (isdigit(in_char))
            return scan_nombre((char)in_char);
        if (in_char == '(')
            return LPAREN;
        if (in_char == ')')
            return RPAREN;
        if (in_char == ';')
            return SEMICOLON;
        if (in_char == ',')
            return COMMA;
        if (in_char == '+')
            return PLUSOP;
        if (in_char == ':') {
            c = getchar();
            if (c == '=')
                return ASSIGNOP;
            fprintf(stderr, "ligne %d : ':' seul, on attendait ':='\n", ligne);
            exit(1);
        }
        if (in_char == '-') {
            c = getchar();
            if (c == '-') {
                do {
                    in_char = getchar();
                } while (in_char != EOF && in_char != '\n');
                if (in_char == '\n')
                    ligne++;
                if (in_char == EOF)
                    return SCANEOF;
                clear_buffer();
                continue;
            }
            if (c != EOF)
                ungetc(c, stdin);
            return MINUSOP;
        }
        fprintf(stderr, "ligne %d : caractere illegal '%c'\n", ligne, in_char);
        exit(1);
    }
    return SCANEOF;
}

/* ============ 2) PARSER DESCENDANT RECURSIF (p.32-35) ============ */

static token current_token; /* anticipation a 1 token */

static token next_token(void)
{
    return current_token;
}

static const char *nom(token t)
{
    switch (t) {
    case SCANEOF:    return "SCANEOF";
    case ID:         return "ID";
    case INTLITERAL: return "INTLITERAL";
    case REALLITERAL:return "REALLITERAL";
    case LPAREN:     return "'('";
    case RPAREN:     return "')'";
    case SEMICOLON:  return "';'";
    case COMMA:      return "','";
    case PLUSOP:     return "'+'";
    case ASSIGNOP:   return "':='";
    case MINUSOP:    return "'-'";
    case BEGIN_:     return "'begin'";
    case END_:       return "'end'";
    case READ_:      return "'read'";
    case WRITE_:     return "'write'";
    }
    return "???";
}

static void syntax_error(token attendu)
{
    fprintf(stderr,
            "ligne %d : erreur de syntaxe : trouve %s (\"%s\"), on attendait %s\n",
            ligne, nom(current_token), token_buffer, nom(attendu));
    exit(1);
}

static void match(token attendu)
{
    if (current_token == attendu)
        current_token = scanner();
    else
        syntax_error(attendu);
}

/* declarations avancees (recursion croisee expression <-> prim) */
static void program(void);
static void inst_list(void);
static void inst(void);
static void id_list(void);
static void expression(void);
static void expr_list(void);
static void add_opp(void);
static void prim(void);

/* <system_goal> ::= <program> SCANEOF */
static void system_goal(void)
{
    program();
    match(SCANEOF);
}

/* <program> ::= BEGIN <inst_list> END */
static void program(void)
{
    match(BEGIN_);
    inst_list();
    match(END_);
}

/* <inst_list> ::= <inst> { <inst> } */
static void inst_list(void)
{
    inst();
    for (;;) {
        switch (next_token()) {
        case ID:
        case READ_:
        case WRITE_:
            inst();
            break;
        default:
            return;
        }
    }
}

/* <inst> ::= ID := <expr> ; | READ ( <id_list> ) ; | WRITE ( <expr_list> ) ; */
static void inst(void)
{
    token t = next_token();
    switch (t) {
    case ID: /* <inst> ::= ID := <expr> ; */
        match(ID);
        match(ASSIGNOP);
        expression();
        match(SEMICOLON);
        break;
    case READ_: /* <inst> ::= READ ( <id_list> ) ; */
        match(READ_);
        match(LPAREN);
        id_list();
        match(RPAREN);
        match(SEMICOLON);
        break;
    case WRITE_: /* <inst> ::= WRITE ( <expr_list> ) ; */
        match(WRITE_);
        match(LPAREN);
        expr_list();
        match(RPAREN);
        match(SEMICOLON);
        break;
    default:
        syntax_error(ID); /* message simple : on attendait un debut d'instruction */
        break;
    }
}

/* <id_list> ::= ID { , ID } */
static void id_list(void)
{
    match(ID);
    while (next_token() == COMMA) {
        match(COMMA);
        match(ID);
    }
}

/* <expr> ::= <prim> { <add_op> <prim> } */
static void expression(void)
{
    token t;
    prim();
    for (t = next_token(); t == PLUSOP || t == MINUSOP; t = next_token()) {
        add_opp();
        prim();
    }
}

/* <expr_list> ::= <expr> { , <expr> } */
static void expr_list(void)
{
    expression();
    while (next_token() == COMMA) {
        match(COMMA);
        expression();
    }
}

/* <add_op> ::= PLUSOP | MINUSOP */
static void add_opp(void)
{
    token t = next_token();
    if (t == PLUSOP || t == MINUSOP)
        match(t);
    else
        syntax_error(PLUSOP);
}

/* <prim> ::= '(' <expr> ')' | ID | INTLITERAL | REALLITERAL (extension TP1) */
static void prim(void)
{
    token t = next_token();
    switch (t) {
    case LPAREN: /* '(' <expr> ')' */
        match(LPAREN);
        expression();
        match(RPAREN);
        break;
    case ID:
        match(ID);
        break;
    case INTLITERAL:
        match(INTLITERAL);
        break;
    case REALLITERAL: /* <-- AJOUT TP1/TP2 : les reels sont des primaires */
        match(REALLITERAL);
        break;
    default:
        syntax_error(INTLITERAL);
        break;
    }
}

int main(void)
{
    current_token = scanner(); /* amorcer l'anticipation */
    system_goal();
    printf("ACCEPTE : programme MICRO syntaxiquement correct.\n");
    return 0;
}
