/*
 * Scanner lexical MICRO - extension des nombres reels.
 *
 * Compilation :
 *     cc -std=c11 -Wall -Wextra -pedantic scanner_reels.c -o scanner_reels
 *
 * Exemple :
 *     echo 'x := 5.2 + 10; -- commentaire' | ./scanner_reels
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#define TOKEN_BUFFER_SIZE 256

typedef enum {
    SCANEOF = 0,
    ID,
    INTLITERAL,
    REALLITERAL, /* Nouveau token : nombre reel, par exemple 5.2. */
    LPAREN,
    RPAREN,
    SEMICOLON,
    COMMA,
    PLUSOP,
    ASSIGNOP,
    MINUSOP,
    LEXICAL_ERROR
} token;

/* Comme dans l'enonce : le lexeme reconnu est disponible globalement. */
char token_buffer[TOKEN_BUFFER_SIZE];

static size_t buffer_length;
static int buffer_overflow;

static void clear_buffer(void)
{
    buffer_length = 0;
    buffer_overflow = 0;
    token_buffer[0] = '\0';
}

static void buffer_char(int c)
{
    if (buffer_length + 1 < sizeof token_buffer) {
        token_buffer[buffer_length++] = (char)c;
        token_buffer[buffer_length] = '\0';
    } else {
        buffer_overflow = 1;
    }
}

static int is_letter(int c)
{
    return c != EOF && isalpha((unsigned char)c);
}

static int is_digit(int c)
{
    return c != EOF && isdigit((unsigned char)c);
}

static int is_alnum_or_underscore(int c)
{
    return c != EOF && (isalnum((unsigned char)c) || c == '_');
}

/* Dans un vrai compilateur, cette fonction consulte la table des mots reserves. */
static token check_reserved(void)
{
    return ID;
}

static token lexical_error(int c)
{
    if (c == EOF) {
        fprintf(stderr, "Erreur lexicale : fin de fichier inattendue dans \"%s\"\n",
                token_buffer);
    } else if (buffer_length > 0) {
        fprintf(stderr, "Erreur lexicale pres de \"%s\" (caractere '%c')\n",
                token_buffer, (char)c);
    } else {
        fprintf(stderr, "Erreur lexicale : caractere '%c'\n", (char)c);
    }

    return LEXICAL_ERROR;
}

static token scan_number(int first_char)
{
    int c;

    clear_buffer();
    buffer_char(first_char);

    /* Partie entiere obligatoire : DIGIT+. */
    while ((c = getchar()) != EOF && is_digit(c)) {
        buffer_char(c);
    }

    if (c == '.') {
        int first_fraction_digit = getchar();

        /*
         * Regle choisie : un reel est DIGIT+ '.' DIGIT+.
         * Ainsi, 5.2 est valide, mais 5. est signale comme mal forme.
         */
        if (!is_digit(first_fraction_digit)) {
            buffer_char('.');
            if (first_fraction_digit != EOF) {
                ungetc(first_fraction_digit, stdin);
            }
            return lexical_error('.');
        }

        buffer_char('.');
        buffer_char(first_fraction_digit);

        while ((c = getchar()) != EOF && is_digit(c)) {
            buffer_char(c);
        }

        if (c != EOF) {
            ungetc(c, stdin);
        }

        if (buffer_overflow) {
            return lexical_error(EOF);
        }
        return REALLITERAL;
    }

    if (c != EOF) {
        ungetc(c, stdin);
    }

    if (buffer_overflow) {
        return lexical_error(EOF);
    }
    return INTLITERAL;
}

token scanner(void)
{
    int in_char;

    clear_buffer();

    for (;;) {
        in_char = getchar();

        if (in_char == EOF) {
            return SCANEOF;
        }

        if (isspace((unsigned char)in_char)) {
            continue;
        }

        if (is_letter(in_char)) {
            int c;

            /* ID ::= LETTER | ID LETTER | ID DIGIT | ID UNDERSCORE */
            buffer_char(in_char);
            while ((c = getchar()) != EOF && is_alnum_or_underscore(c)) {
                buffer_char(c);
            }
            if (c != EOF) {
                ungetc(c, stdin);
            }

            if (buffer_overflow) {
                return lexical_error(EOF);
            }
            return check_reserved();
        }

        if (is_digit(in_char)) {
            return scan_number(in_char);
        }

        if (in_char == '(') {
            return LPAREN;
        }
        if (in_char == ')') {
            return RPAREN;
        }
        if (in_char == ';') {
            return SEMICOLON;
        }
        if (in_char == ',') {
            return COMMA;
        }
        if (in_char == '+') {
            return PLUSOP;
        }

        if (in_char == ':') {
            int c = getchar();

            if (c == '=') {
                return ASSIGNOP;
            }
            if (c != EOF) {
                ungetc(c, stdin);
            }
            return lexical_error(':');
        }

        if (in_char == '-') {
            int c = getchar();

            if (c == '-') {
                /* Commentaire jusqu'a la fin de la ligne. */
                do {
                    in_char = getchar();
                } while (in_char != EOF && in_char != '\n');
                if (in_char == EOF) {
                    return SCANEOF;
                }
                continue;
            }

            if (c != EOF) {
                ungetc(c, stdin);
            }
            return MINUSOP;
        }

        return lexical_error(in_char);
    }
}

static const char *token_name(token current_token)
{
    switch (current_token) {
    case SCANEOF:       return "SCANEOF";
    case ID:            return "ID";
    case INTLITERAL:    return "INTLITERAL";
    case REALLITERAL:   return "REALLITERAL";
    case LPAREN:        return "LPAREN";
    case RPAREN:        return "RPAREN";
    case SEMICOLON:     return "SEMICOLON";
    case COMMA:         return "COMMA";
    case PLUSOP:        return "PLUSOP";
    case ASSIGNOP:      return "ASSIGNOP";
    case MINUSOP:       return "MINUSOP";
    case LEXICAL_ERROR: return "LEXICAL_ERROR";
    }
    return "UNKNOWN_TOKEN";
}

int main(void)
{
    token current_token;

    do {
        current_token = scanner();
        if (current_token == ID || current_token == INTLITERAL ||
            current_token == REALLITERAL) {
            printf("%-14s lexeme=\"%s\"\n", token_name(current_token),
                   token_buffer);
        } else {
            printf("%s\n", token_name(current_token));
        }
    } while (current_token != SCANEOF && current_token != LEXICAL_ERROR);

    return current_token == LEXICAL_ERROR ? EXIT_FAILURE : EXIT_SUCCESS;
}
