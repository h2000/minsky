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

set globals(default_rotation) 0

# Wiring canvas


proc createWire {coords} {
    .wiring.canvas create line $coords -tag wires -arrow last -smooth bezier
}

#toplevel .wiring 
frame .wiring 
grid .wiring -column 0 -row 10 -sticky news
grid columnconfigure . 0 -weight 1
grid rowconfigure . 10 -weight 1

#menu .wiring.menubar -type menubar
frame .wiring.menubar 

#.wiring.menubar add cascade -menu .wiring.menubar.ops -label Operations
#.wiring configure -menu .wiring.menubar

# move mode initially
set interactionMode 2 
radiobutton .wiring.menubar.wiringmode -value 1 -variable interactionMode -command setInteractionMode -text wire
radiobutton .wiring.menubar.movemode -value 2 -variable interactionMode -command setInteractionMode -text move
radiobutton .wiring.menubar.panmode -value 3 -variable interactionMode -command setInteractionMode -text pan
radiobutton .wiring.menubar.lassomode -value 4 -variable interactionMode -command setInteractionMode -text lasso

set menubarLine 0
frame .wiring.menubar.line0

image create photo zoomOutImg -file $minskyHome/icons/zoomOut.gif
button .wiring.menubar.line0.zoomOut -image zoomOutImg -height 24 -width 37 \
    -command {zoom 0.91}
tooltip .wiring.menubar.line0.zoomOut "Zoom Out"

image create photo zoomInImg -file $minskyHome/icons/zoomIn.gif
button .wiring.menubar.line0.zoomIn -image zoomInImg -height 24 -width 37 \
    -command {zoom 1.1}
tooltip .wiring.menubar.line0.zoomIn "Zoom In"

image create photo zoomOrigImg -file $minskyHome/icons/zoomOrig.gif
button .wiring.menubar.line0.zoomOrig -image zoomOrigImg -height 24 -width 37 \
    -command {zoom [expr 1/[zoomFactor]]; recentreCanvas}
tooltip .wiring.menubar.line0.zoomOrig "Reset Zoom"

image create photo godleyImg -file $minskyHome/icons/bank.gif
button .wiring.menubar.line0.godley -image godleyImg -height 24 -width 37 \
    -command {addNewGodleyItem [addGodleyTable 10 10]}
tooltip .wiring.menubar.line0.godley "Godley table"

image create photo varImg -file $minskyHome/icons/var.gif
button .wiring.menubar.line0.var -image varImg -height 24 -width 37 \
    -command addVariable
tooltip .wiring.menubar.line0.var "variable"

image create photo constImg -file $minskyHome/icons/const.gif
button .wiring.menubar.line0.const -height 24 -width 37 -image constImg -command {
    addOperation constant}
tooltip .wiring.menubar.line0.const "constant"

image create photo integrateImg -file $minskyHome/icons/integrate.gif
button .wiring.menubar.line0.integrate -image integrateImg -command {
    addOperation integrate}
tooltip .wiring.menubar.line0.integrate integrate

pack .wiring.menubar.movemode .wiring.menubar.wiringmode .wiring.menubar.lassomode .wiring.menubar.panmode -side left

pack .wiring.menubar.line0.zoomOut .wiring.menubar.line0.zoomIn .wiring.menubar.line0.zoomOrig .wiring.menubar.line0.godley .wiring.menubar.line0.var .wiring.menubar.line0.const .wiring.menubar.line0.integrate -side left

# create buttons for all available operations (aside from those
# handled especially)
foreach op [availableOperations] {
    if {$op=="numOps"} break
    # ignore some operations
    switch $op {
        "constant" -
        "copy" -
        "integrate"  continue 
    }
    # advance to next line in menubar
    if {$op=="time"} {
        incr menubarLine
        frame .wiring.menubar.line$menubarLine
    }
    if {[tk windowingsystem]=="aqua"} {
        # ticket #187
        image create photo [set op]Img -file $minskyHome/icons/$op.gif
    } else {
        image create photo [set op]Img -width 24 -height 24
        operationIcon [set op]Img $op
    }
    button .wiring.menubar.line$menubarLine.$op -image [set op]Img -command "addOperation $op" -height 24 -width 24
    tooltip .wiring.menubar.line$menubarLine.$op $op

    pack .wiring.menubar.line$menubarLine.$op -side left 
}


image create photo plotImg -file $minskyHome/icons/plot.gif
button .wiring.menubar.line$menubarLine.plot -image plotImg \
    -height 24 -width 37 -command {newPlot}
tooltip .wiring.menubar.line$menubarLine.plot "Plot"

pack .wiring.menubar.line$menubarLine.plot -side left 
# pack menubar lines
for {set i 0} {$i<=$menubarLine} {incr i} {
    pack .wiring.menubar.line$i -side top -anchor w
}
pack .wiring.menubar -fill x

canvas .wiring.canvas -height $canvasHeight -width $canvasWidth -scrollregion {-10000 -10000 10000 10000} \
    -closeenough 2 -yscrollcommand ".vscroll set" -xscrollcommand ".hscroll set"
pack .wiring.canvas -fill both -expand 1

ttk::sizegrip .sizegrip
scrollbar .vscroll -orient vertical -command ".wiring.canvas yview"
scrollbar .hscroll -orient horiz -command ".wiring.canvas xview"

grid .sizegrip -row 999 -column 999
grid .vscroll -column 999 -row 10 -rowspan 989 -sticky ns
grid .hscroll -row 999 -column 0 -columnspan 999 -sticky ew

proc get_pointer_x {c} {
  return [expr {[winfo pointerx $c] - [winfo rootx $c]}]
}

proc get_pointer_y {c} {
  return [expr {[winfo pointery $c] - [winfo rooty $c]}]
}

bind . <Key-plus> {zoom 1.1}
bind . <Key-equal> {zoom 1.1}
bind . <Key-minus> {zoom [expr 1.0/1.1]}
# mouse wheel bindings for X11
bind .wiring.canvas <Button-4> {zoom 1.1}
bind .wiring.canvas <Button-5> {zoom [expr 1.0/1.1]}
# mouse wheel bindings for pc and aqua
bind .wiring.canvas <MouseWheel> { if {%D>=0} {zoom [expr 1+.1*%D]} {zoom [expr 1.0/(1+.1*-%D)]} }

bind .wiring.canvas <Alt-Button-1> {
    tk_messageBox -message "Mouse coordinates [.wiring.canvas canvasx %x] [.wiring.canvas canvasy %y]"
}

proc zoom {factor} {
    set x0 [.wiring.canvas canvasx [get_pointer_x .wiring.canvas]]
    set y0 [.wiring.canvas canvasy [get_pointer_y .wiring.canvas]]
    disableEventProcessing
    if {$factor>1} {
        .wiring.canvas scale all $x0 $y0 $factor $factor
        minsky.zoom $x0 $y0 $factor
    } else {
        minsky.zoom $x0 $y0 $factor
        .wiring.canvas scale all $x0 $y0 $factor $factor
    }  
    # sliders need to be readjusted, because zooming doesn't do the right thing
    foreach op [operations.visibleOperations] {
        op.get $op
        foreach item [.wiring.canvas find withtag slider$op] {
            set coords [.wiring.canvas coords $item]
            # should be only one of these anyway...
            .wiring.canvas coords $item [op.x] [sliderYCoord [op.y]]
        }
    }
  
      
    enableEventProcessing
}

.menubar.ops.menu add command -label "Godley Table" -command {addNewGodleyItem [addGodleyTable 10 10]}

.menubar.ops.menu add command -label "Variable" -command "addVariable" 
foreach var [availableOperations] {
    if {$var=="numOps"} break
    .menubar.ops.menu add command -label $var -command "addOperation $var"
}

proc placeNewVar {id} {
    global moveOffsvar$id.x moveOffsvar$id.y
    set moveOffsvar$id.x 0
    set moveOffsvar$id.y 0
    disableEventProcessing
    initGroupList
    setInteractionMode 2

    bind .wiring.canvas <Enter> "move var $id var$id %x %y"
    bind .wiring.canvas <Motion> "move var $id var$id %x %y"
    bind .wiring.canvas <Button> \
        "bind .wiring.canvas <Motion> {}
         bind .wiring.canvas <Enter> {}
         checkAddGroup var $id %x %y
         bind .wiring.canvas <Button> {}"
    enableEventProcessing
}

proc addVariablePostModal {} {
    global globals
    global varInput

    set name [string trim $varInput(Name)]
    set varExists [variables.exists $name]
    set id [newVariable $name]
    var.get $id
    var.rotation $globals(default_rotation)
    var.set
    if {!$varExists} {
        value.get $name
        setItem value init {set varInput(Value)}
        setItem var rotation {set varInput(Rotation)}
    }
    closeEditWindow .wiring.initVar

    placeNewVar $id
}

proc addVariable {} {
    global varInput

    set varInput(title) "Create Variable"
    set varInput(Name) ""
    set varInput(Value) ""
    wm deiconify .wiring.initVar
    ::tk::TabToWindow $varInput(initial_focus);
    tkwait visibility .wiring.initVar
    grab set .wiring.initVar
    wm transient .wiring.initVar

}

proc addOperation {op} {
    global globals constInput
    set id [minsky.addOperation $op]
    op.get $id
    op.rotation $globals(default_rotation)
    op.set
    placeNewOp $id
    if {$op=="constant"} {
	editItem $id op$id
	set constInput(cancelCommand) "cancelPlaceNewOp $id;closeEditWindow .wiring.editConstant"
    }
}

proc placeNewOp {opid} {
    global moveOffsop$opid.x moveOffsop$opid.y
    set moveOffsop$opid.x 0
    set moveOffsop$opid.y 0
    initGroupList
    setInteractionMode 2

    drawOperation $opid
    bind .wiring.canvas <Enter> "move op $opid op$opid %x %y"
    bind .wiring.canvas <Motion> "move op $opid op$opid %x %y"
    bind .wiring.canvas <Button> \
        "bind .wiring.canvas <Motion> {}
         bind .wiring.canvas <Enter> {}
         checkAddGroup op $opid %x %y
         bind .wiring.canvas <Button> {}"
}

proc cancelPlaceNewOp {id} {
    bind .wiring.canvas <Motion> {}
    bind .wiring.canvas <Enter> {}
    bind .wiring.canvas <Button> {}
    .wiring.canvas delete op$id
    deleteOperation $id
    updateCanvas
}

proc cancelPlaceNewOp {id} {
    bind .wiring.canvas <Motion> {}
    bind .wiring.canvas <Enter> {}
    .wiring.canvas delete op$id
    deleteOperation $id
    updateCanvas
}

proc drawOperation {id} {
    op.get $id

    if {[lsearch -exact [image name] opImage$id]!=-1} {
        image delete opImage$id
    }
    image create photo opImage$id -width 200 -height 200

    .wiring.canvas delete op$id
    .wiring.canvas create operation [op.x] [op.y] -id $id -image opImage$id -tags "op$id operations" 
#    .wiring.canvas create rectangle [.wiring.canvas bbox op$id] -tags op$id

    setM1Binding op $id op$id
    op.get $id
    .wiring.canvas bind op$id <<middleMouse>> \
        "wires::startConnect [lindex [op.ports] 0] op$id %x %y"
    .wiring.canvas bind op$id <<middleMouse-Motion>> \
        "wires::extendConnect [lindex [op.ports] 0] op$id %x %y"
    .wiring.canvas bind op$id <<middleMouse-ButtonRelease>> \
        "wires::finishConnect op$id %x %y"
    .wiring.canvas bind op$id  <<contextMenu>> "contextMenu op$id %X %Y"
    .wiring.canvas bind op$id  <Double-Button-1> "doubleClick op$id %X %Y"
}

proc updateItemPos {tag item id} {
    global globals
    $item.get $id
    eval .wiring.canvas coords $tag [$item.x] [$item.y]
    foreach p [$item.ports]  {
        adjustWire $p
    }
    unset globals(updateItemPositionSubmitted$item$id)   
}    

proc submitUpdateItemPos {tag item id} {
    global globals
    if {!
        ([info exists globals(updateItemPositionSubmitted$item$id)] &&
         [set globals(updateItemPositionSubmitted$item$id)])} {
        # only submitted if no update already scheduled
        set globals(updateItemPositionSubmitted$item$id) 1
        after idle updateItemPos $tag $item $id
    }
}


proc moveSet {item id tag x y} {
    $item.get $id
    set x [.wiring.canvas canvasx $x]
    set y [.wiring.canvas canvasy $y]
    global moveOffs$item$id.x moveOffs$item$id.y
    set moveOffs$item$id.x [expr $x-[$item.x]]
    set moveOffs$item$id.y [expr $y-[$item.y]]
    if {"$item"=="groupItem"} {
        initGroupList $id
    } {
        initGroupList
    }
}

proc move {item id tag x y} {
    $item.get $id
    global moveOffs$item$id.x moveOffs$item$id.y
    set x [expr $x-[set moveOffs$item$id.x]]
    set y [expr $y-[set moveOffs$item$id.y]]
    $item.moveTo [.wiring.canvas canvasx $x] [.wiring.canvas canvasy $y]
    $item.zoomFactor [localZoomFactor $item $id [$item.x] [$item.y]]
    $item.set $id
    submitUpdateItemPos $tag $item $id
    switch $item {
        "op" {
            foreach item [.wiring.canvas find withtag slider$id] {
                set coords [.wiring.canvas coords $item]
                # should be only one of these anyway...
                .wiring.canvas coords $item [.wiring.canvas canvasx $x] \
                    [sliderYCoord [.wiring.canvas canvasy $y]]
            }
        }
    }
}

# create a new canvas item for var id
proc newVar {id} {
    global globals
    var.get $id
    if {[lsearch -exact [image name] varImage$id]!=-1} {
        image delete varImage$id
    }
    image create photo varImage$id -width 200 -height 50
    
    .wiring.canvas delete var$id
    set itemId [.wiring.canvas create variable [var.x] [var.y] -image varImage$id -id $id -tags "variables var$id"]
    # wire drawing. Can only start from an output port
    .wiring.canvas bind var$id <<middleMouse>> \
        "wires::startConnect [var.outPort] var$id %x %y"
    .wiring.canvas bind var$id <<middleMouse-Motion>> \
        "wires::extendConnect [var.outPort] var$id %x %y"
    .wiring.canvas bind var$id <<middleMouse-ButtonRelease>> "wires::finishConnect var$id %x %y"

    # context menu
    .wiring.canvas bind var$id <<contextMenu>> "contextMenu var$id %X %Y"
    .wiring.canvas bind var$id <Double-Button-1> "doubleClick var$id %X %Y"
}

proc addNewGodleyItem {id} {
    global moveOffsgodleyItem$id.x moveOffsgodleyItem$id.y
    set moveOffsgodleyItem$id.x 0
    set moveOffsgodleyItem$id.y 0
    setInteractionMode 2

    newGodleyItem $id
  
    # event bindings for initial placement
    bind .wiring.canvas <Enter> "move godleyItem $id godley$id %x %y"
    bind .wiring.canvas <Motion> "move godleyItem $id godley$id %x %y"
    bind .wiring.canvas <Button> \
        "bind .wiring.canvas <Motion> {}; bind .wiring.canvas <Enter> {}"

}
proc newGodleyItem {id} {
    global minskyHome

    godleyItem.get $id

    if {[lsearch -exact [image name] godleyImage$id]!=-1} {
        image delete godleyImage$id
    }
    image create photo godleyImage$id -width 1000 -height 1000
    .wiring.canvas create godley [godleyItem.x] [godleyItem.y] -image godleyImage$id -id $id -xgl $minskyHome/icons/bank.xgl -tags "godleys godley$id"
#    .wiring.canvas create rectangle [.wiring.canvas bbox godley$id]

    setM1Binding godleyItem $id godley$id
    .wiring.canvas bind godley$id <<middleMouse>> \
        "wires::startConnect \[closestOutPort %x %y \] godley$id %x %y"
    .wiring.canvas bind godley$id <<middleMouse-Motion>> \
        "wires::extendConnect \[closestOutPort %x %y \] godley$id %x %y"
    .wiring.canvas bind godley$id <<middleMouse-ButtonRelease>> \
        "wires::finishConnect godley$id %x %y"
    .wiring.canvas bind godley$id  <<contextMenu>> "rightMouseGodley $id %x %y %X %Y"
    .wiring.canvas bind godley$id  <Double-Button-1> "doubleMouseGodley $id %x %y"
}

proc rightMouseGodley {id x y X Y} {
    godleyItem.get $id
    set var [godleyItem.select [.wiring.canvas canvasx $x] [.wiring.canvas canvasy $y]]
    if {$var==-1} {
        contextMenu godley$id $X $Y
    } else {
        .wiring.context delete 0 end
        .wiring.context add command -label "Edit" -command "editItem $var var"
        var.get $var
        .wiring.context add command -label "Copy" -command "
           copyVar $var
           var.rotation 0
           var.set
        "
        .wiring.context post $X $Y

    }
}

proc doubleMouseGodley {id x y} {
    godleyItem.get $id
    set var [godleyItem.select [.wiring.canvas canvasx $x] [.wiring.canvas canvasy $y]]
    if {$var==-1} {
        openGodley $id
    } else {
        editItem $var var
    }
}

proc updateGodleyItem {id} {
    .wiring.canvas delete godley$id
    newGodleyItem $id
}

proc newWire {wire wireid} {
    .wiring.canvas addtag wire$wireid withtag $wire
    .wiring.canvas bind $wire <Enter> "decorateWire $wireid"
    .wiring.canvas bind $wire <<contextMenu>> "contextMenu $wire %X %Y"
}

proc updateCoords {wire handle pos x y} {
#    assert {[llength [.wiring.canvas find withtag wire$wire]]==1}
    set coords [.wiring.canvas coords wire$wire]
    set x0 [lindex $coords $pos]
    set y0 [lindex $coords [expr $pos+1]]
    set x [.wiring.canvas canvasx $x]
    set y [.wiring.canvas canvasy $y]
    .wiring.canvas move $handle [expr $x-$x0] [expr $y-$y0]
    lset coords $pos $x
    lset coords [expr $pos+1] $y
    .wiring.canvas coords wire$wire $coords
    wire.get $wire
    wire.coords $coords
    wire.set
}

proc insertCoords {wire handle pos x y} {
    global handles
#    assert {[llength [.wiring.canvas find withtag wire$wire]]==1}
    if {![info exists handles($handle)]} {
        set handles($handle) 1
        # add current handle coordinates to the wire shape
        set coords [.wiring.canvas coords wire$wire]
        set coords [linsert $coords $pos [.wiring.canvas canvasx $x] \
                    [.wiring.canvas canvasy $y]]
        .wiring.canvas coords wire$wire $coords
    }
    updateCoords $wire $handle $pos $x $y
}

proc createHandle {x y} {
    return [
        .wiring.canvas create oval \
        [expr $x-3] [expr $y-3] [expr $x+3] [expr $y+3] \
        -fill blue  -tag handles
    ]
}

proc deleteHandle {wire handle pos} {
    .wiring.canvas delete $handle
    set coords [lreplace [.wiring.canvas coords wire$wire] $pos [expr $pos+1]]
    .wiring.canvas coords wire$wire $coords        
    puts "deleteHandle: $coords"
    wire.get $wire
    wire.coords $coords
    wire.set
    decorateWire $wire
}
    

proc decorateWire {wire} {
#    global TCLwireid
#    set wireid $TCLwireid($wire)
    .wiring.canvas delete handles
    set coords [.wiring.canvas coords wire$wire]
    for {set i 0} {$i<[llength $coords]-2} {incr i 2} {
        if {$i>0} {
            set h [
                   createHandle \
                       [lindex $coords $i] [lindex $coords [expr $i+1]]
                  ]
            .wiring.canvas bind $h <B1-Motion> \
                "updateCoords $wire $h $i %x %y"
            .wiring.canvas bind $h <Double-Button-1> \
                "deleteHandle $wire $h $i"
            }
        # create a handle in between
        set h [
            createHandle [
                expr ([lindex $coords $i]+[lindex $coords [expr $i+2]])/2
            ] [
                expr ([lindex $coords [expr $i+1]]+\
                    [lindex $coords [expr $i+3]])/2]
        ]
        .wiring.canvas bind $h <B1-Motion> \
            "insertCoords $wire $h [expr $i+2] %x %y" 
        .wiring.canvas bind $h <Double-Button-1> \
            "deleteHandle $wire $h [expr $i+2]"
    }
}

namespace eval wires {
    proc startConnect {portId id x y} {
        set x [.wiring.canvas canvasx $x]
        set y [.wiring.canvas canvasy $y]
        namespace children
        if {![namespace exists $id]} {
            namespace eval $id {
                variable wire
                variable fromPort
                variable x0
                variable y0 
               
                proc extendConnect {x y} {
                    variable wire
                    variable x0
                    variable y0 
                    .wiring.canvas coords $wire $x0 $y0 $x $y
                }
                
                proc finishConnect {x y} {
                    set x [.wiring.canvas canvasx $x]
                    set y [.wiring.canvas canvasy $y]
                    variable wire
                    variable fromPort
                    variable x0
                    variable y0 
                    set portId [closestInPort $x $y]
                    if {$portId>=0} {
                        port.get $portId
                        eval .wiring.canvas coords $wire $x0 $y0 [port.x] [port.y]
                        set wireid [addWire $fromPort $portId \
                                        [.wiring.canvas coords $wire]]
                        if {$wireid == -1} {
                            # wire is invalid
                            .wiring.canvas delete $wire
                        } else {
                            newWire $wire $wireid
                        }
                    } else {
                        .wiring.canvas delete $wire
                    }
                    namespace delete [namespace current]
                }

            }

            set [set id]::wire [createWire "$x $y $x $y"]
#            set port [ports.@elem $portId]
            set [set id]::fromPort $portId
            port.get $portId
            set [set id]::x0 [port.x]
            set [set id]::y0 [port.y]
        }
#        [set id]::startConnect $x $y
    }        

    proc extendConnect {portId id x y} {
        if [namespace exists $id] {
            set x [.wiring.canvas canvasx $x]
            set y [.wiring.canvas canvasy $y]
            [set id]::extendConnect $x $y
        }
    }

    proc finishConnect {id x y} {[set id]::finishConnect $x $y}
}

# adjust the begin or end of line when a port moves
proc adjustWire {portId} {
    foreach w [wiresAttachedToPort $portId] {
        .wiring.canvas delete handles
        wire.get $w
        if [wire.visible] {
            # ensure wire exists on canvas
            if {[.wiring.canvas type wire$w]==""} {
                newWire [createWire [wire.coords]] $w
            }
            eval .wiring.canvas coords wire$w [wire.coords]
        }
   }
}

proc straightenWire {id} {
    wire.get $id
    port.get [wire.from]
    set fx [port.x]
    set fy [port.y]
    port.get [wire.to]
    wire.coords "$fx $fy [port.x] [port.y]"
    wire.set
    eval .wiring.canvas coords wire$id [wire.coords]
}

# a closestOutPort that takes into account panning
proc closestOutPort {x y} {
    return [minsky.closestOutPort [.wiring.canvas canvasx $x] [.wiring.canvas canvasy $y]]
}

proc setM1Binding {item id tag} {
    global interactionMode
    switch $interactionMode {
        1 {
            $item.get $id
            # wiring mode
            .wiring.canvas bind $tag <Button-1> \
                "wires::startConnect \[closestOutPort %x %y\] $tag %x %y"
            .wiring.canvas bind $tag <B1-Motion> \
                "wires::extendConnect \[closestOutPort %x %y\] $tag %x %y"
            .wiring.canvas bind $tag <B1-ButtonRelease>  \
                "wires::finishConnect $tag %x %y"
        }
        2 { 
            # move mode
            .wiring.canvas bind $tag <Button-1> "moveSet $item $id $tag %x %y"
            .wiring.canvas bind $tag <B1-Motion> "move $item $id $tag %x %y"
            .wiring.canvas bind $tag <B1-ButtonRelease> "move $item $id $tag %x %y; checkAddGroup $item $id %x %y"
        }
        default { 
            # pan mode
            .wiring.canvas bind $tag <Button-1> ""
            .wiring.canvas bind $tag <B1-Motion> ""
            .wiring.canvas bind $tag <B1-ButtonRelease> ""
        }
    }
}

proc setInteractionMode {args} {
    global interactionMode
    if [llength $args] {set interactionMode [lindex $args 0]}

    bind .wiring.canvas <Button-1> ""
    bind .wiring.canvas <B1-Motion> ""
    bind .wiring.canvas <B1-ButtonRelease> ""
    
    switch $interactionMode {
        3 {
            bind .wiring.canvas <Button-1> {.wiring.canvas scan mark %x %y}
            bind .wiring.canvas <B1-Motion> {.wiring.canvas scan dragto %x %y 1}
        } 
        4 {
            bind .wiring.canvas <B1-Motion> "lasso %x %y"
            bind .wiring.canvas <B1-ButtonRelease> "lassoEnd %x %y"
        }
    }


    foreach var [variables.#keys] {setM1Binding var $var var$var}
    foreach op [operations.#keys] {setM1Binding op $op op$op}
    foreach id [godleyItems.#keys] {setM1Binding godleyItem $id godley$id}
    foreach id [groupItems.#keys] {setM1Binding groupItem $id groupItem$id}
    foreach id [plots.plots.#keys] {setM1Binding plot $id plot#$id}
}

proc recentreCanvas {} {
    .wiring.canvas xview moveto 0.5
    .wiring.canvas yview moveto 0.5
}
proc updateCanvas {} {
    disableEventProcessing
    global fname showPorts
    .wiring.canvas delete all
    setInteractionMode

    # groups need to be done first, as they adjust port positions (hence wires)
    foreach g [groupItems.visibleGroups] {
        groupItem.get $g
        if {[groupItem.group]==-1} {newGroupItem $g}
    }

    foreach var [variables.visibleVariables] {
        var.get $var
        if {[var.group]==-1} {newVar $var}
    }

    # add operations
    foreach o [operations.visibleOperations] {
        op.get $o
        if {[op.group]==-1} {drawOperation $o}
        if {[op.name]=="constant"} {drawSlider $o [op.x] [op.y]}
    }

    # add wires to canvas
    foreach w [visibleWires] {
        if {[llength [.wiring.canvas find withtag wire$w]]==0} {
            wire.get $w
            set id [createWire [wire.coords]]
            newWire $id $w 
        }
    }

    foreach im [plots.plots.#keys] {
        plot.get $im
        newPlotItem $im [plot.x] [plot.y]
    }

    foreach g [godleyItems.#keys] {
        newGodleyItem $g
    }


# the following loop helps debug port placement
    if {$showPorts} {
        foreach port [ports.#keys] {
            port.get $port
            .wiring.canvas create oval \
                [expr [port.x]-2] [expr [port.y]-2] [expr [port.x]+2] [expr [port.y]+2] \
                -fill {} -outline blue 
        }
    }
  
# adjust wire coordinates to where the ports actually are
#    foreach port [ports.#keys] {
#        adjustWire $port
#    }

    enableEventProcessing
}

# mark a canvas item as in error
proc indicateCanvasItemInError {x y} {
    .wiring.canvas create oval [expr $x-15] [expr $y-15] [expr $x+15] [expr $y+15] -outline red -width 2
}

menu .wiring.context -tearoff 0

proc doubleClick {item x y} {
    # find out what type of item we're referring to
    set tags [.wiring.canvas gettags $item]
    switch -regexp $tags {
        "variables" {
            set tag [lindex $tags [lsearch -regexp $tags {var[0-9]+}]]
            set id [string range $tag 3 end]
            editItem $id $tag
        }
        "operations" {
            set tag [lindex $tags [lsearch -regexp $tags {op[0-9]+}]]
            set id [string range $tag 2 end]
            editItem $id $tag
        }
    }
}

proc toggleCoupled {id} {
    integral.get $id
    integral.toggleCoupled
    integral.set
    updateCanvas
}

# context menu
proc contextMenu {item x y} {
    # find out what type of item we're referring to
    set tags [.wiring.canvas gettags $item]
    switch -regexp $tags {
        "variables" {
            set tag [lindex $tags [lsearch -regexp $tags {var[0-9]+}]]
            set id [string range $tag 3 end]
            var.get $id
	    .wiring.context delete 0 end
            .wiring.context add command -label Help -command {help Variable}
            .wiring.context add command -label "Value [var.value]" 
            .wiring.context add command -label "Edit" -command "editItem $id $tag"
            .wiring.context add command -label "Copy" -command "copyVar $id"
            .wiring.context add command -label "Flip" -command "rotateVar $id 180; flip_default"
            .wiring.context add command -label "Browse object" -command "obj_browser [eval minsky.variables.@elem $id].*"
	    .wiring.context add command -label "Delete" -command "deleteItem $id $tag"
        }
        "operations" {
            set tag [lindex $tags [lsearch -regexp $tags {op[0-9]+}]]
            set id [string range $tag 2 end]
            op.get $id
            .wiring.context delete 0 end
            .wiring.context add command -label Help -command "help [string totitle [op.name]]"
            .wiring.context add command -label "Port values [op.portValues]" 
            .wiring.context add command -label "Edit" -command "editItem $id $tag"             
            if {[op.name]=="integrate"} {
                integral.get $id
                .wiring.context add command -label "Copy Var" -command "copyVar [integral.intVarID]"
            }
            if {[op.name]=="constant"} {
                constant.get $id
                global sliderCheck$id
                set sliderCheck$id [constant.sliderVisible]
                .wiring.context add checkbutton -label "Slider" \
                    -command "drawSlider $id $x $y" \
                    -variable "sliderCheck$id"
            }
            .wiring.context add command -label "Copy" -command "copyOp $id"
            .wiring.context add command -label "Flip" -command "rotateOp $id 180; flip_default"
            op.get $id
            if {[op.name]=="integrate"} {
                .wiring.context add command -label "Toggle var binding" -command "toggleCoupled $id"
            }
            .wiring.context add command -label "Browse object" -command "obj_browser [eval minsky.operations.@elem $id].*"
            .wiring.context add command -label "Delete" -command "deleteItem $id $tag"
        }
        "wires" {
            set tag [lindex $tags [lsearch -regexp $tags {wire[0-9]+}]]
            set id [string range $tag 4 end]
            .wiring.context delete 0 end
            .wiring.context add command -label Help -command {help Wires}
            .wiring.context add command -label "Straighten" -command "straightenWire $id"
            .wiring.context add command -label "Browse object" -command "obj_browser [eval minsky.wires.@elem $id].*"
            .wiring.context add command -label "Delete" -command "deleteItem $id $tag"
        }
        "plots" {
            set tag [lindex $tags [lsearch -regexp $tags {plot#.+}]]
            set id [string range $tag 5 end]
            .wiring.context delete 0 end
            .wiring.context add command -label Help -command {help Plot}
            .wiring.context add command -label "Expand" -command "plotDoubleClick $id"
            .wiring.context add command -label "Browse object" -command "obj_browser [eval minsky.plots.plots.@elem $id].*"
            .wiring.context add command -label "Delete" -command "deletePlot $item $id"
        }
        "godleys" {
            set tag [lindex $tags [lsearch -regexp $tags {godley[0-9]+}]]
            set id [string range $tag 6 end]
            .wiring.context delete 0 end
            .wiring.context add command -label Help -command {help GodleyTable}
            .wiring.context add command -label "Open Godley Table" -command "openGodley $id"
            .wiring.context add command -label "Browse object" -command "obj_browser [eval minsky.godleyItems.@elem $id].*"
            .wiring.context add command -label "Delete Godley Table" -command "deleteItem $id $tag"
        }
        "groupItem" {
            set tag [lindex $tags [lsearch -regexp $tags {groupItem[0-9]+}]]
            set id [string range $tag 9 end]
            groupContext $id $x $y
        }
    }
#    .wiring.context post $x $y
    tk_popup .wiring.context $x $y
}

proc flip_default {} {
   global globals
   set globals(default_rotation) [expr ($globals(default_rotation)+180)%360]
}

proc deleteItem {id tag} {
    .wiring.canvas delete $tag
    switch -regexp $tag {
        "^op" {
            deleteOperation $id
            updateCanvas
        }
        "^wire" {
            .wiring.canvas delete handles
            deleteWire $id
        }
        "^var" {
            deleteVariable $id
            updateCanvas
        }
        "^godley" {
            deleteGodleyTable $id
            destroy .godley$id
            updateCanvas
        }
    }
}

proc copyVar {id} {
    global globals
    set newId [copyVariable $id]
    newVar $newId
    var.get $newId
    var.rotation $globals(default_rotation)
    var.set
    placeNewVar $newId
}

proc copyOp  {id} {
    global globals
    set newId [copyOperation $id]
    op.get $id
    op.rotation $globals(default_rotation)
    op.set
    placeNewOp $newId 
}

proc rotateOp {id angle} {
    op.get $id
    op.rotation [expr [op.rotation]+$angle]
    op.set
    drawOperation $id
    foreach p [op.ports] {
        adjustWire $p
    }
}

proc rotateVar {id angle} {
    var.get $id
    var.rotation [expr [var.rotation]+$angle]
    var.set
    newVar $id
    adjustWire [var.outPort]
    adjustWire [var.inPort]
}

toplevel .wiring.editVar 
wm resizable .wiring.editVar 0 0
wm title .wiring.editVar "Edit Variable"
wm withdraw .wiring.editVar
wm transient .wiring.editVar .wiring

set row 0
grid [label .wiring.editVar.title -textvariable editVarInput(title)] -row $row -column 0 -columnspan 999 -pady 10

set row 10
foreach var {
    "Name"
    "Initial Value"
    "Rotation"
} {
    set rowdict($var) $row
    grid [label .wiring.editVar.label$row -text $var] -row $row -column 10 -sticky e
    grid [entry  .wiring.editVar.entry$row -textvariable editVarInput($var)] -row $row -column 20 -sticky ew -columnspan 2
    incr row 10
}
set editVarInput(initial_focus_value) ".wiring.editVar.entry$rowdict(Initial Value)"
set editVarInput(initial_focus_rotation) .wiring.editVar.entry$rowdict(Rotation)

frame .wiring.editVar.buttonBar
button .wiring.editVar.buttonBar.ok -text OK -command {
                    setItem var name {set "editVarInput(Name)"}
                    setItem value init {set "editVarInput(Initial Value)"}
                    setItem var rotation  {set editVarInput(Rotation)}
                    closeEditWindow .wiring.editVar
                }
button .wiring.editVar.buttonBar.cancel -text Cancel -command {
    closeEditWindow .wiring.editVar}
pack .wiring.editVar.buttonBar.ok [label .wiring.editVar.buttonBar.spacer -width 2] .wiring.editVar.buttonBar.cancel -side left -pady 10
grid .wiring.editVar.buttonBar -row 999 -column 0 -columnspan 1000
bind .wiring.editVar <Key-Return> {invokeOKorCancel .wiring.editVar.buttonBar}

toplevel .wiring.initVar
wm resizable .wiring.initVar 0 0
wm title .wiring.initVar "Specify variable name"
wm withdraw .wiring.initVar
wm transient .wiring.initVar .wiring

set row 0
grid [label .wiring.initVar.title -textvariable varInput(title)] -row $row -column 0 -columnspan 999 -pady 10
frame .wiring.initVar.buttonBar
button .wiring.initVar.buttonBar.ok -text OK -command "addVariablePostModal"
button .wiring.initVar.buttonBar.cancel -text Cancel -command {
    closeEditWindow .wiring.initVar}
pack .wiring.initVar.buttonBar.ok [label .wiring.initVar.buttonBar.spacer -width 2] .wiring.initVar.buttonBar.cancel -side left -pady 10
grid .wiring.initVar.buttonBar -row 999 -column 0 -columnspan 1000
bind .wiring.initVar <Key-Return> {invokeOKorCancel .wiring.initVar.buttonBar}

set row 10
foreach var {
    "Name"
    "Value"
    "Rotation"
} {
    set rowdict($var) $row
    grid [label .wiring.initVar.label$row -text $var] -row $row -column 10 -sticky e
    grid [entry  .wiring.initVar.entry$row -textvariable varInput($var)] -row $row -column 20 -sticky ew -columnspan 2
    incr row 10
}
set varInput(initial_focus) .wiring.initVar.entry$rowdict(Name)

toplevel .wiring.editConstant
wm resizable .wiring.editConstant 0 0
#wm title .wiring.editConstant "Edit Constant"
wm withdraw .wiring.editConstant
wm transient .wiring.editConstant .wiring

set row 0
grid [label .wiring.editConstant.title -textvariable constInput(title)] -row $row -column 0 -columnspan 999 -pady 10
frame .wiring.editConstant.buttonBar
button .wiring.editConstant.buttonBar.ok -text OK -command {eval $constInput(command)}
button .wiring.editConstant.buttonBar.cancel -text Cancel -command {eval $constInput(cancelCommand)}
pack .wiring.editConstant.buttonBar.ok [label .wiring.editConstant.buttonBar.spacer -width 2] .wiring.editConstant.buttonBar.cancel -side left -pady 10
grid .wiring.editConstant.buttonBar -row 999 -column 0 -columnspan 1000

set row 10
foreach var {
    "Name"
    "Value"
    "Rotation"
    "Slider Bounds: Max"
    "Slider Bounds: Min"
    "Slider Step Size"
} {
    set rowdict($var) $row
    grid [label .wiring.editConstant.label$row -text $var] -row $row -column 10 -sticky e
    grid [entry  .wiring.editConstant.entry$row -textvariable constInput($var)] -row $row -column 20 -sticky ew -columnspan 2
    incr row 10
}
set constInput(initial_focus) .wiring.editConstant.entry$rowdict(Name)

# setup textvariable for label of "Value"
set row "$rowdict(Value)"
.wiring.editConstant.label$row configure -textvariable constInput(ValueLabel)

# adjust "Slider Step Size" row to include "relative" radiobutton
set row "$rowdict(Slider Step Size)"
grid configure .wiring.editConstant.entry$row -columnspan 1
grid [checkbutton .wiring.editConstant.checkbox$row -text "relative" -variable "constInput(relative)"] -row $row -column 21 -sticky ew -columnspan 1

bind .wiring.editConstant <Key-Return> {invokeOKorCancel .wiring.editConstant.buttonBar}

toplevel .wiring.editOperation
wm resizable .wiring.editOperation 0 0
wm title .wiring.editOperation "Edit Operation"
wm withdraw .wiring.editOperation
wm transient .wiring.editOperation .wiring

frame .wiring.editOperation.buttonBar
label .wiring.editOperation.title -textvariable opInput(title)
pack .wiring.editOperation.title -pady 10
button .wiring.editOperation.buttonBar.ok -text OK -command {
    setItem op rotation {set opInput(Rotation)}
    closeEditWindow .wiring.editOperation
}
button .wiring.editOperation.buttonBar.cancel -text Cancel -command {
    closeEditWindow .wiring.editOperation}
pack .wiring.editOperation.buttonBar.ok [label .wiring.editOperation.buttonBar.spacer -width 2] .wiring.editOperation.buttonBar.cancel -side left -pady 10
pack .wiring.editOperation.buttonBar -side bottom

bind .wiring.editOperation <Key-Return> {invokeOKorCancel .wiring.editOperation.buttonBar}

frame .wiring.editOperation.rotation
label .wiring.editOperation.rotation.label -text "Rotation"
entry  .wiring.editOperation.rotation.value -width 20 -textvariable opInput(Rotation)
pack .wiring.editOperation.rotation.label .wiring.editOperation.rotation.value -side left
pack .wiring.editOperation.rotation
set opInput(initial_focus) .wiring.editOperation.rotation.value

# set attribute, and commit to original item
proc setItem {modelCmd attr dialogCmd} {
    global constInput varInput editVarInput opInput
    $modelCmd.$attr [string trim [eval $dialogCmd]]
    $modelCmd.set
}

proc setInitVal {var dialogCmd} {
    value.get $var
    value.init [eval $dialogCmd]
    value.set $var
}

proc closeEditWindow {window} {
    grab release $window
    wm withdraw $window
    updateCanvas
}

proc setConstantValue {} {
    global constInput
    constant.value "$constInput(Value)"
    constant.description "$constInput(Name)"
    constant.sliderStepRel "$constInput(relative)"
    constant.sliderMin "$constInput(Slider Bounds: Min)"
    constant.sliderMax "$constInput(Slider Bounds: Max)"
    constant.sliderStep "$constInput(Slider Step Size)"
    constant.sliderBoundsSet 1
}

proc setIntegralIValue {} {
    global constInput
    value.get [integral.description "$constInput(Name)"]
    setItem value init {set constInput(Value)}
}

proc editItem {id tag} {
    global constInput varInput editVarInput opInput
    switch -regexp $tag {
        "^var" {
            var.get $id
            wm title .wiring.editVar "Edit [var.name]"
            value.get [var.name]
            set "editVarInput(Name)" [var.name]
            set "editVarInput(Initial Value)" [value.init]
            set "editVarInput(Rotation)" [var.rotation]
            if {[value.godleyOverridden] || [variables.inputWired [var.name]]} {
                $editVarInput(initial_focus_value) configure -state disabled  -foreground gray
		::tk::TabToWindow $editVarInput(initial_focus_rotation)
            } else {
                $editVarInput(initial_focus_value) configure -state normal  -foreground black
		::tk::TabToWindow $editVarInput(initial_focus_value)
             }
            set editVarInput(title) "[var.name]: Value=[value.value]"
            wm deiconify .wiring.editVar
	    tkwait visibility .wiring.editVar
	    grab set .wiring.editVar
	    wm transient .wiring.editVar
        }
        "^op" {
            op.get $id
            if {[op.name]=="constant" || [op.name]=="integrate"} {
                set constInput(Value) ""
                set "constInput(Slider Bounds: Min)" ""
                set "constInput(Slider Bounds: Max)" ""
                set "constInput(Slider Step Size)" ""
                switch [op.name] {
                    constant {
                        wm title .wiring.editConstant "Edit Constant"
                        constant.get $id
                        set constInput(Name) [constant.description]
                        set constInput(ValueLabel) "Value"
                        set constInput(Value) [constant.value]
                        constant.initOpSliderBounds
			set "constInput(Slider Bounds: Min)" [constant.sliderMin]
			set "constInput(Slider Bounds: Max)" [constant.sliderMax]
			set "constInput(Slider Step Size)" [constant.sliderStep]
                        set constInput(relative) [constant.sliderStepRel]
                        set setValue setConstantValue
                    }
                    integrate {
                        wm title .wiring.editConstant "Edit Integral"
                        integral.get $id
                        set constInput(ValueLabel) "Initial Value"
                        value.get [integral.description]
                        set constInput(Value) [value.init]
                        set setValue setIntegralIValue
                        set constInput(Name) [integral.description]
                    }
                }
                set constInput(title) $constInput(Name)
                set constInput(Rotation) [op.rotation]
                # value needs to be regotten, as var name may have changed
                set constInput(command) "
                        $setValue
                        setSliderProperties $id
                        setItem op rotation {set constInput(Rotation)}
                        closeEditWindow .wiring.editConstant
                    "
		set constInput(cancelCommand) "closeEditWindow .wiring.editConstant"

                wm deiconify .wiring.editConstant
		::tk::TabToWindow $constInput(initial_focus);
		tkwait visibility .wiring.editConstant
		grab set .wiring.editConstant
		wm transient .wiring.editConstant

            } else {
                set opInput(title) [op.name]
                set opInput(Rotation) [op.rotation]
                wm deiconify .wiring.editOperation
		::tk::TabToWindow $opInput(initial_focus);
		tkwait visibility .wiring.editOperation
		grab set .wiring.editOperation
		wm transient .wiring.editOperation
            }
        }
        "^groupItem" {groupEdit $id}
    }
}

proc setOpVal {op x} {
    constant.get $op
    if {$x!=[constant.value]} {
        constant.value $x
        # override the reset flag, to prevent simulation being set to t=0
        resetNotNeeded
    }
}

proc setSliderProperties {id} {
    if [winfo  exists .wiring.slider$id] {
        constant.get $id
        constant.initOpSliderBounds
        if [constant.sliderStepRel] {
            set res [expr [constant.sliderStep]*([constant.sliderMax]-[constant.sliderMin])]
        } else {
            set res [constant.sliderStep]
        }
        
        # ensure resolution is accurate enough to not mutate variable value
        set decPos [string first . [constant.value]]
        if {$decPos==-1} {
            set newRes 1
        } else {
            set newRes [expr pow(10,1+$decPos-[string len [constant.value]])]
        }
       if {$newRes<$res} {set res $newRes}

        # ensure slider does not override value
        constant.adjustSliderBounds

        .wiring.slider$id configure -to [constant.sliderMax] \
            -from [constant.sliderMin] -resolution $res
        .wiring.slider$id set [constant.value]
    }
}

# if y is the y-coordinate of the constant, then return a y-coordinate
# for an attached slider
proc sliderYCoord {y} {
    return [expr $y-15-10*[zoomFactor]]
}

proc drawSlider {op x y} {
    global sliderCheck$op
    constant.get $op
    if {![info exists sliderCheck$op]} {
        # sliderCheck$op gets initialised to constant.sliderVisible,
        # otherwise sliderCheck$op is more up to date
        set sliderCheck$op [constant.sliderVisible]
    }
    if {[constant.sliderVisible]!=[set sliderCheck$op]} {
        constant.sliderVisible [set sliderCheck$op]
    }

    if {[set sliderCheck$op]} {
        if {![winfo exists .wiring.slider$op]} {
            scale .wiring.slider$op -orient horizontal -width 7 -length 50 \
                -showvalue 1 -sliderlength 30 
        }

        setSliderProperties $op

        # configure command after slider initially set to prevent
        # constant value being set to initial state of slider when
        # constructed.
        .wiring.slider$op configure -command "setOpVal $op"

        .wiring.canvas create window [op.x] [sliderYCoord [op.y]] -window .wiring.slider$op -tag slider$op
        # this is needed to ensure the setOpVal is fired _before_
        # moving on to processing the next operation in updateCanvas
        update
    } else {
        #remove any slider
        .wiring.canvas delete slider$op
    }
}

proc tout {args} {
  puts "$args"
}

# example debugging trace statements
#trace add execution placeNewVar enterstep tout
#trace add execution move enterstep tout
