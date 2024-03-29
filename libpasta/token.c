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
    TOKENINFO(TOK_EOF),        TOKENINFO(TOK_AND),
    TOKENINFO(TOK_ARRAY),      TOKENINFO(TOK_ASSIGN),
    TOKENINFO(TOK_ASTERISK),   TOKENINFO(TOK_AT),
    TOKENINFO(TOK_BEGIN),      TOKENINFO(TOK_CARET),
    TOKENINFO(TOK_CASE),       TOKENINFO(TOK_COLON),
    TOKENINFO(TOK_COMMA),      TOKENINFO(TOK_CONST),
    TOKENINFO(TOK_CTRLCODE),   TOKENINFO(TOK_DIGIT),
    TOKENINFO(TOK_DIV),        TOKENINFO(TOK_DO),
    TOKENINFO(TOK_DOLLAR),     TOKENINFO(TOK_DOT),
    TOKENINFO(TOK_DOTDOT),     TOKENINFO(TOK_DOWNTO),
    TOKENINFO(TOK_ELSE),       TOKENINFO(TOK_END),
    TOKENINFO(TOK_EQUAL),      TOKENINFO(TOK_EXIT),
    TOKENINFO(TOK_FILE),       TOKENINFO(TOK_FOR),
    TOKENINFO(TOK_FUNCTION),   TOKENINFO(TOK_GOTO),
    TOKENINFO(TOK_GREATEQL),   TOKENINFO(TOK_GREATER),
    TOKENINFO(TOK_IDENTIFIER), TOKENINFO(TOK_IF),
    TOKENINFO(TOK_IN),         TOKENINFO(TOK_LBRACKET),
    TOKENINFO(TOK_LESSEQL),    TOKENINFO(TOK_LESSER),
    TOKENINFO(TOK_LPAREN),     TOKENINFO(TOK_MINUS),
    TOKENINFO(TOK_MOD),        TOKENINFO(TOK_NEQUAL),
    TOKENINFO(TOK_NIL),        TOKENINFO(TOK_NOT),
    TOKENINFO(TOK_OF),         TOKENINFO(TOK_OR),
    TOKENINFO(TOK_PACKED),     TOKENINFO(TOK_PLUS),
    TOKENINFO(TOK_PROCEDURE),  TOKENINFO(TOK_PROGRAM),
    TOKENINFO(TOK_RBRACKET),   TOKENINFO(TOK_RECORD),
    TOKENINFO(TOK_REPEAT),     TOKENINFO(TOK_RPAREN),
    TOKENINFO(TOK_SEMICOLON),  TOKENINFO(TOK_SET),
    TOKENINFO(TOK_SLASH),      TOKENINFO(TOK_STRING),
    TOKENINFO(TOK_THEN),       TOKENINFO(TOK_TO),
    TOKENINFO(TOK_TYPE),       TOKENINFO(TOK_UNTIL),
    TOKENINFO(TOK_VAR),        TOKENINFO(TOK_WHILE),
    TOKENINFO(TOK_WITH),       {0, 0},
};

struct keyword keywords[] = {
    {"and", TOK_AND},         {"array", TOK_ARRAY},
    {"begin", TOK_BEGIN},     {"case", TOK_CASE},
    {"const", TOK_CONST},     {"div", TOK_DIV},
    {"do", TOK_DO},           {"downto", TOK_DOWNTO},
    {"else", TOK_ELSE},       {"end", TOK_END},
    {"exit", TOK_EXIT},       {"file", TOK_FILE},
    {"for", TOK_FOR},         {"function", TOK_FUNCTION},
    {"goto", TOK_GOTO},       {"if", TOK_IF},
    {"in", TOK_IN},           {"mod", TOK_MOD},
    {"nil", TOK_NIL},         {"not", TOK_NOT},
    {"of", TOK_OF},           {"or", TOK_OR},
    {"packed", TOK_PACKED},   {"procedure", TOK_PROCEDURE},
    {"program", TOK_PROGRAM}, {"record", TOK_RECORD},
    {"repeat", TOK_REPEAT},   {"set", TOK_SET},
    {"then", TOK_THEN},       {"to", TOK_TO},
    {"type", TOK_TYPE},       {"until", TOK_UNTIL},
    {"var", TOK_VAR},         {"while", TOK_WHILE},
    {"with", TOK_WITH},       {0, 0},
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
