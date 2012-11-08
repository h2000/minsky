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

# check that old schmea can still be read correctly
cd $here/test/oldSchema/schema0
for i in *.mky; do
    $here/minsky $here/test/rewriteMky.tcl $i $tmp/tmp.mky
    if test $? -ne 0; then fail; fi
    $here/minsky $here/test/rewriteMky.tcl $tmp/tmp.mky $tmp/tmp1.mky
    if test $? -ne 0; then fail; fi
    diff -q -w  $here/examples/$i $tmp/tmp1.mky
    if test $? -ne 0; then 
        echo "old schema file $i failed to convert"
        fail
    fi
done

pass
