#include "scanner.h"
#include "circbuf.h"
#include "token.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

struct scanner {
	FILE *fp;
	circbuf_t *buf;
};

scanner_t *
scanner_init(FILE *fp)
{
	scanner_t *scanner = malloc(sizeof(scanner_t));

	if (scanner) {
		scanner->fp = fp;
		scanner->buf = circbuf_init();
	}

	// circbuf could not be initialised
	if (!scanner->buf) {
		free(scanner);
		scanner = 0;
	}

	return scanner;
}

void
scanner_free(scanner_t *scanner)
{
	circbuf_free(scanner->buf);
	free(scanner);
}

/** returns the next character in the scanner without consuming it. */
static int
scanner_peek(scanner_t *scanner)
{
	int ch;
	if (circbuf_empty(scanner->buf)) {
		ch = fgetc(scanner->fp);
		circbuf_write(scanner->buf, ch);
	}
	return circbuf_peek(scanner->buf);
}

static int
scanner_peekfar(scanner_t *scanner, int offt)
{
	int nch;

	while (circbuf_bufsiz(scanner->buf) <= offt) {
		nch = fgetc(scanner->fp);
		circbuf_write(scanner->buf, nch);
	}
	return circbuf_peekfar(scanner->buf, offt);
}

#define scanner_discard(scanner) circbuf_read(scanner->buf)

static void
scanner_consume_until_char(scanner_t *scanner, char ch)
{
	while (scanner_peek(scanner) != ch) {
		scanner_discard(scanner);
	}
}

extern void circbuf_debug(circbuf_t *buf);

static int
scanner_peek_clean(scanner_t *scanner)
{
	int ch;
	int valid;

	do {
		ch = scanner_peek(scanner);
		valid = 1;

		// skip invalid characters
		switch (ch) {
		case '\n':
		case '\r':
		case '\t':
		case ' ':
			scanner_discard(scanner);
			valid = 0;
			break;
		case '{':
			scanner_consume_until_char(scanner, '}');
			scanner_discard(scanner);
			valid = 0;
			break;
		case '/':
			if (scanner_peekfar(scanner, 1) == '/') {
				scanner_consume_until_char(scanner, '\n');
				scanner_discard(scanner);
				valid = 0;
			}
			break;
		case '(':
			if (scanner_peekfar(scanner, 1) == '*') {
				for (;;) {
					scanner_consume_until_char(scanner,
					                           '*');
					scanner_discard(scanner);
					if (scanner_peek(scanner) == ')') {
						scanner_discard(scanner);
						break;
					}
				}
				valid = 0;
				break;
			}
		}
	} while (!valid);

	return ch;
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
scanner_read_as_digit(scanner_t *scanner)
{
	int i, len = 0;
	char chr;
	char *value;

	/* find the first len where the character is not a digit */
	/* TODO: what about the real numbers? */
	for (;;) {
		chr = scanner_peekfar(scanner, len);
		if (chr < '0' || chr > '9')
			break;
		len++;
	}

	/* extract the string from the buffer */
	value = malloc(sizeof(char) * len + 1);
	for (i = 0; i < len; i++)
		value[i] = circbuf_read(scanner->buf);
	value[len] = 0;

	return alloc_token_with_meta(TOK_DIGIT, value);
}

token_t *
scanner_read_as_identifier(scanner_t *scanner)
{
	int i, len = 0;
	char chr;
	char *value;
	tokentype_t type;

	for (;;) {
		chr = scanner_peekfar(scanner, len);
		if (!isalpha(chr) && !isdigit(chr))
			break;
		len++;
	}

	/* extract the string from the buffer */
	value = malloc(sizeof(char) * len + 1);
	for (i = 0; i < len; i++)
		value[i] = circbuf_read(scanner->buf);
	value[len] = 0;

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
	int i, len = 0;

	for (;;) {
		chr = scanner_peekfar(scanner, len);
		switch (chr) {
		case '\'':
			len++;
			while (scanner_peekfar(scanner, len) != '\'') {
				// continue reading until the end of the string
				len++;
			}
			len++; // skip the closing quote or this will be an
			       // infinite loop
			break;
		case '#':
			len++;
			while (isdigit(scanner_peekfar(scanner, len)))
				len++;
			break;
		default:
			// the string is over
			meta = malloc(sizeof(char) * (len + 1));
			for (i = 0; i < len; i++)
				meta[i] = circbuf_read(scanner->buf);
			meta[len] = 0;
			return alloc_token_with_meta(TOK_STRING, meta);
		}
	}
}

token_t *
scanner_next(scanner_t *scanner)
{
	int next = scanner_peek_clean(scanner);

	if (next == EOF) {
		return alloc_token(TOK_EOF);
	}

	switch (next) {
	case '*':
		scanner_discard(scanner);
		return alloc_token(TOK_ASTERISK);
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
	case '.':
		scanner_discard(scanner);
		return alloc_token(TOK_DOT);
	case '=':
		scanner_discard(scanner);
		return alloc_token(TOK_EQUAL);
	case '(':
		scanner_discard(scanner);
		return alloc_token(TOK_LPAREN);
	case '-':
		scanner_discard(scanner);
		return alloc_token(TOK_MINUS);
	case '+':
		scanner_discard(scanner);
		return alloc_token(TOK_PLUS);
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
		return scanner_read_as_digit(scanner);
	}
	if ((next >= 'a' && next <= 'z') || (next >= 'A' && next <= 'Z')) {
		return scanner_read_as_identifier(scanner);
	}
	if (next == '\'' || next == '#') {
		return scanner_read_as_string(scanner);
	}

	return alloc_token(TOK_EOF);
}
