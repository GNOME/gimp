; This is a slightly modified copy of the sphere script to show and test 
; the possibilities of the new Script-Fu API extensions.
;
; ----------------------------------------------------------------------
; SF-ADJUSTMENT 
; is only useful in interactive mode, if you call a script from
; the console, it acts just like a normal SF-VALUE
; In interactive mode it creates an adjustment widget in the dialog.
;
; Usage: 
; SF-ADJUSTMENT "label" '(value, lower, upper, step_inc, page_inc, digits, type) 
;
; type is one of: SF-SLIDER(0), SF-SPINNER(1)
; ----------------------------------------------------------------------
; SF-FONT
; creates a font-selection widget in the dialog. It returns a fontname as
; a string. There are two new gimp-text procedures to ease the use of this
; return parameter:
;
;  (gimp-text-fontname image drawable x-pos y-pos text border antialias size unit font)
;  (gimp-text-get-extents-fontname text size unit font))
;
; where font is the fontname you get. The size specified in the fontname 
; is silently ignored. It is only used in the font-selector. So you are asked to
; set it to a useful value (24 pixels is a good choice) when using SF-FONT.
;
; Usage:
; SF-FONT "label" "fontname"		
; ----------------------------------------------------------------------
;
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
; Usage:-
; SF_BRUSH "Brush" '("Circle (03)" 1.0 44 0)
;
; Here the brush dialog will be popped up with a default brush of Circle (03)
; opacity 1.0, spacing 44 and paint mode of Normal (value 0).
; If this selection was unchanged the value passed to the function as a 
; paramater would be '("Circle (03)" 1.0 44 0). BTW the widget used
; is generally available in the libgimpui library for any plugin that
; wishes to select a brush.
; ----------------------------------------------------------------------
;
; SF-PATTERN
; Only useful in interactive mode. It will create a widget in the control
; dialog. The widget consists of a preview area (which when pressed will 
; produce a popup preview ) and a button with the "..." label. The button will
; popup a dialog where patterns can be selected.
;
; Usage:-
; SF-PATTERN "Pattern" "Maple Leaves"
;
; The vaule returned when the script is invoked is a string containing the 
; pattern name. If the above selection was not altered the string would 
; contain "Maple Leaves"
; ----------------------------------------------------------------------
;
; SF-GRADIENT
; Only useful in interactive mode. It will create a widget in the control
; dialog. The widget consists of a button containing a preview of the selected
; gradient. If the button is pressed a gradient selection dialog will popup.
; 
;
; Usage:-
; SF-GRADIENT "Gradient" "Deep_Sea"
;
; The vaule returned when the script is invoked is a string containing the 
; gradient name. If the above selection was not altered the string would 
; contain "Deep_Sea"
;
; ----------------------------------------------------------------------
;
; SF-FILENAME
; Only useful in interactive mode. It will create a widget in the control
; dialog. The widget consists of a button containing the name of a file.
; If the button is pressed a file selection dialog will popup.
;
; Usage:-
; SF-FILENAME "Environment Map" (string-append "" gimp-data-dir "/scripts/beavis.jpg"
;
; The vaule returned when the script is invoked is a string containing the 
; filename.


;
(define (script-fu-test-sphere radius light shadow bg-color sphere-color brush text pattern gradient font size filename)
  (let* ((width (* radius 3.75))
	 (height (* radius 2.5))
	 (img (car (gimp-image-new width height RGB)))
	 (drawable (car (gimp-layer-new img width height RGB_IMAGE "Sphere Layer" 100 NORMAL)))
	 (radians (/ (* light *pi*) 180))
	 (cx (/ width 2))
	 (cy (/ height 2))
	 (light-x (+ cx (* radius (* 0.6 (cos radians)))))
	 (light-y (- cy (* radius (* 0.6 (sin radians)))))
	 (light-end-x (+ cx (* radius (cos (+ *pi* radians)))))
	 (light-end-y (- cy (* radius (sin (+ *pi* radians)))))
	 (offset (* radius 0.1))
	 (text-extents (gimp-text-get-extents-fontname text size PIXELS font))
	 (x-position (- cx (/ (car text-extents) 2)))
	 (y-position (- cy (/ (cadr text-extents) 2)))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background))))
    (gimp-image-disable-undo img)
    (gimp-image-add-layer img drawable 0)
    (gimp-palette-set-foreground sphere-color)
    (gimp-palette-set-background bg-color)
    (gimp-edit-fill img drawable)
    (gimp-palette-set-background '(20 20 20))
    (if (and
	 (or (and (>= light 45) (<= light 75)) (and (<= light 135) (>= light 105)))
	 (= shadow TRUE))
	(let ((shadow-w (* (* radius 2.5) (cos (+ *pi* radians))))
	      (shadow-h (* radius 0.5))
	      (shadow-x cx)
	      (shadow-y (+ cy (* radius 0.65))))
	  (if (< shadow-w 0)
	      (prog1 (set! shadow-x (+ cx shadow-w))
		     (set! shadow-w (- shadow-w))))
	  (gimp-ellipse-select img shadow-x shadow-y shadow-w shadow-h REPLACE TRUE TRUE 7.5)
	  (gimp-patterns-set-pattern pattern)
	  (gimp-bucket-fill img drawable PATTERN-BUCKET-FILL MULTIPLY 100 0 FALSE 0 0)))
    (gimp-ellipse-select img (- cx radius) (- cy radius) (* 2 radius) (* 2 radius) REPLACE TRUE FALSE 0)
    (gimp-blend img drawable FG-BG-RGB NORMAL RADIAL 100 offset REPEAT-NONE
		FALSE 0 0 light-x light-y light-end-x light-end-y)
    (gimp-selection-none img)

    (gimp-gradients-set-active gradient)
    (gimp-ellipse-select img 10 10 50 50 REPLACE TRUE FALSE 0)
    (gimp-blend img drawable CUSTOM NORMAL LINEAR 100 offset REPEAT-NONE
		FALSE 0 0 10 10 30 60)
    (gimp-selection-none img)

    (gimp-palette-set-foreground '(0 0 0))
    (gimp-floating-sel-anchor (car (gimp-text-fontname img drawable 
						       x-position y-position
						       text
						       0 TRUE 
						       size PIXELS 
						       font)))

    (gimp-palette-set-background old-bg)
    (gimp-palette-set-foreground old-fg)
    (gimp-image-enable-undo img)
    (gimp-display-new img)))

(script-fu-register "script-fu-test-sphere"
		    "<Toolbox>/Xtns/Script-Fu/Test/Sphere"
		    "Simple script to test and show the usage of the new Script-Fu API extensions. \n\nNote the use of spinbuttons, sliders, the font, pattern, brush and gradient selectors and do not forget the about dialog..."
		    "Spencer Kimball, Sven Neumann"
		    "Spencer Kimball"
		    "1996, 1998"
		    ""
		    SF-ADJUSTMENT "Radius (in pixels)" '(100 1 5000 1 10 0 1)
		    SF-ADJUSTMENT "Lighting (degrees)" '(45 0 360 1 10 1 0)
		    SF-TOGGLE "Shadow" TRUE
		    SF-COLOR "Background Color" '(255 255 255)
		    SF-COLOR "Sphere Color" '(255 0 0)
	            SF-BRUSH "Brush" '("Circle (03)" 1.0 44 0)
		    SF-STRING "Text" "Script-Fu rocks!"
		    SF-PATTERN "Pattern" "Maple Leaves"
		    SF-GRADIENT "Gradient" "Deep_Sea"
		    SF-FONT "Font" "-freefont-agate-normal-r-normal-*-24-*-*-*-p-*-*-*"
                    SF-ADJUSTMENT "Font size (in pixels)" '(50 1 1000 1 10 0 1)
		    SF-FILENAME "Environment Map" (string-append "" gimp-data-dir "/scripts/beavis.jpg"))


