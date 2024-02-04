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

/*
 * Expression nodes for simple types.
 *
 * These are types that we actually implemented in the scanner, but since
 * they need to interact with other expressions, the functions defined
 * here wrap them into expressions.
 *
 * - parser_identifier, to wrap a TOK_IDENTIFIER.
 * - parser_unsigned_number for unsigned numbers, which are unsigned
 *   scientific numbers like 4, 4e11, 4.22e11, 4e-11, 4.22e-11, 4.22e+11
 *   or 4.22e-11.
 * - parser_unsigned_integer for numbers that only accepts a sequence
 *   of digits, but no commas nor scientific notation.
 */

expr_t *
parser_identifier(parser_t *parser)
{
	return new_literal(parser_token_expect(parser, TOK_IDENTIFIER));
}

expr_t *
parser_unsigned_number(parser_t *parser)
{
	return new_literal(parser_token_expect(parser, TOK_DIGIT));
}

expr_t *
parser_unsigned_integer(parser_t *parser)
{
	token_t *token;
	char *value;

	token = parser_token_expect(parser, TOK_DIGIT);
	if (!token->meta)
		parser_error(parser, token, "TOK_DIGIT has no meta value");
	value = token->meta;
	while (*value) {
		if (*value < '0' || *value > '9')
			parser_error(parser, token, "Expected an integer");
		value++;
	}

	return new_literal(token);
}
