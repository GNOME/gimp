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
; drop-shadow.scm   version 1.01   08/11/97
;
; CHANGE-LOG:
; 1.00 - initial release
; 1.01 - fixed the problem with a remaining copy of the selection
;
;
; Copyright (C) 1997 Sven Neumann (neumanns@uni-duesseldorf.de)
; 
;  
; Adds a drop-shadow of the current selection or alpha-channel. 
;
; This script is derived from my script add-shadow, which has become 
; obsolete now. Thanks to Andrew Donkin (ard@cs.waikato.ac.nz) for his 
; idea to add alpha-support to add-shadow.

  
(define (script-fu-drop-shadow image 
			       drawable 
			       shadow-transl-x 
			       shadow-transl-y 
			       shadow-blur
			       shadow-color
			       shadow-opacity
			       allow-resize)
  (let* ((shadow-blur (max shadow-blur 0))
	 (shadow-opacity (min shadow-opacity 100))
	 (shadow-opacity (max shadow-opacity 0))
	 (type (car (gimp-drawable-type-with-alpha drawable)))
	 (image-width (car (gimp-image-width image)))
	 (image-height (car (gimp-image-height image)))
	 (old-bg (car (gimp-palette-get-background))))

  (gimp-image-disable-undo image)
  
  (gimp-layer-add-alpha drawable)
  (if (= (car (gimp-selection-is-empty image)) TRUE)
      (begin
	(gimp-selection-layer-alpha image drawable)
	(set! from-selection FALSE))
      (begin 
	(set! from-selection TRUE)
	(set! active-selection (car (gimp-selection-save image)))))
  
  (set! selection-bounds (gimp-selection-bounds image))
  (set! select-offset-x (cadr selection-bounds))
  (set! select-offset-y (caddr selection-bounds))
  (set! select-width (- (cadr (cddr selection-bounds)) select-offset-x))
  (set! select-height (- (caddr (cddr selection-bounds)) select-offset-y))
  
  (set! shadow-width (+ select-width (* 2 shadow-blur)))
  (set! shadow-height (+ select-height (* 2 shadow-blur)))

  (set! shadow-offset-x (- select-offset-x shadow-blur))
  (set! shadow-offset-y (- select-offset-y shadow-blur))

  (if (= allow-resize TRUE)
      (begin
	(set! new-image-width image-width)
	(set! new-image-height image-height)
	(set! image-offset-x 0)
	(set! image-offset-y 0)

	(if (< (+ shadow-offset-x shadow-transl-x) 0)
	    (begin
	      	(set! image-offset-x (- 0 (+ shadow-offset-x shadow-transl-x)))
		(set! shadow-offset-x (- 0 shadow-transl-x))
		(set! new-image-width (- new-image-width image-offset-x))))

	(if (< (+ shadow-offset-y shadow-transl-y) 0)
	    (begin
	      	(set! image-offset-y (- 0 (+ shadow-offset-y shadow-transl-y)))
		(set! shadow-offset-y (- 0 shadow-transl-y))
		(set! new-image-height (- new-image-height image-offset-y))))

	(if (> (+ (+ shadow-width shadow-offset-x) shadow-transl-x) 
	       new-image-width)
	    (set! new-image-width 
		  (+ (+ shadow-width shadow-offset-x) shadow-transl-x)))

	(if (> (+ (+ shadow-height shadow-offset-y) shadow-transl-y) 
	       new-image-height)
	    (set! new-image-height
		  (+ (+ shadow-height shadow-offset-y) shadow-transl-y)))

	(gimp-image-resize image
			   new-image-width 
			   new-image-height 
			   image-offset-x 
			   image-offset-y)))


  (set! shadow-layer (car (gimp-layer-new image 
					  shadow-width 
					  shadow-height 
					  type
					  "Drop-Shadow" 
					  shadow-opacity
					  NORMAL)))
  (gimp-layer-set-offsets shadow-layer 
			  shadow-offset-x
			  shadow-offset-y)

  (gimp-drawable-fill shadow-layer TRANS-IMAGE-FILL)
  (gimp-palette-set-background shadow-color)
  (gimp-edit-fill image shadow-layer)
  (gimp-selection-none image)
  (gimp-layer-set-preserve-trans shadow-layer FALSE)
  (if (> shadow-blur 0) (plug-in-gauss-rle 1 
					   image 
					   shadow-layer 
					   shadow-blur 
					   TRUE 
					   TRUE))
  (gimp-image-add-layer image shadow-layer -1)
  (gimp-layer-translate shadow-layer shadow-transl-x shadow-transl-y)

  (if (= from-selection TRUE)
      (begin
	(gimp-selection-load image active-selection)
	(gimp-edit-clear image shadow-layer)
	(gimp-image-remove-channel image active-selection)))

  (if (and 
       (= (car (gimp-layer-is-floating-sel drawable)) 0)
       (= from-selection FALSE))
      (gimp-image-raise-layer image drawable))

  (gimp-image-set-active-layer image drawable)
  (gimp-palette-set-background old-bg)
  (gimp-image-enable-undo image)
  (gimp-displays-flush)))

(script-fu-register "script-fu-drop-shadow" 
		    "<Image>/Script-Fu/Shadow/Drop-Shadow"
		    "Add a drop-shadow of the current selection or 
                     alpha-channel"
		    "Sven Neumann (neumanns@uni-duesseldorf.de)"
		    "Sven Neumann"
		    "08/11/1997"
		    "RGB RGBA GRAY GRAYA"
		    SF-IMAGE "Image" 0
		    SF-DRAWABLE "Drawable" 0
		    SF-VALUE "X offset" "8"
		    SF-VALUE "Y offset" "8"
		    SF-VALUE "Blur Radius" "15"
		    SF-COLOR "Color" '(0 0 0)
		    SF-VALUE "Opacity" "80"
		    SF-TOGGLE "Allow Resizing" TRUE) 
























