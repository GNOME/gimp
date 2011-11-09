;  BLENDED-DROP-SHADOW-LOGO
;  draw the specified text over a blended background using current gimp fg
;   and bg colors.  The finished blend has a drop shadow underneath that blends
;   to the specified bg-color
;  if the blend colors are specified as high intensity, the sharp option
;   should be enabled or the logo will come out blurry

(define (blended-logo-scale size percent)
  (* size percent)
)

(define (apply-blended-logo-effect img
                                   logo-layer
                                   b-size
                                   bg-color
                                   blend-mode
                                   blend-fg
                                   blend-bg
                                   blend-gradient
                                   blend-gradient-reverse)
  (let* (
        (b-size-2 (blended-logo-scale b-size 0.5))
        (f-size (blended-logo-scale b-size 0.75))
        (ds-size (blended-logo-scale b-size 0.5))
        (ts-size (- b-size-2 3))
        (width (car (gimp-drawable-width logo-layer)))
        (height (car (gimp-drawable-height logo-layer)))
        (blend-layer (car (gimp-layer-new img
                                          width height RGBA-IMAGE
                                          "Blend" 100 NORMAL-MODE)))
        (shadow-layer (car (gimp-layer-new img
                                           width height RGBA-IMAGE
                                           "Shadow" 100 NORMAL-MODE)))
        (text-shadow-layer (car (gimp-layer-new img
                                                width height RGBA-IMAGE
                                                "Text Shadow" 100 MULTIPLY-MODE)))
        (tsl-layer-mask (car (gimp-layer-create-mask text-shadow-layer
                                                     ADD-BLACK-MASK)))
        (drop-shadow-layer (car (gimp-layer-new img
                                                width height RGBA-IMAGE
                                                "Drop Shadow" 100 MULTIPLY-MODE)))
        (dsl-layer-mask (car (gimp-layer-create-mask drop-shadow-layer
                                                     ADD-BLACK-MASK)))
        )

    (script-fu-util-image-resize-from-layer img logo-layer)
    (script-fu-util-image-add-layers img text-shadow-layer drop-shadow-layer blend-layer shadow-layer)
    (gimp-image-raise-item img text-shadow-layer)
    (gimp-selection-none img)
    (gimp-edit-clear text-shadow-layer)
    (gimp-edit-clear drop-shadow-layer)
    (gimp-edit-clear blend-layer)
    (gimp-context-set-background bg-color)
    (gimp-drawable-fill shadow-layer BACKGROUND-FILL)
    (gimp-context-set-feather TRUE)
    (gimp-context-set-feather-radius b-size-2 b-size-2)
    (gimp-image-select-rectangle img CHANNEL-OP-REPLACE b-size-2 b-size-2 (- width b-size) (- height b-size))
    (gimp-context-set-feather FALSE)
    (gimp-context-set-background '(0 0 0))
    (gimp-edit-fill shadow-layer BACKGROUND-FILL)
    (gimp-image-select-item img CHANNEL-OP-REPLACE logo-layer)
    (gimp-layer-add-mask text-shadow-layer tsl-layer-mask)
    (gimp-context-set-background '(255 255 255))
    (gimp-edit-fill tsl-layer-mask BACKGROUND-FILL)
    (gimp-selection-feather img f-size)
    (gimp-context-set-background '(63 63 63))
    (gimp-edit-fill drop-shadow-layer BACKGROUND-FILL)
    (gimp-context-set-background '(0 0 0))
    (gimp-edit-fill text-shadow-layer BACKGROUND-FILL)
    (gimp-context-set-foreground '(255 255 255))

    (gimp-edit-blend text-shadow-layer FG-BG-RGB-MODE NORMAL-MODE
                     GRADIENT-SHAPEBURST-ANGULAR 100 0 REPEAT-NONE FALSE
                     FALSE 0 0 TRUE
                     0 0 1 1)

    (gimp-selection-none img)
    (gimp-context-set-foreground blend-fg)
    (gimp-context-set-background blend-bg)
    (gimp-context-set-gradient blend-gradient)

    (gimp-edit-blend blend-layer blend-mode NORMAL-MODE
                     GRADIENT-LINEAR 100 0 REPEAT-NONE blend-gradient-reverse
                     FALSE 0 0 TRUE
                     0 0 width 0)

    (gimp-layer-translate logo-layer (- b-size-2) (- b-size-2))
    (gimp-layer-translate blend-layer (- b-size) (- b-size))
    (gimp-layer-translate text-shadow-layer (- ts-size) (- ts-size))
    (gimp-layer-translate drop-shadow-layer ds-size ds-size)
    (gimp-image-select-item img CHANNEL-OP-REPLACE blend-layer)
    (gimp-layer-add-mask drop-shadow-layer dsl-layer-mask)
    (gimp-context-set-background '(255 255 255))
    (gimp-edit-fill dsl-layer-mask BACKGROUND-FILL)
    (gimp-layer-remove-mask drop-shadow-layer MASK-APPLY)
    (gimp-selection-none img)
  )
)

(define (script-fu-blended-logo-alpha img
                                      logo-layer
                                      b-size
                                      bg-color
                                      blend-mode
                                      blend-fg
                                      blend-bg
                                      blend-gradient
                                      blend-gradient-reverse)
  (begin
    (gimp-context-push)
    (gimp-context-set-defaults)

    (gimp-image-undo-group-start img)
    (apply-blended-logo-effect img logo-layer b-size bg-color
                               blend-mode blend-fg blend-bg
                               blend-gradient blend-gradient-reverse)
    (gimp-image-undo-group-end img)
    (gimp-displays-flush)

    (gimp-context-pop)
  )
)


(script-fu-register "script-fu-blended-logo-alpha"
    _"Blen_ded..."
    _"Add blended backgrounds, highlights, and shadows to the selected region (or alpha)"
    "Spencer Kimball"
    "Spencer Kimball"
    "1996"
    "RGBA"
    SF-IMAGE      "Image"             0
    SF-DRAWABLE   "Drawable"          0
    SF-ADJUSTMENT _"Offset (pixels)"  '(15 1 100 1 10 0 1)
    SF-COLOR      _"Background color" "white"
    SF-OPTION     _"Blend mode"       '(_"FG-BG-RGB"
                                        _"FG-BG-HSV"
                                        _"FG-Transparent"
                                        _"Custom Gradient")
    SF-COLOR      _"Start blend"      '(22 9 129)
    SF-COLOR      _"End blend"        '(129 9 82)
    SF-GRADIENT   _"Gradient"         "Golden"
    SF-TOGGLE     _"Gradient reverse" FALSE
)

(script-fu-menu-register "script-fu-blended-logo-alpha"
                         "<Image>/Filters/Alpha to Logo")


(define (script-fu-blended-logo text
                                size
                                font
                                text-color
                                bg-color
                                blend-mode
                                blend-fg
                                blend-bg
                                blend-gradient
                                blend-gradient-reverse)
  (let* (
        (img (car (gimp-image-new 256 256 RGB)))
        (b-size (blended-logo-scale size 0.1))
        (text-layer (car (gimp-text-fontname img -1 0 0 text b-size TRUE size PIXELS font)))
        )
    (gimp-context-push)
    (gimp-context-set-antialias TRUE)
    (gimp-context-set-feather FALSE)

    (gimp-image-undo-disable img)
    (gimp-context-set-foreground text-color)
    (gimp-layer-set-lock-alpha text-layer TRUE)
    (gimp-edit-fill text-layer FOREGROUND-FILL)
    (apply-blended-logo-effect img text-layer b-size bg-color
                               blend-mode blend-fg blend-bg
                               blend-gradient blend-gradient-reverse)
    (gimp-image-undo-enable img)
    (gimp-display-new img)

    (gimp-context-pop)
  )
)

(script-fu-register "script-fu-blended-logo"
    _"Blen_ded..."
    _"Create a logo with blended backgrounds, highlights, and shadows"
    "Spencer Kimball"
    "Spencer Kimball"
    "1996"
    ""
    SF-STRING     _"Text"               "GIMP"
    SF-ADJUSTMENT _"Font size (pixels)" '(150 2 1000 1 10 0 1)
    SF-FONT       _"Font"               "Crillee"
    SF-COLOR      _"Text color"         '(124 174 255)
    SF-COLOR      _"Background color"   "white"
    SF-OPTION     _"Blend mode"         '(_"FG-BG-RGB"
                                          _"FG-BG-HSV"
                                          _"FG-Transparent"
                                          _"Custom Gradient")
    SF-COLOR      _"Start blend"        '(22 9 129)
    SF-COLOR      _"End blend"          '(129 9 82)
    SF-GRADIENT   _"Gradient"           "Golden"
    SF-TOGGLE     _"Gradient reverse"   FALSE
)

(script-fu-menu-register "script-fu-blended-logo"
                         "<Image>/File/Create/Logos")
