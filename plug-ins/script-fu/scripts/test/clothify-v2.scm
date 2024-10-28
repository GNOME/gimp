


; !!!! This is the version from v2, one drawable
; For testing only.




(define (script-fu-clothify-old timg tdrawable bx by azimuth elevation depth)
  (let* (
        (width (car (gimp-drawable-get-width tdrawable)))
        (height (car (gimp-drawable-get-height tdrawable)))
        (img (car (gimp-image-new width height RGB)))
;       (layer-two (car (gimp-layer-new img width height RGB-IMAGE "Y Dots" 100 LAYER-MODE-MULTIPLY)))
        (layer-one (car (gimp-layer-new img width height RGB-IMAGE "X Dots" 100 LAYER-MODE-NORMAL)))
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

    (set! layer-two (car (gimp-layer-copy layer-one 0)))
    (gimp-layer-set-mode layer-two LAYER-MODE-MULTIPLY)
    (gimp-image-insert-layer img layer-two 0 0)

    (plug-in-gauss RUN-NONINTERACTIVE img layer-one (* 0.32 bx) 0 0)
    (plug-in-gauss RUN-NONINTERACTIVE img layer-two 0 (* 0.32 by) 0)
    (gimp-image-flatten img)
    (set! bump-layer (vector-ref (cadr (gimp-image-get-selected-layers img)) 0))

    (plug-in-c-astretch RUN-NONINTERACTIVE img bump-layer)
    (plug-in-noisify RUN-NONINTERACTIVE img bump-layer FALSE 0.2 0.2 0.2 0.2)

    (plug-in-bump-map RUN-NONINTERACTIVE img tdrawable bump-layer azimuth elevation depth 0 0 0 0 FALSE FALSE 0)
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
  SF-ADJUSTMENT "Elevation"      '(45 0 90 1 10 1 0)
  SF-ADJUSTMENT "Depth"          '(3 1 50 1 10 0 1)
)

; !!! in Demo menu
(script-fu-menu-register "script-fu-clothify-old"
                         "<Image>/Filters/Development/Demos")
