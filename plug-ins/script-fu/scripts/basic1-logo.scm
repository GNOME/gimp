;  DROP-SHADOW-LOGO
;  draw the specified text over a background with a drop shadow

(define (script-fu-basic1-logo text size font bg-color text-color)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
	 (text-layer (car (gimp-text img -1 0 0 text 10 TRUE size PIXELS "*" font "*" "*" "*" "*")))
	 (width (car (gimp-drawable-width text-layer)))
	 (height (car (gimp-drawable-height text-layer)))
	 (bg-layer (car (gimp-layer-new img width height RGB_IMAGE "Background" 100 NORMAL)))
	 (shadow-layer (car (gimp-layer-new img width height RGBA_IMAGE "Shadow" 100 MULTIPLY)))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background))))
    (gimp-image-disable-undo img)
    (gimp-image-resize img width height 0 0)
    (gimp-image-add-layer img shadow-layer 1)
    (gimp-image-add-layer img bg-layer 2)
    (gimp-palette-set-background text-color)
    (gimp-layer-set-preserve-trans text-layer TRUE)
    (gimp-edit-fill img text-layer)
    (gimp-palette-set-background bg-color)
    (gimp-edit-fill img bg-layer)
    (gimp-edit-clear img shadow-layer)
    (gimp-selection-layer-alpha img text-layer)
    (gimp-palette-set-background '(0 0 0))
    (gimp-selection-feather img 7.5)
    (gimp-edit-fill img shadow-layer)
    (gimp-selection-none img)
    (gimp-palette-set-foreground '(255 255 255))
    (gimp-blend img text-layer FG-BG-RGB MULTIPLY RADIAL 100 20 REPEAT-NONE FALSE 0 0 0 0 width height)
    (gimp-layer-translate shadow-layer 3 3)
    (gimp-layer-set-name text-layer text)
    (gimp-palette-set-background old-bg)
    (gimp-palette-set-foreground old-fg)
    (gimp-image-enable-undo img)
    (gimp-display-new img)))

(script-fu-register "script-fu-basic1-logo"
		    "<Toolbox>/Xtns/Script-Fu/Logos/Basic I"
		    "Creates a simple logo with a drop shadow"
		    "Spencer Kimball"
		    "Spencer Kimball"
		    "1996"
		    ""
		    SF-STRING "Text String" "The Gimp"
		    SF-VALUE "Font Size (in pixels)" "100"
		    SF-STRING "Font" "Dragonwick"
		    SF-COLOR "Background Color" '(255 255 255)
		    SF-COLOR "Text Color" '(6 6 206))
