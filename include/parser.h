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
#pragma once

#include "scanner.h"
#include "token.h"
#include <stdlib.h>

typedef enum expr_type {
	UNARY, // -5
	BINARY, // 2+3
	GROUPING, // wrapper
	LITERAL, // 4
} expr_type_t;

typedef struct expr {
	expr_type_t type;
	struct expr *exp_left, *exp_right;
	token_t *token;
	void *literal;
} expr_t;

expr_t *new_unary(token_t *t, expr_t *expr);
expr_t *new_binary(token_t *t, expr_t *left, expr_t *right);
expr_t *new_grouping(expr_t *exp);
expr_t *new_literal(token_t *lit);

typedef struct parser {
	token_t **tokens;
	unsigned int len;
	unsigned int pos;
} parser_t;

parser_t *parser_new();
void parser_load_tokens(parser_t *parser, scanner_t *scanner);
void parser_dump(parser_t *parser);
token_t *parser_peek(parser_t *parser);
token_t *parser_token(parser_t *parser);
token_t *parser_token_expect(parser_t *, tokentype_t);
void parser_consume(parser_t *parser, tokentype_t type);
void __attribute__((noreturn))
parser_error(parser_t *parser, token_t *token, char *error);

expr_t *parser_identifier_list(parser_t *parser);

expr_t *parser_identifier(parser_t *parser);
expr_t *parser_unsigned_integer(parser_t *parser);
expr_t *parser_unsigned_number(parser_t *parser);
expr_t *parser_unsigned_constant(parser_t *parser);
expr_t *parser_constant(parser_t *parser);
expr_t *parser_simple_type(parser_t *parser);
expr_t *parser_type(parser_t *parser);
expr_t *parser_field_list(parser_t *parser);
expr_t *parser_variable(parser_t *parser);
expr_t *parser_parameter_list(parser_t *parser);

void dump_expr(expr_t *expr);
