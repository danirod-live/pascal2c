#include "circbuf.h"
#include "scanner.h"
#include "token.h"
#include <stdlib.h>

static void
print_token(token_t *tok)
{
	if (tok->meta != 0) {
		printf("%s(%s)\n", tokentype_string(tok->type), tok->meta);
	} else {
		puts(tokentype_string(tok->type));
	}
}

int
main()
{
	scanner_t *scanner;
	token_t *tok;
	int eof = 0;

	if ((scanner = scanner_init(stdin)) == 0) {
		puts("error: scanner_init");
		return 1;
	}

	do {
		tok = scanner_next(scanner);
		if (tok && tok->type != TOK_EOF) {
			print_token(tok);
		}
		if (tok->type == TOK_EOF) {
			eof = 1;
		}
		token_free(tok);
		free(tok);
	} while (!eof);

	scanner_free(scanner);
	return 0;
}
