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
;    Truchet  - a script to create Truchet patterns 
;                 by Adrian Likins <aklikins@eos.ncsu.edu>
;                 http://www4.ncsu.edu/~aklikins/
;    version about .8 give or take
;
;  Lots of thanks to Quartic for his help.
;
;
;         The utility of this script is left as an exercise for the reader.

(define (center-ellipse img cx cy rx ry op aa feather frad)
  (gimp-ellipse-select img (- cx rx) (- cy ry) (+ rx rx ) (+ ry ry ) op aa feather frad))

(define (use-tiles img drawable height width img2 drawable2 xoffset yoffset)
  (gimp-edit-copy drawable2)
  (let ((floating-sel (car (gimp-edit-paste drawable FALSE))))
    (gimp-layer-set-offsets floating-sel xoffset yoffset)
    (gimp-floating-sel-anchor floating-sel)
    )
)


(define (create-tiles img drawable1 drawable2 size thickness backcolor forecolor)
  (let* (
	 (half-thickness (/ thickness 2))
	 (outer-radius (+ (/ size 2) half-thickness))
	 (inner-radius (- (/ size 2) half-thickness))
	 )

    (gimp-selection-all img)
    (gimp-palette-set-background backcolor)
    (gimp-edit-fill drawable1)

    (let* (
	   (tempSize (* size 3))
	   (temp-img (car (gimp-image-new tempSize tempSize RGB)))
	   (temp-draw (car (gimp-layer-new temp-img tempSize tempSize RGB_IMAGE "Jabar" 100 NORMAL)))
	  )
      (gimp-image-undo-disable temp-img)
      (gimp-image-add-layer temp-img temp-draw 0)
      (gimp-palette-set-background backcolor)
      (gimp-edit-fill temp-draw)
      
      
      (center-ellipse temp-img size size outer-radius outer-radius REPLACE TRUE FALSE 0)
      (center-ellipse temp-img size size inner-radius inner-radius SUB TRUE FALSE 0)
      
      (center-ellipse temp-img (* size 2) (*  size 2)  outer-radius outer-radius ADD TRUE FALSE 0)
      (center-ellipse temp-img (* size 2) (*  size 2)  inner-radius inner-radius SUB TRUE FALSE 0)
      (gimp-palette-set-background forecolor)
      (gimp-edit-fill temp-draw)
      
      (gimp-selection-none temp-img)

      (gimp-image-resize temp-img size size (- size) (- size))
      ; woo hoo it works....finally...


      (gimp-selection-all temp-img)
      (gimp-edit-copy temp-draw)
      (let ((floating-sel (car (gimp-edit-paste drawable2 FALSE))))
	(gimp-floating-sel-anchor floating-sel))

      
      (let ((floating-sel (car (gimp-edit-paste drawable1 FALSE))))
	(gimp-floating-sel-anchor floating-sel))

      (let ((drawble (car (gimp-flip drawable1 1)))))
	

      ;(gimp-display-new temp-img)
      (gimp-image-delete temp-img)
      )
    )
)


(define (script-fu-truchet size thickness backcolor forecolor xtiles ytiles)
  (let* (
	 (width (* size xtiles))
	 (height (* size ytiles))
	 (img (car (gimp-image-new width height RGB)))
	 (tile (car (gimp-image-new size size RGB)))
	 (layer-one (car (gimp-layer-new img width height RGB "Rambis" 100 NORMAL)))
	 (tiledraw1 (car (gimp-layer-new tile size size RGB "Johnson" 100 NORMAL)))
	 (tiledraw2 (car (gimp-layer-new tile size size RGB "Cooper" 100 NORMAL)))
	 (Xindex 0)
	 (Yindex 0)
	 (old-bg (car (gimp-palette-get-background)))
	 )

    (gimp-image-undo-disable img)
    (gimp-image-undo-disable tile)
    
    (gimp-image-add-layer img layer-one 0)
    (gimp-image-add-layer tile tiledraw1 0)
    (gimp-image-add-layer tile tiledraw2 0)

 
    ;just to look a little better
    (gimp-selection-all img)
    (gimp-palette-set-background backcolor)
    (gimp-edit-fill layer-one)
    (gimp-selection-none img)

    (create-tiles tile tiledraw1 tiledraw2 size thickness backcolor forecolor)
    

    (while (<= Xindex xtiles)
	   (while (<= Yindex ytiles)
		  (if (= (rand 2) 0)
		      (use-tiles img layer-one height width tile tiledraw1 (* Xindex size) (* Yindex size))
		      (use-tiles img layer-one height width tile tiledraw2 (* Xindex size) (* Yindex size))
		      )
		  (set! Yindex (+ Yindex 1))
		  )
	   (set! Yindex 0)
	   (set! Xindex (+ Xindex 1))
	   )
    
    
    (gimp-image-delete tile)
    (gimp-palette-set-background old-bg)
    (gimp-image-undo-enable img)
    (gimp-display-new img)
    )
  )

(script-fu-register "script-fu-truchet"
		    "<Toolbox>/Xtns/Script-Fu/Patterns/Truchet..."
		    "Create a Truchet pattern \n\n Works best with even sized thicknesses"
		    "Adrian Likins <aklikins@eos.ncsu.edu>"
		    "Adrian Likins"
		    "1997"
		    ""

		    SF-ADJUSTMENT "Block Size" '(32 2 512 1 10 1 1)
		    SF-ADJUSTMENT "Thickness" '(2 1 512 1 10 1 1)
		    SF-COLOR "Background Color" '(255 255 255)
		    SF-COLOR "Foreground Color" '(0 0 0)
		    SF-ADJUSTMENT "Number of Xtiles" '(5 1 512 1 10 1 1)
		    SF-ADJUSTMENT "Number of Ytile" '(5 1 512 1 10 1 1)
		    )

















