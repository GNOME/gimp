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
; perspective-shadow.scm   version 1.01   08/11/97
;
; CHANGE-LOG:
; 1.00 - initial release
; 1.01 - fixed the problem with a remaining copy of the selection
;
;
; Copyright (C) 1997 Sven Neumann (neumanns@uni-duesseldorf.de)
; 
;  
; Adds a perspective shadow of the current selection or alpha-channel 
; as a layer below the active layer
;
  
(define (script-fu-perspective-shadow image 
				      drawable
				      alpha
				      rel-distance
				      rel-length
				      shadow-blur
				      shadow-color
				      shadow-opacity
				      interpolate
				      allow-resize)
  (let* ((shadow-blur (max shadow-blur 0))
	 (shadow-opacity (min shadow-opacity 100))
	 (shadow-opacity (max shadow-opacity 0))
	 (rel-length (abs rel-length))
	 (alpha (* (/ alpha 180) *pi*))
	 (type (car (gimp-drawable-type-with-alpha drawable)))
	 (image-width (car (gimp-image-width image)))
	 (image-height (car (gimp-image-height image)))
	 (old-bg (car (gimp-palette-get-background))))
    
  (if (= rel-distance 0) (set! rel-distance 999999)) 
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
 
  (set! abs-length (* rel-length select-height))
  (set! abs-distance (* rel-distance select-height))
  (set! half-bottom-width (/ select-width 2)) 
  (set! half-top-width (* half-bottom-width 
			  (/ (- rel-distance rel-length) rel-distance)))
  
  (set! x0 (+ select-offset-x (+ (- half-bottom-width half-top-width)
				 (* (cos alpha) abs-length))))
  (set! y0 (+ select-offset-y (- select-height
				 (* (sin alpha) abs-length))))
  (set! x1 (+ x0 (* 2 half-top-width)))
  (set! y1 y0)
  (set! x2 select-offset-x)
  (set! y2 (+ select-offset-y select-height))
  (set! x3 (+ x2 select-width))
  (set! y3 y2)
  
  (set! shadow-layer (car (gimp-layer-new image 
  					  select-width 
					  select-height 
					  type
					  "Perspective Shadow" 
					  shadow-opacity
					  NORMAL)))
  (gimp-layer-set-offsets shadow-layer select-offset-x select-offset-y)
  (gimp-drawable-fill shadow-layer TRANS-IMAGE-FILL)
  (gimp-palette-set-background shadow-color)
  (gimp-edit-fill image shadow-layer)
  (gimp-selection-none image)

  (set! shadow-width (+ (- (max x1 x3) (min x0 x2)) (* 2 shadow-blur)))
  (set! shadow-height (+ (- (max y1 y3) (min y0 y2)) (* 2 shadow-blur)))
  (set! shadow-offset-x (- (min x0 x2) shadow-blur))
  (set! shadow-offset-y (- (min y0 y2) shadow-blur))

  (if (= allow-resize TRUE)
      (begin
	(set! new-image-width image-width)
	(set! new-image-height image-height)
	(set! image-offset-x 0)
	(set! image-offset-y 0)

	(if (< shadow-offset-x 0)
	    (begin
	      	(set! image-offset-x (- 0 shadow-offset-x))
		(set! new-image-width (- new-image-width image-offset-x))))

	(if (< shadow-offset-y 0)
	    (begin
	      	(set! image-offset-y (- 0 shadow-offset-y))
		(set! new-image-height (- new-image-height image-offset-y))))

	(if (> (+ shadow-width shadow-offset-x) new-image-width)
	    (set! new-image-width (+ shadow-width shadow-offset-x)))

	(if (> (+ shadow-height shadow-offset-y) new-image-height)
	    (set! new-image-height (+ shadow-height shadow-offset-y)))
	(gimp-image-resize image
			   new-image-width 
			   new-image-height 
			   image-offset-x 
			   image-offset-y)))

  (gimp-image-add-layer image shadow-layer -1)
  
  (gimp-perspective image
		    shadow-layer
		    interpolate
		    x0 y0
		    x1 y1
		    x2 y2
		    x3 y3)

  (if (> shadow-blur 0) 
      (begin
	(gimp-layer-set-preserve-trans shadow-layer FALSE)
	(gimp-layer-resize shadow-layer 
			   shadow-width
			   shadow-height
			   shadow-blur
			   shadow-blur)
	(plug-in-gauss-rle 1 
			   image 
			   shadow-layer 
			   shadow-blur 
			   TRUE 
			   TRUE)))

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

(script-fu-register "script-fu-perspective-shadow" 
		    "<Image>/Script-Fu/Shadow/Perspective"
		    "Add a perspective shadow"
		    "Sven Neumann (neumanns@uni-duesseldorf.de)"
		    "Sven Neumann"
		    "08/11/1997"
		    "RGB RGBA GRAY GRAYA"
		    SF-IMAGE "Image" 0
		    SF-DRAWABLE "Drawable" 0
		    SF-VALUE "Angle" "45"
		    SF-VALUE "Relative horizon distance" "5.0"
		    SF-VALUE "Relative shadow length" "1.0"
		    SF-VALUE "Blur Radius" "3"
		    SF-COLOR "Color" '(0 0 0)
		    SF-VALUE "Opacity" "80"
		    SF-TOGGLE "Interpolate" TRUE
		    SF-TOGGLE "Allow Resizing" FALSE) 

























