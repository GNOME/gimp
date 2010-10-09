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
; Copyright (C) 1997 Andy Thomas alt@picnic.demon.co.uk
;
; Version 0.2 10.6.97 Changed to new script-fu interface in 0.99.10

; Delta the colour by the given amount. Check for boundary conditions
; If < 0 set to zero
; If > 255 set to 255
; Return the new value

(define (script-fu-addborder aimg adraw xsize ysize colour dvalue)

  (define (deltacolour col delta)
    (let* ((newcol (+ col delta)))
      (if (< newcol 0) (set! newcol 0))
      (if (> newcol 255) (set! newcol 255))
      newcol
    )
  )

  (define (adjcolour col delta)
    (mapcar (lambda (x) (deltacolour x delta)) col)
  )

  (define (gen_top_array xsize ysize owidth oheight width height)
    (let* ((n_array (cons-array 10 'double)))
      (aset n_array 0 0 )
      (aset n_array 1 0 )
      (aset n_array 2 xsize)
      (aset n_array 3 ysize)
      (aset n_array 4 (+ xsize owidth))
      (aset n_array 5 ysize)
      (aset n_array 6 width)
      (aset n_array 7 0 )
      (aset n_array 8 0 )
      (aset n_array 9 0 )
      n_array)
  )

  (define (gen_left_array xsize ysize owidth oheight width height)
    (let* ((n_array (cons-array 10 'double)))
      (aset n_array 0 0 )
      (aset n_array 1 0 )
      (aset n_array 2 xsize)
      (aset n_array 3 ysize)
      (aset n_array 4 xsize)
      (aset n_array 5 (+ ysize oheight))
      (aset n_array 6 0 )
      (aset n_array 7 height )
      (aset n_array 8 0 )
      (aset n_array 9 0 )
      n_array)
  )

  (define (gen_right_array xsize ysize owidth oheight width height)
    (let* ((n_array (cons-array 10 'double)))
      (aset n_array 0 width )
      (aset n_array 1 0 )
      (aset n_array 2 (+ xsize owidth))
      (aset n_array 3 ysize)
      (aset n_array 4 (+ xsize owidth))
      (aset n_array 5 (+ ysize oheight))
      (aset n_array 6 width)
      (aset n_array 7 height)
      (aset n_array 8 width )
      (aset n_array 9 0 )
      n_array)
  )

  (define (gen_bottom_array xsize ysize owidth oheight width height)
    (let* ((n_array (cons-array 10 'double)))
      (aset n_array 0 0 )
      (aset n_array 1 height)
      (aset n_array 2 xsize)
      (aset n_array 3 (+ ysize oheight))
      (aset n_array 4 (+ xsize owidth))
      (aset n_array 5 (+ ysize oheight))
      (aset n_array 6 width)
      (aset n_array 7 height)
      (aset n_array 8 0 )
      (aset n_array 9 height)
      n_array)
  )

  (let* ((img (car (gimp-item-get-image adraw)))
         (owidth (car (gimp-image-width img)))
         (oheight (car (gimp-image-height img)))
         (width (+ owidth (* 2 xsize)))
         (height (+ oheight (* 2 ysize)))
         (layer (car (gimp-layer-new img
                                     width height
                                     (car (gimp-drawable-type-with-alpha adraw))
                                     _"Border Layer" 100 NORMAL-MODE))))

    (gimp-context-push)

    (gimp-image-undo-group-start img)

    (gimp-image-resize img
                       width
                       height
                       xsize
                       ysize)

    (gimp-image-insert-layer img layer -1 0)
    (gimp-drawable-fill layer TRANSPARENT-FILL)

    (gimp-context-set-background (adjcolour colour dvalue))
    (gimp-free-select img
                      10
                      (gen_top_array xsize ysize owidth oheight width height)
                      CHANNEL-OP-REPLACE
                      0
                      0
                      0.0)
    (gimp-edit-fill layer BACKGROUND-FILL)
    (gimp-context-set-background (adjcolour colour (/ dvalue 2)))
    (gimp-free-select img
                      10
                      (gen_left_array xsize ysize owidth oheight width height)
                      CHANNEL-OP-REPLACE
                      0
                      0
                      0.0)
    (gimp-edit-fill layer BACKGROUND-FILL)
    (gimp-context-set-background (adjcolour colour (- 0 (/ dvalue 2))))
    (gimp-free-select img
                      10
                      (gen_right_array xsize ysize owidth oheight width height)
                      CHANNEL-OP-REPLACE
                      0
                      0
                      0.0)

    (gimp-edit-fill layer BACKGROUND-FILL)
    (gimp-context-set-background (adjcolour colour (- 0 dvalue)))
    (gimp-free-select img
                      10
                      (gen_bottom_array xsize ysize owidth oheight width height)
                      CHANNEL-OP-REPLACE
                      0
                      0
                      0.0)

    (gimp-edit-fill layer BACKGROUND-FILL)
    (gimp-selection-none img)
    (gimp-image-undo-group-end img)
    (gimp-displays-flush)

    (gimp-context-pop)
    )
)

(script-fu-register "script-fu-addborder"
  _"Add _Border..."
  _"Add a border around an image"
  "Andy Thomas <alt@picnic.demon.co.uk>"
  "Andy Thomas"
  "6/10/97"
  "*"
  SF-IMAGE       "Input image" 0
  SF-DRAWABLE    "Input drawable" 0
  SF-ADJUSTMENT _"Border X size" '(12 1 250 1 10 0 1)
  SF-ADJUSTMENT _"Border Y size" '(12 1 250 1 10 0 1)
  SF-COLOR      _"Border color" '(38 31 207)
  SF-ADJUSTMENT _"Delta value on color" '(25 1 255 1 10 0 1)
)

(script-fu-menu-register "script-fu-addborder"
                         "<Image>/Filters/Decor")
