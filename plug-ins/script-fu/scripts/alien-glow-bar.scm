; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
; 
; Alien Glow themed hrule for web pages
; Copyright (c) 1997 Adrian Likins
; aklikins@eos.ncsu.edu
;
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

(define (script-fu-alien-glow-horizontal-ruler  length height glow-color bg-color flatten)
  (let* ((img (car (gimp-image-new height length RGB)))
	 (border (/ height 4))
	 (ruler-layer (car (gimp-layer-new img (+ length height) (+ height height) RGBA_IMAGE "Ruler" 100 NORMAL)))
	 (glow-layer (car (gimp-layer-new img (+ length height) (+ height height) RGBA_IMAGE "ALien Glow" 100 NORMAL)))
	 (bg-layer (car (gimp-layer-new img (+ length height) (+ height height) RGB_IMAGE "Back" 100 NORMAL)))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background))))
    (gimp-image-disable-undo img)
    (gimp-image-resize img (+ length height) (+ height height) 0 0)
    (gimp-image-add-layer img bg-layer 1)
    (gimp-image-add-layer img glow-layer -1)
    (gimp-image-add-layer img ruler-layer -1)
     
   ; (gimp-layer-set-preserve-trans ruler-layer TRUE)
    (gimp-palette-set-background bg-color)
    (gimp-edit-fill img bg-layer)
    (gimp-edit-clear img glow-layer)
    (gimp-edit-clear img ruler-layer)
    
    (gimp-rect-select img (/ height 2) (/ height 2) length height REPLACE FALSE 0)
    (gimp-palette-set-foreground '(79 79 79))
    (gimp-palette-set-background '(0 0 0))
    (gimp-blend img ruler-layer FG-BG-RGB NORMAL SHAPEBURST-ANGULAR 100 0 REPEAT-NONE FALSE 0 0 0 0 height height)

    (gimp-palette-set-background glow-color)
    (gimp-selection-grow img border)
    (gimp-edit-fill img glow-layer)
    (gimp-selection-none img)
    (plug-in-gauss-rle 1 img glow-layer 25 TRUE TRUE)

    (gimp-palette-set-background old-bg)
    (gimp-palette-set-foreground old-fg)
    (gimp-image-enable-undo img)

    (if (= flatten TRUE)
	(gimp-image-flatten img))

    (gimp-display-new img)))




(script-fu-register "script-fu-alien-glow-horizontal-ruler"
		    "<Toolbox>/Xtns/Script-Fu/Web page themes/Alien Glow/Hrule"
		    "Create an Hrule with the Alien Glow look"
		    "Adrian Likins"
		    "Adrian Likins"
		    "1997"
		    ""
		    SF-VALUE "Bar length" "480"
		    SF-VALUE "Bar Height" "16"
		    SF-COLOR "Glow Color" '(63 252 0)
		    SF-COLOR "Background Color" '(0 0 0)
		    SF-TOGGLE "Flatten Image" TRUE)




