#  @copyright Steve Keen 2012
#  @author Russell Standish
#  This file is part of Minsky.
#
#  Minsky is free software: you can redistribute it and/or modify it
#  under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  Minsky is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with Minsky.  If not, see <http://www.gnu.org/licenses/>.
#


# Godley table

set globals(godley_tables) {}
		    
set fp [open "accountingRules" r]
set accountingRules [read $fp]
close $fp

# set up accounting rules (and their inverse)
foreach {account_type line rule1DRCR rule1sign rule1color rule2DRCR rule2sign rule2color} $accountingRules {
    array set $account_type [list $rule1sign $rule1DRCR $rule2sign $rule2DRCR $rule1DRCR $rule1sign $rule2DRCR $rule2sign]
    array set color${account_type} [list $rule1sign $rule1color $rule2sign $rule2color $rule1DRCR $rule1color $rule2DRCR $rule2color]
}

proc accountingRules {accountType prefix} {
    if {$accountType == "noAssetClass"} { set accountType "SingleEntry" }
    upvar #0 $accountType account_type
    return $account_type($prefix)
}

proc accountingColor {accountType prefix} {
    if {$accountType == "noAssetClass"} { set accountType "SingleEntry" }
    upvar #0 color$accountType account_type
    return $account_type($prefix)
}

proc createGodleyWindow {id} {
    global globals preferences
    toplevel .godley$id 
    wm title .godley$id "Godley Table"
    wm withdraw .godley$id

    lappend globals(godley_tables) $id
    set globals(updateGodleyLaunched$id) 0

    set t .godley$id.table
    table $t \
        -rows 1 \
        -cols 1 \
        -colwidth 20 \
        -titlerows 0 \
        -titlecols 0 \
        -yscrollcommand ".godley$id.sy set" \
        -xscrollcommand ".godley$id.sx set" \
        -coltagcommand colorize \
        -flashmode off \
        -selectmode extended \
        -ellipsis on \
        -width 20 -height 20 \
        -colstretch all \
        -rowstretch all \
        -multiline 0

    bind $t <Return> {%W activate {}}

    #make column 0 narrower
    $t width 0 15

    # make column 1 wider
    $t width 1 30

    $t configure -usecommand 1 -command "setGetCell $id %r %c %i %s %W" 

    $t tag configure black -foreground black
    $t tag configure red -foreground red

    # required to make this behave on Windows.            
    $t tag configure active -foreground black    

    entry .godley$id.topbar -justify center -background CadetBlue2
    bind .godley$id.topbar <Leave> "updateGodleyTitle $id"
    bind .godley$id.topbar <Key-Return> "updateGodleyTitle $id"

    scrollbar .godley$id.sy -command [list $t yview]
    scrollbar .godley$id.sx -command [list $t xview] -orient horizontal

    updateDEmode
    checkbutton .godley$id.doubleEntryMode -text "Double Entry" -variable preferences(godleyDE)

    # default checkbox mode is deselected - select if model variable is set
    #if [godleyItem.table.doubleEntryCompliant] {
    #    .godley$id.doubleEntryMode select
    #}
    pack .godley$id.topbar -fill x
    pack .godley$id.sy -side right -fill y
    pack .godley$id.table -fill both 
    pack .godley$id.sx -fill x
}


trace add variable preferences(godleyDE) write {updateDEmode}

proc updateDEmode args {
  global globals preferences
  foreach id $globals(godley_tables) {
    godleyItem.get $id
    godleyItem.table.setDEmode $preferences(godleyDE)
    godleyItem.set
    updateGodley $id
  }
}
  
proc parse_input {input p v} {
    upvar $p prefix
    upvar $v varName

    # regexp accepts input of the form "?DR|CR|-? VarName"
    # where VarName cannot begin with DR or CR
    # eg   "var"   "-var"    "dr var"   "DR var"

    set retval [regexp {^\s*(([cCdD][rR])?|\s*(-)?)(?:-)*\s*(?![cCdD][rR])\m([[:alnum:]]+)} $input matchstr prefix drcr sign varName]

    # attempt to re-parse the output and make sure it's unchanged
    if {$retval} {
	set retval2 [regexp {^\s*(([cCdD][rR])?|\s*(-)?)(?:-)*\s*(?![cCdD][rR])\m([[:alnum:]]+)} "$prefix $varName" matchstr2 prefix2 drcr2 sign2 varName2]
	if {!$retval2 || $prefix!=$prefix2 || $varName!=$varName2} {
	    return 0;
	}
        
    }

    if {$retval} { set prefix [string toupper $prefix] }

    return $retval
}

proc updateGodleyTitle {id} {
    godleyItem.get $id
    if {[godleyItem.table.title]!=[.godley$id.topbar get]} {
        godleyItem.table.title [.godley$id.topbar get]
        godleyItem.set
        updateGodley $id
    }
}

proc setGetCell {id r c i s w} {
    global preferences

    if {$r>0 && $c>0} {
        minsky.godleyItem.get $id
        set row [expr $r-1]
        set col [expr $c-1]
	set doubleEntryMode [minsky.godleyItem.table.doubleEntryCompliant]
        if $doubleEntryMode {
            # allow for asset class row
            incr row -1
            if {$row==-1} return "";
            # TODO: compute row sum here
            if {$col==[minsky.godleyItem.table.cols]} {
                if {$row>0} {
                    return [minsky.godleyItem.table.rowSum $row]
                } else {
                    # don't sum column labels
                    return "Row Sum"
                }
            }
        }
        if {$i && $id>=0} {
	    set varName $s
	    if {$row>0 && $col>0} {
		if {$doubleEntryMode} {
		    set account_type [godleyItem.table.assetClass $col]
		    if {$account_type == "noAssetClass"} return
		} else {
		    set account_type "SingleEntry"
		}

	# parse_input accepts input of the form "?DR|CR|-? VarName"
	# where VarName cannot begin with DR or CR
	# eg   "var"   "-var"    "dr var"   "DR var"

		if {![parse_input $s prefix varName]} {return}

		switch $prefix {
		   "" {}
		   CR - DR {
		       if {"-" == [accountingRules $account_type $prefix]} {
			    set varName "-$varName"
		       }
		   }
		   - {
			    set varName "-$varName"
		   }
		   default { error "invalid prefix $prefix" }
		}
	    }
            minsky.godleyItem.table.setCell $row $col $varName
            minsky.godleyItem.set $id
            updateGodley $id
        } else {
            set s [minsky.godleyItem.table.getCell $row $col]
	    if {$row>0 && $col>0} {
		if $doubleEntryMode {
		    set account_type [godleyItem.table.assetClass $col]
		    if {$account_type == "noAssetClass"} {
			return "Asset Class Not Set"
		    }
		} else {
		    set account_type "SingleEntry"
		}
		if [string length $s] {
			set account_type [godleyItem.table.assetClass $col]
			set show $s

			# use parse_input to format output for consistency
			set key $s
			set prefix ""
			parse_input $s prefix key

			if {$prefix == "-"} {
			   set sign -
			} else {
			   set sign +
			}
			$w tag cell [accountingColor $account_type $sign] "$r,$c"
			# if $key is a number, just pass it on
			# otherwise apply accounting format
			if {![string is double $key]} {
			    set val ""
			    switch $preferences(godleyDisplayStyle) {
				"DRCR" {
				    if {[t]>0 && $preferences(godleyDisplay)} {
				       set val ""
				       catch {
					   value.get $key
					   set val [value.value]
				       }
				       set val " = $val"
				    }
				    set DRCR [accountingRules $account_type $sign]
				    set show "$DRCR $key$val"
				}
				"sign" {
				    if {[t]>0 && $preferences(godleyDisplay)} {
				       set val ""
				       catch {
					   value.get $key
					   set val [value.value]
				       }
					switch $sign {
					    - { if {[catch {
						    set val "= [expr -($val)]"
						}]} {
						    set val "= $val"
						}
					       }
					    default {set val "= $val"}
					}
				    }
				    set show "$s $val"
				}
				default { error "unknown display style $preferences(godleyDisplayStyle)"}
			    }
			}
			return $show
		     } else {
			return " "
		     }
		} else {
		    return $s
		}
        }
    }
}

proc openGodley {id} {
    if {![winfo exists .godley$id]} {createGodleyWindow $id}
    deiconify .godley$id
    raise .godley$id .
    updateGodley $id
}

proc addRow {id r} {
    godleyItem.get $id
    godleyItem.table.insertRow $r
    godleyItem.set $id
    # if we don't remove activation, sometimes the cell content is not correctly updated
    .godley$id.table activate 0,0
    updateGodley $id
}

proc delRow {id r} {
    godleyItem.get $id
    godleyItem.table.deleteRow $r
    godleyItem.set $id
    .godley$id.table activate 0,0
    updateGodley $id
}

proc addCol {id c} {
    godleyItem.get $id
    godleyItem.table.insertCol $c
    godleyItem.set $id
    .godley$id.table activate 0,0
    updateGodley $id
}

proc delCol {id c} {
    godleyItem.get $id
    godleyItem.table.deleteCol $c
    godleyItem.set $id
    .godley$id.table activate 0,0
    updateGodley $id
}
    
proc updateGodleys {} {
  global globals
  foreach id $globals(godley_tables) {
    updateGodley $id
  }
  
}

proc updateGodleysDisplay {} {
  global globals
  foreach id $globals(godley_tables) {
    updateGodleyDisplay $id
  }
  
}

# sets a when-idle job to update the godley table, to prevent the table being updated too often during rapid fire requests
proc updateGodley {id} {
    global globals
    if {$id < 0 || ![winfo exists .godley$id]} {return}
    if {!$globals(updateGodleyLaunched$id)} {
        set $globals(updateGodleyLaunched$id) 1
        after idle whenIdleUpdateGodley $id
    }
}

proc whenIdleUpdateGodley {id} {
    godleyItem.get $id
    set nrows [expr [godleyItem.table.rows]+1]
    set ncols [expr [godleyItem.table.cols]+1]
    if [godleyItem.table.doubleEntryCompliant] {
        incr nrows
        incr ncols
    }

    # remove any selection - ticket 88
    .godley$id.table selection clear all
    .godley$id.table activate -1,-1

    wm title .godley$id "Godley Table: [godleyItem.table.title]"
    .godley$id.topbar delete 0 end
    if {[godleyItem.table.title]==""} {
        .godley$id.topbar insert 0 "Godley$id"
    } else {
        .godley$id.topbar insert 0 [godleyItem.table.title]
    }
    .godley$id.table configure -rows $nrows -cols $ncols

    # delete row/col buttons
    foreach c [info commands .godley$id.???Buttons{*}] {destroy $c}
    # delete asset class dropdowns
    foreach c [info commands .godley$id.assetClass{*}] {destroy $c}

    # put the double entry book keeping mode button in top left corner
    .godley$id.table window configure 0,0 -window .godley$id.doubleEntryMode

    for {set r 1} {$r<[expr [godleyItem.table.rows]+1]} {incr r} {
        set ro [expr $r+[godleyItem.table.doubleEntryCompliant]]
        frame .godley$id.rowButtons{$r}
        button .godley$id.rowButtons{$r}.add -foreground green -text "+" -command "addRow $id $r"
        pack .godley$id.rowButtons{$r}.add -side left
        if {$r>1} {
            button .godley$id.rowButtons{$r}.del -foreground red -text "-" -command "delRow $id $r"
            pack .godley$id.rowButtons{$r}.del -side left
        }
        .godley$id.table window configure $ro,0 -window .godley$id.rowButtons{$r}
    }
    for {set c 1} {$c<[expr [godleyItem.table.cols]+1]} {incr c} {
        frame .godley$id.colButtons{$c}
        button .godley$id.colButtons{$c}.add -foreground green -text "+" -command "addCol $id $c"
        pack .godley$id.colButtons{$c}.add  -side left
        if {$c>1} {
            button .godley$id.colButtons{$c}.del -foreground red -text "-" -command "delCol $id $c"
            pack .godley$id.colButtons{$c}.del -side left
        }

        .godley$id.table window configure 0,$c -window .godley$id.colButtons{$c}

        if {$c>1 && [godleyItem.table.doubleEntryCompliant]} {
            # C++ table column offset by one wrt TkTable columns
            set col [expr $c-1]
            menubutton .godley$id.assetClass{$c} -menu .godley$id.assetClass{$c}.menu\
                -text [godleyItem.table.assetClass $col] -relief raised
            menu .godley$id.assetClass{$c}.menu
            foreach assetClass [assetClasses] {
                .godley$id.assetClass{$c}.menu add command -label $assetClass \
                    -command "
                      godleyItem.get $id
                      godleyItem.table.assetClass $col $assetClass
                      godleyItem.set
                      updateGodley $id
                    "
            }
           .godley$id.table window configure 1,$c -window .godley$id.assetClass{$c}
        }
    }

    godleyItem.update
    godleyItem.set
    updateGodleyItem $id
    global updateGodleyLaunched
    set updateGodleyLaunched 0
    update
}


proc updateGodleyDisplay {id} {
    global globals
    if {$id < 0 || ![winfo exists .godley$id]} {return}
    if {!$globals(updateGodleyLaunched$id)} {
        set $globals(updateGodleyLaunched$id) 1
        after idle whenIdleUpdateGodleyDisplay $id
    }
}

proc whenIdleUpdateGodleyDisplay {id} {
    .godley$id.table clear cache
}

