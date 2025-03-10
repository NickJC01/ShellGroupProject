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
# Check that valgrind exists
#
valgrind -h > /dev/null 2>&1
if [ $? != 0 ]; then
    echo "SKIP $TEST"
    exit
fi

#
# Test with LOBO
#
valgrind --leak-check=summary $LOBO >$OUT 2>&1 << 'EOF'
whoami
EOF

#
# Analyze results
#
BYTES=`cat $OUT | grep "definitely lost" | tr -s ' ' | cut --delimiter=' ' -f4`
INUSE=`cat $OUT | grep "in use at exit" | tr -s ' ' | cut --delimiter=' ' -f6`

echo "Valgrind reported:" >> $LOG
cat $OUT >> $LOG

if [ "$BYTES" == 0 ] || [ "$INUSE" == 0 ]; then
    echo "PASS $TEST"
else
    echo "FAIL $TEST"
fi
