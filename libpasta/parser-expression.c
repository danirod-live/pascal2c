#include "parser.h"
#include "token.h"

static int
parser_follows_plus_or_minus(parser_t *parser)
{
	token_t *token = parser_peek(parser);
	return token->type == TOK_PLUS || token->type == TOK_MINUS;
}

expr_t *
parser_expression(parser_t *parser)
{
	expr_t *expr;
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
		return new_binary(token,
		                  expr,
		                  parser_simple_expression(parser));
	default:
		return new_grouping(expr);
	}
}

expr_t *
parser_simple_expression(parser_t *parser)
{
	expr_t *expr;
	token_t *token;

	/* If start with + or -, take everything. */
	if (parser_follows_plus_or_minus(parser)) {
		token = parser_token(parser);
		if (parser_follows_plus_or_minus(parser)) {
			parser_error(parser,
			             token,
			             "Unexpected double operator");
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
		if (parser_follows_plus_or_minus(parser)) {
			parser_error(parser,
			             token,
			             "Unexpected double operator");
		}
		return new_binary(token,
		                  expr,
		                  parser_simple_expression(parser));
	default:
		return new_grouping(expr);
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
		return new_grouping(factor);
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
