; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
; 
; Alien Glow themed bullets for web pages
; Copyright (c) 1997 Adrian Likins
; aklikins@eos.ncsu.edu 
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

(define (center-ellipse img cx cy rx ry op aa feather frad)
  (gimp-ellipse-select img (- cx rx) (- cy ry) (+ rx rx ) (+ ry ry ) op aa feather frad))



(define (script-fu-alien-glow-bullet radius glow-color bg-color flatten)
  (let* ((img (car (gimp-image-new radius radius RGB)))
	 (border (/ radius 4))
	 (diameter (* radius 2))
	 (half-radius (/ radius 2))
	 (blend-start (+ half-radius (/ half-radius 2)))
	 (bullet-layer (car (gimp-layer-new img diameter diameter RGBA_IMAGE "Ruler" 100 NORMAL)))
	 (glow-layer (car (gimp-layer-new img diameter diameter RGBA_IMAGE "ALien Glow" 100 NORMAL)))
	 (bg-layer (car (gimp-layer-new img diameter diameter RGB_IMAGE "Back" 100 NORMAL)))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background))))
    (gimp-image-undo-disable img)
    (gimp-image-resize img diameter diameter 0 0)
    (gimp-image-add-layer img bg-layer 1)
    (gimp-image-add-layer img glow-layer -1)
    (gimp-image-add-layer img bullet-layer -1)
     
   ; (gimp-layer-set-preserve-trans ruler-layer TRUE)
    (gimp-palette-set-background bg-color)
    (gimp-edit-fill bg-layer)
    (gimp-edit-clear glow-layer)
    (gimp-edit-clear bullet-layer)

    (center-ellipse img radius radius half-radius half-radius REPLACE TRUE FALSE 0)
    
;    (gimp-rect-select img (/ height 2) (/ height 2) length height REPLACE FALSE 0)
    (gimp-palette-set-foreground '(90 90 90))
    (gimp-palette-set-background '(0 0 0))
    (gimp-blend bullet-layer FG-BG-RGB NORMAL RADIAL 100 0 REPEAT-NONE FALSE 0 0 blend-start blend-start (+ half-radius radius)(+ half-radius radius ))

    (gimp-palette-set-background glow-color)
    (gimp-selection-grow img border)
    (gimp-selection-feather img  border)
    (gimp-edit-fill glow-layer)
    (gimp-selection-none img)
    (if (>= radius 16)
	(plug-in-gauss-rle 1 img glow-layer 25 TRUE TRUE)
	(plug-in-gauss-rle 1 img glow-layer 12 TRUE TRUE))

    (gimp-palette-set-background old-bg)
    (gimp-palette-set-foreground old-fg)

    (if (= flatten TRUE)
	(gimp-image-flatten img))
    (gimp-image-undo-enable img)
    (gimp-display-new img)))




(script-fu-register "script-fu-alien-glow-bullet"
		    "<Toolbox>/Xtns/Script-Fu/Web Page Themes/Alien Glow/Bullet..."
		    "Create a Bullet with an Alien Glow them for web pages"
		    "Adrian Likins"
		    "Adrian Likins"
		    "1997"
		    ""
		    SF-VALUE "Radius" "16"
		    SF-COLOR "Glow Color" '(63 252 0)
		    SF-COLOR "Background Color" '(0 0 0)
		    SF-TOGGLE "Flatten Image?" TRUE)





