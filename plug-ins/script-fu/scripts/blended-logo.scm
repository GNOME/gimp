;  BLENDED-DROP-SHADOW-LOGO
;  draw the specified text over a blended background using current gimp fg
;   and bg colors.  The finished blend has a drop shadow underneath that blends
;   to the specified bg-color
;  if the blend colors are specified as high intensity, the sharp option
;   should be enabled or the logo will come out blurry

(define (scale size percent) (* size percent))

(define (script-fu-blended-logo blend-mode text size font bg-color text-color blend-fg blend-bg)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
	 (b-size (scale size 0.1))
	 (b-size-2 (scale size 0.05))
	 (f-size (scale size 0.075))
	 (ds-size (scale size 0.05))
	 (ts-size (- b-size-2 3))
	 (text-layer (car (gimp-text-fontname img -1 0 0 text b-size TRUE size PIXELS font)))
	 (width (car (gimp-drawable-width text-layer)))
	 (height (car (gimp-drawable-height text-layer)))
	 (blend-layer (car (gimp-layer-new img width height RGBA_IMAGE "Blend" 100 NORMAL)))
	 (shadow-layer (car (gimp-layer-new img width height RGBA_IMAGE "Shadow" 100 NORMAL)))
	 (text-shadow-layer (car (gimp-layer-new img width height RGBA_IMAGE "Text Shadow" 100 MULTIPLY)))
	 (tsl-layer-mask (car (gimp-layer-create-mask text-shadow-layer BLACK-MASK)))
	 (drop-shadow-layer (car (gimp-layer-new img width height RGBA_IMAGE "Drop Shadow" 100 MULTIPLY)))
	 (dsl-layer-mask (car (gimp-layer-create-mask drop-shadow-layer BLACK-MASK)))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background))))
    (gimp-image-disable-undo img)
    (gimp-image-resize img width height 0 0)
    (gimp-image-add-layer img shadow-layer 1)
    (gimp-image-add-layer img blend-layer 1)
    (gimp-image-add-layer img drop-shadow-layer 1)
    (gimp-image-add-layer img text-shadow-layer 0)
    (gimp-selection-none img)
    (gimp-edit-clear text-shadow-layer)
    (gimp-edit-clear drop-shadow-layer)
    (gimp-palette-set-background text-color)
    (gimp-layer-set-preserve-trans text-layer TRUE)
    (gimp-edit-fill text-layer)
    (gimp-palette-set-background bg-color)
    (gimp-drawable-fill shadow-layer BG-IMAGE-FILL)
    (gimp-rect-select img b-size-2 b-size-2 (- width b-size) (- height b-size) REPLACE TRUE b-size-2)
    (gimp-palette-set-background '(0 0 0))
    (gimp-edit-fill shadow-layer)
    (gimp-selection-layer-alpha text-layer)
    (gimp-image-add-layer-mask img text-shadow-layer tsl-layer-mask)
    (gimp-palette-set-background '(255 255 255))
    (gimp-edit-fill tsl-layer-mask)
    (gimp-selection-feather img f-size)
    (gimp-palette-set-background '(63 63 63))
    (gimp-edit-fill drop-shadow-layer)
    (gimp-palette-set-background '(0 0 0))
    (gimp-edit-fill text-shadow-layer)
    (gimp-palette-set-foreground '(255 255 255))
    (gimp-blend text-shadow-layer FG-BG-RGB NORMAL SHAPEBURST-ANGULAR 100 0 REPEAT-NONE FALSE 0 0 0 0 1 1)
    (gimp-selection-none img)
    (gimp-palette-set-foreground blend-fg)
    (gimp-palette-set-background blend-bg)
    (gimp-blend blend-layer blend-mode NORMAL LINEAR 100 0 REPEAT-NONE FALSE 0 0 0 0 width 0)
    (gimp-layer-translate text-layer (- b-size-2) (- b-size-2))
    (gimp-layer-translate blend-layer (- b-size) (- b-size))
    (gimp-layer-translate text-shadow-layer (- ts-size) (- ts-size))
    (gimp-layer-translate drop-shadow-layer ds-size ds-size)
    (gimp-selection-layer-alpha blend-layer)
    (gimp-image-add-layer-mask img drop-shadow-layer dsl-layer-mask)
    (gimp-palette-set-background '(255 255 255))
    (gimp-edit-fill dsl-layer-mask)
    (gimp-image-remove-layer-mask img drop-shadow-layer APPLY)
    (gimp-selection-none img)
    (gimp-layer-set-name text-layer text)
    (gimp-palette-set-foreground old-fg)
    (gimp-palette-set-background old-bg)
    (gimp-image-enable-undo img)
    (gimp-display-new img)))

(script-fu-register "script-fu-blended-logo"
		    "<Toolbox>/Xtns/Script-Fu/Logos/Blended"
		    "Creates logos with blended backgrounds, highlights, and shadows"
		    "Spencer Kimball"
		    "Spencer Kimball"
		    "1996"
		    ""
		    SF-VALUE "Blend Mode" "FG-BG-RGB"
		    SF-STRING "Text String" "The GIMP"
		    SF-ADJUSTMENT "Font Size (pixels)" '(150 2 1000 1 10 0 1)
		    SF-FONT "Font" "-*-Crillee-*-r-*-*-24-*-*-*-p-*-*-*"
		    SF-COLOR "Background Color" '(255 255 255)
		    SF-COLOR "Text Color" '(124 174 255)
		    SF-COLOR "Starting Blend" '(22 9 129)
		    SF-COLOR "Ending Blend" '(129 9 82))
