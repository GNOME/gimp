; GIMP - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; Alien Glow themed bullets for web pages
; Copyright (c) 1997 Adrian Likins
; aklikins@eos.ncsu.edu
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

(define (script-fu-alien-glow-bullet radius
                                     glow-color
                                     bg-color
                                     flatten)

  (define (center-ellipse img cx cy rx ry op aa feather frad)
    (gimp-ellipse-select img (- cx rx) (- cy ry) (+ rx rx) (+ ry ry)
                         op aa feather frad)
  )


  (let* (
        (img (car (gimp-image-new radius radius RGB)))
        (border (/ radius 4))
        (diameter (* radius 2))
        (half-radius (/ radius 2))
        (blend-start (+ half-radius (/ half-radius 2)))
        (bullet-layer (car (gimp-layer-new img
                                           diameter diameter RGBA-IMAGE
                                           _"Bullet" 100 NORMAL-MODE)))
        (glow-layer (car (gimp-layer-new img diameter diameter RGBA-IMAGE
                                         _"Alien Glow" 100 NORMAL-MODE)))
        (bg-layer (car (gimp-layer-new img diameter diameter RGB-IMAGE
                                       _"Background" 100 NORMAL-MODE)))
        )

    (gimp-context-push)

    (gimp-image-undo-disable img)
    (gimp-image-resize img diameter diameter 0 0)
    (gimp-image-insert-layer img bg-layer -1 1)
    (gimp-image-insert-layer img glow-layer -1 -1)
    (gimp-image-insert-layer img bullet-layer -1 -1)

    ; (gimp-layer-set-lock-alpha ruler-layer TRUE)
    (gimp-context-set-background bg-color)
    (gimp-edit-fill bg-layer BACKGROUND-FILL)
    (gimp-edit-clear glow-layer)
    (gimp-edit-clear bullet-layer)

    (center-ellipse img radius radius half-radius half-radius
                    CHANNEL-OP-REPLACE TRUE FALSE 0)

    ; (gimp-rect-select img (/ height 2) (/ height 2) length height CHANNEL-OP-REPLACE FALSE 0)
    (gimp-context-set-foreground '(90 90 90))
    (gimp-context-set-background '(0 0 0))

    (gimp-edit-blend bullet-layer FG-BG-RGB-MODE NORMAL-MODE
                     GRADIENT-RADIAL 100 0 REPEAT-NONE FALSE
                     FALSE 0 0 TRUE
                     blend-start blend-start
                     (+ half-radius radius) (+ half-radius radius))

    (gimp-context-set-foreground glow-color)
    (gimp-selection-grow img border)
    (gimp-selection-feather img  border)
    (gimp-edit-fill glow-layer FOREGROUND-FILL)
    (gimp-selection-none img)
    (if (>= radius 16)
        (plug-in-gauss-rle RUN-NONINTERACTIVE img glow-layer 25 TRUE TRUE)
        (plug-in-gauss-rle RUN-NONINTERACTIVE img glow-layer 12 TRUE TRUE)
    )

    (if (= flatten TRUE)
        (gimp-image-flatten img)
    )
    (gimp-image-undo-enable img)
    (gimp-display-new img)

    (gimp-context-pop)
  )
)

(script-fu-register "script-fu-alien-glow-bullet"
  _"_Bullet..."
  _"Create a bullet graphic with an eerie glow for web pages"
  "Adrian Likins"
  "Adrian Likins"
  "1997"
  ""
  SF-ADJUSTMENT _"Radius"           '(16 1 100 1 10 0 1)
  SF-COLOR      _"Glow color"       '(63 252 0)
  SF-COLOR      _"Background color" "black"
  SF-TOGGLE     _"Flatten image"    TRUE
)

(script-fu-menu-register "script-fu-alien-glow-bullet"
                         "<Image>/File/Create/Web Page Themes/Alien Glow")
