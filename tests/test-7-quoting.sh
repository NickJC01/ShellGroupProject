TEST=$(basename "${0%.sh}")
OUTPUT=$(realpath $(dirname $0))/output
OUT=$OUTPUT/$TEST.out
EOUT=$OUTPUT/$TEST.eout
LOG=$OUTPUT/$TEST.log
LOBO="./lobo_shell.x"
BASH=`which bash`

test "tests" != "$(basename $(dirname $(realpath $0)))" && { echo "FAIL: $0 not in 'tests'"; exit 1; }
! test -x $LOBO && { echo "FAIL: $LOBO must exist"; exit 2; }
mkdir -p $OUTPUT
rm -f $OUT $EOUT

#
# Test Case
#
cat >$LOG << 'EOF'
Commands:
---------
echo "A B C > tests/output/shouldnt-exist"
---------
EOF

#
# Test with LOBO
#
$LOBO >$OUT 2>&1 << 'EOF'
echo "A B C > tests/output/shouldnt-exist"
EOF

#
# Test with BASH
#
$BASH >$EOUT 2>&1 << 'EOF'
echo "A B C > tests/output/shouldnt-exist"
EOF

#
# Analyze results
#
if [ -f tests/output/shouldnt-exist ]; then
    echo "tests/output/shouldnt-exist exists" >> $LOG
    ls -al tests/output/shouldnt-exist >> $LOG 2>&1
    echo "FAIL $TEST"
    rm -f tests/output/shouldnt-exist
    exit
fi

diff $EOUT $OUT >> $LOG
echo "---------" >> $LOG

diff $EOUT $OUT >/dev/null && echo "PASS $TEST" || echo "FAIL $TEST"
