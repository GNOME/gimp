;;; i26-gunya2.scm -*-scheme-*-
;;; Time-stamp: <1997/05/11 18:46:26 narazaki@InetQ.or.jp>
;;; Author: Shuji Narazaki (narazaki@InetQ.or.jp)
; ************************************************************************
; Changed on Feb 4, 1999 by Piet van Oostrum <piet@cs.uu.nl>
; For use with GIMP 1.1.
; All calls to gimp-text-* have been converted to use the *-fontname form.
; The corresponding parameters have been replaced by an SF-FONT parameter.
; ************************************************************************

;;; Comment:
;;;  This is the first font decoration of Imigre-26 (i26)
;;; Code:

(define (script-fu-i26-gunya2 text text-color frame-color font font-size frame-size)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
	 (border (/ font-size 10))
	 (text-layer (car (gimp-text-fontname img -1 0 0 text (* border 2)
					      TRUE font-size PIXELS font))) 
	 (width (car (gimp-drawable-width text-layer)))
	 (height (car (gimp-drawable-height text-layer)))
	 (dist-text-layer (car (gimp-layer-new img width height RGBA_IMAGE
					       "Distorted text" 100 NORMAL)))
	 (dist-frame-layer (car (gimp-layer-new img width height RGBA_IMAGE
						"Distorted text" 100 NORMAL)))
	 (distortion-img (car (gimp-image-new width height GRAY)))
	 (distortion-layer (car (gimp-layer-new distortion-img width height
						GRAY_IMAGE "temp" 100 NORMAL)))
	 (radius (/ font-size 10))
	 (prob 0.5)
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background)))
	 (old-brush (car (gimp-brushes-get-brush)))
	 (old-paint-mode (car (gimp-brushes-get-paint-mode))))
    (gimp-image-undo-disable img)
    (gimp-image-undo-disable distortion-img)
    (gimp-image-resize img width height 0 0)
    (gimp-image-add-layer img dist-text-layer -1)
    (gimp-image-add-layer img dist-frame-layer -1)
    (gimp-image-add-layer distortion-img distortion-layer -1)
    (gimp-selection-none img)
    (gimp-edit-clear dist-text-layer)
    (gimp-edit-clear dist-frame-layer)
    ;; get the text shape
    (gimp-selection-layer-alpha text-layer)
    ;; fill it with the specified color
    (gimp-palette-set-foreground text-color)
    (gimp-edit-fill dist-text-layer FG-IMAGE-FILL)
    ;; get the border shape
    (gimp-selection-border img frame-size)
    (gimp-palette-set-background frame-color)
    (gimp-edit-fill dist-frame-layer BG-IMAGE-FILL)
    (gimp-selection-none img)
    ;; now make the distortion data
    (gimp-palette-set-background '(255 255 255))
    (gimp-edit-fill distortion-layer BG-IMAGE-FILL)
    (plug-in-noisify 1 distortion-img distortion-layer FALSE prob prob prob 0.0)
    (plug-in-gauss-rle 1 distortion-img distortion-layer radius 1 1)
    (plug-in-c-astretch 1 distortion-img distortion-layer)
    (plug-in-gauss-rle 1 distortion-img distortion-layer radius 1 1)
    ;; OK, apply it to dist-text-layer
    (plug-in-displace 1 img dist-text-layer radius radius 1 1
		      distortion-layer distortion-layer 0)
    ;; make the distortion data once again fro the frame
    (gimp-edit-fill distortion-layer BG-IMAGE-FILL)
    (plug-in-noisify 1 distortion-img distortion-layer FALSE prob prob prob 0.0)
    (plug-in-gauss-rle 1 distortion-img distortion-layer radius 1 1)
    (plug-in-c-astretch 1 distortion-img distortion-layer)
    (plug-in-gauss-rle 1 distortion-img distortion-layer radius 1 1)
    ;; then, apply it to dist-frame-layer
    (plug-in-displace 1 img dist-frame-layer radius radius 1 1
		      distortion-layer distortion-layer 0)
    ;; Finally, clear the bottom layer (text-layer)
    (gimp-selection-all img)
    (gimp-palette-set-background '(255 255 255))
    (gimp-edit-fill text-layer BG-IMAGE-FILL)
    ;; post processing
    (gimp-palette-set-foreground old-fg)
    (gimp-palette-set-background old-bg)
    (gimp-brushes-set-brush old-brush)
    (gimp-brushes-set-paint-mode old-paint-mode)
    (gimp-image-set-active-layer img dist-text-layer)
    (gimp-selection-none img)
    (gimp-image-undo-enable img)
    (gimp-image-delete distortion-img)
    (gimp-display-new img)))


(script-fu-register "script-fu-i26-gunya2"
		    _"<Toolbox>/Xtns/Script-Fu/Logos/Imigre-26..."
		    "Two-colored text by hand"
		    "Shuji Narazaki"
		    "Shuji Narazaki"
		    "1997"
		    ""
		    SF-STRING     _"Text" "The GIMP"
		    SF-COLOR      _"Text Color" '(255 0 0)
		    SF-COLOR      _"Frame Color" '(0 34 255)
		    SF-FONT       _"Font" "-*-Becker-*-r-*-*-24-*-*-*-p-*-*-*"
		    SF-ADJUSTMENT _"Font Size (pixels)" '(100 2 1000 1 10 0 1)
		    SF-ADJUSTMENT _"Frame Size" '(2 1 20 1 5 0 1))

;;; i26-gunya2.scm ends here
