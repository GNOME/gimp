
; GIMP - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; FlatLand - creates a tileable pattern that looks like a map
; Copyright (C) 1997 Adrian Likins
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
; FlatLand
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
        (layer-one (car (gimp-layer-new img width height
                                        RGB-IMAGE "bottom" 100 NORMAL-MODE)))
        (layer-two 0)
        )

    (gimp-image-undo-disable img)
    (gimp-image-insert-layer img layer-one 0 0)
   ; (gimp-img-add-layer img layer-two 1)

    (plug-in-solid-noise RUN-NONINTERACTIVE img layer-one 1 0 seed detail xscale yscale )
    (plug-in-c-astretch RUN-NONINTERACTIVE img layer-one)
    (set! layer-two (car (gimp-layer-copy layer-one TRUE)))
    (gimp-image-insert-layer img layer-two 0 -1)
    (gimp-image-set-active-layer img layer-two)

    (plug-in-gradmap RUN-NONINTERACTIVE img layer-two)
    (gimp-image-undo-enable img)
    (gimp-display-new img)
  )
)

(script-fu-register "script-fu-flatland"
  _"_Flatland..."
  _"Create an image filled with a Land Pattern"
  "Adrian Likins <aklikins@eos.ncsu.edu>"
  "Adrian Likins"
  "1997"
  ""
  SF-ADJUSTMENT _"Image width"  '(256 10 2000 1 10 0 1)
  SF-ADJUSTMENT _"Image height" '(256 10 2000 1 10 0 1)
  SF-ADJUSTMENT _"Random seed"  '(80 1 2000000 1 10 0 1)
  SF-ADJUSTMENT _"Detail level" '(3 1 15 1 10 1 0)
  SF-ADJUSTMENT _"Scale X"      '(4 0.1 16 0.1 2 1 1)
  SF-ADJUSTMENT _"Scale Y"      '(4 0.1 16 0.1 2 1 1)
)

(script-fu-menu-register "script-fu-flatland"
                         "<Image>/File/Create/Patterns")
