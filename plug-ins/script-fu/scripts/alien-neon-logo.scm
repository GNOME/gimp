; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
; 
; alien-neon-logo.scm - creates multiple outlines around the letters
; Copyright (C) 1999 Raphael Quinet <quinet@gamers.org>
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
; 1999-12-01 First version.
; 2000-02-19 Do not discard the layer mask so that it can still be edited.
; 2000-03-08 Adapted the script to my gimp-edit-fill changes.
; 

(define (script-alien-neon-logo text size fontname fg-color bg-color band-size gap-size num-bands do-fade)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
	 (fade-size (- (* (+ band-size gap-size) num-bands) 1))
	 (text-layer (car (gimp-text-fontname img -1 0 0 text (+ fade-size 10) TRUE size PIXELS fontname)))
	 (width (car (gimp-drawable-width text-layer)))
	 (height (car (gimp-drawable-height text-layer)))
	 (bg-layer (car (gimp-layer-new img width height RGB_IMAGE "Background" 100 NORMAL)))
	 (bands-layer (car (gimp-layer-new img width height RGBA_IMAGE "Bands" 100 NORMAL)))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background)))
	 )
    (gimp-image-undo-disable img)
    (gimp-image-resize img width height 0 0)
    (gimp-image-add-layer img bg-layer 1)
    (gimp-image-add-layer img bands-layer 1)
    (gimp-selection-none img)
    (gimp-palette-set-background bg-color)
    (gimp-edit-fill bg-layer BG-IMAGE-FILL)
    (gimp-palette-set-background '(0 0 0))
    (gimp-edit-fill bands-layer BG-IMAGE-FILL)
    ; The text layer is never shown: it is only used to create a selection
    (gimp-selection-layer-alpha text-layer)
    (gimp-palette-set-foreground '(255 255 255))
    (gimp-edit-fill bands-layer FG-IMAGE-FILL)

    ; Create multiple outlines by growing and inverting the selection
    ; The bands are black and white because they will be used as a mask.
    (while (> num-bands 0)
	   (gimp-selection-grow img band-size)
	   (gimp-invert bands-layer)
	   (gimp-selection-grow img gap-size)
	   (gimp-invert bands-layer)
	   (set! num-bands (- num-bands 1))
	   )

    ; The fading effect is obtained by masking the image with a gradient.
    ; The gradient is created by filling a bordered selection (white->black).
    (if (= do-fade TRUE)
	(let ((bands-layer-mask (car (gimp-layer-create-mask bands-layer
							     BLACK-MASK)))
	      )
	  (gimp-image-add-layer-mask img bands-layer bands-layer-mask)
	  (gimp-selection-layer-alpha text-layer)
	  (gimp-selection-border img fade-size)
	  (gimp-edit-fill bands-layer-mask FG-IMAGE-FILL)
	  (gimp-image-remove-layer-mask img bands-layer APPLY)
	  )
	)

    ; Transfer the resulting grayscale bands into the layer mask.
    (let ((bands-layer-mask (car (gimp-layer-create-mask bands-layer
							 BLACK-MASK)))
	  )
      (gimp-image-add-layer-mask img bands-layer bands-layer-mask)
      (gimp-selection-none img)
      (gimp-edit-copy bands-layer)
      (gimp-floating-sel-anchor (car (gimp-edit-paste bands-layer-mask
						      FALSE)))
      )

    ; Fill the layer with the foreground color.  The areas that are not
    ; masked become visible.
    (gimp-palette-set-foreground fg-color)
    (gimp-edit-fill bands-layer FG-IMAGE-FILL)
    ;; (gimp-image-remove-layer-mask img bands-layer APPLY)

    ; Clean up and exit.
    (gimp-palette-set-foreground old-fg)
    (gimp-palette-set-background old-bg)
    (gimp-layer-set-visible text-layer 0)
    (gimp-image-set-active-layer img bands-layer)
    (gimp-image-undo-enable img)
    (gimp-display-new img)
    )
  )

(script-fu-register "script-alien-neon-logo"
		    "<Toolbox>/Xtns/Script-Fu/Logos/Alien Neon..."
		    "Creates a psychedelic effect with outlines\nof the specified color around the letters"
		    "Raphael Quinet (quinet@gamers.org)"
		    "Raphael Quinet"
		    "1999-2000"
		    ""
		    SF-STRING "Text String" "The GIMP"
		    SF-ADJUSTMENT "Font size (in pixels)" '(150 2 1000 1 10 0 1)
		    SF-FONT "Font" "-*-blippo-*-r-*-*-24-*-*-*-p-*-*-*"
		    SF-COLOR "Glow color" '(0 255 0)
		    SF-COLOR "Background color" '(0 0 0)
                    SF-ADJUSTMENT "Width of bands" '(2 1 60 1 10 0 0)
                    SF-ADJUSTMENT "Width of gaps" '(2 1 60 1 10 0 0)
                    SF-ADJUSTMENT "Number of bands" '(7 1 100 1 10 0 1)
		    SF-TOGGLE "Fade away?" TRUE
		    )
; end

