; GIMP - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; Based on select-to-brush by
;         Copyright (c) 1997 Adrian Likins aklikins@eos.ncsu.edu
; Author Cameron Gregory, http://www.flamingtext.com/
;
; Takes the current selection, saves it as a pattern and makes it the active
; pattern
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


(define (script-fu-selection-to-pattern image drawable desc filename)

  (let* (
        (selection-width 0)
        (selection-height 0)
        (selection-bounds 0)
        (select-offset-x 0)
        (select-offset-y 0)
        (pattern-draw-type 0)
        (pattern-image-type 0)
        (pattern-image 0)
        (pattern-draw 0)
        (filename2 0)
        )

  (if (= (car (gimp-selection-is-empty image)) TRUE)
      (begin
        (set! selection-width (car (gimp-drawable-width drawable)))
        (set! selection-height (car (gimp-drawable-height drawable)))
      )
      (begin
        (set! selection-bounds (gimp-drawable-mask-bounds drawable))
        (set! select-offset-x (cadr selection-bounds))
        (set! select-offset-y (caddr selection-bounds))
        (set! selection-width (- (cadr (cddr selection-bounds)) select-offset-x))
        (set! selection-height (- (caddr (cddr selection-bounds)) select-offset-y))
      )
  )

  (if (= (car (gimp-drawable-has-alpha drawable)) TRUE)
      (set! pattern-draw-type RGBA-IMAGE)
      (set! pattern-draw-type RGB-IMAGE)
  )

  (set! pattern-image-type RGB)

  (set! pattern-image (car (gimp-image-new selection-width selection-height
                                           pattern-image-type)))

  (set! pattern-draw
        (car (gimp-layer-new pattern-image selection-width selection-height
                             pattern-draw-type "Pattern" 100 NORMAL-MODE)))

  (gimp-drawable-fill pattern-draw TRANSPARENT-FILL)

  (gimp-image-insert-layer pattern-image pattern-draw 0 0)

  (gimp-edit-copy drawable)

  (let ((floating-sel (car (gimp-edit-paste pattern-draw FALSE))))
    (gimp-floating-sel-anchor floating-sel))

  (set! filename2 (string-append gimp-directory
                                 "/patterns/"
                                 filename
                                 (number->string image)
                                 ".pat"))

  (file-pat-save 1 pattern-image pattern-draw filename2 "" desc)
  (gimp-patterns-refresh)
  (gimp-context-set-pattern desc)

  (gimp-image-delete pattern-image)
  (gimp-displays-flush)
  )
)

(script-fu-register "script-fu-selection-to-pattern"
  _"To _Pattern..."
  _"Convert a selection to a pattern"
  "Cameron Gregory <cameron@bloke.com>"
  "Cameron Gregory"
  "09/02/2003"
  "RGB* GRAY*"
  SF-IMAGE     "Image"        0
  SF-DRAWABLE  "Drawable"     0
  SF-STRING   _"Pattern name" "My Pattern"
  SF-STRING   _"File name"    "mypattern"
)
