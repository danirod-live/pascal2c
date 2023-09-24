#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "scanner.h"
#include "token.h"

expr_t *
new_unary(token_t *t, expr_t *expr)
{
	expr_t *exp = (expr_t *) calloc(sizeof(expr_t), 1);
	exp->type = UNARY;
	exp->exp_left = expr;
	exp->token = t;
	return exp;
}

expr_t *
new_binary(token_t *t, expr_t *left, expr_t *right)
{
	expr_t *exp = (expr_t *) calloc(sizeof(expr_t), 1);
	exp->type = BINARY;
	exp->exp_left = left;
	exp->exp_right = right;
	exp->token = t;
	return exp;
}

expr_t *
new_grouping(expr_t *wrap)
{
	expr_t *exp = (expr_t *) calloc(sizeof(expr_t), 1);
	exp->type = GROUPING;
	exp->exp_left = wrap;
	return exp;
}

expr_t *
new_literal(token_t *tok)
{
	expr_t *exp = (expr_t *) calloc(sizeof(expr_t), 1);
	exp->type = LITERAL;
	exp->token = tok;
	return exp;
}

////

parser_t *
parser_new()
{
	parser_t *par = (parser_t *) malloc(sizeof(parser_t));
	par->tokens = NULL;
	par->len = 0;
	par->pos = 0;
	return par;
}

#define TOKEN_LOAD_BUFSIZ 64

static int
parser_append(parser_t *parser, token_t **tlist, int len)
{
	unsigned i;
	token_t **next;

	next = realloc(parser->tokens, sizeof(token_t *) * (parser->len + len));
	if (next == NULL) {
		return 0;
	}

	for (i = 0; i < len; i++) {
		next[i + parser->len] = tlist[i];
	}
	parser->tokens = next;
	parser->len += len;
	return 1;
}

void
parser_load_tokens(parser_t *parser, scanner_t *scanner)
{
	int bufsiz = 0;
	token_t *last_token = NULL;
	token_t *tokens[TOKEN_LOAD_BUFSIZ];

	do {
		last_token = scanner_next(scanner);
		tokens[bufsiz++] = last_token;

		// If the buffer is full, concat to the parser.
		if (bufsiz == TOKEN_LOAD_BUFSIZ) {
			if (parser_append(parser, tokens, TOKEN_LOAD_BUFSIZ)
			    == 0) {
				printf("Algo ha salido mal"); // TODO: borrar
				exit(1);
			}
			bufsiz = 0;
		}
	} while (last_token && last_token->type != TOK_EOF);

	// Add the remaining tokens that did not fill the buffer.
	parser_append(parser, tokens, bufsiz);
}

static void
print_token(token_t *tok)
{
	if (tok->meta != 0) {
		printf("%s(%s)\n", tokentype_string(tok->type), tok->meta);
	} else {
		puts(tokentype_string(tok->type));
	}
}

void
parser_dump(parser_t *parser)
{
	printf("parser with %d tokens at pos %d\n", parser->len, parser->pos);
	for (int i = 0; i < parser->len; i++) {
		print_token(parser->tokens[i]);
	}
}

void __attribute__((noreturn))
parser_error(parser_t *parser, token_t *token, char *error)
{
	printf("Error: %s. ", error);
	print_token(token);
	printf("\n");
	exit(1);
}

token_t *
parser_peek(parser_t *parser)
{
	return parser->tokens[parser->pos];
}

token_t *
parser_token(parser_t *parser)
{
	return parser->tokens[parser->pos++];
}

expr_t *
parser_identifier(parser_t *parser)
{
	token_t *token = parser_token(parser);
	if (token->type == TOK_IDENTIFIER) {
		return new_literal(token);
	}
	parser_error(parser,
	             token,
	             "Token is of invalid type, expected IDENTIFIER");
}

expr_t *
parser_unsigned_integer(parser_t *parser)
{
	token_t *token = parser_token(parser);
	if (token->type == TOK_DIGIT) {
		return new_literal(token);
	}
	parser_error(parser, token, "Token is of invalid type, expected DIGIT");
}

void
parser_consume(parser_t *parser, tokentype_t type)
{
	token_t *token = parser_token(parser);
	if (token->type != type) {
		parser_error(parser, token, "Token is of invalid type");
	}
}

expr_t *
parser_unsigned_number(parser_t *parser)
{
	expr_t *integer_part, *decimal_part = NULL, *exp_part = NULL;
	token_t *e, *dot, *exp_sign = NULL;

	integer_part = parser_unsigned_integer(parser);
	dot = parser_peek(parser);
	if (dot->type == TOK_DOT) {
		parser_consume(parser, TOK_DOT);
		decimal_part = parser_unsigned_integer(parser);
	}

	e = parser_peek(parser);
	if (e->type == TOK_IDENTIFIER
	    && (e->meta[0] == 'e' || e->meta[0] == 'E') && (e->meta[1] == 0)) {
		parser_consume(parser, TOK_IDENTIFIER);

		exp_sign = parser_peek(parser);
		if (exp_sign->type == TOK_PLUS) {
			parser_consume(parser, TOK_PLUS);
		} else if (exp_sign->type == TOK_MINUS) {
			parser_consume(parser, TOK_MINUS);
		}
		exp_part = parser_unsigned_integer(parser);
	}

	printf("Número que he leído: ");
	printf("%s", integer_part->token->meta);
	if (decimal_part != NULL) {
		printf(".%s", decimal_part->token->meta);
	} else {
		printf(".0");
	}
	if (exp_part != NULL) {
		printf("E");
		if (exp_sign != NULL) {
			if (exp_sign->type == TOK_PLUS) {
				printf("+");
			} else if (exp_sign->type == TOK_MINUS) {
				printf("-");
			}
		} else {
			printf("+");
		}
		printf("%s\n", exp_part->token->meta);
	} else {
		printf("E+0");
	}
}
