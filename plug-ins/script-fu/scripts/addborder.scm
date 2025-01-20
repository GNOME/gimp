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
; along with this program.  If not, see <https://www.gnu.org/licenses/>.
;
; Copyright (C) 1997 Andy Thomas alt@picnic.demon.co.uk
;
; Version 0.2 10.6.97 Changed to new script-fu interface in 0.99.10

; Delta the color by the given amount. Check for boundary conditions
; If < 0 set to zero
; If > 255 set to 255
; Return the new value

(define (script-fu-addborder aimg adraws xsize ysize color dvalue allow-resize)

  (define (deltacolor col delta)
    (let* ((newcol (+ col delta)))
      (if (< newcol 0) (set! newcol 0))
      (if (> newcol 255) (set! newcol 255))
      newcol
    )
  )

  (define (adjcolor col delta)
    (map (lambda (x) (deltacolor x delta)) col)
  )

  (define (gen_top_array xsize ysize owidth oheight width height)
    (let* ((n_array (cons-array 10 'double)))
      (vector-set! n_array 0 0 )
      (vector-set! n_array 1 0 )
      (vector-set! n_array 2 xsize)
      (vector-set! n_array 3 ysize)
      (vector-set! n_array 4 (+ xsize owidth))
      (vector-set! n_array 5 ysize)
      (vector-set! n_array 6 width)
      (vector-set! n_array 7 0 )
      (vector-set! n_array 8 0 )
      (vector-set! n_array 9 0 )
      n_array)
  )

  (define (gen_left_array xsize ysize owidth oheight width height)
    (let* ((n_array (cons-array 10 'double)))
      (vector-set! n_array 0 0 )
      (vector-set! n_array 1 0 )
      (vector-set! n_array 2 xsize)
      (vector-set! n_array 3 ysize)
      (vector-set! n_array 4 xsize)
      (vector-set! n_array 5 (+ ysize oheight))
      (vector-set! n_array 6 0 )
      (vector-set! n_array 7 height )
      (vector-set! n_array 8 0 )
      (vector-set! n_array 9 0 )
      n_array)
  )

  (define (gen_right_array xsize ysize owidth oheight width height)
    (let* ((n_array (cons-array 10 'double)))
      (vector-set! n_array 0 width )
      (vector-set! n_array 1 0 )
      (vector-set! n_array 2 (+ xsize owidth))
      (vector-set! n_array 3 ysize)
      (vector-set! n_array 4 (+ xsize owidth))
      (vector-set! n_array 5 (+ ysize oheight))
      (vector-set! n_array 6 width)
      (vector-set! n_array 7 height)
      (vector-set! n_array 8 width )
      (vector-set! n_array 9 0 )
      n_array)
  )

  (define (gen_bottom_array xsize ysize owidth oheight width height)
    (let* ((n_array (cons-array 10 'double)))
      (vector-set! n_array 0 0 )
      (vector-set! n_array 1 height)
      (vector-set! n_array 2 xsize)
      (vector-set! n_array 3 (+ ysize oheight))
      (vector-set! n_array 4 (+ xsize owidth))
      (vector-set! n_array 5 (+ ysize oheight))
      (vector-set! n_array 6 width)
      (vector-set! n_array 7 height)
      (vector-set! n_array 8 0 )
      (vector-set! n_array 9 height)
      n_array)
  )

  (let* (
         (first-layer (vector-ref adraws 0))
         (imagewidth (car (gimp-image-get-width aimg)))
         (imageheight (car (gimp-image-get-height aimg)))
         (innerwidth 0)
         (innerheight 0)
         (outerwidth 0)
         (outerheight 0))

         (if (= allow-resize TRUE)
             (begin
               (set! outerwidth (+ imagewidth (* 2 xsize)))
               (set! outerheight (+ imageheight (* 2 ysize)))
               (set! innerwidth imagewidth)
               (set! innerheight imageheight))
             (begin
               (set! outerwidth imagewidth)
               (set! outerheight imageheight)
               (set! innerwidth (- imagewidth (* 2 xsize)))
               (set! innerheight (- imageheight (* 2 ysize)))))

         (let* ((layer (car (gimp-layer-new aimg _"Border Layer"
                                            outerwidth outerheight
                                            (car (gimp-drawable-type-with-alpha first-layer))
                                            100 LAYER-MODE-NORMAL))))

           (gimp-context-push)
           (gimp-context-set-antialias FALSE)
           (gimp-context-set-feather FALSE)

           (gimp-image-undo-group-start aimg)

           (if (= allow-resize TRUE)
               (gimp-image-resize aimg
                                  outerwidth
                                  outerheight
                                  xsize
                                  ysize))

           (gimp-image-insert-layer aimg layer 0 0)
           (gimp-drawable-fill layer FILL-TRANSPARENT)

           (gimp-context-set-background (adjcolor color dvalue))
           (gimp-image-select-polygon aimg
                                      CHANNEL-OP-REPLACE
                                      (gen_top_array xsize ysize innerwidth innerheight outerwidth outerheight))
           (gimp-drawable-edit-fill layer FILL-BACKGROUND)
           (gimp-context-set-background (adjcolor color (/ dvalue 2)))
           (gimp-image-select-polygon aimg
                                      CHANNEL-OP-REPLACE
                                      (gen_left_array xsize ysize innerwidth innerheight outerwidth outerheight))
           (gimp-drawable-edit-fill layer FILL-BACKGROUND)
           (gimp-context-set-background (adjcolor color (- 0 (/ dvalue 2))))
           (gimp-image-select-polygon aimg
                                      CHANNEL-OP-REPLACE
                                      (gen_right_array xsize ysize innerwidth innerheight outerwidth outerheight))

           (gimp-drawable-edit-fill layer FILL-BACKGROUND)
           (gimp-context-set-background (adjcolor color (- 0 dvalue)))
           (gimp-image-select-polygon aimg
                                      CHANNEL-OP-REPLACE
                                      (gen_bottom_array xsize ysize innerwidth innerheight outerwidth outerheight))

           (gimp-drawable-edit-fill layer FILL-BACKGROUND)
           (gimp-selection-none aimg)
           (gimp-image-undo-group-end aimg)
           (gimp-displays-flush)

           (gimp-context-pop)
           )
    )
)

(script-fu-register-filter "script-fu-addborder"
  _"Add _Border..."
  _"Add a border around an image"
  "Andy Thomas <alt@picnic.demon.co.uk>, Michael Schumacher <schumaml@gmx.de>"
  "Andy Thomas, Michael Schumacher"
  "6/10/1997, 26/05/2017"
  "*"
  SF-ONE-OR-MORE-DRAWABLE
  SF-ADJUSTMENT _"Border X size" '(12 1 250 1 10 0 1)
  SF-ADJUSTMENT _"Border Y size" '(12 1 250 1 10 0 1)
  SF-COLOR      _"Border color" '(38 31 207)
  SF-ADJUSTMENT _"Delta value on color" '(25 1 255 1 10 0 1)
  SF-TOGGLE     _"Allow resizing" TRUE
)

(script-fu-menu-register "script-fu-addborder"
                         "<Image>/Filters/Decor")
