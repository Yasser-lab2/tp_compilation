/*
 * TP1 - Scanner lexical MICRO enrichi avec les reels.
 * Version SIMPLE, fidele a l'enonce photographie (p.27-28) + Exercice 1.1.
 *
 * Compilation :
 *   cc -std=c11 -Wall -Wextra -pedantic tp1_scanner.c -o tp1_scanner
 * Exemple :
 *   echo 'x := 5.2 + 10; -- commentaire' | ./tp1_scanner
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>

typedef enum {
    SCANEOF = 0,
    ID,
    INTLITERAL,
    REALLITERAL, /* NOUVEAU (Exercice 1.1) : ex. 5.2 */
    LPAREN,
    RPAREN,
    SEMICOLON,
    COMMA,
    PLUSOP,
    ASSIGNOP,   /* := */
    MINUSOP
} token;

char token_buffer[256]; /* comme dans l'enonce : lexeme courant */

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

/* Version minimale : pas de mots reserves dans le TP1 (tout est ID).
   Le TP2 ajoutera begin/end/read/write ici. */
static token check_reserved(void)
{
    return ID;
}

static token lexical_error(char c)
{
    if (c == '\0')
        fprintf(stderr, "Erreur lexicale pres de \"%s\"\n", token_buffer);
    else
        fprintf(stderr, "Erreur lexicale : caractere '%c' (buffer=\"%s\")\n",
                c, token_buffer);
    return SCANEOF; /* on arrete simplement le scan minimal */
}

/* --- La SEULE partie modifiee pour l'Exercice 1.1 --- */
static token scan_nombre(char premier)
{
    int c;

    clear_buffer();
    buffer_char(premier);

    /* 1) Partie entiere : DIGIT+ */
    while ((c = getchar()) != EOF && isdigit(c))
        buffer_char((char)c);

    /* 2) Y a-t-il un point decimal ? */
    if (c == '.') {
        int d = getchar();
        /* Regle du cours : REEL ::= DIGIT+ '.' DIGIT+
           donc 5.2 OK, mais 5. REFUSE (il faut au moins 1 chiffre apres). */
        if (d == EOF || !isdigit(d)) {
            buffer_char('.');
            fprintf(stderr,
                    "Erreur lexicale : reel mal forme \"%s\" "
                    "(il faut au moins un chiffre apres le point)\n",
                    token_buffer);
            return lexical_error('\0');
        }
        buffer_char('.');
        buffer_char((char)d);
        while ((c = getchar()) != EOF && isdigit(c))
            buffer_char((char)c);
        if (c != EOF)
            ungetc(c, stdin);
        return REALLITERAL;
    }

    /* 3) Pas de point : c'est un entier */
    if (c != EOF)
        ungetc(c, stdin);
    return INTLITERAL;
}

token scanner(void)
{
    int in_char, c;

    clear_buffer();
    while ((in_char = getchar()) != EOF) {
        if (isspace(in_char))
            continue; /* espaces ignores */

        else if (isalpha(in_char)) {
            /* ID ::= LETTER (LETTER | DIGIT | '_')* */
            buffer_char((char)in_char);
            for (c = getchar(); isalnum(c) || c == '_'; c = getchar())
                buffer_char((char)c);
            ungetc(c, stdin);
            return check_reserved();
        }
        else if (isdigit(in_char)) {
            return scan_nombre((char)in_char);
        }
        else if (in_char == '(')
            return LPAREN;
        else if (in_char == ')')
            return RPAREN;
        else if (in_char == ';')
            return SEMICOLON;
        else if (in_char == ',')
            return COMMA;
        else if (in_char == '+')
            return PLUSOP;
        else if (in_char == ':') { /* look for ":=" */
            c = getchar();
            if (c == '=')
                return ASSIGNOP;
            else {
                if (c != EOF)
                    ungetc(c, stdin);
                return lexical_error((char)in_char);
            }
        }
        else if (in_char == '-') { /* "--" commentaire ou MINUSOP */
            c = getchar();
            if (c == '-') {
                /* commentaire jusqu'a fin de ligne, sans token */
                do {
                    in_char = getchar();
                } while (in_char != EOF && in_char != '\n');
                clear_buffer();
                if (in_char == EOF)
                    return SCANEOF;
                continue; /* repartir dans le while */
            }
            else {
                if (c != EOF)
                    ungetc(c, stdin);
                return MINUSOP;
            }
        }
        else
            return lexical_error((char)in_char);
    }
    return SCANEOF;
}

static const char *nom_token(token t)
{
    switch (t) {
    case SCANEOF:    return "SCANEOF";
    case ID:         return "ID";
    case INTLITERAL: return "INTLITERAL";
    case REALLITERAL:return "REALLITERAL";
    case LPAREN:     return "LPAREN";
    case RPAREN:     return "RPAREN";
    case SEMICOLON:  return "SEMICOLON";
    case COMMA:      return "COMMA";
    case PLUSOP:     return "PLUSOP";
    case ASSIGNOP:   return "ASSIGNOP";
    case MINUSOP:    return "MINUSOP";
    }
    return "???";
}

int main(void)
{
    token t;
    do {
        t = scanner();
        if (t == ID || t == INTLITERAL || t == REALLITERAL)
            printf("%-11s lexeme=\"%s\"\n", nom_token(t), token_buffer);
        else
            printf("%s\n", nom_token(t));
    } while (t != SCANEOF);
    return 0;
}
