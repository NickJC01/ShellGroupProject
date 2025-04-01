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
rm -f "$OUT" "$EOUT" tests/output_test.txt

cat >"$LOG" << 'EOF'
Commands:
---------
printf "Hello\n" > tests/output_test.txt
cat tests/output_test.txt
---------
EOF

# Run LOBO
$LOBO >"$OUT" 2>&1 << 'EOF'
printf "Hello\n" > tests/output_test.txt
cat tests/output_test.txt
EOF

# Run Bash
$BASH >"$EOUT" 2>&1 << 'EOF'
printf "Hello\n" > tests/output_test.txt
cat tests/output_test.txt
EOF

# Normalize endings (optional, if running on mixed platforms)
# dos2unix "$OUT" "$EOUT" 2>/dev/null

echo "--- Expected Output ---"
cat "$EOUT"
echo "--- Actual Output ---"
cat "$OUT"

diff "$EOUT" "$OUT" >> "$LOG"
echo "---------" >> "$LOG"
diff "$EOUT" "$OUT" > /dev/null && echo "PASS $TEST" || echo "FAIL $TEST"
