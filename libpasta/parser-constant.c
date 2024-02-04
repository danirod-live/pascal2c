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
 * Expression node for an unsigned constant.
 *
 * Since we implement digits and identifiers as tokens, the scanner should
 * already provide tokens of the correct type. All that remains, is to test
 * whether these tokens are of a valid type or not.
 *
 * This function always returns literal expressions.
 */
expr_t *
parser_unsigned_constant(parser_t *parser)
{
	token_t *token = parser_token(parser);
	switch (token->type) {
	case TOK_STRING:
	case TOK_NIL:
	case TOK_DIGIT:
	case TOK_IDENTIFIER:
		return new_literal(token);
	default:
		parser_error(parser, token, "Token is of invalid type");
	}
}

/*
 * Expression node for a constant.
 *
 * As the name implies, the difference between a constant and an unsigned
 * constant is that a constant may have sign. When a constant has no sign, it
 * behaves like an unsigned constant, so the literal is returned. When a
 * constant has sign, an unary with the preceding TOK_PLUS or TOK_MINUS is added
 * to the identifier or unsigned number.
 */
expr_t *
parser_constant(parser_t *parser)
{
	token_t *token, *sign;

	token = parser_peek(parser);
	switch (token->type) {
	case TOK_STRING:
	case TOK_NIL:
	case TOK_IDENTIFIER:
	case TOK_DIGIT:
		/* Early return for unsigned constants. */
		return parser_unsigned_constant(parser);
	case TOK_PLUS:
	case TOK_MINUS:
		/* Will treat this below. */
		break;
	default:
		parser_error(parser,
		             token,
		             "Unexpected token type for constant");
	}

	/* If we reach here, is because token is PLUS or MINUS. */
	sign = parser_token(parser);
	token = parser_token(parser);
	switch (token->type) {
	case TOK_IDENTIFIER:
	case TOK_DIGIT:
		return new_unary(sign, new_literal(token));
	default:
		parser_error(parser,
		             token,
		             "Unexpected token after constant sign");
	}
}
