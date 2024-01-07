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

token_t *
parser_validate_token(parser_t *parser, tokentype_t expected)
{
	token_t *next = parser_token(parser);
	if (next->type != expected) {
		parser_error(parser, next, "Token is not of expected type");
	}
	return next;
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
	if (token->type == TOK_IDENTIFIER || token->type == TOK_NIL
	    || token->type == TOK_DIGIT) {
		wrapped = new_literal(token);
		if (sign_token == NULL) {
			return wrapped;
		} else {
			return new_unary(sign_token, wrapped);
		}
	}

	parser_error(parser, next_token, "Unexpected token type");
}

expr_t *
parser_simple_type(parser_t *parser)
{
	token_t *next_symbol, *next_id;
	expr_t *root, *next_node;

	next_symbol = parser_peek(parser);
	if (next_symbol->type == TOK_LPAREN) {
		next_symbol = parser_validate_token(parser, TOK_LPAREN);
		// Branch 2 - (identifiers separated by commas inside brackets)
		root = new_binary(next_symbol, NULL, NULL);
		next_node = root;

		while (1) {
			// Assign left branch
			next_node->exp_left = parser_identifier(parser);

			// Pick what goes on the right branch
			next_symbol = parser_token(parser);
			if (next_symbol->type == TOK_RPAREN) {
				next_node->exp_right = new_literal(next_symbol);
				break;
			} else if (next_symbol->type == TOK_COMMA) {
				next_node->exp_right =
				    new_binary(next_symbol, NULL, NULL);
				next_node = next_node->exp_right;
			} else {
				parser_error(parser,
				             next_symbol,
				             "Expected either COMMA or RPAREN");
			}
		}
	} else {
		next_node = parser_constant(parser);
		next_symbol = parser_peek(parser);
		if (next_symbol->type == TOK_DOTDOT) {
			// Branch 3 - (two identifiers between a ..)
			next_symbol = parser_validate_token(parser, TOK_DOTDOT);
			root = new_binary(next_symbol, NULL, NULL);
			root->exp_left = next_node;
			root->exp_right = parser_constant(parser);
		} else {
			// Branch 1 - identifier alone
			root = new_grouping(next_node);
		}
	}

	return root;
}

expr_t *
parser_type(parser_t *parser)
{
	token_t *next_token, *packed = NULL;
	expr_t *root, *next_expr;

	next_token = parser_peek(parser);

	if (next_token->type == TOK_PACKED) {
		packed = next_token;
		parser_consume(parser, TOK_PACKED);
		next_token = parser_peek(parser);
	}

	switch (next_token->type) {
	case TOK_CARET:
		if (packed) {
			parser_error(parser,
			             next_token,
			             "CARET cannot be PACKED");
		}
		root = new_unary(next_token, NULL);
		parser_consume(parser, TOK_CARET);
		root->exp_left = parser_identifier(parser);
		break;
	case TOK_ARRAY:
		// Consume TOK_ARRAY
		root = new_binary(next_token, NULL, NULL);
		parser_validate_token(parser, TOK_ARRAY);

		// Consume the list of simple types that goes between []
		next_token = parser_validate_token(parser, TOK_LBRACKET);
		root->exp_left = new_binary(next_token, NULL, NULL);
		next_expr = root->exp_left;

		while (1) {
			next_expr->exp_left = parser_simple_type(parser);

			next_token = parser_token(parser);
			if (next_token->type == TOK_COMMA) {
				next_expr->exp_right =
				    new_binary(next_token, NULL, NULL);
				next_expr = next_expr->exp_right;
			} else if (next_token->type == TOK_RBRACKET) {
				next_expr->exp_right = new_literal(next_token);
				break;
			} else {
				parser_error(
				    parser,
				    next_token,
				    "Expected either RBRACKET or COMMA");
			}
		}

		parser_validate_token(parser, TOK_OF);
		root->exp_right = parser_type(parser);
		break;
	case TOK_FILE:
		// Consume TOK_FILE
		root = new_unary(next_token, NULL);
		parser_validate_token(parser, TOK_FILE);

		// Must follow an OF
		parser_validate_token(parser, TOK_OF);

		// Recursive definition
		root->exp_left = parser_type(parser);
		break;
	case TOK_SET:
		// Consume TOK_SET
		root = new_unary(next_token, NULL);
		parser_validate_token(parser, TOK_SET);

		// Must follow an OF
		parser_validate_token(parser, TOK_OF);

		// Must follow a simple type
		root->exp_left = parser_simple_type(parser);
		break;
	case TOK_RECORD:
		// Consume RECORD
		root = new_unary(next_token, NULL);
		parser_validate_token(parser, TOK_RECORD);

		root->exp_left = parser_field_list(parser);

		// Must follow an END.
		parser_validate_token(parser, TOK_END);
		break;
	default:
		if (packed) {
			parser_error(parser,
			             next_token,
			             "Cannot use PACKED in this context");
		}
		root = parser_simple_type(parser);
		break;
	}

	// Wrap in a PACKED if we previously saw the packed keyword.
	if (packed) {
		root = new_unary(packed, root);
	}

	return root;
}

expr_t *
parser_field_list(parser_t *parser)
{
	expr_t *root, *next_token;
	token_t *token = parser_peek(parser);
	int is_first = 1;

	if (token->type == TOK_CASE) {
		parser_error(parser, token, "Unexpected CASE, not implemented");
	}

	root = new_binary(NULL, NULL, NULL);
	next_token = root;

	// each one of the field line in the field list
	for (;;) {
		// each one of the identifiers in this field line
		for (;;) {
			token = parser_peek(parser);
			if (token->type != TOK_IDENTIFIER) {
				if (is_first) {
					parser_error(parser,
					             token,
					             "Unexpected symbol, "
					             "expected IDENTIFIER");
				} else {
					return root;
				}
			}
			next_token->exp_left = parser_identifier(parser);
			is_first = 0;

			// Whether there is a comma or a colon. If none, then it
			// is an error.
			token = parser_peek(parser);
			if (token->type == TOK_COMMA) {
				next_token->exp_right =
				    new_binary(token, NULL, NULL);
				parser_validate_token(parser, TOK_COMMA);
				next_token = next_token->exp_right;
			} else if (token->type == TOK_COLON) {
				next_token->exp_right =
				    new_binary(token, NULL, NULL);
				parser_validate_token(parser, TOK_COLON);
				next_token = next_token->exp_right;
				break;
			} else {
				parser_error(parser,
				             token,
				             "Unexpected token, expected "
				             "either COMMA or COLON");
			}
		}

		// The list of identifiers is over, now we read the type.
		next_token->exp_left = parser_identifier(parser);

		// check if we have another loop
		token = parser_peek(parser);
		if (token->type == TOK_SEMICOLON) {
			next_token->exp_right = new_binary(token, NULL, NULL);
			next_token = next_token->exp_right;
			parser_validate_token(parser, TOK_SEMICOLON);
		} else {
			break;
		}
	}

	return root;
}
