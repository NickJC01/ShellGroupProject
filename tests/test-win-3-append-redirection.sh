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
rm -f "$OUT" "$EOUT" tests/append_output.txt

# Create initial file
echo "line0" > tests/append_output.txt

cat >"$LOG" << 'EOF'
Commands:
---------
echo line1 >> tests/append_output.txt
echo line2 >> tests/append_output.txt
cat tests/append_output.txt
---------
EOF

$LOBO >"$OUT" 2>&1 << 'EOF'
echo line1 >> tests/append_output.txt
echo line2 >> tests/append_output.txt
cat tests/append_output.txt
EOF

$BASH >"$EOUT" 2>&1 << 'EOF'
echo line1 >> tests/append_output.txt
echo line2 >> tests/append_output.txt
cat tests/append_output.txt
EOF

diff "$EOUT" "$OUT" >> "$LOG"
echo "---------" >> "$LOG"
diff "$EOUT" "$OUT" > /dev/null && echo "PASS $TEST" || echo "FAIL $TEST"
