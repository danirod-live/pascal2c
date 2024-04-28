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
parser2_unsigned_constant(parser_t *parser)
{
	expr2_t *exp, *inner;
	token_t *token = parser_peek(parser);
	switch (token->type) {
	case TOK_STRING:
		inner = expression_new(EXP_STRING);
		E_STRING(*inner).token = token;
		break;
	case TOK_NIL:
		inner = expression_new(EXP_NIL);
		break;
	case TOK_DIGIT:
		inner = parser2_unsigned_number(parser);
		break;
	case TOK_IDENTIFIER:
		inner = parser2_identifier(parser);
		break;
	default:
		parser_error(parser, token, "Token is of invalid type");
	}

	exp = expression_new(EXP_UNSIGNED_CONSTANT);
	E_UNSIGNED_CONSTANT(*exp).inner = inner;
	return exp;
}
