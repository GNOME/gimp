;  Starburst
;    Effect courtesy Xach Beane's web page


(define (script-fu-starburst-logo text size fontname burst-color bg-color)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
	 (off (* size 0.03))
	 (count -1)
	 (feather (* size 0.04))
	 (border (+ feather off))
	 (text-layer (car (gimp-text-fontname img -1 0 0 text border TRUE size PIXELS fontname)))
	 (width (car (gimp-drawable-width text-layer)))
	 (height (car (gimp-drawable-height text-layer)))
	 (burst-coords (cons (* width 0.5) (* height 0.5)))
	 (burstradius (* (min height width) 0.35))
	 (bg-layer (car (gimp-layer-new img width height RGB_IMAGE "Background" 100 NORMAL)))
	 (shadow-layer (car (gimp-layer-new img width height RGBA_IMAGE "Shadow" 100 NORMAL)))
	 (burst-layer (car (gimp-layer-new img width height RGBA_IMAGE "Burst" 100 NORMAL)))
	 (layer-mask (car (gimp-layer-create-mask burst-layer BLACK-MASK)))
	 (old-pattern (car (gimp-patterns-get-pattern)))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background))))

    (gimp-image-disable-undo img)
    (gimp-image-resize img width height 0 0)
    (gimp-image-add-layer img bg-layer 1)
    (gimp-image-add-layer img shadow-layer 1)
    (gimp-image-add-layer img burst-layer 0)
    (gimp-image-add-layer-mask img burst-layer layer-mask)
    (gimp-layer-set-preserve-trans text-layer TRUE)

    (gimp-palette-set-background bg-color)
    (gimp-edit-fill img bg-layer)
    (gimp-edit-clear img shadow-layer)
    (gimp-edit-clear img burst-layer)

    (gimp-selection-all img)
    (gimp-patterns-set-pattern "Crack")
    (gimp-bucket-fill img text-layer PATTERN-BUCKET-FILL NORMAL 100 0 FALSE 0 0)
    (gimp-selection-none img)

    (gimp-selection-layer-alpha img text-layer)

    (gimp-palette-set-background '(255 255 255))
    (gimp-edit-fill img layer-mask)
    (gimp-selection-none img)
    (plug-in-nova 1 img burst-layer (car burst-coords) (cdr burst-coords)
		  burst-color burstradius 100)

    (gimp-selection-layer-alpha img text-layer)
    (gimp-palette-set-background '(0 0 0))
    (gimp-selection-feather img feather)
    (gimp-selection-translate img -1 -1)
    (while (< count off)
	   (gimp-edit-fill img shadow-layer)
	   (gimp-selection-translate img 1 1)
	   (set! count (+ count 1)))
    (gimp-selection-none img)

    (gimp-layer-set-name text-layer text)
    (gimp-patterns-set-pattern old-pattern)
    (gimp-palette-set-background old-bg)
    (gimp-palette-set-foreground old-fg)
    (gimp-image-enable-undo img)
    (gimp-display-new img)))


(script-fu-register "script-fu-starburst-logo"
		    "<Toolbox>/Xtns/Script-Fu/Logos/Starburst"
		    "Starburst as inspired by GIMP News"
		    "Spencer Kimball & Xach Beane"
		    "Spencer Kimball & Xach Beane"
		    "1997"
		    ""
		    SF-STRING "Text String" "GIMP"
;		    SF-ADJUSTMENT "Font Size (pixels)" '(150 2 1000 1 10 0 1)
		    SF-ADJUSTMENT "Font Size (pixels)" '(150 0 512 1 10 0 1)
;		    SF-FONT "Font" "-*-Blippo-*-r-*-*-24-*-*-*-p-*-*-*"
		    SF-FONT "Font" "-*-blippo-*-r-*-*-24-*-*-*-p-*-*-*"
		    SF-COLOR "Burst Color" '(60 196 33)
		    SF-COLOR "BG Color" '(255 255 255))
