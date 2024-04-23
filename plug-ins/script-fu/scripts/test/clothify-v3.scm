; CLOTHIFY version 1.02
; Gives the current layer in the indicated image a cloth-like texture.
; Process invented by Zach Beane (Xath@irc.gimp.net)
;
; Tim Newsome <drz@froody.bloke.com> 4/11/97

; v3>>> Adapted to take many drawables, but only handle the first
; v3>>> drawables is-a vector, and there is no formal arg for its length i.e.  n_drawables

(define (script-fu-clothify-v3 timg drawables bx by azimuth elevation depth)
  (let* (
        (tdrawable (aref drawables 0))  v3>>> only the first drawable
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
    (set! bump-layer (car (gimp-image-get-active-layer img)))

    (plug-in-c-astretch RUN-NONINTERACTIVE img bump-layer)
    (plug-in-noisify RUN-NONINTERACTIVE img bump-layer FALSE 0.2 0.2 0.2 0.2)

    (plug-in-bump-map RUN-NONINTERACTIVE img tdrawable bump-layer azimuth elevation depth 0 0 0 0 FALSE FALSE 0)
    (gimp-image-delete img)
    (gimp-displays-flush)

    (gimp-context-pop)

    ; well-behaved requires error if more than one drawable
    ( if (> (vector-length drawables) 1 )
         (begin
            ; Msg to status bar, need not be acknowledged by any user
            (gimp-message "Received more than one drawable.")
            ; Msg propagated in a GError to Gimp's error dialog that must be acknowledged
            (write "Received more than one drawable.")
            ; Indicate err to programmed callers
            #f)
         #t
    )
  )
)

; v3 >>> no image or drawable declared.
; v3 >>> SF-ONE-DRAWABLE means contracts to process only one drawable
(script-fu-register-filter "script-fu-clothify-v3"
  _"_Clothify v3..."
  _"Add a cloth-like texture to the selected region (or alpha)"
  "Tim Newsome <drz@froody.bloke.com>"
  "Tim Newsome"
  "4/11/97"
  "RGB* GRAY*"
  SF-ONE-DRAWABLE
  SF-ADJUSTMENT _"Blur X"         '(9 3 100 1 10 0 1)
  SF-ADJUSTMENT _"Blur Y"         '(9 3 100 1 10 0 1)
  SF-ADJUSTMENT _"Azimuth"        '(135 0 360 1 10 1 0)
  SF-ADJUSTMENT _"Elevation"      '(45 0 90 1 10 1 0)
  SF-ADJUSTMENT _"Depth"          '(3 1 50 1 10 0 1)
)

(script-fu-menu-register "script-fu-clothify-v3"
                         "<Image>/Filters/Development/Demos")
