
proc scale {size percent} {
    return [expr $size * $percent]
}

proc bds-logo {bg text_pattern blend_fg blend_bg tile_type text size font} {
    global RGB TRUE FALSE RGBA_IMAGE NORMAL MULTIPLY 
    global BLACK_MASK PAT_BUCKET_FILL BG_IMAGE_FILL REPLACE
    global FG_BG_RGB SHAPEBURST_ANGULAR LINEAR APPLY
    set PIXELS 0

    set img [gimp_image_new 256 256 $RGB]
    set b_size [scale $size 0.1]
    set b_size_2 [scale $size 0.05]
    set f_size [scale $size 0.075]
    set ds_size [scale $size 0.05]
    set ts_size [expr $b_size - 3]
    set text_layer [gimp_text $img -1 0 0 $text $b_size $TRUE $size \
			$PIXELS "*" $font "*" "*" "*" "*"]
    set width [gimp_drawable_width $text_layer]
    set height [gimp_drawable_height $text_layer]

    set blend_layer [gimp_layer_new $img $width $height $RGBA_IMAGE "Blend" \
			 100 $NORMAL]
    set shadow_layer [gimp_layer_new $img $width $height $RGBA_IMAGE "Shadow" \
			 100 $NORMAL]
    set text_shadow_layer [gimp_layer_new $img $width $height $RGBA_IMAGE \
			       "Text Shadow" 100 $MULTIPLY]
    set tsl_layer_mask [gimp_layer_create_mask $text_shadow_layer $BLACK_MASK]
    set drop_shadow_layer [gimp_layer_new $img $width $height $RGBA_IMAGE \
			       "Drop Shadow" 100 $MULTIPLY]
    set dsl_layer_mask [gimp_layer_create_mask $drop_shadow_layer $BLACK_MASK]
    set old_fg [gimp_palette_get_foreground]
    set old_bg [gimp_palette_get_background]

    gimp_image_disable_undo $img
    gimp_image_resize $img $width $height 0 0
    gimp_image_add_layer $img $shadow_layer 1
    gimp_image_add_layer $img $blend_layer 1
    gimp_image_add_layer $img $drop_shadow_layer 1
    gimp_image_add_layer $img $text_shadow_layer 0
    gimp_selection_all $img
    gimp_layer_set_preserve_trans $text_layer $TRUE
    gimp_bucket_fill $img $text_layer $PAT_BUCKET_FILL $NORMAL \
	100 0 $FALSE 0 0
    gimp_selection_none $img
    gimp_edit_clear $img $text_shadow_layer
    gimp_edit_clear $img $drop_shadow_layer
    gimp_palette_set_background $bg
    gimp_drawable_fill $shadow_layer $BG_IMAGE_FILL
    gimp_rect_select $img $b_size_2 $b_size_2 [expr $width - $b_size] \
	[expr $height - $b_size] $REPLACE $TRUE $b_size_2
    gimp_palette_set_background {0 0 0}
    gimp_edit_fill $img $shadow_layer
    gimp_selection_layer_alpha $img $text_layer
    gimp_image_add_layer_mask $img $text_shadow_layer $tsl_layer_mask

    gimp_palette_set_background {255 255 255}
    gimp_edit_fill $img $tsl_layer_mask
    gimp_selection_feather $img $f_size
    gimp_palette_set_background {63 63 63}
    gimp_edit_fill $img $drop_shadow_layer
    gimp_palette_set_background {0 0 0}
    gimp_edit_fill $img $text_shadow_layer
    gimp_palette_set_foreground {255 255 255}
    gimp_blend $img $text_shadow_layer $FG_BG_RGB NORMAL $SHAPEBURST_ANGULAR \
	100 0 0 0 1 1
    gimp_selection_none $img
    gimp_palette_set_foreground $blend_fg
    gimp_palette_set_background $blend_bg
    gimp_blend $img $blend_layer $FG_BG_RGB $NORMAL $LINEAR 100 0 0 0 $width 0
    plug_in_mosaic $img $blend_layer 12 1 1 0.7 135 0.2 $TRUE $FALSE \
	$tile_type 1 0
    gimp_layer_translate $text_layer -$b_size_2 -$b_size_2
    gimp_layer_translate $blend_layer -$b_size -$b_size
    gimp_layer_translate $text_shadow_layer -$ts_size -$ts_size
    gimp_layer_translate $drop_shadow_layer $ds_size $ds_size
    gimp_selection_layer_alpha $img $blend_layer
    gimp_image_add_layer_mask $img $drop_shadow_layer $dsl_layer_mask
    gimp_palette_set_background {255 255 255}
    gimp_edit_fill $img $dsl_layer_mask
    gimp_image_remove_layer_mask $img $drop_shadow_layer $APPLY
    gimp_selection_none $img
    gimp_layer_set_name $text_layer $text
    gimp_palette_set_foreground [lindex $old_fg 0]
    gimp_palette_set_background [lindex $old_bg 0]
    gimp_image_enable_undo $img
#    gimp_display_new $img
    return $img
}

proc bds-1 {text} {
    return [bds-logo {255 255 255} Fibers {32 106 0} {0 0 106} 0 $text \
		 200 courier]
}
