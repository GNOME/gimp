;;; i26-gunya2.scm -*-scheme-*-
;;; Time-stamp: <1997/05/11 18:46:26 narazaki@InetQ.or.jp>
;;; Author: Shuji Narazaki (narazaki@InetQ.or.jp)

;;; Comment:
;;;  This is the first font decoration of Imigre-26 (i26)
;;; Code:

(define (script-fu-i26-gunya2 text text-color frame-color font font-size frame-size)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
	 (border (/ font-size 10))
	 (text-layer (car (gimp-text img -1 0 0 text (* border 2) TRUE font-size
				     PIXELS "*" font "*" "*" "*" "*")))
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
    (gimp-image-disable-undo img)
    (gimp-image-disable-undo distortion-img)
    (gimp-image-resize img width height 0 0)
    (gimp-image-add-layer img dist-text-layer -1)
    (gimp-image-add-layer img dist-frame-layer -1)
    (gimp-image-add-layer distortion-img distortion-layer -1)
    (gimp-selection-none img)
    (gimp-edit-clear img dist-text-layer)
    (gimp-edit-clear img dist-frame-layer)
    ;; get the text shape
    (gimp-selection-layer-alpha img text-layer)
    ;; fill it with the specified color
    (gimp-palette-set-background text-color)
    (gimp-edit-fill img dist-text-layer)
    ;; get the border shape
    (gimp-selection-border img frame-size)
    (gimp-palette-set-background frame-color)
    (gimp-edit-fill img dist-frame-layer)
    (gimp-selection-none img)
    ;; now make the distortion data
    (gimp-palette-set-background '(255 255 255))
    (gimp-edit-fill distortion-img distortion-layer)
    (plug-in-noisify 1 distortion-img distortion-layer FALSE prob prob prob 0.0)
    (plug-in-gauss-rle 1 distortion-img distortion-layer radius 1 1)
    (plug-in-c-astretch 1 distortion-img distortion-layer) 
    (plug-in-gauss-rle 1 distortion-img distortion-layer radius 1 1)
    ;; OK, apply it to dist-text-layer
    (plug-in-displace 1 img dist-text-layer radius radius 1 1
		      distortion-layer distortion-layer 0)
    ;; make the distortion data once again fro the frame
    (gimp-edit-fill distortion-img distortion-layer)
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
    (gimp-edit-fill img text-layer)
    ;; post processing
    (gimp-palette-set-foreground old-fg)
    (gimp-palette-set-background old-bg)
    (gimp-brushes-set-brush old-brush)
    (gimp-brushes-set-paint-mode old-paint-mode)
    (gimp-image-set-active-layer img dist-text-layer)
    (gimp-image-enable-undo img)
    (gimp-image-delete distortion-img)
    (gimp-display-new img)))


(script-fu-register "script-fu-i26-gunya2"
		    "<Toolbox>/Xtns/Script-Fu/Logos/Imigre-26"
		    "Two-colored text by hand"
		    "Shuji Narazaki"
		    "Shuji Narazaki"
		    "1997"
		    ""
		    SF-STRING "Text" "The GIMP"
		    SF-COLOR "Text Color" '(255 0 0)
		    SF-COLOR "Frame Color" '(0 34 255)
		    SF-STRING "Font" "Becker"
		    SF-VALUE "Font Size" "100"
		    SF-VALUE "Frame Size" "2")

;;; i26-gunya2.scm ends here



