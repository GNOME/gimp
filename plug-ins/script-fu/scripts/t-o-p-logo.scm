;;  Trace of Particles Effect
;; Shuji Narazaki (narazaki@InetQ.or.jp)
;; Time-stamp: <97/03/15 17:27:33 narazaki@InetQ.or.jp>
;; Version 0.2

(define (script-fu-t-o-p-logo text size fontname hit-rate edge-size edge-only base-color bg-color)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
	 (border (/ size 5))
	 (text-layer (car (gimp-text-fontname img -1 0 0 text (* border 2) TRUE size PIXELS fontname)))
	 (width (car (gimp-drawable-width text-layer)))
	 (height (car (gimp-drawable-height text-layer)))
	 (text-layer-mask (car (gimp-layer-create-mask text-layer BLACK-MASK)))
	 (sparkle-layer (car (gimp-layer-new img width height RGBA_IMAGE "Sparkle" 100 NORMAL)))
	 (shadow-layer (car (gimp-layer-new img width height RGBA_IMAGE "Shadow" 90 ADDITION)))
	 (bg-layer (car (gimp-layer-new img width height RGB_IMAGE "Background" 100 NORMAL)))
	 (selection 0)
	 (white '(255 255 255))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background)))
	 (old-brush (car (gimp-brushes-get-brush)))
	 (old-paint-mode (car (gimp-brushes-get-paint-mode))))
    (gimp-image-undo-disable img)
    (gimp-image-resize img width height 0 0)
    (gimp-image-add-layer img sparkle-layer 2)
    (gimp-image-add-layer img shadow-layer 3)
    (gimp-image-add-layer img bg-layer 4)
    (gimp-selection-none img)
    (gimp-edit-clear shadow-layer)
    (gimp-edit-clear sparkle-layer)
    (gimp-palette-set-background base-color)
    (gimp-edit-fill sparkle-layer)
    (gimp-palette-set-background base-color)
    (gimp-selection-layer-alpha text-layer)
    (set! selection (car (gimp-selection-save img)))
    (gimp-selection-grow img edge-size)
    '(plug-in-noisify 1 img sparkle-layer FALSE
		     (* 0.1 hit-rate) (* 0.1 hit-rate) (* 0.1 hit-rate) 0.0)
    (gimp-selection-border img edge-size)
    (plug-in-noisify 1 img sparkle-layer FALSE hit-rate hit-rate hit-rate 0.0)
    (gimp-selection-none img)
    (plug-in-sparkle 1 img sparkle-layer 0.03 0.49 width 6 15 1.0 0.0 0.0 0.0 FALSE FALSE FALSE 0)
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
    (gimp-palette-set-foreground '(0 0 0))
    (gimp-palette-set-background '(255 255 255))
    (gimp-brushes-set-brush "Circle Fuzzy (11)")
    (gimp-selection-feather img border)
    (gimp-edit-fill shadow-layer)
    (gimp-selection-none img)
    (gimp-palette-set-background base-color)
    (gimp-edit-fill bg-layer)
    (gimp-selection-load selection)
    (gimp-brushes-set-brush "Circle Fuzzy (07)")
    (gimp-palette-set-foreground '(255 255 255))
    (gimp-selection-none img)
    (gimp-palette-set-foreground old-fg)
    (gimp-palette-set-background old-bg)
    (gimp-brushes-set-brush old-brush)
    (gimp-brushes-set-paint-mode old-paint-mode)
    (gimp-layer-set-visible text-layer 0)
    (gimp-image-set-active-layer img sparkle-layer)
    (gimp-image-undo-enable img)
    (gimp-display-new img)))

(script-fu-register "script-fu-t-o-p-logo"
		    "<Toolbox>/Xtns/Script-Fu/Logos/Particle Trace..."
		    "Trace of Particles Effect"
		    "Shuji Narazaki (narazaki@InetQ.or.jp)"
		    "Shuji Narazaki"
		    "1997"
		    ""
		    SF-STRING "Text String" "The GIMP"
		    SF-ADJUSTMENT "Font size (in pixels)" '(100 1 1000 1 10 0 1)
		    SF-FONT "Font" "-*-becker-*-r-*-*-24-*-*-*-p-*-*-*"
                    SF-ADJUSTMENT  "Hit Rate [0.0,1.0]" '(0.2 0 1 .01 .01 2 0)
                    SF-ADJUSTMENT "Edge Width" '(2 0 128 1 1 0 0)
		    SF-TOGGLE "Edge Only?" FALSE
		    SF-COLOR "Base Color" '(0 40 0)
		    SF-COLOR "Background Color" '(255 255 255))

; end of t-o-p.scm
