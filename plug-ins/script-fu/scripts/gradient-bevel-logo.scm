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
;  Gradient Bevel v0.1  04/08/98
;  by Brian McFee <keebler@wco.com>
;  Create cool glossy bevelly text

(define (apply-gradient-bevel-logo-effect img
					  logo-layer
					  b-size
					  bevel-height
					  bevel-width
					  bg-color)
  (let* ((width (car (gimp-drawable-width logo-layer)))
	 (height (car (gimp-drawable-height logo-layer)))
         (indentX (+ b-size 12))
	 (indentY (+ b-size (/ height 8)))
	 (bg-layer (car (gimp-layer-new img width height RGBA_IMAGE "Background" 100 NORMAL)))
	 (blur-layer (car (gimp-layer-new img width height RGBA_IMAGE "Blur" 100 NORMAL)))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background))))
    (gimp-image-resize img width height 0 0)
    (gimp-image-add-layer img bg-layer 1)
    (gimp-image-add-layer img blur-layer 1)

    (gimp-selection-all img)
    (gimp-palette-set-background bg-color)
    (gimp-edit-fill bg-layer BG-IMAGE-FILL)
    (gimp-selection-none img)

    (gimp-layer-set-preserve-trans blur-layer TRUE)
    (gimp-palette-set-background '(255 255 255))
    (gimp-selection-all img)
    (gimp-edit-fill blur-layer BG-IMAGE-FILL)
    (gimp-edit-clear blur-layer)
    (gimp-selection-none img)
    (gimp-layer-set-preserve-trans blur-layer FALSE)
    (gimp-selection-layer-alpha logo-layer)
    (gimp-edit-fill blur-layer BG-IMAGE-FILL)
    (plug-in-gauss-rle 1 img blur-layer bevel-width 1 1)
    (gimp-selection-none img)
    (gimp-palette-set-background '(127 127 127))
    (gimp-palette-set-foreground '(255 255 255))
    (gimp-layer-set-preserve-trans logo-layer TRUE)
    (gimp-selection-all img)
    (gimp-blend logo-layer FG-BG-RGB NORMAL RADIAL 95 0 REPEAT-NONE FALSE 0 0 indentX indentY indentX (- height indentY))
    (gimp-selection-none img)
    (gimp-layer-set-preserve-trans logo-layer FALSE)
    (plug-in-bump-map 1 img logo-layer blur-layer 115 bevel-height 5 0 0 0 15 TRUE FALSE 0)
    (gimp-layer-set-offsets blur-layer 5 5)
    (gimp-invert blur-layer)
    (gimp-layer-set-opacity blur-layer 50.0)
    (gimp-image-set-active-layer img logo-layer)
    (gimp-palette-set-background old-bg)
    (gimp-palette-set-foreground old-fg)))

(define (script-fu-gradient-bevel-logo-alpha img
					     logo-layer
					     b-size
					     bevel-height
					     bevel-width
					     bg-color)
  (begin
    (gimp-undo-push-group-start img)
    (apply-gradient-bevel-logo-effect img logo-layer b-size
				      bevel-height bevel-width bg-color)
    (gimp-undo-push-group-end img)
    (gimp-displays-flush)))

(script-fu-register "script-fu-gradient-bevel-logo-alpha"
		    _"<Image>/Script-Fu/Alpha to Logo/Gradient Bevel..."
		    "Makes Shiny Bevelly text"
		    "Brian McFee <keebler@wco.com>"
		    "Brian McFee"
		    "April 1998"
		    "RGBA"
                    SF-IMAGE      "Image" 0
                    SF-DRAWABLE   "Drawable" 0
		    SF-ADJUSTMENT _"Border Size (pixels)" '(22 1 300 1 10 0 1)
		    SF-ADJUSTMENT _"Bevel Height (Sharpness)" '(40 1 250 1 10 0 1)
		    SF-ADJUSTMENT _"Bevel Width" '(2.5 1 200 1 10 1 1)
		    SF-COLOR      _"Background Color" '(255 255 255)
		    )

(define (script-fu-gradient-bevel-logo text
				       size
				       font
				       bevel-height
				       bevel-width
				       bg-color)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
         (border (/ size 4))
	 (text-layer (car (gimp-text-fontname img -1 0 0 text border TRUE size PIXELS font))))
    (gimp-image-undo-disable img)
    (gimp-layer-set-name text-layer text)
    (apply-gradient-bevel-logo-effect img text-layer border
				      bevel-height bevel-width bg-color)
    (gimp-image-undo-enable img)
    (gimp-display-new img)))

(script-fu-register "script-fu-gradient-bevel-logo"
		    _"<Toolbox>/Xtns/Script-Fu/Logos/Gradient Bevel..."
		    "Makes Shiny Bevelly text"
		    "Brian McFee <keebler@wco.com>"
		    "Brian McFee"
		    "April 1998"
		    ""
		    SF-STRING     _"Text" "Moo"
		    SF-ADJUSTMENT _"Font Size (pixels)" '(90 2 1000 1 10 0 1)
		    SF-FONT       _"Font" "-*-futura_poster-*-r-*-*-24-*-*-*-p-*-*-*"
		    SF-ADJUSTMENT _"Bevel Height (Sharpness)" '(40 1 250 1 10 0 1)
		    SF-ADJUSTMENT _"Bevel Width" '(2.5 1 200 1 10 1 1)
		    SF-COLOR      _"Background Color" '(255 255 255)
		    )
