; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
; 
; "Copy Visible" -- copy the visible selection so that it can be pasted easily
; Copyright (C) 2004 Raphaël Quinet, Adrian Likins, Sven Neumann
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
; 2004-04-14 This script was almost rewritten from scratch
;     by Raphaël Quinet <raphael@gimp.org>
;     see also http://bugzilla.gnome.org/show_bug.cgi?id=139989
;
; The code is new but the API is the same as in the previous version:
; "Copy Visible"  version 0.11 01/24/98
;     by Adrian Likins <adrian@gimp.org>
;   _heavily_ based on:
;        cyn-merge.scm   version 0.02   10/10/97
;        Copyright (C) 1997 Sven Neumann (neumanns@uni-duesseldorf.de)
;
; 

(define (script-fu-copy-visible orig-image
				drawable)
  (let* ((image (car (gimp-image-duplicate orig-image)))
	 (layers (gimp-image-get-layers image))
	 (num-layers (car layers))
	 (num-visible-layers 0)
	 (visible-layer -1)
	 (layer-array (cadr layers)))
  
    (gimp-image-undo-disable image)

    ; count the number of visible layers
    (while (> num-layers 0)
	   (let* ((layer (aref layer-array (- num-layers 1)))
		  (is-visible (car (gimp-drawable-get-visible layer))))
	     (if (= is-visible TRUE)
		 (begin
		   (set! num-visible-layers (+ num-visible-layers 1))
		   (set! visible-layer layer)))
	     (set! num-layers (- num-layers 1))))

    ; if there are several visible layers, merge them
    ; if there is only one layer and it has a layer mask, apply the mask
    (if (> num-visible-layers 1)
	(set! visible-layer (car (gimp-image-merge-visible-layers
				  image
				  EXPAND-AS-NECESSARY)))
        (if (= num-visible-layers 1)
            (if (not (= (car (gimp-layer-get-mask visible-layer)) -1))
                (car (gimp-layer-remove-mask visible-layer MASK-APPLY)))))

    ; copy the visible layer
    (if (> num-visible-layers 0)
	(gimp-edit-copy visible-layer))

    ; delete the temporary copy of the image
    (gimp-image-delete image)))

(script-fu-register "script-fu-copy-visible"
		    _"Copy _Visible"
		    "Copy the visible selection"
		    "Sven Neumann <sven@gimp.org>, Adrian Likins <adrian@gimp.org>, Raphael Quinet <raphael@gimp.org>"
		    "Sven Neumann, Adrian Likins, Raphael Quinet"
		    "01/24/1998"
		    "RGB* INDEXED* GRAY*"
		    SF-IMAGE    "Image"    0
		    SF-DRAWABLE "Drawable" 0)

(script-fu-menu-register "script-fu-copy-visible"
			 "<Image>/Edit/Copy")
