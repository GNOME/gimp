;  COOL-METAL
;  Create a text effect that looks like metal with a reflection of
;  the horizon, a reflection of the text in the mirrored ground, and
;  an interesting dropshadow
;  This script was inspired by Rob Malda's 'coolmetal.gif' graphic

(define (script-fu-cool-metal-logo text size font bg-color seascape)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
	 (feather (/ size 5))
	 (smear 7.5)
	 (period (/ size 3))
	 (amplitude (/ size 40))
	 (shrink (+ 1 (/ size 30)))
	 (depth (/ size 20))
	 (text-layer (car (gimp-text-fontname img -1 0 0 text 0 TRUE size PIXELS font)))
	 (width (car (gimp-drawable-width text-layer)))
	 (height (car (gimp-drawable-height text-layer)))
	 (img-width (+ width (* 0.15 height) 10))
	 (img-height (+ (* 1.85 height) 10))
	 (bg-layer (car (gimp-layer-new img img-width img-height RGB_IMAGE "Background" 100 NORMAL)))
	 (shadow-layer (car (gimp-layer-new img img-width img-height RGBA_IMAGE "Shadow" 100 NORMAL)))
	 (reflect-layer (car (gimp-layer-new img width height RGBA_IMAGE "Reflection" 100 NORMAL)))
	 (channel 0)
	 (fs 0)
	 (layer-mask 0)
	 (old-gradient (car (gimp-gradients-get-active)))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background))))
    (gimp-image-undo-disable img)
    (gimp-image-resize img img-width img-height 0 0)
    (gimp-image-add-layer img bg-layer 1)
    (gimp-image-add-layer img reflect-layer 1)
    (gimp-image-add-layer img shadow-layer 1)
    (gimp-layer-set-preserve-trans text-layer TRUE)

    (gimp-palette-set-background bg-color)
    (gimp-edit-fill bg-layer)
    (gimp-edit-clear reflect-layer)
    (gimp-palette-set-background '(0 0 0))
    (gimp-edit-fill shadow-layer)

    (if (= seascape 1)
	(gimp-gradients-set-active "Horizon_2")
	(gimp-gradients-set-active "Horizon_1"))
    (gimp-blend text-layer CUSTOM NORMAL LINEAR 100 0 REPEAT-NONE FALSE 0 0 0 0 0 (+ height 5))
    (gimp-rect-select img 0 (- (/ height 2) feather) img-width (* 2 feather) REPLACE 0 0)
    (plug-in-gauss-iir 1 img text-layer smear TRUE TRUE)
    (gimp-selection-none img)
    (plug-in-ripple 1 img text-layer period amplitude 1 0 1 TRUE FALSE)
    (gimp-layer-translate text-layer 5 5)
    (gimp-layer-resize text-layer img-width img-height 5 5)

    (gimp-selection-layer-alpha text-layer)
    (set! channel (car (gimp-selection-save img)))
    (gimp-selection-shrink img shrink)
    (gimp-selection-invert img)
    (plug-in-gauss-rle 1 img channel feather TRUE TRUE)
    (gimp-selection-layer-alpha text-layer)
    (gimp-selection-invert img)
    (gimp-palette-set-background '(0 0 0))
    (gimp-edit-fill channel)
    (gimp-selection-none img)

    (plug-in-bump-map 1 img text-layer channel 135 45 depth 0 0 0 0 FALSE FALSE 0)

    (gimp-selection-layer-alpha text-layer)
    (set! fs (car (gimp-selection-float shadow-layer 0 0)))
    (gimp-edit-clear shadow-layer)
    (gimp-perspective fs FALSE
		      (+ 5 (* 0.15 height)) (- height (* 0.15 height))
		      (+ 5 width (* 0.15 height)) (- height (* 0.15 height))
		      5 height
		      (+ 5 width) height)
    (gimp-floating-sel-anchor fs)
    (plug-in-gauss-rle 1 img shadow-layer smear TRUE TRUE)

    (gimp-rect-select img 5 5 width height REPLACE FALSE 0)
    (gimp-edit-copy text-layer)
    (set! fs (car (gimp-edit-paste reflect-layer FALSE)))
    (gimp-floating-sel-anchor fs)
    (gimp-scale reflect-layer FALSE 0 0 width (* 0.85 height))
    (gimp-flip reflect-layer 1)
    (gimp-layer-set-offsets reflect-layer 5 (+ 3 height))

    (set! layer-mask (car (gimp-layer-create-mask reflect-layer WHITE-MASK)))
    (gimp-image-add-layer-mask img reflect-layer layer-mask)
    (gimp-palette-set-foreground '(255 255 255))
    (gimp-palette-set-background '(0 0 0))
    (gimp-blend layer-mask FG-BG-RGB NORMAL LINEAR 100 0 REPEAT-NONE
		FALSE 0 0 0 (- (/ height 2)) 0 height)

    (gimp-image-remove-channel img channel)

    (gimp-layer-set-name text-layer text)
    (gimp-gradients-set-active old-gradient)
    (gimp-palette-set-background old-bg)
    (gimp-palette-set-foreground old-fg)
    (gimp-image-undo-enable img)
    (gimp-display-new img)))


(script-fu-register "script-fu-cool-metal-logo"
		    "<Toolbox>/Xtns/Script-Fu/Logos/Cool Metal..."
		    "Metallic logos with reflections and perspective shadows"
		    "Spencer Kimball & Rob Malda"
		    "Spencer Kimball & Rob Malda"
		    "1997"
		    ""
		    SF-STRING "Text String" "Cool Metal"
		    SF-ADJUSTMENT "Font Size (pixels)" '(100 2 1000 1 10 0 1)
		    SF-FONT "Font" "-*-Crillee-*-r-*-*-24-*-*-*-p-*-*-*"
		    SF-COLOR "Background Color" '(255 255 255)
		    SF-TOGGLE "Seascape" FALSE)
