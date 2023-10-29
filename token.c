#include "token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct tokeninfo {
	tokentype_t token;
	const char *str;
};

struct keyword {
	const char *equiv;
	tokentype_t token;
};

#define TOKENINFO(token) \
	{ \
		token, #token \
	}

struct tokeninfo tokens[] = {
    TOKENINFO(TOK_EOF),        TOKENINFO(TOK_ASSIGN),
    TOKENINFO(TOK_ASTERISK),   TOKENINFO(TOK_AT),
    TOKENINFO(TOK_BEGIN),      TOKENINFO(TOK_CARET),
    TOKENINFO(TOK_COLON),      TOKENINFO(TOK_COMMA),
    TOKENINFO(TOK_CTRLCODE),   TOKENINFO(TOK_CONST),
    TOKENINFO(TOK_DIGIT),      TOKENINFO(TOK_DIV),
    TOKENINFO(TOK_DOLLAR),     TOKENINFO(TOK_DOT),
    TOKENINFO(TOK_DOTDOT),     TOKENINFO(TOK_END),
    TOKENINFO(TOK_EQUAL),      TOKENINFO(TOK_GREATER),
    TOKENINFO(TOK_IDENTIFIER), TOKENINFO(TOK_LBRACKET),
    TOKENINFO(TOK_LESSER),     TOKENINFO(TOK_LPAREN),
    TOKENINFO(TOK_MINUS),      TOKENINFO(TOK_MOD),
    TOKENINFO(TOK_NIL),        TOKENINFO(TOK_PLUS),
    TOKENINFO(TOK_PROGRAM),    TOKENINFO(TOK_SLASH),
    TOKENINFO(TOK_STRING),     TOKENINFO(TOK_RBRACKET),
    TOKENINFO(TOK_RPAREN),     TOKENINFO(TOK_SEMICOLON),
    TOKENINFO(TOK_VAR),        {0, 0},
};

struct keyword keywords[] = {
    {"begin", TOK_BEGIN},
    {"div", TOK_DIV},
    {"end", TOK_END},
    {"mod", TOK_MOD},
    {"program", TOK_PROGRAM},
    {"var", TOK_VAR},
    {"nil", TOK_NIL},
    {0, 0},
};

static void
lowercase(char *str)
{
	char *c = str;
	while (c && *c) {
		if (*c >= 'A' && *c <= 'Z') {
			*c = (*c) + 0x20;
		}
		c++;
	}
}

tokentype_t
match_identifier(char *input)
{
	struct keyword *kw;
	char *lower;

	/* create a copy of the string in order to manipulate it */
	lower = strdup(input);
	lowercase(lower);

	for (kw = keywords; kw->equiv; kw++) {
		if (!strcmp(lower, kw->equiv)) {
			free(lower);
			return kw->token;
		}
	}

	free(lower);
	return TOK_IDENTIFIER;
}

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

void
token_free(token_t *token)
{
	// if there is no meta, nothing should be freed
	if (token->meta) {
		free(token->meta);
	}
}
