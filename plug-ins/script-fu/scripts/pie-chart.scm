; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
; 
; Pie chart script --- create simple pie charts from a list of value/color pairs
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


; Constants

(define pi 3.141592653589793238462643383279502884)
(define epsilon 1.0e-8)

; Points

(define (make-point x y)
  (cons x y))

(define (point-x point)
  (car point))

(define (point-y point)
  (cdr point))

; Convert a list of points to an array of doubles

(define (point-list->double-array point-list)
  (let* ((how-many (length point-list))
	 (count 0)
	 (a (cons-array (* 2 how-many) 'double)))
    (while point-list
	   (aset a (* 2 count) (point-x (car point-list)))
	   (aset a (+ 1 (* 2 count)) (point-y (car point-list)))
	   (set! point-list (cdr point-list))
	   (set! count (+ count 1)))
    a))

; Returns the point on the image border which is intersected by the
; vector which starts at the center of the image and that has the
; specified angle

(define (intersect-with-border center width angle)
  (let* ((vec-x (cos angle))
	 (vec-y (- (sin angle)))
	 (factor-x (if (> (abs vec-x) epsilon)
		       (max (/ (- center) vec-x)
			    (/ (- width center) vec-x))
		       -1))
	 (factor-y (if (> (abs vec-y) epsilon)
		       (max (/ (- center vec-y))
			    (/ (- width center) vec-y))
		       -1))
	 (factor (cond ((and (> factor-x 0) (> factor-y 0))
			(min factor-x factor-y))
		       ((> factor-x 0) factor-x)
		       (else factor-y))))
    (make-point (+ center (* vec-x factor))
		(+ center (* vec-y factor)))))

; On which side of the image does the point lie?

(define (which-side point width)
  (let ((x (point-x point))
	(y (point-y point)))
    (cond ((< x epsilon) 'left)
	  ((< y epsilon) 'top)
	  ((< (abs (- x width)) epsilon) 'right)
	  ((< (abs (- y width)) epsilon) 'bottom))))

; Tests whether a point comes after another point, even when their sides are equal

(define (point-after? point1 point2 side)
  (cond ((eq? side 'top) (< (point-x point1) (point-x point2)))
	((eq? side 'left) (> (point-y point1) (point-y point2)))
	((eq? side 'bottom) (> (point-x point1) (point-x point2)))
	((eq? side 'right) (< (point-y point1) (point-y point2)))))

; Rotates sides counter-clockwise

(define (next-side side)
  (cond ((eq? side 'top) 'left)
	((eq? side 'left) 'bottom)
	((eq? side 'bottom) 'right)
	((eq? side 'right) 'top)))

; Moves a point to the specified side

(define (move-point-to-side width point dest-side)
  (cond ((eq? dest-side 'left) (make-point 0 (point-y point)))
	((eq? dest-side 'right) (make-point width (point-y point)))
	((eq? dest-side 'top) (make-point (point-x point) 0))
	((eq? dest-side 'bottom) (make-point (point-x point) width))))

; Slides the initial point along the image border to the final point
; and returns a list of the visited points

(define (make-slide-point-list width point side final-point final-side)
  (if (and (eq? side final-side)
	   (point-after? point final-point side))
      (list point final-point)
      (cons point
	    (make-slide-point-list width
				   (move-point-to-side width point (next-side side))
				   (next-side side)
				   final-point
				   final-side))))

; Creates an array of points ready for gimp-free-select
  
(define (create-slice-intersect-array center width angle1 angle2)
  (let* ((inter-1 (intersect-with-border center width angle1))
	 (inter-2 (intersect-with-border center width angle2))
	 (point-list (cons (make-point center center)
			   (make-slide-point-list width
						  inter-1
						  (which-side inter-1 width)
						  inter-2
						  (which-side inter-2 width)))))
    (cons (length point-list)
	  (point-list->double-array point-list))))

; Value/color pairs

(define (get-value value-color-pair)
  (car value-color-pair))

(define (get-color value-color-pair)
  (cadr value-color-pair))

(define (calc-total value-color-list)
  (define (total-iter values total)
    (if (null? values)
	total
	(total-iter (cdr values)
		    (+ total (get-value (car values))))))
  (total-iter value-color-list 0))

; Misc

(define (degrees->radians angle)
  (/ (* angle pi) 180.0))

; The main pie-chart function

(define (script-fu-pie-chart width value-color-list start-angle)
  (define (paint-slices image drawable total angle1 values)
    (if (not (null? values))
	(let* ((item (car values))
	       (value (/ (get-value item) total))
	       (color (get-color item))
	       (angle2 (+ angle1
			  (* 2.0 pi value)))
	       (i-array (create-slice-intersect-array (/ (- width 1) 2)
						      width
						      angle1
						      angle2))
	       (num-points (car i-array))
	       (points (cdr i-array)))
	  
	  (gimp-selection-none image)
	  (gimp-ellipse-select image 0 0 width width REPLACE TRUE FALSE 0)
	  (gimp-free-select image
			    (* 2 num-points)
			    points
			    INTERSECT
			    TRUE
			    FALSE
			    0)
	  (gimp-palette-set-background color)
	  (gimp-edit-fill image drawable)
	  
	  (paint-slices image drawable total angle2 (cdr values)))))
  
  (let* ((img (car (gimp-image-new width width RGB)))
	 (layer (car (gimp-layer-new img width width RGB_IMAGE "Pie chart" 100 NORMAL)))
	 (old-fg-color (car (gimp-palette-get-foreground)))
	 (old-bg-color (car (gimp-palette-get-background)))
	 (total (calc-total value-color-list)))
    
    (gimp-image-disable-undo img)
    (gimp-image-add-layer img layer 0)
    
    (gimp-edit-fill img layer)
    
    (paint-slices img
		  layer
		  total
		  (degrees->radians start-angle)
		  value-color-list)
    
    (gimp-selection-none img)
    (gimp-palette-set-foreground old-fg-color)
    (gimp-palette-set-background old-bg-color)
    (gimp-image-enable-undo img)
    (gimp-display-new img)))

; Register!

(script-fu-register "script-fu-pie-chart"
		    "<Toolbox>/Xtns/Script-Fu/Misc/Pie chart"
		    "Pie chart"
		    "Federico Mena Quintero"
		    "Federico Mena Quintero"
		    "June 1997"
		    ""
		    SF-VALUE "Width" "401"
		    SF-VALUE "Value/color list" "'((10 (255 0 0)) (20 (0 0 255)) (30 (0 255 0)) (40 (255 255 0)))"
		    SF-VALUE "Start angle" "0.0")
