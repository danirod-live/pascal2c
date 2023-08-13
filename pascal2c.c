#include "circbuf.h"
#include "scanner.h"
#include "token.h"

int
main()
{
	scanner_t *scanner;
	tokentype_t tok;

	if ((scanner = scanner_init(stdin)) == 0) {
		puts("error: scanner_init");
		return 1;
	}

	while ((tok = scanner_next(scanner)) != TOK_EOF) {
		puts(tokentype_string(tok));
	}

	scanner_free(scanner);
	return 0;
}
