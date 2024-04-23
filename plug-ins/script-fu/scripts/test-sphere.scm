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
; Resources: Brush, Font, Gradient, Palette, Pattern
;
; Resources are Gimp objects for installed data such as fonts.

; Resources have a unique integer ID for the lifetime of a Gimp session.
; In ScriptFu, a resource argument is bound to an integer ID.
;
; Resources have names, often unique, but not always.
; For example, fonts from different collections of fonts might
; have the same names.
;
; ScriptFu v3 does not let you name a default resource.
; In v2, the third term of a triplet named a default resource.
; In v3, the third term is just a placeholder and is ignored.
; This is compatible with v2 scripts, but any default
; in a v2 script will be ignored in v3.
; (Similarly as for SF-IMAGE.)
;
; In the code below, a default name is shown,
; but is ignored since v3.
; Some even name resources that are not installed with Gimp any longer.
;
; In non-interactive mode, a resource argument in ScriptFu
; is an integer ID passed by the caller.
;
; In interactive mode, the widget to choose a resource
; initially shows a resource from the context.
; The user can choose a different value.
;
; The resource chooser widgets are buttons.
; The choice is previewed in the label of the button,
; or in a preview window beside the button.
; Clicking the button shows another dialog for choosing.
; Clicking a preview window shows an enlarged view of the choice.
;
; ----------------------------------------------------------------------
; SF-BRUSH
;
; Usage:
; SF-BRUSH "Brush to paint with" ""

; Note that v2 required a list for the third argument, the default.
; If you need the spacing attribute of the brush, get it from the brush.
; If you want to set opacity and mode when painting with the brush,
; declare more arguments and set them into a temporary context.
; ----------------------------------------------------------------------
; Usage:
; SF-FONT     "Font to render with"     ""
; SF-GRADIENT "Gradient to render with" ""
; SF-PALETTE  "Palette to render with"  ""
; SF-PATTERN  "Pattern to render with"  ""
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
; "Gimp" prefix. The second parameter specifies the default value, using
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
                                       "Sphere Layer" 100 LAYER-MODE-NORMAL)))
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
         (= shadow TRUE))
        (let ((shadow-w (* (* radius 2.5) (cos (+ *pi* radians))))
              (shadow-h (* radius 0.5))
              (shadow-x cx)
              (shadow-y (+ cy (* radius 0.65))))
          (if (< shadow-w 0)
              (begin (set! shadow-x (+ cx shadow-w))
                     (set! shadow-w (- shadow-w))))

          (gimp-context-set-feather TRUE)
          (gimp-context-set-feather-radius 7.5 7.5)
          (gimp-image-select-ellipse img CHANNEL-OP-REPLACE shadow-x shadow-y shadow-w shadow-h)
          (gimp-context-set-pattern pattern)
          (gimp-drawable-edit-fill drawable FILL-PATTERN)))

    (gimp-context-set-feather FALSE)
    (gimp-image-select-ellipse img CHANNEL-OP-REPLACE (- cx radius) (- cy radius)
                               (* 2 radius) (* 2 radius))

    (gimp-context-set-gradient-fg-bg-rgb)
    (gimp-drawable-edit-gradient-fill drawable
				      GRADIENT-RADIAL offset
				      FALSE 1 0
				      TRUE
				      light-x light-y
				      light-end-x light-end-y)

    (gimp-selection-none img)

    (gimp-image-select-ellipse img CHANNEL-OP-REPLACE 10 10 50 50)

    (gimp-context-set-gradient gradient)
    (gimp-context-set-gradient-reverse gradient-reverse)
    (gimp-drawable-edit-gradient-fill drawable
				      GRADIENT-LINEAR offset
				      FALSE 1 0
				      TRUE
				      10 10
				      30 60)

    (gimp-selection-none img)

    (gimp-context-set-foreground '(0 0 0))
    (gimp-floating-sel-anchor (car (gimp-text-font     img drawable
                                                       x-position y-position
                                                       multi-text
                                                       0 TRUE
                                                       size
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
  ; v3 even for extension-scriptfu old-style plugins,
  ; only declare name of default brush
  SF-BRUSH      "Brush"              "2. Hardness 100"
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
                         "<Image>/Filters/Development/Demos")
