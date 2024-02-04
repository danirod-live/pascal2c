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
#include "token.h"

static int has_extra(parser_t *parser);
static expr_t *extra(parser_t *parser);
static expr_t *expression_list(parser_t *parser);

/* TODO: How does this work? */
expr_t *
parser_variable(parser_t *parser)
{
	token_t *ident = parser_token_expect(parser, TOK_IDENTIFIER);
	expr_t *nested;

	/* Check if the identifier comes alone or not. */
	if (has_extra(parser)) {
		nested = extra(parser);
		return new_unary(ident, nested);
	} else {
		return new_literal(ident);
	}
}

static expr_t *
extra(parser_t *parser)
{
	token_t *token;
	expr_t *expr;

	/* We are protected by has_extra, take the token. */
	token = parser_token(parser);
	expr = new_binary(token, NULL, NULL);

	/* Some token types also have meta. */
	if (token->type == TOK_DOT) {
		expr->exp_left = parser_identifier(parser);
	} else if (token->type == TOK_LBRACKET) {
		expr->exp_left = expression_list(parser);
	}

	/* Still more parts. */
	if (has_extra(parser)) {
		expr->exp_right = extra(parser);
	}

	return expr;
}

static int
has_extra(parser_t *parser)
{
	token_t *tok = parser_peek(parser);
	return tok->type == TOK_CARET || tok->type == TOK_DOT
	       || tok->type == TOK_LBRACKET;
}

static expr_t *
expression_list(parser_t *parser)
{
	expr_t *root = NULL, *next, *expr;
	token_t *token;

	for (;;) {
		expr = parser_expression(parser);
		token = parser_peek(parser);

		if (token->type == TOK_RBRACKET) {
			token = parser_token(parser);

			/* No more arguments after the one we currently have. */
			if (root == NULL) {
				root = new_unary(token, expr);
			} else {
				next->exp_right = new_unary(token, expr);
			}
			return root;
		} else if (token->type == TOK_COMMA) {
			token = parser_token(parser);

			if (root == NULL) {
				root = new_binary(token, expr, NULL);
				next = root;
			} else {
				next->exp_right = new_binary(token, expr, NULL);
				next = next->exp_right;
			}
		} else {
			parser_error(parser, token, "Unexpected token");
		}
	}
}
