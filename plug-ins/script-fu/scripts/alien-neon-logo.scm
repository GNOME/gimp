; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
; 
; alien-neon-logo.scm - creates multiple outlines around the letters
; Copyright (C) 1999-2000 Raphael Quinet <quinet@gamers.org>
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
; 2000-04-02 Split the script in two parts: one using text, one using alpha.
; 2000-05-29 More modifications for "Alpha to Logo" using a separate function.
;
; To do: use a channel mask for creating the bands, instead of working in the
; image.  gimp-invert would then work on one grayscale channel instead of
; wasting CPU cycles on three identical R, G, B channels.
; 

(define (apply-alien-neon-logo-effect img
				      logo-layer
				      fg-color
				      bg-color 
				      band-size 
				      gap-size 
				      num-bands 
				      do-fade)
  (let* ((fade-size (- (* (+ band-size gap-size) num-bands) 1))
	 (width (car (gimp-drawable-width logo-layer)))
	 (height (car (gimp-drawable-height logo-layer)))
	 (bg-layer (car (gimp-layer-new img width height RGB_IMAGE "Background" 100 NORMAL)))
	 (bands-layer (car (gimp-layer-new img width height RGBA_IMAGE "Bands" 100 NORMAL)))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background))))
    (gimp-image-resize img width height 0 0)
    (gimp-image-add-layer img bg-layer 1)
    (gimp-image-add-layer img bands-layer 1)
    (gimp-selection-none img)
    (gimp-palette-set-background bg-color)
    (gimp-edit-fill bg-layer BG-IMAGE-FILL)
    (gimp-palette-set-background '(0 0 0))
    (gimp-edit-fill bands-layer BG-IMAGE-FILL)
    ; The text layer is never shown: it is only used to create a selection
    (gimp-selection-layer-alpha logo-layer)
    (gimp-palette-set-foreground '(255 255 255))
    (gimp-edit-fill bands-layer FG-IMAGE-FILL)

    ; Create multiple outlines by growing and inverting the selection
    ; The bands are black and white because they will be used as a mask.
    (while (> num-bands 0)
	   (gimp-selection-grow img band-size)
	   (gimp-invert bands-layer)
	   (gimp-selection-grow img gap-size)
	   (gimp-invert bands-layer)
	   (set! num-bands (- num-bands 1)))

    ; The fading effect is obtained by masking the image with a gradient.
    ; The gradient is created by filling a bordered selection (white->black).
    (if (= do-fade TRUE)
	(let ((bands-layer-mask (car (gimp-layer-create-mask bands-layer
							     BLACK-MASK))))
	  (gimp-image-add-layer-mask img bands-layer bands-layer-mask)
	  (gimp-selection-layer-alpha logo-layer)
	  (gimp-selection-border img fade-size)
	  (gimp-edit-fill bands-layer-mask FG-IMAGE-FILL)
	  (gimp-image-remove-layer-mask img bands-layer APPLY)))

    ; Transfer the resulting grayscale bands into the layer mask.
    (let ((bands-layer-mask (car (gimp-layer-create-mask bands-layer
							 BLACK-MASK))))
      (gimp-image-add-layer-mask img bands-layer bands-layer-mask)
      (gimp-selection-none img)
      (gimp-edit-copy bands-layer)
      (gimp-floating-sel-anchor (car (gimp-edit-paste bands-layer-mask
						      FALSE))))

    ; Fill the layer with the foreground color.  The areas that are not
    ; masked become visible.
    (gimp-palette-set-foreground fg-color)
    (gimp-edit-fill bands-layer FG-IMAGE-FILL)
    ;; (gimp-image-remove-layer-mask img bands-layer APPLY)

    ; Clean up and exit.
    (gimp-palette-set-foreground old-fg)
    (gimp-palette-set-background old-bg)
    (gimp-layer-set-visible logo-layer 0)
    (gimp-image-set-active-layer img bands-layer)
    (gimp-displays-flush)))

(define (script-fu-alien-neon-logo-alpha img
					 logo-layer
					 fg-color
					 bg-color
					 band-size
					 gap-size
					 num-bands
					 do-fade)
  (begin
    (gimp-undo-push-group-start img)
    (apply-alien-neon-logo-effect img logo-layer fg-color bg-color 
				  band-size gap-size num-bands do-fade)
    (gimp-undo-push-group-end img)
    (gimp-displays-flush)))

(script-fu-register "script-fu-alien-neon-logo-alpha"
		    _"<Image>/Script-Fu/Alpha to Logo/Alien Neon..."
		    "Creates a psychedelic effect with outlines of the specified color around the letters"
		    "Raphael Quinet (quinet@gamers.org)"
		    "Raphael Quinet"
		    "1999-2000"
		    "RGBA"
                    SF-IMAGE      "Image" 0
                    SF-DRAWABLE   "Drawable" 0
		    SF-COLOR      _"Glow Color" '(0 255 0)
		    SF-COLOR      _"Background Color" '(0 0 0)
                    SF-ADJUSTMENT _"Width of Bands" '(2 1 60 1 10 0 0)
                    SF-ADJUSTMENT _"Width of Gaps" '(2 1 60 1 10 0 0)
                    SF-ADJUSTMENT _"Number of Bands" '(7 1 100 1 10 0 1)
		    SF-TOGGLE     _"Fade Away" TRUE
		    )

(define (script-fu-alien-neon-logo text 
				   size 
				   fontname 
				   fg-color 
				   bg-color 
				   band-size 
				   gap-size 
				   num-bands 
				   do-fade)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
	 (fade-size (- (* (+ band-size gap-size) num-bands) 1))
	 (text-layer (car (gimp-text-fontname img -1 0 0 text (+ fade-size 10) TRUE size PIXELS fontname))))
    (gimp-image-undo-disable img)
    (gimp-layer-set-name text-layer text)
    (apply-alien-neon-logo-effect img text-layer fg-color bg-color
				  band-size gap-size num-bands do-fade)
    (gimp-image-undo-enable img)
    (gimp-display-new img)))

(script-fu-register "script-fu-alien-neon-logo"
		    _"<Toolbox>/Xtns/Script-Fu/Logos/Alien Neon..."
		    "Creates a psychedelic effect with outlines of the specified color around the letters"
		    "Raphael Quinet (quinet@gamers.org)"
		    "Raphael Quinet"
		    "1999-2000"
		    ""
		    SF-STRING     _"Text" "The GIMP"
		    SF-ADJUSTMENT _"Font Size (pixels)" '(150 2 1000 1 10 0 1)
		    SF-FONT       _"Font" "-*-blippo-*-r-*-*-24-*-*-*-p-*-*-*"
		    SF-COLOR      _"Glow Color" '(0 255 0)
		    SF-COLOR      _"Background Color" '(0 0 0)
                    SF-ADJUSTMENT _"Width of Bands" '(2 1 60 1 10 0 0)
                    SF-ADJUSTMENT _"Width of Gaps" '(2 1 60 1 10 0 0)
                    SF-ADJUSTMENT _"Number of Bands" '(7 1 100 1 10 0 1)
		    SF-TOGGLE     _"Fade Away" TRUE
		    )

; end

