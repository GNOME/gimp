; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
;  Supposed to look vaguely like roughly carved wood. Chipped away if you will.
;
;  Options: Text String -  the string to make the logo from
;           Font        -  which font to use
;           Font Size   -  how big
;           Chip Amount - how rought he chipping is (how spread the bump map is)
;           Blur Amount - the bump layer is blurred slighty by this amount
;           Invert      - whether or not to invert the bumpmap (gives a carved in feel)
;           Drop Shadow - whether or not to draw a drop shadow
;           Keep bump layer? - whether to keep the layer used as the bump map
;           fill bg with pattern? - whether to fill the background with the pattern or leave it white
;           Keep Backgroun - whether or not to remove the background layer
; 
;  Adrian Likins  (Adrian@gimp.org)
;  Jan 11, 1998 v1
;
;  see http://www.gimp.org/~adrian/script.html
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
;  Some suggested patterns: Dried mud, 3D green, Slate
;

(define (script-fu-logo-chip-away text font font-size spread-amount blur-amount invert drop-shadow keep-bump bg-fill keep-back pattern)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
	 (text-layer (car (gimp-text-fontname img -1 0 0
				     text 30 TRUE font-size PIXELS font)))
	 (width (car (gimp-drawable-width text-layer)))
	 (height (car (gimp-drawable-height text-layer)))
	 (bg-layer (car (gimp-layer-new img width height RGB_IMAGE "Background" 100 NORMAL)))
	 (bump-layer (car (gimp-layer-new img width height RGBA_IMAGE "Bump Layer" 100 NORMAL)))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background))))
    
    (gimp-image-undo-disable img)
    (gimp-image-resize img width height 0 0)
    (gimp-image-add-layer img bg-layer 1)
    (gimp-layer-set-preserve-trans text-layer TRUE)
    (gimp-patterns-set-pattern pattern)

    (gimp-palette-set-background '(255 255 255))
    (gimp-selection-all img)

    (if (= bg-fill TRUE)
	(gimp-bucket-fill bg-layer 2 NORMAL 100 255 FALSE 1 1)
	(gimp-edit-fill bg-layer))

    (gimp-selection-all img)
    (gimp-edit-clear bump-layer)
    (gimp-selection-none img)
    (gimp-selection-layer-alpha text-layer)
    (gimp-edit-fill bump-layer)
    (gimp-bucket-fill text-layer 2 NORMAL 100 255 FALSE 1 1)
    (gimp-selection-none img)
    
    (gimp-image-add-layer img bump-layer 1)

    (gimp-layer-set-preserve-trans bump-layer FALSE)
    (plug-in-spread 1 img bump-layer spread-amount spread-amount)
    (gimp-selection-layer-alpha bump-layer)
    (plug-in-gauss-rle 1 img bump-layer blur-amount TRUE TRUE)
   
    (gimp-selection-none img)
    
    (plug-in-bump-map 1 img text-layer bump-layer 135.00 25.0 60 0 0 0 0 TRUE invert 1)

    (gimp-layer-set-visible bump-layer FALSE)

     (if (= drop-shadow TRUE)
	(begin
	  (let* ((shadow-layer (car (gimp-layer-new img width height RGBA_IMAGE "Shadow layer" 100 NORMAL))))
	    (gimp-selection-all img)
	    (gimp-edit-clear shadow-layer)
	    (gimp-selection-none img)
	    (gimp-selection-layer-alpha text-layer)
	    (gimp-palette-set-background '(0 0 0))
	    (gimp-edit-fill shadow-layer)
	    (gimp-selection-none img)
	    (plug-in-gauss-rle 1 img shadow-layer 5 TRUE TRUE)
	    (gimp-image-add-layer img shadow-layer 1)
	    (gimp-layer-translate shadow-layer 6 6))))

     (if (= keep-bump FALSE)
	 (gimp-image-remove-layer img bump-layer))

   

     (if (= keep-back FALSE)
	 (gimp-image-remove-layer img bg-layer))
    
    (gimp-palette-set-foreground old-fg)
    (gimp-palette-set-background old-bg)
    (gimp-image-undo-enable img)
    (gimp-display-new img)
    
    ))


(script-fu-register "script-fu-logo-chip-away"
		    "<Toolbox>/Xtns/Script-Fu/Logos/Chip Away..."
		    "Chip away effect"
		    "Adrian Likins <adrian@gimp.org>"
		    "Adrian Likins <adrian@gimp.org>"
		    "1997"
		    ""
		    SF-STRING "Text String" "Sloth"
		    SF-FONT "Font" "-*-roostheavy-*-r-*-*-24-*-*-*-p-*-*-*"
		    SF-ADJUSTMENT "Font Size (pixels)" '(200 2 1000 1 10 0 1)
		    SF-VALUE "Chip Amount" "30"
		    SF-VALUE "Blur Amount" "3"
		    SF-TOGGLE "Invert?" FALSE
		    SF-TOGGLE "Drop Shadow?" TRUE
		    SF-TOGGLE "Keep Bump layer?" FALSE
		    SF-TOGGLE "Fill bg with pattern?" TRUE
		    SF-TOGGLE "Keep Background?" TRUE
		    SF-PATTERN "Pattern" "Burlwood")


