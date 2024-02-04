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

static expr_t *parser_field_list_branch(parser_t *parser);
static expr_t *parse_constant_list(parser_t *parser);

/* TODO: How does this work? */
expr_t *
parser_field_list(parser_t *parser)
{
	expr_t *root = NULL, *next;
	expr_t *left, *idents, *type;
	token_t *token, *tokenpeek;

	/* Stage 1: normal fields list. */
	for (;;) {
		/* is a new identifier incoming? */
		token = parser_peek(parser);
		if (token->type != TOK_IDENTIFIER) {
			break;
		}

		/* build a new left_node for next_expr with the metadata of
		 * the current field list line. assuming that the line
		 * has the format [idents] : [type], this will be a binary
		 * node using this exact format: [idents] : [type] */

		/* get the identifier list separated by commas. */
		idents = parser_identifier_list(parser);
		token = parser_token(parser);
		if (token->type != TOK_COLON) {
			parser_error(parser,
			             token,
			             "COLON expected after field list");
		}
		type = parser_type(parser);

		/* craft the node and add it to the list. */
		left = new_binary(token, idents, type);

		if (!root) {
			root = new_binary(NULL, left, NULL);
			next = root;
		} else {
			next->exp_right = new_binary(NULL, left, NULL);
			next = next->exp_right;
		}

		/* there may be a semicolon here. */
		token = parser_peek(parser);
		if (token->type != TOK_SEMICOLON) {
			return root;
		}
		parser_token_expect(parser, TOK_SEMICOLON);
	}

	/* Stage 2: case line -- if there is one at all. */
	token = parser_peek(parser);
	if (token->type == TOK_CASE) {
		parser_token_expect(parser, TOK_CASE);

		/* two choices: [case x : t of] or [case t of]
		 * we will always craft a node rooted by of, with t on
		 * right there should always be an identifier following
		 * case, if that identifier is followed by : there is
		 * another one, else should be an of. */
		token = parser_token(parser);
		if (token->type != TOK_IDENTIFIER) {
			parser_error(parser,
			             token,
			             "Expected an identifier following "
			             "the CASE");
		}

		tokenpeek = parser_token(parser);
		switch (tokenpeek->type) {
		case TOK_OF: /* [case t of] */
			left = new_unary(tokenpeek, new_literal(token));
			break;
		case TOK_COLON: /* [case x : t of] */
			left = new_binary(tokenpeek, NULL, new_literal(token));
			token = parser_token(parser);
			if (token->type != TOK_IDENTIFIER) {
				parser_error(parser,
				             token,
				             "Expected an identifier following "
				             "the COLON");
			}

			tokenpeek = parser_token(parser);
			if (tokenpeek->type != TOK_OF) {
				parser_error(parser,
				             token,
				             "Expected OF after secondd token");
			}
			left->token = tokenpeek;
			left->exp_left = new_literal(token);
			break;
		default:
			parser_error(parser,
			             token,
			             "Expected either a COLON or OF");
		}

		if (!root) {
			root = new_binary(NULL, left, NULL);
			next = root;
		} else {
			next->exp_right = new_binary(NULL, left, NULL);
			next = next->exp_right;
		}

		/* and now comes a list of possible values for this case
		 * with their field lists. it is mandatory to have at
		 * least one, but there may be more. */
		left = parser_field_list_branch(parser);
		next->exp_right = new_binary(NULL, left, NULL);
		next = next->exp_right;

		for (;;) {
			/* if a semicolon continues, there is still
			 * more. */
			token = parser_peek(parser);
			if (token->type != TOK_SEMICOLON) {
				break;
			}

			/* wait, there's more */
			parser_token_expect(parser, TOK_SEMICOLON);
			left = parser_field_list_branch(parser);
			next->exp_right = new_binary(NULL, left, NULL);
			next = next->exp_right;
		}
	} // closes if (token->type == TOK_CASE)

	if (root == NULL) {
		/* there should be at least something, either a field or
		 * a case
		 */
		token = parser_peek(parser);
		parser_error(parser,
		             token,
		             "There should be either IDENT or CASE");
	}

	return root;
}

static expr_t *
parse_constant_list(parser_t *parser)
{
	expr_t *root = NULL, *next, *constant;
	token_t *token;

	for (;;) {
		/* parse the constant and add it to the chain. */
		constant = parser_constant(parser);
		if (!root) {
			root = new_binary(NULL, constant, NULL);
			next = root;
		} else {
			next->exp_right = new_binary(NULL, constant, NULL);
			next = next->exp_right;
		}

		/* check if there are more tokens to parse. */
		token = parser_peek(parser);
		if (token->type != TOK_COMMA) {
			return root;
		}
		parser_token_expect(parser, TOK_COMMA);
	}
}

static expr_t *
parser_field_list_branch(parser_t *parser)
{
	expr_t *constant, *fields;
	token_t *token;

	constant = parse_constant_list(parser);
	token = parser_token(parser);
	if (token->type != TOK_COLON) {
		parser_error(parser, token, "Expected a COLON here");
	}
	parser_token_expect(parser, TOK_LPAREN);
	fields = parser_field_list(parser);
	parser_token_expect(parser, TOK_RPAREN);

	return new_binary(token, constant, fields);
}