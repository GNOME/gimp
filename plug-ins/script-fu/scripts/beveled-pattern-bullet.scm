; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
; 
; Beveled pattern bullet for web pages
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


(define (script-fu-beveled-pattern-bullet diameter pattern transparent)
  (let* ((old-bg-color (car (gimp-palette-get-background)))
	 (img (car (gimp-image-new diameter diameter RGB)))
	 (background (car (gimp-layer-new img diameter diameter RGBA_IMAGE "Bullet" 100 NORMAL)))
	 (bumpmap (car (gimp-layer-new img diameter diameter RGBA_IMAGE "Bumpmap" 100 NORMAL))))
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
    (gimp-ellipse-select img 1 1 (- diameter 2) (- diameter 2) REPLACE TRUE FALSE 0)
    (gimp-edit-fill bumpmap BG-IMAGE-FILL)

    (gimp-palette-set-background '(255 255 255))
    (gimp-ellipse-select img 2 2 (- diameter 4) (- diameter 4) REPLACE TRUE FALSE 0)
    (gimp-edit-fill bumpmap BG-IMAGE-FILL)

    (gimp-selection-none img)

    ; Bumpmap

    (plug-in-bump-map 1 img background bumpmap 135 45 2 0 0 0 0 TRUE FALSE 0)

    ; Background

    (gimp-palette-set-background '(0 0 0))
    (gimp-ellipse-select img 0 0 diameter diameter REPLACE TRUE FALSE 0)
    (gimp-selection-invert img)
    (gimp-edit-clear background)
    (gimp-selection-none img)

    (gimp-image-set-active-layer img background)
    (gimp-image-remove-layer img bumpmap)

    (if (= transparent FALSE)
	(gimp-image-flatten img))

    (gimp-palette-set-background old-bg-color)
    (gimp-image-undo-enable img)
    (gimp-display-new img)))


(script-fu-register "script-fu-beveled-pattern-bullet"
		    _"<Toolbox>/Xtns/Script-Fu/Web Page Themes/Beveled Pattern/Bullet..."
		    "Beveled pattern bullet"
		    "Federico Mena Quintero"
		    "Federico Mena Quintero"
		    "July 1997"
		    ""
		    SF-ADJUSTMENT _"Diameter"               '(16 1 150 1 10 0 1)
		    SF-PATTERN    _"Pattern"                "Wood"
		    SF-TOGGLE     _"Transparent Background" FALSE)
