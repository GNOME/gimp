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

; Resizes the image so as to include the selected layer.
; The resulting image has the selected layer size.
; Copyright (C) 2002 Chauk-Mean PROUM
;
(define (script-fu-util-image-resize-from-layer image layer)
  (let* (
        (width (car (gimp-drawable-width layer)))
        (height (car (gimp-drawable-height layer)))
        (posx (- (car (gimp-drawable-offsets layer))))
        (posy (- (cadr (gimp-drawable-offsets layer))))
        )

    (gimp-image-resize image width height posx posy)
  )
)

; Add the specified layers to the image.
; The layers will be added in the given order below the
; active layer.
;
(define (script-fu-util-image-add-layers image . layers)
  (while (not (null? layers))
    (let ((layer (car layers)))
      (set! layers (cdr layers))
      (gimp-image-insert-layer image layer 0 -1)
      (gimp-image-lower-layer image layer)
    )
  )
)

