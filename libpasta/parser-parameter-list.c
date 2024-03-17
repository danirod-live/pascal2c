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

/*
 * Expression nodes for the parameter lists.
 *
 * If the parameter list is empty, then the expression node values NULL.
 *
 * Otherwise, take each one of the groups of the parameter list, separated
 * by semicolons. As in, take "a, b, c: integer" and "d, e: real" out of
 * "a, b, c: integer; d, e: real".
 *
 * The apex of the returned expression node is a binary node whose token is
 * an LPAREN. Its left child is a parameter group, and its right child
 * is either the literal RPAREN if that was the last parameter group, or
 * a binary with the next parameter group. Each consecutive parameter
 * group is rooted by the token SEMICOLON and it is recursive: it's always
 * the value of the parameter group on the left child, and the next
 * parameter group in the right child, if applies.
 *
 * For example, in (a, b, c: integer; d, e: real) you get the following:
 *
 * - BINARY (LPAREN)
 *   |- The expression for a, b, c : integer
 *   |- BINARY (SEMICOLON)
 *      |- The expression for d, e: real
 *      |- LITERAL (RPAREN)
 *
 * A more elaborate approach for (a, b: integer; c, d: real; e: string):
 *
 * - BINARY (LPAREN)
 *   |- The expression for a, b : integer
 *   |- BINARY (SEMICOLON)
 *      |- The expression for c, d : real
 *      |- BINARY (SEMICOLON)
 *         |- The expression for e : string
 *         |- LITERAL (RPAREN)
 *
 * In regards to each parameter set, it is a chain of unary expressions,
 * with each child pointing to the next. The root of this chain is the
 * token IDENTIFIER for the data type, and consecutive childs are each
 * one of the parameters, in left to right order, such as:
 *
 * - a, b, c: integer :: IDENT(integer) > IDENT(a) > IDENT(b) > IDENT(c)
 * - d, e: real :: IDENT(real) > IDENT(d) > IDENT(e)
 *
 * However, if a parameter group has the VAR keyword, then the first node
 * is actually a binary tree where the left child is the token VAR, and
 * the right child is the chain of variable identifiers.
 *
 * So for a complete example of (a, b, c: integer; VAR d, e: real):
 *
 * - BINARY (LPAREN)
 *   |- UNARY (IDENTIFIER(integer))
 *   |  |- UNARY (IDENTIFIER(a))
 *   |     |- UNARY (IDENTIFIER(b))
 *   |        |- UNARY (IDENTIFIER(c))
 *   |- BINARY (SEMICOLON)
 *      |- BINARY (IDENTIFIER(real))
 *      |  |- LITERAL (VAR)
 *      |  |- UNARY (IDENTIFIER(d))
 *      |     |- UNARY (IDENTIFIER(e))
 *      |- LITERAL (RPAREN)
 */

static expr_t *do_parse_idtype_block(parser_t *);
static expr_t *do_parse_parameter_list(parser_t *);

expr_t *
parser_parameter_list(parser_t *parser)
{
	expr_t *root = NULL;

	if (parser_peek(parser)->type == TOK_LPAREN) {
		if (parser_peek_far(parser, 1)->type != TOK_RPAREN) {
			root = do_parse_parameter_list(parser);
		}
	}
	return root;
}

/* The actual function, called only if parameter list is not empty. */
static expr_t *
do_parse_parameter_list(parser_t *parser)
{
	token_t *token;
	expr_t *root = NULL, *next;

	/* Apex of the entire parameter list structure. */
	token = parser_token_expect(parser, TOK_LPAREN);

	for (;;) {
		/* Advance the chain. */
		if (root == NULL) {
			root = new_binary(token, NULL, NULL);
			next = root;
		} else {
			next->exp_right = new_binary(token, NULL, NULL);
			next = next->exp_right;
		}

		/* Parse left child. */
		next->exp_left = do_parse_idtype_block(parser);

		/* Parse right child. */
		token = parser_token(parser);
		if (token->type == TOK_RPAREN) {
			/* We done. */
			next->exp_right = new_literal(token);
			break;
		} else if (token->type != TOK_SEMICOLON) {
			parser_error(parser, token, "Expected ) or ;");
		}

		/* If we reach here, token is a SEMICOLON. The value is carried
		 * over to the next iteration, where it is added as the token of
		 * the following child in the chain.
		 */
	}
	return root;
}

/* A helper for parsing the [VAR] [ident list] : [type] */
static expr_t *
do_parse_idtype_block(parser_t *parser)
{
	token_t *type;
	expr_t *parlist, *var = NULL;

	if (parser_peek(parser)->type == TOK_VAR)
		var = new_literal(parser_token(parser));
	parlist = parser_identifier_list(parser);
	parser_token_expect(parser, TOK_COLON);
	type = parser_token_expect(parser, TOK_IDENTIFIER);

	if (var == NULL) {
		return new_unary(type, parlist);
	} else {
		return new_binary(type, var, parlist);
	}
}
