#!/usr/bin/env ligma-script-fu-interpreter-3.0

; v3 >>> Has shebang, is interpreter

; This is a a test script to test Script-Fu parameter API.

; For LIGMA 3: uses LigmaImageProcedure, LigmaProcedureDialog, LigmaConfig

; See also test-sphere.scm, for LIGMA 2, from which this is derived
; Diffs marked with ; v3 >>>


; v3 >>> signature of LigmaImageProcedure
; drawables is a vector
(define (script-fu-test-sphere-v3
                               image
                               drawables
                               radius
                               light
                               shadow
                               bg-color
                               sphere-color
                               brush
                               text
                               multi-text
                               pattern
                               gradient
                               gradient-reverse
                               font
                               size
                               unused-palette
                               unused-filename
                               unused-orientation
                               unused-interpolation
                               unused-dirname
                               unused-image
                               unused-layer
                               unused-channel
                               unused-drawable)
  (let* (
        (width (* radius 3.75))
        (height (* radius 2.5))
        (img (car (ligma-image-new width height RGB)))
        (drawable (car (ligma-layer-new img width height RGB-IMAGE
                                       "Sphere Layer" 100 LAYER-MODE-NORMAL)))
        (radians (/ (* light *pi*) 180))
        (cx (/ width 2))
        (cy (/ height 2))
        (light-x (+ cx (* radius (* 0.6 (cos radians)))))
        (light-y (- cy (* radius (* 0.6 (sin radians)))))
        (light-end-x (+ cx (* radius (cos (+ *pi* radians)))))
        (light-end-y (- cy (* radius (sin (+ *pi* radians)))))
        (offset (* radius 0.1))
        (text-extents (ligma-text-get-extents-fontname multi-text
                                                      size PIXELS
                                                      font))
        (x-position (- cx (/ (car text-extents) 2)))
        (y-position (- cy (/ (cadr text-extents) 2)))
        (shadow-w 0)
        (shadow-x 0)
        )

    (ligma-context-push)
    (ligma-context-set-defaults)

    (ligma-image-undo-disable img)
    (ligma-image-insert-layer img drawable 0 0)
    (ligma-context-set-foreground sphere-color)
    (ligma-context-set-background bg-color)
    (ligma-drawable-edit-fill drawable FILL-BACKGROUND)
    (ligma-context-set-background '(20 20 20))

    (if (and
         (or (and (>= light 45) (<= light 75))
             (and (<= light 135) (>= light 105)))
         (= shadow TRUE))
        (let ((shadow-w (* (* radius 2.5) (cos (+ *pi* radians))))
              (shadow-h (* radius 0.5))
              (shadow-x cx)
              (shadow-y (+ cy (* radius 0.65))))
          (if (< shadow-w 0)
              (begin (set! shadow-x (+ cx shadow-w))
                     (set! shadow-w (- shadow-w))))

          (ligma-context-set-feather TRUE)
          (ligma-context-set-feather-radius 7.5 7.5)
          (ligma-image-select-ellipse img CHANNEL-OP-REPLACE shadow-x shadow-y shadow-w shadow-h)
          (ligma-context-set-pattern pattern)
          (ligma-drawable-edit-fill drawable FILL-PATTERN)))

    (ligma-context-set-feather FALSE)
    (ligma-image-select-ellipse img CHANNEL-OP-REPLACE (- cx radius) (- cy radius)
                               (* 2 radius) (* 2 radius))

    (ligma-context-set-gradient-fg-bg-rgb)
    (ligma-drawable-edit-gradient-fill drawable
				      GRADIENT-RADIAL offset
				      FALSE 0 0
				      TRUE
				      light-x light-y
				      light-end-x light-end-y)

    (ligma-selection-none img)

    (ligma-image-select-ellipse img CHANNEL-OP-REPLACE 10 10 50 50)

    (ligma-context-set-gradient gradient)
    (ligma-context-set-gradient-reverse gradient-reverse)
    (ligma-drawable-edit-gradient-fill drawable
				      GRADIENT-LINEAR offset
				      FALSE 0 0
				      TRUE
				      10 10
				      30 60)

    (ligma-selection-none img)

    (ligma-context-set-foreground '(0 0 0))
    (ligma-floating-sel-anchor (car (ligma-text-fontname img drawable
                                                       x-position y-position
                                                       multi-text
                                                       0 TRUE
                                                       size PIXELS
                                                       font)))

    (ligma-image-undo-enable img)
    (ligma-display-new img)

    (ligma-context-pop)
  )
)

; v3 >>> use script-fu-register-filter
; v3 >>> menu item is v3, alongside old one
; v3 >>> not yet localized

(script-fu-register-filter "script-fu-test-sphere-v3"
  "Sphere v3..."
  "Test script-fu-register-filter and LigmaProcedureDialog: needs 2 selected layers."
  "Spencer Kimball, Sven Neumann"
  "Spencer Kimball"
  "1996, 1998"
  "*"  ; image types any
  SF-TWO-OR-MORE-DRAWABLE  ; v3 >>> additional argument
  SF-ADJUSTMENT "Radius (in pixels)" (list 100 1 5000 1 10 0 SF-SPINNER)
  SF-ADJUSTMENT "Lighting (degrees)" (list 45 0 360 1 10 1 SF-SLIDER)
  SF-TOGGLE     "Shadow"             TRUE
  SF-COLOR      "Background color"   "white"
  SF-COLOR      "Sphere color"       "red"
  SF-BRUSH      "Brush"              '("2. Hardness 100" 100 44 0)
  SF-STRING     "Text"               "Tiny-Fu rocks!"
  SF-TEXT       "Multi-line text"    "Hello,\nWorld!"
  SF-PATTERN    "Pattern"            "Maple Leaves"
  SF-GRADIENT   "Gradient"           "Deep Sea"
  SF-TOGGLE     "Gradient reverse"   FALSE
  SF-FONT       "Font"               "Agate"
  SF-ADJUSTMENT "Font size (pixels)" '(50 1 1000 1 10 0 1)
  SF-PALETTE    "Palette"            "Default"
  SF-FILENAME   "Environment map"
                (string-append ligma-data-directory
                               "/scripts/images/beavis.jpg")
  SF-OPTION     "Orientation"        '("Horizontal"
                                       "Vertical")
  SF-ENUM       "Interpolation"      '("InterpolationType" "linear")
  SF-DIRNAME    "Output directory"   "/var/tmp/"
  SF-IMAGE      "Image"              -1
  SF-LAYER      "Layer"              -1
  SF-CHANNEL    "Channel"            -1
  SF-DRAWABLE   "Drawable"           -1
  SF-VECTORS    "Vectors"            -1
)

(script-fu-menu-register "script-fu-test-sphere-v3"
                         "<Image>/Filters/Development/Script-Fu/Test")
