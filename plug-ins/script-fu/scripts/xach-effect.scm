; LIGMA - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; xach effect script
; Copyright (c) 1997 Adrian Likins
; aklikins@eos.ncsu.edu
;
; based on a idea by Xach Beane <xach@mint.net>
;
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


(define (script-fu-xach-effect image
                               drawable
                               hl-offset-x
                               hl-offset-y
                               hl-color
                               hl-opacity-comp
                               ds-color
                               ds-opacity
                               ds-blur
                               ds-offset-x
                               ds-offset-y
                               keep-selection)
  (let* (
        (ds-blur (max ds-blur 0))
        (ds-opacity (min ds-opacity 100))
        (ds-opacity (max ds-opacity 0))
        (type (car (ligma-drawable-type-with-alpha drawable)))
        (image-width (car (ligma-image-get-width image)))
        (hl-opacity (list hl-opacity-comp hl-opacity-comp hl-opacity-comp))
        (image-height (car (ligma-image-get-height image)))
        (active-selection 0)
        (from-selection 0)
        (theLayer 0)
        (hl-layer 0)
        (shadow-layer 0)
        (mask 0)
        )

    (ligma-context-push)
    (ligma-context-set-defaults)

    (ligma-image-undo-group-start image)
    (ligma-layer-add-alpha drawable)

    (if (= (car (ligma-selection-is-empty image)) TRUE)
        (begin
          (ligma-image-select-item image CHANNEL-OP-REPLACE drawable)
          (set! active-selection (car (ligma-selection-save image)))
          (set! from-selection FALSE))
        (begin
          (set! from-selection TRUE)
          (set! active-selection (car (ligma-selection-save image)))))

    (set! hl-layer (car (ligma-layer-new image image-width image-height type _"Highlight" 100 LAYER-MODE-NORMAL)))
    (ligma-image-insert-layer image hl-layer 0 -1)

    (ligma-selection-none image)
    (ligma-drawable-edit-clear hl-layer)
    (ligma-image-select-item image CHANNEL-OP-REPLACE active-selection)

    (ligma-context-set-background hl-color)
    (ligma-drawable-edit-fill hl-layer FILL-BACKGROUND)
    (ligma-selection-translate image hl-offset-x hl-offset-y)
    (ligma-drawable-edit-fill hl-layer FILL-BACKGROUND)
    (ligma-selection-none image)
    (ligma-image-select-item image CHANNEL-OP-REPLACE active-selection)

    (set! mask (car (ligma-layer-create-mask hl-layer ADD-MASK-WHITE)))
    (ligma-layer-add-mask hl-layer mask)

    (ligma-context-set-background hl-opacity)
    (ligma-drawable-edit-fill mask FILL-BACKGROUND)

    (set! shadow-layer (car (ligma-layer-new image
                                            image-width
                                            image-height
                                            type
                                            _"Shadow"
                                            ds-opacity
                                            LAYER-MODE-NORMAL)))
    (ligma-image-insert-layer image shadow-layer 0 -1)
    (ligma-selection-none image)
    (ligma-drawable-edit-clear shadow-layer)
    (ligma-image-select-item image CHANNEL-OP-REPLACE active-selection)
    (ligma-selection-translate image ds-offset-x ds-offset-y)
    (ligma-context-set-background ds-color)
    (ligma-drawable-edit-fill shadow-layer FILL-BACKGROUND)
    (ligma-selection-none image)
    (plug-in-gauss-rle RUN-NONINTERACTIVE image shadow-layer ds-blur TRUE TRUE)
    (ligma-image-select-item image CHANNEL-OP-REPLACE active-selection)
    (ligma-drawable-edit-clear shadow-layer)
    (ligma-image-lower-item image shadow-layer)

    (if (= keep-selection FALSE)
        (ligma-selection-none image))

    (ligma-image-set-selected-layers image 1 (vector drawable))
    (ligma-image-remove-channel image active-selection)
    (ligma-image-undo-group-end image)
    (ligma-displays-flush)

    (ligma-context-pop)
  )
)

(script-fu-register "script-fu-xach-effect"
  _"_Xach-Effect..."
  _"Add a subtle translucent 3D effect to the selected region (or alpha)"
  "Adrian Likins <adrian@ligma.org>"
  "Adrian Likins"
  "9/28/97"
  "RGB* GRAY*"
  SF-IMAGE       "Image"                   0
  SF-DRAWABLE    "Drawable"                0
  SF-ADJUSTMENT _"Highlight X offset"      '(-1 -100 100 1 10 0 1)
  SF-ADJUSTMENT _"Highlight Y offset"      '(-1 -100 100 1 10 0 1)
  SF-COLOR      _"Highlight color"         "white"
  SF-ADJUSTMENT _"Highlight opacity"       '(66 0 255 1 10 0 0)
  SF-COLOR      _"Drop shadow color"       "black"
  SF-ADJUSTMENT _"Drop shadow opacity"     '(100 0 100 1 10 0 0)
  SF-ADJUSTMENT _"Drop shadow blur radius" '(12 0 255 1 10 0 1)
  SF-ADJUSTMENT _"Drop shadow X offset"    '(5 0 255 1 10 0 1)
  SF-ADJUSTMENT _"Drop shadow Y offset"    '(5 0 255 1 10 0 1)
  SF-TOGGLE     _"Keep selection"          TRUE
)

(script-fu-menu-register "script-fu-xach-effect"
                         "<Image>/Filters/Light and Shadow/Shadow")
