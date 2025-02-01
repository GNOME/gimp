


; !!!! This is the version from v2, one drawable
; For testing only.




(define (script-fu-clothify-old timg tdrawable bx by azimuth elevation depth)
  (let* (
        (width (car (gimp-drawable-get-width tdrawable)))
        (height (car (gimp-drawable-get-height tdrawable)))
        (img (car (gimp-image-new width height RGB)))
;       (layer-two (car (gimp-layer-new img "Y Dots" width height RGB-IMAGE 100 LAYER-MODE-MULTIPLY)))
        (layer-one (car (gimp-layer-new img "X Dots" width height RGB-IMAGE 100 LAYER-MODE-NORMAL)))
        (layer-two 0)
        (bump-layer 0)
        )

    (gimp-context-push)
    (gimp-context-set-defaults)

    (gimp-image-undo-disable img)

    (gimp-image-insert-layer img layer-one 0 0)

    (gimp-context-set-background '(255 255 255))
    (gimp-drawable-edit-fill layer-one FILL-BACKGROUND)

    (gimp-drawable-merge-new-filter layer-one "gegl:noise-rgb" 0 LAYER-MODE-REPLACE 1.0
                                    "independent" FALSE "red" 0.7 "alpha" 0.7
                                    "correlated" FALSE "seed" (msrg-rand) "linear" TRUE)

    (set! layer-two (car (gimp-layer-copy layer-one)))
    (gimp-layer-set-mode layer-two LAYER-MODE-MULTIPLY)
    (gimp-image-insert-layer img layer-two 0 0)

    (gimp-drawable-merge-new-filter layer-one "gegl:gaussian-blur" 0 LAYER-MODE-REPLACE 1.0 "std-dev-x" (* 0.32 bx) "std-dev-y" 0.0 "filter" "auto")
    (gimp-drawable-merge-new-filter layer-two "gegl:gaussian-blur" 0 LAYER-MODE-REPLACE 1.0 "std-dev-x" 0.0 "std-dev-y" (* 0.32 by) "filter" "auto")
    (gimp-image-flatten img)
    ; No container length returned since 3.0rc1
    (set! bump-layer (vector-ref (car (gimp-image-get-selected-layers img)) 0))

    (gimp-drawable-merge-new-filter bump-layer "gegl:stretch-contrast" 0 LAYER-MODE-REPLACE 1.0 "keep-colors" FALSE)
    (gimp-drawable-merge-new-filter bump-layer "gegl:noise-rgb" 0 LAYER-MODE-REPLACE 1.0
                                    "independent" FALSE "red" 0.2 "alpha" 0.2
                                    "correlated" FALSE "seed" (msrg-rand) "linear" TRUE)

    (let* ((filter (car (gimp-drawable-filter-new tdrawable "gegl:bump-map" ""))))
      (gimp-drawable-filter-configure filter LAYER-MODE-REPLACE 1.0
                                      "azimuth" azimuth "elevation" elevation "depth" depth
                                      "offset-x" 0 "offset-y" 0 "waterlevel" 0.0 "ambient" 0.0
                                      "compensate" FALSE "invert" FALSE "type" "linear"
                                      "tiled" FALSE)
      (gimp-drawable-filter-set-aux-input filter "aux" bump-layer)
      (gimp-drawable-merge-filter tdrawable filter)
    )
    (gimp-image-delete img)
    (gimp-displays-flush)

    (gimp-context-pop)
  )
)

; !!! deprecated register function
(script-fu-register "script-fu-clothify-old"
  "Clothify v2..."
  "Add a cloth-like texture to the selected region (or alpha)"
  "Tim Newsome <drz@froody.bloke.com>"
  "Tim Newsome"
  "4/11/97"
  "RGB* GRAY*"
  ; !!! no sensitivity
  SF-IMAGE       "image"         0
  ; !!! one drawable
  SF-DRAWABLE    "drawable"      0
  SF-ADJUSTMENT "Blur X"         '(9 3 100 1 10 0 1)
  SF-ADJUSTMENT "Blur Y"         '(9 3 100 1 10 0 1)
  SF-ADJUSTMENT "Azimuth"        '(135 0 360 1 10 1 0)
  SF-ADJUSTMENT "Elevation"      '(45 0.5 90 1 10 1 0)
  SF-ADJUSTMENT "Depth"          '(3 1 50 1 10 0 1)
)

; !!! in Demo menu
(script-fu-menu-register "script-fu-clothify-old"
                         "<Image>/Filters/Development/Demos")
