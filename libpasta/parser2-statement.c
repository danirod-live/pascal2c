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
expr2_t *function_call_stmt(parser_t *parser);

expr2_t *
parser2_statement(parser_t *parser)
{
	if (detect_label(parser)) {
		return label_stmt(parser);
	} else {
		return function_call_stmt(parser);
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
