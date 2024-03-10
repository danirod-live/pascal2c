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
#include "scanner.h"
#include "token.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct scanner {
	char *buffer;
	unsigned int len;
	unsigned int pos;
	unsigned int line, col;
};

/** returns the next character in the scanner without consuming it. */
static int
scanner_peek(scanner_t *scanner)
{
	if (scanner->pos >= scanner->len) {
		return EOF;
	}
	return scanner->buffer[scanner->pos];
}

static int
scanner_peekfar(scanner_t *scanner, int offt)
{
	size_t ea = scanner->pos + offt;
	if (ea >= scanner->len) {
		return EOF;
	}
	return scanner->buffer[ea];
}

static void
scanner_advance(scanner_t *scanner)
{
	if (scanner->buffer[scanner->pos] == '\n') {
		scanner->line++;
		scanner->col = 0;
	}
	scanner->pos++;
	scanner->col++;
}

static void
scanner_consume_until_char(scanner_t *scanner, char ch)
{
	while (scanner->buffer[scanner->pos] != ch)
		scanner_advance(scanner);
}

static void
consume_until_closing_bracket(scanner_t *scanner)
{
	// read until we find a closing bracket
	while (scanner->buffer[scanner->pos] != '}')
		scanner_advance(scanner);
	scanner_advance(scanner); // skip the bracket itself
}

static void
consume_until_closing_trigraph(scanner_t *scanner)
{
	// read until we find a *)
	for (;;) {
		while (scanner->buffer[scanner->pos] != '*')
			scanner_advance(scanner);
		if (scanner->buffer[scanner->pos + 1] == ')') {
			// a real trigraph ending.
			scanner->pos += 2;
			scanner->col += 2;
			return;
		} else {
			// not a real trigraph ending, just an alone asterisk
			// skip it and continue reading characters
			scanner_advance(scanner);
		}
	}
}

static void
consume_slash_comment(scanner_t *scanner)
{
	// read until the end of the line
	while (scanner->buffer[scanner->pos] != '\n')
		scanner_advance(scanner);
	while (scanner->buffer[scanner->pos] == '\n')
		scanner_advance(scanner);
}

// check that the scanner points at a valid character, and moves the
// offset if it doesn't. however, this function will not touch the
// offset if it already points at a valid character
static void
scanner_clean(scanner_t *scanner)
{
	char ch;
	int valid;

	do {
		valid = 1;

		ch = scanner->buffer[scanner->pos];
		switch (ch) {
		case '\n':
		case '\r':
		case '\t':
		case ' ':
			scanner_advance(scanner);
			valid = 0;
			break;
		case '{':
			consume_until_closing_bracket(scanner);
			valid = 0;
			break;
		case '/':
			if (scanner->buffer[scanner->pos + 1] == '/') {
				consume_slash_comment(scanner);
				valid = 0;
			}
			break;
		case '(':
			if (scanner->buffer[scanner->pos + 1] == '*') {
				consume_until_closing_trigraph(scanner);
				valid = 0;
			}
			break;
		}
	} while (!valid);
}

static void
scanner_discard(scanner_t *scanner)
{
	scanner_advance(scanner);
	scanner_clean(scanner);
}

static token_t *
alloc_token_with_meta(scanner_t *scanner, tokentype_t type, char *value)
{
	token_t *tok;
	if ((tok = malloc(sizeof(token_t))) != NULL) {
		tok->type = type;
		tok->meta = value;
		tok->line = scanner->line;
		tok->col = scanner->col;
	}
	return tok;
}

static token_t *
alloc_token(scanner_t *scanner, tokentype_t type)
{
	return alloc_token_with_meta(scanner, type, 0);
}

static token_t *
scanner_read_as_number(scanner_t *scanner)
{
	int len = 0, flag = 0;
	char chr;
	char *value;
	token_t *token;

	for (;;) {
		chr = scanner_peekfar(scanner, len);

		// Common case: read digits.
		if (chr >= '0' && chr <= '9') {
			len++;
			continue;
		}

		// Switch from integer to decimal part.
		if (chr == '.' && flag < 1) {
			flag = 1;

			// Check that there is a number after the dot
			chr = scanner_peekfar(scanner, len + 1);
			if (chr >= '0' && chr <= '9') {
				len++;
			} else {
				// Avoid incrementing len when invalid to
				// prevent the dot being part of the number.
				// Let's let the dot as a separate token, and
				// someone else will figure out.
				flag = 3;
			}

			continue;
		}

		// Switch from integer or decimal to exponential.
		if (((chr == 'e') || (chr == 'E')) && flag < 2) {
			flag = 2;

			// Check what is after the E.
			chr = scanner_peekfar(scanner, len + 1);
			if (chr == '+' || chr == '-') {
				// Check the character after the plus and the
				// minus.
				chr = scanner_peekfar(scanner, len + 2);
				if (chr >= '0' && chr <= '9') {
					len += 2;
				} else {
					// Not a number, stop.
					flag = 3;
				}
			} else if (chr >= '0' && chr <= '9') {
				len++;
			} else {
				// Not a number, stop.
				flag = 3;
			}
			continue;
		}

		// If we reach here, then we found a symbol that does not belong
		// to this
		break;
	}

	/* extract the string from the buffer */
	value = malloc(sizeof(char) * len + 1);
	memcpy(value, scanner->buffer + scanner->pos, len);
	value[len] = 0;

	/* consume the characters once read */
	token = alloc_token_with_meta(scanner, TOK_DIGIT, value);
	scanner->pos += len;
	scanner->col += len;
	return token;
}

static token_t *
scanner_read_as_identifier(scanner_t *scanner)
{
	int len = 0;
	char chr;
	char *value;
	tokentype_t type;
	token_t *token;

	for (;;) {
		chr = scanner_peekfar(scanner, len);
		if (!isalpha(chr) && !isdigit(chr) && chr != '_')
			break;
		len++;
	}

	/* extract the string from the buffer */
	value = malloc(sizeof(char) * len + 1);
	memcpy(value, scanner->buffer + scanner->pos, len);
	value[len] = 0;

	/* check out the lookup table in case it is a keyword. */
	type = match_identifier(value);
	if (type == TOK_IDENTIFIER) {
		token = alloc_token_with_meta(scanner, type, value);
	} else {
		token = alloc_token(scanner, type);
	}

	/* consume the characters */
	scanner->pos += len;
	scanner->col += len;

	return token;
}

static token_t *
scanner_read_as_string(scanner_t *scanner)
{
	char *meta;
	char chr;
	int len = 0;
	token_t *tok;

	for (;;) {
		chr = scanner_peekfar(scanner, len);
		switch (chr) {
		case '\'':
			len++;
			while (scanner_peekfar(scanner, len) != '\'') {
				// continue reading until the end of the
				// string
				len++;
			}
			len++; // skip the closing quote or this will be
			       // an infinite loop
			break;
		case '#':
			len++;
			while (isdigit(scanner_peekfar(scanner, len)))
				len++;
			break;
		default:
			// the string is over
			meta = malloc(sizeof(char) * len + 1);
			memcpy(meta, scanner->buffer + scanner->pos, len);
			meta[len] = 0;
			tok = alloc_token_with_meta(scanner, TOK_STRING, meta);
			scanner->pos += len;
			scanner->col += len;
			return tok;
		}
	}
}

scanner_t *
scanner_init(char *buffer, size_t len)
{
	scanner_t *scanner = malloc(sizeof(scanner_t));

	if (scanner) {
		scanner->buffer = buffer;
		scanner->len = len;
		scanner->pos = 0;
		scanner->line = 1;
		scanner->col = 1;

		// skip the BOM mark
		if (scanner->buffer[0] == (char) 0xEF
		    && scanner->buffer[1] == (char) 0xBB
		    && scanner->buffer[2] == (char) 0xBF) {
			scanner->pos = 4;
		}
	}

	return scanner;
}

void
scanner_free(scanner_t *scanner)
{
	free(scanner);
}

token_t *
scanner_next(scanner_t *scanner)
{
	scanner_clean(scanner);
	int next = scanner_peek(scanner);
	token_t *token;

	if (next == EOF) {
		return alloc_token(scanner, TOK_EOF);
	}

	switch (next) {
	case '*':
		token = alloc_token(scanner, TOK_ASTERISK);
		scanner_discard(scanner);
		return token;
	case '@':
		token = alloc_token(scanner, TOK_AT);
		scanner_discard(scanner);
		return token;
	case '^':
		token = alloc_token(scanner, TOK_CARET);
		scanner_discard(scanner);
		return token;
	case ':':
		if (scanner_peekfar(scanner, 1) == '=') {
			token = alloc_token(scanner, TOK_ASSIGN);
			scanner_discard(scanner);
			scanner_discard(scanner);
			return token;
		}
		token = alloc_token(scanner, TOK_COLON);
		scanner_discard(scanner);
		return token;
	case ',':
		token = alloc_token(scanner, TOK_COMMA);
		scanner_discard(scanner);
		return token;
	case '$':
		token = alloc_token(scanner, TOK_DOLLAR);
		scanner_discard(scanner);
		return token;
	case '.':
		if (scanner_peekfar(scanner, 1) == '.') {
			token = alloc_token(scanner, TOK_DOTDOT);
			scanner_discard(scanner);
			scanner_discard(scanner);
			return token;
		} else {
			token = alloc_token(scanner, TOK_DOT);
			scanner_discard(scanner);
			return token;
		}
	case '=':
		token = alloc_token(scanner, TOK_EQUAL);
		scanner_discard(scanner);
		return token;
	case '>':
		if (scanner_peekfar(scanner, 1) == '=') {
			token = alloc_token(scanner, TOK_GREATEQL);
			scanner_discard(scanner);
			scanner_discard(scanner);
			return token;
		} else {
			token = alloc_token(scanner, TOK_GREATER);
			scanner_discard(scanner);
			return token;
		}
	case '[':
		token = alloc_token(scanner, TOK_LBRACKET);
		scanner_discard(scanner);
		return token;
	case '<':
		if (scanner_peekfar(scanner, 1) == '=') {
			token = alloc_token(scanner, TOK_LESSEQL);
			scanner_discard(scanner);
			scanner_discard(scanner);
			return token;
		} else if (scanner_peekfar(scanner, 1) == '>') {
			token = alloc_token(scanner, TOK_NEQUAL);
			scanner_discard(scanner);
			scanner_discard(scanner);
			return token;
		} else {
			token = alloc_token(scanner, TOK_LESSER);
			scanner_discard(scanner);
			return token;
		}
	case '(':
		token = alloc_token(scanner, TOK_LPAREN);
		scanner_discard(scanner);
		return token;
	case '-':
		token = alloc_token(scanner, TOK_MINUS);
		scanner_discard(scanner);
		return token;
	case '+':
		token = alloc_token(scanner, TOK_PLUS);
		scanner_discard(scanner);
		return token;
	case ']':
		token = alloc_token(scanner, TOK_RBRACKET);
		scanner_discard(scanner);
		return token;
	case ')':
		token = alloc_token(scanner, TOK_RPAREN);
		scanner_discard(scanner);
		return token;
	case ';':
		token = alloc_token(scanner, TOK_SEMICOLON);
		scanner_discard(scanner);
		return token;
	case '/':
		token = alloc_token(scanner, TOK_SLASH);
		scanner_discard(scanner);
		return token;
	}

	if (next >= '0' && next <= '9') {
		return scanner_read_as_number(scanner);
	}
	if (isupper(next) || islower(next) || next == '_') {
		return scanner_read_as_identifier(scanner);
	}
	if (next == '\'' || next == '#') {
		return scanner_read_as_string(scanner);
	}

	return alloc_token(scanner, TOK_EOF);
}
