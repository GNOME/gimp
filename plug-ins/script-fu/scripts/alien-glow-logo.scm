;  ALIEN-GLOW
;  Create a text effect that simulates an eerie alien glow around text

(define (apply-alien-glow-logo-effect img
				      logo-layer
				      size
				      glow-color)
  (let* ((border (/ size 4))
	 (grow (/ size 30))
	 (feather (/ size 4))
	 (width (car (gimp-drawable-width logo-layer)))
	 (height (car (gimp-drawable-height logo-layer)))
	 (bg-layer (car (gimp-layer-new img width height RGB_IMAGE "Background" 100 NORMAL)))
	 (glow-layer (car (gimp-layer-new img width height RGBA_IMAGE "Alien Glow" 100 NORMAL)))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background))))
    (gimp-image-resize img width height 0 0)
    (gimp-image-add-layer img bg-layer 1)
    (gimp-image-add-layer img glow-layer 1)
    (gimp-layer-set-preserve-trans logo-layer TRUE)
    (gimp-palette-set-background '(0 0 0))
    (gimp-edit-fill bg-layer BG-IMAGE-FILL)
    (gimp-edit-clear glow-layer)
    (gimp-selection-layer-alpha logo-layer)
    (gimp-selection-grow img grow)
    (gimp-selection-feather img feather)
    (gimp-palette-set-foreground glow-color)
    (gimp-edit-fill glow-layer FG-IMAGE-FILL)
    (gimp-selection-none img)
    (gimp-palette-set-background '(0 0 0))
    (gimp-palette-set-foreground '(79 79 79))
    (gimp-blend logo-layer FG-BG-RGB NORMAL SHAPEBURST-ANGULAR 100 0 REPEAT-NONE FALSE 0 0 0 0 1 1)
    (gimp-palette-set-background old-bg)
    (gimp-palette-set-foreground old-fg)))

(define (script-fu-alien-glow-logo-alpha img
					 logo-layer
					 size
					 glow-color)
  (begin
    (gimp-undo-push-group-start img)
    (apply-alien-glow-logo-effect img logo-layer size glow-color)
    (gimp-undo-push-group-end img)
    (gimp-displays-flush)))

(script-fu-register "script-fu-alien-glow-logo-alpha"
		    _"<Image>/Script-Fu/Alpha to Logo/Alien Glow..."
		    "Create an X-Files-esque logo with the specified glow color"
		    "Spencer Kimball"
		    "Spencer Kimball"
		    "1997"
		    "RGBA"
                    SF-IMAGE      "Image" 0
                    SF-DRAWABLE   "Drawable" 0
		    SF-ADJUSTMENT _"Glow Size (pixels * 4)" '(150 2 1000 1 10 0 1)
		    SF-COLOR      _"Glow Color" '(63 252 0))

(define (script-fu-alien-glow-logo text
				   size
				   font
				   glow-color)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
	 (border (/ size 4))
	 (grow (/ size 30))
	 (feather (/ size 4))
	 (text-layer (car (gimp-text-fontname img -1 0 0 text border TRUE size PIXELS font)))
	 (width (car (gimp-drawable-width text-layer)))
	 (height (car (gimp-drawable-height text-layer)))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background))))
    (gimp-image-undo-disable img)
    (gimp-layer-set-name text-layer text)
    (apply-alien-glow-logo-effect img text-layer size glow-color)
    (gimp-image-undo-enable img)
    (gimp-display-new img)))

(script-fu-register "script-fu-alien-glow-logo"
		    _"<Toolbox>/Xtns/Script-Fu/Logos/Alien Glow..."
		    "Create an X-Files-esque logo with the specified glow color"
		    "Spencer Kimball"
		    "Spencer Kimball"
		    "1997"
		    ""
		    SF-STRING     _"Text" "ALIEN"
		    SF-ADJUSTMENT _"Font Size (pixels)" '(150 2 1000 1 10 0 1)
		    SF-FONT       _"Font" "-*-futura_poster-*-r-*-*-24-*-*-*-p-*-*-*"
		    SF-COLOR      _"Glow Color" '(63 252 0))
