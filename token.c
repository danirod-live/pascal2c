#include "token.h"
#include <stdio.h>

struct tokeninfo {
	tokentype_t token;
	const char *str;
};

#define TOKENINFO(token) \
	{ \
		token, #token \
	}

struct tokeninfo tokens[] = {
    TOKENINFO(TOK_EOF),
    TOKENINFO(TOK_ASSIGN),
    TOKENINFO(TOK_ASTERISK),
    TOKENINFO(TOK_BEGIN),
    TOKENINFO(TOK_COLON),
    TOKENINFO(TOK_COMMA),
    TOKENINFO(TOK_CTRLCODE),
    TOKENINFO(TOK_CONST),
    TOKENINFO(TOK_DIGIT),
    TOKENINFO(TOK_DIV),
    TOKENINFO(TOK_DOT),
    TOKENINFO(TOK_END),
    TOKENINFO(TOK_EQUAL),
    TOKENINFO(TOK_IDENTIFIER),
    TOKENINFO(TOK_LPAREN),
    TOKENINFO(TOK_MINUS),
    TOKENINFO(TOK_MOD),
    TOKENINFO(TOK_PLUS),
    TOKENINFO(TOK_PROGRAM),
    TOKENINFO(TOK_SLASH),
    TOKENINFO(TOK_STRING),
    TOKENINFO(TOK_RPAREN),
    TOKENINFO(TOK_SEMICOLON),
    TOKENINFO(TOK_VAR),
    {0, 0},
};

const char *
tokentype_string(tokentype_t tok)
{
	struct tokeninfo *tt = tokens;
	while (tt->str) {
		if (tt->token == tok) {
			return tt->str;
		}
		tt++;
	}
	return "<null>";
}
