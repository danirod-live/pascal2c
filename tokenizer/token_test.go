package tokenizer

import "testing"

func TestTokenWithMetaToString(t *testing.T) {
	tok := Token{token: TokenDigit, meta: "12345"}
	exp := "TOK_DIGIT(12345)"
	if act := tok.String(); act != exp {
		t.Errorf("Expected tok.String() to be %s, was %s", exp, act)
	}
}

func TestTokenWithoutMetaToString(t *testing.T) {
	tok := Token{token: TokenBegin}
	exp := "TOK_BEGIN"
	if act := tok.String(); act != exp {
		t.Errorf("Expected tok.String() to be %s, was %s", exp, act)
	}
}
