set DebugLevel 0
set size 120
set font "ParkAvenue"
set text_pattern "Fibers"
set bg_color {255 255 255}

proc gen-bg {img} {
    set width [gimp-image-width $img]
    set height [gimp-image-height $img]

    set b1 [gimp-layer-new $img $width $height RGBA_IMAGE "Blend 1" \
			 50 NORMAL]
    set old_fg [gimp-palette-get-foreground]
    set old_bg [gimp-palette-get-background]

    gimp-palette-set-foreground {47 17 216}
    gimp-palette-set-background {55 214 23}
    
    gimp-blend $img $b1 FG_BG_RGB NORMAL CONICAL_SYMMETRIC 100 \
	0 95 95 40 45 
    gimp-blend $img $b1 FG_BG_RGB NORMAL CONICAL_SYMMETRIC 50 \
	0 280 180 310 205
    gimp-image-add-layer $img $b1 1

    set b2 [gimp-layer-new $img $width $height RGBA_IMAGE "Blend 2" \
			 100 NORMAL]
    gimp-palette-set-foreground {247 229 37}
    gimp-palette-set-background {244 24 31}
    gimp-blend $img $b2 FG_BG_RGB NORMAL CONICAL_SYMMETRIC 100 \
	0 270 95 300 220
    gimp-blend $img $b2 FG_BG_RGB NORMAL CONICAL_SYMMETRIC 50 \
	0 105 180 58 205
    gimp-image-add-layer $img $b2 1

    foreach layer [lindex [gimp-image-get-layers $img] 1] {
	if {$layer != $b1 && $layer != $b2} {
	    gimp-layer-set-visible $layer 0
	}
    }
    
    set d [gimp-image-merge-visible-layers $img 0]

    gimp-image-set-active-layer $img $d
    plug-in-c-astretch $img $d
    plug-in-mosaic $img $d 15 15 1.1 0.0 60 0.5 1 1 1 1 0

    gimp-palette-set-foreground $old_fg
    gimp-palette-set-background $old_bg

    return $d
}

proc add-text {img text} {
    global size bg_color text_pattern font 

    set f_size [expr $size * 0.075]
    set b_size [expr $size * 0.1]
    set b_size_2 [expr $size * 0.05]
    set ts_size [expr $b_size_2 - 3]
    set ds_size [expr $size * 0.05]
    set old_w [gimp-image-width $img]
    set old_h [gimp-image-height $img]

    set text_layer [gimp-text $img -1 0 0 $text $b_size TRUE $size PIXELS "*" $font "*" "*" "*" "*"]

    set old_fg [gimp-palette-get-foreground]
    set old_bg [gimp-palette-get-background]

    set width [gimp-drawable-width $text_layer]
    set height [gimp-drawable-height $text_layer]
    gimp-image-resize $img $width $height 0 0

    set text_shadow_layer [gimp-layer-new $img $width $height RGBA_IMAGE "Text Shadow" 100 MULTIPLY]
    set tsl_layer_mask [gimp-layer-create-mask $text_shadow_layer BLACK_MASK]

    set drop_shadow_layer [gimp-layer-new $img $width $height RGBA_IMAGE "Drop Shadow" 100 MULTIPLY]
    set dsl_layer_mask [gimp-layer-create-mask $drop_shadow_layer BLACK_MASK]
    
    gimp-image-add-layer $img $drop_shadow_layer 1
    gimp-image-add-layer $img $text_shadow_layer 0

    gimp-selection-all $img
    gimp-patterns-set-pattern $text_pattern
    gimp-layer-set-preserve-trans $text_layer TRUE
    gimp-bucket-fill $img $text_layer PATTERN_BUCKET_FILL NORMAL 100 0 FALSE 0 0
    gimp-selection-none $img
    gimp-edit-clear $img $text_shadow_layer
    gimp-edit-clear $img $drop_shadow_layer
    gimp-palette-set-background $bg_color
    
    gimp-selection-layer-alpha $img $text_layer
    gimp-image-add-layer-mask $img $text_shadow_layer $tsl_layer_mask
    gimp-palette-set-background {255 255 255}
    gimp-edit-fill $img $tsl_layer_mask
    gimp-selection-feather $img $f_size
    gimp-palette-set-background {23 23 23}
    gimp-edit-fill $img $drop_shadow_layer
    gimp-palette-set-background {0 0 0}
    gimp-edit-fill $img $text_shadow_layer
    gimp-palette-set-foreground {255 255 255}
    gimp-blend $img $text_shadow_layer FG_BG_RGB NORMAL SHAPEBURST_ANGULAR 100 0 0 0 1 1
    gimp-selection-none $img
    gimp-layer-translate $text_layer -$b_size_2 -$b_size_2
    gimp-layer-translate $text_shadow_layer -$ts_size -$ts_size
    gimp-layer-translate $drop_shadow_layer $ds_size $ds_size
    gimp-image-add-layer-mask $img $drop_shadow_layer $dsl_layer_mask
    gimp-palette-set-background {255 255 255}
    gimp-edit-fill $img $dsl_layer_mask
    gimp-image-remove-layer-mask $img $drop_shadow_layer APPLY

    gimp-palette-set-foreground $old_fg
    gimp-palette-set-background $old_bg

    foreach layer [lindex [gimp-image-get-layers $img] 1] {
	if {$layer != $text_layer
	    && $layer != $text_shadow_layer
	    && $layer != $drop_shadow_layer} {
	    gimp-layer-set-visible $layer 0
	}
    }
    set d [gimp-image-merge-visible-layers $img 0]

    gimp-image-resize $img $old_w $old_h 0 0
    return $d

}


proc run {} {
    set bg_w 352
    set bg_h 240

    set img [gimp-image-new 352 240 RGB]
    set bg_d [gen-bg $img]
    set t_d [add-text $img "Eric L. Hernes, esq."]
    gimp-layer-set-visible $bg_d 0
    gimp-layer-set-visible $t_d 0

    set frame 0
    set tx_w [gimp-drawable-width $t_d]
    set tx_h [gimp-drawable-height $t_d]
    set y [expr $bg_h - $tx_h]

#    for {set x $bg_w;set frame 0} {$x + $tx_w > 0} {incr x -1;incr frame} 
#    for {set x [expr $bg_w - 584];set frame 583} {$x + $tx_w > 0} {incr x -3;incr frame} 
    set ppf 2
    set frame 0
#    {$x + $tx_w > 0} 
    set frame 412
    set total [expr ($bg_w + $tx_w) / $ppf]
    set eframe $total
    set eframe 413
    for {set x [expr $bg_w - [expr $frame * $ppf]]} {$frame < $eframe} {
	incr x -$ppf;incr frame} {
	set bg [gimp-layer-copy $bg_d 0]
	plug-in-whirl $img $bg [expr -10 * $frame]
	set bg_c [gimp-layer-copy $bg 0]
	gimp-layer-delete $bg
	gimp-image-add-layer $img $bg_c 0
	set t_c [gimp-layer-copy $t_d 0]
	gimp-image-add-layer $img $t_c 0
	gimp-layer-set-visible $bg_c 1
	gimp-layer-set-visible $t_c 1
	gimp-layer-set-offsets $t_c $x $y
	set d [gimp-image-merge-visible-layers $img 2]
	set fn [format "/src/t/%04d.jpg" $frame]
	puts "$fn : $x $y \[$frame / $total\]"
	file-jpeg-save $img $d $fn 100 0
	gimp-layer-set-visible $d 0
	gimp-layer-delete $d
    }
}

proc w {i d} {
    set r [plug-in-whirl $i $d -25]
    gimp-displays-flush
    return $r
}

proc m {i d} {
    set r [plug-in-mosaic $i $d 15 15 1.1 0.0 60 0.5 1 1 1 1 0]
    gimp-displays-flush
    return $r
}

proc r {} {
    set i [gimp-image-new 256 256 RGB]
    set d [gen-bg $i]
    set tx [add-text $i "Eric L. Hernes"]
    gimp-display-new $i
    return [list $i $d $tx]
}