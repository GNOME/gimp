;  COOL-METAL
;  Create a text effect that looks like metal with a reflection of
;  the horizon, a reflection of the text in the mirrored ground, and
;  an interesting dropshadow
;  This script was inspired by Rob Malda's 'coolmetal.gif' graphic

(define (apply-cool-metal-logo-effect img
                                      logo-layer
                                      size
                                      bg-color
                                      gradient
                                      gradient-reverse)
  (let* (
        (feather (/ size 5))
        (smear 7.5)
        (period (/ size 3))
        (amplitude (/ size 40))
        (shrink (+ 1 (/ size 30)))
        (depth (/ size 20))
        (width (car (gimp-drawable-width logo-layer)))
        (height (car (gimp-drawable-height logo-layer)))
        (posx (- (car (gimp-drawable-offsets logo-layer))))
        (posy (- (cadr (gimp-drawable-offsets logo-layer))))
        (img-width (+ width (* 0.15 height) 10))
        (img-height (+ (* 1.85 height) 10))
        (bg-layer (car (gimp-layer-new img img-width img-height RGB-IMAGE "Background" 100 NORMAL-MODE)))
        (shadow-layer (car (gimp-layer-new img img-width img-height RGBA-IMAGE "Shadow" 100 NORMAL-MODE)))
        (reflect-layer (car (gimp-layer-new img width height RGBA-IMAGE "Reflection" 100 NORMAL-MODE)))
        (channel 0)
        (fs 0)
        (layer-mask 0)
        )

    (gimp-context-push)

    (gimp-selection-none img)
    (gimp-image-resize img img-width img-height posx posy)
    (script-fu-util-image-add-layers img shadow-layer reflect-layer bg-layer)
    (gimp-layer-set-lock-alpha logo-layer TRUE)

    (gimp-context-set-background bg-color)
    (gimp-edit-fill bg-layer BACKGROUND-FILL)
    (gimp-edit-clear reflect-layer)
    (gimp-context-set-background '(0 0 0))
    (gimp-edit-fill shadow-layer BACKGROUND-FILL)

    (gimp-context-set-gradient gradient)

    (gimp-edit-blend logo-layer CUSTOM-MODE NORMAL-MODE
                     GRADIENT-LINEAR 100 0 REPEAT-NONE gradient-reverse
                     FALSE 0 0 TRUE
                     0 0 0 (+ height 5))

    (gimp-rect-select img 0 (- (/ height 2) feather) img-width (* 2 feather) CHANNEL-OP-REPLACE 0 0)
    (plug-in-gauss-iir RUN-NONINTERACTIVE img logo-layer smear TRUE TRUE)
    (gimp-selection-none img)
    (plug-in-ripple RUN-NONINTERACTIVE img logo-layer period amplitude 1 0 1 TRUE FALSE)
    (gimp-layer-translate logo-layer 5 5)
    (gimp-layer-resize logo-layer img-width img-height 5 5)

    (gimp-selection-layer-alpha logo-layer)
    (set! channel (car (gimp-selection-save img)))
    (gimp-selection-shrink img shrink)
    (gimp-selection-invert img)
    (plug-in-gauss-rle RUN-NONINTERACTIVE img channel feather TRUE TRUE)
    (gimp-selection-layer-alpha logo-layer)
    (gimp-selection-invert img)
    (gimp-context-set-background '(0 0 0))
    (gimp-edit-fill channel BACKGROUND-FILL)
    (gimp-selection-none img)

    (plug-in-bump-map RUN-NONINTERACTIVE img logo-layer channel 135 45 depth 0 0 0 0 FALSE FALSE 0)

    (gimp-selection-layer-alpha logo-layer)
    (set! fs (car (gimp-selection-float shadow-layer 0 0)))
    (gimp-edit-clear shadow-layer)
    (gimp-drawable-transform-perspective-default fs
                      (+ 5 (* 0.15 height)) (- height (* 0.15 height))
                      (+ 5 width (* 0.15 height)) (- height (* 0.15 height))
                      5 height
                      (+ 5 width) height
                      FALSE FALSE)
    (gimp-floating-sel-anchor fs)
    (plug-in-gauss-rle RUN-NONINTERACTIVE img shadow-layer smear TRUE TRUE)

    (gimp-rect-select img 5 5 width height CHANNEL-OP-REPLACE FALSE 0)
    (gimp-edit-copy logo-layer)
    (set! fs (car (gimp-edit-paste reflect-layer FALSE)))
    (gimp-floating-sel-anchor fs)
    (gimp-drawable-transform-scale-default reflect-layer
                                           0 0 width (* 0.85 height)
                                           FALSE FALSE)
    (gimp-drawable-transform-flip-simple reflect-layer ORIENTATION-VERTICAL
                                         TRUE 0 TRUE)
    (gimp-layer-set-offsets reflect-layer 5 (+ 3 height))

    (set! layer-mask (car (gimp-layer-create-mask reflect-layer ADD-WHITE-MASK)))
    (gimp-layer-add-mask reflect-layer layer-mask)
    (gimp-context-set-foreground '(255 255 255))
    (gimp-context-set-background '(0 0 0))
    (gimp-edit-blend layer-mask FG-BG-RGB-MODE NORMAL-MODE
                     GRADIENT-LINEAR 100 0 REPEAT-NONE FALSE
                     FALSE 0 0 TRUE
                     0 (- (/ height 2)) 0 height)

    (gimp-image-remove-channel img channel)

    (gimp-context-pop)
  )
)


(define (script-fu-cool-metal-logo-alpha img
                                         logo-layer
                                         size
                                         bg-color
                                         gradient
                                         gradient-reverse)
  (begin
    (gimp-image-undo-group-start img)

    (if (= (car (gimp-layer-is-floating-sel logo-layer)) TRUE)
        (begin
            (gimp-floating-sel-to-layer logo-layer)
            (set! logo-layer (car (gimp-image-get-active-layer img)))
        )
     )

    (apply-cool-metal-logo-effect img logo-layer size bg-color
                                  gradient gradient-reverse)
    (gimp-image-undo-group-end img)
    (gimp-displays-flush)
  )
)

(script-fu-register "script-fu-cool-metal-logo-alpha"
  _"Cool _Metal..."
  _"Add a metallic effect to the selected region (or alpha) with reflections and perspective shadows"
  "Spencer Kimball & Rob Malda"
  "Spencer Kimball & Rob Malda"
  "1997"
  "RGBA"
  SF-IMAGE      "Image"                 0
  SF-DRAWABLE   "Drawable"              0
  SF-ADJUSTMENT _"Effect size (pixels)" '(100 2 1000 1 10 0 1)
  SF-COLOR      _"Background color"     "white"
  SF-GRADIENT   _"Gradient"             "Horizon 1"
  SF-TOGGLE     _"Gradient reverse"     FALSE
)

(script-fu-menu-register "script-fu-cool-metal-logo-alpha"
                         "<Image>/Filters/Alpha to Logo")


(define (script-fu-cool-metal-logo text
                                   size
                                   font
                                   bg-color
                                   gradient
                                   gradient-reverse)
  (let* (
        (img (car (gimp-image-new 256 256 RGB)))
        (text-layer (car (gimp-text-fontname img -1 0 0 text 0 TRUE
                                              size PIXELS font)))
        )
    (gimp-image-undo-disable img)
    (apply-cool-metal-logo-effect img text-layer size bg-color
                                  gradient gradient-reverse)
    (gimp-image-undo-enable img)
    (gimp-display-new img)
  )
)

(script-fu-register "script-fu-cool-metal-logo"
  _"Cool _Metal..."
  _"Create a metallic logo with reflections and perspective shadows"
  "Spencer Kimball & Rob Malda"
  "Spencer Kimball & Rob Malda"
  "1997"
  ""
  SF-STRING     _"Text"               "Cool Metal"
  SF-ADJUSTMENT _"Font size (pixels)" '(100 2 1000 1 10 0 1)
  SF-FONT       _"Font"               "Crillee"
  SF-COLOR      _"Background color"   "white"
  SF-GRADIENT   _"Gradient"           "Horizon 1"
  SF-TOGGLE     _"Gradient reverse"   FALSE
)

(script-fu-menu-register "script-fu-cool-metal-logo"
                         "<Image>/File/Create/Logos")
