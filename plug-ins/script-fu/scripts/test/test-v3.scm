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
;       (layer-two (gimp-layer-new img width height RGB-IMAGE "Y Dots" 100 LAYER-MODE-MULTIPLY))
        (layer-one (gimp-layer-new img width height RGB-IMAGE "X Dots" 100 LAYER-MODE-NORMAL))
        (layer-two 0)
        (bump-layer 0)
        )

    (gimp-context-push)
    (gimp-context-set-defaults)

    (gimp-image-undo-disable img)

    (gimp-image-insert-layer img layer-one 0 0)

    (gimp-context-set-background '(255 255 255))
    (gimp-drawable-edit-fill layer-one FILL-BACKGROUND)

    (plug-in-noisify RUN-NONINTERACTIVE img layer-one FALSE 0.7 0.7 0.7 0.7)

    (set! layer-two (gimp-layer-copy layer-one 0))
    (gimp-layer-set-mode layer-two LAYER-MODE-MULTIPLY)
    (gimp-image-insert-layer img layer-two 0 0)

    (plug-in-gauss-rle RUN-NONINTERACTIVE img layer-one bx TRUE FALSE)
    (plug-in-gauss-rle RUN-NONINTERACTIVE img layer-two by FALSE TRUE)
    (gimp-image-flatten img)
    ; Note the inner call returns (<numeric length> <vector>)
    ; We still need cadr even in v3 language
    (set! bump-layer (aref (cadr (gimp-image-get-selected-layers img)) 0))

    (plug-in-c-astretch RUN-NONINTERACTIVE img bump-layer)
    (plug-in-noisify RUN-NONINTERACTIVE img bump-layer FALSE 0.2 0.2 0.2 0.2)

    (plug-in-bump-map RUN-NONINTERACTIVE img tdrawable bump-layer azimuth elevation depth 0 0 0 0 FALSE FALSE 0)
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
  SF-ADJUSTMENT _"Elevation"      '(45 0 90 1 10 1 0)
  SF-ADJUSTMENT _"Depth"          '(3 1 50 1 10 0 1)
)

(script-fu-menu-register "script-fu-testv3"
                         "<Image>/Filters/Development/Demos")
