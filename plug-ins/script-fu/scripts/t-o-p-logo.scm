;;  Trace of Particles Effect
;; Shuji Narazaki (narazaki@InetQ.or.jp)
;; Time-stamp: <97/03/15 17:27:33 narazaki@InetQ.or.jp>
;; Version 0.2

(define (apply-t-o-p-logo-effect img
                                 logo-layer
                                 b-size
                                 hit-rate
                                 edge-size
                                 edge-only
                                 base-color
                                 bg-color)
  (let* (
        (width (car (gimp-drawable-width logo-layer)))
        (height (car (gimp-drawable-height logo-layer)))
        (logo-layer-mask (car (gimp-layer-create-mask logo-layer ADD-BLACK-MASK)))
        (sparkle-layer (car (gimp-layer-new img width height RGBA-IMAGE "Sparkle" 100 NORMAL-MODE)))
        (shadow-layer (car (gimp-layer-new img width height RGBA-IMAGE "Shadow" 90 ADDITION-MODE)))
        (bg-layer (car (gimp-layer-new img width height RGB-IMAGE "Background" 100 NORMAL-MODE)))
        (selection 0)
        (white '(255 255 255))
        )

    (gimp-context-push)

    (script-fu-util-image-resize-from-layer img logo-layer)
    (script-fu-util-image-add-layers img sparkle-layer shadow-layer bg-layer)
    (gimp-selection-none img)
    (gimp-edit-clear shadow-layer)
    (gimp-edit-clear sparkle-layer)
    (gimp-context-set-background base-color)
    (gimp-edit-fill sparkle-layer BACKGROUND-FILL)
    (gimp-selection-layer-alpha logo-layer)
    (set! selection (car (gimp-selection-save img)))
    (gimp-selection-grow img edge-size)
    (plug-in-noisify RUN-NONINTERACTIVE img sparkle-layer FALSE
                     (* 0.1 hit-rate) (* 0.1 hit-rate) (* 0.1 hit-rate) 0.0)
    (gimp-selection-border img edge-size)
    (plug-in-noisify RUN-NONINTERACTIVE img sparkle-layer FALSE hit-rate hit-rate hit-rate 0.0)
    (gimp-selection-none img)
    (plug-in-sparkle RUN-NONINTERACTIVE img sparkle-layer 0.03 0.49 width 6 15 1.0 0.0 0.0 0.0 FALSE FALSE FALSE 0)
    (gimp-selection-load selection)
    (gimp-selection-shrink img edge-size)
    (gimp-levels sparkle-layer 0 0 255 1.2 0 255)
    (gimp-selection-load selection)
    (gimp-selection-border img edge-size)
    (gimp-levels sparkle-layer 0 0 255 0.5 0 255)
    (gimp-selection-load selection)
    (gimp-selection-grow img (/ edge-size 2.0))
    (gimp-selection-invert img)
    (gimp-edit-clear sparkle-layer)
    (if (= edge-only TRUE)
        (begin
          (gimp-selection-load selection)
          (gimp-selection-shrink img (/ edge-size 2.0))
          (gimp-edit-clear sparkle-layer)
          (gimp-selection-load selection)
          (gimp-selection-grow img (/ edge-size 2.0))
          (gimp-selection-invert img)))
    (gimp-context-set-foreground '(0 0 0))
    (gimp-context-set-background '(255 255 255))
    (gimp-context-set-brush "Circle Fuzzy (11)")
    (gimp-selection-feather img b-size)
    (gimp-edit-fill shadow-layer BACKGROUND-FILL)

    (gimp-selection-none img)
    (gimp-context-set-background bg-color)
    (gimp-edit-fill bg-layer BACKGROUND-FILL)

    (gimp-item-set-visible logo-layer 0)
    (gimp-image-set-active-layer img sparkle-layer)

    (gimp-context-pop)
  )
)


(define (script-fu-t-o-p-logo-alpha img
                                    logo-layer
                                    b-size
                                    hit-rate
                                    edge-size
                                    edge-only
                                    base-color
                                    bg-color)
  (begin
    (gimp-image-undo-group-start img)
    (apply-t-o-p-logo-effect img logo-layer b-size hit-rate
                             edge-size edge-only base-color bg-color)
    (gimp-image-undo-group-end img)
    (gimp-displays-flush)
  )
)

(script-fu-register "script-fu-t-o-p-logo-alpha"
  _"_Particle Trace..."
  _"Add a Trace of Particles effect to the selected region (or alpha)"
  "Shuji Narazaki (narazaki@InetQ.or.jp)"
  "Shuji Narazaki"
  "1997"
  "RGBA"
  SF-IMAGE      "Image"                 0
  SF-DRAWABLE   "Drawable"              0
  SF-ADJUSTMENT _"Border size (pixels)" '(20 1 200 1 10 0 1)
  SF-ADJUSTMENT _"Hit rate"             '(0.2 0 1 .01 .01 2 0)
  SF-ADJUSTMENT _"Edge width"           '(2 0 128 1 1 0 0)
  SF-TOGGLE     _"Edge only"            FALSE
  SF-COLOR      _"Base color"           '(0 40 0)
  SF-COLOR      _"Background color"     "white"
)

(script-fu-menu-register "script-fu-t-o-p-logo-alpha"
                         "<Image>/Filters/Alpha to Logo")


(define (script-fu-t-o-p-logo text
                              size
                              fontname
                              hit-rate
                              edge-size
                              edge-only
                              base-color
                              bg-color)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
         (border (/ size 5))
         (text-layer (car (gimp-text-fontname img -1 0 0 text (* border 2) TRUE size PIXELS fontname))))
    (gimp-image-undo-disable img)
    (apply-t-o-p-logo-effect img text-layer border hit-rate
                             edge-size edge-only base-color bg-color)
    (gimp-image-undo-enable img)
    (gimp-display-new img)
  )
)

(script-fu-register "script-fu-t-o-p-logo"
  _"_Particle Trace..."
  _"Create a logo using a Trace Of Particles effect"
  "Shuji Narazaki (narazaki@InetQ.or.jp)"
  "Shuji Narazaki"
  "1997"
  ""
  SF-STRING     _"Text"               "GIMP"
  SF-ADJUSTMENT _"Font size (pixels)" '(100 1 1000 1 10 0 1)
  SF-FONT       _"Font"               "Becker"
  SF-ADJUSTMENT _"Hit rate"           '(0.2 0 1 .01 .01 2 0)
  SF-ADJUSTMENT _"Edge width"         '(2 0 128 1 1 0 0)
  SF-TOGGLE     _"Edge only"          FALSE
  SF-COLOR      _"Base color"         '(0 40 0)
  SF-COLOR      _"Background color"   "white"
)

(script-fu-menu-register "script-fu-t-o-p-logo"
                         "<Image>/File/Create/Logos")
