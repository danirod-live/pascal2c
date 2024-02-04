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

/* TODO: How does this work? */
expr_t *
parser_variable(parser_t *parser)
{
	expr_t *root, *suffix = NULL, *ext = suffix;
	token_t *token;

	token = parser_token(parser);
	if (token->type != TOK_IDENTIFIER) {
		parser_error(parser,
		             token,
		             "Unexpected token is not an IDENTIFIER");
	}

	// TODO: should not allow an space here.
	token = parser_peek(parser);
	for (;;) {
		switch (token->type) {
		case TOK_CARET:
			token = parser_token(parser);
			if (suffix == NULL) {
				suffix = new_unary(token, NULL);
				ext = suffix;
			} else {
				ext->exp_left = new_unary(token, NULL);
				ext = ext->exp_left;
			}
			break;
		case TOK_LBRACKET:
			token = parser_token(parser);
			if (suffix == NULL) {
			} else {
			}
			break;
		case TOK_DOT:
			token = parser_token(parser);
			if (suffix == NULL) {
				suffix = new_binary(token, NULL, NULL);
				ext = suffix;
			} else {
				ext->exp_left = new_binary(token, NULL, NULL);
				ext = ext->exp_left;
			}
			token = parser_token(parser);
			if (token->type != TOK_IDENTIFIER) {
				parser_error(parser,
				             token,
				             "Expected TOK_IDENTIFIER");
			}
			ext->exp_right = new_literal(token);
			break;
		default:
			root = new_unary(token, suffix);
			return root;
		}
	}

	// Should not reach here!
	return NULL;
}