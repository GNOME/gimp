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
; along with this program.  If not, see <https://www.gnu.org/licenses/>.


(define (script-fu-lava image
                        drawables
                        seed
                        tile_size
                        mask_size
                        gradient
                        keep-selection
                        separate-layer
                        current-grad)
  (let* (
        (first-layer (vector-ref drawables 0))
        (type (car (gimp-drawable-type-with-alpha first-layer)))
        (image-width (car (gimp-image-get-width image)))
        (image-height (car (gimp-image-get-height image)))
        (active-selection 0)
        (selection-bounds 0)
        (select-offset-x 0)
        (select-offset-y 0)
        (select-width 0)
        (select-height 0)
        (lava-layer 0)
        (active-layer 0)
        (selected-layers-array (car (gimp-image-get-selected-layers image)))
        (num-selected-layers (vector-length selected-layers-array))
        )

    (if (= num-selected-layers 1)
        (begin
            (gimp-context-push)
            (gimp-context-set-defaults)
            (gimp-image-undo-group-start image)

            (if (= (car (gimp-drawable-has-alpha first-layer)) FALSE)
                (gimp-layer-add-alpha first-layer)
            )

            (if (= (car (gimp-selection-is-empty image)) TRUE)
                (gimp-image-select-item image CHANNEL-OP-REPLACE first-layer)
            )

            (set! active-selection (car (gimp-selection-save image)))
            (gimp-image-set-selected-layers image (make-vector 1 first-layer))

            (set! selection-bounds (gimp-selection-bounds image))
            (set! select-offset-x (cadr selection-bounds))
            (set! select-offset-y (caddr selection-bounds))
            (set! select-width (- (cadr (cddr selection-bounds)) select-offset-x))
            (set! select-height (- (caddr (cddr selection-bounds)) select-offset-y))

            (if (= separate-layer TRUE)
                (begin
                  (set! lava-layer (car (gimp-layer-new image
                                                        "Lava Layer"
                                                        select-width
                                                        select-height
                                                        type
                                                        100
                                                        LAYER-MODE-NORMAL-LEGACY)))

                  (gimp-image-insert-layer image lava-layer 0 -1)
                  (gimp-layer-set-offsets lava-layer select-offset-x select-offset-y)
                  (gimp-selection-none image)
                  (gimp-drawable-edit-clear lava-layer)

                  (gimp-image-select-item image CHANNEL-OP-REPLACE active-selection)
                  (gimp-image-set-selected-layers image (make-vector 1 lava-layer))
                )
            )

            (set! selected-layers-array (car (gimp-image-get-selected-layers image)))
            (set! num-selected-layers (vector-length selected-layers-array))
            (set! active-layer (vector-ref selected-layers-array (- num-selected-layers 1)))

            (if (= current-grad FALSE)
                (gimp-context-set-gradient gradient)
            )

            (let* ((width  (cadddr (gimp-drawable-mask-intersect active-layer)))
                   (height (caddr (cddr (gimp-drawable-mask-intersect active-layer)))))
              (gimp-drawable-merge-new-filter active-layer "gegl:noise-solid" 0 LAYER-MODE-REPLACE 1.0 "tileable" FALSE "turbulent" TRUE "seed" seed
                                                                                                       "detail" 2 "x-size" 2.0 "y-size" 2.0
                                                                                                       "width" width "height" height)
            )
            (gimp-drawable-merge-new-filter active-layer "gegl:cubism" 0 LAYER-MODE-REPLACE 1.0 "tile-size" tile_size "tile-saturation" 2.5 "bg-color" '(0 0 0))
            (gimp-drawable-merge-new-filter active-layer "gegl:oilify" 0 LAYER-MODE-REPLACE 1.0 "mask-radius" (max 1 (/ mask_size 2)) "use-inten" FALSE)
            (gimp-drawable-merge-new-filter active-layer "gegl:edge" 0 LAYER-MODE-REPLACE 1.0 "amount" 2.0 "border-behavior" "none" "algorithm" "sobel")
            (gimp-drawable-merge-new-filter active-layer "gegl:gaussian-blur" 0 LAYER-MODE-REPLACE 1.0 "std-dev-x" 0.64 "std-dev-y" 0.64 "filter" "auto")
            (plug-in-gradmap #:run-mode RUN-NONINTERACTIVE #:image image #:drawables selected-layers-array)

            (if (= keep-selection FALSE)
                (gimp-selection-none image)
            )

            (gimp-image-set-selected-layers image (make-vector 1 first-layer))
            (gimp-image-remove-channel image active-selection)

            (gimp-image-undo-group-end image)
            (gimp-context-pop)

            (gimp-displays-flush)
        )
    ; else
        (gimp-message _"Lava works with exactly one selected layer")
    )
  )
)

(script-fu-register-filter "script-fu-lava"
  _"_Lava..."
  _"Fill the current selection with lava"
  "Adrian Likins <adrian@gimp.org>"
  "Adrian Likins"
  "10/12/97"
  "RGB* GRAY*"
  SF-ONE-OR-MORE-DRAWABLE
  SF-ADJUSTMENT _"Seed"           '(10 1 30000 1 10 0 1)
  SF-ADJUSTMENT _"Size"           '(10 0 100 1 10 0 1)
  SF-ADJUSTMENT _"Roughness"      '(7 3 50 1 10 0 0)
  SF-GRADIENT   _"Gradient"       "Incandescent"
  SF-TOGGLE     _"Keep selection" TRUE
  SF-TOGGLE     _"Separate layer" TRUE
  SF-TOGGLE     _"Use current gradient" FALSE
)

(script-fu-menu-register "script-fu-lava"
                         "<Image>/Filters/Render")
