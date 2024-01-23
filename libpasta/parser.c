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
dump_expr_impl(expr_t *expr, int indent)
{
	int i;

	if (!expr) {
		return;
	}

	for (i = 0; i < indent; i++) {
		printf("|");
		if (i == indent - 1) {
			printf("- ");
		} else {
			printf("  ");
		}
	}

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
	dump_expr_impl(expr->exp_left, indent + 1);
	dump_expr_impl(expr->exp_right, indent + 1);
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

static int
token_is_constant(token_t *token)
{
	switch (token->type) {
	case TOK_PLUS:
	case TOK_MINUS:
	case TOK_IDENTIFIER:
	case TOK_DIGIT:
	case TOK_NIL:
	case TOK_STRING:
		return 1;
	default:
		return 0;
	}
}

expr_t *
parse_identifier_list(parser_t *parser)
{
	expr_t *root = NULL, *next = root;
	token_t *token;

	for (;;) {
		/* read the next identifier (it has to be an identifier). */
		token = parser_peek(parser);
		parser_consume(parser, TOK_IDENTIFIER);

		/* add the token into the list of identifiers. */
		if (!root) {
			root = new_unary(token, NULL);
			next = root;
		} else {
			next->exp_left = new_unary(token, NULL);
			next = next->exp_left;
		}

		/* check if there are more tokens to parse. */
		token = parser_peek(parser);
		if (token->type != TOK_COMMA) {
			return root;
		}
		parser_consume(parser, TOK_COMMA);
	}
}

static expr_t *
parse_constant_list(parser_t *parser)
{
	expr_t *root = NULL, *next, *constant;
	token_t *token;

	for (;;) {
		/* parse the constant and add it to the chain. */
		constant = parser_constant(parser);
		if (!root) {
			root = new_binary(NULL, constant, NULL);
			next = root;
		} else {
			next->exp_right = new_binary(NULL, constant, NULL);
			next = next->exp_right;
		}

		/* check if there are more tokens to parse. */
		token = parser_peek(parser);
		if (token->type != TOK_COMMA) {
			return root;
		}
		parser_consume(parser, TOK_COMMA);
	}
}

static expr_t *
parser_field_list_branch(parser_t *parser)
{
	expr_t *constant, *fields;
	token_t *token;

	constant = parse_constant_list(parser);
	token = parser_token(parser);
	if (token->type != TOK_COLON) {
		parser_error(parser, token, "Expected a COLON here");
	}
	parser_consume(parser, TOK_LPAREN);
	fields = parser_field_list(parser);
	parser_consume(parser, TOK_RPAREN);

	return new_binary(token, constant, fields);
}

expr_t *
parser_field_list(parser_t *parser)
{
	expr_t *root = NULL, *next;
	expr_t *left, *idents, *type;
	token_t *token, *tokenpeek;

	/* Stage 1: normal fields list. */
	for (;;) {
		/* is a new identifier incoming? */
		token = parser_peek(parser);
		if (token->type != TOK_IDENTIFIER) {
			break;
		}

		/* build a new left_node for next_expr with the metadata of
		 * the current field list line. assuming that the line
		 * has the format [idents] : [type], this will be a binary
		 * node using this exact format: [idents] : [type] */

		/* get the identifier list separated by commas. */
		idents = parse_identifier_list(parser);
		token = parser_token(parser);
		if (token->type != TOK_COLON) {
			parser_error(parser,
			             token,
			             "COLON expected after field list");
		}
		type = parser_type(parser);

		/* craft the node and add it to the list. */
		left = new_binary(token, idents, type);

		if (!root) {
			root = new_binary(NULL, left, NULL);
			next = root;
		} else {
			next->exp_right = new_binary(NULL, left, NULL);
			next = next->exp_right;
		}

		/* there may be a semicolon here. */
		token = parser_peek(parser);
		if (token->type != TOK_SEMICOLON) {
			return root;
		}
		parser_consume(parser, TOK_SEMICOLON);
	}

	/* Stage 2: case line -- if there is one at all. */
	token = parser_peek(parser);
	if (token->type == TOK_CASE) {
		parser_consume(parser, TOK_CASE);

		/* two choices: [case x : t of] or [case t of]
		 * we will always craft a node rooted by of, with t on
		 * right there should always be an identifier following
		 * case, if that identifier is followed by : there is
		 * another one, else should be an of. */
		token = parser_token(parser);
		if (token->type != TOK_IDENTIFIER) {
			parser_error(parser,
			             token,
			             "Expected an identifier following "
			             "the CASE");
		}

		tokenpeek = parser_token(parser);
		switch (tokenpeek->type) {
		case TOK_OF: /* [case t of] */
			left = new_unary(tokenpeek, new_literal(token));
			break;
		case TOK_COLON: /* [case x : t of] */
			left = new_binary(tokenpeek, NULL, new_literal(token));
			token = parser_token(parser);
			if (token->type != TOK_IDENTIFIER) {
				parser_error(parser,
				             token,
				             "Expected an identifier following "
				             "the COLON");
			}

			tokenpeek = parser_token(parser);
			if (tokenpeek->type != TOK_OF) {
				parser_error(parser,
				             token,
				             "Expected OF after secondd token");
			}
			left->token = tokenpeek;
			left->exp_left = new_literal(token);
			break;
		default:
			parser_error(parser,
			             token,
			             "Expected either a COLON or OF");
		}

		if (!root) {
			root = new_binary(NULL, left, NULL);
			next = root;
		} else {
			next->exp_right = new_binary(NULL, left, NULL);
			next = next->exp_right;
		}

		/* and now comes a list of possible values for this case
		 * with their field lists. it is mandatory to have at
		 * least one, but there may be more. */
		left = parser_field_list_branch(parser);
		next->exp_right = new_binary(NULL, left, NULL);
		next = next->exp_right;

		for (;;) {
			/* if a semicolon continues, there is still
			 * more. */
			token = parser_peek(parser);
			if (token->type != TOK_SEMICOLON) {
				break;
			}

			/* wait, there's more */
			parser_consume(parser, TOK_SEMICOLON);
			left = parser_field_list_branch(parser);
			next->exp_right = new_binary(NULL, left, NULL);
			next = next->exp_right;
		}
	} // closes if (token->type == TOK_CASE)

	if (root == NULL) {
		/* there should be at least something, either a field or
		 * a case
		 */
		token = parser_peek(parser);
		parser_error(parser,
		             token,
		             "There should be either IDENT or CASE");
	}

	return root;
}

expr_t *
parser_variable(parser_t *parser)
{
	expr_t *root, *suffix = NULL, *ext = suffix;
	token_t *token;

	token = parser_token(parser);
	if (token->type != TOK_IDENTIFIER) {
		parser_error(parser,
		             token,
		             "Unexpected token is not an IDENTIFIER");
	}

	// TODO: should not allow an space here.
	token = parser_peek(parser);
	for (;;) {
		switch (token->type) {
		case TOK_CARET:
			token = parser_token(parser);
			if (suffix == NULL) {
				suffix = new_unary(token, NULL);
				ext = suffix;
			} else {
				ext->exp_left = new_unary(token, NULL);
				ext = ext->exp_left;
			}
			break;
		case TOK_LBRACKET:
			token = parser_token(parser);
			if (suffix == NULL) {
			} else {
			}
			break;
		case TOK_DOT:
			token = parser_token(parser);
			if (suffix == NULL) {
				suffix = new_binary(token, NULL, NULL);
				ext = suffix;
			} else {
				ext->exp_left = new_binary(token, NULL, NULL);
				ext = ext->exp_left;
			}
			token = parser_token(parser);
			if (token->type != TOK_IDENTIFIER) {
				parser_error(parser,
				             token,
				             "Expected TOK_IDENTIFIER");
			}
			ext->exp_right = new_literal(token);
			break;
		default:
			root = new_unary(token, suffix);
			return root;
		}
	}

	// Should not reach here!
	return NULL;
}
