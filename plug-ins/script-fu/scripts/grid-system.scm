;;; grid-system.scm -*-scheme-*-
;;; Time-stamp: <1997/07/03 23:30:19 narazaki@InetQ.or.jp>
;;; This file is a part of:
;;;   The GIMP (Copyright (C) 1995 Spencer Kimball and Peter Mattis)
;;; Author: Shuji Narazaki (narazaki@InetQ.or.jp)
;;; Version 0.2

;;; Code:
(define (script-fu-grid-system img drw x-divides y-divides)
  (define (update-segment! s x0 y0 x1 y1)
    (aset s 0 x0)
    (aset s 1 y0)
    (aset s 2 x1)
    (aset s 3 y1))
  (let* ((drw-width (car (gimp-drawable-width drw)))
	 (drw-height (car (gimp-drawable-height drw)))
	 (drw-offset-x (nth 0 (gimp-drawable-offsets drw)))
	 (drw-offset-y (nth 1 (gimp-drawable-offsets drw)))
	 (segment (cons-array 4 'double))
	 (stepped-x 0)
	 (stepped-y 0)
	 (temp 0)
	 (total-step-x (apply + x-divides))
	 (total-step-y (apply + y-divides)))
    (gimp-image-disable-undo img)
    (while (not (null? (cdr x-divides)))
      (set! stepped-x (+ stepped-x (car x-divides)))
      (set! temp (* drw-width (/ stepped-x total-step-x)))
      (set! x-divides (cdr x-divides))
      (update-segment! segment
		       (+ drw-offset-x temp) drw-offset-y
		       (+ drw-offset-x temp) (+ drw-offset-y drw-height))
      (gimp-pencil img drw 4 segment))
    (while (not (null? (cdr y-divides)))
      (set! stepped-y (+ stepped-y (car y-divides)))
      (set! temp (* drw-height (/ stepped-y total-step-y)))
      (set! y-divides (cdr y-divides))
      (update-segment! segment
		       drw-offset-x (+ drw-offset-y temp)
		       (+ drw-offset-x drw-width) (+ drw-offset-y temp))
      (gimp-pencil img drw 4 segment))
    (gimp-image-enable-undo img)
    (gimp-displays-flush)))

(script-fu-register "script-fu-grid-system" 
		    "<Image>/Script-Fu/Render/Make Grid System"
		    "Draw grid as specified by X-DIVIDES (list of propotions relative to the drawable) and Y-DIVIDES. The color and width of grid is detemined by the current settings of brush."
		    "Shuji Narazaki <narazaki@InetQ.or.jp>"
		    "Shuji Narazaki"
		    "1997"
		    "RGB*, INDEXED*, GRAY*"
		    SF-IMAGE "Image to use" 0
		    SF-DRAWABLE "Drawable to draw grid" 0
		    SF-VALUE "Grids: X" "'(1 5 1 5 1)"
		    SF-VALUE "Grids: Y" "'(1 5 1 5 1)"
)
  
;;; grid-system.scm ends here