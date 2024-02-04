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

static int follows_label(parser_t *parser);
static expr_t *assignment_or_procedure(parser_t *parser);
static expr_t *assignment(parser_t *parser);
static expr_t *procedure(parser_t *parser);
static expr_t *arguments(parser_t *parser);
static expr_t *begin(parser_t *parser);

expr_t *
parser_statement(parser_t *parser)
{
	token_t *peek;

	if (follows_label(parser)) {
		// TODO: Need to take the label. Drop it for now.
		parser_token(parser);
		parser_token(parser);
	}

	peek = parser_peek(parser);
	switch (peek->type) {
	case TOK_IDENTIFIER:
		return assignment_or_procedure(parser);
	case TOK_BEGIN:
		return begin(parser);
	case TOK_END: /* Most probably this is an empty expression. */
	case TOK_SEMICOLON:
		return NULL;
	default:
		parser_error(parser, peek, "Not implemented yet");
	}
}

static int
follows_label(parser_t *parser)
{
	token_t *label, *colon;

	label = parser_peek(parser);
	switch (label->type) {
	case TOK_IDENTIFIER:
	case TOK_DIGIT:
		break;
	default:
		return 0;
	}

	colon = parser_peek_far(parser, 1);
	return colon->type == TOK_COLON;
}

static expr_t *
assignment_or_procedure(parser_t *parser)
{
	token_t *symbol = parser_peek_far(parser, 1);

	switch (symbol->type) {
	case TOK_LBRACKET:
	case TOK_DOT:
	case TOK_CARET:
	case TOK_ASSIGN:
		return assignment(parser);
	case TOK_LPAREN:
	default:
		return procedure(parser);
	}
}

static expr_t *
assignment(parser_t *parser)
{
	expr_t *variable = parser_variable(parser);
	token_t *assign = parser_token_expect(parser, TOK_ASSIGN);
	expr_t *expr = parser_expression(parser);
	return new_binary(assign, variable, expr);
}

static expr_t *
procedure(parser_t *parser)
{
	expr_t *ident = parser_identifier(parser);
	token_t *token = parser_peek(parser);
	if (token->type == TOK_LPAREN) {
		/* Arguments of the function call. */
		return new_binary(token, ident, arguments(parser));
	} else {
		return ident;
	}
}

static expr_t *
arguments(parser_t *parser)
{
	token_t *lparen = parser_token_expect(parser, TOK_LPAREN);
	token_t *following;
	expr_t *root = new_binary(lparen, NULL, NULL);
	expr_t *expr;
	expr_t *next = root;

	for (;;) {
		expr = parser_expression(parser);
		next->exp_left = expr;

		following = parser_token(parser);
		switch (following->type) {
		case TOK_COMMA:
			next->exp_right = new_binary(following, NULL, NULL);
			next = next->exp_right;
			break;
		case TOK_RPAREN:
			next->exp_right = new_literal(following);
			return root;
		default:
			parser_error(parser, following, "Unexpected argument");
		}
	}
}

static expr_t *
begin(parser_t *parser)
{
	token_t *begin = parser_token_expect(parser, TOK_BEGIN);
	token_t *following;
	expr_t *root = new_binary(begin, NULL, NULL);
	expr_t *next = root;

	for (;;) {
		next->exp_left = parser_statement(parser);
		following = parser_token(parser);
		switch (following->type) {
		case TOK_SEMICOLON:
			next->exp_right = new_binary(following, NULL, NULL);
			next = next->exp_right;
			break;
		case TOK_END:
			next->exp_right = new_literal(following);
			return root;
		default:
			parser_error(parser, following, "Unexpected token");
		}
	}
}