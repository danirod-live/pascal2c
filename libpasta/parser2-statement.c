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
#include <stdlib.h>

#include "parser.h"

int detect_label(parser_t *);
expr2_t *label_stmt(parser_t *);
expr2_t *assignment_stmt(parser_t *);
expr2_t *assignment_or_function_call(parser_t *parser);
expr2_t *function_call_stmt(parser_t *parser);
expr2_t *begin_stmt(parser_t *parser);
expr2_t *if_stmt(parser_t *parser);
expr2_t *repeat_stmt(parser_t *parser);
expr2_t *while_stmt(parser_t *parser);
expr2_t *for_stmt(parser_t *parser);
expr2_t *case_stmt(parser_t *parser);
expr2_t *case_branch_stmt(parser_t *parser);
expr2_t *with_stmt(parser_t *parser);
expr2_t *goto_stmt(parser_t *parser);
expr2_t *exit_stmt(parser_t *parser);

expr2_t *
parser2_statement(parser_t *parser)
{
	token_t *token;

	if (detect_label(parser)) {
		return label_stmt(parser);
	}

	token = parser_peek(parser);
	switch (token->type) {
	case TOK_BEGIN:
		return begin_stmt(parser);
	case TOK_IF:
		return if_stmt(parser);
	case TOK_REPEAT:
		return repeat_stmt(parser);
	case TOK_WHILE:
		return while_stmt(parser);
	case TOK_FOR:
		return for_stmt(parser);
	case TOK_CASE:
		return case_stmt(parser);
	case TOK_WITH:
		return with_stmt(parser);
	case TOK_GOTO:
		return goto_stmt(parser);
	case TOK_EXIT:
		return exit_stmt(parser);
	default:
		return assignment_or_function_call(parser);
	}
}

expr2_t *
assignment_or_function_call(parser_t *parser)
{
	token_t *token = parser_peek_far(parser, 1);

	switch (token->type) {
	case TOK_LBRACKET:
	case TOK_DOT:
	case TOK_CARET:
	case TOK_ASSIGN:
		return assignment_stmt(parser);
	default:
		return function_call_stmt(parser);
	}
}

expr2_t *
begin_stmt(parser_t *parser)
{
	int next;
	token_t *token;
	expr2_t *aux, *exp = expression_new(EXP_STMT_BEGIN);
	expr_stmt_begin_t *begin = &(E_STMT_BEGIN(*exp));

	parser_token_expect(parser, TOK_BEGIN);
	for (;;) {
		aux = parser2_statement(parser);

		next = begin->statement_count++;
		begin->statements =
		    realloc(begin->statements,
		            sizeof(expr2_t *) * begin->statement_count);
		begin->statements[next] = aux;

		token = parser_token(parser);
		if (token->type == TOK_END) {
			return exp;
		}
		if (token->type != TOK_SEMICOLON) {
			parser_error(parser,
			             token,
			             "Expected SEMICOLON or END");
		}
	}
}

expr2_t *
with_stmt(parser_t *parser)
{
	expr2_t *aux, *exp = expression_new(EXP_STMT_WITH);
	expr_stmt_with_t *with = &(E_STMT_WITH(*exp));
	int next;
	token_t *token;

	parser_token_expect(parser, TOK_WITH);
	for (;;) {
		aux = parser2_variable(parser);

		next = with->variable_count++;
		with->variables =
		    realloc(with->variables,
		            sizeof(expr2_t *) * with->variable_count);
		with->variables[next] = aux;

		token = parser_token(parser);
		if (token->type == TOK_DO) {
			with->statement = parser2_statement(parser);
			return exp;
		} else if (token->type != TOK_COMMA) {
			parser_error(parser, token, "Expected COMMA or DO");
		}
	}
}

expr2_t *
goto_stmt(parser_t *parser)
{
	expr2_t *exp = expression_new(EXP_STMT_GOTO);
	expr_stmt_goto_t *gotostmt = &(E_STMT_GOTO(*exp));
	parser_token_expect(parser, TOK_GOTO);
	gotostmt->identifier = parser2_identifier(parser);
	return exp;
}

expr2_t *
if_stmt(parser_t *parser)
{
	expr2_t *exp = expression_new(EXP_STMT_IF);
	expr_stmt_if_t *ifstmt = &(E_STMT_IF(*exp));
	token_t *peek;

	parser_token_expect(parser, TOK_IF);
	ifstmt->condition = parser2_expression(parser);
	parser_token_expect(parser, TOK_THEN);
	ifstmt->then = parser2_statement(parser);

	peek = parser_peek(parser);
	if (peek->type == TOK_ELSE) {
		parser_token_expect(parser, TOK_ELSE);
		ifstmt->else_ = parser2_statement(parser);
	} else {
		ifstmt->else_ = NULL;
	}

	return exp;
}

expr2_t *
while_stmt(parser_t *parser)
{
	expr2_t *exp = expression_new(EXP_STMT_WHILE);
	expr_stmt_while_t *whilestmt = &(E_STMT_WHILE(*exp));

	parser_token_expect(parser, TOK_WHILE);
	whilestmt->condition = parser2_expression(parser);
	parser_token_expect(parser, TOK_DO);
	whilestmt->statement = parser2_statement(parser);

	return exp;
}

expr2_t *
for_stmt(parser_t *parser)
{
	expr2_t *exp = expression_new(EXP_STMT_FOR);
	expr_stmt_for_t *forstmt = &(E_STMT_FOR(*exp));
	token_t *token;

	parser_token_expect(parser, TOK_FOR);
	forstmt->identifier = parser2_identifier(parser);
	parser_token_expect(parser, TOK_ASSIGN);
	forstmt->start = parser2_expression(parser);

	token = parser_token(parser);
	switch (token->type) {
	case TOK_TO:
		forstmt->direction = 1;
		break;
	case TOK_DOWNTO:
		forstmt->direction = -1;
		break;
	default:
		parser_error(parser, token, "Expected either TO or DOWNTO");
		break;
	}

	forstmt->end = parser2_expression(parser);
	parser_token_expect(parser, TOK_DO);
	forstmt->statement = parser2_statement(parser);

	return exp;
}

expr2_t *
case_stmt(parser_t *parser)
{
	expr2_t *aux, *exp = expression_new(EXP_STMT_CASE);
	expr_stmt_case_t *casestmt = &(E_STMT_CASE(*exp));
	int next;
	token_t *token;

	parser_token_expect(parser, TOK_CASE);
	casestmt->expression = parser2_expression(parser);
	parser_token_expect(parser, TOK_OF);

	for (;;) {
		aux = case_branch_stmt(parser);

		next = casestmt->case_count++;
		casestmt->cases =
		    realloc(casestmt->cases,
		            sizeof(expr2_t *) * casestmt->case_count);
		casestmt->cases[next] = aux;

		token = parser_token(parser);
		if (token->type == TOK_END) {
			return exp;
		} else if (token->type != TOK_SEMICOLON) {
			parser_error(parser,
			             token,
			             "Expected SEMICOLON or END");
		}
	}
}

expr2_t *
case_branch_stmt(parser_t *parser)
{
	expr2_t *aux, *exp = expression_new(EXP_STMT_CASE_BRANCH);
	expr_stmt_case_branch_t *branch = &(E_STMT_CASE_BRANCH(*exp));
	int next;
	token_t *token;

	for (;;) {
		aux = parser2_constant(parser);

		next = branch->constant_count++;
		branch->constants =
		    realloc(branch->constants,
		            sizeof(expr2_t *) * branch->constant_count);
		branch->constants[next] = aux;

		token = parser_token(parser);
		if (token->type == TOK_COLON) {
			branch->statement = parser2_statement(parser);
			return exp;
		} else if (token->type != TOK_COMMA) {
			parser_error(parser,
			             token,
			             "Expected either COLON or COMMA");
		}
	}
}

expr2_t *
repeat_stmt(parser_t *parser)
{
	expr2_t *aux, *exp = expression_new(EXP_STMT_REPEAT);
	expr_stmt_repeat_t *repeat = &(E_STMT_REPEAT(*exp));
	int next;
	token_t *token;

	parser_token_expect(parser, TOK_REPEAT);
	for (;;) {
		aux = parser2_statement(parser);

		next = repeat->statement_count++;
		repeat->statements =
		    realloc(repeat->statements,
		            sizeof(expr2_t *) * repeat->statement_count);
		repeat->statements[next] = aux;

		token = parser_token(parser);
		if (token->type == TOK_UNTIL) {
			repeat->condition = parser2_expression(parser);
			return exp;
		} else if (token->type != TOK_SEMICOLON) {
			parser_error(parser,
			             token,
			             "Expected  SEMICOLON or UNTIL");
		}
	}
}

int
detect_label(parser_t *parser)
{
	token_t *token;
	token = parser_peek(parser);
	if (token->type == TOK_IDENTIFIER) {
		token = parser_peek_far(parser, 1);
		if (token->type == TOK_COLON) {
			return 1;
		}
	}
	return 0;
}

expr2_t *
label_stmt(parser_t *parser)
{
	expr2_t *stmt = expression_new(EXP_STMT_LABEL);
	E_STMT_LABEL(*stmt).label = parser2_identifier(parser);
	parser_token_expect(parser, TOK_COLON);
	E_STMT_LABEL(*stmt).nested = parser2_statement(parser);
	return stmt;
}

expr2_t *
assignment_stmt(parser_t *parser)
{
	expr2_t *exp = expression_new(EXP_STMT_ASSIGN);
	E_STMT_ASSIGNMENT(*exp).identifier = parser2_variable(parser);
	parser_token_expect(parser, TOK_ASSIGN);
	E_STMT_ASSIGNMENT(*exp).expression = parser2_expression(parser);
	return exp;
}

expr2_t *
function_call_stmt(parser_t *parser)
{
	token_t *token;
	expr2_t *param, *exp = expression_new(EXP_STMT_FUNCTION_CALL);
	expr_stmt_function_call_t *call = &(E_STMT_FUNCTION_CALL(*exp));
	unsigned int next;

	call->identifier = parser2_identifier(parser);
	call->params = NULL;
	call->param_count = 0;

	token = parser_peek(parser);
	if (token->type == TOK_LPAREN) {
		parser_token(parser);
		token = parser_peek(parser);
		if (token->type == TOK_RPAREN) {
			parser_token(parser);
			return exp;
		}

		for (;;) {
			param = parser2_expression(parser);
			next = call->param_count++;
			call->params =
			    realloc(call->params,
			            sizeof(expr2_t *) * call->param_count);
			call->params[next] = param;

			token = parser_peek(parser);
			switch (token->type) {
			case TOK_RPAREN:
				parser_token(parser);
				return exp;
			case TOK_COMMA:
				parser_token(parser);
				break;
			default:
				parser_error(parser,
				             token,
				             "Expected COMMA or RPAREN");
			}
		}
	}

	return exp;
}

expr2_t *
exit_stmt(parser_t *parser)
{
	expr2_t *exp = expression_new(EXP_STMT_EXIT);
	expr_stmt_exit_t *exit = &(E_STMT_EXIT(*exp));
	token_t *token;

	parser_token_expect(parser, TOK_EXIT);
	token = parser_peek(parser);
	if (token->type != TOK_LPAREN) {
		exit->program = 0;
		exit->identifier = NULL;
		return exp;
	}

	parser_token_expect(parser, TOK_LPAREN);
	token = parser_peek(parser);
	if (token->type == TOK_PROGRAM) {
		parser_token_expect(parser, TOK_PROGRAM);
		exit->program = 1;
		exit->identifier = NULL;
	} else {
		exit->program = 0;
		exit->identifier = parser2_identifier(parser);
	}
	return exp;
}
