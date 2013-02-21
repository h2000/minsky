#! /bin/sh

here=`pwd`
if test $? -ne 0; then exit 2; fi
tmp=/tmp/$$
mkdir $tmp
if test $? -ne 0; then exit 2; fi
cd $tmp
if test $? -ne 0; then exit 2; fi

fail()
{
    echo "FAILED" 1>&2
    cd $here
    chmod -R u+w $tmp
    rm -rf $tmp
    exit 1
}

pass()
{
    echo "PASSED" 1>&2
    cd $here
    chmod -R u+w $tmp
    rm -rf $tmp
    exit 0
}

trap "fail" 1 2 3 15

# checks whether Runge-Kutta solver works correctly on example files
# by comapring with Octave
cp -r $here/examples .
for i in examples/*.mky; do
    echo "doing: $i"
    $here/minsky $here/test/compareWithOctave.tcl $i
    if test $? -ne 0; then fail; fi
    octave --silent $here/test/compareWithOctave.m $i
    if test $? -ne 0; then fail; fi
done
pass
