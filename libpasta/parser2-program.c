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

#include <parser.h>
#include <stdlib.h>

static void subidents(parser_t *parser, expr_program_t *program);

expr2_t *
parser2_program(parser_t *parser)
{
	expr2_t *exp = expression_new(EXP_PROGRAM);
	expr_program_t *program = &(E_PROGRAM(*exp));
	token_t *token;

	parser_token_expect(parser, TOK_PROGRAM);
	program->identifier = parser2_identifier(parser);

	token = parser_peek(parser);
	if (token->type == TOK_LPAREN) {
		subidents(parser, program);
	}

	parser_token_expect(parser, TOK_SEMICOLON);
	program->block = parser2_block(parser);
	return exp;
}

static void
subidents(parser_t *parser, expr_program_t *program)
{
	int next;
	expr2_t *aux;
	token_t *token;

	parser_token_expect(parser, TOK_LPAREN);

	for (;;) {
		token = parser_peek(parser);
		if (token->type == TOK_RPAREN) {
			parser_token_expect(parser, TOK_RPAREN);
			return;
		}

		if (program->subident_count > 0) {
			parser_token_expect(parser, TOK_COMMA);
		}

		aux = parser2_identifier(parser);
		next = program->subident_count++;
		program->subidents =
		    realloc(program->subidents,
		            sizeof(expr2_t *) * program->subident_count);
		program->subidents[next] = aux;
	}
}
