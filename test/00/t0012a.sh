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

# basic godley table tests

# insert ecolab script code here
# use \$ in place of $ to refer to variable contents
# exit 0 to indicate pass, and exit 1 to indicate failure
cat >input.tcl <<EOF
source $here/test/assert.tcl
set godley [minsky.addGodleyTable 0 0]
minsky.godleyItem.get \$godley
minsky.godleyItem.table.clear
assert {[minsky.godleyItem.table.rows]==0} ""
assert {[minsky.godleyItem.table.cols]==0} ""

# check insertRow/insertCol
minsky.godleyItem.table.insertRow 0
minsky.godleyItem.table.insertCol 0
assert {[minsky.godleyItem.table.rows]==1} ""
assert {[minsky.godleyItem.table.cols]==1} ""
minsky.godleyItem.table.setCell 0 0 11
assert {[minsky.godleyItem.table.getCell 0 0]==11} ""
minsky.godleyItem.table.insertRow 0
minsky.godleyItem.table.insertCol 0
assert {[minsky.godleyItem.table.rows]==2} ""
assert {[minsky.godleyItem.table.cols]==2} ""
assert {[minsky.godleyItem.table.getCell 1 1]==11} ""
minsky.godleyItem.table.insertRow 2
minsky.godleyItem.table.insertCol 2
assert {[minsky.godleyItem.table.rows]==3} ""
assert {[minsky.godleyItem.table.cols]==3} ""
assert {[minsky.godleyItem.table.getCell 1 1]==11} ""
for {set r 0} {\$r<3} {incr r} {
  for {set c 0} {\$c<3} {incr c} {
     minsky.godleyItem.table.setCell \$r \$c \$r\$c
  }
}
# test column variables
assert {"[minsky.godleyItem.table.getColumnVariables]"=="01 02"} ""
# interior variables
assert {"[minsky.godleyItem.table.getVariables]"=="11 12 21 22"} ""

# test delete column
minsky.godleyItem.table.deleteCol 2
assert {"[minsky.godleyItem.table.getColumnVariables]"=="02"} ""
assert {"[minsky.godleyItem.table.getVariables]"=="12 22"} ""
assert {[minsky.godleyItem.table.getCell 1 1]==12} ""
minsky.godleyItem.table.deleteRow 2
assert {"[minsky.godleyItem.table.getColumnVariables]"=="02"} ""
assert {"[minsky.godleyItem.table.getVariables]"=="22"} ""
assert {[minsky.godleyItem.table.getCell 1 1]==22} ""
assert {[minsky.godleyItem.table.rows]==2} ""
assert {[minsky.godleyItem.table.cols]==2} ""

assert {"[minsky.godleyItem.table.getVariables]"=="22"} ""

# test rowsum, and stricter test of get variables
minsky.godleyItem.table.clear
minsky.godleyItem.table.resize 4 4
assert {[minsky.godleyItem.table.rows]==4} ""
assert {[minsky.godleyItem.table.cols]==4} ""
minsky.godleyItem.table.setCell 1 0 "Initial Conditions"
minsky.godleyItem.table.setCell 1 1 10
minsky.godleyItem.table.setCell 1 3 -10
minsky.godleyItem.table.setCell 2 1 a
minsky.godleyItem.table.setCell 2 2 b
minsky.godleyItem.table.setCell 2 3 -a
assert {[minsky.godleyItem.table.rowSum 1]==0} ""
assert {[minsky.godleyItem.table.rowSum 2]=="b"} ""
assert {"[minsky.godleyItem.table.getVariables]"=="a b"} ""

minsky.deleteGodleyTable \$godley
assert {[minsky.godleyItems.size]==0} ""
exit
EOF

$here/minsky input.tcl
if test $? -ne 0; then fail; fi

pass
