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
; round-corners.scm   version 1.01   12/13/97
;
; CHANGE-LOG:
; 1.00 - initial release
; 1.01 - some code cleanup, no real changes
;
; Copyright (C) 1997 Sven Neumann (neumanns@uni-duesseldorf.de) 
;  
;
; Rounds the corners of an image, optionally adding a drop-shadow and
; a background layer
;
; The script works on RGB and grayscale images that contain only 
; one layer. It creates a copy of the image or can optionally work 
; on the original. The script uses the current background color to 
; create a background layer. It makes a call to the script drop-shadow.
;
; This script is derived from my script add-shadow, which has become 
; obsolet now.



(define (script-fu-round-corners img
				 drawable
				 radius
				 shadow-toggle
				 shadow-x
				 shadow-y
				 shadow-blur
				 background-toggle
				 work-on-copy)
  (let* ((shadow-blur (abs shadow-blur))
	 (radius (abs radius))
	 (diam (* 2 radius))
	 (width (car (gimp-image-width img)))
	 (height (car (gimp-image-height img)))
	 (type (car (gimp-drawable-type-with-alpha drawable)))
	 (image (cond ((= work-on-copy TRUE)
		       (car (gimp-channel-ops-duplicate img)))
		      ((= work-on-copy FALSE)
		       img)))
	 (pic-layer (car (gimp-image-active-drawable image))))

  (gimp-image-undo-disable image)

  ; add an alpha channel to the image
  (gimp-layer-add-alpha pic-layer)
  
  ; round the edges  
  (gimp-selection-none image)
  (gimp-rect-select image 0 0 radius radius ADD 0 0)
  (gimp-ellipse-select image 0 0 diam diam SUB TRUE 0 0)
  (gimp-rect-select image (- width radius) 0 radius radius ADD 0 0)
  (gimp-ellipse-select image (- width diam) 0 diam diam SUB TRUE 0 0)
  (gimp-rect-select image 0 (- height radius) radius radius ADD 0 0)
  (gimp-ellipse-select image 0 (- height diam) diam diam SUB TRUE 0 0)
  (gimp-rect-select image (- width radius) (- height radius)
		    radius radius ADD 0 0)
  (gimp-ellipse-select image (- width diam) (- height diam)
		       diam diam SUB TRUE 0 0)
  (gimp-edit-clear pic-layer)
  (gimp-selection-none image)
  
  ; optionally add a shadow
  (if (= shadow-toggle TRUE)
      (begin
	(script-fu-drop-shadow image
			       pic-layer
			       shadow-x
			       shadow-y
			       shadow-blur
			       '(0 0 0)
			       80
			       TRUE)
	(set! width (car (gimp-image-width image)))
	(set! height (car (gimp-image-height image)))))
      
  ; optionally add a background
  (if (= background-toggle TRUE)
      (let* ((bg-layer (car (gimp-layer-new image
					    width
					    height
					    type
					    "Background"
					    100
					    NORMAL))))
	(gimp-drawable-fill bg-layer BG-IMAGE-FILL)
	(gimp-image-add-layer image bg-layer -1)
	(gimp-image-raise-layer image pic-layer)
	(if (= shadow-toggle TRUE)
	    (gimp-image-lower-layer image bg-layer))))

; clean up after the script
  (gimp-image-undo-enable image)
  (if (= work-on-copy TRUE)
      (gimp-display-new image))
  (gimp-displays-flush)))

(script-fu-register "script-fu-round-corners"
		    "<Image>/Script-Fu/Decor/Round Corners..."
		    "Round the corners of an image and optionally adds a drop-shadow and a background"
		    "Sven Neumann (neumanns@uni-duesseldorf.de)"
		    "Sven Neumann"
		    "12/13/1997"
		    "RGB GRAY"
		    SF-IMAGE "Image" 0
		    SF-DRAWABLE "Drawable" 0
		    SF-VALUE "Radius of Edges" "12"
		    SF-TOGGLE "Add drop-shadow" TRUE
		    SF-VALUE "Shadow x" "8"
		    SF-VALUE "Shadow y" "8"
		    SF-VALUE "Blur Radius" "15"
		    SF-TOGGLE "Add background" TRUE
		    SF-TOGGLE "Work on copy" TRUE)

























