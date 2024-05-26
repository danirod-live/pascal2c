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

static expr2_t *parse_chunk(parser_t *parser);
static void parse_parameters(parser_t *parser, expr2_t **plist);

expr2_t *
parser2_parameter_list(parser_t *parser)
{
	token_t *next;
	expr2_t *plist;

	// Initialize the expression node.
	plist = expression_new(EXP_PLIST);
	E_PLIST(*plist).chunks = NULL;
	E_PLIST(*plist).chunks_count = 0;

	// Check if there are things to process.
	next = parser_peek(parser);
	if (next->type == TOK_LPAREN) {
		parser_token_expect(parser, TOK_LPAREN);

		// Check if this is an empty parameter list.
		// NOTE: This behaviour should be disabled in strict mode.
		next = parser_peek(parser);
		if (next->type != TOK_RPAREN) {
			parse_parameters(parser, &plist);
			// parse_parameters should have left the RPAREN.
		}

		parser_token_expect(parser, TOK_RPAREN);
	}

	return plist;
}

static void
parse_parameters(parser_t *parser, expr2_t **plist)
{
	expr2_t *chunk;
	expr_plist_t *payload = &(E_PLIST(**plist));
	int new_size, next_index;
	token_t *next;

	for (;;) {
		chunk = parse_chunk(parser);
		assert(chunk != NULL);

		next_index = payload->chunks_count;
		payload->chunks_count++;
		new_size = sizeof(expr2_t *) * payload->chunks_count;
		payload->chunks = realloc(payload->chunks, new_size);
		payload->chunks[next_index] = chunk;

		next = parser_peek(parser);
		switch (next->type) {
		case TOK_SEMICOLON:
			parser_token(parser);
			break;
		case TOK_RPAREN:
			// Don't eat the RPAREN -> parser2_parameter_list
			return;
		default:
			parser_error(parser,
			             next,
			             "Expected SEMICOLON or RPAREN");
		}
	}
}

static expr2_t *
parse_chunk(parser_t *parser)
{
	token_t *next;
	expr2_t *ident, *chunk = expression_new(EXP_PLIST_CHUNK);
	expr_plist_chunk_t *payload;
	int next_ident, id_size;

	if (chunk == NULL)
		return chunk;
	payload = &(E_PLIST_CHUNK(*chunk));
	payload->identifiers = NULL;
	payload->identifier_count = 0;
	payload->data_type = NULL;
	payload->is_reference = 0;

	// Check for VAR presence.
	next = parser_peek(parser);
	if (next->type == TOK_VAR) {
		payload->is_reference = 1;
		parser_token(parser);
	}

	// Read the identifier list.
	for (;;) {
		ident = parser2_identifier(parser);
		next_ident = payload->identifier_count;
		payload->identifier_count++;
		id_size = sizeof(expr2_t *) * payload->identifier_count;
		payload->identifiers = realloc(payload->identifiers, id_size);
		payload->identifiers[next_ident] = ident;

		next = parser_token(parser);
		switch (next->type) {
		case TOK_COMMA:
			break;
		case TOK_COLON:
			// Done with the id list, parse data type
			payload->data_type = parser2_identifier(parser);
			return chunk;
		default:
			parser_error(parser, next, "Expected COMMA or DOT");
		}
	}
}
