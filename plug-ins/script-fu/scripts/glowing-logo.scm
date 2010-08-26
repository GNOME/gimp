;  GLOWING
;  Create a text effect that simulates a glowing hot logo

(define (apply-glowing-logo-effect img
                                   logo-layer
                                   size
                                   bg-color)
  (let* (
        (grow (/ size 4))
        (feather1 (/ size 3))
        (feather2 (/ size 7))
        (feather3 (/ size 10))
        (width (car (gimp-drawable-width logo-layer)))
        (height (car (gimp-drawable-height logo-layer)))
        (posx (- (car (gimp-drawable-offsets logo-layer))))
        (posy (- (cadr (gimp-drawable-offsets logo-layer))))
        (glow-layer (car (gimp-layer-copy logo-layer TRUE)))
        (bg-layer (car (gimp-layer-new img width height RGB-IMAGE "Background" 100 NORMAL-MODE)))
        )

    (gimp-context-push)

    (script-fu-util-image-resize-from-layer img logo-layer)
    (script-fu-util-image-add-layers img glow-layer bg-layer)
    (gimp-layer-translate glow-layer posx posy)

    (gimp-selection-none img)
    (gimp-context-set-background bg-color)
    (gimp-edit-fill bg-layer BACKGROUND-FILL)

    (gimp-layer-set-lock-alpha logo-layer TRUE)
    (gimp-context-set-background '(0 0 0))
    (gimp-edit-fill logo-layer BACKGROUND-FILL)

    (gimp-selection-layer-alpha logo-layer)
    (gimp-selection-feather img feather1)
    (gimp-context-set-background '(221 0 0))
    (gimp-edit-fill glow-layer BACKGROUND-FILL)
    (gimp-edit-fill glow-layer BACKGROUND-FILL)
    (gimp-edit-fill glow-layer BACKGROUND-FILL)

    (gimp-selection-layer-alpha logo-layer)
    (gimp-selection-feather img feather2)
    (gimp-context-set-background '(232 217 18))
    (gimp-edit-fill glow-layer BACKGROUND-FILL)
    (gimp-edit-fill glow-layer BACKGROUND-FILL)

    (gimp-selection-layer-alpha logo-layer)
    (gimp-selection-feather img feather3)
    (gimp-context-set-background '(255 255 255))
    (gimp-edit-fill glow-layer BACKGROUND-FILL)
    (gimp-selection-none img)

    (gimp-layer-set-mode logo-layer OVERLAY-MODE)
    (gimp-item-set-name glow-layer "Glow Layer")

    (gimp-context-pop)
  )
)


(define (script-fu-glowing-logo-alpha img
                                      logo-layer
                                      size
                                      bg-color)
  (begin
    (gimp-image-undo-group-start img)
    (apply-glowing-logo-effect img logo-layer (* size 3) bg-color)
    (gimp-image-undo-group-end img)
    (gimp-displays-flush)
  )
)

(script-fu-register "script-fu-glowing-logo-alpha"
  _"Glo_wing Hot..."
  _"Add a glowing hot metal effect to the selected region (or alpha)"
  "Spencer Kimball"
  "Spencer Kimball"
  "1997"
  "RGBA"
  SF-IMAGE      "Image"                 0
  SF-DRAWABLE   "Drawable"              0
  SF-ADJUSTMENT _"Effect size (pixels)" '(50 1 500 1 10 0 1)
  SF-COLOR      _"Background color"     '(7 0 20)
)

(script-fu-menu-register "script-fu-glowing-logo-alpha"
                         "<Image>/Filters/Alpha to Logo")


(define (script-fu-glowing-logo text
                                size
                                font
                                bg-color)
  (let* (
        (img (car (gimp-image-new 256 256 RGB)))
        (border (/ size 4))
        (text-layer (car (gimp-text-fontname img -1 0 0 text border TRUE size PIXELS font)))
        )
    (gimp-image-undo-disable img)
    (apply-glowing-logo-effect img text-layer size bg-color)
    (gimp-image-undo-enable img)
    (gimp-display-new img)
  )
)

(script-fu-register "script-fu-glowing-logo"
  _"Glo_wing Hot..."
  _"Create a logo that looks like glowing hot metal"
  "Spencer Kimball"
  "Spencer Kimball"
  "1997"
  ""
  SF-STRING     _"Text"               "GLOWING"
  SF-ADJUSTMENT _"Font size (pixels)" '(150 2 1000 1 10 0 1)
  SF-FONT       _"Font"               "Slogan"
  SF-COLOR      _"Background color"   '(7 0 20)
)

(script-fu-menu-register "script-fu-glowing-logo"
                         "<Image>/File/Create/Logos")
