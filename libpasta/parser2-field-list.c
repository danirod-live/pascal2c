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

#include <assert.h>
#include <parser.h>
#include <stdio.h>
#include <stdlib.h>

expr2_t *
parser2_field_list(parser_t *parser)
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
