


; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
; 
;  Selection-to-brush 
; Copyright (c) 1997 Adrian Likins
; aklikins@eos.ncsu.edu
;
; Takes the current selection, saves it as a brush, and makes it the active brush..
;
; NOTE: You need to change the home dir to the approriate place.
;       Also, in .13 there are bugs in the gbr plugin that might make it sigsegv
;
;       Parts of this script from Sven Neuman's Drop-Shadow and 
;       Seth Burgess's mkbrush scripts.
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


(define (script-fu-selection-to-brush image drawable desc filename spacing)
  (let* (
	 (type (car (gimp-drawable-type-with-alpha drawable)))
	 (old-bg (car (gimp-palette-get-background))))
  
    (set! selection-bounds (gimp-selection-bounds image))
    (set! select-offset-x (cadr selection-bounds))
    (set! select-offset-y (caddr selection-bounds))
    (set! selection-width (- (cadr (cddr selection-bounds)) select-offset-x))
    (set! selection-height (- (caddr (cddr selection-bounds)) select-offset-y))

    (gimp-image-disable-undo image)
    
    (if (= (car (gimp-selection-is-empty image)) TRUE)
	(begin
	  (gimp-selection-layer-alpha drawable)
	  (set! active-selection (car (gimp-selection-save image)))
	  (set! from-selection FALSE))
	(begin
	  (set! from-selection TRUE)
	  (set! active-selection (car (gimp-selection-save image)))))

    (gimp-edit-copy drawable)

    (set! brush-image (car (gimp-image-new selection-width selection-height 1)))
    (set! brush-draw (car (gimp-layer-new brush-image selection-width selection-height GRAY_IMAGE "Sloth" 100 NORMAL)))
    (gimp-image-add-layer brush-image brush-draw 0)
    
    (gimp-selection-none brush-image)
    (gimp-palette-set-background '(255 255 255))
    (gimp-drawable-fill brush-draw BG-IMAGE-FILL)

    (let ((floating-sel (car (gimp-edit-paste brush-draw FALSE))))
      (gimp-floating-sel-anchor floating-sel)
      )

    (set! data-dir (car (gimp-gimprc-query "gimp_dir")))
    (set! filename2 (string-append data-dir
					 "/brushes/"
					 filename
					 (number->string image)
					 ".gbr"))

    (gimp-invert brush-draw)
    (file-gbr-save 1 brush-image brush-draw filename2 "" spacing desc)
    (gimp-brushes-refresh) ; My own modification!  You'll need my diff.
    (gimp-brushes-set-brush desc)
    
    (gimp-palette-set-background old-bg)

    ;(set! image_test (gimp-procedural-db-get-data image))
    (print (number->string image))
    (print "this is a test.......test..")
    (gimp-image-enable-undo image)
    (gimp-image-set-active-layer image drawable)
    (gimp-image-delete brush-image)
    (gimp-displays-flush)))

(script-fu-register "script-fu-selection-to-brush"
 ; I like this script under the main selection menu, but that doesnt seem quite kosher
;		    "<Image>/Select/Selection To Brush"
		    "<Image>/Script-Fu/Selection/To Brush"
		    "Convert a selection to a brush"
		    "Adrian Likins <adrian@gimp.org>"
		    "Adrian Likins"
		    "10/07/97"
		    "RGB RGBA GRAY GRAYA"
		    SF-IMAGE "Image" 0
		    SF-DRAWABLE "Drawable" 0
;		    SF-STRING "Home directory" "/home/aklikins/"
		    SF-STRING "Description" "Brush"
		    SF-STRING "Filename" "SlothBrush"
;		    SF-VALUE "Spacing" "25")
		    SF-ADJUSTMENT "Spacing" '(25 0 1000 1 1 1 0))


