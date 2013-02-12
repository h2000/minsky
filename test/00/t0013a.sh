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

# double entry bookkeeping tests

# insert ecolab script code here
# use \$ in place of $ to refer to variable contents
# exit 0 to indicate pass, and exit 1 to indicate failure
cat >input.tcl <<EOF
source $here/test/assert.tcl
set godley [minsky.addGodleyTable 0 0]
minsky.godleyItem.get \$godley
minsky.godleyItem.table.clear
minsky.godleyItem.table.resize 4 4
minsky.godleyItem.table.setDEmode 1
minsky.godleyItem.table.setCell 1 0 "Initial Conditions"
minsky.godleyItem.table.setCell 1 1 10
minsky.godleyItem.table.setCell 1 3 -10
minsky.godleyItem.table.setCell 2 1 a
minsky.godleyItem.table.setCell 2 2 b
minsky.godleyItem.table.setCell 2 3 -a

# TODO - this test will need to be modified when ticket #58 is dealt with
minsky.godleyItem.table.assetClass 1 asset
minsky.godleyItem.table.assetClass 2 liability
minsky.godleyItem.table.assetClass 3 liability
assert {[minsky.godleyItem.table.rowSum 1]==0} ""
assert {[minsky.godleyItem.table.rowSum 2]=="b"} ""
assert {"[minsky.godleyItem.table.getVariables]"=="a b"} ""

exit
EOF

$here/minsky input.tcl
if test $? -ne 0; then fail; fi

pass
