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
parser_type(parser_t *parser)
{
	token_t *next_token, *packed = NULL;
	expr_t *root, *next_expr;

	next_token = parser_peek(parser);

	if (next_token->type == TOK_PACKED) {
		packed = next_token;
		parser_token_expect(parser, TOK_PACKED);
		next_token = parser_peek(parser);
	}

	switch (next_token->type) {
	case TOK_CARET:
		if (packed) {
			parser_error(parser,
			             next_token,
			             "CARET cannot be PACKED");
		}
		root = new_unary(next_token, NULL);
		parser_token_expect(parser, TOK_CARET);
		root->exp_left = parser_identifier(parser);
		break;
	case TOK_ARRAY:
		// Consume TOK_ARRAY
		root = new_binary(next_token, NULL, NULL);
		parser_token_expect(parser, TOK_ARRAY);

		// Consume the list of simple types that goes between []
		next_token = parser_token_expect(parser, TOK_LBRACKET);
		root->exp_left = new_binary(next_token, NULL, NULL);
		next_expr = root->exp_left;

		while (1) {
			next_expr->exp_left = parser_simple_type(parser);

			next_token = parser_token(parser);
			if (next_token->type == TOK_COMMA) {
				next_expr->exp_right =
				    new_binary(next_token, NULL, NULL);
				next_expr = next_expr->exp_right;
			} else if (next_token->type == TOK_RBRACKET) {
				next_expr->exp_right = new_literal(next_token);
				break;
			} else {
				parser_error(
				    parser,
				    next_token,
				    "Expected either RBRACKET or COMMA");
			}
		}

		parser_token_expect(parser, TOK_OF);
		root->exp_right = parser_type(parser);
		break;
	case TOK_FILE:
		// Consume TOK_FILE
		root = new_unary(next_token, NULL);
		parser_token_expect(parser, TOK_FILE);

		// Must follow an OF
		parser_token_expect(parser, TOK_OF);

		// Recursive definition
		root->exp_left = parser_type(parser);
		break;
	case TOK_SET:
		// Consume TOK_SET
		root = new_unary(next_token, NULL);
		parser_token_expect(parser, TOK_SET);

		// Must follow an OF
		parser_token_expect(parser, TOK_OF);

		// Must follow a simple type
		root->exp_left = parser_simple_type(parser);
		break;
	case TOK_RECORD:
		// Consume RECORD
		root = new_unary(next_token, NULL);
		parser_token_expect(parser, TOK_RECORD);

		root->exp_left = parser_field_list(parser);

		// Must follow an END.
		parser_token_expect(parser, TOK_END);
		break;
	default:
		if (packed) {
			parser_error(parser,
			             next_token,
			             "Cannot use PACKED in this context");
		}
		root = parser_simple_type(parser);
		break;
	}

	// Wrap in a PACKED if we previously saw the packed keyword.
	if (packed) {
		root = new_unary(packed, root);
	}

	return root;
}