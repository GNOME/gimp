;  CHROME-IT
;   State of the art chrome effect for user-specified mask
;   This script requires a grayscale image containing a single layer.
;   This layer is used as the mask for the SOTA chrome effect

(define (set-pt a index x y)
  (prog1
   (aset a (* index 2) x)
   (aset a (+ (* index 2) 1) y)))

(define (spline-chrome-it)
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
  (/ (* 1.108 val) 2.55))

(define (rval col)
  (car col))

(define (gval col)
  (cadr col))

(define (bval col)
  (caddr col))

(define (sota-scale val scale chrome-factor)
  (* (sqrt val) (* scale chrome-factor)))

(define (copy-layer-chrome-it dest-image dest-drawable source-image source-drawable)
  (gimp-selection-all dest-image)
  (gimp-edit-clear dest-image dest-drawable)
  (gimp-selection-none dest-image)
  (gimp-selection-all source-image)
  (gimp-edit-copy source-image source-drawable)
      (let ((floating-sel (car (gimp-edit-paste dest-image dest-drawable FALSE))))
	(gimp-floating-sel-anchor floating-sel)))

(define (script-fu-sota-chrome-it mask-img mask-drawable chrome-saturation
				  chrome-lightness chrome-factor env-map hc cc carve-white)
  (let* ((banding-img (car (gimp-file-load 1 env-map env-map)))
	 (banding-layer (car (gimp-image-active-drawable banding-img)))
	 (banding-height (car (gimp-drawable-height banding-layer)))
	 (banding-width (car (gimp-drawable-width banding-layer)))
	 (banding-type (car (gimp-drawable-type banding-layer)))
	 (width (car (gimp-drawable-width mask-drawable)))
	 (height (car (gimp-drawable-height mask-drawable)))
	 (img (car (gimp-image-new width height GRAY)))
	 (size (min width height))
	 (offx1 (sota-scale size 0.33 chrome-factor))
	 (offy1 (sota-scale size 0.25 chrome-factor))
	 (offx2 (sota-scale size (- 0.33) chrome-factor))
	 (offy2 (sota-scale size (- 0.25) chrome-factor))
	 (feather (sota-scale size 0.5 chrome-factor))
	 (brush-size (sota-scale size 0.5 chrome-factor))
	 (mask (car (gimp-channel-new img width height "Chrome Stencil" 50 '(0 0 0))))
	 (bg-layer (car (gimp-layer-new img width height GRAY_IMAGE "Background" 100 NORMAL)))
	 (layer1 (car (gimp-layer-new img banding-width banding-height banding-type "Layer1" 100 NORMAL)))
	 (layer2 (car (gimp-layer-new img width height GRAYA_IMAGE "Layer 2" 100 DIFFERENCE)))
	 (layer3 (car (gimp-layer-new img width height GRAYA_IMAGE "Layer 3" 100 NORMAL)))
	 (shadow (car (gimp-layer-new img width height GRAYA_IMAGE "Drop Shadow" 100 NORMAL)))
	 (mask-fs 0)
	 (layer-mask 0)
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background)))
	 (old-brush (car (gimp-brushes-get-brush)))
	 (old-pattern (car (gimp-patterns-get-pattern))))
    (gimp-image-disable-undo img)

    (gimp-image-add-channel img mask 0)
    (gimp-image-add-layer img bg-layer 0)
    (gimp-image-add-layer img shadow 0)
    (gimp-image-add-layer img layer3 0)
    (gimp-image-add-layer img layer2 0)

    (gimp-edit-copy mask-img mask-drawable)
    (set! mask-fs (car (gimp-edit-paste img mask FALSE)))
    (gimp-floating-sel-anchor mask-fs)
    (if (= carve-white FALSE)
	(gimp-invert img mask))

    (gimp-palette-set-background '(255 255 255))
    (gimp-selection-none img)
    (gimp-edit-fill img layer2)
    (gimp-edit-fill img layer3)
    (gimp-edit-clear img shadow)

    (gimp-layer-set-visible bg-layer FALSE)
    (gimp-layer-set-visible shadow FALSE)

    (gimp-selection-load img mask)
    (gimp-palette-set-background '(0 0 0))
    (gimp-selection-translate img offx1 offy1)
    (gimp-selection-feather img feather)
    (gimp-edit-fill img layer2)
    (gimp-selection-translate img (* 2 offx2) (* 2 offy2))
    (gimp-edit-fill img layer3)
    (gimp-selection-none img)
    (set! layer2 (car (gimp-image-merge-visible-layers img CLIP-TO-IMAGE)))
    (gimp-invert img layer2)

    (copy-layer-chrome-it img layer1 banding-img banding-layer)
    (gimp-image-delete banding-img)
    (gimp-image-add-layer img layer1 0)
    (gimp-layer-scale layer1 width height FALSE)
    (plug-in-gauss-iir 1 img layer1 10 TRUE TRUE)
    (gimp-layer-set-opacity layer1 50)
    (set! layer1 (car (gimp-image-merge-visible-layers img CLIP-TO-IMAGE)))
    (gimp-curves-spline img layer1 0 18 (spline-chrome-it))

    (set! layer-mask (car (gimp-layer-create-mask layer1 BLACK-MASK)))
    (gimp-image-add-layer-mask img layer1 layer-mask)
    (gimp-selection-load img mask)
    (gimp-palette-set-background '(255 255 255))
    (gimp-edit-fill img layer-mask)

    (set! layer2 (car (gimp-layer-copy layer1 TRUE)))
    (gimp-image-add-layer img layer2 0)
    (gimp-brushes-set-brush (brush brush-size))
    (gimp-palette-set-foreground '(255 255 255))
    (gimp-edit-stroke img layer-mask)

    (gimp-palette-set-background '(0 0 0))
    (gimp-selection-feather img (* feather 1.5))
    (gimp-selection-translate img (* 2.5 offx1) (* 2.5 offy1))
    (gimp-edit-fill img shadow)

    (gimp-selection-all img)
    (gimp-patterns-set-pattern "Marble #1")
    (gimp-bucket-fill img bg-layer PATTERN-BUCKET-FILL NORMAL 100 0 FALSE 0 0)
    (gimp-selection-none img)

    (gimp-convert-rgb img)

    (gimp-color-balance img layer1 0 TRUE (shadows (rval hc)) (shadows (gval hc)) (shadows (bval hc)))
    (gimp-color-balance img layer1 1 TRUE (midtones (rval hc)) (midtones (gval hc)) (midtones (bval hc)))
    (gimp-color-balance img layer1 2 TRUE (highlights (rval hc)) (highlights (gval hc)) (highlights (bval hc)))

    (gimp-color-balance img layer2 0 TRUE (shadows (rval cc)) (shadows (gval cc)) (shadows (bval cc)))
    (gimp-color-balance img layer2 1 TRUE (midtones (rval cc)) (midtones (gval cc)) (midtones (bval cc)))
    (gimp-color-balance img layer2 2 TRUE (highlights (rval cc)) (highlights (gval cc)) (highlights (bval cc)))
    (gimp-hue-saturation img layer2 0 0 chrome-lightness chrome-saturation)

    (gimp-layer-set-visible shadow TRUE)
    (gimp-layer-set-visible bg-layer TRUE)

    (gimp-layer-set-name layer2 "Chrome")
    (gimp-layer-set-name layer1 "Highlight")

    (gimp-image-remove-channel img mask)

    (gimp-palette-set-foreground old-fg)
    (gimp-palette-set-background old-bg)
    (gimp-brushes-set-brush old-brush)
    (gimp-patterns-set-pattern old-pattern)
    (gimp-display-new img)
    (gimp-image-enable-undo img)))

(script-fu-register "script-fu-sota-chrome-it"
		    "<Image>/Script-Fu/Stencil Ops/Chrome-It"
		    "Use the specified [GRAY] drawable as a stencil to run the chrome effect on."
		    "Spencer Kimball"
		    "Spencer Kimball"
		    "1997"
		    "GRAY"
		    SF-IMAGE "Chrome Image" 0
		    SF-DRAWABLE "Chrome Mask" 0
		    SF-VALUE "Chrome Saturation" "-80"
		    SF-VALUE "Chrome Lightness" "-47"
		    SF-VALUE "Chrome Factor" "0.75"
		    SF-STRING "Environment Map" (string-append "" gimp-data-dir "/scripts/beavis.jpg")
		    SF-COLOR "Highlight Balance" '(211 95 0)
		    SF-COLOR "Chrome Balance" '(0 0 0)
		    SF-TOGGLE "Chrome White Areas" TRUE)
