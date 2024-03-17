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

expr_t *
parser_simple_type(parser_t *parser)
{
	token_t *next_symbol, *next_id;
	expr_t *root, *next_node;

	next_symbol = parser_peek(parser);
	if (next_symbol->type == TOK_LPAREN) {
		next_symbol = parser_token_expect(parser, TOK_LPAREN);
		// Branch 2 - (identifiers separated by commas inside brackets)
		root = new_binary(next_symbol, NULL, NULL);
		next_node = root;

		while (1) {
			// Assign left branch
			next_node->exp_left = parser_identifier(parser);

			// Pick what goes on the right branch
			next_symbol = parser_token(parser);
			if (next_symbol->type == TOK_RPAREN) {
				next_node->exp_right = new_literal(next_symbol);
				break;
			} else if (next_symbol->type == TOK_COMMA) {
				next_node->exp_right =
				    new_binary(next_symbol, NULL, NULL);
				next_node = next_node->exp_right;
			} else {
				parser_error(parser,
				             next_symbol,
				             "Expected either COMMA or RPAREN");
			}
		}
	} else {
		next_node = parser_constant(parser);
		next_symbol = parser_peek(parser);
		if (next_symbol->type == TOK_DOTDOT) {
			// Branch 3 - (two identifiers between a ..)
			next_symbol = parser_token_expect(parser, TOK_DOTDOT);
			root = new_binary(next_symbol, NULL, NULL);
			root->exp_left = next_node;
			root->exp_right = parser_constant(parser);
		} else if (next_symbol->type == TOK_LBRACKET) {
			next_symbol = parser_token_expect(parser, TOK_LBRACKET);
			root = new_binary(next_symbol, NULL, NULL);
			root->exp_left = next_node;
			root->exp_right = parser_expression(parser);
			parser_token_expect(parser, TOK_RBRACKET);
		} else {
			// Branch 1 - identifier alone
			root = new_grouping(next_node);
		}
	}

	return root;
}
