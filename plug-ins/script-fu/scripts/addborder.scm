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
; Copyright (C) 1997 Andy Thomas alt@picnic.demon.co.uk
;
; Version 0.2 10.6.97 Changed to new script-fu interface in 0.99.10

; Delta the colour by the given amount. Check for boundary conditions
; If < 0 set to zero 
; If > 255 set to 255
; Return the new value

(define (deltacolour col delta)
  (let* ((newcol (+ col delta)))
;    (print "Old colour -") (print col)
    (if (< newcol 0) (set! newcol 0))
    (if (> newcol 255) (set! newcol 255))
;    (print "New colour") (print newcol)
    newcol)
  )

(define (adjcolour col delta)
  (mapcar (lambda (x) (deltacolour x delta)) col)
)

(define (gen_top_array xsize ysize owidth oheight width height)
  (let* ((n_array (cons-array 10 'double)))
    (aset n_array 0 0 )
    (aset n_array 1 0 )
    (aset n_array 2 xsize)
    (aset n_array 3 ysize)
    (aset n_array 4 (+ xsize owidth))
    (aset n_array 5 ysize)
    (aset n_array 6 width)
    (aset n_array 7 0 )
    (aset n_array 8 0 )
    (aset n_array 9 0 )
    n_array)
)

(define (gen_left_array xsize ysize owidth oheight width height)
  (let* ((n_array (cons-array 10 'double)))
    (aset n_array 0 0 )
    (aset n_array 1 0 )
    (aset n_array 2 xsize)
    (aset n_array 3 ysize)
    (aset n_array 4 xsize)
    (aset n_array 5 (+ ysize oheight))
    (aset n_array 6 0 )
    (aset n_array 7 height )
    (aset n_array 8 0 )
    (aset n_array 9 0 )
    n_array)
)

(define (gen_right_array xsize ysize owidth oheight width height)
  (let* ((n_array (cons-array 10 'double)))
    (aset n_array 0 width )
    (aset n_array 1 0 )
    (aset n_array 2 (+ xsize owidth))
    (aset n_array 3 ysize)
    (aset n_array 4 (+ xsize owidth))
    (aset n_array 5 (+ ysize oheight))
    (aset n_array 6 width)
    (aset n_array 7 height)
    (aset n_array 8 width )
    (aset n_array 9 0 )
    n_array)
)

(define (gen_bottom_array xsize ysize owidth oheight width height)
  (let* ((n_array (cons-array 10 'double)))
    (aset n_array 0 0 )
    (aset n_array 1 height)
    (aset n_array 2 xsize)
    (aset n_array 3 (+ ysize oheight))
    (aset n_array 4 (+ xsize owidth))
    (aset n_array 5 (+ ysize oheight))
    (aset n_array 6 width)
    (aset n_array 7 height)
    (aset n_array 8 0 )
    (aset n_array 9 height)
    n_array)
)

(define (script-fu-addborder aimg adraw xsize ysize colour dvalue)
  (let* ((img (car (gimp-drawable-image adraw)))
	 (owidth (car (gimp-image-width img)))
	 (oheight (car (gimp-image-height img)))
	 (old-fg (car (gimp-palette-get-foreground)))
         (old-bg (car (gimp-palette-get-background)))
	 (width (+ owidth (* 2 xsize)))
	 (height (+ oheight (* 2 ysize)))
	 (layer (car (gimp-layer-new img width height RGBA_IMAGE "Border-Layer" 100 NORMAL))))
;Add this for debugging    (verbose 4)
    (gimp-image-disable-undo img)
    (gimp-drawable-fill layer TRANS-IMAGE-FILL)
    (gimp-image-resize img
		       width
    		       height
    		       xsize
    		       ysize)
    (gimp-palette-set-background (adjcolour colour dvalue))
    (gimp-free-select img
		      10
		      (gen_top_array xsize ysize owidth oheight width height)
		      REPLACE
		      0
		      0
		      0.0)
    (gimp-edit-fill layer)
    (gimp-palette-set-background (adjcolour colour (/ dvalue 2)))
    (gimp-free-select img
		      10
		      (gen_left_array xsize ysize owidth oheight width height)
		      REPLACE
		      0
		      0
		      0.0)
    (gimp-edit-fill layer)
    (gimp-palette-set-background (adjcolour colour (- 0 (/ dvalue 2))))
    (gimp-free-select img
		      10
		      (gen_right_array xsize ysize owidth oheight width height)
		      REPLACE
		      0
		      0
		      0.0)

    (gimp-edit-fill layer)
    (gimp-palette-set-background (adjcolour colour (- 0 dvalue)))
    (gimp-free-select img
		      10
		      (gen_bottom_array xsize ysize owidth oheight width height)
		      REPLACE
		      0
		      0
		      0.0)

    (gimp-edit-fill layer)
    (gimp-selection-none img)
    (gimp-image-add-layer img layer 0)
    (gimp-image-enable-undo img)
    (gimp-palette-set-background old-bg)
    (gimp-displays-flush)
    )
)


(script-fu-register "script-fu-addborder"
		    "<Image>/Script-Fu/Modify/Add Border"
		    "Add a border around an image"
		    "Andy Thomas <alt@picnic.demon.co.uk>"
		    "Andy Thomas"
		    "6/10/97"
		    "RGB*"
		    SF-IMAGE "Input Image" 0
		    SF-DRAWABLE "Input Drawable" 0
		    SF-VALUE "Border x size" "12"
		    SF-VALUE "Border y size" "12"
		    SF-COLOR "Border Colour" '(38 31 207)
		    SF-VALUE "Delta value on colour" "25")
