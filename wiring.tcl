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

# Wiring canvas

proc createWire {coords} {
    .wiring.canvas create line $coords -tag wires -arrow last -smooth bezier
}

#toplevel .wiring 
frame .wiring 
pack .wiring -fill both -expand 1
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

image create photo godleyImg -file $minskyHome/icons/bank.gif
button .wiring.menubar.godley -image godleyImg -height 24 -width 37 \
    -command {addNewGodleyItem [addGodleyTable 10 10]}
tooltip .wiring.menubar.godley "Godley table"

image create photo varImg -file $minskyHome/icons/var.gif
button .wiring.menubar.var -image varImg -height 24 -width 37 \
    -command addVariable
tooltip .wiring.menubar.var "variable"

image create photo constImg -file $minskyHome/icons/const.gif
button .wiring.menubar.const -height 24 -width 37 -image constImg -command {
    addOperation constant}
tooltip .wiring.menubar.const "constant"

image create photo timeImg -file $minskyHome/icons/time.gif
button .wiring.menubar.time -height 24 -image timeImg -command {
    addOperation "time"}
tooltip .wiring.menubar.time "time"

image create photo integrateImg -file $minskyHome/icons/integrate.gif
button .wiring.menubar.integrate -image integrateImg -command {
    addOperation integrate}
tooltip .wiring.menubar.integrate integrate

image create photo expImg -file $minskyHome/icons/exp.gif
button .wiring.menubar.exp -image expImg -command {addOperation exp}
tooltip .wiring.menubar.exp exp

image create photo addImg -file $minskyHome/icons/add.gif
button .wiring.menubar.add -image addImg -command {addOperation add}
tooltip .wiring.menubar.add add

image create photo subtractImg -file $minskyHome/icons/subtract.gif
button .wiring.menubar.subtract -image subtractImg -command {
    addOperation subtract}
tooltip .wiring.menubar.subtract subtract

image create photo multiplyImg -file $minskyHome/icons/multiply.gif
button .wiring.menubar.multiply -image multiplyImg -command {
    addOperation multiply}
tooltip .wiring.menubar.multiply multiply

image create photo divideImg -file $minskyHome/icons/divide.gif
button .wiring.menubar.divide -image divideImg -command {addOperation divide}
tooltip .wiring.menubar.divide divide

image create photo plotImg -file $minskyHome/icons/plot.gif
button .wiring.menubar.plot -image plotImg -height 24 -width 37 \
    -command {newPlot}
tooltip .wiring.menubar.plot "Plot"

pack .wiring.menubar.wiringmode .wiring.menubar.movemode .wiring.menubar.panmode .wiring.menubar.lassomode -side left
pack .wiring.menubar.godley .wiring.menubar.var .wiring.menubar.const .wiring.menubar.time -side left
pack .wiring.menubar.integrate .wiring.menubar.exp .wiring.menubar.add .wiring.menubar.subtract .wiring.menubar.multiply .wiring.menubar.divide .wiring.menubar.plot -side left
pack .wiring.menubar -fill x


canvas .wiring.canvas -height 600 -width 800 -scrollregion {0 0 10000 10000} \
    -closeenough 2
pack .wiring.canvas -fill both -expand 1

.menubar.ops.menu add command -label "Godley Table" -command {addNewGodleyItem [addGodleyTable 10 10]}

.menubar.ops.menu add command -label "Variable" -command "addVariable" 
foreach var [availableOperations] { 
    .menubar.ops.menu add command -label $var -command "addOperation $var"
}

proc placeNewVar {id} {
    global moveOffsvar$id.x moveOffsvar$id.y
    set moveOffsvar$id.x 0
    set moveOffsvar$id.y 0
    setInteractionMode 2

    bind .wiring.canvas <Enter> "move var $id var$id %x %y"
    bind .wiring.canvas <Motion> "move var $id var$id %x %y"
    bind .wiring.canvas <Button> \
        "bind .wiring.canvas <Motion> {}; bind .wiring.canvas <Enter> {}"
}

proc addVariablePostModal {} {
    set name [string trim [.wiring.initVar.text.value get]]
    set varExists [variables.exists $name]
    set id [newVariable $name]
    if {!$varExists} {
        var.get $id
        value.get $name
        setItem value init {.wiring.initVar.val.value get}
        setItem var rotation {.wiring.initVar.rotation.value get}
    }
    closeEditWindow .wiring.initVar

    placeNewVar $id
}

proc addVariable {} {
    .wiring.initVar.title configure -text "Create Variable"
    .wiring.initVar.text.value delete 0 end
    .wiring.initVar.text.value insert 0 ""
    .wiring.initVar.val.value delete 0 end
    .wiring.initVar.val.value insert 0 0
    .wiring.initVar.buttonBar.ok configure \
        -command "addVariablePostModal"
    wm deiconify .wiring.initVar
    grab .wiring.initVar
}

proc addOperation {op} {
    set id [minsky.addOperation $op]
    placeNewOp $id
    if {$op=="constant"} {editItem $id op$id}
}

proc placeNewOp {opid} {
    global moveOffsop$opid.x moveOffsop$opid.y
    set moveOffsop$opid.x 0
    set moveOffsop$opid.y 0
    setInteractionMode 2

    drawOperation $opid
    bind .wiring.canvas <Enter> "move op $opid op$opid %x %y"
    bind .wiring.canvas <Motion> "move op $opid op$opid %x %y"
    bind .wiring.canvas <Button> \
        "bind .wiring.canvas <Motion> {}; bind .wiring.canvas <Enter> {}"
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
    global updateItemPositionSubmitted
    $item.get $id
    eval .wiring.canvas coords $tag [$item.x] [$item.y]
    foreach p [$item.ports]  {
        adjustWire $p
    }
    set updateItemPositionSubmitted 0   
}    

set updateItemPositionSubmitted 0

proc moveSet {item id tag x y} {
    $item.get $id
    set x [.wiring.canvas canvasx $x]
    set y [.wiring.canvas canvasy $y]
    global moveOffs$item$id.x moveOffs$item$id.y
    set moveOffs$item$id.x [expr $x-[$item.x]]
    set moveOffs$item$id.y [expr $y-[$item.y]]
}

proc move {item id tag x y} {
    $item.get $id
    global moveOffs$item$id.x moveOffs$item$id.y
    set x [expr $x-[set moveOffs$item$id.x]]
    set y [expr $y-[set moveOffs$item$id.y]]
    $item.moveTo [.wiring.canvas canvasx $x] [.wiring.canvas canvasy $y]
    $item.set $id
    global updateItemPositionSubmitted
    if {!$updateItemPositionSubmitted} {
        # only submitted if no update already scheduled
        set updateItemPositionSubmitted 1
        after idle updateItemPos $tag $item $id
    }
    if {$item=="op"} {
        foreach item [.wiring.canvas find withtag slider$id] {
            set coords [.wiring.canvas coords $item]
            .wiring.canvas move $item [expr $x-[lindex $coords 0]] [expr $y-[lindex $coords 1]-25]
        }
    }
}

# create a new canvas item for var id
proc newVar {id} {
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
    .wiring.canvas move $handle [expr $x-$x0] [expr $y-$y0]
    lset coords $pos $x
    lset coords [expr $pos+1] $y
    .wiring.canvas coords wire$wire $coords
    wireCoords $wire $coords
}

proc insertCoords {wire handle pos x y} {
    global handles
#    assert {[llength [.wiring.canvas find withtag wire$wire]]==1}
    if {![info exists handles($handle)]} {
        set handles($handle) 1
        # add current handle coordinates to the wire shape
        set coords [.wiring.canvas coords wire$wire]
        set coords [linsert $coords $pos $x $y]
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
    set coords [lreplace [.wiring.canvas coords $wire] $pos [expr $pos+1]]
    .wiring.canvas coords $wire $coords        
    wireCoords $wire $coords
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
#                        set port [ports.@elem $portId]
                        eval .wiring.canvas coords $wire $x0 $y0 [portCoords $portId]
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
            set portCoords [portCoords $portId]
            set [set id]::x0 [lindex $portCoords 0]
            set [set id]::y0 [lindex $portCoords 1]
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
            .wiring.canvas bind $tag <B1-ButtonRelease> "move $item $id $tag %x %y"
        }
        default { 
            # pan mode
            .wiring.canvas bind $tag <B1-Motion> ""
            .wiring.canvas bind $tag <B1-ButtonRelease> ""
        }
    }
}

proc setInteractionMode {args} {
    global interactionMode
    if [llength $args] {set interactionMode [lindex $args 0]}

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
    foreach id [groupItems.#keys] {setM1Binding groupItem $id group$id}
}

proc updateCanvas {} {
    global fname showPorts
#    wm title .wiring "Wiring diagram: $fname (Prototype)"
#    .wiring.canvas delete variables operations wires handles plots
    .wiring.canvas delete all
    foreach var [variables.visibleVariables] {
        newVar $var

    }

    setInteractionMode

    # groups need to be done first, as they adjust port positions (hence wires)
    foreach g [groupItems.#keys] {
        newGroupItem $g
    }

    # TODO add operations
    foreach o [operations.visibleOperations] {
        op.get $o
        drawOperation $o
        if {[op.name]=="constant"} {drawSlider $o [op.x] [op.y]}
    }

    # add wires to canvas
    foreach w [visibleWires] {
#        set wire [wires.@elem $w]
        set id [createWire [wireCoords $w]]
        newWire $id $w 
    }

    foreach im [plots.plots.#keys] {
        newPlotItem $im [plots.X $im] [plots.Y $im]
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
    op.get $id
    op.toggleCoupled
    op.set
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
            .wiring.context delete 0 end
            .wiring.context add command -label "Delete" -command "deleteItem $id $tag"
            .wiring.context add command -label "Edit" -command "editItem $id $tag"
            .wiring.context add command -label "Copy" -command "copyVar $id"
            .wiring.context add command -label "Flip" -command "rotateVar $id 180"
        }
        "operations" {
            set tag [lindex $tags [lsearch -regexp $tags {op[0-9]+}]]
            set id [string range $tag 2 end]
            op.get $id
            .wiring.context delete 0 end
            .wiring.context add command -label "Delete" -command "deleteItem $id $tag"
            .wiring.context add command -label "Edit" -command "editItem $id $tag"             
            if {[op.name]=="integrate"} {
                .wiring.context add command -label "Copy Var" -command "copyVar [op.intVarID]"
            }
            if {[op.name]=="constant"} {
                global sliderCheck$id
                set sliderCheck$id [op.sliderVisible]
                .wiring.context add checkbutton -label "Slider" \
                    -command "drawSlider $id $x $y" \
                    -variable "sliderCheck$id"
            }
            .wiring.context add command -label "Copy" -command "copyOp $id"
            .wiring.context add command -label "Flip" -command "rotateOp $id 180"
            op.get $id
            if {[op.name]=="integrate"} {
                .wiring.context add command -label "Toggle var binding" -command "toggleCoupled $id"
            }
        }
        "wires" {
            set tag [lindex $tags [lsearch -regexp $tags {wire[0-9]+}]]
            set id [string range $tag 4 end]
            .wiring.context delete 0 end
            .wiring.context add command -label "Straighten" -command "straightenWire $id"
            .wiring.context add command -label "Delete" -command "deleteItem $id $tag"
        }
        "plots" {
            set tag [lindex $tags [lsearch -regexp $tags {plot#.+}]]
            set id [string range $tag 5 end]
            .wiring.context delete 0 end
            .wiring.context add command -label "Expand" -command "plotDoubleClick $id"
            .wiring.context add command -label "Delete" -command "deletePlot $item $id"
        }
        "godleys" {
            set tag [lindex $tags [lsearch -regexp $tags {godley[0-9]+}]]
            set id [string range $tag 6 end]
            .wiring.context delete 0 end
            .wiring.context add command -label "Open Godley Table" -command "openGodley $id"
            .wiring.context add command -label "Delete Godley Table" -command "deleteItem $id $tag"
        }
        "group" {
            set tag [lindex $tags [lsearch -regexp $tags {group[0-9]+}]]
            set id [string range $tag 5 end]
            groupContext $id $x $y
        }
    }
#    .wiring.context post $x $y
    tk_popup .wiring.context $x $y
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
            updateCanvas
        }
    }
}

proc copyVar {id} {
    set newId [copyVariable $id]
    newVar $newId
    placeNewVar $newId
}
proc copyOp  {id} {placeNewOp [copyOperation $id]}

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
wm title .wiring.editVar "Edit Variable"
wm withdraw .wiring.editVar
wm transient .wiring.editVar .wiring

label .wiring.editVar.name
pack .wiring.editVar.name

frame .wiring.editVar.init
label .wiring.editVar.init.label -text "Initial Value"
entry  .wiring.editVar.init.val -width 20
pack .wiring.editVar.init.label .wiring.editVar.init.val -side left

frame .wiring.editVar.rot
label .wiring.editVar.rot.label -text "     Rotation"
entry  .wiring.editVar.rot.val -width 20
pack .wiring.editVar.rot.label .wiring.editVar.rot.val -side left

pack .wiring.editVar.init .wiring.editVar.rot

frame .wiring.editVar.buttonBar
button .wiring.editVar.buttonBar.ok -text OK
button .wiring.editVar.buttonBar.cancel -text Cancel -command {
    closeEditWindow .wiring.editVar}
pack .wiring.editVar.buttonBar.ok .wiring.editVar.buttonBar.cancel -side left
pack .wiring.editVar.buttonBar -side bottom
bind .wiring.editVar <Key-Return> {invokeOKorCancel .wiring.editVar.buttonBar}

toplevel .wiring.initVar
wm title .wiring.initVar "Specify variable name"
wm withdraw .wiring.initVar
wm transient .wiring.initVar .wiring

frame .wiring.initVar.buttonBar
label .wiring.initVar.title
pack .wiring.initVar.title
button .wiring.initVar.buttonBar.ok -text OK
button .wiring.initVar.buttonBar.cancel -text Cancel -command {
    closeEditWindow .wiring.initVar}
pack .wiring.initVar.buttonBar.ok .wiring.initVar.buttonBar.cancel -side left
pack .wiring.initVar.buttonBar -side bottom

frame .wiring.initVar.text
label .wiring.initVar.text.label -text "Name"
entry  .wiring.initVar.text.value -width 20
pack .wiring.initVar.text.label .wiring.initVar.text.value -side left

frame .wiring.initVar.val
label .wiring.initVar.val.label -text "Value"
entry  .wiring.initVar.val.value -width 20
pack .wiring.initVar.val.label .wiring.initVar.val.value -side left

frame .wiring.initVar.rotation
label .wiring.initVar.rotation.label -text "Rotation"
entry  .wiring.initVar.rotation.value -width 20
pack .wiring.initVar.rotation.label .wiring.initVar.rotation.value -side left
pack .wiring.initVar.text .wiring.initVar.val .wiring.initVar.rotation 

toplevel .wiring.editConstant
wm title .wiring.editConstant "Edit Constant"
wm withdraw .wiring.editConstant
wm transient .wiring.editConstant .wiring

frame .wiring.editConstant.buttonBar
label .wiring.editConstant.title
pack .wiring.editConstant.title
button .wiring.editConstant.buttonBar.ok -text OK
button .wiring.editConstant.buttonBar.cancel -text Cancel -command {
    closeEditWindow .wiring.editConstant}
pack .wiring.editConstant.buttonBar.ok .wiring.editConstant.buttonBar.cancel -side left
pack .wiring.editConstant.buttonBar -side bottom

frame .wiring.editConstant.text
label .wiring.editConstant.text.label -text "Name"
entry  .wiring.editConstant.text.value -width 20
pack .wiring.editConstant.text.label .wiring.editConstant.text.value -side left

frame .wiring.editConstant.val
label .wiring.editConstant.val.label -text "Value"
entry  .wiring.editConstant.val.value -width 20
pack .wiring.editConstant.val.label .wiring.editConstant.val.value -side left

frame .wiring.editConstant.rotation
label .wiring.editConstant.rotation.label -text "Rotation"
entry  .wiring.editConstant.rotation.value -width 20
pack .wiring.editConstant.rotation.label .wiring.editConstant.rotation.value -side left

frame .wiring.editConstant.sliderBoundsMax
label .wiring.editConstant.sliderBoundsMax.label -text "Slider Bounds: Max"
entry  .wiring.editConstant.sliderBoundsMax.val -width 20
pack .wiring.editConstant.sliderBoundsMax.val .wiring.editConstant.sliderBoundsMax.label -side right

frame .wiring.editConstant.sliderBoundsMin
label .wiring.editConstant.sliderBoundsMin.label -text "Slider Bounds: Min"
entry  .wiring.editConstant.sliderBoundsMin.val -width 20
pack .wiring.editConstant.sliderBoundsMin.val .wiring.editConstant.sliderBoundsMin.label -side right

frame .wiring.editConstant.stepSize
label .wiring.editConstant.stepSize.label -text "Slider Step Size:"
entry  .wiring.editConstant.stepSize.val -width 20
checkbutton .wiring.editConstant.stepSize.rel -text "relative" -variable relStepSize
pack .wiring.editConstant.stepSize.label .wiring.editConstant.stepSize.val .wiring.editConstant.stepSize.rel -side left

pack .wiring.editConstant.text .wiring.editConstant.val .wiring.editConstant.rotation .wiring.editConstant.sliderBoundsMin .wiring.editConstant.sliderBoundsMax .wiring.editConstant.stepSize

bind .wiring.editConstant <Key-Return> {invokeOKorCancel .wiring.editConstant.buttonBar}


toplevel .wiring.editOperation
wm title .wiring.editOperation "Edit Constant"
wm withdraw .wiring.editOperation
wm transient .wiring.editOperation .wiring

frame .wiring.editOperation.buttonBar
label .wiring.editOperation.title
pack .wiring.editOperation.title
button .wiring.editOperation.buttonBar.ok -text OK
button .wiring.editOperation.buttonBar.cancel -text Cancel -command {
    closeEditWindow .wiring.editOperation}
pack .wiring.editOperation.buttonBar.ok .wiring.editOperation.buttonBar.cancel -side left
pack .wiring.editOperation.buttonBar -side bottom

bind .wiring.editOperation <Key-Return> {invokeOKorCancel .wiring.editOperation.buttonBar}

frame .wiring.editOperation.rotation
label .wiring.editOperation.rotation.label -text "Rotation"
entry  .wiring.editOperation.rotation.value -width 20
pack .wiring.editOperation.rotation.label .wiring.editOperation.rotation.value -side left
pack .wiring.editOperation.rotation

# set attribute, and commit to original item
proc setItem {modelCmd attr dialogCmd} {
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
    op.value [.wiring.editConstant.val.value get]
    global relStepSize
    op.sliderStepRel $relStepSize
    op.sliderMin [.wiring.editConstant.sliderBoundsMin.val get]
    op.sliderMax [.wiring.editConstant.sliderBoundsMax.val get]
    op.sliderStep [.wiring.editConstant.stepSize.val get]
    op.sliderBoundsSet 1
    op.set
}

proc setIntegralIValue {} {
    value.get [op.description]
    setItem value init {.wiring.editConstant.val.value get}
}

proc editItem {id tag} {
    switch -regexp $tag {
        "^var" {
            var.get $id
            wm title .wiring.editVar "Edit [var.name]"
            .wiring.editVar.init.val delete 0 end
            value.get [var.name]
            .wiring.editVar.init.val insert 0 [value.init]
            if {[value.godleyOverridden] || [variables.inputWired [var.name]]} {
                .wiring.editVar.init.val configure -state disabled  -foreground gray
            } else {
                .wiring.editVar.init.val configure -state normal  -foreground black
             }
           .wiring.editVar.rot.val delete 0 end
           .wiring.editVar.rot.val insert 0 [var.rotation]
            .wiring.editVar.buttonBar.ok configure \
                -command {
                    setItem value init {.wiring.editVar.init.val get}
                    setItem var rotation  {.wiring.editVar.rot.val get}
                    closeEditWindow .wiring.editVar
                }
            .wiring.editVar.name configure -text "[var.name]: Value=[value.value]"
            wm deiconify .wiring.editVar
            grab .wiring.editVar
        }
        "^op" {
            op.get $id

            if {[op.name]=="constant" || [op.name]=="integrate"} {
                .wiring.editConstant.val.value delete 0 end
                .wiring.editConstant.sliderBoundsMin.val delete 0 end
                .wiring.editConstant.sliderBoundsMax.val delete 0 end
                .wiring.editConstant.stepSize.val delete 0 end
                switch [op.name] {
                    constant {
                        .wiring.editConstant.val.label configure -text "Value"
                        .wiring.editConstant.val.value insert 0 [op.value]
                        initOpSliderBounds
                        .wiring.editConstant.sliderBoundsMin.val insert 0 [op.sliderMin]
                        .wiring.editConstant.sliderBoundsMax.val insert 0 [op.sliderMax]
                        .wiring.editConstant.stepSize.val insert 0 [op.sliderStep]
                        set relStepSize [op.sliderStepRel]
                        set setValue setConstantValue
                    }
                    integrate {
                        .wiring.editConstant.val.label configure -text "Initial Value"
                        value.get [op.description]
                        .wiring.editConstant.val.value insert 0 [value.init]
                        set setValue setIntegralIValue
                    }
                }
                .wiring.editConstant.title configure -text [op.name]
                .wiring.editConstant.text.value delete 0 end
                .wiring.editConstant.text.value insert 0 [op.description]
                .wiring.editConstant.rotation.value delete 0 end
                .wiring.editConstant.rotation.value insert 0 [op.rotation]
                # value needs to be regotten, as var name may have changed
                .wiring.editConstant.buttonBar.ok configure \
                    -command "
                        setItem op description {.wiring.editConstant.text.value get}
                        $setValue
                        setSliderProperties $id
                        setItem op rotation {.wiring.editConstant.rotation.value get}
                        closeEditWindow .wiring.editConstant
                    "

                wm deiconify .wiring.editConstant
                grab .wiring.editConstant

            } else {
                .wiring.editOperation.title configure -text [op.name]
                .wiring.editOperation.rotation.value delete 0 end
                .wiring.editOperation.rotation.value insert 0 [op.rotation]
                .wiring.editOperation.buttonBar.ok configure \
                    -command {
                        setItem op rotation {.wiring.editOperation.rotation.value get}
                        closeEditWindow .wiring.editOperation
                    }
                wm deiconify .wiring.editOperation
                grab .wiring.editOperation
            }
        }
        "^group" {groupEdit $id}
    }
}

proc setOpVal {op x} {
    op.get $op
    op.value $x
    op.set
}

# initialises sliderbounds based on current value, if not set otherwise
proc initOpSliderBounds {} {
    if {![op.sliderBoundsSet]} {
        if {[op.value]==0} {
            op.sliderMin -1
            op.sliderMax 1
            op.sliderStep 0.1
        } else {
            op.sliderMin [expr -[op.value]*10]
            op.sliderMax [expr [op.value]*10]
            op.sliderStep [expr abs(0.1*[op.value])]
        }
        op.sliderStepRel 0
        op.sliderBoundsSet 1
        op.set
    }
}
 
proc setSliderProperties {id} {
    if [winfo  exists .wiring.slider$id] {
        op.get $id
        initOpSliderBounds
        if [op.sliderStepRel] {
            set res [expr [op.sliderStep]*([op.sliderMax]-[op.sliderMin])]
        } else {
            set res [op.sliderStep]
        }
        
        # ensure resolution is accurate enough to not mutate variable value
        set decPos [string first . [op.value]]
        if {$decPos==-1} {
            set newRes 1
        } else {
            set newRes [expr pow(10,1+$decPos-[string len [op.value]])]
        }
       if {$newRes<$res} {set res $newRes}

        # ensure slider does not override value
        if {[op.sliderMax]<[op.value]} {op.sliderMax [op.value]}
        if {[op.sliderMin]>[op.value]} {op.sliderMin [op.value]}

        set origValue [op.value]
        .wiring.slider$id configure -to [op.sliderMax] -from [op.sliderMin] -resolution $res
        .wiring.slider$id set [op.value]
        op.value $origValue
        op.set
    }
}

proc drawSlider {op x y} {
    global sliderCheck$op
    op.get $op
    if {![info exists sliderCheck$op]} {
        # sliderCheck$op gets initialised to op.sliderVisible,
        # otherwise sliderCheck$op is more up to date
        set sliderCheck$op [op.sliderVisible]
    }
    op.sliderVisible [set sliderCheck$op]
    op.set

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

        .wiring.canvas create window [op.x] [expr [op.y]-25] -window .wiring.slider$op -tag slider$op
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
#trace add execution placeNewOp enterstep tout
#trace add execution setOpVal enter tout
#trace add execution setOpVal leave tout
#trace add execution drawOperation enter tout
#trace add execution drawOperation leave tout
