; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
; 
; Alien Glow themed button
; Copyright (c) 1997 Adrian Likins
; aklikins@eos.ncsu.edu
;
; based on code from Frederico Mena Quintero (Quartic)
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

(define (blend-bumpmap img drawable x1 y1 x2 y2)
  (gimp-blend drawable
	      FG-BG-RGB
	      DARKEN-ONLY
	      LINEAR
	      100
	      0
	      REPEAT-NONE
	      FALSE
	      0
	      0
	      x1
	      y1
	      x2
	      y2))

(define (script-fu-alien-glow-button text
			    font
			    size
			    text-color
			    glow-color
			    bg-color
			    padding
			    glow-radius
			    flatten)
  (let* ((old-fg-color (car (gimp-palette-get-foreground)))
	 (old-bg-color (car (gimp-palette-get-background)))
	 (text-extents (gimp-text-get-extents-fontname text
						       size
						       PIXELS
						       font))
	 (ascent (text-ascent text-extents))
	 (descent (text-descent text-extents))
	 
	 (img-width (+ (* 2  padding)
		       (- (text-width text-extents)
			  (text-width (gimp-text-get-extents-fontname " "
								      size
								      PIXELS
								      font)))))
	 (img-height (+ (* 2 padding)
			(+ ascent descent)))
	 (layer-height img-height)
	 (layer-width img-width)
	 (img-width (+ img-width glow-radius))
	 (img-height (+ img-height glow-radius))
	 (img (car (gimp-image-new  img-width   img-height  RGB)))
	 (bg-layer (car (gimp-layer-new img img-width img-height RGBA_IMAGE "Background" 100 NORMAL)))
	 (glow-layer (car (gimp-layer-new img img-width img-height RGBA_IMAGE "Glow" 100 NORMAL)))
	 (button-layer (car (gimp-layer-new img layer-width layer-height RGBA_IMAGE "Button" 100 NORMAL))))

    (gimp-image-undo-disable img)

    ; Create bumpmap layer
    
    (gimp-image-add-layer img bg-layer -1)
    (gimp-palette-set-foreground '(0 0 0))
    (gimp-palette-set-background bg-color)
    (gimp-edit-fill bg-layer BG-IMAGE-FILL)
    (gimp-image-add-layer img glow-layer -1)

    ; Create text layer

    (gimp-image-add-layer img button-layer -1)
    (gimp-layer-set-offsets button-layer (/ glow-radius 2) (/ glow-radius 2))
    (gimp-selection-none img)
    (gimp-rect-select img 0 0 img-width img-height REPLACE FALSE 0)
    (gimp-palette-set-foreground '(100 100 100))
    (gimp-palette-set-background '(0 0 0))
    (gimp-blend button-layer FG-BG-RGB NORMAL SHAPEBURST-ANGULAR 100 0 REPEAT-NONE FALSE 0 0 0 0 img-height img-width)
    (gimp-edit-clear glow-layer)
    (gimp-rect-select img (/ glow-radius 4) (/ glow-radius 4) (- img-width (/ glow-radius 2)) (- img-height (/ glow-radius 2)) REPLACE FALSE 0 )
    (gimp-palette-set-foreground glow-color)
    (gimp-edit-fill glow-layer FG-IMAGE-FILL)
    (gimp-selection-none img)
    (plug-in-gauss-rle 1 img glow-layer glow-radius TRUE TRUE)
    (gimp-palette-set-foreground text-color)
    (let ((textl (car (gimp-text-fontname
		       img -1 0 0 text 0 TRUE size PIXELS font))))
      (gimp-layer-set-offsets textl
			      (+  padding (/ glow-radius 2))
			      (+ (+ padding descent) (/ glow-radius 2))))
    ; Done
    (gimp-selection-none img)
    (gimp-palette-set-foreground old-fg-color)
    (gimp-palette-set-background old-bg-color)
    (gimp-image-undo-enable img)
    (if (= flatten TRUE)
	(gimp-image-flatten img))
    (gimp-display-new img)))

; Register!

(script-fu-register "script-fu-alien-glow-button"
		    "<Toolbox>/Xtns/Script-Fu/Web Page Themes/Alien Glow/Button..."
		    "Button with an eerie glow"
		    "Adrian Likins"
		    "Adrian Likins"
		    "July 1997"
		    ""
		    SF-STRING "Text" "Hello world!"
		    SF-FONT "Font" "-*-futura_poster-*-*-*-*-22-*-*-*-*-*-*-*"
		    SF-ADJUSTMENT "Font Size (pixels)" '(22 2 100 1 1 0 1)
		    SF-COLOR "Text color" '(0 0 0)
		    SF-COLOR "Glow Color" '(63 252 0)
		    SF-COLOR "Background Color" '(0 0 0)
		    SF-VALUE "Padding" "6"
		    SF-VALUE "Glow Radius" "10"
		    SF-TOGGLE "Flatten Image?" TRUE)
