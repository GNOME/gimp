; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
; 
; Bump-mapped title script --- create a bump-mapped title image for web pages
; Copyright (C) 1997 Federico Mena Quintero
; federico@nuclecu.unam.mx
; ************************************************************************
; Changed on Feb 4, 1999 by Piet van Oostrum <piet@cs.uu.nl>
; For use with GIMP 1.1.
verted to use the *-fontname form.
; The corresponding parameters have been replaced by an SF-FONT parameter.
; The call to gimp-palette-set-background has been given a real layer
; (although it is not used) otherwise gimp 1.1 crashed.
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


(define (script-fu-title-header text
				size
;				foundry
;				family
;				weight
;				slant
;				set-width
				fontname)
  (let* (; Parameters
	 
	 (padding 8)
	 (fade-width 64)
	 
	 ; Save foreground and background colors

	 (old-fg-color (car (gimp-palette-get-foreground)))
	 (old-bg-color (car (gimp-palette-get-background)))
	 
	 ; Image 

	 (img (car (gimp-image-new 256 256 RGB)))

	 ; Text layer
	 
	 (text-layer (car (gimp-text-fontname
			   img
			   -1
			   0
			   0
			   text
			   padding
			   TRUE
			   size
			   PIXELS
			   fontname)))
	 (text-width (car (gimp-drawable-width text-layer)))
	 (text-height (car (gimp-drawable-height text-layer)))

	 ; Sizes

	 (text-layers-offset (/ text-height 2))

	 (img-width (+ text-layers-offset text-width fade-width))
	 (img-height text-height)

	 ; Additional layers
	 
	 (bg-layer (car (gimp-layer-new img img-width img-height RGBA_IMAGE "bg-layer" 100 NORMAL)))
	 (bumpmap-layer (car (gimp-layer-new img
					     text-width
					     text-height
					     RGBA_IMAGE
					     "bumpmap-layer"
					     100
					     NORMAL)))
	 (fore-layer (car (gimp-layer-new img text-width text-height RGBA_IMAGE "fore-layer" 100 NORMAL))))
    
    ; Create image
    
    (gimp-image-undo-disable img)
    (gimp-image-resize img img-width img-height 0 0)
    
    (gimp-image-add-layer img bg-layer -1)
    (gimp-image-add-layer img bumpmap-layer -1)
    (gimp-image-add-layer img fore-layer -1)
;    (gimp-image-add-layer img text-layer -1)
    (gimp-image-raise-layer img text-layer)
    (gimp-image-raise-layer img text-layer)
    (gimp-image-raise-layer img text-layer)
    (gimp-layer-set-offsets bg-layer 0 0)
    (gimp-layer-set-offsets text-layer text-layers-offset 0)
    (gimp-layer-set-offsets bumpmap-layer text-layers-offset 0)
    (gimp-layer-set-offsets fore-layer text-layers-offset 0)

    ; Create bumpmap layer

    (gimp-palette-set-background '(0 0 0))
    (gimp-edit-fill bumpmap-layer)
    (gimp-selection-layer-alpha text-layer)
    (gimp-palette-set-background '(255 255 255))
    (gimp-edit-fill bumpmap-layer)
    (gimp-selection-none img)
    (plug-in-gauss-rle 1 img bumpmap-layer 4.0 TRUE TRUE)

    ; Fore layer, bumpmap

    (gimp-palette-set-background '(255 255 255))
    (gimp-edit-fill fore-layer)
    (plug-in-bump-map 1 img fore-layer bumpmap-layer 135.0 45.0 4 0 0 0 0 FALSE FALSE 0)

    ; Text layer

    (gimp-layer-set-visible text-layer TRUE)
    (gimp-layer-set-preserve-trans text-layer TRUE)
    (gimp-blend text-layer CUSTOM NORMAL LINEAR 100 0 REPEAT-NONE FALSE 0.2 3
		padding
		padding
		(- text-width padding 1)
		(- text-height padding 1))

    ; Semicircle at the left

    (gimp-palette-set-background '(0 0 0))
    (gimp-edit-fill bg-layer)
    
    (gimp-ellipse-select img 0 0 text-height text-height REPLACE TRUE FALSE 0)
    (gimp-palette-set-background (car (gimp-color-picker text-layer text-layers-offset 0 TRUE FALSE)))
    (gimp-edit-fill bg-layer)

    ; Fade-out gradient at the right

    (gimp-rect-select img (- img-width fade-width) 0 fade-width text-height REPLACE FALSE 0)
    (gimp-palette-set-foreground (car (gimp-palette-get-background)))
    (gimp-palette-set-background '(0 0 0))
    (gimp-blend bg-layer
		FG-BG-RGB
		NORMAL
		LINEAR
		100
		0
		REPEAT-NONE
		FALSE
		0.2
		3
		(- img-width fade-width)
		0
		(- img-width 1)
		0)
    (gimp-selection-none img)

    ; Done
    
;    (gimp-image-flatten img)
    (gimp-palette-set-foreground old-fg-color)
    (gimp-palette-set-background old-bg-color)
    (gimp-image-undo-enable img)
    (gimp-display-new img)))

(script-fu-register "script-fu-title-header"
		    "<Toolbox>/Xtns/Script-Fu/Logos/Web title header"
		    "Web title header"
		    "Federico Mena Quintero"
		    "Federico Mena Quintero"
		    "June 1997"
		    ""
		    SF-STRING "Text" "Hello world!"
		    SF-ADJUSTMENT  "Text size" '(32 2 256 1 10 0 0)
		    SF-FONT "Font" "-adobe-helvetica-bold-r-normal-*-30-*-*-*-p-*-*-*")
