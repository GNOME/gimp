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

(define (script-fu-chalk-logo text size font bg-color chalk-color)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
	 (border (/ size 4))
	 (text-layer (car (gimp-text-fontname img -1 0 0 text border TRUE size PIXELS font)))
	 (width (car (gimp-drawable-width text-layer)))
	 (height (car (gimp-drawable-height text-layer)))
	 (bg-layer (car (gimp-layer-new img width height RGB_IMAGE "Background" 100 NORMAL)))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background))))

    (gimp-image-undo-disable img)
    (gimp-image-resize img width height 0 0)

    (gimp-image-add-layer img bg-layer 1)
    (gimp-palette-set-background bg-color)
    (gimp-edit-fill bg-layer)
    (gimp-edit-clear text-layer)

    ; is there any other way to do this?
    ; the sobel edge detect won't work with the methods in other scripts
    (gimp-palette-set-foreground chalk-color)
    (set! float-layer (car (gimp-text-fontname img text-layer 0 0 text border TRUE size PIXELS font)))
    (gimp-floating-sel-anchor float-layer)

    ; the actual effect
    (plug-in-gauss-rle 1 img text-layer 2.0 1 1)
    (plug-in-spread 1 img text-layer 5.0 5.0)
    (plug-in-ripple 1 img text-layer 27 2 0 0 0 TRUE TRUE)
    (plug-in-ripple 1 img text-layer 27 2 1 0 0 TRUE TRUE)
    (plug-in-sobel 1 img text-layer TRUE TRUE TRUE)
    (gimp-levels text-layer 0 0 120 3.5 0 255)

    ; work-around for sobel edge detect screw-up (why does this happen?)
    ; the top line of the image has some garbage instead of the bgcolor
    (gimp-rect-select img 0 0 width 1 ADD FALSE 0)
    (gimp-edit-clear text-layer)
    (gimp-selection-none img)

    (gimp-layer-set-preserve-trans text-layer TRUE)
    (gimp-layer-set-name text-layer text)
    (gimp-palette-set-background old-bg)
    (gimp-palette-set-foreground old-fg)
    (gimp-image-undo-enable img)
    (gimp-display-new img)))

(script-fu-register "script-fu-chalk-logo"
                    "<Toolbox>/Xtns/Script-Fu/Logos/Chalk..."
                    "Chalk scribbled logos"
                    "Manish Singh <msingh@uclink4.berkeley.edu>"
                    "Manish Singh"
                    "October 1997"
                    ""
                    SF-STRING "Text String" "CHALK"
                    SF-ADJUSTMENT "Font Size (pixels)" '(150 2 1000 1 10 0 1)
                    SF-FONT "Font" "-*-Cooper-*-r-*-*-24-*-*-*-p-*-*-*"
                    SF-COLOR "Background Color" '(0 0 0)
                    SF-COLOR "Chalk Color" '(255 255 255))
