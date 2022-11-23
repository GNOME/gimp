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
    ; The result is used as "ligma-drawable-color-balance" color parameter
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
    (ligma-selection-all dest-image)
    (ligma-drawable-edit-clear dest-drawable)
    (ligma-selection-none dest-image)
    (ligma-selection-all source-image)
    (ligma-edit-copy 1 (vector source-drawable))
    (let* (
           (pasted (ligma-edit-paste dest-drawable FALSE))
           (num-pasted (car pasted))
           (floating-sel (aref (cadr pasted) (- num-pasted 1)))
          )
     (ligma-floating-sel-anchor floating-sel)
    )
  )

  (let* (
        (banding-img (car (ligma-file-load RUN-NONINTERACTIVE env-map)))
        (banding-layer (aref (cadr (ligma-image-get-selected-drawables banding-img)) 0))
        (banding-height (car (ligma-drawable-get-height banding-layer)))
        (banding-width (car (ligma-drawable-get-width banding-layer)))
        (banding-type (car (ligma-drawable-type banding-layer)))
        (width (car (ligma-drawable-get-width mask-drawable)))
        (height (car (ligma-drawable-get-height mask-drawable)))
        (img (car (ligma-image-new width height GRAY)))
        (size (min width height))
        (offx1 (sota-scale size 0.33 chrome-factor))
        (offy1 (sota-scale size 0.25 chrome-factor))
        (offx2 (sota-scale size (- 0.33) chrome-factor))
        (offy2 (sota-scale size (- 0.25) chrome-factor))
        (feather (sota-scale size 0.5 chrome-factor))
        (brush-size (sota-scale size 0.5 chrome-factor))
        (brush-name (car (ligma-brush-new "Chrome It")))
        (mask (car (ligma-channel-new img width height "Chrome Stencil" 50 '(0 0 0))))
        (bg-layer (car (ligma-layer-new img width height GRAY-IMAGE _"Background" 100 LAYER-MODE-NORMAL)))
        (layer1 (car (ligma-layer-new img banding-width banding-height banding-type _"Layer 1" 100 LAYER-MODE-NORMAL)))
        (layer2 (car (ligma-layer-new img width height GRAYA-IMAGE _"Layer 2" 100 LAYER-MODE-DIFFERENCE)))
        (layer3 (car (ligma-layer-new img width height GRAYA-IMAGE _"Layer 3" 100 LAYER-MODE-NORMAL)))
        (shadow (car (ligma-layer-new img width height GRAYA-IMAGE _"Drop Shadow" 100 LAYER-MODE-NORMAL)))
        (layer-mask 0)
        )

    (ligma-context-push)
    (ligma-context-set-defaults)

    (ligma-image-undo-disable img)

    (ligma-image-insert-channel img mask -1 0)
    (ligma-image-insert-layer img bg-layer 0 0)
    (ligma-image-insert-layer img shadow 0 0)
    (ligma-image-insert-layer img layer3 0 0)
    (ligma-image-insert-layer img layer2 0 0)

    (ligma-edit-copy 1 (vector mask-drawable))

    ; Clipboard is copy of mask-drawable.  Paste into mask, a channel, and anchor it.
    (let* (
           (pasted (ligma-edit-paste mask FALSE))
           (num-pasted (car pasted))
           (floating-sel (aref (cadr pasted) (- num-pasted 1)))
          )
     (ligma-floating-sel-anchor floating-sel)
    )

    (if (= carve-white FALSE)
        (ligma-drawable-invert mask FALSE)
    )

    (ligma-context-set-background '(255 255 255))
    (ligma-selection-none img)
    (ligma-drawable-edit-fill layer2 FILL-BACKGROUND)
    (ligma-drawable-edit-fill layer3 FILL-BACKGROUND)
    (ligma-drawable-edit-clear shadow)

    (ligma-item-set-visible bg-layer FALSE)
    (ligma-item-set-visible shadow FALSE)

    (ligma-image-select-item img CHANNEL-OP-REPLACE mask)
    (ligma-context-set-background '(0 0 0))
    (ligma-selection-translate img offx1 offy1)
    (ligma-selection-feather img feather)
    (ligma-drawable-edit-fill layer2 FILL-BACKGROUND)
    (ligma-selection-translate img (* 2 offx2) (* 2 offy2))
    (ligma-drawable-edit-fill layer3 FILL-BACKGROUND)
    (ligma-selection-none img)
    (set! layer2 (car (ligma-image-merge-visible-layers img CLIP-TO-IMAGE)))
    (ligma-drawable-invert layer2 FALSE)

    (ligma-image-insert-layer img layer1 0 0)
    (copy-layer-chrome-it img layer1 banding-img banding-layer)
    (ligma-image-delete banding-img)
    (ligma-layer-scale layer1 width height FALSE)
    (plug-in-gauss-iir RUN-NONINTERACTIVE img layer1 10 TRUE TRUE)
    (ligma-layer-set-opacity layer1 50)
    (set! layer1 (car (ligma-image-merge-visible-layers img CLIP-TO-IMAGE)))
    (ligma-drawable-curves-spline layer1 HISTOGRAM-VALUE 18 (spline-chrome-it))

    (set! layer-mask (car (ligma-layer-create-mask layer1 ADD-MASK-BLACK)))
    (ligma-layer-add-mask layer1 layer-mask)
    (ligma-image-select-item img CHANNEL-OP-REPLACE mask)
    (ligma-context-set-background '(255 255 255))
    (ligma-drawable-edit-fill layer-mask FILL-BACKGROUND)

    (set! layer2 (car (ligma-layer-copy layer1 TRUE)))
    (ligma-image-insert-layer img layer2 0 0)

    (ligma-brush-set-shape brush-name BRUSH-GENERATED-CIRCLE)
    (ligma-brush-set-spikes brush-name 2)
    (ligma-brush-set-hardness brush-name 1.0)
    (ligma-brush-set-spacing brush-name 25)
    (ligma-brush-set-aspect-ratio brush-name 1)
    (ligma-brush-set-angle brush-name 0)
    (cond (<= brush-size 17) (ligma-brush-set-radius brush-name (\ brush-size 2))
	  (else ligma-brush-set-radius brush-name (\ 19 2)))
    (ligma-context-set-brush brush-name)

    (ligma-context-set-foreground '(255 255 255))
    (ligma-drawable-edit-stroke-selection layer-mask)

    (ligma-context-set-background '(0 0 0))
    (ligma-selection-feather img (* feather 1.5))
    (ligma-selection-translate img (* 2.5 offx1) (* 2.5 offy1))
    (ligma-drawable-edit-fill shadow FILL-BACKGROUND)

    (ligma-selection-all img)
    (ligma-context-set-pattern "Marble #1")
    (ligma-drawable-edit-fill bg-layer FILL-PATTERN)
    (ligma-selection-none img)

    (ligma-image-convert-rgb img)

    (ligma-drawable-color-balance layer1 TRANSFER-SHADOWS TRUE
				 (shadows (rval hc))
				 (shadows (gval hc))
				 (shadows (bval hc)))
    (ligma-drawable-color-balance layer1 TRANSFER-MIDTONES TRUE
				 (midtones (rval hc))
				 (midtones (gval hc))
				 (midtones (bval hc)))
    (ligma-drawable-color-balance layer1 TRANSFER-HIGHLIGHTS TRUE
				 (highlights (rval hc))
				 (highlights (gval hc))
				 (highlights (bval hc)))

    (ligma-drawable-color-balance layer2 TRANSFER-SHADOWS TRUE
				 (shadows (rval cc))
				 (shadows (gval cc))
				 (shadows (bval cc)))
    (ligma-drawable-color-balance layer2 TRANSFER-MIDTONES TRUE
				 (midtones (rval cc))
				 (midtones (gval cc))
				 (midtones (bval cc)))
    (ligma-drawable-color-balance layer2 TRANSFER-HIGHLIGHTS TRUE
				 (highlights (rval cc))
				 (highlights (gval cc))
				 (highlights (bval cc)))
    (ligma-drawable-hue-saturation layer2 HUE-RANGE-ALL
				  0.0
				  chrome-lightness
				  chrome-saturation
				  0.0)

    (ligma-item-set-visible shadow TRUE)
    (ligma-item-set-visible bg-layer TRUE)

    (ligma-item-set-name layer2 _"Chrome")
    (ligma-item-set-name layer1 _"Highlight")

    (ligma-image-remove-channel img mask)

    (ligma-brush-delete brush-name)

    (ligma-display-new img)
    (ligma-image-undo-enable img)

    (ligma-context-pop)
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
                (string-append ligma-data-directory
                              "/scripts/images/beavis.jpg")
  SF-COLOR      _"Highlight balance"  '(211 95 0)
  SF-COLOR      _"Chrome balance"     "black"
  SF-TOGGLE     _"Chrome white areas" TRUE
)

(script-fu-menu-register "script-fu-sota-chrome-it"
                         "<Image>/Filters/Decor")
