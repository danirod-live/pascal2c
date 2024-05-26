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

static expr2_t *simple_type_list(parser_t *parser);
static expr2_t *simple_type_normal(parser_t *parser);

expr2_t *
parser2_simple_type(parser_t *parser)
{
	token_t *peek;

	peek = parser_peek(parser);
	if (peek->type == TOK_LPAREN) {
		return simple_type_list(parser);
	} else {
		return simple_type_normal(parser);
	}
}

static expr2_t *
simple_type_list(parser_t *parser)
{
	expr2_t *expr;
	expr2_t **identifiers = NULL;
	int next_pos, count = 0;
	token_t *next_token;

	parser_token_expect(parser, TOK_LPAREN);
	do {
		next_pos = count;
		count++;
		identifiers = realloc(identifiers, sizeof(expr2_t *) * count);
		identifiers[next_pos] = parser2_identifier(parser);

		next_token = parser_token(parser);
	} while (next_token->type == TOK_COMMA);

	if (next_token->type != TOK_RPAREN) {
		parser_error(parser,
		             next_token,
		             "Not a COMMA and not an RPAREN");
	}

	expr = expression_new(EXP_SIMPLE_TYPE);
	E_SIMPLE_TYPE(*expr).type = SIMPLE_TYPE_LIST;
	E_SIMPLE_TYPE(*expr).components = identifiers;
	E_SIMPLE_TYPE(*expr).component_count = count;
	return expr;
}

static expr2_t *
simple_type_normal(parser_t *parser)
{
	token_t *next_token;
	expr2_t *exp_st;
	expr2_t **exp_array;

	exp_array = malloc(2 * sizeof(expr2_t *));
	exp_array[0] = parser2_constant(parser);
	next_token = parser_peek(parser);

	exp_st = expression_new(EXP_SIMPLE_TYPE);
	E_SIMPLE_TYPE(*exp_st).components = exp_array;
	if (next_token->type == TOK_DOTDOT) {
		// Remove the DOTDOT.
		parser_token(parser);

		E_SIMPLE_TYPE(*exp_st).type = SIMPLE_TYPE_RANGE;
		exp_array[1] = parser2_constant(parser);
		E_SIMPLE_TYPE(*exp_st).component_count = 2;
	} else {
		E_SIMPLE_TYPE(*exp_st).type = SIMPLE_TYPE_SINGLE;
		E_SIMPLE_TYPE(*exp_st).component_count = 1;
	}

	return exp_st;
}
