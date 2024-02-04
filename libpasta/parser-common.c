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

/**
 * Gets the next token from the parser input, but also tests that it is
 * a token of the given type. if the token is not of such type, it will
 * fail.
 */
token_t *
parser_token_expect(parser_t *parser, tokentype_t type)
{
	token_t *token;

	token = parser_token(parser);
	if (token->type != type) {
		parser_error(parser, token, "Token is not of expected type");
	}
	return token;
}

/**
 * Parses a comma separated list of identifiers, such as a, b, c.
 * Returns a chain of UNARY nodes in linked list mode where each node
 * is an identifier, sorted left to right.
 *
 * For instance, for the identifier list "a, b, c", the following is
 * generated:
 *
 * - UNARY IDENTIFIER(a)
 *   |- UNARY IDENTIFIER(b)
 *      |- UNARY IDENTIFIER(c)
 */
expr_t *
parser_identifier_list(parser_t *parser)
{
	expr_t *root = NULL, *next;
	token_t *token;

	for (;;) {
		/* Consume the next identifier and add it to the linked list. */
		token = parser_token_expect(parser, TOK_IDENTIFIER);
		if (!root) {
			root = new_unary(token, NULL);
			next = root;
		} else {
			next->exp_left = new_unary(token, NULL);
			next = next->exp_left;
		}

		/* Are there more tokens to parse? */
		token = parser_peek(parser);
		if (token->type != TOK_COMMA) {
			break;
		}
		parser_token_expect(parser, TOK_COMMA);
	}
	return root;
}
