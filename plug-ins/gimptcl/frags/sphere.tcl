
proc Shadow_Sphere {radius sphere_color bg_color light shadow} {
    global RGB_IMAGE RGB NORMAL TRUE FALSE FG_BG_RGB
    global MULTIPLY REPLACE BG_BUCKET_FILL RADIAL

#     puts "radius: $radius"
#     puts "sphere_color: $sphere_color"
#     puts "bg_color: $bg_color"
#     puts "light: $light"
#     puts "shadow: $shadow"

    set w [expr $radius * 3.52]
    set h [expr $radius * 2.40]

    set img [gimp-image-new $w $h $RGB]
    set drawable [gimp-layer-new $img $w $h \
		      $RGB_IMAGE "Sphere" 100 $NORMAL]

    set radians [expr ($light * 3.1415926) / 180]
    set cx [expr $w / 2]
    set cy [expr $h / 2]

    set light_x [expr $cx + ((cos($radians) * 0.6) * $radius)]
    set light_y [expr $cy - ((sin($radians) * 0.6) * $radius)]
    set light_end_x [expr $cx + ($radius * (cos(3.1415926 + $radians)))]
    set light_end_y [expr $cy - ($radius * (sin(3.1415926 + $radians)))]
    set offset [expr $radius * 0.1]

    gimp-image-disable-undo $img
    gimp-image-add-layer $img $drawable 0
    gimp-palette-set-foreground $sphere_color
    gimp-palette-set-background $bg_color
    gimp-edit-fill $img $drawable
    gimp-palette-set-background {20 20 20}

    if {((($light >= 45) && ($light <= 75))
	 || (($light <= 135) && ($light >= 105)))
	&& ($shadow == $TRUE)} {
	set shadow_w [expr ($radius * 2.5) * (cos(3.1415926 * $radians))]
	set shadow_h [expr $radius * 0.5]
	set shadow_x $cx
	set shadow_y [expr $cy + ($radius * 0.65)]
	if {$shadow_w < 0} {
	    set shadow_x [expr $cx + $shadow_w]
	    set shadow_w [expr -$shadow_w]
	}
	gimp-ellipse-select $img $shadow_x $shadow_y \
	    $shadow_w $shadow_h $REPLACE $TRUE $TRUE 7.5
	gimp-bucket-fill $img $drawable $BG_BUCKET_FILL \
	    $MULTIPLY 100 0 $FALSE 0 0

    }
    gimp-ellipse-select $img [expr $cx - $radius] \
     [expr $cy - $radius] [expr 2 * $radius] [expr 2 * $radius] \
	$REPLACE $TRUE $FALSE 0
    gimp-blend $img $drawable $FG_BG_RGB $NORMAL $RADIAL 100 \
	$offset $light_x $light_y $light_end_x $light_end_y
    gimp-selection-none $img
    gimp-image-enable-undo $img
    gimp-display-new $img
    return $img
}

#Shadow_Sphere 100 {255 0 0} {255 255 255} 135 $TRUE
