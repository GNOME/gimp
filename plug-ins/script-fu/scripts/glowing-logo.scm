;  GLOWING
;  Create a text effect that simulates a glowing hot logo

(define (apply-glowing-logo-effect img
				   logo-layer
				   size
				   bg-color)
  (let* ((grow (/ size 4))
	 (feather1 (/ size 3))
	 (feather2 (/ size 7))
	 (feather3 (/ size 10))
	 (width (car (gimp-drawable-width logo-layer)))
	 (height (car (gimp-drawable-height logo-layer)))
	 (glow-layer (car (gimp-layer-copy logo-layer TRUE)))
	 (bg-layer (car (gimp-layer-new img width height RGB_IMAGE "Background" 100 NORMAL)))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background))))
    (gimp-image-resize img width height 0 0)
    (gimp-image-add-layer img bg-layer 1)
    (gimp-image-add-layer img glow-layer 1)

    (gimp-selection-none img)
    (gimp-palette-set-background bg-color)
    (gimp-edit-fill bg-layer BG-IMAGE-FILL)

    (gimp-layer-set-preserve-trans logo-layer TRUE)
    (gimp-palette-set-background '(0 0 0))
    (gimp-edit-fill logo-layer BG-IMAGE-FILL)

    (gimp-selection-layer-alpha logo-layer)
    (gimp-selection-feather img feather1)
    (gimp-palette-set-background '(221 0 0))
    (gimp-edit-fill glow-layer BG-IMAGE-FILL)
    (gimp-edit-fill glow-layer BG-IMAGE-FILL)
    (gimp-edit-fill glow-layer BG-IMAGE-FILL)

    (gimp-selection-layer-alpha logo-layer)
    (gimp-selection-feather img feather2)
    (gimp-palette-set-background '(232 217 18))
    (gimp-edit-fill glow-layer BG-IMAGE-FILL)
    (gimp-edit-fill glow-layer BG-IMAGE-FILL)

    (gimp-selection-layer-alpha logo-layer)
    (gimp-selection-feather img feather3)
    (gimp-palette-set-background '(255 255 255))
    (gimp-edit-fill glow-layer BG-IMAGE-FILL)
    (gimp-selection-none img)

    (gimp-layer-set-mode logo-layer OVERLAY)
    (gimp-layer-set-name glow-layer "Glow Layer")

    (gimp-palette-set-background old-bg)
    (gimp-palette-set-foreground old-fg)))


(define (script-fu-glowing-logo-alpha img
				      logo-layer
				      size
				      bg-color)
  (begin
    (gimp-undo-push-group-start img)
    (apply-glowing-logo-effect img logo-layer size bg-color)
    (gimp-undo-push-group-end img)
    (gimp-displays-flush)))

(script-fu-register "script-fu-glowing-logo-alpha"
		    _"<Image>/Script-Fu/Alpha to Logo/Glowing Hot..."
		    "Glowing hot logos"
		    "Spencer Kimball"
		    "Spencer Kimball"
		    "1997"
		    "RGBA"
                    SF-IMAGE      "Image" 0
                    SF-DRAWABLE   "Drawable" 0
		    SF-ADJUSTMENT _"Effect Size (pixels * 3)" '(150 2 1000 1 10 0 1)
		    SF-COLOR      _"Background Color" '(7 0 20)
		    )

(define (script-fu-glowing-logo text
				size
				font
				bg-color)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
	 (border (/ size 4))
	 (text-layer (car (gimp-text-fontname img -1 0 0 text border TRUE size PIXELS font))))
    (gimp-image-undo-disable img)
    (apply-glowing-logo-effect img text-layer size bg-color)
    (gimp-image-undo-enable img)
    (gimp-display-new img)))

(script-fu-register "script-fu-glowing-logo"
		    _"<Toolbox>/Xtns/Script-Fu/Logos/Glowing Hot..."
		    "Glowing hot logos"
		    "Spencer Kimball"
		    "Spencer Kimball"
		    "1997"
		    ""
		    SF-STRING     _"Text" "GLOWING"
		    SF-ADJUSTMENT _"Font Size (pixels)" '(150 2 1000 1 10 0 1)
		    SF-FONT       _"Font" "-*-Slogan-*-r-*-*-24-*-*-*-p-*-*-*"
		    SF-COLOR      _"Background Color" '(7 0 20)
		    )
