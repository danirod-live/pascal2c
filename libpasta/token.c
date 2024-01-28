/* libpasta -- an AST parser for Pascal
 * Copyright (C) 2024 Dani Rodríguez <dani@danirod.es>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
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
    TOKENINFO(TOK_EOF),      TOKENINFO(TOK_AND),
    TOKENINFO(TOK_ASSIGN),   TOKENINFO(TOK_ASTERISK),
    TOKENINFO(TOK_AT),       TOKENINFO(TOK_ARRAY),
    TOKENINFO(TOK_BEGIN),    TOKENINFO(TOK_CARET),
    TOKENINFO(TOK_CASE),     TOKENINFO(TOK_COLON),
    TOKENINFO(TOK_COMMA),    TOKENINFO(TOK_CTRLCODE),
    TOKENINFO(TOK_CONST),    TOKENINFO(TOK_DIGIT),
    TOKENINFO(TOK_DIV),      TOKENINFO(TOK_DOLLAR),
    TOKENINFO(TOK_DOT),      TOKENINFO(TOK_DOTDOT),
    TOKENINFO(TOK_END),      TOKENINFO(TOK_EQUAL),
    TOKENINFO(TOK_FILE),     TOKENINFO(TOK_GREATER),
    TOKENINFO(TOK_GREATEQL), TOKENINFO(TOK_IDENTIFIER),
    TOKENINFO(TOK_IN),       TOKENINFO(TOK_LBRACKET),
    TOKENINFO(TOK_LESSER),   TOKENINFO(TOK_LESSEQL),
    TOKENINFO(TOK_LPAREN),   TOKENINFO(TOK_MINUS),
    TOKENINFO(TOK_MOD),      TOKENINFO(TOK_NEQUAL),
    TOKENINFO(TOK_NIL),      TOKENINFO(TOK_NOT),
    TOKENINFO(TOK_OF),       TOKENINFO(TOK_OR),
    TOKENINFO(TOK_PACKED),   TOKENINFO(TOK_PLUS),
    TOKENINFO(TOK_PROGRAM),  TOKENINFO(TOK_RECORD),
    TOKENINFO(TOK_SET),      TOKENINFO(TOK_SLASH),
    TOKENINFO(TOK_STRING),   TOKENINFO(TOK_RBRACKET),
    TOKENINFO(TOK_RPAREN),   TOKENINFO(TOK_SEMICOLON),
    TOKENINFO(TOK_VAR),      {0, 0},
};

struct keyword keywords[] = {
    {"and", TOK_AND},
    {"array", TOK_ARRAY},
    {"begin", TOK_BEGIN},
    {"case", TOK_CASE},
    {"div", TOK_DIV},
    {"end", TOK_END},
    {"file", TOK_FILE},
    {"in", TOK_IN},
    {"mod", TOK_MOD},
    {"not", TOK_NOT},
    {"of", TOK_OF},
    {"or", TOK_OR},
    {"packed", TOK_PACKED},
    {"program", TOK_PROGRAM},
    {"record", TOK_RECORD},
    {"set", TOK_SET},
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
