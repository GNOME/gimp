; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
; 
; Predator effect
; Copyright (c) 1997 Adrian Likins
; aklikins@eos.ncsu.ed
;
;  The idea here is too make the image/selection look sort of like 
;  the view the predator had in the movies. ie, kind of a thermogram
;  type of thing. Works best on colorful rgb images.
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


(define (script-fu-predator image
			      drawable
			      edge-amount
			      pixelize
			      pixel-size
			      keep-selection
			      seperate-layer)
  (let* (
	 (type (car (gimp-drawable-type-with-alpha drawable)))
	 (image-width (car (gimp-image-width image)))
	 (image-height (car (gimp-image-height image)))
	 (old-gradient (car (gimp-gradients-get-active)))
	 (old-bg (car (gimp-palette-get-background))))
    
    (gimp-image-disable-undo image)
    (gimp-layer-add-alpha drawable)
    
    (if (= (car (gimp-selection-is-empty image)) TRUE)
	(begin
	  (gimp-selection-layer-alpha drawable)
	  (set! active-selection (car (gimp-selection-save image)))
	  (set! from-selection FALSE))
	(begin
	  
	  (set! from-selection TRUE)
	  (set! active-selection (car (gimp-selection-save image)))))
    
    (set! selection-bounds (gimp-selection-bounds image))
    (set! select-offset-x (cadr selection-bounds))
    (set! select-offset-y (caddr selection-bounds))
    (set! select-width (- (cadr (cddr selection-bounds)) select-offset-x))
    (set! select-height (- (caddr (cddr selection-bounds)) select-offset-y))
    
    (if (= seperate-layer TRUE)
	(begin
	  (set! effect-layer (car (gimp-layer-new image
						select-width
						select-height
						type
						"glow layer"
						100
						NORMAL)))
    
	  (gimp-layer-set-offsets effect-layer select-offset-x select-offset-y)
	  (gimp-image-add-layer image effect-layer -1)
	  (gimp-selection-none image)
	  (gimp-edit-clear effect-layer)
    
	  (gimp-selection-load active-selection)
	  (gimp-edit-copy drawable)
	  (let ((floating-sel (car (gimp-edit-paste effect-layer FALSE))))
	    (gimp-floating-sel-anchor floating-sel)
	    )
	  (gimp-image-set-active-layer image effect-layer )))
    (set! active-layer (car (gimp-image-get-active-layer image)))

    ; all the fun stuff goes here
    (if (= pixelize TRUE)
	(plug-in-pixelize 1 image active-layer pixel-size))
    (plug-in-max-rgb 1 image active-layer 0)
    (plug-in-edge 1 image active-layer edge-amount 1)
    
    ; clean up the selection copy
    (gimp-selection-load active-selection)
    (gimp-gradients-set-active old-gradient)
    (gimp-palette-set-background old-bg)
    
    (if (= keep-selection FALSE)
	(gimp-selection-none image))
    
    (gimp-image-enable-undo image)
    (gimp-image-set-active-layer image drawable)
    (gimp-image-remove-channel image active-selection)
    (gimp-displays-flush)))

(script-fu-register "script-fu-predator"
		    "<Image>/Script-Fu/Decor/Predator"
		    "Fills the current selection with test"
		    "Adrian Likins <adrian@gimp.org>"
		    "Adrian Likins"
		    "10/12/97"
		    "RGB RGBA"
		    SF-IMAGE "Image" 0
		    SF-DRAWABLE "Drawable" 0
		    SF-ADJUSTMENT "edge-amount" '(2 0 24 1 1 0 0)
		    SF-TOGGLE "pixelize?" TRUE
		    SF-ADJUSTMENT "Pixel Amount" '(3 1 16 1 1 0 0)
		    SF-TOGGLE "Keep Selection?" TRUE
		    SF-TOGGLE "Seperate Layer?" TRUE)






