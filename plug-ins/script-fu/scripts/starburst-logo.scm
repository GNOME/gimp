;  Starburst
;    Effect courtesy Xach Beane's web page


(define (apply-starburst-logo-effect img
				     logo-layer
				     size
				     burst-color
				     bg-color)
  (let* ((off (* size 0.03))
	 (count -1)
	 (feather (* size 0.04))
	 (border (+ feather off))
	 (width (car (gimp-drawable-width logo-layer)))
	 (height (car (gimp-drawable-height logo-layer)))
	 (burst-coords (cons (* width 0.5) (* height 0.5)))
	 (burstradius (* (min height width) 0.35))
	 (bg-layer (car (gimp-layer-new img width height RGB-IMAGE "Background" 100 NORMAL-MODE)))
	 (shadow-layer (car (gimp-layer-new img width height RGBA-IMAGE "Shadow" 100 NORMAL-MODE)))
	 (burst-layer (car (gimp-layer-new img width height RGBA-IMAGE "Burst" 100 NORMAL-MODE)))
	 (layer-mask (car (gimp-layer-create-mask burst-layer ADD-BLACK-MASK)))
	 (old-pattern (car (gimp-patterns-get-pattern)))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background))))

    (gimp-selection-none img)
    (script-fu-util-image-resize-from-layer img logo-layer)
    (gimp-image-add-layer img bg-layer 1)
    (gimp-image-add-layer img shadow-layer 1)
    (gimp-image-add-layer img burst-layer 0)
    (gimp-layer-add-mask burst-layer layer-mask)
    (gimp-layer-set-preserve-trans logo-layer TRUE)

    (gimp-palette-set-background bg-color)
    (gimp-edit-fill bg-layer BACKGROUND-FILL)
    (gimp-edit-clear shadow-layer)
    (gimp-edit-clear burst-layer)

    (gimp-selection-all img)
    (gimp-patterns-set-pattern "Crack")
    (gimp-bucket-fill logo-layer PATTERN-BUCKET-FILL NORMAL-MODE 100 0 FALSE 0 0)
    (gimp-selection-none img)

    (gimp-selection-layer-alpha logo-layer)

    (gimp-palette-set-background '(255 255 255))
    (gimp-edit-fill layer-mask BACKGROUND-FILL)
    (gimp-selection-none img)
    (plug-in-nova 1 img burst-layer (car burst-coords) (cdr burst-coords)
		  burst-color burstradius 100 0)

    (gimp-selection-layer-alpha logo-layer)
    (gimp-palette-set-background '(0 0 0))
    (gimp-selection-feather img feather)
    (gimp-selection-translate img -1 -1)
    (while (< count off)
	   (gimp-edit-fill shadow-layer BACKGROUND-FILL)
	   (gimp-selection-translate img 1 1)
	   (set! count (+ count 1)))
    (gimp-selection-none img)

    (gimp-patterns-set-pattern old-pattern)
    (gimp-palette-set-background old-bg)
    (gimp-palette-set-foreground old-fg)))


(define (script-fu-starburst-logo-alpha img
					logo-layer
					size
					burst-color
					bg-color)
  (begin
    (gimp-image-undo-group-start img)
    (apply-starburst-logo-effect img logo-layer size burst-color bg-color)
    (gimp-image-undo-group-end img)
    (gimp-displays-flush)))

(script-fu-register "script-fu-starburst-logo-alpha"
		    _"<Image>/Script-Fu/Alpha to Logo/Starb_urst..."
		    "Starburst as inspired by GIMP News"
		    "Spencer Kimball & Xach Beane"
		    "Spencer Kimball & Xach Beane"
		    "1997"
		    "RGBA"
                    SF-IMAGE      "Image" 0
                    SF-DRAWABLE   "Drawable" 0
		    SF-ADJUSTMENT _"Effect Size (pixels * 30)" '(150 0 512 1 10 0 1)
		    SF-COLOR      _"Burst Color" '(60 196 33)
		    SF-COLOR      _"Background Color" '(255 255 255)
		    )


(define (script-fu-starburst-logo text size fontname burst-color bg-color)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
	 (off (* size 0.03))
	 (feather (* size 0.04))
	 (border (+ feather off))
	 (text-layer (car (gimp-text-fontname img -1 0 0 text border TRUE size PIXELS fontname))))
    (gimp-image-undo-disable img)
    (gimp-drawable-set-name text-layer text)
    (apply-starburst-logo-effect img text-layer size burst-color bg-color)
    (gimp-image-undo-enable img)
    (gimp-display-new img)))

(script-fu-register "script-fu-starburst-logo"
		    _"<Toolbox>/Xtns/Script-Fu/Logos/Starb_urst..."
		    "Starburst as inspired by GIMP News"
		    "Spencer Kimball & Xach Beane"
		    "Spencer Kimball & Xach Beane"
		    "1997"
		    ""
		    SF-STRING     _"Text" "GIMP"
		    SF-ADJUSTMENT _"Font Size (pixels)" '(150 0 512 1 10 0 1)
		    SF-FONT       _"Font" "Blippo"
		    SF-COLOR      _"Burst Color" '(60 196 33)
		    SF-COLOR      _"Background Color" '(255 255 255)
		    )
