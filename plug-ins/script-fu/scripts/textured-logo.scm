;  BLENDED-DROP-SHADOW-LOGO
;  draw the specified text over a blended background using current gimp fg
;   and bg colors.  The finished blend has a drop shadow underneath that blends
;   to the specified bg-color
;  if the blend colors are specified as high intensity, the sharp option
;   should be enabled or the logo will come out blurry

(define (scale size percent) (* size percent))

(define (apply-textured-logo-effect img
				    logo-layer
				    b-size
				    text-pattern
				    tile-type
				    bg-color
				    blend-fg
				    blend-bg)
  (let* ((b-size-2 (scale b-size 0.5))
	 (f-size (scale b-size 0.75))
	 (ds-size (scale b-size 0.5))
	 (ts-size (- b-size-2 3))
	 (width (car (gimp-drawable-width logo-layer)))
	 (height (car (gimp-drawable-height logo-layer)))
	 (blend-layer (car (gimp-layer-new img width height RGBA_IMAGE "Blend" 100 NORMAL)))
	 (shadow-layer (car (gimp-layer-new img width height RGBA_IMAGE "Shadow" 100 NORMAL)))
	 (text-shadow-layer (car (gimp-layer-new img width height RGBA_IMAGE "Text Shadow" 100 MULTIPLY)))
	 (tsl-layer-mask (car (gimp-layer-create-mask text-shadow-layer BLACK-MASK)))
	 (drop-shadow-layer (car (gimp-layer-new img width height RGBA_IMAGE "Drop Shadow" 100 MULTIPLY)))
	 (dsl-layer-mask (car (gimp-layer-create-mask drop-shadow-layer BLACK-MASK)))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background)))
	 (old-pattern (car (gimp-patterns-get-pattern))))    
    (gimp-image-resize img width height 0 0)
    (gimp-image-add-layer img shadow-layer 1)
    (gimp-image-add-layer img blend-layer 1)
    (gimp-image-add-layer img drop-shadow-layer 1)
    (gimp-image-add-layer img text-shadow-layer 0)
    (gimp-selection-all img)
    (gimp-patterns-set-pattern text-pattern)
    (gimp-layer-set-preserve-trans logo-layer TRUE)
    (gimp-bucket-fill logo-layer PATTERN-BUCKET-FILL NORMAL 100 0 FALSE 0 0)
    (gimp-selection-none img)
    (gimp-edit-clear text-shadow-layer)
    (gimp-edit-clear drop-shadow-layer)
    (gimp-palette-set-background bg-color)
    (gimp-drawable-fill shadow-layer BG-IMAGE-FILL)
    (gimp-rect-select img b-size-2 b-size-2 (- width b-size) (- height b-size) REPLACE TRUE b-size-2)
    (gimp-palette-set-background '(0 0 0))
    (gimp-edit-fill shadow-layer BG-IMAGE-FILL)
    (gimp-selection-layer-alpha logo-layer)
    (gimp-image-add-layer-mask img text-shadow-layer tsl-layer-mask)
    (gimp-palette-set-background '(255 255 255))
    (gimp-edit-fill tsl-layer-mask BG-IMAGE-FILL)
    (gimp-selection-feather img f-size)
    (gimp-palette-set-background '(63 63 63))
    (gimp-edit-fill drop-shadow-layer BG-IMAGE-FILL)
    (gimp-palette-set-background '(0 0 0))
    (gimp-edit-fill text-shadow-layer BG-IMAGE-FILL)
    (gimp-palette-set-foreground '(255 255 255))
    (gimp-blend text-shadow-layer FG-BG-RGB NORMAL SHAPEBURST-ANGULAR 100 0 REPEAT-NONE FALSE 0 0 0 0 1 1)
    (gimp-selection-none img)
    (gimp-palette-set-foreground blend-fg)
    (gimp-palette-set-background blend-bg)
    (gimp-blend blend-layer FG-BG-RGB NORMAL LINEAR 100 0 REPEAT-NONE FALSE 0 0 0 0 width 0)
    (plug-in-mosaic 1 img blend-layer 12 1 1 0.7 135 0.2 TRUE FALSE tile-type 1 0)
    (gimp-layer-translate logo-layer (- b-size-2) (- b-size-2))
    (gimp-layer-translate blend-layer (- b-size) (- b-size))
    (gimp-layer-translate text-shadow-layer (- ts-size) (- ts-size))
    (gimp-layer-translate drop-shadow-layer ds-size ds-size)
    (gimp-selection-layer-alpha blend-layer)
    (gimp-image-add-layer-mask img drop-shadow-layer dsl-layer-mask)
    (gimp-palette-set-background '(255 255 255))
    (gimp-edit-fill dsl-layer-mask BG-IMAGE-FILL)
    (gimp-image-remove-layer-mask img drop-shadow-layer APPLY)
    (gimp-selection-none img)
    (gimp-patterns-set-pattern old-pattern)
    (gimp-palette-set-foreground old-fg)
    (gimp-palette-set-background old-bg)))

(define (script-fu-textured-logo-alpha img
				       logo-layer
				       b-size
				       text-pattern
				       tile-type
				       bg-color
				       blend-fg
				       blend-bg)
  (begin
    (gimp-undo-push-group-start img)
    (apply-textured-logo-effect img logo-layer b-size text-pattern tile-type
				bg-color blend-fg blend-bg)
    (gimp-undo-push-group-end img)
    (gimp-displays-flush)))

(script-fu-register "script-fu-textured-logo-alpha"
		    _"<Image>/Script-Fu/Alpha to Logo/Textured..."
		    "Creates textured logos with blended backgrounds, highlights, and shadows"
		    "Spencer Kimball"
		    "Spencer Kimball"
		    "1996"
		    "RGBA"
                    SF-IMAGE      "Image" 0
                    SF-DRAWABLE   "Drawable" 0
		    SF-ADJUSTMENT _"Border Size (pixels)" '(20 1 100 1 10 0 1)
		    SF-PATTERN    _"Pattern" "Fibers"
		    SF-OPTION     _"Mosaic Tile Type" '(_"Squares" "Hexagons" "Octagons")
		    SF-COLOR      _"Background Color" '(255 255 255)
		    SF-COLOR      _"Starting Blend" '(32 106 0)
		    SF-COLOR      _"Ending Blend" '(0 0 106)
		    )

(define (script-fu-textured-logo text
				 size
				 fontname
				 text-pattern
				 tile-type
				 bg-color
				 blend-fg
				 blend-bg)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
	 (b-size (scale size 0.1))
	 (text-layer (car (gimp-text-fontname img -1 0 0 text b-size TRUE size PIXELS fontname))))
    (gimp-image-undo-disable img)
    (gimp-layer-set-name text-layer text)
    (apply-textured-logo-effect img text-layer b-size text-pattern tile-type
				bg-color blend-fg blend-bg)
    (gimp-image-undo-enable img)
    (gimp-display-new img)))

(script-fu-register "script-fu-textured-logo"
		    _"<Toolbox>/Xtns/Script-Fu/Logos/Textured..."
		    "Creates textured logos with blended backgrounds, highlights, and shadows"
		    "Spencer Kimball"
		    "Spencer Kimball"
		    "1996"
		    ""
		    SF-STRING     _"Text" "The GIMP"
		    SF-ADJUSTMENT _"Font Size (pixels)" '(200 1 1000 1 10 0 1)
		    SF-FONT       _"Font" "-*-cuneifontlight-*-r-*-*-24-*-*-*-p-*-*-*"
		    SF-PATTERN    _"Text Pattern" "Fibers"
		    SF-OPTION     _"Mosaic Tile Type" '(_"Squares" "Hexagons" "Octagons")
		    SF-COLOR      _"Background Color" '(255 255 255)
		    SF-COLOR      _"Starting Blend" '(32 106 0)
		    SF-COLOR      _"Ending Blend" '(0 0 106)
		    )
