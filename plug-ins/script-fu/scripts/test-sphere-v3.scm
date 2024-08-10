#!/usr/bin/env gimp-script-fu-interpreter-3.0
;!# Close comment started on first line. Needed by gettext.

; v3 >>> Has shebang, is interpreter

; This is a a test script to test Script-Fu parameter API.

; For GIMP 3: uses GimpImageProcedure, GimpProcedureDialog, GimpConfig

; See also test-sphere.scm, for GIMP 2, from which this is derived
; Diffs marked with ; v3 >>>

; Also modified to use script-fu-use-v3
; I.E. binding of boolean and binding of PDB returns is changed.
; TRUE => #t in many places
; (car (...)) => (...) in many places


; v3 >>> signature of GimpImageProcedure
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
                               orientation
                               unused-interpolation
                               unused-dirname
                               unused-image
                               unused-layer
                               unused-channel
                               unused-drawable)
  (script-fu-use-v3)
  (let* (
        (width (* radius 3.75))
        (height (* radius 2.5))
        (img (gimp-image-new width height RGB)) ; v3 >>> elide car
        (drawable (gimp-layer-new img width height RGB-IMAGE
                                  "Sphere Layer" 100 LAYER-MODE-NORMAL))
        (radians (/ (* light *pi*) 180))
        (cx (/ width 2))
        (cy (/ height 2))
        (light-x (+ cx (* radius (* 0.6 (cos radians)))))
        (light-y (- cy (* radius (* 0.6 (sin radians)))))
        (light-end-x (+ cx (* radius (cos (+ *pi* radians)))))
        (light-end-y (- cy (* radius (sin (+ *pi* radians)))))
        (offset (* radius 0.1))
        (text-extents (gimp-text-get-extents-font multi-text
                                                  size
                                                  font))
        (x-position (- cx (/ (car text-extents) 2)))
        (y-position (- cy (/ (cadr text-extents) 2)))
        (shadow-w 0)
        (shadow-x 0)
        )

    (gimp-context-push)
    (gimp-context-set-defaults)

    (gimp-image-undo-disable img)
    (gimp-image-insert-layer img drawable 0 0)
    (gimp-context-set-foreground sphere-color)
    (gimp-context-set-background bg-color)
    (gimp-drawable-edit-fill drawable FILL-BACKGROUND)
    (gimp-context-set-background '(20 20 20))

    (if (and
         (or (and (>= light 45) (<= light 75))
             (and (<= light 135) (>= light 105)))
         ; v3 >>> conditional doesn't need (= shadow TRUE)
         shadow )
        (let ((shadow-w (* (* radius 2.5) (cos (+ *pi* radians))))
              (shadow-h (* radius 0.5))
              (shadow-x cx)
              (shadow-y (+ cy (* radius 0.65))))
          (if (< shadow-w 0)
              (begin (set! shadow-x (+ cx shadow-w))
                     (set! shadow-w (- shadow-w))))

          (gimp-context-set-feather #t)
          (gimp-context-set-feather-radius 7.5 7.5)
          (gimp-image-select-ellipse img CHANNEL-OP-REPLACE shadow-x shadow-y shadow-w shadow-h)
          (gimp-context-set-pattern pattern)
          (gimp-drawable-edit-fill drawable FILL-PATTERN)))

    (gimp-context-set-feather #f) ; v3 >>> FALSE => #f
    (gimp-image-select-ellipse img CHANNEL-OP-REPLACE (- cx radius) (- cy radius)
                               (* 2 radius) (* 2 radius))

    (gimp-context-set-gradient-fg-bg-rgb)
    (gimp-drawable-edit-gradient-fill drawable
				      GRADIENT-RADIAL offset
				      #f 1 1 ; v3 >>> and also supersampling enum starts at 1 now
				      #t
				      light-x light-y
				      light-end-x light-end-y)

    (gimp-selection-none img)

    (gimp-image-select-ellipse img CHANNEL-OP-REPLACE 10 10 50 50)

    (gimp-context-set-gradient gradient)
    (gimp-context-set-gradient-reverse gradient-reverse)
    (gimp-drawable-edit-gradient-fill drawable
				      GRADIENT-LINEAR offset
				      #f 1 1
				      #t
				      10 10
				      30 60)

    (gimp-selection-none img)

    (gimp-context-set-foreground '(0 0 0))
    (gimp-floating-sel-anchor (gimp-text-font
                                img drawable
                                x-position y-position
                                multi-text
                                0 #t
                                size
                                font))

    (if (= orientation 1)
      (gimp-image-rotate img ROTATE-DEGREES90))

    (gimp-image-undo-enable img)
    (gimp-display-new img)

    (gimp-context-pop)
  )
)

; v3 >>> use script-fu-register-filter
; v3 >>> menu item is v3, alongside old one
; v3 >>> not yet localized

; Translate the menu item and help, but not the dialog labels,
; since only plugin authors will read the dialog labels.

(script-fu-register-filter "script-fu-test-sphere-v3"
  ; Translator: this means "in the Scheme programming language" aka ScriptFu.
  _"Plug-In Example in _Scheme"
  _"Plug-in example in Scheme"
  "Spencer Kimball, Sven Neumann"
  "Spencer Kimball"
  "1996, 1998"
  "*"  ; image types any
  SF-ONE-OR-MORE-DRAWABLE  ; v3 >>> additional argument
  SF-ADJUSTMENT "Radius (in pixels)" (list 100 1 5000 1 10 0 SF-SPINNER)
  SF-ADJUSTMENT "Lighting (degrees)" (list 45 0 360 1 10 1 SF-SLIDER)
  SF-TOGGLE     "Shadow"             #t    ; v3 >>>
  SF-COLOR      "Background color"   "white"
  SF-COLOR      "Sphere color"       "red"
  ; v3 >>> only declare name of default brush
  SF-BRUSH      "Brush"              "2. Hardness 100"
  SF-STRING     "Text"               "Tiny-Fu rocks!"
  SF-TEXT       "Multi-line text"    "Hello,\nWorld!"
  SF-PATTERN    "Pattern"            "Maple Leaves"
  SF-GRADIENT   "Gradient"           "Deep Sea"
  SF-TOGGLE     "Gradient reverse"   #f    ; v3 >>>
  SF-FONT       "Font"               "Agate"
  SF-ADJUSTMENT "Font size (pixels)" '(50 1 1000 1 10 0 1)
  SF-PALETTE    "Palette"            "Default"
  SF-FILENAME   "Environment map"
                (string-append gimp-data-directory
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
                         "<Image>/Filters/Development/Plug-In Examples")
