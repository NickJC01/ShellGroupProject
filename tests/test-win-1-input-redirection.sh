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
rm -f "$OUT" "$EOUT"

# Create test input file
echo -e "line1\nline2\nline3" > tests/test_input.txt

# Log what we're running
cat >"$LOG" << 'EOF'
Commands:
---------
wc -l < tests/test_input.txt
---------
EOF

# Run LOBO with real redirected stdin (not here-doc)
echo "wc -l < tests/test_input.txt" | $LOBO >"$OUT" 2>&1

# Run with Bash for comparison
echo "wc -l < tests/test_input.txt" | $BASH >"$EOUT" 2>&1

# Compare
diff "$EOUT" "$OUT" >> "$LOG"
echo "---------" >> "$LOG"

if diff "$EOUT" "$OUT" > /dev/null; then
    echo "PASS $TEST"
else
    echo "FAIL $TEST"
    echo "--- Expected ---"
    cat "$EOUT"
    echo "--- Got ---"
    cat "$OUT"
fi
