;  CHROME-LOGOS

(define (apply-chrome-logo-effect img
				  logo-layer
				  offsets
				  bg-color)
  (let* ((offx1 (* offsets 0.4))
	 (offy1 (* offsets 0.3))
	 (offx2 (* offsets (- 0.4)))
	 (offy2 (* offsets (- 0.3)))
	 (feather (* offsets 0.5))
	 (width (car (gimp-drawable-width logo-layer)))
	 (height (car (gimp-drawable-height logo-layer)))
	 (layer1 (car (gimp-layer-new img width height RGBA_IMAGE "Layer 1" 100 DIFFERENCE)))
	 (layer2 (car (gimp-layer-new img width height RGBA_IMAGE "Layer 2" 100 DIFFERENCE)))
	 (layer3 (car (gimp-layer-new img width height RGBA_IMAGE "Layer 3" 100 NORMAL)))
	 (shadow (car (gimp-layer-new img width height RGBA_IMAGE "Drop Shadow" 100 NORMAL)))
	 (background (car (gimp-layer-new img width height RGB_IMAGE "Background" 100 NORMAL)))
	 (layer-mask (car (gimp-layer-create-mask layer1 BLACK-MASK)))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background))))
    (gimp-image-resize img width height 0 0)
    (gimp-image-add-layer img background 1)
    (gimp-image-add-layer img shadow 1)
    (gimp-image-add-layer img layer3 1)
    (gimp-image-add-layer img layer2 1)
    (gimp-image-add-layer img layer1 1)
    (gimp-palette-set-background '(255 255 255))
    (gimp-selection-none img)
    (gimp-edit-fill layer1 BG-IMAGE-FILL)
    (gimp-edit-fill layer2 BG-IMAGE-FILL)
    (gimp-edit-fill layer3 BG-IMAGE-FILL)
    (gimp-edit-clear shadow)
    (gimp-selection-layer-alpha logo-layer)
    (gimp-layer-set-visible logo-layer FALSE)
    (gimp-layer-set-visible shadow FALSE)
    (gimp-layer-set-visible background FALSE)
    (gimp-palette-set-background '(0 0 0))
    (gimp-edit-fill layer1 BG-IMAGE-FILL)
    (gimp-selection-translate img offx1 offy1)
    (gimp-selection-feather img feather)
    (gimp-edit-fill layer2 BG-IMAGE-FILL)
    (gimp-selection-translate img (* 2 offx2) (* 2 offy2))
    (gimp-edit-fill layer3 BG-IMAGE-FILL)
    (gimp-selection-none img)
    (set! layer1 (car (gimp-image-merge-visible-layers img CLIP-TO-IMAGE)))
    (gimp-invert layer1)
    (gimp-image-add-layer-mask img layer1 layer-mask)
    (gimp-selection-layer-alpha logo-layer)
    (gimp-palette-set-background '(255 255 255))
    (gimp-selection-feather img feather)
    (gimp-edit-fill layer-mask BG-IMAGE-FILL)
    (gimp-palette-set-background '(0 0 0))
    (gimp-selection-translate img offx1 offy1)
    (gimp-edit-fill shadow BG-IMAGE-FILL)
    (gimp-selection-none img)
    (gimp-palette-set-background bg-color)
    (gimp-edit-fill background BG-IMAGE-FILL)
    (gimp-layer-set-visible shadow TRUE)
    (gimp-layer-set-visible background TRUE)
    (gimp-layer-set-name layer1 (car (gimp-layer-get-name logo-layer)))
    (gimp-image-remove-layer img logo-layer)
    (gimp-palette-set-foreground old-fg)
    (gimp-palette-set-background old-bg)))

(define (script-fu-chrome-logo-alpha img
				     logo-layer
				     offsets
				     bg-color)
  (begin
    (gimp-undo-push-group-start img)
    (apply-chrome-logo-effect img logo-layer offsets bg-color)
    (gimp-undo-push-group-end img)
    (gimp-displays-flush)))

(script-fu-register "script-fu-chrome-logo-alpha"
		    _"<Image>/Script-Fu/Alpha to Logo/Chrome..."
		    "Somewhat simplistic, but cool chromed logos"
		    "Spencer Kimball"
		    "Spencer Kimball & Peter Mattis"
		    "1997"
		    "RGBA"
                    SF-IMAGE      "Image" 0
                    SF-DRAWABLE   "Drawable" 0
		    SF-ADJUSTMENT _"Offsets (pixels * 2)" '(10 2 100 1 10 0 1)
		    SF-COLOR      _"Background Color" '(191 191 191)
		    )

(define (script-fu-chrome-logo text
			       size
			       font
			       bg-color)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
	 (b-size (* size 0.2))
	 (text-layer (car (gimp-text-fontname img -1 0 0 text b-size TRUE size PIXELS font))))
    (gimp-image-undo-disable img)
    (gimp-layer-set-name text-layer text)
    (apply-chrome-logo-effect img text-layer (* size 0.1) bg-color)
    (gimp-image-undo-enable img)
    (gimp-display-new img)))

(script-fu-register "script-fu-chrome-logo"
		    _"<Toolbox>/Xtns/Script-Fu/Logos/Chrome..."
		    "Somewhat simplistic, but cool chromed logos"
		    "Spencer Kimball"
		    "Spencer Kimball & Peter Mattis"
		    "1997"
		    ""
		    SF-STRING     _"Text" "The GIMP"
		    SF-ADJUSTMENT _"Font Size (pixels)" '(100 2 1000 1 10 0 1)
		    SF-FONT       _"Font" "-*-Bodoni-*-r-*-*-24-*-*-*-p-*-*-*"
		    SF-COLOR      _"Background Color" '(191 191 191)
		    )
