; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
; 
; Beveled pattern arrow for web pages
; Copyright (C) 1997 Federico Mena Quintero
; federico@nuclecu.unam.mx
; 
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.
; 
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
; 
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


(define (map proc seq)
  (if (null? seq)
      '()
      (cons (proc (car seq))
	    (map proc (cdr seq)))))

(define (for-each proc seq)
  (if (not (null? seq))
      (begin
	(proc (car seq))
	(for-each proc (cdr seq)))))

(define (make-point x y)
  (cons x y))

(define (point-x p)
  (car p))

(define (point-y p)
  (cdr p))

(define (point-list->double-array point-list)
  (let* ((how-many (length point-list))
	 (a (cons-array (* 2 how-many) 'double))
	 (count 0))
    (for-each (lambda (p)
		(aset a (* count 2) (point-x p))
		(aset a (+ 1 (* count 2)) (point-y p))
		(set! count (+ count 1)))
	      point-list)
    a))

(define (rotate-points points size orientation)
  (map (lambda (p)
	 (let ((px (point-x p))
	       (py (point-y p)))
	   (cond ((eq? orientation 'right) (make-point px py))
		 ((eq? orientation 'left) (make-point (- size px) py))
		 ((eq? orientation 'up) (make-point py (- size px)))
		 ((eq? orientation 'down) (make-point py px)))))
       points))

(define (make-arrow size offset)
  (list (make-point offset offset)
	(make-point (- size offset) (/ size 2))
	(make-point offset (- size offset))))

(define (script-fu-beveled-pattern-arrow size orientation pattern)
  (let* ((old-bg-color (car (gimp-palette-get-background)))
	 (img (car (gimp-image-new size size RGB)))
	 (background (car (gimp-layer-new img size size RGB_IMAGE "Arrow" 100 NORMAL)))
	 (bumpmap (car (gimp-layer-new img size size RGB_IMAGE "Bumpmap" 100 NORMAL)))
	 (big-arrow (point-list->double-array (rotate-points (make-arrow size 6) size orientation)))
	 (med-arrow (point-list->double-array (rotate-points (make-arrow size 7) size orientation)))
	 (small-arrow (point-list->double-array (rotate-points (make-arrow size 8) size orientation))))

    (gimp-image-disable-undo img)
    (gimp-image-add-layer img background -1)
    (gimp-image-add-layer img bumpmap -1)

    ; Create pattern layer

    (gimp-palette-set-background '(0 0 0))
    (gimp-edit-fill img background)
    (gimp-patterns-set-pattern pattern)
    (gimp-bucket-fill img background PATTERN-BUCKET-FILL NORMAL 100 0 FALSE 0 0)

    ; Create bumpmap layer

    (gimp-edit-fill img bumpmap)

    (gimp-palette-set-background '(127 127 127))
    (gimp-rect-select img 1 1 (- size 2) (- size 2) REPLACE FALSE 0)
    (gimp-edit-fill img bumpmap)

    (gimp-palette-set-background '(255 255 255))
    (gimp-rect-select img 2 2 (- size 4) (- size 4) REPLACE FALSE 0)
    (gimp-edit-fill img bumpmap)

    (gimp-palette-set-background '(127 127 127))
    (gimp-free-select img 6 big-arrow REPLACE TRUE FALSE 0)
    (gimp-edit-fill img bumpmap)

    (gimp-palette-set-background '(0 0 0))
    (gimp-free-select img 6 med-arrow REPLACE TRUE FALSE 0)
    (gimp-edit-fill img bumpmap)

    (gimp-selection-none img)

    ; Bumpmap

    (plug-in-bump-map 1 img background bumpmap 135 45 2 0 0 0 0 TRUE FALSE 0)

    ; Darken arrow

    (gimp-palette-set-background '(255 255 255))
    (gimp-edit-fill img bumpmap)

    (gimp-palette-set-background '(192 192 192))
    (gimp-free-select img 6 small-arrow REPLACE TRUE FALSE 0)
    (gimp-edit-fill img bumpmap)

    (gimp-selection-none img)

    (gimp-layer-set-mode bumpmap MULTIPLY)

    (gimp-image-flatten img)

    (gimp-palette-set-background old-bg-color)
    (gimp-image-enable-undo img)
    (gimp-display-new img)))


(script-fu-register "script-fu-beveled-pattern-arrow"
		    "<Toolbox>/Xtns/Script-Fu/Web page themes/Beveled pattern/Arrow"
		    "Beveled pattern arrow"
		    "Federico Mena Quintero"
		    "Federico Mena Quintero"
		    "July 1997"
		    ""
		    SF-VALUE "Size"        "32"
		    SF-VALUE "Orientation" "'right"
		    SF-PATTERN "Pattern"     "Wood")
