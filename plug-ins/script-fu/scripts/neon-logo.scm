;  NEON
;  Create a text effect that simulates neon lighting

(define (set-pt a index x y)
  (prog1
   (aset a (* index 2) x)
   (aset a (+ (* index 2) 1) y)))

(define (neon-spline1)
  (let* ((a (cons-array 6 'byte)))
    (set-pt a 0 0 0)
    (set-pt a 1 127 145)
    (set-pt a 2 255 255)
    a))

(define (neon-spline2)
  (let* ((a (cons-array 6 'byte)))
    (set-pt a 0 0 0)
    (set-pt a 1 110 150)
    (set-pt a 2 255 255)
    a))

(define (neon-spline3)
  (let* ((a (cons-array 6 'byte)))
    (set-pt a 0 0 0)
    (set-pt a 1 100 185)
    (set-pt a 2 255 255)
    a))

(define (neon-spline4)
  (let* ((a (cons-array 8 'byte)))
    (set-pt a 0 0 0)
    (set-pt a 1 64 64)
    (set-pt a 2 127 192)
    (set-pt a 3 255 255)
    a))

(define (find-hue-offset color)
  (let* ((R (car color))
	 (G (cadr color))
	 (B (caddr color))
	 (max-val (max R G B))
	 (min-val (min R G B))
	 (delta (- max-val min-val))
	 (hue 0))
    (if (= max 0) 0
	(begin
	  (cond ((= max-val R) (set! hue (/ (- G B) (* 1.0 delta))))
		((= max-val G) (set! hue (+ 2 (/ (- B R) (* 1.0 delta)))))
		((= max-val B) (set! hue (+ 4 (/ (- R G) (* 1.0 delta))))))
	  (set! hue (* hue 60))
	  (if (< hue 0) (set! hue (+ hue 360)))
	  (if (> hue 360) (set! hue (- hue 360)))
	  (if (> hue 180) (set! hue (- hue 360)))
	  hue))))

(define (apply-neon-logo-effect img
				tube-layer
				size
				bg-color
				glow-color
				shadow)
  (let* ((tube-hue (find-hue-offset glow-color))
	 (shrink (/ size 14))
	 (grow (/ size 40))
	 (feather (/ size 5))
	 (feather1 (/ size 25))
	 (feather2 (/ size 12))
	 (inc-shrink (/ size 100))
	 (shadow-shrink (/ size 40))
	 (shadow-feather (/ size 20))
	 (shadow-offx (/ size 10))
	 (shadow-offy (/ size 10))
	 (width (car (gimp-drawable-width tube-layer)))
	 (height (car (gimp-drawable-height tube-layer)))
	 (glow-layer (car (gimp-layer-new img width height RGBA_IMAGE "Neon Glow" 100 NORMAL)))
	 (bg-layer (car (gimp-layer-new img width height RGB_IMAGE "Background" 100 NORMAL)))
	 (shadow-layer (if (= shadow TRUE)
			   (car (gimp-layer-new img width height RGBA_IMAGE "Shadow" 100 NORMAL))
			   0))
	 (selection 0)
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background))))
    (gimp-image-resize img width height 0 0)
    (gimp-image-add-layer img bg-layer 1)
    (if (not (= shadow 0))
	(begin
	  (gimp-image-add-layer img shadow-layer 1)
	  (gimp-edit-clear shadow-layer)))
    (gimp-image-add-layer img glow-layer 1)

    (gimp-palette-set-background '(0 0 0))
    (gimp-selection-layer-alpha tube-layer)
    (set! selection (car (gimp-selection-save img)))
    (gimp-selection-none img)

    (gimp-edit-clear glow-layer)
    (gimp-edit-clear tube-layer)

    (gimp-palette-set-background bg-color)
    (gimp-edit-fill bg-layer BG-IMAGE-FILL)

    (gimp-selection-load selection)
    (gimp-palette-set-background '(255 255 255))
    (gimp-edit-fill tube-layer BG-IMAGE-FILL)
    (gimp-selection-shrink img shrink)
    (gimp-palette-set-background '(0 0 0))
    (gimp-edit-fill selection BG-IMAGE-FILL)
    (gimp-edit-clear tube-layer)

    (gimp-selection-none img)
    (if (not (= feather1 0)) (plug-in-gauss-rle 1 img tube-layer feather1 TRUE TRUE))
    (gimp-selection-load selection)
    (if (not (= feather2 0)) (plug-in-gauss-rle 1 img tube-layer feather2 TRUE TRUE))

    (gimp-selection-feather img inc-shrink)
    (gimp-selection-shrink img inc-shrink)
    (gimp-curves-spline tube-layer 4 6 (neon-spline1))

    (gimp-selection-load selection)
    (gimp-selection-feather img inc-shrink)
    (gimp-selection-shrink img (* inc-shrink 2))
    (gimp-curves-spline tube-layer 4 6 (neon-spline2))

    (gimp-selection-load selection)
    (gimp-selection-feather img inc-shrink)
    (gimp-selection-shrink img (* inc-shrink 3))
    (gimp-curves-spline tube-layer 4 6 (neon-spline3))

    (gimp-layer-set-preserve-trans tube-layer 1)
    (gimp-selection-layer-alpha tube-layer)
    (gimp-selection-invert img)
    (gimp-palette-set-background glow-color)
    (gimp-edit-fill tube-layer BG-IMAGE-FILL)

    (gimp-selection-none img)
    (gimp-layer-set-preserve-trans tube-layer 0)
    (gimp-curves-spline tube-layer 4 8 (neon-spline4))

    (gimp-selection-load selection)
    (gimp-selection-grow img grow)
    (gimp-selection-invert img)
    (gimp-edit-clear tube-layer)
    (gimp-selection-invert img)

    (gimp-selection-feather img feather)
    (gimp-edit-fill glow-layer BG-IMAGE-FILL)

    (if (not (= shadow 0))
	(begin
          (gimp-selection-load selection)
          (gimp-selection-grow img grow)
	  (gimp-selection-shrink img shadow-shrink)
	  (gimp-selection-feather img shadow-feather)
	  (gimp-selection-translate img shadow-offx shadow-offy)
	  (gimp-palette-set-background '(0 0 0))
	  (gimp-edit-fill shadow-layer BG-IMAGE-FILL)))
    (gimp-selection-none img)

    (gimp-layer-set-name tube-layer "Neon Tubes")
    (gimp-palette-set-background old-bg)
    (gimp-palette-set-foreground old-fg)
    (gimp-image-remove-channel img selection)))

(define (script-fu-neon-logo-alpha img
				   tube-layer
				   size
				   bg-color
				   glow-color
				   shadow)
  (begin
    (gimp-undo-push-group-start img)
    (apply-neon-logo-effect img tube-layer size bg-color glow-color shadow)
    (gimp-undo-push-group-end img)
    (gimp-displays-flush)))

(script-fu-register "script-fu-neon-logo-alpha"
		    _"<Image>/Script-Fu/Alpha to Logo/Neon..."
		    "Neon logos"
		    "Spencer Kimball"
		    "Spencer Kimball"
		    "1997"
		    "RGBA"
                    SF-IMAGE      "Image" 0
                    SF-DRAWABLE   "Drawable" 0
		    SF-ADJUSTMENT _"Effect Size (pixels * 5)" '(150 2 1000 1 10 0 1)
		    SF-COLOR      _"Background Color" '(0 0 0)
		    SF-COLOR      _"Glow Color" '(38 211 255)
		    SF-TOGGLE     _"Create Shadow" FALSE
		    )

(define (script-fu-neon-logo text
			     size
			     font
			     bg-color
			     glow-color
			     shadow)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
	 (border (/ size 4))
	 (tube-layer (car (gimp-text-fontname img -1 0 0 text border TRUE size PIXELS font))))
    (gimp-image-undo-disable img)
    (gimp-layer-set-name tube-layer text)
    (apply-neon-logo-effect img tube-layer size bg-color glow-color shadow)
    (gimp-image-undo-enable img)
    (gimp-display-new img)))

(script-fu-register "script-fu-neon-logo"
		    _"<Toolbox>/Xtns/Script-Fu/Logos/Neon..."
		    "Neon logos"
		    "Spencer Kimball"
		    "Spencer Kimball"
		    "1997"
		    ""
		    SF-STRING     _"Text" "NEON"
		    SF-ADJUSTMENT _"Font Size (pixels)" '(150 2 1000 1 10 0 1)
		    SF-FONT       _"Font" "-*-Blippo-*-*-*-*-24-*-*-*-*-*-*-*"
		    SF-COLOR      _"Background Color" '(0 0 0)
		    SF-COLOR      _"Glow Color" '(38 211 255)
		    SF-TOGGLE     _"Create Shadow" FALSE
		    )
