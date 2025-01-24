;  CHROME-IT

; The filter generates a new image, having alpha channels.
; The source image is not touched.
; It renders, not filters.
; The source image is flattened and used as mask for the effect.

; Terminology:
; The effect is commonly known as the "chrome effect".
; "SOTA" means "state of the art" (but the art may have improved.)
; The algorithm's origin is not documented.
; "Stencil" seems to mean that the effect roughly follows the contours
; of shapes in the image, not the usual meaning.




; An adaptor from the original signature and mode requirements
; to a new inner signature and relaxed outer mode requirements.
;
; 1. Adapt to inner signature taking an image instead of a file.
; Adapts env-map, an image file to be opened, to banding-img, an open image.
; When user doesn't choose a file, banding-img is a copy of the primary mask-img.
; The effect is something, if not the same as when user chooses a different image
; than the primary image.
; In SFv2 the file chooser had a default, so the user could NOT choose a None file
; for the env-map file, the secondary image.
;
; The UI design for the secondary "banding image":
; the user must choose a file of an image instead of an open image.
; FUTURE: require the user to choose an open image for the secondary image.
;
; 2. Adapt relaxed outer mode requirement to inner flat requirement.
; Adapt means copy and flatten (delete alpha) before passing to inner.
; The filter originally required a GRAY image without an alpha.
; That produces an image that looks like the source image.
; The inner algorithm also succeeds on any image mode, with or without alpha,
; but the result is less like the source image, in shape.
; For user friendliness, relax the requirements but retain the original effect
; by copying and flattening whatever image is passed.

(define (script-fu-sota-chrome-it mask-img mask-drawables chrome-saturation
         chrome-lightness chrome-factor env-map hc cc carve-white)

  ; use v3 binding of return args from PDB
  (script-fu-use-v3)

  (let* (
        ; 1. convert choice of secondary image file to an open image
        (banding-img  ; secondary, other image
          ; when user chose no env-map secondary image file
          (if (= (string-length env-map) 0)
            ; copy source image
            (gimp-image-duplicate mask-img)
            ; else open chosen file
            (gimp-file-load RUN-NONINTERACTIVE env-map)))
        ; 2. copy source image, flattened
        (copy-source-img (gimp-image-duplicate mask-img))
        ; side effect on image, not use the returned layer
        (flat-source-layer (gimp-image-flatten copy-source-img))
        (copy-source-drawables (gimp-image-get-layers copy-source-img)))

    ; call inner, passing adapted args
    (chrome-it-inner
       copy-source-img copy-source-drawables ; adapted
       chrome-saturation chrome-lightness chrome-factor
       banding-img ; adapted
       hc cc carve-white)

    ; Cleanup adapted images
    ; Inner deletes banding-img
    (gimp-image-delete copy-source-img))
)


; The original filter, now taking a image:banding-img instead of file:env-map
(define (chrome-it-inner mask-img mask-drawables chrome-saturation
         chrome-lightness chrome-factor banding-img hc cc carve-white)

  (define (set-pt a index x y)
    (begin
      (vector-set! a (* index 2) x)
      (vector-set! a (+ (* index 2) 1) y)))

  (define (spline-chrome-it)
    (let* ((a (cons-array 18 'double)))
      (set-pt a 0 0.0   0.0)
      (set-pt a 1 0.125 0.9216)
      (set-pt a 2 0.25  0.0902)
      (set-pt a 3 0.375 0.9020)
      (set-pt a 4 0.5   0.0989)
      (set-pt a 5 0.625 0.9549)
      (set-pt a 6 0.75  0.0784)
      (set-pt a 7 0.875 0.9412)
      (set-pt a 8 1.0   0.1216)
      a))


  (define (shadows val)
    (/ (* 0.96 val) 2.55))

  (define (midtones val)
    (/ val 2.55))

  (define (highlights val)
    ; The result is used as "gimp-drawable-color-balance" color parameter
    ; and thus must be restricted to -100.0 <= highlights <= 100.0.
    (min (/ (* 1.108 val) 2.55) 100.0))

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
    (gimp-drawable-edit-clear dest-drawable)
    (gimp-selection-none dest-image)
    (gimp-selection-all source-image)
    (gimp-edit-copy (vector source-drawable))
    (let* (
           (pasted (gimp-edit-paste dest-drawable #f))
           (num-pasted (vector-length pasted))
           (floating-sel (vector-ref pasted (- num-pasted 1)))
          )
     (gimp-floating-sel-anchor floating-sel)))

  (let* (
        (banding-layer (vector-ref (gimp-image-get-selected-drawables banding-img) 0))
        (banding-height (gimp-drawable-get-height banding-layer))
        (banding-width (gimp-drawable-get-width banding-layer))
        (banding-type (gimp-drawable-type banding-layer))
        (mask-drawable (vector-ref mask-drawables 0))
        (width (gimp-drawable-get-width mask-drawable))
        (height (gimp-drawable-get-height mask-drawable))
        (img (gimp-image-new width height GRAY))
        (size (min width height))
        (offx1 (sota-scale size 0.33 chrome-factor))
        (offy1 (sota-scale size 0.25 chrome-factor))
        (offx2 (sota-scale size (- 0.33) chrome-factor))
        (offy2 (sota-scale size (- 0.25) chrome-factor))
        (feather (sota-scale size 0.5 chrome-factor))
        (brush-size (sota-scale size 0.5 chrome-factor))
        (brush (gimp-brush-new "Chrome It"))
        (mask (gimp-channel-new img "Chrome Stencil" width height 50 '(0 0 0)))
        (bg-layer (gimp-layer-new img _"Background" width height GRAY-IMAGE 100 LAYER-MODE-NORMAL))
        (layer1 (gimp-layer-new img _"Layer 1" banding-width banding-height banding-type 100 LAYER-MODE-NORMAL))
        (layer2 (gimp-layer-new img _"Layer 2" width height GRAYA-IMAGE 100 LAYER-MODE-DIFFERENCE))
        (layer3 (gimp-layer-new img _"Layer 3" width height GRAYA-IMAGE 100 LAYER-MODE-NORMAL))
        (shadow (gimp-layer-new img _"Drop Shadow" width height GRAYA-IMAGE 100 LAYER-MODE-NORMAL))
        (layer-mask 0)
        (marble-pattern (gimp-pattern-get-by-name "Marble #1"))
        )

    (gimp-context-push)
    (gimp-context-set-defaults)

    (gimp-image-undo-disable img)

    (gimp-image-insert-channel img mask -1 0)
    (gimp-image-insert-layer img bg-layer 0 0)
    (gimp-image-insert-layer img shadow 0 0)
    (gimp-image-insert-layer img layer3 0 0)
    (gimp-image-insert-layer img layer2 0 0)

    (gimp-edit-copy (vector mask-drawable))

    ; Clipboard is copy of mask-drawable.  Paste into mask, a channel, and anchor it.
    (let* (
           (pasted (gimp-edit-paste mask #f))
           (num-pasted (vector-length pasted))
           (floating-sel (vector-ref pasted (- num-pasted 1)))
          )
     (gimp-floating-sel-anchor floating-sel)
    )

    ; carve-white is 0 or 1, not #f or #t
    (if (= carve-white TRUE)
        (gimp-drawable-invert mask #f))

    (gimp-context-set-background '(255 255 255))
    (gimp-selection-none img)
    (gimp-drawable-edit-fill layer2 FILL-BACKGROUND)
    (gimp-drawable-edit-fill layer3 FILL-BACKGROUND)
    (gimp-drawable-edit-clear shadow)

    (gimp-item-set-visible bg-layer #f)
    (gimp-item-set-visible shadow #f)

    (gimp-image-select-item img CHANNEL-OP-REPLACE mask)
    (gimp-context-set-background '(0 0 0))
    (gimp-selection-translate img offx1 offy1)
    (gimp-selection-feather img feather)
    (gimp-drawable-edit-fill layer2 FILL-BACKGROUND)
    (gimp-selection-translate img (* 2 offx2) (* 2 offy2))
    (gimp-drawable-edit-fill layer3 FILL-BACKGROUND)
    (gimp-selection-none img)
    (set! layer2 (gimp-image-merge-visible-layers img CLIP-TO-IMAGE))
    (gimp-drawable-invert layer2 #f)

    (gimp-image-insert-layer img layer1 0 0)
    (copy-layer-chrome-it img layer1 banding-img banding-layer)
    (gimp-image-delete banding-img)
    (gimp-layer-scale layer1 width height #f)
    (gimp-drawable-merge-new-filter layer1 "gegl:gaussian-blur" 0 LAYER-MODE-REPLACE 1.0 "std-dev-x" 3.2 "std-dev-y" 3.2 "filter" "auto")
    (gimp-layer-set-opacity layer1 50)
    (set! layer1 (gimp-image-merge-visible-layers img CLIP-TO-IMAGE))
    (gimp-drawable-curves-spline layer1 HISTOGRAM-VALUE (spline-chrome-it))

    (set! layer-mask (gimp-layer-create-mask layer1 ADD-MASK-BLACK))
    (gimp-layer-add-mask layer1 layer-mask)
    (gimp-image-select-item img CHANNEL-OP-REPLACE mask)
    (gimp-context-set-background '(255 255 255))
    (gimp-drawable-edit-fill layer-mask FILL-BACKGROUND)

    (set! layer2 (gimp-layer-copy layer1))
    (gimp-layer-add-alpha layer2)
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

    (gimp-drawable-color-balance layer1 TRANSFER-SHADOWS #t
				 (shadows (rval hc))
				 (shadows (gval hc))
				 (shadows (bval hc)))
    (gimp-drawable-color-balance layer1 TRANSFER-MIDTONES #t
				 (midtones (rval hc))
				 (midtones (gval hc))
				 (midtones (bval hc)))
    (gimp-drawable-color-balance layer1 TRANSFER-HIGHLIGHTS #t
				 (highlights (rval hc))
				 (highlights (gval hc))
				 (highlights (bval hc)))

    (gimp-drawable-color-balance layer2 TRANSFER-SHADOWS #t
				 (shadows (rval cc))
				 (shadows (gval cc))
				 (shadows (bval cc)))
    (gimp-drawable-color-balance layer2 TRANSFER-MIDTONES #t
				 (midtones (rval cc))
				 (midtones (gval cc))
				 (midtones (bval cc)))
    (gimp-drawable-color-balance layer2 TRANSFER-HIGHLIGHTS #t
				 (highlights (rval cc))
				 (highlights (gval cc))
				 (highlights (bval cc)))
    (gimp-drawable-hue-saturation layer2 HUE-RANGE-ALL
				  0.0
				  chrome-lightness
				  chrome-saturation
				  0.0)

    (gimp-item-set-visible shadow #t)
    (gimp-item-set-visible bg-layer #t)

    (gimp-item-set-name layer2 _"Chrome")
    (gimp-item-set-name layer1 _"Highlight")

    (gimp-image-remove-channel img mask)

    (gimp-resource-delete brush)

    (gimp-display-new img)
    (gimp-image-undo-enable img)

    (gimp-context-pop)
  )
)

; FIXME the description is confusing:
; 1. Filter renders a new image and does not "add ... to selected region"
; 2. It is unclear that "specified..stencil" refers to the source image
;    and doesn't mean the secondary "Environment map"
; 3. The original did not permit an alpha so "(or alpha)" is unclear
; Suggest: "Renders a new image having a chrome effect on shapes in the image."
(script-fu-register-filter "script-fu-sota-chrome-it"
  _"Stencil C_hrome..."
  _"Add a chrome effect to the selected region (or alpha) using a specified (grayscale) stencil"
  "Spencer Kimball"
  "Spencer Kimball"
  "1997"
  "*"
  SF-ONE-OR-MORE-DRAWABLE
  SF-ADJUSTMENT _"Chrome saturation" '(-80 -100 100 1 10 0 0)
  SF-ADJUSTMENT _"Chrome lightness"  '(-47 -100 100 1 10 0 0)
  SF-ADJUSTMENT _"Chrome factor"     '(0.75 0 1 0.1 0.2 2 0)
  SF-FILENAME   _"Environment map"
                (string-append gimp-data-directory
                              "/scripts/images/beavis.jpg")
  SF-COLOR      _"Highlight balance"  '(211 95 0)
  SF-COLOR      _"Chrome balance"     "black"
  SF-TOGGLE     _"Chrome white areas" #t
)

(script-fu-menu-register "script-fu-sota-chrome-it"
                         "<Image>/Filters/Decor")
