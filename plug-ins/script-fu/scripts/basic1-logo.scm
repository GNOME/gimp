;  DROP-SHADOW-LOGO
;  draw the specified text over a background with a drop shadow

(define (apply-basic1-logo-effect img
				  logo-layer
				  bg-color
				  text-color)
  (let* ((width (car (gimp-drawable-width logo-layer)))
	 (height (car (gimp-drawable-height logo-layer)))
	 (bg-layer (car (gimp-layer-new img width height RGBA_IMAGE "Background" 100 NORMAL)))
	 (shadow-layer (car (gimp-layer-new img width height RGBA_IMAGE "Shadow" 100 MULTIPLY)))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background))))
    (gimp-image-resize img width height 0 0)
    (gimp-image-add-layer img shadow-layer 1)
    (gimp-image-add-layer img bg-layer 2)
    (gimp-palette-set-foreground text-color)
    (gimp-layer-set-preserve-trans logo-layer TRUE)
    (gimp-edit-fill logo-layer FG-IMAGE-FILL)
    (gimp-palette-set-background bg-color)
    (gimp-edit-fill bg-layer BG-IMAGE-FILL)
    (gimp-edit-clear shadow-layer)
    (gimp-selection-layer-alpha logo-layer)
    (gimp-palette-set-background '(0 0 0))
    (gimp-selection-feather img 7.5)
    (gimp-edit-fill shadow-layer BG-IMAGE-FILL)
    (gimp-selection-none img)
    (gimp-palette-set-foreground '(255 255 255))
    (gimp-blend logo-layer FG-BG-RGB MULTIPLY RADIAL 100 20 REPEAT-NONE FALSE 0 0 0 0 width height)
    (gimp-layer-translate shadow-layer 3 3)
    (gimp-palette-set-background old-bg)
    (gimp-palette-set-foreground old-fg)))

(define (script-fu-basic1-logo-alpha img
				     logo-layer
				     bg-color
				     text-color)
  (begin
    (gimp-undo-push-group-start img)
    (apply-basic1-logo-effect img logo-layer bg-color text-color)
    (gimp-undo-push-group-end img)
    (gimp-displays-flush)))

(script-fu-register "script-fu-basic1-logo-alpha"
		    _"<Image>/Script-Fu/Alpha to Logo/Basic I..."
		    "Creates a simple logo with a drop shadow"
		    "Spencer Kimball"
		    "Spencer Kimball"
		    "1996"
		    "RGBA"
                    SF-IMAGE      "Image" 0
                    SF-DRAWABLE   "Drawable" 0
		    SF-COLOR      _"Background Color" '(255 255 255)
		    SF-COLOR      _"Text Color" '(6 6 206))

(define (script-fu-basic1-logo text
			       size
			       font
			       bg-color
			       text-color)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
	 (text-layer (car (gimp-text-fontname img -1 0 0 text 10 TRUE size PIXELS font))))
    (gimp-image-undo-disable img)
    (gimp-layer-set-name text-layer text)
    (apply-basic1-logo-effect img text-layer bg-color text-color)
    (gimp-image-undo-enable img)
    (gimp-display-new img)))

(script-fu-register "script-fu-basic1-logo"
		    _"<Toolbox>/Xtns/Script-Fu/Logos/Basic I..."
		    "Creates a simple logo with a drop shadow"
		    "Spencer Kimball"
		    "Spencer Kimball"
		    "1996"
		    ""
		    SF-STRING     _"Text" "The Gimp"
		    SF-ADJUSTMENT _"Font Size (pixels)" '(100 2 1000 1 10 0 1)
		    SF-FONT       _"Font" "-*-Dragonwick-*-r-*-*-24-*-*-*-p-*-*-*"
		    SF-COLOR      _"Background Color" '(255 255 255)
		    SF-COLOR      _"Text Color" '(6 6 206))
