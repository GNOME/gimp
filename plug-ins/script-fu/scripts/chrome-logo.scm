;  CHROME-LOGOS

(define (apply-chrome-logo-effect img
                                  logo-layer
                                  offsets
                                  bg-color)
  (let* (
        (offx1 (* offsets 0.4))
        (offy1 (* offsets 0.3))
        (offx2 (* offsets (- 0.4)))
        (offy2 (* offsets (- 0.3)))
        (feather (* offsets 0.5))
        (width (car (gimp-drawable-width logo-layer)))
        (height (car (gimp-drawable-height logo-layer)))
        (layer1 (car (gimp-layer-new img width height RGBA-IMAGE "Layer 1" 100 DIFFERENCE-MODE)))
        (layer2 (car (gimp-layer-new img width height RGBA-IMAGE "Layer 2" 100 DIFFERENCE-MODE)))
        (layer3 (car (gimp-layer-new img width height RGBA-IMAGE "Layer 3" 100 NORMAL-MODE)))
        (shadow (car (gimp-layer-new img width height RGBA-IMAGE "Drop Shadow" 100 NORMAL-MODE)))
        (background (car (gimp-layer-new img width height RGB-IMAGE "Background" 100 NORMAL-MODE)))
        (layer-mask (car (gimp-layer-create-mask layer1 ADD-BLACK-MASK)))
        )

    (gimp-context-push)

    (script-fu-util-image-resize-from-layer img logo-layer)
    (script-fu-util-image-add-layers img layer1 layer2 layer3 shadow background)
    (gimp-context-set-background '(255 255 255))
    (gimp-selection-none img)
    (gimp-edit-fill layer1 BACKGROUND-FILL)
    (gimp-edit-fill layer2 BACKGROUND-FILL)
    (gimp-edit-fill layer3 BACKGROUND-FILL)
    (gimp-edit-clear shadow)
    (gimp-selection-layer-alpha logo-layer)
    (gimp-item-set-visible logo-layer FALSE)
    (gimp-item-set-visible shadow FALSE)
    (gimp-item-set-visible background FALSE)
    (gimp-context-set-background '(0 0 0))
    (gimp-edit-fill layer1 BACKGROUND-FILL)
    (gimp-selection-translate img offx1 offy1)
    (gimp-selection-feather img feather)
    (gimp-edit-fill layer2 BACKGROUND-FILL)
    (gimp-selection-translate img (* 2 offx2) (* 2 offy2))
    (gimp-edit-fill layer3 BACKGROUND-FILL)
    (gimp-selection-none img)
    (set! layer1 (car (gimp-image-merge-visible-layers img CLIP-TO-IMAGE)))
    ; if the original image contained more than one visible layer:
    (while (> (car (gimp-image-get-layer-position img layer1)) 
              (car (gimp-image-get-layer-position img shadow)))
      (gimp-image-raise-layer img layer1)
    )
    (gimp-invert layer1)
    (gimp-layer-add-mask layer1 layer-mask)
    (gimp-selection-layer-alpha logo-layer)
    (gimp-context-set-background '(255 255 255))
    (gimp-selection-feather img feather)
    (gimp-edit-fill layer-mask BACKGROUND-FILL)
    (gimp-context-set-background '(0 0 0))
    (gimp-selection-translate img offx1 offy1)
    (gimp-edit-fill shadow BACKGROUND-FILL)
    (gimp-selection-none img)
    (gimp-context-set-background bg-color)
    (gimp-edit-fill background BACKGROUND-FILL)
    (gimp-item-set-visible shadow TRUE)
    (gimp-item-set-visible background TRUE)
    (gimp-item-set-name layer1 (car (gimp-item-get-name logo-layer)))
    (gimp-image-remove-layer img logo-layer)

    (gimp-context-pop)
  )
)


(define (script-fu-chrome-logo-alpha img
                                     logo-layer
                                     offsets
                                     bg-color)
  (begin
    (gimp-image-undo-group-start img)
    (apply-chrome-logo-effect img logo-layer offsets bg-color)
    (gimp-image-undo-group-end img)
    (gimp-displays-flush)
  )
)

(script-fu-register "script-fu-chrome-logo-alpha"
  _"C_hrome..."
  _"Add a simple chrome effect to the selected region (or alpha)"
  "Spencer Kimball"
  "Spencer Kimball & Peter Mattis"
  "1997"
  "RGBA"
  SF-IMAGE       "Image"                0
  SF-DRAWABLE    "Drawable"             0
  SF-ADJUSTMENT _"Offsets (pixels * 2)" '(10 2 100 1 10 0 1)
  SF-COLOR      _"Background Color"     "lightgrey"
)

(script-fu-menu-register "script-fu-chrome-logo-alpha"
                         "<Image>/Filters/Alpha to Logo")


(define (script-fu-chrome-logo text
                               size
                               font
                               bg-color)
  (let* (
        (img (car (gimp-image-new 256 256 RGB)))
        (b-size (* size 0.2))
        (text-layer (car (gimp-text-fontname img -1 0 0 text b-size TRUE size PIXELS font)))
        )
    (gimp-image-undo-disable img)
    (apply-chrome-logo-effect img text-layer (* size 0.1) bg-color)
    (gimp-image-undo-enable img)
    (gimp-display-new img)
  )
)

(script-fu-register "script-fu-chrome-logo"
  _"C_hrome..."
  _"Create a simplistic, but cool, chromed logo"
  "Spencer Kimball"
  "Spencer Kimball & Peter Mattis"
  "1997"
  ""
  SF-STRING     _"Text"               "GIMP"
  SF-ADJUSTMENT _"Font size (pixels)" '(100 2 1000 1 10 0 1)
  SF-FONT       _"Font"               "Bodoni"
  SF-COLOR      _"Background color"   "lightgrey"
)

(script-fu-menu-register "script-fu-chrome-logo"
                         "<Image>/File/Create/Logos")
