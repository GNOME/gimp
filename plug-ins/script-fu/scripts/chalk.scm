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
; chalk.scm  version 0.11  10/10/97
;
; Copyright (C) 1997 Manish Singh <msingh@uclink4.berkeley.edu>
;  
; Makes a logo with a chalk-like text effect.

(define (apply-chalk-logo-effect img
				 logo-layer
				 bg-color)
  (let* ((width (car (gimp-drawable-width logo-layer)))
	 (height (car (gimp-drawable-height logo-layer)))
	 (bg-layer (car (gimp-layer-new img width height RGB_IMAGE "Background" 100 NORMAL)))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background))))

    (gimp-image-resize img width height 0 0)
    (gimp-image-add-layer img bg-layer 1)
    (gimp-palette-set-background bg-color)
    (gimp-edit-fill bg-layer BG-IMAGE-FILL)

    ; the actual effect
    (gimp-layer-set-preserve-trans logo-layer FALSE)
    (plug-in-gauss-rle 1 img logo-layer 2.0 1 1)
    (plug-in-spread 1 img logo-layer 5.0 5.0)
    (plug-in-ripple 1 img logo-layer 27 2 0 0 0 TRUE TRUE)
    (plug-in-ripple 1 img logo-layer 27 2 1 0 0 TRUE TRUE)
    (plug-in-sobel 1 img logo-layer TRUE TRUE TRUE)
    (gimp-levels logo-layer 0 0 120 3.5 0 255)

    ; work-around for sobel edge detect screw-up (why does this happen?)
    ; the top line of the image has some garbage instead of the bgcolor
    (gimp-rect-select img 0 0 width 1 ADD FALSE 0)
    (gimp-edit-clear logo-layer)
    (gimp-selection-none img)

    (gimp-palette-set-background old-bg)
    (gimp-palette-set-foreground old-fg)))


(define (script-fu-chalk-logo-alpha img
			      logo-layer
			      bg-color)
  (begin
    (gimp-undo-push-group-start img)
    (apply-chalk-logo-effect img logo-layer bg-color)
    (gimp-undo-push-group-end img)
    (gimp-displays-flush)))

(script-fu-register "script-fu-chalk-logo-alpha"
                    _"<Image>/Script-Fu/Alpha to Logo/Chalk..."
                    "Chalk scribbled logos"
                    "Manish Singh <msingh@uclink4.berkeley.edu>"
                    "Manish Singh"
                    "October 1997"
                    "RGBA"
                    SF-IMAGE      "Image" 0
                    SF-DRAWABLE   "Drawable" 0
                    SF-COLOR      _"Background Color" '(0 0 0)
		    )


(define (script-fu-chalk-logo text
			      size
			      font
			      bg-color
			      chalk-color)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
	 (border (/ size 4))
	 (text-layer (car (gimp-text-fontname img -1 0 0 text border TRUE size PIXELS font)))
	 (old-fg (car (gimp-palette-get-foreground))))
    (gimp-image-undo-disable img)
    (gimp-layer-set-name text-layer text)
    (gimp-palette-set-foreground chalk-color)
    (gimp-layer-set-preserve-trans text-layer TRUE)
    (gimp-edit-fill text-layer FG-IMAGE-FILL)
    (gimp-palette-set-foreground old-fg)
    (apply-chalk-logo-effect img text-layer bg-color)
    (gimp-image-undo-enable img)
    (gimp-display-new img)))

(script-fu-register "script-fu-chalk-logo"
                    _"<Toolbox>/Xtns/Script-Fu/Logos/Chalk..."
                    "Chalk scribbled logos"
                    "Manish Singh <msingh@uclink4.berkeley.edu>"
                    "Manish Singh"
                    "October 1997"
                    ""
                    SF-STRING     _"Text" "CHALK"
                    SF-ADJUSTMENT _"Font Size (pixels)" '(150 2 1000 1 10 0 1)
                    SF-FONT       _"Font" "-*-Cooper-*-r-*-*-24-*-*-*-p-*-*-*"
                    SF-COLOR      _"Background Color" '(0 0 0)
                    SF-COLOR      _"Chalk Color" '(255 255 255)
		    )
