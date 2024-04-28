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
#include "parser.h"
#include <stdlib.h>

static int has_extra(parser_t *parser);
static void extra_array(parser_t *parser, expr2_t *expr);
static void consume_extra(expr2_t *expr, parser_t *parser);

expr2_t *
parser2_variable(parser_t *parser)
{
	expr2_t *exp;

	exp = expression_new(EXP_VARIABLE);
	E_VARIABLE(*exp).identifier = parser2_identifier(parser);
	while (has_extra(parser)) {
		consume_extra(exp, parser);
	}
	return exp;
}

static void
consume_extra(expr2_t *expr, parser_t *parser)
{
	expr2_t *extra;
	token_t *token;
	unsigned int next_path, size;

	token = parser_token(parser);
	switch (token->type) {
	case TOK_DOT:
		extra = expression_new(EXP_VARIABLE_PATH_DOT);
		E_VARIABLE_PATH_DOT(*extra).identifier =
		    parser2_identifier(parser);
		break;
	case TOK_CARET:
		extra = expression_new(EXP_VARIABLE_PATH_CARET);
		break;
	case TOK_LBRACKET:
		extra = expression_new(EXP_VARIABLE_PATH_ARRAY);
		extra_array(parser, extra);
		break;
	default:
		parser_error(parser, token, "Invalid extra");
	}

	next_path = E_VARIABLE(*expr).path_count;
	E_VARIABLE(*expr).path_count++;
	size = sizeof(expr2_t *) * E_VARIABLE(*expr).path_count;
	E_VARIABLE(*expr).paths = realloc(E_VARIABLE(*expr).paths, size);
	E_VARIABLE(*expr).paths[next_path] = extra;
}

static int
has_extra(parser_t *parser)
{
	token_t *tok = parser_peek(parser);
	return tok->type == TOK_CARET || tok->type == TOK_DOT
	       || tok->type == TOK_LBRACKET;
}

static void
extra_array(parser_t *parser, expr2_t *expr)
{
#define E_EXPRESSIONS(e) (E_VARIABLE_PATH_ARRAY(e).expressions)
#define E_EXP_COUNT(e) (E_VARIABLE_PATH_ARRAY(e).exp_count)
	int size, next_exp;
	token_t *separator;

	E_EXP_COUNT(*expr) = 0;
	E_EXPRESSIONS(*expr) = NULL;
	for (;;) {
		next_exp = E_EXP_COUNT(*expr);
		E_EXP_COUNT(*expr)++;
		size = E_EXP_COUNT(*expr) * sizeof(expr2_t);
		E_EXPRESSIONS(*expr) = realloc(E_EXPRESSIONS(*expr), size);
		E_EXPRESSIONS(*expr)[next_exp] = parser2_expression(parser);

		separator = parser_token(parser);
		switch (separator->type) {
		case TOK_RBRACKET:
			return;
		case TOK_COMMA:
			continue;
		default:
			parser_error(parser, separator, "Unexpected separator");
		}
	}
#undef E_EXPRESSIONS
#undef E_EXP_COUNT
}
