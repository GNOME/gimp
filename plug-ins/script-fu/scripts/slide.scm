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
; slide.scm   version 0.30   12/13/97
;
; CHANGE-LOG:
; 0.20 - first public release
; 0.30 - some code cleanup
;        now uses the rotate plug-in to improve speed
;
; !still in development!
; TODO: - change the script so that the film is rotated, not the image
;       - antialiasing
;       - make 'add background' an option
;       - ?
;
; Copyright (C) 1997 Sven Neumann (neumanns@uni-duesseldorf.de) 
;  
; makes your picture look like a slide
;
; The script works on RGB and grayscale images that contain only 
; one layer. The image is cropped to fit into an aspect ratio of 1:1,5.
; It creates a copy of the image or can optionally work on the original. 
; The script uses the current background color to create a background 
; layer.


(define (crop width height ratio)
  (cond ((>= width (* ratio height)) (* ratio height))
	((< width (* ratio height))   width)))


(define (script-fu-slide img 
			 drawable
			 text
			 number
			 fontname
			 font-color
			 work-on-copy)
  (let* ((type (car (gimp-drawable-type-with-alpha drawable)))
	 (image (cond ((= work-on-copy TRUE) 
		       (car (gimp-channel-ops-duplicate img)))
		      ((= work-on-copy FALSE) 
		       img)))
	 (owidth (car (gimp-image-width image)))
	 (oheight (car (gimp-image-height image)))
	 (ratio (if (>= owidth oheight) (/ 3 2) 
		                        (/ 2 3)))
	 (crop-width (crop owidth oheight ratio)) 
	 (crop-height (/ crop-width ratio))
	 (width (* (max crop-width crop-height) 1.05))
	 (height (* (min crop-width crop-height) 1.5))
	 (hole-width (/ width 20))
	 (hole-space (/ width 8))
	 (hole-height (/ width 12))
	 (hole-radius (/ hole-width 4))
	 (hole-start (- (/ (rand 1000) 1000) 0.5))
	 (old-bg (car (gimp-palette-get-background)))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (film-layer (car (gimp-layer-new image 
					  width 
					  height 
					  type 
					  "Film" 
					  100 
					  NORMAL)))
	 (bg-layer (car (gimp-layer-new image 
					width 
					height 
					type 
					"Background" 
					100 
					NORMAL)))
	 (pic-layer (car (gimp-image-active-drawable image)))
	 (numbera (string-append number "A")))


  (gimp-image-disable-undo image)

; add an alpha channel to the image
  (gimp-layer-add-alpha pic-layer)

; crop, resize and eventually rotate the image 
  (gimp-crop image 
	     crop-width 
	     crop-height 
	     (/ (- owidth crop-width) 2)
	     (/ (- oheight crop-height) 2))
  (gimp-image-resize image 
		     width 
		     height 
		     (/ (- width crop-width) 2)
		     (/ (- height crop-height) 2))
  (if (< ratio 1) (plug-in-rotate 1
				  image 
				  pic-layer
				  1
				  FALSE)) 

; add the background layer
  (gimp-drawable-fill bg-layer BG-IMAGE-FILL)
  (gimp-image-add-layer image bg-layer -1)

; add the film layer
  (gimp-palette-set-background '(0 0 0))
  (gimp-drawable-fill film-layer BG-IMAGE-FILL)

; add the text
  (gimp-palette-set-foreground font-color)
  (gimp-floating-sel-anchor (car (gimp-text-fontname image
					    film-layer
					    (+ hole-start (* -0.25 width))
					    (* 0.01 height)
					    text
					    0
					    TRUE
					    (* 0.040 height) PIXELS fontname)))
  (gimp-floating-sel-anchor (car (gimp-text-fontname image
					    film-layer
					    (+ hole-start (* 0.75 width))
					    (* 0.01 height)
					    text
					    0
					    TRUE
					    (* 0.040 height) PIXELS
					    fontname )))
  (gimp-floating-sel-anchor (car (gimp-text-fontname image
					    film-layer		  
					    (+ hole-start (* 0.35 width))
					    (* 0.01 height)
					    number
					    0
					    TRUE
					    (* 0.050 height) PIXELS
					    fontname )))
  (gimp-floating-sel-anchor (car (gimp-text-fontname image
					    film-layer		  
					    (+ hole-start (* 0.35 width))
					    (* 0.95 height)
					    number
					    0
					    TRUE
					    (* 0.050 height) PIXELS
					    fontname )))
  (gimp-floating-sel-anchor (car (gimp-text-fontname image
					      film-layer		  
					      (+ hole-start (* 0.85 width))
					      (* 0.95 height)
					      numbera
					      0
					      TRUE
					      (* 0.045 height) PIXELS
					      fontname )))

; create a mask for the holes and cut them out
  (let* ((film-mask (car (gimp-layer-create-mask film-layer WHITE-MASK)))
	 (hole hole-start)
	 (top-y (* height 0.06))
	 (bottom-y(* height 0.855))) 
    (gimp-selection-none image)
    (while (< hole 8)
	   (gimp-rect-select image 
			     (* hole-space hole)
			     top-y
			     hole-width
			     hole-height
			     ADD
			     FALSE
			     0)	 
	   (gimp-rect-select image 
			     (* hole-space hole)
			     bottom-y
			     hole-width
			     hole-height
			     ADD
			     FALSE
			     0)
	   (set! hole (+ hole 1)))

    (gimp-palette-set-foreground '(0 0 0))
    (gimp-edit-fill image film-mask)
    (gimp-selection-none image)
    (plug-in-gauss-rle 1 image film-mask hole-radius TRUE TRUE)
    (gimp-threshold image film-mask 127 255)

    (gimp-image-add-layer image film-layer -1)
    (gimp-image-add-layer-mask image film-layer film-mask))

; reorder the layers
  (gimp-image-raise-layer image pic-layer)
  (gimp-image-raise-layer image pic-layer)

; clean up after the script
  (gimp-selection-none image)
  (gimp-palette-set-background old-bg)
  (gimp-palette-set-foreground old-fg)
  (gimp-image-enable-undo image)
  (if (= work-on-copy TRUE) (gimp-display-new image))
  (gimp-displays-flush)))

(script-fu-register "script-fu-slide" 
		    "<Image>/Script-Fu/Decor/Slide"
		    "Gives the image the look of a slide"
		    "Sven Neumann (neumanns@uni-duesseldorf.de)"
		    "Sven Neumann"
		    "12/13/1997"
		    "RGB GRAY"
		    SF-IMAGE "Image" 0
		    SF-DRAWABLE "Drawable" 0
		    SF-STRING "Text" "The GIMP"
		    SF-STRING "Number" "32"
		    SF-FONT "Font" "-*-utopia-*-r-*-*-24-*-*-*-p-*-*-*"
		    SF-COLOR "Font Color" '(255 180 0)
		    SF-TOGGLE "Work on copy" TRUE) 

























