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

expr2_t *
parser2_identifier(parser_t *parser)
{
	expr2_t *exp = expression_new(EXP_IDENTIFIER);
	E_IDENTIFIER(*exp).token = parser_token_expect(parser, TOK_IDENTIFIER);
	return exp;
}

expr2_t *
parser2_unsigned_number(parser_t *parser)
{
	expr2_t *exp = expression_new(EXP_UNSIGNED_NUMBER);
	E_UNSIGNED_NUMBER(*exp).token = parser_token_expect(parser, TOK_DIGIT);
	return exp;
}

expr2_t *
parser2_unsigned_integer(parser_t *parser)
{
	char *value;
	expr2_t *exp;
	token_t *token;

	// Read the token and expect that there is a number inside.
	token = parser_token_expect(parser, TOK_DIGIT);
	if (!token->meta)
		parser_error(parser, token, "TOK_DIGIT has no meta value");
	value = token->meta;

	// Check that the number is actually an unsigned integer.
	while (*value) {
		if (*value < '0' || *value > '9')
			parser_error(parser, token, "Expected an integer");
		value++;
	}

	// If we reach here, is valid.
	exp = expression_new(EXP_UNSIGNED_INTEGER);
	E_UNSIGNED_INTEGER(*exp).token = token;
	return exp;
}
