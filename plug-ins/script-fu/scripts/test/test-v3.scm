; Old style script testing v3 binding of return values from PDB

; Modified from clothify.scm
; 1. Calls (script-fu-use-v3) very early
; 2. Uses v3 binding for PDB returns, i.e. elides many (car ()) from original

; Expect:
;   this works
;   after this plugin is invoked, all other /scripts still work, when invoked by menu item

; !!! You can't do this at top level, it affects all scripts in extension-script-fu
; (script-fu-use-v3)

(define (script-fu-testv3 timg tdrawable bx by azimuth elevation depth)
  (script-fu-use-v3)
  (let* (
        (width (gimp-drawable-get-width tdrawable))
        (height (gimp-drawable-get-height tdrawable))
        (img (gimp-image-new width height RGB))
;       (layer-two (gimp-layer-new img "Y Dots" width height RGB-IMAGE 100 LAYER-MODE-MULTIPLY))
        (layer-one (gimp-layer-new img "X Dots" width height RGB-IMAGE 100 LAYER-MODE-NORMAL))
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

    (set! layer-two (gimp-layer-copy layer-one))
    (gimp-layer-set-mode layer-two LAYER-MODE-MULTIPLY)
    (gimp-image-insert-layer img layer-two 0 0)

    (gimp-drawable-merge-new-filter layer-one "gegl:gaussian-blur" 0 LAYER-MODE-REPLACE 1.0
                                    "std-dev-x" (* 0.32 bx) "std-dev-y" 0.0 "filter" "auto")
    (gimp-drawable-merge-new-filter layer-two "gegl:gaussian-blur" 0 LAYER-MODE-REPLACE 1.0
                                    "std-dev-x" 0.0 "std-dev-y" (* 0.32 by) "filter" "auto")
    (gimp-image-flatten img)
    ; Note the inner call returns (<numeric length> <vector>)
    ; We still need cadr even in v3 language
    (set! bump-layer (vector-ref (cadr (gimp-image-get-selected-layers img)) 0))

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


(script-fu-register "script-fu-testv3"
  _"Test script-fu-use-v3..."
  _"Test use of script-fu-use-v3 in old scripts"
  "lloyd konneker"
  ""
  "2023"
  "RGB* GRAY*"
  SF-IMAGE       "Input image"    0
  SF-DRAWABLE    "Input drawable" 0
  SF-ADJUSTMENT _"Blur X"         '(9 3 100 1 10 0 1)
  SF-ADJUSTMENT _"Blur Y"         '(9 3 100 1 10 0 1)
  SF-ADJUSTMENT _"Azimuth"        '(135 0 360 1 10 1 0)
  SF-ADJUSTMENT _"Elevation"      '(45 0.5 90 1 10 1 0)
  SF-ADJUSTMENT _"Depth"          '(3 1 50 1 10 0 1)
)

(script-fu-menu-register "script-fu-testv3"
                         "<Image>/Filters/Development/Demos")
