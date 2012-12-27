#!/bin/sh
# the next line restarts using minsky \
	exec minsky "$0" ${1+"$@"}

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

# disable tear-off menus
option add *Menu.tearOff 0


set minskyHome [file dirname [info nameofexecutable]]
set fname ""
set workDir [pwd]

# default canvas size. Overridden by previously resized window size
# saved in .minskyrc
set canvasWidth 600
set canvasHeight 800

# read in .rc file, which differs on Windows and unix
set rcfile ""
if {$tcl_platform(platform)=="unix"} {
  set rcfile "$env(HOME)/.minskyrc"
} elseif {$tcl_platform(platform)=="windows"} {
  set rcfile "$env(USERPROFILE)/.minskyrc"
}
if [file exists $rcfile] {
  source $rcfile
}


if {![file exists [file join $tcl_library init.tcl]]
} {
    set tcl_library $minskyHome/library/tcl8.5
}


# hopefully, we can now rely on TCL/Tk to find its own libraries.
if {![info exists tk_library]} {
  regsub tcl [file tail [info library]] tk tk_library
  set tk_library [file join [file dirname [info library]] $tk_library]
}
if {![file exists [file join $tk_library tk.tcl]]
} {
    set tk_library $minskyHome/library/tk8.5
}

proc setFname {name} {
    global fname workDir
    if [string length $name] {
        set fname $name
        set workDir [file dirname $name]
        catch {wm title . "Minsky (prototype): $fname"}
    }
}

#if argv(1) has .tcl extension, it is a script, otherwise it is data
if {$argc>1} {
#    tk_dialog .dialog "" "arg=$argv(1)" "" 0 OK
    if [string match "*.tcl" $argv(1)] {
	    source $argv(1)
	} else {
	    minsky.load $argv(1)
            setFname $argv(1)
	}
}

source $tcl_library/init.tcl
source [file join $tk_library tk.tcl]
source [file join $tk_library bgerror.tcl]
source $minskyHome/library/init.tcl

GUI
wm deiconify .
tk appname [file rootname [file tail $argv(0)]]
wm title . "Minsky (prototype): $fname" 

if {[string equal unix $tcl_platform(platform)]} {
    if {[catch {load $minskyHome/library/libTktable2.11[info sharedlibextension]}]} {
        load libTktable2.11[info sharedlibextension]
    }
} else {
    load Tktable211.dll
}

source $minskyHome/library/tooltip.tcl
namespace import tooltip::tooltip

source $minskyHome/library/Xecolab/obj-browser.tcl

# Macs have a weird numbering of mouse buttons, so lets virtualise B2 & B3
# see http://wiki.tcl.tk/14728
if {[tk windowingsystem] == "aqua"} {
    event add <<contextMenu>> <Button-2> <Control-Button-1>
    event add <<middleMouse>> <Button-3>
    event add <<middleMouse-Motion>> <B3-Motion>
    event add <<middleMouse-ButtonRelease>> <B3-ButtonRelease>
} else {
    event add <<contextMenu>> <Button-3>
    event add <<middleMouse>> <Button-2>
    event add <<middleMouse-Motion>> <B2-Motion>
    event add <<middleMouse-ButtonRelease>> <B2-ButtonRelease>
}

use_namespace minsky

proc deiconify {widget} {
    wm deiconify $widget
    after idle "wm deiconify $widget; raise $widget; focus -force $widget"
}

labelframe .menubar -relief raised
menubutton .menubar.file -menu .menubar.file.menu -text File  -underline 0
menu .menubar.file.menu

button .menubar.rkData -text "Runge Kutta" -relief flat -command {
    foreach {var text} $rkVars { set rkVarInput($var) [$var] }
    deiconify .rkDataForm
    update idletasks
    ::tk::TabToWindow $rkVarInput(initial_focus)
    tkwait visibility .rkDataForm
    grab set .rkDataForm
    wm transient .rkDataForm .
} -underline 0 
tooltip .menubar.rkData "Set Runge Kutta parameters"

menubutton .menubar.ops -menu .menubar.ops.menu -text Operations -underline 0
menu .menubar.ops.menu

label .menubar.statusbar -text "t: 0 dt: 0"

# classic mode
set classicMode 0

if {$classicMode} {
        button .menubar.run -text run -command runstop
        button .menubar.reset -text reset -command reset
        button .menubar.step -text step -command step
} else {
        image create photo runButton -file "$minskyHome/icons/Play.gif" 
        image create photo stopButton -file "$minskyHome/icons/Pause.gif"
        image create photo resetButton -file "$minskyHome/icons/Rewind.gif"
        image create photo stepButton -file "$minskyHome/icons/Last.gif"
        # iconic mode
        button .menubar.run -image runButton -height 25 -width 25 -command runstop
        button .menubar.reset -image resetButton -height 25 -width 25 -command reset
        button .menubar.step -image stepButton -height 25 -width 25  -command step
        tooltip .menubar.run "Run/Stop"
        tooltip .menubar.reset "Reset simulation"
        tooltip .menubar.step "Step simulation"
}

menubutton .menubar.options -menu .menubar.options.menu -text Options -underline 0
menu .menubar.options.menu
.menubar.options.menu add command -label "Preferences" -command {
    foreach var [array names preferences_input] {
	set preferences_input($var) $preferences($var)
    }
    deiconify .preferencesForm
    update idletasks
    ::tk::TabToWindow $preferences(initial_focus)
    tkwait visibility .preferencesForm
    grab set .preferencesForm
    wm transient .preferencesForm .
}


# placement of menu items in menubar
pack .menubar.file -side left
pack .menubar.ops -side left
pack .menubar.options -side left
pack .menubar.rkData -side left

pack .menubar.run .menubar.reset .menubar.step -side left
pack .menubar.statusbar -side right -fill x

grid .menubar -row 0 -column 0 -columnspan 1000 -sticky ew

# File menu
.menubar.file.menu add command -label "About Minsky" -command aboutMinsky
.menubar.file.menu add command -label "New System" -command newSystem  -underline 0 -accelerator ^N
.menubar.file.menu add command -label "Open" -command openFile -underline 0 -accelerator ^O
.menubar.file.menu add command -label "Save" -command save -underline 0 -accelerator ^S
.menubar.file.menu add command -label "SaveAs" -command saveAs 
.menubar.file.menu add command -label "Output LaTeX" -command {
    latex [tk_getSaveFile -defaultextension .tex -initialdir $workDir]
}
    
.menubar.file.menu add command -label "Quit" -command finishUp -underline 0 -accelerator ^Q
.menubar.file.menu add separator
.menubar.file.menu add command  -foreground #5f5f5f -label "Debugging Use"
.menubar.file.menu add command -label "Redraw" -command updateCanvas
.menubar.file.menu add checkbutton -label "Show Ports" -variable showPorts -command updateCanvas -onvalue 1 -offvalue 0 
.menubar.file.menu add command -label "Object Browser" -command obj_browser
.menubar.file.menu add command -label "Command" -command cli

bind . <Destroy> finishUp
# keyboard accelerators
bind . <Control-s> save
bind . <Control-o> openFile
bind . <Control-n> newSystem
bind . <Control-q> finishUp
bind . <Alt-r> {.menubar.rkData invoke}

source $minskyHome/godley.tcl
source $minskyHome/wiring.tcl
source $minskyHome/plots.tcl
source $minskyHome/group.tcl

set delay 0
set running 0

proc runstop {} {
  global running classicMode
  if {$running} {
    set running 0
    if {$classicMode} {
            .menubar.run configure -text run
        } else {
            .menubar.run configure -image runButton
        }
  } else {
    set running 1
    if {$classicMode} {
            .menubar.run configure -text stop
        } else {
             .menubar.run configure -image stopButton
        }
    simulate
 }
}

proc step {} {
    set lastt [t]
    minsky.step
    .menubar.statusbar configure -text "t: [t] dt: [expr [t]-$lastt]"
    updateGodleysDisplay
}


proc simulate {} {
    uplevel #0 {
      if {$running} {
          after $delay {step; simulate; update}
        }
    }
}

proc reset {} {
    global running 
    set running 0
    set tstep 0
    minsky.reset
    .menubar.statusbar configure -text "t: 0 dt: 0"
    .menubar.run configure -image runButton

    global oplist lastOp
    set oplist [opOrder]
    updateCanvas
    updateGodleysDisplay
    set lastOp -1
}

# load/save 

proc openFile {} {
    global fname workDir preferences
    set ofname [tk_getOpenFile -multiple 1 -filetypes {
	    {Minsky {.mky}} {XML {.xml}} {All {.*}}} -initialdir $workDir]
    if [string length $ofname] {
        newSystem
        setFname $ofname

        #not sure why this is needed
#        .wiring.canvas delete variables operations wires handles plots
        minsky.load $fname
        updateCanvas
	foreach g [godleyItems.#keys] {
	    godleyItem.get $g
	    set preferences(godleyDE) [godleyItem.table.doubleEntryCompliant]
	}
    }
}

proc save {} {
    global fname
    if {![string length $fname]} {
	    setFname [tk_getSaveFile -defaultextension .mky]}            
    if [string length $fname] {
        minsky.save $fname
    }
}

proc saveAs {} {
    global fname workDir
    setFname [tk_getSaveFile -defaultextension .mky -initialdir $workDir]
    if [string length $fname] {
        minsky.save $fname
    }
}

proc newSystem {} {
    deleteSubsidiaryTopLevels
    clearAll
    updateCanvas 
    global fname
    set fname ""
    wm title . "Minsky (prototype): New System"
}

# for debugging purposes
#button .nextOp -text "Next Operation" -command {
#    global oplist lastOp
#
#    if {[llength $oplist]>0} {
#        if {$lastOp>=0} {.wiring.canvas itemconfigure $lastOp -outline black}
#        set lastOp [.wiring.canvas create rectangle \
#                    [.wiring.canvas bbox op[lindex $oplist 0]] -outline red
#                   ]
#        set oplist [lrange $oplist 1 end]
#        }
#}
#pack append .buttonbar .nextOp left


toplevel .rkDataForm
wm resizable .rkDataForm 0 0

set rkVars {
          stepMin   "Min Step Size"
          stepMax   "Max Step Size"
          nSteps     "no. steps per iteration"
          epsAbs     "Absolute error"
          epsRel     "Relative error"
}

set row 0

grid [label .rkDataForm.label$row -text "Runge-Kutta parameters"] -column 1 -columnspan 999 -pady 10
incr row 10

foreach {var text} $rkVars {
    set rowdict($text) $row
    grid [label .rkDataForm.label$row -text $text] -column 10 -row $row -sticky e
    grid [entry  .rkDataForm.text$row -width 20 -textvariable rkVarInput($var)] -column 20 -row $row -sticky ew
    incr row 10
}
set rkVarInput(initial_focus) ".rkDataForm.text$rowdict(Min Step Size)"
frame .rkDataForm.buttonBar
button .rkDataForm.buttonBar.ok -text OK -command {setRKparms; closeRKDataForm}
button .rkDataForm.buttonBar.cancel -text cancel -command {closeRKDataForm}
pack .rkDataForm.buttonBar.ok [label .rkDataForm.buttonBar.spacer -width 2] .rkDataForm.buttonBar.cancel -side left -pady 10
grid .rkDataForm.buttonBar -column 1 -row 999 -columnspan 999

# invokes OK or cancel button with given window, depending on current focus
proc invokeOKorCancel {window} {
    if [string equal [focus] "$window.cancel"] {
            $window.cancel invoke
        } else {
            $window.ok invoke
        }
}

bind .rkDataForm <Key-Return> {invokeOKorCancel .rkDataForm.buttonBar}

wm title .rkDataForm "Runge-Kutta parameters"
wm withdraw .rkDataForm 

proc closeRKDataForm {} {
    grab release .rkDataForm
    wm withdraw .rkDataForm
}

proc setRKparms {} {
    global rkVars rkVarInput
    foreach {var text} $rkVars { $var $rkVarInput($var) }
}

toplevel .preferencesForm
wm resizable .preferencesForm 0 0

set row 0

grid [label .preferencesForm.label$row -text "Preferences"] -column 1 -columnspan 999 -pady 10
incr row 10

# pad the left and right
grid [frame .preferencesForm.f1] -column 1 -row 1 -rowspan 999 -padx 10
grid [frame .preferencesForm.f2] -column 999 -row 1 -rowspan 999 -padx 10


# add rows to this table to add new preferences

# valid types are "text", "bool" and "{ enum label1 val1 label2 val2 ... }"

#   varName              Label                    DefaultVal    type

set preferencesVars {

    godleyDE             "Godley Table Double Entry"     1      bool

    godleyDisplay        "Godley Table Show Values"      1      bool

godleyDisplayStyle       "Godley Table Output Style"    sign  { enum
                                                               "DR/CR" DRCR
							       "+/-" sign } 
}

foreach {var text default type} $preferencesVars {

    set preferences($var) $default

    set rowdict($text) $row

    grid [label .preferencesForm.label$row -text $text] -column 10 -row $row -sticky e -pady 5

    switch $type {
    	text {
	    grid [entry  .preferencesForm.text$row -width 20 -textvariable preferences_input($var)] -column 20 -row $row -sticky ew -columnspan 999
	}
	bool {
	    grid [checkbutton .preferencesForm.cb$row -variable preferences_input($var)] -row $row -column 20 -sticky w
	}
	default {
	    if {[llength $type] > 1} {
	       switch [lindex $type 0] {
		    enum {
			set column 20
			foreach {valtext val} [lrange $type 1 end] {
			    grid [radiobutton .preferencesForm.rb${row}v$column  -text $valtext -variable preferences_input($var) -value $val] -row $row -column $column
			    incr column 
			}
		    }
	       }
	    } else { error "unknown preferences widget $type" }
	}
    }

    incr row 10
}

set preferences(initial_focus) ".preferencesForm.cb$rowdict(Godley Table Double Entry)"

frame .preferencesForm.buttonBar
button .preferencesForm.buttonBar.ok -text OK -command {setPreferenceParms; closePreferencesForm;updateGodleysDisplay}
button .preferencesForm.buttonBar.cancel -text cancel -command {closePreferencesForm}
pack .preferencesForm.buttonBar.ok [label .preferencesForm.buttonBar.spacer -width 2] .preferencesForm.buttonBar.cancel -side left -pady 10
grid .preferencesForm.buttonBar -column 1 -row 999 -columnspan 999

bind .preferencesForm <Key-Return> {invokeOKorCancel .preferencesForm.buttonBar}

wm title .preferencesForm "Preferences"
wm withdraw .preferencesForm 

proc closePreferencesForm {} {
    grab release .preferencesForm
    wm withdraw .preferencesForm
}

proc setPreferenceParms {} {
    global preferencesVars preferences preferences_input

    foreach var [array names preferences_input] {
	set preferences($var) $preferences_input($var)
    }
}

toplevel .aboutMinsky
wm resizable .aboutMinsky 0 0
label .aboutMinsky.text

button .aboutMinsky.ok -text OK -command {
    grab release .aboutMinsky
    wm withdraw .aboutMinsky
 }

wm withdraw .aboutMinsky
pack .aboutMinsky.text .aboutMinsky.ok

proc aboutMinsky {} {
    .aboutMinsky.text configure -text "
   Minsky [minskyVersion]\n
   EcoLab [ecolabVersion]\n
   Tcl/Tk [info tclversion]

   Minsky is FREE software, distributed under the 
   GNU General Public License. It comes with ABSOLUTELY NO WARRANTY. 
   See http://www.gnu.org/licenses/ for details
   " 
    deiconify .aboutMinsky 

    # wierd trick to resize toplevel windows:
    update idletasks
    wm geometry .aboutMinsky

    tkwait visibility .aboutMinsky
    grab set .aboutMinsky
    wm transient .aboutMinsky .
}

# delete subsidiary toplevel such as godleys and plots
proc deleteSubsidiaryTopLevels {} {
    global globals

    set globals(default_rotation) 0
    set globals(godley_tables) {}

    foreach w [info commands .godley*] {destroy $w}
    foreach w [info commands .plot*] {destroy $w}
    .wiring.canvas delete all
    foreach image [image names] {
        if [regexp ".plot.*|godleyImage.*|groupImage.*|varImage.*|opImage.*|plot_image.*" $image] {
                image delete $image
            }
    }
}

# a hook to allow code to be run after Minsky has initialised itself
if {[llength [info commands afterMinskyStarted]]>0} {
    afterMinskyStarted
}

proc finishUp {} {
    # check if the model has been saved yet
    if [edited] {
        switch [tk_messageBox -message "Save before exiting?" -type yesnocancel] {
            yes save
            no {}
            cancel return
        }
    }

    # if we have a valid rc file location, write out the directory of
    # the last file loaded
    global rcfile workDir 
    if {$rcfile!=""} {
        set rc [open $rcfile w]
        puts $rc "set workDir \"$workDir\""
        puts $rc "set canvasWidth [winfo width .wiring.canvas]"
        puts $rc "set canvasHeight [winfo height .wiring.canvas]"
        close $rc
    }
    # why is this needed?
    proc bgerror x {} 
    exit
}

proc setFname {name} {
    global fname workDir
    if [string length $name] {
        set fname $name
        set workDir [file dirname $name]
        catch {wm title . "Minsky (prototype): $fname"}
    }
}

if {$argc>1 && ![string match "*.tcl" $argv(1)]} {
# we have loaded a Minsky model, so must refresh the canvas
    updateCanvas
}

