;  FROZEN-TEXT effect
;  Thanks to Ed Mackey for this one
;   Written by Spencer Kimball

(define (apply-frosty-logo-effect img
                                  logo-layer
                                  size
                                  bg-color)
  (let* (
        (border (/ size 5))
        (width (+ (car (gimp-drawable-width logo-layer)) border))
        (height (+ (car (gimp-drawable-height logo-layer)) border))
        (logo-layer-mask (car (gimp-layer-create-mask logo-layer
                                                      ADD-BLACK-MASK)))
        (sparkle-layer (car (gimp-layer-new img width height RGBA-IMAGE
                                            "Sparkle" 100 NORMAL-MODE)))
        (matte-layer (car (gimp-layer-new img width height RGBA-IMAGE
                                          "Matte" 100 NORMAL-MODE)))
        (shadow-layer (car (gimp-layer-new img width height RGBA-IMAGE
                                           "Shadow" 90 MULTIPLY-MODE)))
        (bg-layer (car (gimp-layer-new img width height RGB-IMAGE
                                       "Background" 100 NORMAL-MODE)))
        (selection 0)
        )

    (gimp-context-push)

    (gimp-layer-add-mask logo-layer logo-layer-mask)
    (script-fu-util-image-resize-from-layer img shadow-layer)
    (gimp-image-add-layer img sparkle-layer 2)
    (gimp-image-add-layer img matte-layer 3)
    (gimp-image-add-layer img shadow-layer 4)
    (gimp-image-add-layer img bg-layer 5)
    (gimp-selection-none img)
    (gimp-edit-clear sparkle-layer)
    (gimp-edit-clear matte-layer)
    (gimp-edit-clear shadow-layer)
    (gimp-selection-layer-alpha logo-layer)
    (set! selection (car (gimp-selection-save img)))
    (gimp-selection-feather img border)
    (gimp-context-set-background '(0 0 0))
    (gimp-edit-fill sparkle-layer BACKGROUND-FILL)
    (plug-in-noisify RUN-NONINTERACTIVE img sparkle-layer FALSE 0.2 0.2 0.2 0.0)
    (plug-in-c-astretch RUN-NONINTERACTIVE img sparkle-layer)
    (gimp-selection-none img)
    (plug-in-sparkle RUN-NONINTERACTIVE img sparkle-layer 0.03 0.5
                     (/ (min width height) 2)
                     6 15 1.0 0.0 0.0 0.0 FALSE FALSE FALSE 0)
    (gimp-levels sparkle-layer 1 0 255 0.2 0 255)
    (gimp-levels sparkle-layer 2 0 255 0.7 0 255)
    (gimp-selection-layer-alpha sparkle-layer)
    (gimp-context-set-foreground '(0 0 0))
    (gimp-context-set-brush "Circle Fuzzy (11)")
    (gimp-edit-stroke matte-layer)
    (gimp-selection-feather img border)
    (gimp-edit-fill shadow-layer BACKGROUND-FILL)
    (gimp-selection-none img)
    (gimp-context-set-background bg-color)
    (gimp-edit-fill bg-layer BACKGROUND-FILL)
    (gimp-context-set-background '(0 0 0))
    (gimp-edit-fill logo-layer BACKGROUND-FILL)
    (gimp-selection-load selection)
    (gimp-context-set-background '(255 255 255))
    (gimp-edit-fill logo-layer-mask BACKGROUND-FILL)
    (gimp-selection-feather img border)
    (gimp-selection-translate img (/ border 2) (/ border 2))
    (gimp-edit-fill logo-layer BACKGROUND-FILL)
    (gimp-layer-remove-mask logo-layer 0)
    (gimp-selection-load selection)
    (gimp-context-set-brush "Circle Fuzzy (07)")
    (gimp-context-set-paint-mode BEHIND-MODE)
    (gimp-context-set-foreground '(186 241 255))
    (gimp-edit-stroke logo-layer)
    (gimp-selection-none img)
    (gimp-image-remove-channel img selection)

    (for-each (lambda (the-layer)
              (gimp-layer-resize the-layer (- width border) (- height border)
                                           (- border) (- border))
              )
              (list sparkle-layer matte-layer bg-layer)
    )

    (gimp-layer-resize shadow-layer (- width border) (- height border) 0 0)
    (gimp-layer-translate shadow-layer border border)

    (script-fu-util-image-resize-from-layer img logo-layer)

    (gimp-layer-translate bg-layer (- 0 border) (- 0 border))

    (gimp-context-pop)
  )
)

(define (script-fu-frosty-logo-alpha img
                                     logo-layer
                                     size
                                     bg-color)

  (let* (
        (position (- (car(gimp-image-get-layer-position img logo-layer)) 1))
        (duplicate (car (gimp-layer-new-from-drawable logo-layer img)))
        (name (car (gimp-layer-get-name logo-layer)))
        (select (cons-array 4 'byte))
        (crop 0)
        (temp 0)
        )

    (gimp-image-undo-group-start img)

    (gimp-selection-layer-alpha logo-layer)
    (gimp-image-add-layer img duplicate -1)
    (gimp-layer-resize-to-image-size duplicate)
    (gimp-selection-none img)
    (apply-frosty-logo-effect img duplicate size bg-color)
    (set! crop (aref (cadr (gimp-image-get-layers img)) (+ position 6)))
    (set! temp (aref (cadr (gimp-image-get-layers img)) (+ position 3)))
    (gimp-selection-layer-alpha temp)
    (aset select 0 (cadr (gimp-selection-bounds img)))
    (aset select 1 (caddr (gimp-selection-bounds img)))
    (aset select 2 (cadddr (gimp-selection-bounds img)))
    (aset select 3 (cadddr (cdr(gimp-selection-bounds img))))

    (gimp-layer-resize crop
           (+ 20 (- (aref select 2) (aref select 0)))
           (+ 20 (- (aref select 3) (aref select 1)))
           (- 0 (+ 0 (aref select 0)))
           (- 0 (+ 0 (aref select 1))))
    (gimp-layer-resize duplicate
           (car (gimp-drawable-width logo-layer))
           (car (gimp-drawable-height logo-layer))
           (- 0 (car (gimp-drawable-offsets logo-layer)))
           (- 0 (cadr (gimp-drawable-offsets logo-layer))))
    (gimp-image-remove-layer img logo-layer)
    (gimp-layer-set-name duplicate name)

    (gimp-selection-none img)
    (gimp-image-undo-group-end img)
    (gimp-displays-flush)
  )
)

(script-fu-register "script-fu-frosty-logo-alpha"
  _"_Frosty..."
  _"Add a frost effect to the selected region (or alpha) with an added drop shadow"
  "Spencer Kimball & Ed Mackey"
  "Spencer Kimball & Ed Mackey"
  "1997"
  "RGBA"
  SF-IMAGE      "Image"                 0
  SF-DRAWABLE   "Drawable"              0
  SF-ADJUSTMENT _"Effect size (pixels)" '(100 2 1000 1 10 0 1)
  SF-COLOR      _"Background color"     "white"
)

(script-fu-menu-register "script-fu-frosty-logo-alpha"
                         "<Image>/Filters/Alpha to Logo")


(define (script-fu-frosty-logo text
                               size
                               font
                               bg-color)
  (let* (
        (img (car (gimp-image-new 256 256 RGB)))
        (border (/ size 5))
        (text-layer (car (gimp-text-fontname img -1 0 0 text (* border 2) TRUE size PIXELS font)))
        )
    (gimp-image-undo-disable img)
    (apply-frosty-logo-effect img text-layer size bg-color)
    (gimp-image-undo-enable img)
    (gimp-display-new img)
  )
)

(script-fu-register "script-fu-frosty-logo"
  _"_Frosty..."
  _"Create frozen logo with an added drop shadow"
  "Spencer Kimball & Ed Mackey"
  "Spencer Kimball & Ed Mackey"
  "1997"
  ""
  SF-STRING _"Text"                   "GIMP"
  SF-ADJUSTMENT _"Font size (pixels)" '(100 2 1000 1 10 0 1)
  SF-FONT   _"Font"                   "Becker"
  SF-COLOR  _"Background color"       "white"
)

(script-fu-menu-register "script-fu-frosty-logo"
                         "<Toolbox>/Xtns/Logos")
