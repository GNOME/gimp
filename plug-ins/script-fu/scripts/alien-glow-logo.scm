;  ALIEN-GLOW
;  Create a text effect that simulates an eerie alien glow around text

(define (script-fu-alien-glow-logo text size font glow-color)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
	 (border (/ size 4))
	 (grow (/ size 30))
	 (feather (/ size 4))
	 (text-layer (car (gimp-text-fontname img -1 0 0 text border TRUE size PIXELS font)))
	 (width (car (gimp-drawable-width text-layer)))
	 (height (car (gimp-drawable-height text-layer)))
	 (bg-layer (car (gimp-layer-new img width height RGB_IMAGE "Background" 100 NORMAL)))
	 (glow-layer (car (gimp-layer-new img width height RGBA_IMAGE "Alien Glow" 100 NORMAL)))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background))))
    (gimp-image-undo-disable img)
    (gimp-image-resize img width height 0 0)
    (gimp-image-add-layer img bg-layer 1)
    (gimp-image-add-layer img glow-layer 1)
    (gimp-layer-set-preserve-trans text-layer TRUE)
    (gimp-palette-set-background '(0 0 0))
    (gimp-edit-fill bg-layer)
    (gimp-edit-clear glow-layer)
    (gimp-selection-layer-alpha text-layer)
    (gimp-selection-grow img grow)
    (gimp-selection-feather img feather)
    (gimp-palette-set-background glow-color)
    (gimp-edit-fill glow-layer)
    (gimp-selection-none img)
    (gimp-palette-set-background '(0 0 0))
    (gimp-palette-set-foreground '(79 79 79))
    (gimp-blend text-layer FG-BG-RGB NORMAL SHAPEBURST-ANGULAR 100 0 REPEAT-NONE FALSE 0 0 0 0 1 1)
    (gimp-layer-set-name text-layer text)
    (gimp-palette-set-background old-bg)
    (gimp-palette-set-foreground old-fg)
    (gimp-image-undo-enable img)
    (gimp-display-new img)))


(script-fu-register "script-fu-alien-glow-logo"
		    "<Toolbox>/Xtns/Script-Fu/Logos/Alien Glow..."
		    "Create an X-Files-esque logo with the specified glow color"
		    "Spencer Kimball"
		    "Spencer Kimball"
		    "1997"
		    ""
		    SF-STRING "Text String" "ALIEN"
		    SF-ADJUSTMENT "Font Size (pixels)" '(150 2 1000 1 10 0 1)
		    SF-FONT "Font" "-*-futura_poster-*-r-*-*-24-*-*-*-p-*-*-*"
		    SF-COLOR "Glow Color" '(63 252 0))
