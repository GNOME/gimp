; GIMP - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; Lava effect
; Copyright (c) 1997 Adrian Likins
; aklikins@eos.ncsu.edu
;
; based on a idea by Sven Riedel <lynx@heim8.tu-clausthal.de>
; tweaked a bit by Sven Neumann <neumanns@uni-duesseldorf.de>
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


(define (script-fu-lava image
                        drawable
                        seed
                        tile_size
                        mask_size
                        gradient
                        keep-selection
                        separate-layer
                        current-grad)
  (let* (
        (type (car (gimp-drawable-type-with-alpha drawable)))
        (image-width (car (gimp-image-width image)))
        (image-height (car (gimp-image-height image)))
        (active-selection 0)
        (selection-bounds 0)
        (select-offset-x 0)
        (select-offset-y 0)
        (select-width 0)
        (select-height 0)
        (lava-layer 0)
        (active-layer 0)
        )

    (gimp-context-push)
    (gimp-image-undo-group-start image)

    (if (= (car (gimp-drawable-has-alpha drawable)) FALSE)
        (gimp-layer-add-alpha drawable)
    )

    (if (= (car (gimp-selection-is-empty image)) TRUE)
        (gimp-selection-layer-alpha drawable)
    )

    (set! active-selection (car (gimp-selection-save image)))
    (gimp-image-set-active-layer image drawable)

    (set! selection-bounds (gimp-selection-bounds image))
    (set! select-offset-x (cadr selection-bounds))
    (set! select-offset-y (caddr selection-bounds))
    (set! select-width (- (cadr (cddr selection-bounds)) select-offset-x))
    (set! select-height (- (caddr (cddr selection-bounds)) select-offset-y))

    (if (= separate-layer TRUE)
        (begin
          (set! lava-layer (car (gimp-layer-new image
                                                select-width
                                                select-height
                                                type
                                                "Lava Layer"
                                                100
                                                NORMAL-MODE)))

          (gimp-image-insert-layer image lava-layer 0 -1)
          (gimp-layer-set-offsets lava-layer select-offset-x select-offset-y)
          (gimp-selection-none image)
          (gimp-edit-clear lava-layer)

          (gimp-selection-load active-selection)
          (gimp-image-set-active-layer image lava-layer)
        )
    )

    (set! active-layer (car (gimp-image-get-active-layer image)))

    (if (= current-grad FALSE)
        (gimp-context-set-gradient gradient)
    )

    (plug-in-solid-noise RUN-NONINTERACTIVE image active-layer FALSE TRUE seed 2 2 2)
    (plug-in-cubism RUN-NONINTERACTIVE image active-layer tile_size 2.5 0)
    (plug-in-oilify RUN-NONINTERACTIVE image active-layer mask_size 0)
    (plug-in-edge RUN-NONINTERACTIVE image active-layer 2 0 0)
    (plug-in-gauss-rle RUN-NONINTERACTIVE image active-layer 2 TRUE TRUE)
    (plug-in-gradmap RUN-NONINTERACTIVE image active-layer)

    (if (= keep-selection FALSE)
        (gimp-selection-none image)
    )

    (gimp-image-set-active-layer image drawable)
    (gimp-image-remove-channel image active-selection)

    (gimp-image-undo-group-end image)
    (gimp-context-pop)

    (gimp-displays-flush)
  )
)

(script-fu-register "script-fu-lava"
  _"_Lava..."
  _"Fill the current selection with lava"
  "Adrian Likins <adrian@gimp.org>"
  "Adrian Likins"
  "10/12/97"
  "RGB* GRAY*"
  SF-IMAGE       "Image"          0
  SF-DRAWABLE    "Drawable"       0
  SF-ADJUSTMENT _"Seed"           '(10 1 30000 1 10 0 1)
  SF-ADJUSTMENT _"Size"           '(10 0 100 1 10 0 1)
  SF-ADJUSTMENT _"Roughness"      '(7 3 50 1 10 0 0)
  SF-GRADIENT   _"Gradient"       "German flag smooth"
  SF-TOGGLE     _"Keep selection" TRUE
  SF-TOGGLE     _"Separate layer" TRUE
  SF-TOGGLE     _"Use current gradient" FALSE
)

(script-fu-menu-register "script-fu-lava"
                         "<Image>/Filters/Render")
