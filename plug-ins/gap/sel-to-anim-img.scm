; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
; 
; Selection to AnimationImage
; This script requires plug-in-gap-layers-run-animfilter (that is part of
; the gap Plugin-collection, available in the plugin registry)
;
; - 26.5.1999 hof: adapted for GIMP 1.1 Plug-in Interfaces
; - Takes the Current selection and saves it n-times (as multiple layers) in one seperate image.
;    (base functions taken from Selection to Image ((c) by Adrian Likins adrian@gimp.org)
; - added number of copies and
; - optional { BG-FILL | TRANS-FILL }
; - if there was no selection at begin the selection is cleared at end 
; - optional call of plug-in-gap-layers-run-animfilter
;   (to create animation effects by applying any available PDB filter foreach copy
;    (==layer) of the generated image
;    with constant or varying values)
; - if we just copied some layers set the generated brush-image clean at end
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


(define (script-fu-selection-to-anim-image image drawable copies bgfill filter-all)
  (let* (
         (idx 0)
	 (draw-type (car (gimp-drawable-type-with-alpha drawable)))
	 (image-type (car (gimp-image-base-type image)))
	 (old-bg (car (gimp-palette-get-background))))

    (set! selection-bounds (gimp-selection-bounds image))
    (set! select-offset-x (cadr selection-bounds))
    (set! select-offset-y (caddr selection-bounds))
    (set! selection-width (- (cadr (cddr selection-bounds)) select-offset-x))
    (set! selection-height (- (caddr (cddr selection-bounds)) select-offset-y)) 

    (gimp-image-undo-disable image)

    (if (= (car (gimp-selection-is-empty image)) TRUE)
	(begin
          ; if the layer has no alpha select all
          (if (= (car (gimp-drawable-has-alpha drawable)) FALSE)
             (begin
                (gimp-selection-all image)
             )
             (begin
                (gimp-selection-layer-alpha drawable)
             )
          )
	  (set! active-selection (car (gimp-selection-save image)))
	  (set! from-selection FALSE)
	)
	(begin 
	  (set! active-selection (car (gimp-selection-save image)))
	  (set! from-selection TRUE)
	)
    )

    (gimp-edit-copy drawable)

    (set! brush-image (car (gimp-image-new selection-width selection-height image-type)))

    (while (< idx copies)
       (set! draw-name (string-append "frame_0" (number->string idx)))
       (set! brush-draw (car (gimp-layer-new brush-image selection-width selection-height draw-type draw-name 100 NORMAL)))
       (gimp-image-add-layer brush-image brush-draw 0)
       (if (= bgfill TRUE)
           (gimp-drawable-fill brush-draw BG-IMAGE-FILL)
           (gimp-drawable-fill brush-draw TRANS-IMAGE-FILL)
       )
       (let ((floating-sel (car (gimp-edit-paste brush-draw FALSE))))
         (gimp-floating-sel-anchor floating-sel)
       )
       (set! idx (+ idx 1))
    )

    (if (= from-selection FALSE)
        (gimp-selection-none image)
    )

    (gimp-palette-set-background old-bg)
    (gimp-image-undo-enable image)
    (gimp-image-set-active-layer image drawable)
    (gimp-image-clean-all brush-image)
    (gimp-display-new brush-image)
    (gimp-displays-flush))
    (if (= filter-all TRUE)
        ; INTERACTIVE animated call of any other plugin
        ; (drawable and plugin name are dummy parameters
        ; the plug-in to run is selcted by the built in PDB-browserdialog
        ; of plug-in-gap-layers-run-animfilter)
        (plug-in-gap-layers-run-animfilter 0 brush-image brush-draw "plug-in-bend")
    )
)

(script-fu-register "script-fu-selection-to-anim-image"
		    _"<Image>/Script-Fu/Animators/Selection to AnimImage..."
		    "Create a multilayer image from current selection and apply any PDB Filter to all layer-copies"
		    "Wolfgang Hofer <hof@hotbot.com>"
		    "Wolfgang Hofer"
		    "20/05/98"
		    "RGB RGBA GRAY GRAYA"
		    SF-IMAGE "Image" 0
		    SF-DRAWABLE "Drawable" 0
                    SF-ADJUSTMENT _"Number of Copies"           '(10 1 1024 1 10 0 1)
                    SF-TOGGLE     _"Fill with BG Color"         TRUE
                    SF-TOGGLE     _"Anim-Filter for all Copies" TRUE)



