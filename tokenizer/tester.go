package tokenizer

func isIdentifierStarter(ch rune) bool {
	return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch == '_')
}

func isIdentifierLetter(ch rune) bool {
	return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch == '_')
}

func isStringStarter(ch rune) bool {
	return ch == '\'' || ch == '#'
}
