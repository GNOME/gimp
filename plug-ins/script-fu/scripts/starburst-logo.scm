;  Starburst
;    Effect courtesy Xach Beane's web page


(define (apply-starburst-logo-effect img logo-layer size burst-color bg-color)
  (let* (
        (off (* size 0.03))
        (count -1)
        (feather (* size 0.04))
        (border (+ feather off))
        (width (car (gimp-drawable-width logo-layer)))
        (height (car (gimp-drawable-height logo-layer)))
        (burst-coords (cons (* width 0.5) (* height 0.5)))
        (burstradius (* (min height width) 0.35))
        (bg-layer (car (gimp-layer-new img width height RGB-IMAGE "Background" 100 NORMAL-MODE)))
        (shadow-layer (car (gimp-layer-new img width height RGBA-IMAGE "Shadow" 100 NORMAL-MODE)))
        (burst-layer (car (gimp-layer-new img width height RGBA-IMAGE "Burst" 100 NORMAL-MODE)))
        (layer-mask (car (gimp-layer-create-mask burst-layer ADD-BLACK-MASK)))
        )

    (gimp-context-push)

    (gimp-selection-none img)
    (script-fu-util-image-resize-from-layer img logo-layer)
    (gimp-image-add-layer img bg-layer 1)
    (gimp-image-add-layer img shadow-layer 1)
    (gimp-image-add-layer img burst-layer 0)
    (gimp-layer-add-mask burst-layer layer-mask)
    (gimp-layer-set-lock-alpha logo-layer TRUE)

    (gimp-context-set-background bg-color)
    (gimp-edit-fill bg-layer BACKGROUND-FILL)
    (gimp-edit-clear shadow-layer)
    (gimp-edit-clear burst-layer)

    (gimp-selection-all img)
    (gimp-context-set-pattern "Crack")
    (gimp-edit-bucket-fill logo-layer PATTERN-BUCKET-FILL
                           NORMAL-MODE 100 0 FALSE 0 0)
    (gimp-selection-none img)

    (gimp-selection-layer-alpha logo-layer)

    (gimp-context-set-background '(255 255 255))
    (gimp-edit-fill layer-mask BACKGROUND-FILL)
    (gimp-selection-none img)
    (plug-in-nova RUN-NONINTERACTIVE img burst-layer (car burst-coords) (cdr burst-coords)
          burst-color burstradius 100 0)

    (gimp-selection-layer-alpha logo-layer)
    (gimp-context-set-background '(0 0 0))
    (gimp-selection-feather img feather)
    (gimp-selection-translate img -1 -1)
    (while (< count off)
       (gimp-edit-fill shadow-layer BACKGROUND-FILL)
       (gimp-selection-translate img 1 1)
       (set! count (+ count 1)))
    (gimp-selection-none img)

    (gimp-context-pop)
  )
)


(define (script-fu-starburst-logo-alpha img logo-layer size burst-color bg-color)
  (gimp-image-undo-group-start img)
  (apply-starburst-logo-effect img logo-layer size burst-color bg-color)
  (gimp-image-undo-group-end img)
  (gimp-displays-flush)
)

(script-fu-register "script-fu-starburst-logo-alpha"
  _"Starb_urst..."
  _"Fill the selected region (or alpha) with a starburst gradient and add a shadow"
  "Spencer Kimball & Xach Beane"
  "Spencer Kimball & Xach Beane"
  "1997"
  "RGBA"
  SF-IMAGE       "Image"                     0
  SF-DRAWABLE    "Drawable"                  0
  SF-ADJUSTMENT _"Effect size (pixels * 30)" '(150 0 512 1 10 0 1)
  SF-COLOR      _"Burst color"               '(60 196 33)
  SF-COLOR      _"Background color"          '(255 255 255)
)

(script-fu-menu-register "script-fu-starburst-logo-alpha"
                         "<Image>/Filters/Alpha to Logo")


(define (script-fu-starburst-logo text size fontname burst-color bg-color)
  (let* (
        (img (car (gimp-image-new 256 256 RGB)))
        (off (* size 0.03))
        (feather (* size 0.04))
        (border (+ feather off))
        (text-layer (car (gimp-text-fontname img -1 0 0 text border TRUE size PIXELS fontname)))
        )
    (gimp-image-undo-disable img)
    (apply-starburst-logo-effect img text-layer size burst-color bg-color)
    (gimp-image-undo-enable img)
    (gimp-display-new img)
  )
)

(script-fu-register "script-fu-starburst-logo"
  _"Starb_urst..."
  _"Create a logo using a starburst gradient"
  "Spencer Kimball & Xach Beane"
  "Spencer Kimball & Xach Beane"
  "1997"
  ""
  SF-STRING     _"Text"               "GIMP"
  SF-ADJUSTMENT _"Font size (pixels)" '(150 0 512 1 10 0 1)
  SF-FONT       _"Font"               "Blippo"
  SF-COLOR      _"Burst color"        '(60 196 33)
  SF-COLOR      _"Background color"   "white"
)

(script-fu-menu-register "script-fu-starburst-logo"
                         "<Toolbox>/Xtns/Logos")
