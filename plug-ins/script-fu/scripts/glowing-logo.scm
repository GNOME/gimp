;  GLOWING
;  Create a text effect that simulates a glowing hot logo

(define (script-fu-glowing-logo text size font bg-color)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
	 (border (/ size 4))
	 (grow (/ size 4))
	 (feather1 (/ size 3))
	 (feather2 (/ size 7))
	 (feather3 (/ size 10))
	 (text-layer (car (gimp-text-fontname img -1 0 0 text border TRUE size PIXELS font)))
	 (width (car (gimp-drawable-width text-layer)))
	 (height (car (gimp-drawable-height text-layer)))
	 (glow-layer (car (gimp-layer-copy text-layer TRUE)))
	 (bg-layer (car (gimp-layer-new img width height RGB_IMAGE "Background" 100 NORMAL)))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background))))
    (gimp-image-disable-undo img)
    (gimp-image-resize img width height 0 0)
    (gimp-image-add-layer img bg-layer 1)
    (gimp-image-add-layer img glow-layer 1)

    (gimp-selection-none img)
    (gimp-palette-set-background bg-color)
    (gimp-edit-fill bg-layer)

    (gimp-layer-set-preserve-trans text-layer TRUE)
    (gimp-palette-set-background '(0 0 0))
    (gimp-edit-fill text-layer)

    (gimp-selection-layer-alpha text-layer)
    (gimp-selection-feather img feather1)
    (gimp-palette-set-background '(221 0 0))
    (gimp-edit-fill glow-layer)
    (gimp-edit-fill glow-layer)
    (gimp-edit-fill glow-layer)

    (gimp-selection-layer-alpha text-layer)
    (gimp-selection-feather img feather2)
    (gimp-palette-set-background '(232 217 18))
    (gimp-edit-fill glow-layer)
    (gimp-edit-fill glow-layer)

    (gimp-selection-layer-alpha text-layer)
    (gimp-selection-feather img feather3)
    (gimp-palette-set-background '(255 255 255))
    (gimp-edit-fill glow-layer)
    (gimp-selection-none img)

    (gimp-layer-set-name text-layer text)
    (gimp-layer-set-mode text-layer OVERLAY)
    (gimp-layer-set-name glow-layer "Glow Layer")

    (gimp-palette-set-background old-bg)
    (gimp-palette-set-foreground old-fg)
    (gimp-image-enable-undo img)
    (gimp-display-new img)))


(script-fu-register "script-fu-glowing-logo"
		    "<Toolbox>/Xtns/Script-Fu/Logos/Glowing Hot"
		    "Glowing hot logos"
		    "Spencer Kimball"
		    "Spencer Kimball"
		    "1997"
		    ""
		    SF-STRING "Text String" "GLOWING"
		    SF-ADJUSTMENT "Font Size (pixels)" '(150 2 1000 1 10 0 1)
		    SF-FONT "Font" "-*-Slogan-*-r-*-*-24-*-*-*-p-*-*-*"
		    SF-COLOR "Background Color" '(7 0 20))
