#!/opt/local/bin/tclsh

source ../godley.tcl

proc a {s} {
    set prefix ""
    set varName ""
    return [list $s [list [parse_input $s prefix varName] $prefix $varName]]
}

set testset {
"variable"
" variable"
" variable "
"variable "
"-variable"
"--variable"
"- variable"
" - variable"
" -- variable"
"cr variable"
" cr variable"
" cr  variable"
" cr -variable"
"Dr variable"
" dR variable"
"  dr  variable"
" DR -variable"
" DR -variable "
" DR --variable"
" DR - variable"
" DR --- variable"
" DR -variable = 98.0"
"DR = 98.0"
"- = 98.0"
"dr - a = 98.0"
""
"-"
"-dr a"
"--"
"dr"
"drdr"
"dr dr"
"dr dre"
"dr dre e"
"dr dre dre"
"dr dr e"
"dr drdrdr e"
"dr drCRdr"
"dr dr dr dr"
"dr dog"
"- dog"
"-dr"
"dr - dr - dr e"
" - dr e"
"dog"
}

set resultset  {
variable {1 {} variable}
{ variable} {1 {} variable}
{ variable } {1 {} variable}
{variable } {1 {} variable}
-variable {1 - variable}
--variable {1 - variable}
{- variable} {1 - variable}
{ - variable} {1 - variable}
{ -- variable} {1 - variable}
{cr variable} {1 CR variable}
{ cr variable} {1 CR variable}
{ cr  variable} {1 CR variable}
{ cr -variable} {0 {} {}}
{Dr variable} {1 DR variable}
{ dR variable} {1 DR variable}
{  dr  variable} {1 DR variable}
{ DR -variable} {0 {} {}}
{ DR -variable } {0 {} {}}
{ DR --variable} {0 {} {}}
{ DR - variable} {0 {} {}}
{ DR --- variable} {0 {} {}}
{ DR -variable = 98.0} {0 {} {}}
{DR = 98.0} {0 {} {}}
{- = 98.0} {0 {} {}}
{dr - a = 98.0} {0 {} {}}
{} {0 {} {}}
- {0 {} {}}
{-dr a} {0 {} {}}
-- {0 {} {}}
dr {0 {} {}}
drdr {0 {} {}}
{dr dr} {0 {} {}}
{dr dre} {0 {} {}}
{dr dre e} {0 {} {}}
{dr dre dre} {0 {} {}}
{dr dr e} {0 {} {}}
{dr drdrdr e} {0 {} {}}
{dr drCRdr} {0 {} {}}
{dr dr dr dr} {0 {} {}}
{dr dog} {1 DR dog}
{- dog} {1 - dog}
-dr {0 {} {}}
{dr - dr - dr e} {0 {} {}}
{ - dr e} {0 {} {}}
dog {1 {} dog}
}

proc genTestResults {} {
    foreach {l} $::testset {
	set res [a $l]
	puts stdout $res
    }
}

proc runtest {} {
    puts -nonewline stdout "testing resultset......"
    flush stdout
    set failed 0
    foreach {l r} $::resultset {
        set res [lindex [a $l] 1]
	if {$res==$r} {
	    # puts stdout "OK \"$l\""
	} else {
	puts stdout "FAIL \"$l\" = $res expected $r"
	set failed 1
	}
    }
    if {!$failed} {puts stdout passed} else {exit $failed}
}

proc runtest2 {} {
    set failed 0
    puts -nonewline stdout "testset chaining......."
    flush stdout
    foreach {l} $::testset {
	incr failed [chainingCheck $l]
    }
    if {!$failed} {puts stdout passed} else {exit $failed}
}

proc chainingCheck {l} {
    set failed 0
    set res [lindex [a $l] 1]
    foreach {ret prefix var} $res {}
    set res2 [lindex [a [concat $prefix $var]] 1]
    foreach {ret2 prefix2 var2} $res2 {}
    foreach {v1 v2} {ret ret2 prefix prefix2 var var2} {
	if {[set $v1] != [set $v2]} { incr failed 1; puts stdout "FAIL chaining for \"$l\" \"[set $v1]\" != \"[set $v2]\""}
    }
    return $failed
}

proc runtest3 {} {
    set failed 0
    puts -nonewline stdout "random test chaining...."
    flush stdout
    set tokens {dr CR variable drvar - -- " " = "!@#~$%^&*()_+{}[]:'\"\\,<.>?/"}
    set i 0
    set j 0
    while {1} {
	incr j
	set l {}
	foreach {ii} {1 2 3 4 5 6 7 8 9 10} {
	   set l [concat $l " " [lindex $tokens [expr int(rand()*9)]]]
	}
	incr failed [chainingCheck $l]
	if  {![expr $j%100]} {
	    incr i
	    puts -nonewline stdout [lindex {"\b|" "\b\\" "\b-" "\b/"} [expr $i%4]]
	    flush stdout
	    }
        if {$j>100000} {puts -nonewline stdout "\b"; break}
    }
    if {!$failed} {puts stdout passed} else {exit $failed}
}

#genTestResults
runtest
runtest2
runtest3

#:! tclsh %

