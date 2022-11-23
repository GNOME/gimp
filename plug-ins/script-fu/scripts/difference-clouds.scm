; Plugin for the GNU Image Manipulation Program
; Copyright (C) 2006 Martin Nordholts
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
;
; Renders Difference Clouds onto a layer, i.e. solid noise merged down with the
; Difference Mode
;

(define (script-fu-difference-clouds image
                                     drawable)

  (let* ((draw-offset-x (car (ligma-drawable-get-offsets drawable)))
         (draw-offset-y (cadr (ligma-drawable-get-offsets drawable)))
         (has-sel       (car (ligma-drawable-mask-intersect drawable)))
         (sel-offset-x  (cadr (ligma-drawable-mask-intersect drawable)))
         (sel-offset-y  (caddr (ligma-drawable-mask-intersect drawable)))
         (width         (cadddr (ligma-drawable-mask-intersect drawable)))
         (height        (caddr (cddr (ligma-drawable-mask-intersect drawable))))
         (type          (car (ligma-drawable-type-with-alpha drawable)))
         (diff-clouds  -1)
         (offset-x      0)
         (offset-y      0)
        )

    (ligma-image-undo-group-start image)

    ; Create the cloud layer
    (set! diff-clouds (car (ligma-layer-new image width height type
                                             "Clouds" 100 LAYER-MODE-DIFFERENCE)))

    ; Add the cloud layer above the current layer
    (ligma-image-insert-layer image diff-clouds 0 -1)

    ; Clear the layer (so there are no noise in it)
    (ligma-drawable-fill diff-clouds FILL-TRANSPARENT)

    ; Selections are relative to the drawable; adjust the final offset
    (set! offset-x (+ draw-offset-x sel-offset-x))
    (set! offset-y (+ draw-offset-y sel-offset-y))

    ; Offset the clouds layer
    (if (ligma-item-is-layer drawable)
      (ligma-item-transform-translate diff-clouds offset-x offset-y))

    ; Show the solid noise dialog
    (plug-in-solid-noise SF-RUN-MODE image diff-clouds 0 0 0 1 4.0 4.0)

    ; Merge the clouds layer with the layer below
    (ligma-image-merge-down image diff-clouds EXPAND-AS-NECESSARY)

    (ligma-image-undo-group-end image)

    (ligma-displays-flush)
  )
)

(script-fu-register "script-fu-difference-clouds"
                    _"_Difference Clouds..."
                    _"Solid noise applied with Difference layer mode"
                    "Martin Nordholts <enselic@hotmail.com>"
                    "Martin Nordholts"
                    "2006/10/25"
                    "RGB* GRAY*"
                    SF-IMAGE       "Image"           0
                    SF-DRAWABLE    "Drawable"        0)

(script-fu-menu-register "script-fu-difference-clouds"
			 "<Image>/Filters/Render/Noise")
