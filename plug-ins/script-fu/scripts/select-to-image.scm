; GIMP - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; Selection to Image
; Copyright (c) 1997 Adrian Likins
; aklikins@eos.ncsu.edu
;
; Takes the Current selection and saves it as a seperate image.
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
; along with this program.  If not, see <http://www.gnu.org/licenses/>.


(define (script-fu-selection-to-image image drawable)
  (let* (
        (draw-type (car (gimp-drawable-type-with-alpha drawable)))
        (image-type (car (gimp-image-base-type image)))
        (selection-bounds (gimp-selection-bounds image))
        (select-offset-x (cadr selection-bounds))
        (select-offset-y (caddr selection-bounds))
        (selection-width (- (cadr (cddr selection-bounds)) select-offset-x))
        (selection-height (- (caddr (cddr selection-bounds)) select-offset-y))
        (active-selection 0)
        (from-selection 0)
        (new-image 0)
        (new-draw 0)
        )

    (gimp-context-push)

    (gimp-image-undo-disable image)

    (if (= (car (gimp-selection-is-empty image)) TRUE)
        (begin
          (gimp-selection-layer-alpha drawable)
          (set! active-selection (car (gimp-selection-save image)))
          (set! from-selection FALSE)
        )
        (begin
          (set! from-selection TRUE)
          (set! active-selection (car (gimp-selection-save image)))
        )
    )

    (gimp-edit-copy drawable)

    (set! new-image (car (gimp-image-new selection-width
                                         selection-height image-type)))
    (set! new-draw (car (gimp-layer-new new-image
                                        selection-width selection-height
                                        draw-type "Selection" 100 NORMAL-MODE)))
    (gimp-image-insert-layer new-image new-draw -1 0)
    (gimp-drawable-fill new-draw BACKGROUND-FILL)

    (let ((floating-sel (car (gimp-edit-paste new-draw FALSE))))
      (gimp-floating-sel-anchor floating-sel)
    )

    (gimp-image-undo-enable image)
    (gimp-image-set-active-layer image drawable)
    (gimp-display-new new-image)
    (gimp-displays-flush)

    (gimp-context-pop)
  )
)

(script-fu-register "script-fu-selection-to-image"
  _"To _Image"
  _"Convert a selection to an image"
  "Adrian Likins <adrian@gimp.org>"
  "Adrian Likins"
  "10/07/97"
  "RGB* GRAY*"
  SF-IMAGE "Image"       0
  SF-DRAWABLE "Drawable" 0
)
