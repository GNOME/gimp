; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
;
;
; selection-round.scm   version 1.02   1998/02/06
;
; CHANGE-LOG:
; 1.00 - initial release
; 1.01 - some code cleanup, no real changes
; 1.02 - made script undoable
;
; Copyright (C) 1997, 1998 Sven Neumann <sven@gimp.org>
; 
;  
; Rounds the current selection by cutting of rectangles from the edges and 
; adding circles. The relative radius describes the radius of this cricles
; in relation to the selections width or height (depends on which of these 
; is smaller) 

(define (script-fu-selection-round image
	 		           drawable
                                   radius)

  (let* ((radius (min radius 1.0))
	 (radius (max radius 0.0))
	 (select-bounds (gimp-selection-bounds image))
	 (has-selection (car select-bounds))
	 (select-x1 (cadr select-bounds))
	 (select-y1 (caddr select-bounds))
	 (select-x2 (cadr (cddr select-bounds)))
	 (select-y2 (caddr (cddr select-bounds)))
	 (select-width (- select-x2 select-x1))
	 (select-height (- select-y2 select-y1))
	 (cut-radius 0)
	 (ellipse-radius 0))

  (gimp-undo-push-group-start image)

  (if (> select-width select-height)
      (begin
	(set! cut-radius (* radius (/ select-height 2)))
	(set! ellipse-radius (* radius select-height)))
      (begin
	(set! cut-radius (* radius (/ select-width 2)))
	(set! ellipse-radius (* radius select-width))))
  (gimp-rect-select image
		    select-x1
		    select-y1
		    (+ cut-radius 1)
		    (+ cut-radius 1)
		    SUB
		    FALSE 0)
  (gimp-rect-select image
		    select-x1
		    (- select-y2 cut-radius)
		    (+ cut-radius 1)
		    (+ cut-radius 1)
		    SUB
		    FALSE 0)
  (gimp-rect-select image
		    (- select-x2 cut-radius)
		    select-y1
		    (+ cut-radius 1)
		    (+ cut-radius 1)
		    SUB
		    FALSE 0)
  (gimp-rect-select image
		    (- select-x2 cut-radius)
		    (- select-y2 cut-radius)
		    (+ cut-radius 1)
		    (+ cut-radius 1)
		    SUB
		    FALSE 0)
  (gimp-ellipse-select image
		       select-x1
		       select-y1
		       ellipse-radius
		       ellipse-radius
		       ADD
		       TRUE
		       FALSE 0)
  (gimp-ellipse-select image
		       select-x1
		       (- select-y2 ellipse-radius)
		       ellipse-radius
		       ellipse-radius
		       ADD
		       TRUE
		       FALSE 0)
  (gimp-ellipse-select image
		       (- select-x2 ellipse-radius)
		       select-y1
		       ellipse-radius
		       ellipse-radius
		       ADD
		       TRUE
		       FALSE 0)
  (gimp-ellipse-select image
		       (- select-x2 ellipse-radius)
		       (- select-y2 ellipse-radius)
		       ellipse-radius
		       ellipse-radius
		       ADD
		       TRUE
		       FALSE 0)

  (gimp-undo-push-group-end image)
  (gimp-displays-flush)))


(script-fu-register "script-fu-selection-round"
		    "<Image>/Script-Fu/Selection/Round..."
		    "Rounds the active selection. The selection should be rectangular."
		    "Sven Neumann <sven@gimp.org>"
		    "Sven Neumann"
		    "1998/02/06"
		    "*"
		    SF-IMAGE "Image" 0
		    SF-DRAWABLE "Drawable" 0
		    SF-ADJUSTMENT "Relative Radius" '(1 0 128 .1 1 1 1))
