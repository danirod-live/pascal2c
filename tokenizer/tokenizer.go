package tokenizer

import (
	"errors"
	"strings"
	"unicode"
)

type Tokenizer struct {
	buffer string
	cursor uint
}

func NewTokenizer(buffer []byte) *Tokenizer {
	return &Tokenizer{buffer: string(buffer), cursor: 0}
}

func (tok *Tokenizer) NextToken() *Token {
	if !tok.moreTokens() {
		return &Token{token: TokenEof}
	}
	nextToken := tok.nextToken()
	return &nextToken
}

func (tok *Tokenizer) nextToken() Token {
	switch ch := tok.peek(); {
	case ch == ':':
		if tok.peekFar(1) == '=' {
			tok.advance()
			tok.advance()
			return NewToken(TokenAssign)
		}
		tok.advance()
		return NewToken(TokenColon)
	case ch == '*':
		tok.advance()
		return NewToken(TokenAsterisk)
	case ch == ',':
		tok.advance()
		return NewToken(TokenComma)
	case ch == '.':
		tok.advance()
		return NewToken(TokenDot)
	case ch == '=':
		tok.advance()
		return NewToken(TokenEqual)
	case ch == '(':
		tok.advance()
		return NewToken(TokenLparen)
	case ch == '-':
		tok.advance()
		return NewToken(TokenMinus)
	case ch == '+':
		tok.advance()
		return NewToken(TokenPlus)
	case ch == ')':
		tok.advance()
		return NewToken(TokenRparen)
	case ch == ';':
		tok.advance()
		return NewToken(TokenSemicolon)
	case ch == '/':
		tok.advance()
		return NewToken(TokenSlash)
	case isStringStarter(ch):
		return tok.nextStringOrChar()
	case isIdentifierStarter(ch):
		return tok.nextIdentifier()
	case unicode.IsDigit(ch):
		return tok.nextDigit()
	default:
		err := errors.New("Unknown rune: " + string(ch))
		panic(err)
	}
}

func (tok *Tokenizer) nextStringOrChar() Token {
	var offt uint = 0
	for {
		switch tok.peekFar(offt) {
		case '\'':
			offt++
			for tok.peekFar(offt) != '\'' {
				offt++
			}
			offt++
		case '#':
			offt++
			for unicode.IsNumber(tok.peekFar(offt)) {
				offt++
			}
		default:
			// compose string and return
			str := tok.buffer[tok.cursor:(tok.cursor + offt)]
			tok.cursor += offt - 1
			tok.advance()
			return NewToken(TokenString, str)
		}
	}
}

func (tok *Tokenizer) nextIdentifier() Token {
	var offt uint = 1
	for isIdentifierLetter(tok.peekFar(offt)) {
		offt++
	}
	ident := tok.buffer[tok.cursor:(tok.cursor + offt)]
	tok.cursor += offt - 1
	tok.advance()

	// Maybe is a special identifier
	lower := strings.ToLower(ident)
	if toktype, found := keywordTokens[lower]; found {
		return NewToken(toktype)
	}
	return NewToken(TokenIdentifier, ident)
}

func (tok *Tokenizer) nextDigit() Token {
	var offt uint = 1
	for unicode.IsDigit(tok.peekFar(offt)) {
		offt++
	}
	number := tok.buffer[tok.cursor:(tok.cursor + offt)]
	tok.cursor += offt - 1
	tok.advance()
	return NewToken(TokenDigit, number)
}

func (tok *Tokenizer) moreTokens() bool {
	return tok.cursor < uint(len(tok.buffer))
}

func (tok *Tokenizer) advance() {
	for {
		tok.cursor++

		// we reach eof
		if !tok.moreTokens() {
			break
		}

		// skip whitespaces
		if unicode.IsSpace(tok.peek()) {
			continue
		}

		// skip comments until end of line
		if tok.peek() == '/' && tok.peekFar(1) == '/' {
			tok.advanceUntilToken('\n')
			continue
		}

		// if {comment}
		if tok.peek() == '{' {
			tok.advanceUntilToken('}')
			continue
		}

		// if (*comment*)
		if tok.peek() == '(' && tok.peekFar(1) == '*' {
			for {
				tok.advanceUntilToken('*')
				tok.cursor++
				if tok.peek() != ')' {
					continue
				}
				tok.cursor++
				break
			}
			continue
		}

		// this is a valid character.
		break
	}
}

func (tok *Tokenizer) advanceUntilToken(ch rune) {
	for rune(tok.buffer[tok.cursor]) != ch {
		tok.cursor++
	}
}

func (tok *Tokenizer) peek() rune {
	return rune(tok.buffer[tok.cursor])
}

func (tok *Tokenizer) peekFar(offt uint) rune {
	if tok.cursor+offt >= uint(len(tok.buffer)) {
		return 0
	}
	return rune(tok.buffer[tok.cursor+offt])
}
