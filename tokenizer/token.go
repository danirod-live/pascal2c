package tokenizer

import "fmt"

func NewToken(t TokenType, meta ...string) Token {
	if len(meta) == 1 {
		return Token{token: t, meta: meta[0]}
	}
	return Token{token: t, meta: ""}
}

type Token struct {
	token TokenType
	meta  string
}

func (t *Token) String() string {
	if t.meta != "" {
		return fmt.Sprintf("%s(%s)", t.token.String(), t.meta)
	}
	return t.token.String()
}

func (t *Token) Eof() bool {
	return t.token == TokenEof
}
