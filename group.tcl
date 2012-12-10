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

# Group (or block) functionality

# convert degrees to radian
proc radian {deg} {
    return [expr $deg*3.1415927/180]
}

proc newGroupItem {id} {
    global minskyHome
    groupItem.get $id
    if {[lsearch -exact [image name] groupImage$id]!=-1} {
        image delete groupImage$id
    }
    image create photo groupImage$id -width [groupItem.width] -height [groupItem.height]
    .wiring.canvas create group [groupItem.x] [groupItem.y] -image groupImage$id -id $id -xgl $minskyHome/icons/group.xgl -tags "group$id groups"
    .wiring.canvas lower group$id

     setM1Binding groupItem $id group$id
    .wiring.canvas bind group$id <<middleMouse-Motion>> \
        "wires::extendConnect \[closestOutPort %x %y \] group$id %x %y"
    .wiring.canvas bind group$id <<middleMouse-ButtonRelease>> \
        "wires::finishConnect group$id %x %y"
    .wiring.canvas bind group$id  <<contextMenu>> "contextMenu group$id %X %Y"
    .wiring.canvas bind group$id  <Double-Button-1> "groupEdit $id"
   
}

proc deleteGroupItem {id} {
    groupItem.get $id
    ungroup $id
    updateCanvas
}

proc lasso {x y} {
    global lassoStart
    if {![info exists lassoStart]} {
        set lassoStart "$x $y"
        .wiring.canvas create rectangle $x $y $x $y -tag lasso
    }
    eval .wiring.canvas coords lasso $lassoStart $x $y
}

proc lassoEnd {x y} {
    global lassoStart
    if [info exists lassoStart] {
        eval group $x $y $lassoStart
        .wiring.canvas delete lasso
        updateCanvas
        unset lassoStart
        # convert back to move mode for later editing
        global interactionMode
        set interactionMode 2
        setInteractionMode
    }
}

proc groupContext {id x y} {
    .wiring.context delete 0 end
    .wiring.context add command -label "Ungroup" -command "deleteGroupItem $id"
    .wiring.context add command -label "Edit" -command "groupEdit $id"
    .wiring.context add command -label "Resize" -command "group::resize $id"
    .wiring.context add command -label "Copy" -command "group::copy $id"
    .wiring.context add command -label "Flip" -command "group::flip $id"
    .wiring.context add command -label "Browse object" -command "obj_browser [eval minsky.groupItems.@elem $id].*"

}

toplevel .wiring.editGroup
wm title .wiring.editGroup "Edit Group"
wm withdraw .wiring.editGroup
wm transient .wiring.editGroup .wiring

frame .wiring.editGroup.name
label .wiring.editGroup.name.label -text "Name"
entry  .wiring.editGroup.name.val -width 20
pack .wiring.editGroup.name.label .wiring.editGroup.name.val -side left

frame .wiring.editGroup.rot
label .wiring.editGroup.rot.label -text "     Rotation"
entry  .wiring.editGroup.rot.val -width 20
pack .wiring.editGroup.rot.label .wiring.editGroup.rot.val -side left

pack .wiring.editGroup.name .wiring.editGroup.rot

frame .wiring.editGroup.buttonBar
button .wiring.editGroup.buttonBar.ok -text OK
button .wiring.editGroup.buttonBar.cancel -text Cancel -command {
    closeEditWindow .wiring.editGroup}
pack .wiring.editGroup.buttonBar.ok .wiring.editGroup.buttonBar.cancel -side left
pack .wiring.editGroup.buttonBar -side bottom

bind .wiring.editGroup <Key-Return> {invokeOKorCancel .wiring.editGroup.buttonBar}

proc groupEdit {id} {
    groupItem.get $id
    .wiring.editGroup.name.val delete 0 end
    .wiring.editGroup.name.val insert 0 [groupItem.name]
    .wiring.editGroup.rot.val delete 0 end
    .wiring.editGroup.rot.val insert 0 [groupItem.rotation]
    .wiring.editGroup.buttonBar.ok configure \
        -command {
            setItem groupItem name {.wiring.editGroup.name.val get}
            setItem groupItem rotation {.wiring.editGroup.rot.val get}
            groupItem.updatePortLocation
            closeEditWindow .wiring.editGroup
        }
    wm deiconify .wiring.editGroup
    grab .wiring.editGroup
}

namespace eval group {
    proc resize {id} {
        groupItem.get $id
        set bbox [.wiring.canvas bbox group$id]
        variable orig_width [expr [lindex $bbox 2]-[lindex $bbox 0]]
        variable orig_height [expr [lindex $bbox 3]-[lindex $bbox 1]]
        variable orig_x [groupItem.x]
        variable orig_y [groupItem.y]
        set item [eval .wiring.canvas create rectangle $bbox]
        bind .wiring.canvas <Motion> "group::resizeRect $item %x %y"
        bind .wiring.canvas <ButtonRelease> "group::resizeItem $item $id %x %y"
    }

    # resize the bounding box to indicate how big we want the icon to be
    proc resizeRect {item x y} {
        variable orig_x
        variable orig_y
        set w [expr abs($x-$orig_x)]
        set h [expr abs($y-$orig_y)]
        .wiring.canvas coords $item  [expr $orig_x-$w] [expr $orig_y-$h] \
            [expr $orig_x+$w] [expr $orig_y+$h]
    }

    # compute width and height and redraw item
    proc resizeItem {item id x y} {
        .wiring.canvas delete $item
        variable orig_width
        variable orig_height
        variable orig_x
        variable orig_y
        set scalex [expr 2*abs($x-$orig_x)/double($orig_width)]
        set scaley [expr 2*abs($y-$orig_y)/double($orig_height)]
        # compute rotated scale factors
        set angle [radian [groupItem.rotation]]
        set rx [expr $scalex*cos($angle)-$scaley*sin($angle)]
        set ry [expr $scalex*sin($angle)+$scaley*cos($angle)]
        groupItem.width [expr int(ceil(abs($rx*[groupItem.width])))]
        groupItem.height [expr int(ceil(abs($ry*[groupItem.height])))]
        groupItem.computeDisplayZoom
        groupItem.set
        .wiring.canvas delete group$id
        newGroupItem $id
        foreach p [groupItem.ports]  {
            adjustWire $p
        }
        bind .wiring.canvas <Motion> {}
        bind .wiring.canvas <ButtonRelease> {}
    }

    proc copy {id} {
        set newId [copyGroup $id]
        newGroupItem $newId
        # this only works in move mode
        global interactionMode 
        set interactionMode 2
        setInteractionMode
        bind .wiring.canvas <Motion> "move groupItem $newId group$newId %x %y"
        bind .wiring.canvas <ButtonRelease> {
            bind .wiring.canvas <Motion> {}
            bind .wiring.canvas <ButtonRelease> {}
        }
    }

    proc flip {id} {
        groupItem.get $id
        groupItem.rotation [expr [groupItem.rotation]+180]
        groupItem.set
        .wiring.canvas delete group$id
        newGroupItem $id
        foreach p [groupItem.ports] {
            adjustWire $p
        }
    }
}
