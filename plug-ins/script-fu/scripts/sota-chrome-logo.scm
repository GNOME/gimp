;  CHROME-LOGO2
;   State of the art chrome logos
;

(define (set-pt a index x y)
  (prog1
   (aset a (* index 2) x)
   (aset a (+ (* index 2) 1) y)))

(define (spline1)
  (let* ((a (cons-array 18 'byte)))
    (set-pt a 0 0 0)
    (set-pt a 1 31 235)
    (set-pt a 2 63 23)
    (set-pt a 3 95 230)
    (set-pt a 4 127 25)
    (set-pt a 5 159 210)
    (set-pt a 6 191 20)
    (set-pt a 7 223 240)
    (set-pt a 8 255 31)
    a))

(define (brush brush-size)
  (cond ((<= brush-size 5) "Circle Fuzzy (05)")
	((<= brush-size 7) "Circle Fuzzy (07)")
	((<= brush-size 9) "Circle Fuzzy (09)")
	((<= brush-size 11) "Circle Fuzzy (11)")
	((<= brush-size 13) "Circle Fuzzy (13)")
	((<= brush-size 15) "Circle Fuzzy (15)")
	((<= brush-size 17) "Circle Fuzzy (17)")
	(else "Circle Fuzzy (19)")))

(define (shadows val)
  (/ (* 0.96 val) 2.55))

(define (midtones val)
  (/ val 2.55))

(define (highlights val)
  (let ((h (/ (* 1.108 val) 2.55)))
    (if (> h 100) 100 h)))

(define (rval col)
  (car col))

(define (gval col)
  (cadr col))

(define (bval col)
  (caddr col))

(define (sota-scale val scale chrome-factor)
  (* (sqrt val) (* scale chrome-factor)))

(define (copy-layer-sota dest-image dest-drawable source-image source-drawable)
  (gimp-selection-all dest-image)
  (gimp-edit-clear dest-drawable)
  (gimp-selection-none dest-image)
  (gimp-selection-all source-image)
  (gimp-edit-copy source-drawable)
      (let ((floating-sel (car (gimp-edit-paste dest-drawable FALSE))))
	(gimp-floating-sel-anchor floating-sel)))

(define (script-fu-sota-chrome-logo chrome-saturation chrome-lightness chrome-factor
				    text size fontname env-map hc cc)
  (let* ((img (car (gimp-image-new 256 256 GRAY)))
	 (banding-img (car (gimp-file-load 1 env-map env-map)))
	 (banding-layer (car (gimp-image-active-drawable banding-img)))
	 (banding-height (car (gimp-drawable-height banding-layer)))
	 (banding-width (car (gimp-drawable-width banding-layer)))
	 (banding-type (car (gimp-drawable-type banding-layer)))
	 (b-size (sota-scale size 2 chrome-factor))
	 (offx1 (sota-scale size 0.33 chrome-factor))
	 (offy1 (sota-scale size 0.25 chrome-factor))
	 (offx2 (sota-scale size (- 0.33) chrome-factor))
	 (offy2 (sota-scale size (- 0.25) chrome-factor))
	 (feather (sota-scale size 0.5 chrome-factor))
	 (brush-size (sota-scale size 0.5 chrome-factor))
	 (text-layer (car (gimp-text-fontname img -1 0 0 text (* b-size 2) TRUE size PIXELS fontname)))
	 (width (car (gimp-drawable-width text-layer)))
	 (height (car (gimp-drawable-height text-layer)))
	 (layer1 (car (gimp-layer-new img banding-width banding-height banding-type "Layer1" 100 NORMAL)))
	 (layer2 (car (gimp-layer-new img width height GRAYA_IMAGE "Layer 2" 100 DIFFERENCE)))
	 (layer3 (car (gimp-layer-new img width height GRAYA_IMAGE "Layer 3" 100 NORMAL)))
	 (shadow (car (gimp-layer-new img width height GRAYA_IMAGE "Drop Shadow" 100 NORMAL)))
	 (layer-mask 0)
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background)))
	 (old-brush (car (gimp-brushes-get-brush)))
	 (old-pattern (car (gimp-patterns-get-pattern))))
  
    (gimp-image-undo-disable img)
    (gimp-image-resize img width height 0 0)
    (gimp-image-add-layer img shadow 0)
    (gimp-image-add-layer img layer3 0)
    (gimp-image-add-layer img layer2 0)
    (gimp-palette-set-background '(255 255 255))
    (gimp-selection-none img)
    (gimp-edit-fill layer2)
    (gimp-edit-fill layer3)
    (gimp-edit-clear shadow)
    (gimp-layer-set-visible text-layer FALSE)
    (gimp-layer-set-visible shadow FALSE)

    (gimp-rect-select img (/ b-size 2) (/ b-size 2) (- width b-size) (- height b-size) REPLACE 0 0)
    (gimp-rect-select img b-size b-size (- width (* b-size 2)) (- height (* b-size 2)) SUB 0 0)
    (gimp-edit-fill text-layer)

    (gimp-selection-layer-alpha text-layer)
    (gimp-palette-set-background '(0 0 0))
    (gimp-selection-translate img offx1 offy1)
    (gimp-selection-feather img feather)
    (gimp-edit-fill layer2)
    (gimp-selection-translate img (* 2 offx2) (* 2 offy2))
    (gimp-edit-fill layer3)
    (gimp-selection-none img)
    (gimp-layer-set-visible  layer2 TRUE)
    (gimp-layer-set-visible  layer3 TRUE)
    (set! layer2 (car (gimp-image-merge-visible-layers img CLIP-TO-IMAGE)))
    (gimp-invert layer2)

    (copy-layer-sota img layer1 banding-img banding-layer)
    (gimp-image-delete banding-img)
    (gimp-image-add-layer img layer1 0)
    (gimp-layer-scale layer1 width height FALSE)
    (plug-in-gauss-iir 1 img layer1 10 TRUE TRUE)
    (gimp-layer-set-opacity layer1 50)
    (gimp-layer-set-visible  layer1 TRUE)
    (gimp-layer-set-visible  layer2 TRUE)
    (set! layer1 (car (gimp-image-merge-visible-layers img CLIP-TO-IMAGE)))
    (gimp-curves-spline layer1 0 18 (spline1))

    (set! layer-mask (car (gimp-layer-create-mask layer1 BLACK-MASK)))
    (gimp-image-add-layer-mask img layer1 layer-mask)
    (gimp-selection-layer-alpha text-layer)
    (gimp-palette-set-background '(255 255 255))
    (gimp-edit-fill layer-mask)

    (set! layer2 (car (gimp-layer-copy layer1 TRUE)))
    (gimp-image-add-layer img layer2 0)
    (gimp-brushes-set-brush (brush brush-size))
    (gimp-palette-set-foreground '(255 255 255))
    (gimp-edit-stroke layer-mask)

    (gimp-palette-set-background '(0 0 0))
    (gimp-selection-feather img (* feather 1.5))
    (gimp-selection-translate img (* 2.5 offx1) (* 2.5 offy1))
    (gimp-edit-fill shadow)

    (gimp-selection-all img)
    (gimp-patterns-set-pattern "Marble #1")
    (gimp-bucket-fill text-layer PATTERN-BUCKET-FILL NORMAL 100 0 FALSE 0 0)
    (gimp-selection-none img)

    (gimp-convert-rgb img)

    (gimp-color-balance layer1 0 TRUE (shadows (rval hc)) (shadows (gval hc)) (shadows (bval hc)))
    (gimp-color-balance layer1 1 TRUE (midtones (rval hc)) (midtones (gval hc)) (midtones (bval hc)))
    (gimp-color-balance layer1 2 TRUE (highlights (rval hc)) (highlights (gval hc)) (highlights (bval hc)))

    (gimp-color-balance layer2 0 TRUE (shadows (rval cc)) (shadows (gval cc)) (shadows (bval cc)))
    (gimp-color-balance layer2 1 TRUE (midtones (rval cc)) (midtones (gval cc)) (midtones (bval cc)))
    (gimp-color-balance layer2 2 TRUE (highlights (rval cc)) (highlights (gval cc)) (highlights (bval cc)))
    (gimp-hue-saturation layer2 0 0 chrome-lightness chrome-saturation)

    (gimp-layer-set-visible shadow TRUE)
    (gimp-layer-set-visible text-layer TRUE)

    (gimp-layer-set-name text-layer "Background")
    (gimp-layer-set-name layer2 "Chrome")
    (gimp-layer-set-name layer1 "Highlight")

    (gimp-layer-translate shadow (/ b-size -4) (/ b-size -4))
    (gimp-layer-translate layer2 (/ b-size -4) (/ b-size -4))
    (gimp-layer-translate layer1 (/ b-size -4) (/ b-size -4))

    (gimp-palette-set-foreground old-fg)
    (gimp-palette-set-background old-bg)
    (gimp-brushes-set-brush old-brush)
    (gimp-patterns-set-pattern old-pattern)
    (gimp-image-undo-enable img)
    (gimp-display-new img)))

(script-fu-register "script-fu-sota-chrome-logo"
		    "<Toolbox>/Xtns/Script-Fu/Logos/SOTA Chrome"
		    "State of the art chromed logos."
		    "Spencer Kimball"
		    "Spencer Kimball"
		    "1997"
		    ""
		    SF-ADJUSTMENT "Chrome Saturation" '(-80 -100 100 1 10 0 0)
		    SF-ADJUSTMENT "Chrome Lightness" '(-47 -100 100 1 10 0 0)
		    SF-ADJUSTMENT  "Chrome Factor" '(.75 0 1 .1 .01 2 0)
		    SF-STRING "Text String" "The GIMP"
		    SF-ADJUSTMENT "Font Size (pixels)" '(150 2 1000 1 10 0 1)
		    SF-FONT "Font" "-*-RoostHeavy-*-r-*-*-24-*-*-*-p-*-*-*"
		    SF-FILENAME "Environment Map" (string-append "" gimp-data-dir "/scripts/beavis.jpg")
		    SF-COLOR "Highlight Balance" '(211 95 0)
		    SF-COLOR "Chrome Balance" '(0 0 0))
