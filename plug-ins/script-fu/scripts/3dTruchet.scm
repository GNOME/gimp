; GIMP - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

(define (center-ellipse img
                        cx
                        cy
                        rx
                        ry
                        op
                        aa
                        feather
                        frad)
  (gimp-ellipse-select img (- cx rx) (- cy ry) (+ rx rx) (+ ry ry)
                       op aa feather frad)
)

(define (use-tile img
                  drawable
                  height
                  width
                  img2
                  drawable2
                  xoffset
                  yoffset)
  (gimp-edit-copy drawable2)
  (let (
       (floating-sel (car (gimp-edit-paste drawable FALSE)))
       )
    (gimp-layer-set-offsets floating-sel xoffset yoffset)
    (gimp-floating-sel-anchor floating-sel)
  )
)


(define (create-tile img
                     drawable1
                     drawable2
                     size
                     thickness
                     backcolor
                     begincolor
                     endcolor
                     supersample)
  (let* (
        (half-thickness (/ thickness 2))
        (outer-radius (+ (/ size 2) half-thickness))
        (inner-radius (- (/ size 2) half-thickness))
        )

    (gimp-selection-all img)
    (gimp-context-set-background backcolor)
    (gimp-edit-fill drawable1 BACKGROUND-FILL)

    (let* (
          (tempSize (* size 3))
          (temp-img (car (gimp-image-new tempSize tempSize RGB)))
          (temp-draw (car (gimp-layer-new temp-img tempSize tempSize
                                          RGB-IMAGE "Jabar" 100 NORMAL-MODE)))
          (temp-draw2 (car (gimp-layer-new temp-img tempSize tempSize
                                          RGB-IMAGE "Jabar" 100 NORMAL-MODE)))
          )

      (gimp-image-undo-disable temp-img)
      (gimp-image-insert-layer temp-img temp-draw -1 0)
      (gimp-image-insert-layer temp-img temp-draw2 -1 0)
      (gimp-context-set-background backcolor)
      (gimp-edit-fill temp-draw BACKGROUND-FILL)
      (gimp-edit-fill temp-draw2 BACKGROUND-FILL)

      ;weird aint it
      (gimp-context-set-background begincolor)
      (gimp-context-set-foreground endcolor)

      (center-ellipse temp-img size size outer-radius outer-radius
                      CHANNEL-OP-REPLACE TRUE FALSE 0)
      (center-ellipse temp-img size size inner-radius inner-radius
                      CHANNEL-OP-SUBTRACT TRUE FALSE 0)

      (center-ellipse temp-img (* size 2) (*  size 2)  outer-radius outer-radius
                      CHANNEL-OP-ADD TRUE FALSE 0)
      (center-ellipse temp-img (* size 2) (*  size 2)  inner-radius inner-radius
                      CHANNEL-OP-SUBTRACT TRUE FALSE 0)

      (gimp-edit-blend temp-draw FG-BG-RGB-MODE NORMAL-MODE
                       GRADIENT-SHAPEBURST-ANGULAR 100 0 REPEAT-NONE FALSE
                       supersample 3 0.2 TRUE
                       size size (* size 2) (/ size 2))

      (center-ellipse temp-img size (* size 2)  outer-radius outer-radius
                      CHANNEL-OP-REPLACE TRUE FALSE 0)
      (center-ellipse temp-img size (* size 2) inner-radius inner-radius
                      CHANNEL-OP-SUBTRACT TRUE FALSE 0)

      (center-ellipse temp-img (* size 2) size  outer-radius outer-radius
                      CHANNEL-OP-ADD TRUE FALSE 0)
      (center-ellipse temp-img (* size 2) size  inner-radius inner-radius
                      CHANNEL-OP-SUBTRACT TRUE FALSE 0)

      ;(gimp-edit-fill temp-img temp-draw2 BACKGROUND-FILL)

      (gimp-edit-blend temp-draw2 FG-BG-RGB-MODE NORMAL-MODE
                       GRADIENT-SHAPEBURST-ANGULAR 100 0 REPEAT-NONE FALSE
                       supersample 3 0.2 TRUE
                       size size (* size 2) (* size 2))

      (gimp-selection-none temp-img)

      (gimp-image-resize temp-img size size (- size) (- size))
      ; woo hoo it works....finally...


      (gimp-selection-all temp-img)
      (gimp-edit-copy temp-draw)
      (let ((floating-sel (car (gimp-edit-paste drawable2 FALSE))))
        (gimp-floating-sel-anchor floating-sel))

      (gimp-edit-copy temp-draw2)
      (let ((floating-sel (car (gimp-edit-paste drawable1 FALSE))))
        (gimp-floating-sel-anchor floating-sel))

      ;(let ((drawble (car (gimp-drawable-transform-flip-simple img drawable1
      ;                     ORIENTATION-HORIZONTAL
      ;                     TRUE 0 TRUE)))))


      ;(gimp-display-new temp-img)
      (gimp-image-delete temp-img)
    )
  )
)


(define (script-fu-3dtruchet size
                             thickness
                             backcolor
                             begincolor
                             endcolor
                             supersample
                             xtiles
                             ytiles)
  (let* (
        (width (* size xtiles))
        (height (* size ytiles))
        (img (car (gimp-image-new width height RGB)))
        (tile (car (gimp-image-new size size RGB)))
        (layer-one (car (gimp-layer-new img width height
                                        RGB-IMAGE "Rambis" 100 NORMAL-MODE)))
        (tiledraw1 (car (gimp-layer-new tile size size
                                        RGB-IMAGE "Johnson" 100 NORMAL-MODE)))
        (tiledraw2 (car (gimp-layer-new tile size size
                                        RGB-IMAGE "Cooper" 100 NORMAL-MODE)))
        (Xindex 0)
        (Yindex 0)
        )

    (gimp-context-push)

    (gimp-image-undo-disable img)
    (gimp-image-undo-disable tile)

    (gimp-image-insert-layer img layer-one -1 0)
    (gimp-image-insert-layer tile tiledraw1 -1 0)
    (gimp-image-insert-layer tile tiledraw2 -1 0)

    ;just to look a little better
    (gimp-selection-all img)
    (gimp-context-set-background backcolor)
    (gimp-edit-fill layer-one BACKGROUND-FILL)
    (gimp-selection-none img)

    (create-tile tile tiledraw1 tiledraw2 size thickness
                 backcolor begincolor endcolor supersample)


    (while (<= Xindex xtiles)
      (while (<= Yindex ytiles)
        (if (= (rand 2) 0)
            (use-tile img layer-one height width tile
                      tiledraw1 (* Xindex size) (* Yindex size))
            (use-tile img layer-one height width tile
                      tiledraw2 (* Xindex size) (* Yindex size))
        )
        (set! Yindex (+ Yindex 1))
      )
      (set! Yindex 0)
      (set! Xindex (+ Xindex 1))
    )

    (gimp-image-delete tile)
    (gimp-image-undo-enable img)
    (gimp-display-new img)

    (gimp-context-pop)
  )
)

(script-fu-register "script-fu-3dtruchet"
  _"3_D Truchet..."
  _"Create an image filled with a 3D Truchet pattern"
  "Adrian Likins <aklikins@eos.ncsu.edu>"
  "Adrian Likins"
  "1997"
  ""
  SF-ADJUSTMENT _"Block size"        '(64 5 1000 1 10 0 1)
  SF-ADJUSTMENT _"Thickness"         '(12 2 100 1 10 0 1)
  SF-COLOR      _"Background color"  "white"
  SF-COLOR      _"Start blend"       "black"
  SF-COLOR      _"End blend"         "white"
  SF-TOGGLE     _"Supersample"       TRUE
  SF-ADJUSTMENT _"Number of X tiles" '(5 1 1000 1 10 0 1)
  SF-ADJUSTMENT _"Number of Y tiles" '(5 1 1000 1 10 0 1)
)

(script-fu-menu-register "script-fu-3dtruchet"
                         "<Image>/File/Create/Patterns")
