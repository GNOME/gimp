; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
; 
; Round Button --- create a round beveled Web button.
; Copyright (C) 1998 Federico Mena Quintero & Arturo Espinosa Aldama
; federico@nuclecu.unam.mx arturo@nuclecu.unam.mx
; ************************************************************************
; Changed on Feb 4, 1999 by Piet van Oostrum <piet@cs.uu.nl>
; For use with GIMP 1.1.
; All calls to gimp-text-* have been converted to use the *-fontname form.
; The corresponding parameters have been replaced by an SF-FONT parameter.
; ************************************************************************
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

(define (round-select img x y width height ratio)
  (let* ((diameter (* ratio height)))
    (gimp-ellipse-select img x y diameter height ADD FALSE 0 0)
    (gimp-ellipse-select img (+ x (- width diameter)) y
			 diameter height ADD FALSE 0 0)
    (gimp-rect-select img (+ x (/ diameter 2)) y
		      (- width diameter) height ADD FALSE 0)))
  
(define (script-fu-round-button text
			    size
			    font
			    ul-color
			    lr-color
			    text-color
			    ul-color-high
			    lr-color-high
			    hlight-color
			    xpadding
			    ypadding
			    bevel
			    ratio
			    notpressed
			    notpressed-active
			    pressed)

  (cond ((eqv? notpressed TRUE)
	 (do-pupibutton text size font ul-color lr-color
			text-color xpadding ypadding bevel ratio 0)))
  (cond ((eqv? notpressed-active TRUE)
	 (do-pupibutton text size font ul-color-high lr-color-high
			hlight-color xpadding ypadding bevel ratio 0)))
  (cond ((eqv? pressed TRUE)
	 (do-pupibutton text size font ul-color-high lr-color-high
			hlight-color xpadding ypadding bevel ratio 1))))
  
(define (do-pupibutton text
			    size
			    font
			    ul-color
			    lr-color
			    text-color
			    xpadding
			    ypadding
			    bevel
			    ratio
			    pressed)

  (let* ((old-fg-color (car (gimp-palette-get-foreground)))
	 (old-bg-color (car (gimp-palette-get-background)))
	 
	 (text-extents (gimp-text-get-extents-fontname text
					      size
					      PIXELS
					      font))
	 (ascent (text-ascent text-extents))
	 (descent (text-descent text-extents))
	 
	 (height (+ (* 2 (+ ypadding bevel))
			(+ ascent descent)))

	 (radius (/ (* ratio height) 4))

	 (width (+ (* 2 (+ radius xpadding)) bevel
		       (- (text-width text-extents)
			  (text-width (gimp-text-get-extents-fontname " "
							     size
							     PIXELS
							     font)))))
							     
	 (img (car (gimp-image-new width height RGB)))

	 (bumpmap (car (gimp-layer-new img width height
				       RGBA_IMAGE "Bumpmap" 100 NORMAL)))
	 (gradient (car (gimp-layer-new img width height
					RGBA_IMAGE "Button" 100 NORMAL))))
    (gimp-image-undo-disable img)

    ; Create bumpmap layer
    
    (gimp-image-add-layer img bumpmap -1)
    (gimp-selection-none img)
    (gimp-palette-set-background '(0 0 0))
    (gimp-edit-fill bumpmap)

    (round-select img (/ bevel 2) (/ bevel 2)
		  (- width bevel) (- height bevel) ratio)
    (gimp-palette-set-background '(255 255 255))
    (gimp-edit-fill bumpmap)

    (gimp-selection-none img)
    (plug-in-gauss-rle 1 img bumpmap bevel 1 1)

    ; Create gradient layer

    (gimp-image-add-layer img gradient -1)
    (gimp-edit-clear gradient)
    (round-select img 0 0 width height ratio)
    (gimp-palette-set-foreground ul-color)
    (gimp-palette-set-background lr-color)

    (gimp-blend gradient
		FG-BG-RGB
		NORMAL
		LINEAR
		100
		0
		REPEAT-NONE
		FALSE
		0
		0
		0
		0
		0
		(- height 1))

    (gimp-selection-none img)

    (plug-in-bump-map 1 img gradient bumpmap
		      135 45 bevel 0 0 0 0 TRUE pressed 0)

;     Create text layer

    (cond ((eqv? pressed 1) (set! bevel (+ bevel 1))))

    (gimp-palette-set-foreground text-color)
    (let ((textl (car (gimp-text-fontname
		       img -1 0 0 text 0 TRUE size PIXELS
		       font))))
      (gimp-layer-set-offsets textl
			      (+ xpadding radius bevel)
			      (+ ypadding descent bevel)))

;   Delete some fucked-up pixels.

    (gimp-selection-none img)
    (round-select img 1 1 (- width 1) (- height 1) ratio)
    (gimp-selection-invert img)
    (gimp-edit-clear gradient)

;     Done

    (gimp-image-remove-layer img bumpmap)
    (gimp-image-merge-visible-layers img EXPAND-AS-NECESSARY)

    (gimp-selection-none img)
    (gimp-palette-set-foreground old-fg-color)
    (gimp-palette-set-background old-bg-color)
    (gimp-image-undo-enable img)
    (gimp-display-new img)))

; Register!

(script-fu-register "script-fu-round-button"
		    "<Toolbox>/Xtns/Script-Fu/Buttons/Round Button..."
		    "Round button"
		    "Arturo Espinosa (stolen from quartic's beveled button)"
		    "Arturo Espinosa & Federico Mena Quintero"
		    "June 1998"
		    ""
		    SF-STRING "Text" ""
		    SF-ADJUSTMENT "Font Size (pixels)" '(16 2 100 1 1 0 1)
		    SF-FONT  "Font" "-*-helvetica-*-r-*-*-24-*-*-*-p-*-*-*"
		    SF-COLOR "Upper color" '(192 192 0)
		    SF-COLOR "Lower color" '(128 108 0)
		    SF-COLOR "Text color" '(0 0 0)
		    SF-COLOR "Upper color (active)" '(255 255 0)
		    SF-COLOR "Lower color (active)" '(128 108 0)
		    SF-COLOR "Text color (active)" '(0 0 192)
		    SF-VALUE "xPadding" "4"
		    SF-VALUE "yPadding" "4"
		    SF-VALUE "Bevel width" "2"
		    SF-VALUE "Round ratio" "1"
		    SF-TOGGLE "Not-pressed         " TRUE
		    SF-TOGGLE "Not-pressed (active)" TRUE
		    SF-TOGGLE "Pressed             " TRUE)


