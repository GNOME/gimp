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
; blend-anim.scm   version 1.00   08/11/97
;
; Copyright (C) 1997 Sven Neumann (neumanns@uni-duesseldorf.de)
; 
;  
; Blends two or more layers over a backgound, so that an animation can
; be saved. A minimum of three layers is required.

(define (multi-raise-layer image layer times)
  (while (> times 0)
	 (gimp-image-raise-layer image layer)
	 (set! times (- times 1))))
	   
  
(define (script-fu-blend-anim img
			      drawable
			      frames
			      max-blur
			      looped)
  (let* ((max-blur (max max-blur 0))
	 (frames (max frames 0))
	 (image (car (gimp-channel-ops-duplicate img)))
	 (width (car (gimp-image-width image)))
	 (height (car (gimp-image-height image)))
	 (layers (gimp-image-get-layers image))
	 (num-layers (car layers))
	 (layer-array (cadr layers)))
  
  (if (> num-layers 2)
      (begin

	(gimp-image-disable-undo image)

	(if (= looped TRUE)
	    (begin
	      (set! copy (car (gimp-layer-copy 
                                 (aref layer-array (- num-layers 2)) TRUE)))
	      (gimp-image-add-layer image copy 0)
	      (set! layers (gimp-image-get-layers image))
	      (set! num-layers (car layers))
	      (set! layer-array (cadr layers))))

		
	(set! bg-layer (aref layer-array (- num-layers 1)))
	(set! slots (- num-layers 2)) 
	
	; make all layers invisible and check for sizes
	(set! max-width 0)
        (set! max-height 0)
        (set! min-offset-x width)
        (set! min-offset-y height)
	(set! layer-count slots)
	(gimp-layer-set-visible bg-layer FALSE)
	(while (> layer-count -1)
	       (set! layer (aref layer-array layer-count))
	       (gimp-layer-set-visible layer FALSE)
	       (set! layer-width (+ (car (gimp-drawable-width layer))
				    (* max-blur 2)))
               (set! layer-height (+ (car (gimp-drawable-height layer))
				    (* max-blur 2)))
	       (set! layer-offsets (gimp-drawable-offsets layer))
	       (set! layer-offset-x (- (car layer-offsets) max-blur))
	       (set! layer-offset-y (- (cadr layer-offsets) max-blur))
	       (set! max-width (max max-width layer-width))
	       (set! max-height (max max-height layer-height))
	       (set! min-offset-x (min min-offset-x layer-offset-x))
	       (set! min-offset-y (min min-offset-y layer-offset-y))	       
	       (set! layer-count (- layer-count 1)))
	(set! offset-x (- (car (gimp-drawable-offsets bg-layer)) 
			  min-offset-x))
	(set! offset-y (- (cadr (gimp-drawable-offsets bg-layer)) 
			  min-offset-y))
	
        (set! layer-count slots)
	(while (> layer-count 0)
	   (set! frame-count frames)
	   (set! lower-layer (aref layer-array layer-count))
	   (set! upper-layer (aref layer-array (- layer-count 1)))
	   (while (> frame-count 0)
	      (set! opacity (* (/ frame-count (+ frames 1)) 100))
	      (set! blur (/ (* opacity max-blur) 100))
	      (set! upper-copy (car (gimp-layer-copy upper-layer TRUE)))
	      (set! lower-copy (car (gimp-layer-copy lower-layer TRUE)))
	      (set! bg-copy (car (gimp-layer-copy bg-layer TRUE)))
	      (gimp-image-add-layer image bg-copy 0)
	      (gimp-image-add-layer image lower-copy 0)
	      (gimp-image-add-layer image upper-copy 0)
	      (gimp-layer-set-visible upper-copy TRUE)
	      (gimp-layer-set-visible lower-copy TRUE)
	      (gimp-layer-set-visible bg-copy TRUE)
	      (gimp-layer-set-opacity upper-copy (- 100 opacity))
	      (gimp-layer-set-opacity lower-copy opacity)
	      (gimp-layer-set-opacity bg-copy 100)
	      (if (> max-blur 0)
		  (begin
		    (gimp-layer-set-preserve-trans upper-copy FALSE)
		    (set! layer-width (car (gimp-drawable-width upper-copy)))
		    (set! layer-height (car (gimp-drawable-height upper-copy)))
		    (gimp-layer-resize upper-copy
				       (+ layer-width (* blur 2))
				       (+ layer-height (* blur 2))
				       blur
				       blur)
		    (plug-in-gauss-rle 1 
				       image 
				       upper-copy 
				       blur 
				       TRUE TRUE)
		    (set! blur (- max-blur blur))
		    (gimp-layer-set-preserve-trans lower-copy FALSE)
		    (set! layer-width (car (gimp-drawable-width lower-copy)))
		    (set! layer-height (car (gimp-drawable-height lower-copy)))
		    (gimp-layer-resize lower-copy
				       (+ layer-width (* blur 2))
				       (+ layer-height (* blur 2))
				       blur
				       blur)
		    (plug-in-gauss-rle 1 
				       image 
				       lower-copy 
				       blur 
				       TRUE TRUE)))
	      (gimp-layer-resize bg-copy 
				 max-width
				 max-height
				 offset-x
				 offset-y)
	      (set! merged-layer (car (gimp-image-merge-visible-layers 
				       image CLIP-TO-IMAGE)))
	      (gimp-layer-set-visible merged-layer FALSE)
	      (set! frame-count (- frame-count 1)))
	   (set! layer-count (- layer-count 1)))

	(set! layer-count 0)
	(while (< layer-count slots)
	   (set! orig-layer (aref layer-array layer-count))    
	   (set! bg-copy (car (gimp-layer-copy bg-layer TRUE)))
	   (gimp-image-add-layer image 
				 bg-copy 
				 (* layer-count (+ frames 1)))
	   (multi-raise-layer image 
	                      orig-layer 
                              (+ (* (- slots layer-count) frames) 1))
	   (gimp-layer-set-visible orig-layer TRUE)
	   (gimp-layer-set-visible bg-copy TRUE)
	   (gimp-layer-resize bg-copy 
			      max-width
			      max-height
			      offset-x
			      offset-y)
	   (set! merged-layer (car (gimp-image-merge-visible-layers 
				    image CLIP-TO-IMAGE)))
	   (gimp-layer-set-visible merged-layer FALSE)
	   (set! layer-count (+ layer-count 1)))

	(set! orig-layer (aref layer-array (- num-layers 2)))
	(gimp-layer-set-visible bg-layer TRUE)
	(gimp-layer-set-visible orig-layer TRUE)
	(gimp-image-merge-visible-layers image CLIP-TO-IMAGE)

	; make all layers visible again
	(set! result-layers (gimp-image-get-layers image))
	(set! num-result-layers (car result-layers))
	(set! result-layer-array (cadr result-layers))
	(set! layer-count (- num-result-layers 1))
	(while (> layer-count -1)
	   (set! layer (aref result-layer-array layer-count))
	   (gimp-layer-set-visible layer TRUE)
	   (set! name (string-append "Frame " 
				     (number->string 
				      (- num-result-layers layer-count) 10)))
	   (gimp-layer-set-name layer name)
	   (set! layer-count (- layer-count 1)))	   

	(if (= looped TRUE)
	    (gimp-image-remove-layer image (aref result-layer-array 0)))
	      
	(gimp-image-enable-undo image)
	(gimp-display-new image)
	(gimp-displays-flush)))))

(script-fu-register "script-fu-blend-anim" 
		    "<Image>/Script-Fu/Animators/Blend"
		    "Blend two or more layers over a background, so that an 
                     animation can be saved"
		    "Sven Neumann (neumanns@uni-duesseldorf.de)"
		    "Sven Neumann"
		    "08/11/1997"
		    "RGB RGBA GRAY GRAYA"
		    SF-IMAGE "Image" 0
		    SF-DRAWABLE "Drawable" 0
		    SF-VALUE "Intermediate Frames" "3"
		    SF-VALUE "Max. Blur Radius" "0"
		    SF-TOGGLE "Looped" TRUE)
























