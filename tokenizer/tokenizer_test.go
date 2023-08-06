package tokenizer

import "testing"

func TestPeekReturnsRuneAtPosition(t *testing.T) {
	cases := []struct {
		buffer   string
		position uint
		exp      rune
	}{
		{buffer: "program", position: 1, exp: 'r'},
		{buffer: "begin;", position: 0, exp: 'b'},
		{buffer: "begin;", position: 5, exp: ';'},
	}

	for _, tt := range cases {
		tok := Tokenizer{buffer: tt.buffer, cursor: tt.position}
		if act := tok.peek(); act != tt.exp {
			t.Errorf("Expected peek() to yield %v, did %v", tt.exp, act)
		}
	}
}

func TestAdvanceYieldsNextRune(t *testing.T) {
	cases := []struct {
		buffer   string
		position uint
		exp      uint
	}{
		{buffer: "program", position: 0, exp: 1},
		{buffer: "program helloworld", position: 6, exp: 8},
		{buffer: "begin;\nx := 5;\nend.", position: 5, exp: 7},
		{buffer: "begin;\n\tx := 5;\nend.", position: 5, exp: 8},
		{buffer: "program", position: 6, exp: 7},
	}

	for _, tt := range cases {
		tok := Tokenizer{buffer: tt.buffer, cursor: tt.position}
		tok.advance()
		if tok.cursor != tt.exp {
			t.Errorf("Expected cursor to be at position %d, was at %d", tt.exp, tok.cursor)
		}
	}
}
