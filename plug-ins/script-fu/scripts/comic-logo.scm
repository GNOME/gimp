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
;  Comic Book Logo v0.1  04/08/98
;  by Brian McFee
;  Creates snazzy-looking text, inspired by watching a Maxx marathon :)

(define (apply-comic-logo-effect img
				 logo-layer
				 gradient
				 ol-width
				 ol-color
				 bg-color)
  (let* ((width (car (gimp-drawable-width logo-layer)))
	 (height (car (gimp-drawable-height logo-layer)))
	 (bg-layer (car (gimp-layer-new img width height RGBA_IMAGE "Background" 100 NORMAL)))
	 (white-layer (car (gimp-layer-copy logo-layer 1)))
	 (black-layer (car (gimp-layer-copy logo-layer 1)))
	 (old-gradient (car (gimp-gradients-get-active)))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background))))
    (gimp-image-resize img width height 0 0)
    (gimp-image-add-layer img bg-layer 1)
    (gimp-image-add-layer img white-layer 1)
    (gimp-layer-set-name white-layer "White")
    (gimp-image-add-layer img black-layer 1)
    (gimp-layer-set-name black-layer "Black")
  
    (gimp-selection-all img)
    (gimp-palette-set-background bg-color)
    (gimp-edit-fill bg-layer BG-IMAGE-FILL)
    (gimp-selection-none img)

    (gimp-layer-set-preserve-trans white-layer TRUE)
    (gimp-palette-set-background ol-color)
    (gimp-selection-all img)
    (gimp-edit-fill white-layer BG-IMAGE-FILL)
    (gimp-layer-set-preserve-trans white-layer FALSE)
    (plug-in-spread 1 img white-layer (* 3 ol-width) (* 3 ol-width))
    (plug-in-gauss-rle 1 img white-layer (* 2 ol-width) 1 1)
    (plug-in-threshold-alpha 1 img white-layer 0)
    (gimp-selection-none img)

    (gimp-palette-set-background '(0 0 0))
    (gimp-layer-set-preserve-trans black-layer TRUE)
    (gimp-selection-all img)
    (gimp-edit-fill black-layer BG-IMAGE-FILL)
    (gimp-selection-none img)
    (gimp-layer-set-preserve-trans black-layer FALSE)
    (plug-in-gauss-rle 1 img black-layer ol-width 1 1)
    (plug-in-threshold-alpha 1 img black-layer 0)

    (gimp-gradients-set-active gradient)
    (gimp-layer-set-preserve-trans logo-layer TRUE)
    (gimp-selection-all img)
    (gimp-blend logo-layer CUSTOM NORMAL LINEAR 100 0 REPEAT-NONE FALSE 0 0 0 (* height 0.33333) 0 (* height 0.83333))
    (plug-in-noisify 1 img logo-layer 0 0.20 0.20 0.20 0.20)
    (gimp-selection-none img)
    (gimp-layer-set-preserve-trans logo-layer FALSE)
    (gimp-brightness-contrast logo-layer 0 30)
    (plug-in-threshold-alpha 1 img logo-layer 60)
    (gimp-image-set-active-layer img logo-layer)
    (gimp-gradients-set-active old-gradient)
    (gimp-palette-set-background old-bg)
    (gimp-palette-set-foreground old-fg)))

(define (script-fu-comic-logo-alpha img
				    logo-layer
				    gradient
				    ol-width
				    ol-color
				    bg-color)
  (begin
    (gimp-undo-push-group-start img)
    (apply-comic-logo-effect img logo-layer gradient ol-width ol-color
			     bg-color)
    (gimp-undo-push-group-end img)
    (gimp-displays-flush)))

(script-fu-register "script-fu-comic-logo-alpha"
		    _"<Image>/Script-Fu/Alpha to Logo/Comic Book..."
		    "Comic-book Style Logos"
		    "Brian McFee <keebler@wco.com>"
		    "Brian McFee"
		    "April 1998"
		    "RGBA"
                    SF-IMAGE      "Image" 0
                    SF-DRAWABLE   "Drawable" 0
		    SF-GRADIENT   _"Gradient" "Incandescent"
		    SF-ADJUSTMENT _"Outline Size" '(5 1 100 1 10 0 1)
		    SF-COLOR      _"Outline Color" '(255 255 255)
		    SF-COLOR      _"Background Color" '(255 255 255)
		    )


(define (script-fu-comic-logo text
			      size
			      font
			      gradient
			      ol-width
			      ol-color
			      bg-color)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
         (border (/ size 4))
	 (text-layer (car (gimp-text-fontname img -1 0 0 text border TRUE size PIXELS font))))
    (gimp-image-undo-disable img)
    (gimp-layer-set-name text-layer text)
    (apply-comic-logo-effect img text-layer gradient ol-width ol-color
			     bg-color)
    (gimp-image-undo-enable img)
    (gimp-display-new img)))

(script-fu-register "script-fu-comic-logo"
		    _"<Toolbox>/Xtns/Script-Fu/Logos/Comic Book..."
		    "Comic-book Style Logos"
		    "Brian McFee <keebler@wco.com>"
		    "Brian McFee"
		    "April 1998"
		    ""
		    SF-STRING     _"Text" "Moo"
		    SF-ADJUSTMENT _"Font Size (pixels)" '(85 2 1000 1 10 0 1)
		    SF-FONT       _"Font" "-*-tribeca-*-i-*-*-24-*-*-*-p-*-*-*"
		    SF-GRADIENT   _"Gradient" "Incandescent"
		    SF-ADJUSTMENT _"Outline Size" '(5 1 100 1 10 0 1)
		    SF-COLOR      _"Outline Color" '(255 255 255)
		    SF-COLOR      _"Background Color" '(255 255 255)
		    )
