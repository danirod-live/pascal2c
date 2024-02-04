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

static int simex_follows_plusminus(parser_t *parser);
static expr_t *clean_expression(expr_t *expression);

/*
 * Expression node for an expression.
 *
 * These kind of nodes represents the lowest operator precedende for an entire
 * expression tree. It is one or two simple expressions, which have a higher
 * precedence. If there are two simple expressions, they will be linked by an
 * operator: < > <= >= = <>.
 *
 * For expressions that are only made of simple expressions, the node will be a
 * grouping with the inner simple expression. Such is the case for "3":
 *
 * - GROUPING: The simple expression for "3"
 *
 * When the expression has two simple expressions joined by an operator, the
 * node will be a binary, with the operator as token and both operands as left
 * and right child. This is the case for "x > 4":
 *
 * - BINARY (GREATER)
 *   - GROUPING: The simple expression that represents x
 *   - GROUPING: The simple expression that represents 4
 */
expr_t *
parser_expression(parser_t *parser)
{
	expr_t *expr, *second;
	token_t *token;

	expr = parser_simple_expression(parser);
	token = parser_peek(parser);
	switch (token->type) {
	case TOK_GREATER:
	case TOK_GREATEQL:
	case TOK_LESSER:
	case TOK_LESSEQL:
	case TOK_EQUAL:
	case TOK_NEQUAL:
	case TOK_IN:
		parser_token(parser);
		second = parser_simple_expression(parser);
		return new_binary(token, expr, second);
	default:
		return clean_expression(new_grouping(expr));
	}
}

/*
 * Expression nodes for the simple expression.
 *
 * A simple expression contains one or more terms, which have higher precedende
 * than simple expressions, separated between + - and OR.  In a similar fashion
 * to other members of a global expression tree, it is either a grouping of a
 * term, or a binary of two terms with the operator as an apex.
 *
 * The oddity in simple expressions is that they optionally accept a plus and a
 * minus in front of the first term. To simplify the operation, at the moment
 * if the first element is one of these optional operators, we take it out and
 * yield an unary of the prefix and the remaining expression.
 *
 * As an example, for the simple expression "2", you get the following:
 *
 * - GROUPING: the term that represents "2"
 *
 * For the simple expression "2 + 3", you get the following:
 *
 * - BINARY (PLUS)
 *   - GROUPING: The term that represents 2
 *   - GROUPING: The term that represents 3
 *
 * Multiple terms form nested binary nodes, such as "1 + 2 + 3":
 *
 * - BINARY (PLUS)
 *   - GROUPING: The term that represents 1
 *   - BINARY (PLUS)
 *     - GROUPING: The term that represents 2
 *     - GROUPING: The term that represents 3
 *
 * In the following case, the expression is "-4 + 2 OR 3":
 *
 * - UNARY (-)
 *   - BINARY (PLUS)
 *     - GROUPING: The term that represents 4
 *     - BINARY (OR)
 *       - GROUPING: The term that represents 2
 *       - GROUPING: The term that represents 3
 */
expr_t *
parser_simple_expression(parser_t *parser)
{
	expr_t *expr;
	token_t *token;

	/* Unwrap a plus minus prefix. Note that in the graphs this is
	 * represented as an arrow before the first node. To make this easier to
	 * solve, I do this recursively. However, keep in mind that no two
	 * consecutive operators are allowed, so we must check that a valid term
	 * symbol follows the prefix. */
	if (simex_follows_plusminus(parser)) {
		token = parser_token(parser);
		if (simex_follows_plusminus(parser)) {
			parser_error(parser, token, "double operator");
		}
		return new_unary(token, parser_simple_expression(parser));
	}

	expr = parser_term(parser);
	token = parser_peek(parser);
	switch (token->type) {
	case TOK_PLUS:
	case TOK_MINUS:
	case TOK_OR:
		parser_token(parser);
		if (simex_follows_plusminus(parser)) {
			parser_error(parser, token, "double operator");
		}
		return new_binary(token,
		                  expr,
		                  parser_simple_expression(parser));
	default:
		return clean_expression(new_grouping(expr));
	}
}

/*
 * [factor] = GROUPING(f)
 * [f1] * [f2] = BINARY(*, f1, f2)
 * [f1] * [f2] / [f3] = BINARY(/, BINARY(*, f1, f2), f3)
 */
expr_t *
parser_term(parser_t *parser)
{
	expr_t *factor;
	token_t *token;

	factor = parser_factor(parser);
	token = parser_peek(parser);
	switch (token->type) {
	case TOK_ASTERISK:
	case TOK_SLASH:
	case TOK_DIV:
	case TOK_MOD:
	case TOK_AND:
		parser_token(parser);
		return new_binary(token, factor, parser_term(parser));
	default:
		return clean_expression(new_grouping(factor));
	}
}

/*
 * Extracts the comma separated list of expressions between brackets
 * in the IDENTIFIER branch of the FACTOR expression tree.
 */
static expr_t *
factor_id_expression_list(parser_t *parser)
{
	token_t *token;
	expr_t *root, *next;

	token = parser_token_expect(parser, TOK_LPAREN);
	root = new_binary(token, parser_expression(parser), NULL);
	next = root;

	for (;;) {
		token = parser_token(parser);
		switch (token->type) {
		case TOK_RPAREN:
			next->exp_right = new_literal(token);
			return root;
		case TOK_COMMA:
			next->exp_right =
			    new_binary(token, parser_expression(parser), NULL);
			next = next->exp_right;
			break;
		default:
			parser_error(parser, token, "Expected ) or ,");
		}
	}
}

static expr_t *
factor_id_set(parser_t *parser)
{
	token_t *token;
	expr_t *root, *next;

	token = parser_token_expect(parser, TOK_LBRACKET);
	root = new_binary(token, parser_expression(parser), NULL);
	next = root;

	for (;;) {
		token = parser_token(parser);

		/* Check if it is the end of a range. */
		if (token->type == TOK_DOTDOT) {
			next->exp_left = new_binary(token,
			                            next->exp_left,
			                            parser_expression(parser));
			token = parser_token(parser);
		}

		/* Is this the end? */
		switch (token->type) {
		case TOK_RBRACKET:
			next->exp_right = new_literal(token);
			return root;
		case TOK_COMMA:
			next->exp_right =
			    new_binary(token, parser_expression(parser), NULL);
			next = next->exp_right;
			break;
		default:
			parser_error(parser, token, "unexpected");
		}
	}
}

expr_t *
parser_factor(parser_t *parser)
{
	token_t *token, *token2;
	expr_t *expr;

	token = parser_peek(parser);
	switch (token->type) {
	case TOK_IDENTIFIER:
		token2 = parser_peek_far(parser, 1);
		switch (token2->type) {
		case TOK_LBRACKET:
		case TOK_DOT:
		case TOK_CARET:
			expr = parser_variable(parser);
			break;
		case TOK_LPAREN:
			parser_token(parser);
			expr =
			    new_unary(token, factor_id_expression_list(parser));
			break;
		default:
			expr = parser_unsigned_constant(parser);
		}
		break;
	case TOK_DIGIT:
	case TOK_NIL:
	case TOK_STRING:
		expr = parser_unsigned_constant(parser);
		break;
	case TOK_NOT:
		parser_token_expect(parser, TOK_NOT);
		expr = new_unary(token, parser_factor(parser));
		break;
	case TOK_LPAREN:
		parser_token(parser);
		expr = parser_expression(parser);
		parser_token_expect(parser, TOK_RPAREN);
		break;
	case TOK_LBRACKET:
		expr = factor_id_set(parser);
		break;
	default:
		parser_error(parser, token, "Unexpected type");
	}

	return expr;
}

/* Returns true if peeking the parser shows a plus or a minus. */
static int
simex_follows_plusminus(parser_t *parser)
{
	token_t *token = parser_peek(parser);
	return token->type == TOK_PLUS || token->type == TOK_MINUS;
}

static expr_t *
clean_expression(expr_t *expr)
{
	expr_t *nested;

	if (expr->type == GROUPING && expr->exp_left->type == GROUPING) {
		nested = expr->exp_left;
		expr_free(expr);
		return clean_expression(nested);
	}

	return expr;
}
