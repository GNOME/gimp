#!@THEGIMPTCL@

proc gimptcl_query {} {
    gimp-install-procedure "plug_in_sbutton" \
	"Turn the selection into a button" \
	"None Yet" \
	"Eric L. Hernes" \
	"ELH" \
	"1997" \
	"<Image>/Select/Make Button" \
	"RGB*, GRAY*" \
	plugin \
	{
	    {int32 "run_mode" "Interactive, non-interactive"}
	    {image "image" "The image"}
	    {drawable "drawable" "The drawable"}
	    {int32 "width" "The Button Width"}
	    {int32 "hightlight" "The amount of highlight"}
	    {int32 "shadow" "The amount of shadow"}
	    {string "angle" "one of `TopLeft' `TopRight' 'BottomLeft' `BottomRight' or a number in the rage [0-360]'"}
	} \
	{
	}
}

proc gimptcl_run {mode image drawable width highlight shadow angle} {
    global theImage theDrawable
    global theBorderWidth theHighlightLevel theShadowLevel
    global theLightAngle theLightAngleMode theLightAngleValue

    set theImage $image
    set theDrawable $drawable

    set stuff [gimp-get-data plug_in_sbutton]
    if {$stuff == ""} {
	set theBorderWidth 10
	set theHighlightLevel 100
	set theShadowLevel 100
	set theLightAngle TopLeft

	set theLightAngleMode TopLeft
	set theLightAngleValue 135
    } else {
	set theBorderWidth [lindex $stuff 0]
	set theHighlightLevel [lindex $stuff 1]
	set theShadowLevel [lindex $stuff 2]
	set theLightAngleMode [lindex $stuff 3]
	set theLightAngleValue [lindex $stuff 4]
    }

    switch $mode {
	0 {
	    buttonUi
	}

	1 {
	    set theBorderWidth $width
	    set theHighlightLevel $highlight
	    set theShadowLevel $shadow
	    set theLightAngle $angle
	    buttonCore
	}

	2 {
	    set theBorderWidth [lindex $stuff 0]
	    set theHighlightLevel [lindex $stuff 1]
	    set theShadowLevel [lindex $stuff 2]
	    set theLightAngle [lindex $stuff 3]
	    buttonCore
	}
	default {
	    puts stderr "don't know how to handle run mode $mode"
	}
    }
}

proc buttonCore {} {
    global theImage theDrawable
    global theBorderWidth theHighlightLevel theShadowLevel
    global theLightAngleMode theLightAngleValue

    set s_bounds [gimp-selection-bounds $theImage]

    set sx1 [lindex $s_bounds 1]
    set sy1 [lindex $s_bounds 2]

    set sx2 [lindex $s_bounds 3]
    set sy2 [lindex $s_bounds 4]
   
    set sw [expr $sx2 - $sx1].0
    set sh [expr $sy2 - $sy1].0

    set theta [expr atan($sh/$sw) * 180 / 3.1415926]

    switch  $theLightAngleMode {
	TopLeft {
	    set theAngle [expr 180 - $theta]
	}
	TopRight {
	    set theAngle [expr 180 + $theta]
	}
	BottomLeft {
	    set theAngle $theta
	}
	BottomRight {
	    set theAngle [expr 360 - $theta]
	}
	default {
	    set theAngle $theLightAngle
	}
    }

    doButton $theImage $theDrawable $theBorderWidth $theHighlightLevel \
	$theShadowLevel $theAngle

    set saveData [list $theBorderWidth $theHighlightLevel \
		  $theShadowLevel $theLightAngleMode $theAngle]

    gimp-set-data plug_in_sbutton $saveData

#    puts stderr "setting (plug_in_sbutton :$saveData:)"

    destroy .
}

proc doButton {img drw width highlight shadow angle} {

    set theta [expr ($angle * 3.1415926 / 180) + (3.1415926 / 2)]

    set bg [gimp-palette-get-background]
    set fg [gimp-palette-get-foreground]
    set selection [gimp-selection-save $img]

    set w [gimp-image-width $img]
    set h [gimp-image-height $img]

    set layers [lindex [gimp-image-get-layers $img] 1]

    set vlayers [list]

    foreach l $layers {
	if [gimp-layer-get-visible layer-$l] {
	    lappend vlayers $l
	}
	gimp-layer-set-visible layer-$l 0
    }
    gimp-layer-set-visible $drw 1

    gimp-undo-push-group-start $img

    set button_layer [gimp-layer-new $img $w $h RGBA_IMAGE \
			  "button (angle $angle)" 100 OVERLAY]
    gimp-image-add-layer $img $button_layer -1
    gimp-drawable-fill $button_layer TRANS_IMAGE_FILL

    #	gimp-selection-border $img $width
    gimp-palette-set-background {0 0 0}
    gimp-edit-fill $img $button_layer
    gimp-selection-shrink $img $width
    gimp-edit-clear $img $button_layer
    gimp-selection-layer-alpha $img $button_layer

    set hl [expr 128 + $highlight]
    set sl [expr 128 - $shadow]
    gimp-palette-set-background "$hl $hl $hl"
    gimp-palette-set-foreground "$sl $sl $sl"

    set s_bounds [gimp-selection-bounds $img]

    set sx1 [lindex $s_bounds 1]
    set sy1 [lindex $s_bounds 2]

    set sx2 [lindex $s_bounds 3]
    set sy2 [lindex $s_bounds 4]
   
    set sw [expr $sx2 - $sx1]
    set sh [expr $sy2 - $sy1]

    set sw2 [expr $sw / 2]
    set sh2 [expr $sh / 2]

    set xdelta [expr cos($theta) * ($sw / 2)]
    set ydelta [expr sin($theta) * ($sh / 2)]

    set x1 [expr $sw2 - $xdelta + $sx1]
    set y1 [expr $sh2 - $ydelta + $sy1]

    set x2 [expr $sw2 + $xdelta + $sx1]
    set y2 [expr $sh2 + $ydelta + $sy1]

    gimp-blend $img $button_layer FG-BG-RGB NORMAL LINEAR 100 0 0 0 0 0 $x1 $y1 $x2 $y2 

    gimp-image-merge-visible-layers $img EXPAND-AS-NECESSARY
    foreach l $vlayers {
      gimp-layer-set-visible layer-$l 1
    }

    gimp-palette-set-background $bg
    gimp-palette-set-foreground $fg
    gimp-selection-load $img $selection
    gimp-image-remove-channel $img $selection

    gimp-undo-push-group-end $img

    gimp-drawable-update $button_layer 0 0 $w $h
    gimp-displays-flush
}

proc buttonUi {} {
    #
    # defaults...

    wm title . "Selection to Button"

    frame .control
    button .control.ok -text "Ok" -command {
	buttonCore
    }

    button .control.cancel -text "Cancel" -command {destroy .}

    pack .control.ok .control.cancel -side left

    frame .parameters

    set lev [frame .parameters.levels -relief groove -borderwidth 3]

    scale $lev.width -label "Border Width" \
	-from 0 -to 50 -variable theBorderWidth -orient horizontal

    scale $lev.highlight -label "Highlight Level" \
	-from 0 -to 127 -variable theHighlightLevel -orient horizontal

    scale $lev.shadow -label "Shadow Level" \
	-from 0 -to 127 -variable theShadowLevel -orient horizontal

    pack $lev.width $lev.highlight $lev.shadow

    set la [frame .parameters.lightAngle -relief groove -borderwidth 3]

    label $la.label -text "Light Angle"

    radiobutton $la.topLeft -text "Top Left" \
	-variable theLightAngleMode -value TopLeft \
	-command "$la.ui.scale configure -state disabled"

    radiobutton $la.topRight -text "Top Right" \
	-variable theLightAngleMode -value TopRight \
	-command "$la.ui.scale configure -state disabled"

    radiobutton $la.bottomLeft -text "Bottom Left" \
	-variable theLightAngleMode -value BottomLeft \
	-command "$la.ui.scale configure -state disabled"

    radiobutton $la.bottomRight -text "Bottom Right" \
	-variable theLightAngleMode -value BottomRight \
	-command "$la.ui.scale configure -state disabled"

    frame $la.ui
    radiobutton $la.ui.button -text "Set Angle" \
	-variable theLightAngleMode -value UserSet \
	-command "$la.ui.scale configure -state normal"

    scale $la.ui.scale -state disabled \
	-from 0 -to 360 -variable theLightAngleValue -orient horizontal

    pack $la.ui.button $la.ui.scale -side left

    pack $la.label

    pack $la.topLeft $la.topRight $la.bottomLeft $la.bottomRight $la.ui \
	-anchor w

    pack $lev $la -side left -anchor n -fill both

    pack .parameters .control
}
