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
;    3dTruchet  - a script to create Truchet patterns 
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

(define (use-tile img drawable height width img2 drawable2 xoffset yoffset)
  (gimp-edit-copy img2 drawable2)
  (let ((floating-sel (car (gimp-edit-paste img drawable FALSE))))
    (gimp-layer-set-offsets floating-sel xoffset yoffset)
    (gimp-floating-sel-anchor floating-sel)
    )
  )


(define (create-tile img drawable1 drawable2 size thickness backcolor begincolor endcolor supersample)
  (let* (
	 (half-thickness (/ thickness 2))
	 (outer-radius (+ (/ size 2) half-thickness))
	 (inner-radius (- (/ size 2) half-thickness))
	 )

    (gimp-selection-all img)
    (gimp-palette-set-background backcolor)
    (gimp-edit-fill img drawable1)

    (let* (
	   (tempSize (* size 3))
	   (temp-img (car (gimp-image-new tempSize tempSize RGB))) 
	   (temp-draw (car (gimp-layer-new temp-img tempSize tempSize RGB_IMAGE "Jabar" 100 NORMAL)))
      	   (temp-draw2 (car (gimp-layer-new temp-img tempSize tempSize RGB_IMAGE "Jabar" 100 NORMAL))))

      (gimp-image-disable-undo temp-img)
      (gimp-image-add-layer temp-img temp-draw 0)
      (gimp-image-add-layer temp-img temp-draw2 0)
      (gimp-palette-set-background backcolor)
      (gimp-edit-fill temp-img temp-draw)
      (gimp-edit-fill temp-img temp-draw2)

      ;weird aint it
      (gimp-palette-set-background begincolor)
      (gimp-palette-set-foreground endcolor)

      (center-ellipse temp-img size size outer-radius outer-radius REPLACE TRUE FALSE 0)
      (center-ellipse temp-img size size inner-radius inner-radius SUB TRUE FALSE 0)
      
      (center-ellipse temp-img (* size 2) (*  size 2)  outer-radius outer-radius ADD TRUE FALSE 0)
      (center-ellipse temp-img (* size 2) (*  size 2)  inner-radius inner-radius SUB TRUE FALSE 0)
      (gimp-blend temp-img temp-draw FG-BG-RGB NORMAL SHAPEBURST-ANGULAR 100 0 FALSE supersample 3 .2 size size (* size 2) (/ size 2) )

      (center-ellipse temp-img size (* size 2)  outer-radius outer-radius REPLACE TRUE FALSE 0)
      (center-ellipse temp-img size (* size 2) inner-radius inner-radius SUB TRUE FALSE 0)

      (center-ellipse temp-img (* size 2) size  outer-radius outer-radius ADD TRUE FALSE 0)
      (center-ellipse temp-img (* size 2) size  inner-radius inner-radius SUB TRUE FALSE 0)
      ;(gimp-edit-fill temp-img temp-draw2)
      (gimp-blend temp-img temp-draw2 FG-BG-RGB NORMAL SHAPEBURST-ANGULAR 100 0 FALSE supersample 3 .2 size size (* size 2) (* size 2) )
      
      (gimp-selection-none temp-img)

      (gimp-image-resize temp-img size size (- size) (- size))
      ; woo hoo it works....finally...


      (gimp-selection-all temp-img)
      (gimp-edit-copy temp-img temp-draw)
      (let ((floating-sel (car (gimp-edit-paste img drawable2 FALSE))))
	(gimp-floating-sel-anchor floating-sel))

      (gimp-edit-copy temp-img temp-draw2)
      (let ((floating-sel (car (gimp-edit-paste img drawable1 FALSE))))
	(gimp-floating-sel-anchor floating-sel))

      ;(let ((drawble (car (gimp-flip img drawable1 0)))))
	

      ;(gimp-display-new temp-img)
      (gimp-image-delete temp-img)
      )
    )
  )


(define (script-fu-3dtruchet size thickness backcolor begincolor endcolor supersample xtiles ytiles)
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
	 )

    (gimp-image-disable-undo img)
    (gimp-image-disable-undo tile)
    
    (gimp-image-add-layer img layer-one 0)
    (gimp-image-add-layer tile tiledraw1 0)
    (gimp-image-add-layer tile tiledraw2 0)

 
    ;just to look a little better
    (gimp-selection-all img)
    (gimp-palette-set-background backcolor) 
    (gimp-edit-fill img layer-one)
    (gimp-selection-none img)

    (create-tile tile tiledraw1 tiledraw2 size thickness backcolor begincolor endcolor supersample)
    

    (while (<= Xindex xtiles)
	   (while (<= Yindex ytiles)
		  (if (= (rand 2) 0)
		      (use-tile img layer-one height width tile tiledraw1 (* Xindex size) (* Yindex size))
		      (use-tile img layer-one height width tile tiledraw2 (* Xindex size) (* Yindex size))
		      )
		  (set! Yindex (+ Yindex 1))
		  )
	   (set! Yindex 0)
	   (set! Xindex (+ Xindex 1))
	   )
    
    
    (gimp-image-delete tile)
    (gimp-image-enable-undo img)
    (gimp-display-new img)
    )
  )

(script-fu-register "script-fu-3dtruchet"
		    "<Toolbox>/Xtns/Script-Fu/Patterns/3D Truchet"
		    "3D Truchet pattern"
		    "Adrian Likins <aklikins@eos.ncsu.edu>"
		    "Adrian Likins"
		    "1997"
		    ""
		    SF-VALUE "Block Size" "64"
		    SF-VALUE "Thickness" "12"
		    SF-COLOR "Background Color" '(255 255 255)
		    SF-COLOR "Begin Blend" '(0 0 0)
		    SF-COLOR "End Blend" '(255 255 255)
		    SF-VALUE "Supersample" "TRUE"
		    SF-VALUE "Number of Xtiles" "5"
		    SF-VALUE "Number of Ytiles" "5"
		    )

















