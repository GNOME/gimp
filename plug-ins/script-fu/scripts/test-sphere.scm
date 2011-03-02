; This is a a test script to show and test the possibilities of the
; Script-Fu parameter API.
;
; ----------------------------------------------------------------------
; SF-ADJUSTMENT
; is only useful in interactive mode, if you call a script from
; the console, it acts just like a normal SF-VALUE
; In interactive mode it creates an adjustment widget in the dialog.
;
; Usage:
; SF-ADJUSTMENT "label" '(value lower upper step_inc page_inc digits type)
;
; type is one of: SF-SLIDER(0), SF-SPINNER(1)
;
; ----------------------------------------------------------------------
; SF-COLOR
; creates a color button in the dialog. It accepts either a list of three
; values for the red, green and blue components or a color name in CSS
; notatation
;
; Usage:
; SF-COLOR "label" '(red green blue)
; SF-COLOR "label" "color"
;
; ----------------------------------------------------------------------
; SF-FONT
; creates a font-selection widget in the dialog. It returns a fontname as
; a string. There are two new gimp-text procedures to ease the use of this
; return parameter:
;
;  (gimp-text-fontname image drawable
;                      x-pos y-pos text border antialias size unit font)
;  (gimp-text-get-extents-fontname text size unit font))
;
; where font is the fontname you get. The size specified in the fontname
; is silently ignored. It is only used in the font-selector. So you are
; asked to set it to a useful value (24 pixels is a good choice) when
; using SF-FONT.
;
; Usage:
; SF-FONT "label" "fontname"
;
; ----------------------------------------------------------------------
; SF-BRUSH
; is only useful in interactive mode. It will create a widget in the control
; dialog. The widget consists of a preview area (which when pressed will
; produce a popup preview ) and a button with the "..." label. The button will
; popup a dialog where brushes can be selected and each of the
; characteristics of the brush can be modified.
;
; The actual value returned when the script is invoked is a list
; consisting of Brush name, opacity, spacing and brush mode in the same
; units as passed in as the default value.
;
; Usage:
; SF-BRUSH "Brush" '("Circle (03)" 100 44 0)
;
; Here the brush dialog will be popped up with a default brush of Circle (03)
; opacity 100 spacing 44 and paint mode of Normal (value 0).
; If this selection was unchanged the value passed to the function as a
; parameter would be '("Circle (03)" 100 44 0).
;
; ----------------------------------------------------------------------
; SF-PATTERN
; Only useful in interactive mode. It will create a widget in the control
; dialog. The widget consists of a preview area (which when pressed will
; produce a popup preview ) and a button with the "..." label. The button will
; popup a dialog where patterns can be selected.
;
; Usage:
; SF-PATTERN "Pattern" "Maple Leaves"
;
; The value returned when the script is invoked is a string containing the
; pattern name. If the above selection was not altered the string would
; contain "Maple Leaves"
;
; ----------------------------------------------------------------------
; SF-GRADIENT
; Only useful in interactive mode. It will create a widget in the control
; dialog. The widget consists of a button containing a preview of the selected
; gradient. If the button is pressed a gradient selection dialog will popup.
;
; Usage:
; SF-GRADIENT "Gradient" "Deep Sea"
;
; The value returned when the script is invoked is a string containing the
; gradient name. If the above selection was not altered the string would
; contain "Deep Sea"
;
; ----------------------------------------------------------------------
; SF-PALETTE
; Only useful in interactive mode. It will create a widget in the control
; dialog. The widget consists of a button containing a preview of the selected
; palette. If the button is pressed a palette selection dialog will popup.
;
; Usage:
; SF-PALETTE "Palette" "Named Colors"
;
; The value returned when the script is invoked is a string containing the
; palette name. If the above selection was not altered the string would
; contain "Named Colors"
;
; ----------------------------------------------------------------------
; SF-FILENAME
; Only useful in interactive mode. It will create a widget in the control
; dialog. The widget consists of a button containing the name of a file.
; If the button is pressed a file selection dialog will popup.
;
; Usage:
; SF-FILENAME "Environment Map"
;             (string-append "" gimp-data-directory "/scripts/beavis.jpg")
;
; The value returned when the script is invoked is a string containing the
; filename.
;
; ----------------------------------------------------------------------
; SF-DIRNAME
; Only useful in interactive mode. Very similar to SF-FILENAME, but the
; created widget allows to choose a directory instead of a file.
;
; Usage:
; SF-DIRNAME "Image Directory" "/var/tmp/images"
;
; The value returned when the script is invoked is a string containing the
; dirname.
;
; ----------------------------------------------------------------------
; SF-OPTION
; Only useful in interactive mode. It will create a widget in the control
; dialog. The widget is a combo-box showing the options that are passed
; as a list. The first option is the default choice.
;
; Usage:
; SF-OPTION "Orientation" '("Horizontal" "Vertical")
;
; The value returned when the script is invoked is the number of the
; chosen option, where the option first is counted as 0.
;
; ----------------------------------------------------------------------
; SF-ENUM
; Only useful in interactive mode. It will create a widget in the control
; dialog. The widget is a combo-box showing all enum values for the given
; enum type. This has to be the name of a registered enum, without the
; "Gimp" prefix. The second parameter speficies the default value, using
; the enum value's nick.
;
; Usage:
; SF-ENUM "Interpolation" '("InterpolationType" "linear")
;
; The value returned when the script is invoked corresponds to chosen
; enum value.
;
; ----------------------------------------------------------------------


(define (script-fu-test-sphere radius
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
        (img (car (gimp-image-new width height RGB)))
        (drawable (car (gimp-layer-new img width height RGB-IMAGE
                                       "Sphere Layer" 100 NORMAL-MODE)))
        (radians (/ (* light *pi*) 180))
        (cx (/ width 2))
        (cy (/ height 2))
        (light-x (+ cx (* radius (* 0.6 (cos radians)))))
        (light-y (- cy (* radius (* 0.6 (sin radians)))))
        (light-end-x (+ cx (* radius (cos (+ *pi* radians)))))
        (light-end-y (- cy (* radius (sin (+ *pi* radians)))))
        (offset (* radius 0.1))
        (text-extents (gimp-text-get-extents-fontname multi-text
                                                      size PIXELS
                                                      font))
        (x-position (- cx (/ (car text-extents) 2)))
        (y-position (- cy (/ (cadr text-extents) 2)))
        (shadow-w 0)
        (shadow-x 0)
        )

    (gimp-context-push)

    (gimp-image-undo-disable img)
    (gimp-image-insert-layer img drawable 0 0)
    (gimp-context-set-foreground sphere-color)
    (gimp-context-set-background bg-color)
    (gimp-edit-fill drawable BACKGROUND-FILL)
    (gimp-context-set-background '(20 20 20))

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

          (gimp-ellipse-select img shadow-x shadow-y shadow-w shadow-h
                               CHANNEL-OP-REPLACE TRUE TRUE 7.5)
          (gimp-context-set-pattern pattern)
          (gimp-edit-bucket-fill drawable PATTERN-BUCKET-FILL MULTIPLY-MODE
                            100 0 FALSE 0 0)))

    (gimp-ellipse-select img (- cx radius) (- cy radius)
                         (* 2 radius) (* 2 radius) CHANNEL-OP-REPLACE TRUE FALSE 0)

    (gimp-edit-blend drawable FG-BG-RGB-MODE NORMAL-MODE
                     GRADIENT-RADIAL 100 offset REPEAT-NONE FALSE
                     FALSE 0 0 TRUE
                     light-x light-y light-end-x light-end-y)

    (gimp-selection-none img)

    (gimp-context-set-gradient gradient)
    (gimp-ellipse-select img 10 10 50 50 CHANNEL-OP-REPLACE TRUE FALSE 0)

    (gimp-edit-blend drawable CUSTOM-MODE NORMAL-MODE
                     GRADIENT-LINEAR 100 offset REPEAT-NONE gradient-reverse
                     FALSE 0 0 TRUE
                     10 10 30 60)

    (gimp-selection-none img)

    (gimp-context-set-foreground '(0 0 0))
    (gimp-floating-sel-anchor (car (gimp-text-fontname img drawable
                                                       x-position y-position
                                                       multi-text
                                                       0 TRUE
                                                       size PIXELS
                                                       font)))

    (gimp-image-undo-enable img)
    (gimp-display-new img)

    (gimp-context-pop)
  )
)

(script-fu-register "script-fu-test-sphere"
  _"_Sphere..."
  "Simple script to test and show the usage of the new Script-Fu API extensions."
  "Spencer Kimball, Sven Neumann"
  "Spencer Kimball"
  "1996, 1998"
  ""
  SF-ADJUSTMENT "Radius (in pixels)" (list 100 1 5000 1 10 0 SF-SPINNER)
  SF-ADJUSTMENT "Lighting (degrees)" (list 45 0 360 1 10 1 SF-SLIDER)
  SF-TOGGLE     "Shadow"             TRUE
  SF-COLOR      "Background color"   "white"
  SF-COLOR      "Sphere color"       "red"
  SF-BRUSH      "Brush"              '("Circle (03)" 1.0 44 0)
  SF-STRING     "Text"               "Tiny-Fu rocks!"
  SF-TEXT       "Multi-line text"    "Hello,\nWorld!"
  SF-PATTERN    "Pattern"            "Maple Leaves"
  SF-GRADIENT   "Gradient"           "Deep Sea"
  SF-TOGGLE     "Gradient reverse"   FALSE
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

(script-fu-menu-register "script-fu-test-sphere"
                         "<Image>/Filters/Languages/Script-Fu/Test")
