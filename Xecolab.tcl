# hopefully, we can now rely on TCL/Tk to find its own libraries.
if {![info exists tk_library]} {
  regsub tcl [file tail [info library]] tk tk_library
  set tk_library [file join [file dirname [info library]] $tk_library]
}
if {![file exists [file join $tk_library tk.tcl]]
} {
    set tk_library library/tk8.5
}
source [file join $tk_library tk.tcl]
source [file join $tk_library bgerror.tcl]
wm deiconify .
tk appname [file rootname [file tail $argv(0)]]
wm title . [file rootname [file tail $argv(0)]]

# Top level button bar
frame .buttonbar 

# returns control to main(), which then exits cleanly. 
# exit calls exit(0), which does not clean up properly
button .quit -text quit -command {set running 0; exit_ecolab}

button .run -text run -command {
    .run configure -relief sunken
    .stop configure -relief raised
    set running 1
    simulate}

button .stop -text stop -command {
    global running
    set running 0
    .run configure -relief raised
    .stop configure -relief sunken
}

button .reset -text reset -command {reset}

button .command -text Command -command cli
button .obj_browsw -text "Object Browser" -command obj_browser

button .user1 -text User1 

button .user2 -text User2

#button .user3 -text User3

pack append  .buttonbar .quit  left  .run  left  .stop left .reset left
pack append  .buttonbar .command left .obj_browsw left .user1 left .user2 left  
#.user3 left

pack append . .buttonbar top

proc mem_avail {} {return "[lindex [exec vmstat] 24]KB"}

# Status bar
label .statusbar -text "Not Started Yet"
pack append . .statusbar top

#source $ecolab_library/Xecolab/filebrowser.tcl
source $ecolab_library/Xecolab/obj-browser.tcl
source $ecolab_library/Xecolab/netgraph.tcl
source $ecolab_library/Xecolab/connect.tcl
if [info exists blt_library] {
    source $ecolab_library/Xecolab/display.tcl
    source $ecolab_library/Xecolab/plot.tcl
    source $ecolab_library/Xecolab/histogram.tcl
} else {
    puts stdout "plot, display and histogram not implemented"
}

# memory exhausted dialog box
label .mem_exhausted -text "Memory is Exhausted" -height 5 -relief raised
button .mem_exhausted.ok -text OK -command "place forget .mem_exhausted"
place .mem_exhausted.ok -relx .5 -rely .6 
    


proc assert {ex} {
    uplevel 1 {
        if {![expr $ex]} {
            puts stdout "$x failed"
        }
    }
}
