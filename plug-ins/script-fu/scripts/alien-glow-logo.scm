;  ALIEN-GLOW
;  Create a text effect that simulates an eerie alien glow around text

(define (apply-alien-glow-logo-effect img
                                      logo-layer
                                      size
                                      glow-color)
  (let* (
        (border (/ size 4))
        (grow (/ size 30))
        (feather (/ size 4))
        (width (car (gimp-drawable-width logo-layer)))
        (height (car (gimp-drawable-height logo-layer)))
        (bg-layer (car (gimp-layer-new img
                                       width height RGB-IMAGE
                                       "Background" 100 NORMAL-MODE)))
        (glow-layer (car (gimp-layer-new img
                                         width height RGBA-IMAGE
                                         "Alien Glow" 100 NORMAL-MODE)))
        )

    (gimp-context-push)
    (gimp-context-set-defaults)

    (gimp-selection-none img)
    (script-fu-util-image-resize-from-layer img logo-layer)
    (script-fu-util-image-add-layers img glow-layer bg-layer)
    (gimp-layer-set-lock-alpha logo-layer TRUE)
    (gimp-context-set-background '(0 0 0))
    (gimp-edit-fill bg-layer BACKGROUND-FILL)
    (gimp-edit-clear glow-layer)
    (gimp-image-select-item img CHANNEL-OP-REPLACE logo-layer)
    (gimp-selection-grow img grow)
    (gimp-selection-feather img feather)
    (gimp-context-set-foreground glow-color)
    (gimp-edit-fill glow-layer FOREGROUND-FILL)
    (gimp-selection-none img)

    (gimp-context-set-background '(0 0 0))
    (gimp-context-set-foreground '(79 79 79))

    (gimp-edit-blend logo-layer FG-BG-RGB-MODE NORMAL-MODE
                     GRADIENT-SHAPEBURST-ANGULAR 100 0 REPEAT-NONE FALSE
                     FALSE 0 0 TRUE
                     0 0 1 1)

    (gimp-context-pop)
  )
)


(define (script-fu-alien-glow-logo-alpha img
                                         logo-layer
                                         size
                                         glow-color)
  (begin
    (gimp-image-undo-group-start img)
    (apply-alien-glow-logo-effect img logo-layer size glow-color)
    (gimp-image-undo-group-end img)
    (gimp-displays-flush)
  )
)

(script-fu-register "script-fu-alien-glow-logo-alpha"
  _"Alien _Glow..."
  _"Add an eerie glow around the selected region (or alpha)"
  "Spencer Kimball"
  "Spencer Kimball"
  "1997"
  "RGBA"
  SF-IMAGE       "Image"                  0
  SF-DRAWABLE    "Drawable"               0
  SF-ADJUSTMENT _"Glow size (pixels * 4)" '(150 2 1000 1 10 0 1)
  SF-COLOR      _"Glow color"             '(63 252 0)
)

(script-fu-menu-register "script-fu-alien-glow-logo-alpha"
                         "<Image>/Filters/Alpha to Logo")


(define (script-fu-alien-glow-logo text
                                   size
                                   font
                                   glow-color)
  (let* (
        (img (car (gimp-image-new 256 256 RGB)))
        (border (/ size 4))
        (grow (/ size 30))
        (feather (/ size 4))
        (text-layer (car (gimp-text-fontname img
                                             -1 0 0 text border TRUE
                                             size PIXELS font)))
        (width (car (gimp-drawable-width text-layer)))
        (height (car (gimp-drawable-height text-layer)))
        )

    (gimp-image-undo-disable img)
    (apply-alien-glow-logo-effect img text-layer size glow-color)
    (gimp-image-undo-enable img)
    (gimp-display-new img)
  )
)

(script-fu-register "script-fu-alien-glow-logo"
  _"Alien _Glow..."
  _"Create a logo with an alien glow around the text"
  "Spencer Kimball"
  "Spencer Kimball"
  "1997"
  ""
  SF-STRING     _"Text"               "ALIEN"
  SF-ADJUSTMENT _"Font size (pixels)" '(150 2 1000 1 10 0 1)
  SF-FONT       _"Font"               "Sans Bold"
  SF-COLOR      _"Glow color"         '(63 252 0)
)

(script-fu-menu-register "script-fu-alien-glow-logo"
                         "<Image>/File/Create/Logos")
