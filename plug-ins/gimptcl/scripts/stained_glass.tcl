#!@THEGIMPTCL@
# -*-Mode: Tcl;-*-
#

set red 0
set green 0
set blue 0
set fg1 [list 47 17 216]
set bg1 [list 55 214 23]
set fg2 [list 247 229 37]
set bg2 [list 244 24 31]

proc gimptcl_query {} {
    gimp-install-procedure "plug_in_stained_glass" \
	"Crank out stained-glassy backgrounds" \
	"None Yet" \
	"Eric L. Hernes" \
	"Eric L. Hernes" \
	"1997" \
        "<Image>/Filters/Distorts/Stained Glass" \
	"RGB" \
	plugin \
	{
	    {int32 "run_mode" "Interactive, Non-interactive"}
	    {image "image" "The image to build the background for"}
	    {drawable "drawable" "The drawable to use"}
	} \
	{}
#    puts stderr "Testicle installed"
}
# 	    {color "foreground1" "First Foreground Color"}
# 	    {color "background1" "First Background Color"}
# 	    {color "foreground2" "Second Foreground Color"}
# 	    {color "background2" "Second Background Color"}

proc gimptcl_run {mode img drw} {
#    puts stderr "mode $mode; image: $img; drawable: $drw"
    switch $mode {
	0 {
#	    puts "stained-glass: interactive!"
	    global image
	    set image $img
	    build-ui
	}
	1 {
#	    puts "stained-glass: non-interactive!"
	}
	2 {
#	    puts "stained-glass: last args!"
	}
    }
}

proc gen-bg {img fg1 bg1 fg2 bg2} {
#     puts "gen-bg:"
#     puts "      img $img"
#     puts "      fg1 $fg1"
#     puts "      bg1 $bg1"
#     puts "      fg2 $fg2"
#     puts "      bg2 $bg2"

    set width [gimp-image-width $img]
    set height [gimp-image-height $img]

    set b1 [gimp-layer-new $img $width $height RGBA_IMAGE "Blend 1" \
			 100 NORMAL]
    set old_fg [gimp-palette-get-foreground]
    set old_bg [gimp-palette-get-background]

    gimp-palette-set-foreground $fg1
    gimp-palette-set-background $bg1
    
    gimp-blend $img $b1 FG_BG_RGB NORMAL CONICAL_SYMMETRIC 100 0 \
	[expr $width * 0.25] [expr $height * 0.40] \
	[expr $width * 0.12] [expr $height * 0.20]
    gimp-blend $img $b1 FG_BG_RGB NORMAL CONICAL_SYMMETRIC 50 0 \
	[expr $width * 0.85] [expr $height * 0.75] \
	[expr $width * 0.92] [expr $height * 0.85]
    gimp-image-add-layer $img $b1 1

     set b2 [gimp-layer-new $img $width $height RGBA_IMAGE "Blend 2" \
 			 50 SUBTRACT]
     gimp-palette-set-foreground $fg2
     gimp-palette-set-background $bg2
     gimp-blend $img $b2 FG_BG_RGB NORMAL CONICAL_SYMMETRIC 100 0 \
 	[expr $width * 0.76] [expr $height * 0.39] \
 	[expr $width * 0.88] [expr $height * 0.91]

     gimp-blend $img $b2 FG_BG_RGB NORMAL CONICAL_SYMMETRIC 50 0 \
 	[expr $width * 0.30] [expr $height * 0.75] \
 	[expr $width * 0.15] [expr $height * 0.85]
     gimp-image-add-layer $img $b2 1

     foreach layer [lindex [gimp-image-get-layers $img] 1] {
 	if {$layer != $b1 && $layer != $b2} {
 	    gimp-layer-set-visible $layer 0
 	}
     }
   
     set d [gimp-image-merge-visible-layers $img 0]

    gimp-image-set-active-layer $img $d
    plug-in-c-astretch $img $d
    plug-in-mosaic $img $d 15 15 1.1 0.15 60 0.5 1 1 1 1 0

    gimp-palette-set-foreground $old_fg
    gimp-palette-set-background $old_bg

    gimp-displays-flush
    gimp-layer-set-name $d "Stained Glass BG"
#    return $d
}

proc build-ui {} {
    global fg1 bg1 fg2 bg2
    set _fg1 [format "\#%02x%02x%02x" [lindex $fg1 0] [lindex $fg1 1] [lindex $fg1 2]]
    set _bg1 [format "\#%02x%02x%02x" [lindex $bg1 0] [lindex $bg1 1] [lindex $bg1 2]]
    set _fg2 [format "\#%02x%02x%02x" [lindex $fg2 0] [lindex $fg2 1] [lindex $fg2 2]]
    set _bg2 [format "\#%02x%02x%02x" [lindex $bg2 0] [lindex $bg2 1] [lindex $bg2 2]]

    frame .ctl
    frame .ctl.1 -relief ridge -borderwidth 3
    button .ctl.1.f -background $_fg1 -text "Foreground 1" \
	-command {colorset .ctl.1.f fg1 [list $red $green $blue]}
    button .ctl.1.b -background $_bg1 -text "Background 1" \
	-command {colorset .ctl.1.b bg1 [list $red $green $blue]}
    pack .ctl.1.f .ctl.1.b -side left -fill both

    frame .ctl.2 -relief ridge -borderwidth 3
    button .ctl.2.f -background $_fg2 -text "Foreground 2" \
	-command {colorset .ctl.2.f fg2 [list $red $green $blue]}
    button .ctl.2.b -background $_bg2 -text "Background 2" \
	-command {colorset .ctl.2.b bg2 [list $red $green $blue]}
    pack .ctl.2.f .ctl.2.b -side left -fill both

    pack .ctl.1 .ctl.2 -side top -fill both
    label .ctl.l -textvariable GimpPDBCmd
    pack .ctl.l

    frame .ctl.aq
    button .ctl.aq.apply -text Apply -command {gen-bg $image $fg1 $bg1 $fg2 $bg2}
    button .ctl.aq.quit -text Quit -command "destroy ."
    pack .ctl.aq.apply .ctl.aq.quit -side left

    pack .ctl.aq -side bottom

    rgb-widget .rgb

    pack .rgb .ctl -side left -fill both
}

proc colorset {widget var val} {
    global red green blue $var

    set $var $val
    $widget configure -background [format "\#%02x%02x%02x" $red $green $blue]
    catch "$widget configure -activebackground [format "\#%02x%02x%02x" $red $green $blue]" e
}

proc rgb-widget {t} {
    frame $t -relief ridge -borderwidth 3
    label $t.b -text "Color"
    frame $t.s
    foreach c {red green blue} {
	frame $t.s.$c
	label $t.s.$c.n -text $c -justify r
	scale $t.s.$c.s -orient horizontal -from 0 -to 255 \
	    -command "colorset $t.b $c"
	pack $t.s.$c.n $t.s.$c.s -side left -fill both
	pack $t.s.$c -side top -anchor e
    }
    pack $t.s $t.b -fill both
}

proc sg-apply {} {
    global image fg1 bg1 fg2 bg2
    
}
