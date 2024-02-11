#!/bin/sh

cd "$(dirname "$0")"

function testcase() {
	INPUT_MODE="$1"
	INPUT_FILE="$2"
	EXPECTED_FILE="$3"
	OUTPUT_FILE=$(mktemp)

	../build/repl -e$1 -q < $2 > $OUTPUT_FILE

	if [[ $? != 0 ]]; then
		return 1
	fi

	diff --color -u "$EXPECTED_FILE" "$OUTPUT_FILE"
	return "$?"
}

function updatecase() {
	INPUT_MODE="$1"
	INPUT_FILE="$2"
	EXPECTED_FILE="$3"

	../build/repl -e$1 -q < $2 > $3
	return $?
}

# If the expected file does not exist, it should fail, but should
# create the file with the contents of the curernt output for the
# given input.
#
# If the given input does not exist, it should fail.
#
# If both files exist but they are different, it should fail.
#
# If both files exists but they are different and you use a special
# flag, it should update the snapshot.
#
# If both files exist and they are same, return 0

EXIT_CODE=0
UPDATE=0

if [[ "$1" == "-update" ]]; then
	UPDATE=1
fi

function assert_output() {
	if ! [ -f "$2" ] ; then
		echo "[crit] file $2 not found"
		EXIT_CODE=1
	elif ! [ -f "$3" ] ; then
		echo "[fail] file $3 not found"
		if [[ "$UPDATE" == "1" ]] ; then
			echo "Updating snapshot $3..."
			updatecase "$1" "$2" "$3"
		fi
		EXIT_CODE=1
	elif ! testcase "$1" "$2" "$3" ; then
		echo "[fail] $1 / $2 "
		EXIT_CODE=1
		if [[ "$UPDATE" == "1" ]] ; then
			echo "Updating snapshot $3..."
			updatecase "$1" "$2" "$3"
		fi
	else
		echo "[ ok ] $1 / $2"
	fi
}

function assert_fails() {
	if ! [ -f "$2" ] ; then
		echo "[crit] file $2 not found"
		EXIT_CODE=1
	elif ../build/repl -e$1 -q < $2 >/dev/null 2>&1 ; then
		echo "[fail] $1 / $2  expected to fail"
	else
		echo "[ ok ] $1 / $2"
	fi
}

assert_output identifier ident_ok.pas ident_ok.exp
assert_fails identifier ident_fail.pas
assert_output variable variable_normal.pas variable_normal.exp
assert_output variable variable_idx.pas variable_idx.exp
assert_output variable variable_dot.pas variable_dot.exp
assert_output variable variable_caret.pas variable_caret.exp
assert_output variable variable_complex.pas variable_complex.exp

exit $EXIT_CODE
