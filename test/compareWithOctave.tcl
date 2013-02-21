minsky.load $argv(2)
minsky.reset
minsky.matlab $argv(2).m

# run the simulation for a while, and output the data values
set dat [open $argv(2).dat w]
puts $dat "d=\["
for {set i 0} {$i<20} {incr i} {
    minsky.step
    puts -nonewline $dat "\[[minsky.t] "
    foreach var [minsky.variables.stockVars] {
        minsky.value.get $var
        puts -nonewline $dat "[minsky.value.value] "
    }
    puts $dat "\];"
}
puts $dat "\];"
exit
