; CLOTHIFY version 1.02
; Gives the current layer in the indicated image a cloth-like texture.
; Process invented by Zach Beane (Xath@irc.gimp.net)
;
; Tim Newsome <drz@froody.bloke.com> 4/11/97

(define (script-fu-clothify timg tdrawable bx by azimuth elevation depth)
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

    (plug-in-gauss-rle RUN-NONINTERACTIVE img layer-one bx TRUE FALSE)
    (plug-in-gauss-rle RUN-NONINTERACTIVE img layer-two by FALSE TRUE)
    (gimp-image-flatten img)
    (set! bump-layer (aref (cadr (gimp-image-get-selected-layers img)) 0))

    (plug-in-c-astretch RUN-NONINTERACTIVE img bump-layer)
    (plug-in-noisify RUN-NONINTERACTIVE img bump-layer FALSE 0.2 0.2 0.2 0.2)

    (plug-in-bump-map RUN-NONINTERACTIVE img tdrawable bump-layer azimuth elevation depth 0 0 0 0 FALSE FALSE 0)
    (gimp-image-delete img)
    (gimp-displays-flush)

    (gimp-context-pop)
  )
)


(script-fu-register "script-fu-clothify"
  _"_Clothify..."
  _"Add a cloth-like texture to the selected region (or alpha)"
  "Tim Newsome <drz@froody.bloke.com>"
  "Tim Newsome"
  "4/11/97"
  "RGB* GRAY*"
  SF-IMAGE       "Input image"    0
  SF-DRAWABLE    "Input drawable" 0
  SF-ADJUSTMENT _"Blur X"         '(9 3 100 1 10 0 1)
  SF-ADJUSTMENT _"Blur Y"         '(9 3 100 1 10 0 1)
  SF-ADJUSTMENT _"Azimuth"        '(135 0 360 1 10 1 0)
  SF-ADJUSTMENT _"Elevation"      '(45 0 90 1 10 1 0)
  SF-ADJUSTMENT _"Depth"          '(3 1 50 1 10 0 1)
)

(script-fu-menu-register "script-fu-clothify"
                         "<Image>/Filters/Artistic")
