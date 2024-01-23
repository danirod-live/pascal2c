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

/*
 * Expression nodes for unsigned number and unsigned integer.
 *
 * They are here since they look similar. The only difference is that
 * number accepts any kind of scientific number while integer only
 * accepts strings made of digits.
 *
 * Note that the parsing of numbers is already done at scanner level.
 * Therefore, these functions only have to wrap the tokens into a
 * proper expression node.
 */

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
