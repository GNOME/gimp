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
; "Copy Visible"  version 0.05 10/19/97
;     by Adrian Likins <adrian@gimp.org>
;   _heavily_ based on:
;        cyn-merge.scm   version 0.02   10/10/97
;        Copyright (C) 1997 Sven Neumann (neumanns@uni-duesseldorf.de)
; 

(define (script-fu-copy-visible image
			     drawable)
  (let* ((layers (gimp-image-get-layers image))
	 (num-layers (car layers))
	 (layer-array (cadr layers)))
  
  (gimp-image-disable-undo image)
	
  ; copy all visible layers and make them invisible
  (set! layer-count 1)
  (set! visi-array (cons-array num-layers))
  (while (<= layer-count num-layers)
	 (set! layer (aref layer-array (- num-layers layer-count)))
	 (aset visi-array (- num-layers layer-count) 
	                  (car (gimp-layer-get-visible layer)))
	 (if (= TRUE (car (gimp-layer-get-visible layer)))
	     (begin
	       (set! copy (car (gimp-layer-copy layer TRUE)))
	       (gimp-image-add-layer image copy -1)
	       (gimp-layer-set-visible copy TRUE)
	       (gimp-layer-set-visible layer FALSE)))
	 (set! layer-count (+ layer-count 1)))
  
  ; merge all visible layers
  ;changed
  (set! merged-layer (car (gimp-image-merge-visible-layers image EXPAND-AS-NECESSARY)))
  
  ; restore the layers visibilty
  (set! layer-count 0)
  (while (< layer-count num-layers)
	 (set! layer (aref layer-array layer-count))
	 (gimp-layer-set-visible layer (aref visi-array layer-count))
	 (set! layer-count (+ layer-count 1)))
  ;changed
  (gimp-edit-copy image merged-layer)
  (gimp-image-set-active-layer image drawable)
  (gimp-image-remove-layer image merged-layer)

  (gimp-image-enable-undo image)
  (gimp-displays-flush)))

(script-fu-register "script-fu-copy-visible" 
;  I use the script under the edit menu, but probabaly bad style for a dist version.
;		    "<Image>/Edit/Copy Visible"
		    "<Image>/Script-Fu/Selection/Copy Visible"
		    "Copy the visible selction"
		    "Sven Neumann (neumanns@uni-duesseldorf.de), Adrian Likins <adrian@gimp.org>"
		    "Sven Neumann, Adrian Likins"
		    "10/19/1997"
		    "RGB* INDEXED* GRAY*"
		    SF-IMAGE "Image" 0
		    SF-DRAWABLE "Drawable" 0)
























