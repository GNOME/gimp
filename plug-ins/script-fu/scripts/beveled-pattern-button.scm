; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
; 
; Beveled pattern button for web pages
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


(define (text-width extents)
  (car extents))

(define (text-height extents)
  (cadr extents))

(define (text-ascent extents)
  (caddr extents))

(define (text-descent extents)
  (cadr (cddr extents)))

(define (script-fu-beveled-pattern-button
	 text text-size foundry family weight slant set-width spacing text-color pattern pressed)
  (let* ((old-bg-color (car (gimp-palette-get-background)))

	 (text-extents (gimp-text-get-extents
			text text-size PIXELS foundry family weight slant set-width spacing))
	 (ascent (text-ascent text-extents))
	 (descent (text-descent text-extents))

	 (xpadding 8)
	 (ypadding 6)

	 (width (+ (* 2 xpadding)
		   (- (text-width text-extents)
		      (text-width
		       (gimp-text-get-extents
			" " text-size PIXELS foundry family weight slant set-width spacing)))))
	 (height (+ (* 2 ypadding)
		    (+ ascent descent)))
	 
	 (img (car (gimp-image-new width height RGB)))
	 (background (car (gimp-layer-new img width height RGBA_IMAGE "Background" 100 NORMAL)))
	 (bumpmap (car (gimp-layer-new img width height RGBA_IMAGE "Bumpmap" 100 NORMAL)))
	 (textl (car
		 (gimp-text
		  img -1 0 0 text 0 TRUE text-size PIXELS foundry family weight slant set-width spacing))))

    (gimp-image-disable-undo img)
    (gimp-image-add-layer img background 1)
    (gimp-image-add-layer img bumpmap 1)

    ; Create pattern layer

    (gimp-palette-set-background '(0 0 0))
    (gimp-edit-fill img background)
    (gimp-patterns-set-pattern pattern)
    (gimp-bucket-fill img background PATTERN-BUCKET-FILL NORMAL 100 0 FALSE 0 0)

    ; Create bumpmap layer

    (gimp-edit-fill img bumpmap)

    (gimp-palette-set-background '(127 127 127))
    (gimp-rect-select img 1 1 (- width 2) (- height 2) REPLACE FALSE 0)
    (gimp-edit-fill img bumpmap)

    (gimp-palette-set-background '(255 255 255))
    (gimp-rect-select img 2 2 (- width 4) (- height 4) REPLACE FALSE 0)
    (gimp-edit-fill img bumpmap)

    (gimp-selection-none img)

    ; Bumpmap

    (plug-in-bump-map 1 img background bumpmap 135 45 2 0 0 0 0 TRUE pressed 0)

    ; Color and position text

    (gimp-palette-set-background text-color)
    (gimp-layer-set-preserve-trans textl TRUE)
    (gimp-edit-fill img textl)

    (gimp-layer-set-offsets textl
			    xpadding
			    (+ ypadding descent))

    ; Clean up

    (gimp-image-set-active-layer img background)
    (gimp-image-remove-layer img bumpmap)
    (gimp-image-flatten img)

    (gimp-palette-set-background old-bg-color)
    (gimp-image-enable-undo img)
    (gimp-display-new img)))


(script-fu-register "script-fu-beveled-pattern-button"
		    "<Toolbox>/Xtns/Script-Fu/Web page themes/Beveled pattern/Button"
		    "Beveled pattern button"
		    "Federico Mena Quintero"
		    "Federico Mena Quintero"
		    "July 1997"
		    ""
		    SF-VALUE  "Text"       "\"Hello world!\""
		    SF-VALUE  "Text size"  "32"
		    SF-VALUE  "Foundry"    "\"adobe\""
		    SF-VALUE  "Family"     "\"utopia\""
		    SF-VALUE  "Weight"     "\"bold\""
		    SF-VALUE  "Slant"      "\"r\""
		    SF-VALUE  "Set width"  "\"normal\""
		    SF-VALUE  "Spacing"    "\"p\""
		    SF-COLOR  "Text color" '(0 0 0)
		    SF-VALUE  "Pattern"    "\"Wood\""
		    SF-TOGGLE "Pressed?"   FALSE)
