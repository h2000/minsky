#!../minsky

# usage minsky createFileLog <system.mky>

# writes a log file on stdout for system <system.mky>

source $minskyHome/library/init.tcl
use_namespace minsky
minsky.load $argv(2)

# prepare element accessors for later use
foreach name [variables.values.#keys] {
        variables.values.@elem $name
}
use_namespace minsky

set nsteps 10
puts "nsteps 10"
for {set step 0} {$step<$nsteps} {incr step} {
    step
    puts -nonewline "[t] "
    foreach name [variables.values.#keys] {
        puts -nonewline "$name [variables.values($name).value] "
    }
    puts ""
}
exit
