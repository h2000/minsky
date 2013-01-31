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

#check addWire business rules
cat >input.tcl <<EOF
source $here/test/assert.tcl
minsky.addOperation exp
minsky.addOperation exp
assert {[minsky.addWire 0 3 ]==-1} "wire needs coordinates"
set wireAdded [minsky.addWire 0 3 {0 0 0 0}]
assert "\$wireAdded!=-1" ""
assert {[minsky.addWire 0 3 {0 0 0 0}]==-1} "duplicate wire added!"
assert {[minsky.addWire 0 1 {0 0 0 0}]==-1} "self wire allowed!"
assert {[minsky.addWire 1 2 {0 0 0 0}]==-1} "input wired to output!"
minsky.wire.get \$wireAdded
assert {[minsky.wire.from]==0 && [minsky.wire.to]==3} ""
minsky.deleteWire \$wireAdded
assert {[minsky.wires.size]==0} ""
exit
EOF

$here/minsky input.tcl
if test $? -ne 0; then fail; fi

pass
