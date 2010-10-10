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
      a
    )
  )

  (define (brush brush-size)
    (cond ((<= brush-size 5) "Circle Fuzzy (05)")
          ((<= brush-size 7) "Circle Fuzzy (07)")
          ((<= brush-size 9) "Circle Fuzzy (09)")
          ((<= brush-size 11) "Circle Fuzzy (11)")
          ((<= brush-size 13) "Circle Fuzzy (13)")
          ((<= brush-size 15) "Circle Fuzzy (15)")
          ((<= brush-size 17) "Circle Fuzzy (17)")
          (else "Circle Fuzzy (19)")
    )
  )

  (define (shadows val)
    (/ (* 0.96 val) 2.55)
  )

  (define (midtones val)
    (/ val 2.55)
  )

  (define (highlights val)
    ; The result is used as "gimp-color-balance" color parameter
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
    (gimp-edit-clear dest-drawable)
    (gimp-selection-none dest-image)
    (gimp-selection-all source-image)
    (gimp-edit-copy source-drawable)
    (let (
         (floating-sel (car (gimp-edit-paste dest-drawable FALSE)))
         )
         (gimp-floating-sel-anchor floating-sel)
    )
  )

  (let* (
        (banding-img (car (gimp-file-load RUN-NONINTERACTIVE env-map env-map)))
        (banding-layer (car (gimp-image-get-active-drawable banding-img)))
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
        (bg-layer (car (gimp-layer-new img width height GRAY-IMAGE _"Background" 100 NORMAL-MODE)))
        (layer1 (car (gimp-layer-new img banding-width banding-height banding-type _"Layer1" 100 NORMAL-MODE)))
        (layer2 (car (gimp-layer-new img width height GRAYA-IMAGE _"Layer 2" 100 DIFFERENCE-MODE)))
        (layer3 (car (gimp-layer-new img width height GRAYA-IMAGE _"Layer 3" 100 NORMAL-MODE)))
        (shadow (car (gimp-layer-new img width height GRAYA-IMAGE _"Drop Shadow" 100 NORMAL-MODE)))
        (mask-fs 0)
        (layer-mask 0)
        )

    (gimp-context-push)

    (gimp-image-undo-disable img)

    (gimp-image-insert-channel img mask -1 0)
    (gimp-image-insert-layer img bg-layer -1 0)
    (gimp-image-insert-layer img shadow -1 0)
    (gimp-image-insert-layer img layer3 -1 0)
    (gimp-image-insert-layer img layer2 -1 0)

    (gimp-edit-copy mask-drawable)
    (set! mask-fs (car (gimp-edit-paste mask FALSE)))
    (gimp-floating-sel-anchor mask-fs)
    (if (= carve-white FALSE)
        (gimp-invert mask)
    )

    (gimp-context-set-background '(255 255 255))
    (gimp-selection-none img)
    (gimp-edit-fill layer2 BACKGROUND-FILL)
    (gimp-edit-fill layer3 BACKGROUND-FILL)
    (gimp-edit-clear shadow)

    (gimp-item-set-visible bg-layer FALSE)
    (gimp-item-set-visible shadow FALSE)

    (gimp-item-to-selection mask CHANNEL-OP-REPLACE)
    (gimp-context-set-background '(0 0 0))
    (gimp-selection-translate img offx1 offy1)
    (gimp-selection-feather img feather)
    (gimp-edit-fill layer2 BACKGROUND-FILL)
    (gimp-selection-translate img (* 2 offx2) (* 2 offy2))
    (gimp-edit-fill layer3 BACKGROUND-FILL)
    (gimp-selection-none img)
    (set! layer2 (car (gimp-image-merge-visible-layers img CLIP-TO-IMAGE)))
    (gimp-invert layer2)

    (gimp-image-insert-layer img layer1 -1 0)
    (copy-layer-chrome-it img layer1 banding-img banding-layer)
    (gimp-image-delete banding-img)
    (gimp-layer-scale layer1 width height FALSE)
    (plug-in-gauss-iir RUN-NONINTERACTIVE img layer1 10 TRUE TRUE)
    (gimp-layer-set-opacity layer1 50)
    (set! layer1 (car (gimp-image-merge-visible-layers img CLIP-TO-IMAGE)))
    (gimp-curves-spline layer1 HISTOGRAM-VALUE 18 (spline-chrome-it))

    (set! layer-mask (car (gimp-layer-create-mask layer1 ADD-BLACK-MASK)))
    (gimp-layer-add-mask layer1 layer-mask)
    (gimp-item-to-selection mask CHANNEL-OP-REPLACE)
    (gimp-context-set-background '(255 255 255))
    (gimp-edit-fill layer-mask BACKGROUND-FILL)

    (set! layer2 (car (gimp-layer-copy layer1 TRUE)))
    (gimp-image-insert-layer img layer2 -1 0)
    (gimp-context-set-brush (brush brush-size))
    (gimp-context-set-foreground '(255 255 255))
    (gimp-edit-stroke layer-mask)

    (gimp-context-set-background '(0 0 0))
    (gimp-selection-feather img (* feather 1.5))
    (gimp-selection-translate img (* 2.5 offx1) (* 2.5 offy1))
    (gimp-edit-fill shadow BACKGROUND-FILL)

    (gimp-selection-all img)
    (gimp-context-set-pattern "Marble #1")
    (gimp-edit-bucket-fill bg-layer PATTERN-BUCKET-FILL NORMAL-MODE 100 0 FALSE 0 0)
    (gimp-selection-none img)

    (gimp-image-convert-rgb img)

    (gimp-color-balance layer1 SHADOWS TRUE
                        (shadows (rval hc)) (shadows (gval hc)) (shadows (bval hc)))
    (gimp-color-balance layer1 MIDTONES TRUE
                        (midtones (rval hc)) (midtones (gval hc)) (midtones (bval hc)))
    (gimp-color-balance layer1 HIGHLIGHTS TRUE
                        (highlights (rval hc)) (highlights (gval hc)) (highlights (bval hc)))

    (gimp-color-balance layer2 SHADOWS TRUE
                        (shadows (rval cc)) (shadows (gval cc)) (shadows (bval cc)))
    (gimp-color-balance layer2 MIDTONES TRUE
                        (midtones (rval cc)) (midtones (gval cc)) (midtones (bval cc)))
    (gimp-color-balance layer2 HIGHLIGHTS TRUE
                        (highlights (rval cc)) (highlights (gval cc)) (highlights (bval cc)))
    (gimp-hue-saturation layer2 ALL-HUES 0.0 chrome-lightness chrome-saturation)

    (gimp-item-set-visible shadow TRUE)
    (gimp-item-set-visible bg-layer TRUE)

    (gimp-item-set-name layer2 _"Chrome")
    (gimp-item-set-name layer1 _"Highlight")

    (gimp-image-remove-channel img mask)

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
  SF-ADJUSTMENT _"Chrome factor"     '(0.75 0 1 0.1 0.01 2 0)
  SF-FILENAME   _"Environment map"
                (string-append gimp-data-directory
                              "/scripts/images/beavis.jpg")
  SF-COLOR      _"Highlight balance"  '(211 95 0)
  SF-COLOR      _"Chrome balance"     "black"
  SF-TOGGLE     _"Chrome white areas" TRUE
)

(script-fu-menu-register "script-fu-sota-chrome-it"
                         "<Image>/Filters/Decor")
