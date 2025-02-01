; CLOTHIFY version 1.02
; Gives the current layer in the indicated image a cloth-like texture.
; Process invented by Zach Beane (Xath@irc.gimp.net)
;
; Tim Newsome <drz@froody.bloke.com> 4/11/97

; v3>>> Adapted to take many drawables, but only handle the first
; v3>>> drawables is-a vector, and there is no formal arg for its length i.e.  n_drawables

(define (script-fu-clothify-v3 timg drawables bx by azimuth elevation depth)
  (let* (
        (tdrawable (vector-ref drawables 0))  v3>>> only the first drawable
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

    (gimp-drawable-merge-new-filter layer-one "gegl:gaussian-blur" 0 LAYER-MODE-REPLACE 1.0
                                    "std-dev-x" (* 0.32 bx) "std-dev-y" 0.0 "filter" "auto")
    (gimp-drawable-merge-new-filter layer-two "gegl:gaussian-blur" 0 LAYER-MODE-REPLACE 1.0
                                    "std-dev-x" 0.0 "std-dev-y" (* 0.32 by) "filter" "auto")
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
  SF-ADJUSTMENT _"Elevation"      '(45 0.5 90 1 10 1 0)
  SF-ADJUSTMENT _"Depth"          '(3 1 50 1 10 0 1)
)

(script-fu-menu-register "script-fu-clothify-v3"
                         "<Image>/Filters/Development/Demos")
