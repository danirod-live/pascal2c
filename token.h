#pragma once

typedef enum tokentype {
	TOK_EOF,

	TOK_ASSIGN,
	TOK_ASTERISK,
	TOK_BEGIN,
	TOK_COLON,
	TOK_COMMA,
	TOK_CTRLCODE,
	TOK_CONST,
	TOK_DIGIT,
	TOK_DIV,
	TOK_DOT,
	TOK_END,
	TOK_EQUAL,
	TOK_IDENTIFIER,
	TOK_LPAREN,
	TOK_MINUS,
	TOK_MOD,
	TOK_PLUS,
	TOK_PROGRAM,
	TOK_SLASH,
	TOK_STRING,
	TOK_RPAREN,
	TOK_SEMICOLON,
	TOK_VAR,
} tokentype_t;

const char *tokentype_string(tokentype_t token);
