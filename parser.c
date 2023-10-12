#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "scanner.h"
#include "token.h"

static void
print_token(token_t *tok)
{
	if (tok == 0) {
		return;
	}
	if (tok->meta != 0) {
		printf("%s(%s)\n", tokentype_string(tok->type), tok->meta);
	} else {
		puts(tokentype_string(tok->type));
	}
}

static void
dump_expr_impl(expr_t *expr, int padding)
{
	if (!expr) {
		return;
	}
	char *space = malloc(sizeof(char) * padding + 1);
	memset(space, ' ', padding);
	space[padding] = 0;

	printf("%s", space);
	switch (expr->type) {
	case BINARY:
		printf("BINARY ");
		break;
	case UNARY:
		printf("UNARY ");
		break;
	case GROUPING:
		printf("GROUPING ");
		break;
	case LITERAL:
		printf("LITERAL ");
		break;
	}
	print_token(expr->token);
	dump_expr_impl(expr->exp_left, padding + 2);
	dump_expr_impl(expr->exp_right, padding + 2);
	free(space);
}

void
dump_expr(expr_t *expr)
{
	dump_expr_impl(expr, 0);
}

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

static int
is_clean_integer(char *number)
{
	while (number && *number) {
		if (*number < '0' || *number > '9') {
			return 0;
		}
		number++;
	}
	return 1;
}

expr_t *
parser_unsigned_integer(parser_t *parser)
{
	token_t *token = parser_token(parser);
	if (token->type == TOK_DIGIT) {
		if (is_clean_integer(token->meta)) {
			return new_literal(token);
		}
		parser_error(parser, token, "Expected integer");
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
	token_t *token = parser_token(parser);
	if (token->type == TOK_DIGIT) {
		return new_literal(token);
	}
	parser_error(parser, token, "Token is of invalid type, expected DIGIT");
}

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

// TODO: Should review the AST, because this is going to be nuts in the future.
expr_t *
parser_constant(parser_t *parser)
{
	token_t *token;
	token_t *next_token = parser_peek(parser);
	expr_t *wrapped;
	token_t *sign_token = NULL;

	if (next_token->type == TOK_STRING) {
		token = parser_token(parser);
		return new_literal(token);
	}

	if (next_token->type == TOK_PLUS || next_token->type == TOK_MINUS) {
		sign_token = parser_token(parser);
	}

	token = parser_token(parser);
	if (token->type == TOK_IDENTIFIER || token->type == TOK_DIGIT) {
		wrapped = new_literal(token);
		if (sign_token == NULL) {
			return wrapped;
		} else {
			return new_unary(sign_token, wrapped);
		}
	}

	parser_error(parser, next_token, "Unexpected token type");
}
