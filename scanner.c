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
scanner_consume_until_char(scanner_t *scanner, char ch)
{
	while (scanner->buffer[scanner->pos] != ch)
		scanner->pos++;
}

static void
consume_until_closing_bracket(scanner_t *scanner)
{
	// read until we find a closing bracket
	while (scanner->buffer[scanner->pos] != '}')
		scanner->pos++;
	scanner->pos++; // skip the bracket itself
}

static void
consume_until_closing_trigraph(scanner_t *scanner)
{
	// read until we find a *)
	for (;;) {
		while (scanner->buffer[scanner->pos] != '*')
			scanner->pos++;
		if (scanner->buffer[scanner->pos + 1] == ')') {
			// a real trigraph ending.
			scanner->pos += 2;
			return;
		} else {
			// not a real trigraph ending, just an alone asterisk
			// skip it and continue reading characters
			scanner->pos++;
		}
	}
}

static void
consume_slash_comment(scanner_t *scanner)
{
	// read until the end of the line
	while (scanner->buffer[scanner->pos] != '\n')
		scanner->pos++;
	while (scanner->buffer[scanner->pos] == '\n')
		scanner->pos++;
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
			scanner->pos++;
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
	scanner->pos++;
	scanner_clean(scanner);
}

static token_t *
alloc_token_with_meta(tokentype_t type, char *value)
{
	token_t *tok;
	if ((tok = malloc(sizeof(token_t))) != NULL) {
		tok->type = type;
		tok->meta = value;
	}
	return tok;
}

static token_t *
alloc_token(tokentype_t type)
{
	return alloc_token_with_meta(type, 0);
}

static token_t *
scanner_read_as_number(scanner_t *scanner)
{
	int len = 0, flag = 0;
	char chr;
	char *value;

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
	scanner->pos += len;

	return alloc_token_with_meta(TOK_DIGIT, value);
}

static token_t *
scanner_read_as_identifier(scanner_t *scanner)
{
	int len = 0;
	char chr;
	char *value;
	tokentype_t type;

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

	/* consume the characters */
	scanner->pos += len;

	/* check out the lookup table in case it is a keyword. */
	type = match_identifier(value);
	if (type == TOK_IDENTIFIER) {
		return alloc_token_with_meta(type, value);
	}
	return alloc_token(type);
}

static token_t *
scanner_read_as_string(scanner_t *scanner)
{
	char *meta;
	char chr;
	int len = 0;

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
			scanner->pos += len;
			return alloc_token_with_meta(TOK_STRING, meta);
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

	if (next == EOF) {
		return alloc_token(TOK_EOF);
	}

	switch (next) {
	case '*':
		scanner_discard(scanner);
		return alloc_token(TOK_ASTERISK);
	case '@':
		scanner_discard(scanner);
		return alloc_token(TOK_AT);
	case '^':
		scanner_discard(scanner);
		return alloc_token(TOK_CARET);
	case ':':
		if (scanner_peekfar(scanner, 1) == '=') {
			scanner_discard(scanner);
			scanner_discard(scanner);
			return alloc_token(TOK_ASSIGN);
		}
		scanner_discard(scanner);
		return alloc_token(TOK_COLON);
	case ',':
		scanner_discard(scanner);
		return alloc_token(TOK_COMMA);
	case '$':
		scanner_discard(scanner);
		return alloc_token(TOK_DOLLAR);
	case '.':
		if (scanner_peekfar(scanner, 1) == '.') {
			scanner_discard(scanner);
			scanner_discard(scanner);
			return alloc_token(TOK_DOTDOT);
		} else {
			scanner_discard(scanner);
			return alloc_token(TOK_DOT);
		}
	case '=':
		scanner_discard(scanner);
		return alloc_token(TOK_EQUAL);
	case '>':
		scanner_discard(scanner);
		return alloc_token(TOK_GREATER);
	case '[':
		scanner_discard(scanner);
		return alloc_token(TOK_LBRACKET);
	case '<':
		scanner_discard(scanner);
		return alloc_token(TOK_LESSER);
	case '(':
		scanner_discard(scanner);
		return alloc_token(TOK_LPAREN);
	case '-':
		scanner_discard(scanner);
		return alloc_token(TOK_MINUS);
	case '+':
		scanner_discard(scanner);
		return alloc_token(TOK_PLUS);
	case ']':
		scanner_discard(scanner);
		return alloc_token(TOK_RBRACKET);
	case ')':
		scanner_discard(scanner);
		return alloc_token(TOK_RPAREN);
	case ';':
		scanner_discard(scanner);
		return alloc_token(TOK_SEMICOLON);
	case '/':
		scanner_discard(scanner);
		return alloc_token(TOK_SLASH);
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

	return alloc_token(TOK_EOF);
}
