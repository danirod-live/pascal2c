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
static expr_t *ifthen(parser_t *parser);
static expr_t *begin(parser_t *parser);
static expr_t *repeat(parser_t *parser);
static expr_t *repeat_stmts(parser_t *parser);
static expr_t *whiledo(parser_t *parser);
static expr_t *forloop(parser_t *parser);
static expr_t *caseof(parser_t *parser);
static expr_t *caselist(parser_t *parser);
static expr_t *constlist(parser_t *parser);
static expr_t *with(parser_t *parser);
static expr_t *gotostmt(parser_t *parser);
static expr_t *exitstmt(parser_t *parser);

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
	case TOK_IF:
		return ifthen(parser);
	case TOK_REPEAT:
		return repeat(parser);
	case TOK_WHILE:
		return whiledo(parser);
	case TOK_FOR:
		return forloop(parser);
	case TOK_CASE:
		return caseof(parser);
	case TOK_WITH:
		return with(parser);
	case TOK_GOTO:
		return gotostmt(parser);
	case TOK_EXIT:
		return exitstmt(parser);
	default:
		/* Most probably an empty expression. */
		return NULL;
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
	expr_t *args, *ident = parser_identifier(parser);
	token_t *token = parser_peek(parser);
	if (token->type == TOK_LPAREN) {
		/* Arguments of the function call. */
		args = arguments(parser);
		if (args != NULL) {
			return new_binary(token, ident, args);
		}
	}
	return ident;
}

static expr_t *
arguments(parser_t *parser)
{
	token_t *following, *lparen = parser_token_expect(parser, TOK_LPAREN);
	expr_t *root, *expr, *next;

	/* If the parenthesis are empty, there are no arguments. */
	following = parser_peek(parser);
	if (following->type == TOK_RPAREN) {
		parser_token_expect(parser, TOK_RPAREN);
		return NULL;
	}

	root = new_binary(lparen, NULL, NULL);
	next = root;
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

static expr_t *
ifthen(parser_t *parser)
{
	token_t *iftoken = parser_token_expect(parser, TOK_IF);
	expr_t *condition = parser_expression(parser);
	token_t *thentoken = parser_token_expect(parser, TOK_THEN);
	expr_t *iftrue = parser_statement(parser);
	token_t *maybeelse = parser_peek(parser);

	expr_t *thenbranch = new_binary(thentoken, iftrue, NULL);
	expr_t *root = new_binary(iftoken, condition, thenbranch);

	if (maybeelse->type == TOK_ELSE) {
		token_t *elsetoken = parser_token_expect(parser, TOK_ELSE);
		expr_t *iffalse = parser_statement(parser);
		thenbranch->exp_right = new_unary(elsetoken, iffalse);
	}

	return root;
}

static expr_t *
repeat(parser_t *parser)
{
	token_t *repeattoken = parser_token_expect(parser, TOK_REPEAT);
	expr_t *statements = repeat_stmts(parser);
	token_t *untilkw = parser_token_expect(parser, TOK_UNTIL);
	expr_t *condition = parser_expression(parser);
	return new_binary(repeattoken,
	                  statements,
	                  new_unary(untilkw, condition));
}

static expr_t *
repeat_stmts(parser_t *parser)
{
	expr_t *root = NULL, *next, *stmt;
	token_t *token;

	for (;;) {
		stmt = parser_statement(parser);
		token = parser_peek(parser);
		if (token->type == TOK_SEMICOLON) {
			parser_token_expect(parser, TOK_SEMICOLON);
			if (root == NULL) {
				root = new_binary(token, stmt, NULL);
				next = root;
			} else {
				next->exp_right = new_binary(token, stmt, NULL);
				next = next->exp_right;
			}
		} else if (token->type == TOK_UNTIL) {
			if (root == NULL) {
				root = new_grouping(stmt);
			} else {
				next->exp_right = new_grouping(stmt);
			}
			return root;
		} else {
			parser_error(parser,
			             token,
			             "Expected semicolon or until");
		}
	}
}

static expr_t *
whiledo(parser_t *parser)
{
	token_t *whiletoken = parser_token_expect(parser, TOK_WHILE);
	expr_t *whileexpr = parser_expression(parser);
	parser_token_expect(parser, TOK_DO);
	expr_t *whilestmt = parser_statement(parser);
	return new_binary(whiletoken, whileexpr, whilestmt);
}

/*
 * FOR
 * |- <ident>
 * |  |- TO | DOWNTO
 * |     |- <start expr>
 * |     |- <end expr>
 * |-<expr>
 */
static expr_t *
forloop(parser_t *parser)
{
	token_t *fortoken = parser_token_expect(parser, TOK_FOR);
	expr_t *ident = parser_identifier(parser);
	parser_token_expect(parser, TOK_ASSIGN);
	expr_t *startexpr = parser_expression(parser);
	token_t *todownto = parser_token(parser);
	expr_t *endexpr = parser_expression(parser);
	parser_token_expect(parser, TOK_DO);
	expr_t *stmt = parser_statement(parser);

	if (todownto->type != TOK_TO && todownto->type != TOK_DOWNTO) {
		parser_error(parser, todownto, "Expected either TO or DOWNTO");
	}

	return new_binary(
	    fortoken,
	    new_unary(ident->token, new_binary(todownto, startexpr, endexpr)),
	    stmt);
}

static expr_t *
caseof(parser_t *parser)
{
	token_t *casetoken = parser_token_expect(parser, TOK_CASE);
	expr_t *expr = parser_expression(parser);
	parser_token_expect(parser, TOK_OF);
	expr_t *cases = caselist(parser);
	return new_binary(casetoken, expr, cases);
}

static expr_t *
caselist(parser_t *parser)
{
	expr_t *root = NULL, *next;
	expr_t *consts, *stmt, *caseitem;
	token_t *colon, *separator, *peek;
	for (;;) {
		consts = constlist(parser);
		colon = parser_token_expect(parser, TOK_COLON);
		stmt = parser_statement(parser);
		caseitem = new_binary(colon, consts, stmt);

		separator = parser_token(parser);
		if (root == NULL) {
			root = new_binary(separator, caseitem, NULL);
			next = root;
		} else {
			next->exp_right = new_binary(separator, caseitem, NULL);
			next = next->exp_right;
		}
		switch (separator->type) {
		case TOK_END:
			// We are done here.
			return root;
		case TOK_SEMICOLON:
			// Semicolon and END is valid. Check for this.
			peek = parser_peek(parser);
			if (peek->type == TOK_END) {
				parser_token(parser);
				return root;
			}
			// We have another case.
			break;
		default:
			parser_error(parser,
			             separator,
			             "Unexpected token here");
		}
	}
}

static expr_t *
constlist(parser_t *parser)
{
	expr_t *root = NULL, *next;
	expr_t *constant = parser_constant(parser);
	token_t *peek = parser_peek(parser);
	switch (peek->type) {
	case TOK_COLON:
		// This is the only const we have, so we return it.
		return constant;
	case TOK_COMMA:
		// There is more than a const, so we have to group them.
		break;
	default:
		parser_error(parser, peek, "Unexpected token inside case");
	}

	// If this line is reached, we have more than one const.
	parser_token_expect(parser, TOK_COMMA);
	root = new_binary(peek, constant, NULL);
	next = root;

	// Keep reading constants.
	for (;;) {
		constant = parser_constant(parser);
		peek = parser_peek(parser);
		switch (peek->type) {
		case TOK_COLON:
			next->exp_right = constant;
			return root;
		case TOK_COMMA:
			parser_token_expect(parser, TOK_COMMA);
			next->exp_right = new_binary(peek, constant, NULL);
			next = next->exp_right;
			break;
		default:
			parser_error(parser,
			             peek,
			             "Unexpected token inside case");
		}
	}
}

static expr_t *
variablelist(parser_t *parser)
{
	expr_t *root = NULL, *next;
	expr_t *var = parser_variable(parser);
	token_t *sep = parser_peek(parser);

	switch (sep->type) {
	case TOK_COMMA:
		// We have to read more.
		parser_token(parser);
		break;
	case TOK_DO:
		// Single variable.
		return var;
	default:
		parser_error(parser, sep, "Unexpected token inside WITH");
	}

	root = new_binary(sep, var, NULL);
	next = root;

	for (;;) {
		var = parser_variable(parser);
		sep = parser_peek(parser);

		switch (sep->type) {
		case TOK_COMMA:
			sep = parser_token(parser);
			next->exp_right = new_binary(sep, var, NULL);
			next = next->exp_right;
			break;
		case TOK_DO:
			next->exp_right = var;
			return root;
		default:
			parser_error(parser,
			             sep,
			             "Unexpected token inside WITH");
		}
	}
}

static expr_t *
with(parser_t *parser)
{
	token_t *withword = parser_token_expect(parser, TOK_WITH);
	expr_t *variables = variablelist(parser);
	parser_token_expect(parser, TOK_DO);
	expr_t *stmt = parser_statement(parser);
	return new_binary(withword, variables, stmt);
}

static expr_t *
gotostmt(parser_t *parser)
{
	token_t *gotoword = parser_token_expect(parser, TOK_GOTO);

	// FIXME: maybe these days labels can be alphanumeric as well
	expr_t *gotoaddr = parser_unsigned_integer(parser);
	return new_unary(gotoword, gotoaddr);
}

static expr_t *
exitstmt(parser_t *parser)
{
	token_t *exitword = parser_token(parser);
	parser_token_expect(parser, TOK_LPAREN);

	token_t *peek = parser_peek(parser);
	expr_t *exitparam;
	if (peek->type == TOK_PROGRAM) {
		parser_token(parser);
		exitparam = new_literal(peek);
	} else {
		exitparam = parser_identifier(parser);
	}

	parser_token_expect(parser, TOK_RPAREN);
	return new_unary(exitword, exitparam);
}
