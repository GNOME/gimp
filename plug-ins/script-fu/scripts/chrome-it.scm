;  CHROME-IT
;   State of the art chrome effect for user-specified mask
;   This script requires a grayscale image containing a single layer.
;   This layer is used as the mask for the SOTA chrome effect

(define (script-fu-sota-chrome-it mask-img mask-drawable chrome-saturation
         chrome-lightness chrome-factor env-map hc cc carve-white)

  (define (set-pt a index x y)
    (begin
      (aset a (* index 2) x)
      (aset a (+ (* index 2) 1) y)
    )
  )

  (define (spline-chrome-it)
    (let* ((a (cons-array 18 'double)))
      (set-pt a 0 0.0   0.0)
      (set-pt a 1 0.125 0.9216)
      (set-pt a 2 0.25  0.0902)
      (set-pt a 3 0.375 0.9020)
      (set-pt a 4 0.5   0.0989)
      (set-pt a 5 0.625 0.9549)
      (set-pt a 6 0.75  00784)
      (set-pt a 7 0.875 0.9412)
      (set-pt a 8 1.0   0.1216)
      a
    )
  )


  (define (shadows val)
    (/ (* 0.96 val) 2.55)
  )

  (define (midtones val)
    (/ val 2.55)
  )

  (define (highlights val)
    ; The result is used as "gimp-drawable-color-balance" color parameter
    ; and thus must be restricted to -100.0 <= highlights <= 100.0.
    (min (/ (* 1.108 val) 2.55) 100.0)
  )

  (define (rval col)
    (car col)
  )

  (define (gval col)
    (cadr col)
  )

  (define (bval col)
    (caddr col)
  )

  (define (sota-scale val scale chrome-factor)
    (* (sqrt val) (* scale chrome-factor))
  )

  (define (copy-layer-chrome-it dest-image dest-drawable source-image source-drawable)
    (gimp-selection-all dest-image)
    (gimp-drawable-edit-clear dest-drawable)
    (gimp-selection-none dest-image)
    (gimp-selection-all source-image)
    (gimp-edit-copy 1 (vector source-drawable))
    (let* (
           (pasted (gimp-edit-paste dest-drawable FALSE))
           (num-pasted (car pasted))
           (floating-sel (aref (cadr pasted) (- num-pasted 1)))
          )
     (gimp-floating-sel-anchor floating-sel)
    )
  )

  (let* (
        (banding-img (car (gimp-file-load RUN-NONINTERACTIVE env-map)))
        (banding-layer (aref (cadr (gimp-image-get-selected-drawables banding-img)) 0))
        (banding-height (car (gimp-drawable-get-height banding-layer)))
        (banding-width (car (gimp-drawable-get-width banding-layer)))
        (banding-type (car (gimp-drawable-type banding-layer)))
        (width (car (gimp-drawable-get-width mask-drawable)))
        (height (car (gimp-drawable-get-height mask-drawable)))
        (img (car (gimp-image-new width height GRAY)))
        (size (min width height))
        (offx1 (sota-scale size 0.33 chrome-factor))
        (offy1 (sota-scale size 0.25 chrome-factor))
        (offx2 (sota-scale size (- 0.33) chrome-factor))
        (offy2 (sota-scale size (- 0.25) chrome-factor))
        (feather (sota-scale size 0.5 chrome-factor))
        (brush-size (sota-scale size 0.5 chrome-factor))
        (brush (car (gimp-brush-new "Chrome It")))
        (mask (car (gimp-channel-new img width height "Chrome Stencil" 50 '(0 0 0))))
        (bg-layer (car (gimp-layer-new img width height GRAY-IMAGE _"Background" 100 LAYER-MODE-NORMAL)))
        (layer1 (car (gimp-layer-new img banding-width banding-height banding-type _"Layer 1" 100 LAYER-MODE-NORMAL)))
        (layer2 (car (gimp-layer-new img width height GRAYA-IMAGE _"Layer 2" 100 LAYER-MODE-DIFFERENCE)))
        (layer3 (car (gimp-layer-new img width height GRAYA-IMAGE _"Layer 3" 100 LAYER-MODE-NORMAL)))
        (shadow (car (gimp-layer-new img width height GRAYA-IMAGE _"Drop Shadow" 100 LAYER-MODE-NORMAL)))
        (layer-mask 0)
        (marble-pattern (car (gimp-pattern-get-by-name "Marble #1")))
        )

    (gimp-context-push)
    (gimp-context-set-defaults)

    (gimp-image-undo-disable img)

    (gimp-image-insert-channel img mask -1 0)
    (gimp-image-insert-layer img bg-layer 0 0)
    (gimp-image-insert-layer img shadow 0 0)
    (gimp-image-insert-layer img layer3 0 0)
    (gimp-image-insert-layer img layer2 0 0)

    (gimp-edit-copy 1 (vector mask-drawable))

    ; Clipboard is copy of mask-drawable.  Paste into mask, a channel, and anchor it.
    (let* (
           (pasted (gimp-edit-paste mask FALSE))
           (num-pasted (car pasted))
           (floating-sel (aref (cadr pasted) (- num-pasted 1)))
          )
     (gimp-floating-sel-anchor floating-sel)
    )

    (if (= carve-white FALSE)
        (gimp-drawable-invert mask FALSE)
    )

    (gimp-context-set-background '(255 255 255))
    (gimp-selection-none img)
    (gimp-drawable-edit-fill layer2 FILL-BACKGROUND)
    (gimp-drawable-edit-fill layer3 FILL-BACKGROUND)
    (gimp-drawable-edit-clear shadow)

    (gimp-item-set-visible bg-layer FALSE)
    (gimp-item-set-visible shadow FALSE)

    (gimp-image-select-item img CHANNEL-OP-REPLACE mask)
    (gimp-context-set-background '(0 0 0))
    (gimp-selection-translate img offx1 offy1)
    (gimp-selection-feather img feather)
    (gimp-drawable-edit-fill layer2 FILL-BACKGROUND)
    (gimp-selection-translate img (* 2 offx2) (* 2 offy2))
    (gimp-drawable-edit-fill layer3 FILL-BACKGROUND)
    (gimp-selection-none img)
    (set! layer2 (car (gimp-image-merge-visible-layers img CLIP-TO-IMAGE)))
    (gimp-drawable-invert layer2 FALSE)

    (gimp-image-insert-layer img layer1 0 0)
    (copy-layer-chrome-it img layer1 banding-img banding-layer)
    (gimp-image-delete banding-img)
    (gimp-layer-scale layer1 width height FALSE)
    (plug-in-gauss-iir RUN-NONINTERACTIVE img layer1 10 TRUE TRUE)
    (gimp-layer-set-opacity layer1 50)
    (set! layer1 (car (gimp-image-merge-visible-layers img CLIP-TO-IMAGE)))
    (gimp-drawable-curves-spline layer1 HISTOGRAM-VALUE 18 (spline-chrome-it))

    (set! layer-mask (car (gimp-layer-create-mask layer1 ADD-MASK-BLACK)))
    (gimp-layer-add-mask layer1 layer-mask)
    (gimp-image-select-item img CHANNEL-OP-REPLACE mask)
    (gimp-context-set-background '(255 255 255))
    (gimp-drawable-edit-fill layer-mask FILL-BACKGROUND)

    (set! layer2 (car (gimp-layer-copy layer1 TRUE)))
    (gimp-image-insert-layer img layer2 0 0)

    (gimp-brush-set-shape brush BRUSH-GENERATED-CIRCLE)
    (gimp-brush-set-spikes brush 2)
    (gimp-brush-set-hardness brush 1.0)
    (gimp-brush-set-spacing brush 25)
    (gimp-brush-set-aspect-ratio brush 1)
    (gimp-brush-set-angle brush 0)
    (cond (<= brush-size 17) (gimp-brush-set-radius brush (\ brush-size 2))
	  (else gimp-brush-set-radius brush (\ 19 2)))
    (gimp-context-set-brush brush)

    (gimp-context-set-foreground '(255 255 255))
    (gimp-drawable-edit-stroke-selection layer-mask)

    (gimp-context-set-background '(0 0 0))
    (gimp-selection-feather img (* feather 1.5))
    (gimp-selection-translate img (* 2.5 offx1) (* 2.5 offy1))
    (gimp-drawable-edit-fill shadow FILL-BACKGROUND)

    (gimp-selection-all img)
    (gimp-context-set-pattern marble-pattern)
    (gimp-drawable-edit-fill bg-layer FILL-PATTERN)
    (gimp-selection-none img)

    (gimp-image-convert-rgb img)

    (gimp-drawable-color-balance layer1 TRANSFER-SHADOWS TRUE
				 (shadows (rval hc))
				 (shadows (gval hc))
				 (shadows (bval hc)))
    (gimp-drawable-color-balance layer1 TRANSFER-MIDTONES TRUE
				 (midtones (rval hc))
				 (midtones (gval hc))
				 (midtones (bval hc)))
    (gimp-drawable-color-balance layer1 TRANSFER-HIGHLIGHTS TRUE
				 (highlights (rval hc))
				 (highlights (gval hc))
				 (highlights (bval hc)))

    (gimp-drawable-color-balance layer2 TRANSFER-SHADOWS TRUE
				 (shadows (rval cc))
				 (shadows (gval cc))
				 (shadows (bval cc)))
    (gimp-drawable-color-balance layer2 TRANSFER-MIDTONES TRUE
				 (midtones (rval cc))
				 (midtones (gval cc))
				 (midtones (bval cc)))
    (gimp-drawable-color-balance layer2 TRANSFER-HIGHLIGHTS TRUE
				 (highlights (rval cc))
				 (highlights (gval cc))
				 (highlights (bval cc)))
    (gimp-drawable-hue-saturation layer2 HUE-RANGE-ALL
				  0.0
				  chrome-lightness
				  chrome-saturation
				  0.0)

    (gimp-item-set-visible shadow TRUE)
    (gimp-item-set-visible bg-layer TRUE)

    (gimp-item-set-name layer2 _"Chrome")
    (gimp-item-set-name layer1 _"Highlight")

    (gimp-image-remove-channel img mask)

    (gimp-resource-delete brush)

    (gimp-display-new img)
    (gimp-image-undo-enable img)

    (gimp-context-pop)
  )
)

(script-fu-register "script-fu-sota-chrome-it"
  _"Stencil C_hrome..."
  _"Add a chrome effect to the selected region (or alpha) using a specified (grayscale) stencil"
  "Spencer Kimball"
  "Spencer Kimball"
  "1997"
  "GRAY"
  SF-IMAGE       "Chrome image"      0
  SF-DRAWABLE    "Chrome mask"       0
  SF-ADJUSTMENT _"Chrome saturation" '(-80 -100 100 1 10 0 0)
  SF-ADJUSTMENT _"Chrome lightness"  '(-47 -100 100 1 10 0 0)
  SF-ADJUSTMENT _"Chrome factor"     '(0.75 0 1 0.1 0.2 2 0)
  SF-FILENAME   _"Environment map"
                (string-append gimp-data-directory
                              "/scripts/images/beavis.jpg")
  SF-COLOR      _"Highlight balance"  '(211 95 0)
  SF-COLOR      _"Chrome balance"     "black"
  SF-TOGGLE     _"Chrome white areas" TRUE
)

(script-fu-menu-register "script-fu-sota-chrome-it"
                         "<Image>/Filters/Decor")
