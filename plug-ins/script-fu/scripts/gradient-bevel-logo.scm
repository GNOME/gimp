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

(define (script-fu-gradient-bevel-logo text size font bevel-height bevel-width)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
         (border (/ size 4))
	 (text-layer (car (gimp-text img -1 0 0 text border TRUE size PIXELS "*" font "*" "*" "*" "*")))
	 (width (car (gimp-drawable-width text-layer)))
	 (height (car (gimp-drawable-height text-layer)))
         (indentX (+ border 12))
	 (indentY (+ border (/ height 8)))
	 (bg-layer (car (gimp-layer-new img width height RGBA_IMAGE "Background" 100 NORMAL)))
	 (blur-layer (car (gimp-layer-new img width height RGBA_IMAGE "Blur" 100 NORMAL)))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background))))
    (gimp-image-disable-undo img)
    (gimp-image-resize img width height 0 0)
    (gimp-image-add-layer img bg-layer 1)
    (gimp-image-add-layer img blur-layer 1)

    (gimp-selection-all img bg-layer)
    (gimp-edit-fill img bg-layer)
    (gimp-selection-none img)

    (gimp-layer-set-preserve-trans blur-layer TRUE)
    (gimp-palette-set-background '(255 255 255))
    (gimp-selection-all img blur-layer)
    (gimp-edit-fill img blur-layer)
    (gimp-edit-clear img blur-layer)
    (gimp-selection-none img)
    (gimp-layer-set-preserve-trans blur-layer FALSE)
    (gimp-selection-layer-alpha img text-layer)
    (gimp-edit-fill img blur-layer)
    (plug-in-gauss-rle 1 img blur-layer bevel-width 1 1)
    (gimp-selection-none img)
    (gimp-palette-set-background '(127 127 127))
    (gimp-palette-set-foreground '(255 255 255))
    (gimp-layer-set-preserve-trans text-layer TRUE)
    (gimp-selection-all img text-layer)
    (gimp-blend img text-layer FG-BG-RGB NORMAL RADIAL 95 0 REPEAT-NONE FALSE 0 0 indentX indentY indentX (- height indentY))
    (gimp-selection-none img)
    (gimp-layer-set-preserve-trans text-layer FALSE)
    (gimp-layer-set-name text-layer text)
    (plug-in-bump-map 1 img text-layer blur-layer 115 bevel-height 5 0 0 0 15 TRUE FALSE 0)
    (gimp-layer-set-offsets blur-layer 5 5)
    (gimp-invert img blur-layer)
    (gimp-layer-set-opacity blur-layer 50.0)
    (gimp-image-set-active-layer img text-layer)
    (gimp-palette-set-background old-bg)
    (gimp-palette-set-foreground old-fg)
    (gimp-image-enable-undo img)
    (gimp-display-new img)))


(script-fu-register "script-fu-gradient-bevel-logo"
		    "<Toolbox>/Xtns/Script-Fu/Logos/Gradient Bevel"
		    "Makes Shiny Bevelly text"
		    "Brian McFee <keebler@wco.com>"
		    "Brian McFee"
		    "April 1998"
		    ""
		    SF-STRING "Text String" "Moo"
		    SF-VALUE "Font Size (in pixels)" "90"
		    SF-STRING "Font" "futura_poster"
		    SF-VALUE "Bevel Height (sharpness)" "40"
		    SF-VALUE "Bevel Width" "2.5")
