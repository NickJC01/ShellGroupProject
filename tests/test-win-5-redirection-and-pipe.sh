#!/bin/bash
TEST=$(basename "${0%.sh}")
OUTPUT=$(realpath "$(dirname "$0")")/output
OUT=$OUTPUT/$TEST.out
EOUT=$OUTPUT/$TEST.eout
LOG=$OUTPUT/$TEST.log
LOBO="./lobo_shell.x"
BASH=$(which bash)

[ "$(basename "$(dirname "$(realpath "$0")")")" != "tests" ] && { echo "FAIL: $0 not in 'tests'"; exit 1; }
[ ! -x $LOBO ] && { echo "FAIL: $LOBO must exist"; exit 2; }

mkdir -p "$OUTPUT"
rm -f "$OUT" "$EOUT" tests/pipe_input.txt tests/pipe_output.txt

# Create input file
echo -e "alpha\nbeta\ngamma" > tests/pipe_input.txt

cat >"$LOG" << 'EOF'
Commands:
---------
cat < tests/pipe_input.txt | grep a > tests/pipe_output.txt
cat tests/pipe_output.txt
---------
EOF

$LOBO >"$OUT" 2>&1 << 'EOF'
cat < tests/pipe_input.txt | grep a > tests/pipe_output.txt
cat tests/pipe_output.txt
EOF

$BASH >"$EOUT" 2>&1 << 'EOF'
cat < tests/pipe_input.txt | grep a > tests/pipe_output.txt
cat tests/pipe_output.txt
EOF

diff "$EOUT" "$OUT" >> "$LOG"
echo "---------" >> "$LOG"
diff "$EOUT" "$OUT" > /dev/null && echo "PASS $TEST" || echo "FAIL $TEST"
