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

.menubar.ops.menu add command -label Plot -command "newPlot"

set nextPlotImage 0

proc newPlotItem {image x y} {
    global nextPlotImage
    if  {[lsearch -exact [image names] $image]!=-1} {
        image delete $image
    }
    image create photo $image -width 150 -height 150
    
    set id [.wiring.canvas create plot $x $y -image $image -scale 1 -rotation 0\
                -tags "plots plot#$image"]
    setM1Binding plot $image plots#$image
    .wiring.canvas bind $id <Double-Button-1> "plotDoubleClick $image"
    .wiring.canvas bind $id <<contextMenu>> "contextMenu $id %X %Y"
}

proc newPlot {} {
    # place this at the mouse if in canvas, otherwise at 0 0

    set image [plots.nextPlotID]
    set id [eval newPlotItem $image 0 0 ]

    bind .wiring.canvas <Enter> "movePlot $image %x %y"
    bind .wiring.canvas <Motion> "movePlot $image %x %y"
    bind .wiring.canvas <Button-1> \
        "bind .wiring.canvas <Motion> {}; bind .wiring.canvas <Enter> {}"

    return $id
}

set updatePlotPositionSubmitted 0

proc updatePlotPosition {image} {
    global updatePlotPositionSubmitted
    plot.get $image
    .wiring.canvas coords plot#$image [plot.x] [plot.y]
    foreach port [plot.ports] {
        adjustWire $port
    }
    set updatePlotPositionSubmitted 0   
}

proc movePlot {image x y} {
    plot.get $image
    plot.moveTo  $x $y
    plot.set
    # for some reason, we should queue up screen updates to happen once idle, to prevent Tk from getting overloaded
    global updatePlotPositionSubmitted
    if {!$updatePlotPositionSubmitted} {
        # only submitted if no update already scheduled
        set updatePlotPositionSubmitted 1
        after idle updatePlotPosition $image
    }
}

toplevel .pltWindowOptions
frame .pltWindowOptions.xticks
label .pltWindowOptions.xticks.label -text "Number of x ticks"
entry  .pltWindowOptions.xticks.val -width 20
pack .pltWindowOptions.xticks.label .pltWindowOptions.xticks.val  -side left

frame .pltWindowOptions.yticks
label .pltWindowOptions.yticks.label -text "Number of y ticks"
entry  .pltWindowOptions.yticks.val -width 20
pack .pltWindowOptions.yticks.label .pltWindowOptions.yticks.val  -side left

frame .pltWindowOptions.grid
label .pltWindowOptions.grid.label -text "Grid"
label .pltWindowOptions.grid.sublabel -text "Subgrid"
checkbutton .pltWindowOptions.grid.val -variable plotWindowOptions_grid -command {plot.grid $plotWindowOptions_grid}
checkbutton .pltWindowOptions.grid.subval -variable plotWindowOptions_subgrid -command {plot.subgrid $plotWindowOptions_subgrid}
pack .pltWindowOptions.grid.label  .pltWindowOptions.grid.val  .pltWindowOptions.grid.sublabel  .pltWindowOptions.grid.subval  -side left

frame .pltWindowOptions.buttonBar
button .pltWindowOptions.buttonBar.ok -text OK
button .pltWindowOptions.buttonBar.cancel -text Cancel -command {
    wm withdraw .pltWindowOptions
    grab release .pltWindowOptions 
}
pack .pltWindowOptions.buttonBar.ok .pltWindowOptions.buttonBar.cancel -side left
pack .pltWindowOptions.buttonBar -side bottom

pack .pltWindowOptions.xticks .pltWindowOptions.yticks .pltWindowOptions.grid
wm withdraw .pltWindowOptions
set plotWindowOptions_grid
set plotWindowOptions_subgrid

proc setPlotOptions {image} {
    global plotWindowOptions_grid plotWindowOptions_subgrid
# TODO this can lose data if sim is running. When plot becomes an actual reference, not a copy, this problem will be averted
    plot.get $image
    plot.grid $plotWindowOptions_grid
    plot.subgrid $plotWindowOptions_subgrid
    plot.nxTicks [.pltWindowOptions.xticks.val get]
    plot.nyTicks [.pltWindowOptions.yticks.val get]
    plot.set
    plot.redraw
    wm withdraw .pltWindowOptions 
    grab release .pltWindowOptions 
}

proc doPlotOptions {image} {
    global plotWindowOptions_grid plotWindowOptions_subgrid
    plot.get $image
    set plotWindowOptions_grid [plot.grid]
    set plotWindowOptions_subgrid [plot.subgrid]


    .pltWindowOptions.xticks.val delete 0 end
    .pltWindowOptions.xticks.val insert 0 [plot.nxTicks]
    .pltWindowOptions.yticks.val delete 0 end
    .pltWindowOptions.yticks.val insert 0 [plot.nyTicks]
    .pltWindowOptions.buttonBar.ok configure -command "setPlotOptions $image"
    wm deiconify .pltWindowOptions
    grab .pltWindowOptions
}

# w and h are requested window size, dw, dh are difference between
# image and window dimensions
proc resizePlot {image w h dw dh} {
    if {[winfo width .plot$image]!=[expr [.plot$image.image cget -width]+$dw] ||
        [winfo height .plot$image]!=[expr [.plot$image.image cget -height]+$dh]} {
        .plot$image.image configure -height [expr [winfo height .plot$image]-$dh] -width [expr [winfo width .plot$image]-$dw]
        plots.addImage $image .plot$image.image
    }
}

# double click handling for plot (creates new toplevel plot window)
proc plotDoubleClick {image} {

    toplevel .plot$image
    image create photo .plot$image.image -width 500 -height 500
    label .plot$image.label -image .plot$image.image

    labelframe .plot$image.menubar -relief raised
    button .plot$image.menubar.options -text Options -command "doPlotOptions $image" -relief flat
    pack .plot$image.menubar.options -side left

    pack .plot$image.menubar  -side top -fill x
    pack .plot$image.label
    
    plots.addImage $image .plot$image.image

    # calculate the difference between the window an image sizes
    update
    set dw [expr [winfo width .plot$image]-[.plot$image.image cget -width]]
    set dh [expr [winfo height .plot$image]-[.plot$image.image cget -height]]

    bind .plot$image <Configure> "resizePlot $image  %w %h $dw $dh"
}
    
proc deletePlot {item image} {
    .wiring.canvas delete $item
    minsky.deletePlot $image
    updateCanvas
}
    
