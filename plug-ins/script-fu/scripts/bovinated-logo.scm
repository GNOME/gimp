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
;  Bovinated Logos v0.1 04/08/98
;  by Brian McFee <keebler@wco.com>
;  Creates Cow-spotted logs.. what else?

(define (script-fu-bovinated-logo text size font)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
         (border (/ size 4))
	 (text-layer (car (gimp-text-fontname img -1 0 0 text border TRUE size PIXELS font)))
	 (width (car (gimp-drawable-width text-layer)))
	 (height (car (gimp-drawable-height text-layer)))
	 (bg-layer (car (gimp-layer-new img width height RGBA_IMAGE "Background" 100 NORMAL)))
	 (blur-layer (car (gimp-layer-new img width height RGBA_IMAGE "Blur" 100 NORMAL)))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background))))
    (gimp-image-disable-undo img)
    (gimp-image-resize img width height 0 0)
    (gimp-image-add-layer img bg-layer 1)
    (gimp-image-add-layer img blur-layer 1)

    (gimp-selection-all img)
    (gimp-edit-fill bg-layer)
    (gimp-selection-none img)

    (gimp-layer-set-preserve-trans blur-layer TRUE)
    (gimp-palette-set-background '(255 255 255))
    (gimp-selection-all img)
    (gimp-edit-fill blur-layer)
    (gimp-edit-clear blur-layer)
    (gimp-palette-set-background '(191 191 191))
    (gimp-selection-none img)
    (gimp-layer-set-preserve-trans blur-layer FALSE)
    (gimp-selection-layer-alpha text-layer)
    (gimp-edit-fill blur-layer)
    (plug-in-gauss-rle 1 img blur-layer 5.0 1 1)
    (gimp-selection-none img)
    (gimp-layer-set-preserve-trans text-layer TRUE)
    (gimp-selection-all img)
    (plug-in-solid-noise 1 img text-layer 0 0 23 1 16.0 4.0)
    (gimp-brightness-contrast text-layer 0 127)
    (gimp-selection-none img)
    (gimp-layer-set-preserve-trans text-layer FALSE)
    (gimp-layer-set-name text-layer text)
    (plug-in-bump-map 1 img text-layer blur-layer 135 50 10 0 0 0 30 TRUE FALSE 0)
    (gimp-layer-set-offsets blur-layer 5 5)
    (gimp-invert blur-layer)
    (gimp-layer-set-opacity blur-layer 50.0)
    (gimp-image-set-active-layer img text-layer)
    (gimp-palette-set-background old-bg)
    (gimp-palette-set-foreground old-fg)
    (gimp-image-enable-undo img)
    (gimp-display-new img)))


(script-fu-register "script-fu-bovinated-logo"
		    "<Toolbox>/Xtns/Script-Fu/Logos/Bovination"
		    "Makes Cow-spotted logos"
		    "Brian McFee <keebler@wco.com>"
		    "Brian McFee"
		    "April 1998"
		    ""
		    SF-STRING "Text String" "Fear the Cow"
		    SF-ADJUSTMENT "Font Size (pixels)" '(80 2 1000 1 10 0 1)
		    SF-FONT "Font" "-*-roostheavy-*-r-*-*-24-*-*-*-p-*-*-*")
