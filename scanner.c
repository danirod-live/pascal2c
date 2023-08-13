#include "scanner.h"
#include "circbuf.h"
#include "token.h"

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

tokentype_t
scanner_next(scanner_t *scanner)
{
	int next = scanner_peek_clean(scanner);

	if (next == EOF) {
		return TOK_EOF;
	}

	switch (next) {
	case '*':
		scanner_discard(scanner);
		return TOK_ASTERISK;
	case ',':
		scanner_discard(scanner);
		return TOK_COMMA;
	case '.':
		scanner_discard(scanner);
		return TOK_DOT;
	case '=':
		scanner_discard(scanner);
		return TOK_EQUAL;
	case '(':
		scanner_discard(scanner);
		return TOK_LPAREN;
	case '-':
		scanner_discard(scanner);
		return TOK_MINUS;
	case '+':
		scanner_discard(scanner);
		return TOK_PLUS;
	case ')':
		scanner_discard(scanner);
		return TOK_RPAREN;
	case ';':
		scanner_discard(scanner);
		return TOK_SEMICOLON;
	case '/':
		scanner_discard(scanner);
		return TOK_SLASH;
	}
	return TOK_EOF;
}
