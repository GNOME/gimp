
; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
; 
; FlatLand - creates a tileable pattern that looks like a map
; Copyright (C) 1997 Adrian Likins
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
; FaltLand 
;
;    When used with the Land gradient It produces a tileble pattern that
;    looks a lot like a map. 
;
;    Should be really cool once map-sphere starts working again.
;
;    To use: open gradient editor, load the Land gradient then run the script.
;
;     Adrian Likins <aklikins@eos.ncsu.edu>
;


(define (script-fu-flatland width height seed detail xscale yscale)
  (let* (
	 (img (car (gimp-image-new width height RGB)))
	 (layer-one (car (gimp-layer-new img width height RGB "bottom" 100 NORMAL)))
	)

  (gimp-image-undo-disable img)
  (gimp-image-add-layer img layer-one 0)
 ; (gimp-img-add-layer img layer-two 1)

  (plug-in-solid-noise 1 img layer-one 1 0 seed detail xscale yscale )
  (plug-in-c-astretch 1 img layer-one)
  (set! layer-two (car (gimp-layer-copy layer-one TRUE)))
  (gimp-image-add-layer img layer-two -1)
  (gimp-image-set-active-layer img layer-two)

  (plug-in-gradmap 1 img layer-two)
  (gimp-image-undo-enable img)
  (gimp-display-new img)
))

(script-fu-register "script-fu-flatland"
		    "<Toolbox>/Xtns/Script-Fu/Patterns/Flatland..."
		    "A Land Pattern"
		    "Adrian Likins <aklikins@eos.ncsu.edu>"
		    "Adrian Likins"
		    "1997"
		    ""
       		    SF-VALUE "Image Width" "256"
		    SF-VALUE "Image Height" "256"
		    SF-VALUE "Random Seed" "80"
		    SF-VALUE "Detail Level" "3.0"
		    SF-VALUE "X Scale" "4"
		    SF-VALUE "Y Scale" "4")




