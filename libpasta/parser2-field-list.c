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
#include <stdio.h>
#include <stdlib.h>

static expr2_t *parser2_field_list_row(parser_t *parser);
static expr2_t *parser2_field_list_case(parser_t *parser);
static expr2_t *parser2_field_list_case_row(parser_t *parser);

expr2_t *
parser2_field_list(parser_t *parser)
{
	expr2_t *field_list, *case_stmt = NULL;
	expr2_t **rows = NULL;
	int count = 0, next_count;
	expr_field_list_t *flist;
	token_t *token;

	token = parser_peek(parser);
	while (token->type != TOK_CASE) {
		/* TODO: Handle empty line with only a semicolon. */

		/* Read next standard row. */
		next_count = count++;
		rows = realloc(rows, sizeof(expr2_t *) * count);
		rows[next_count] = parser2_field_list_row(parser);

		/* More rows? */
		token = parser_peek(parser);
		if (token->type != TOK_SEMICOLON) {
			goto compose_return;
		}
		parser_token(parser); // consume SEMICOLON

		/* Prepare for next iteration. */
		token = parser_peek(parser);
	}

	/* If I'm here is because there is a CASE. */
	case_stmt = parser2_field_list_case(parser);

compose_return:
	field_list = expression_new(EXP_FIELD_LIST);
	flist = &(E_FIELD_LIST(*field_list));
	flist->rows = rows;
	flist->row_count = count;
	flist->case_stmt = case_stmt;
	return field_list;
}

static expr2_t *
parser2_field_list_case(parser_t *parser)
{
	expr2_t *ident1, *ident2 = NULL;
	token_t *token;
	expr2_t **rows = NULL;
	unsigned int count = 0, next_row;

	expr2_t *fcase;
	expr_field_list_case_t *payload;

	fcase = expression_new(EXP_FIELD_LIST_CASE);
	payload = &(E_FIELD_LIST_CASE(*fcase));

	parser_token_expect(parser, TOK_CASE);
	ident1 = parser2_identifier(parser);
	token = parser_token(parser);
	switch (token->type) {
	case TOK_COLON:
		ident2 = parser2_identifier(parser);
		parser_token_expect(parser, TOK_OF);
		break;
	case TOK_OF:
		// Safe to continue
		break;
	default:
		parser_error(parser, token, "Neither a COLON nor an OF");
	}

	if (ident2 == NULL) {
		payload->type = ident1;
	} else {
		payload->identifier = ident1;
		payload->type = ident2;
	}

	do {
		next_row = count++;
		rows = realloc(rows, sizeof(expr2_t *) * count);
		rows[next_row] = parser2_field_list_case_row(parser);

		// Yes, I check the same in the do-while, but I need to consume
		// the token anyway, just don't touch the original variable.
		token = parser_peek(parser);
		if (token->type == TOK_SEMICOLON) {
			parser_token(parser);
		}
	} while (token->type == TOK_SEMICOLON);

	payload->rows = rows;
	payload->rows_count = count;
	return fcase;
}

static expr2_t *
parser2_field_list_case_row(parser_t *parser)
{
	expr2_t *flist, *row;
	expr2_t **consts = NULL;
	int const_count = 0, next;
	token_t *token;

	do {
		next = const_count++;
		consts = realloc(consts, sizeof(expr2_t *) * const_count);
		consts[next] = parser2_constant(parser);
		token = parser_token(parser);
	} while (token->type == TOK_COMMA);

	if (token->type != TOK_COLON) {
		parser_error(parser, token, "Expecting colon or comma");
	}

	parser_token_expect(parser, TOK_LPAREN);
	flist = parser2_field_list(parser);
	parser_token_expect(parser, TOK_RPAREN);

	row = expression_new(EXP_FIELD_LIST_CASE_ROW);
	E_FIELD_LIST_CASE_ROW(*row).constants = consts;
	E_FIELD_LIST_CASE_ROW(*row).constant_count = const_count;
	E_FIELD_LIST_CASE_ROW(*row).field_list = flist;
	return row;
}

static expr2_t *
parser2_field_list_row(parser_t *parser)
{
	expr2_t **idents = NULL;
	expr2_t *it, *flist;
	expr_field_list_row_t *payload;
	token_t *tok;
	int count = 0, next_count;

	for (;;) {
		// Parse the next identifier.
		it = parser2_identifier(parser);
		next_count = count++;
		idents = realloc(idents, count * sizeof(expr2_t *));
		idents[next_count] = it;

		// Find the token that comes next to end the loop.
		// If it is a comma, none of the cases will match,
		// therefore, it will run another iteration. Break
		// in case of COLON, because it means that every
		// identifier has been parsed. If it is neither a
		// COLON nor a COMMA, clearly it is an error.
		tok = parser_token(parser);
		if (tok->type == TOK_COLON) {
			// Stop the loop
			break;
		} else if (tok->type != TOK_COMMA) {
			// Does not return
			parser_error(parser, tok, "Expected COLON or COMMA");
		}
	}

	flist = expression_new(EXP_FIELD_LIST_ROW);
	payload = (expr_field_list_row_t *) &flist->payload.field_list_row;
	payload->identifiers = idents;
	payload->identifier_count = count;
	payload->data_type = parser2_type(parser);
	return flist;
}
