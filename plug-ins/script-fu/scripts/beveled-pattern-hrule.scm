; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
; 
; Beveled pattern hrule for web pages
; Copyright (C) 1997 Federico Mena Quintero
; federico@nuclecu.unam.mx
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


(define (script-fu-beveled-pattern-hrule width height pattern)
  (let* ((old-bg-color (car (gimp-palette-get-background)))
	 (img (car (gimp-image-new width height RGB)))
	 (background (car (gimp-layer-new img width height RGB_IMAGE "Hrule" 100 NORMAL)))
	 (bumpmap (car (gimp-layer-new img width height RGBA_IMAGE "Bumpmap" 100 NORMAL))))
    (gimp-image-undo-disable img)
    (gimp-image-add-layer img background -1)
    (gimp-image-add-layer img bumpmap -1)

    ; Create pattern layer

    (gimp-palette-set-background '(0 0 0))
    (gimp-edit-fill background BG-IMAGE-FILL)
    (gimp-patterns-set-pattern pattern)
    (gimp-bucket-fill background PATTERN-BUCKET-FILL NORMAL 100 0 FALSE 0 0)

    ; Create bumpmap layer

    (gimp-edit-fill bumpmap BG-IMAGE-FILL)

    (gimp-palette-set-background '(127 127 127))
    (gimp-rect-select img 1 1 (- width 2) (- height 2) REPLACE FALSE 0)
    (gimp-edit-fill bumpmap BG-IMAGE-FILL)

    (gimp-palette-set-background '(255 255 255))
    (gimp-rect-select img 2 2 (- width 4) (- height 4) REPLACE FALSE 0)
    (gimp-edit-fill bumpmap BG-IMAGE-FILL)

    (gimp-selection-none img)

    ; Bumpmap

    (plug-in-bump-map 1 img background bumpmap 135 45 2 0 0 0 0 TRUE FALSE 0)

    (gimp-image-set-active-layer img background)
    (gimp-image-remove-layer img bumpmap)

    (gimp-palette-set-background old-bg-color)
    (gimp-image-undo-enable img)
    (gimp-display-new img)))


(script-fu-register "script-fu-beveled-pattern-hrule"
		    "<Toolbox>/Xtns/Script-Fu/Web Page Themes/Beveled Pattern/Hrule..."
		    "Beveled pattern hrule"
		    "Federico Mena Quintero"
		    "Federico Mena Quintero"
		    "July 1997"
		    ""
		    SF-VALUE "Width"   "480"
		    SF-VALUE "Height"  "16"
		    SF-PATTERN "Pattern" "Wood")
