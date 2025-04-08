TEST=$(basename "${0%.sh}")
OUTPUT=$(realpath "$(dirname "$0")")/output
OUT="$OUTPUT/$TEST.out"
EOUT="$OUTPUT/$TEST.eout"
LOG="$OUTPUT/$TEST.log"
LOBO="./lobo_shell.x"
BASH=$(which bash)

test "tests" != "$(basename "$(dirname "$(realpath "$0")")")" && { echo "FAIL: $0 not in 'tests'"; exit 1; }
[ ! -x "$LOBO" ] && { echo "FAIL: $LOBO must exist"; exit 2; }

mkdir -p "$OUTPUT"
rm -f "$OUT" "$EOUT" tests/output/tmp

umask 002

cat >"$LOG" << 'EOF'
Commands:
---------
echo "This is double quoted text" > tests/output/tmp
echo 'This is single quoted text' >> tests/output/tmp
cat tests/output/tmp
---------
EOF

# LOBO
"$LOBO" >"$OUT" 2>&1 << 'EOF'
echo "This is double quoted text" > tests/output/tmp
echo 'This is single quoted text' >> tests/output/tmp
cat tests/output/tmp
EOF

# Bash
"$BASH" >"$EOUT" 2>&1 << 'EOF'
echo "This is double quoted text" > tests/output/tmp
echo 'This is single quoted text' >> tests/output/tmp
cat tests/output/tmp
EOF

# Check file exists and contents
if [ ! -s tests/output/tmp ]; then
    echo "FAIL $TEST: tmp not created or empty" >> "$LOG"
    ls -l tests/output/tmp >> "$LOG" 2>&1
    echo "FAIL $TEST"
    exit 1
fi

# Check permissions
if [ -z "$(find tests/output/tmp -perm -664)" ]; then
    echo "FAIL $TEST: tmp permissions wrong" >> "$LOG"
    stat tests/output/tmp >> "$LOG"
    echo "FAIL $TEST"
    exit 2
fi

# Log debug output
echo "[DEBUG] TMP CONTENTS:" >> "$LOG"
cat tests/output/tmp >> "$LOG"
echo "[DEBUG] EOUT:" >> "$LOG"
cat "$EOUT" >> "$LOG"
echo "[DEBUG] OUT:" >> "$LOG"
cat "$OUT" >> "$LOG"

# Compare
diff <(sed -E 's/[[:space:]]+$//' "$EOUT") <(sed -E 's/[[:space:]]+$//' "$OUT") >> "$LOG"
echo "---------" >> "$LOG"

# Final pass/fail
if diff -q <(sed -E 's/[[:space:]]+$//' "$EOUT") <(sed -E 's/[[:space:]]+$//' "$OUT") >/dev/null; then
    echo "PASS $TEST"
else
    echo "FAIL $TEST"
fi
