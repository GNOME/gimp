; LIGMA - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; Make-Brush - a script for the script-fu program
; by Seth Burgess 1997 <sjburges@ou.edu>
;
; 18-Dec-2000 fixed to work with the new convention (not inverted) of
;             gbr saver (jtl@ligma.org)
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


(define (script-fu-make-brush-rectangular name width height spacing)
  (let* (
        (img (car (ligma-image-new width height GRAY)))
        (drawable (car (ligma-layer-new img
                                       width height GRAY-IMAGE
                                       "MakeBrush" 100 LAYER-MODE-NORMAL)))
        (filename (string-append ligma-directory
                                 "/brushes/r"
                                 (number->string width)
                                 "x"
                                 (number->string height)
                                 ".gbr"))
        )

    (ligma-context-push)
    (ligma-context-set-defaults)

    (ligma-image-undo-disable img)
    (ligma-image-insert-layer img drawable 0 0)

    (ligma-context-set-background '(255 255 255))
    (ligma-drawable-fill drawable FILL-BACKGROUND)

    (ligma-image-select-rectangle img CHANNEL-OP-REPLACE 0 0 width height)

    (ligma-context-set-background '(0 0 0))
    (ligma-drawable-edit-fill drawable FILL-BACKGROUND)

    (file-gbr-save 1 img 1 (vector drawable) filename spacing name)
    (ligma-image-delete img)

    (ligma-context-pop)

    (ligma-brushes-refresh)
    (ligma-context-set-brush name)
  )
)

(script-fu-register "script-fu-make-brush-rectangular"
  _"_Rectangular..."
  _"Create a rectangular brush"
  "Seth Burgess <sjburges@ou.edu>"
  "Seth Burgess"
  "1997"
  ""
  SF-STRING     _"Name"    "Rectangle"
  SF-ADJUSTMENT _"Width"   '(20 1 200 1 10 0 1)
  SF-ADJUSTMENT _"Height"  '(20 1 200 1 10 0 1)
  SF-ADJUSTMENT _"Spacing" '(25 1 100 1 10 1 0)
)

(script-fu-menu-register "script-fu-make-brush-rectangular"
                         "<Brushes>")


(define (script-fu-make-brush-rectangular-feathered name width height
                                                    feathering spacing)
  (let* (
        (widthplus (+ width feathering))
        (heightplus (+ height feathering))
        (img (car (ligma-image-new widthplus heightplus GRAY)))
        (drawable (car (ligma-layer-new img
                                       widthplus heightplus GRAY-IMAGE
                                       "MakeBrush" 100 LAYER-MODE-NORMAL)))
        (filename (string-append ligma-directory
                                 "/brushes/r"
                                 (number->string width)
                                 "x"
                                 (number->string height)
                                 "f"
                                 (number->string feathering)
                                 ".gbr"))
        )

    (ligma-context-push)
    (ligma-context-set-paint-mode LAYER-MODE-NORMAL)
    (ligma-context-set-opacity 100.0)

    (ligma-image-undo-disable img)
    (ligma-image-insert-layer img drawable 0 0)

    (ligma-context-set-background '(255 255 255))
    (ligma-drawable-fill drawable FILL-BACKGROUND)

    (cond
      ((< 0 feathering)
       (ligma-context-set-feather TRUE)
       (ligma-context-set-feather-radius feathering feathering)
       (ligma-image-select-rectangle img CHANNEL-OP-REPLACE
           (/ feathering 2) (/ feathering 2) width height))
      ((>= 0 feathering)
      (ligma-context-set-feather FALSE)
      (ligma-image-select-rectangle img CHANNEL-OP-REPLACE 0 0 width height))
    )

    (ligma-context-set-background '(0 0 0))
    (ligma-drawable-edit-fill drawable FILL-BACKGROUND)

    (file-gbr-save 1 img 1 (vector drawable) filename spacing name)
    (ligma-image-delete img)

    (ligma-context-pop)

    (ligma-brushes-refresh)
    (ligma-context-set-brush name)
  )
)

(script-fu-register "script-fu-make-brush-rectangular-feathered"
  _"Re_ctangular, Feathered..."
  _"Create a rectangular brush with feathered edges"
  "Seth Burgess <sjburges@ou.edu>"
  "Seth Burgess"
  "1997"
  ""
  SF-STRING     _"Name"       "Rectangle"
  SF-ADJUSTMENT _"Width"      '(20 1 200 1 10 0 1)
  SF-ADJUSTMENT _"Height"     '(20 1 200 1 10 0 1)
  SF-ADJUSTMENT _"Feathering" '(4 1 100 1 10 0 1)
  SF-ADJUSTMENT _"Spacing"    '(25 1 100 1 10 1 0)
)

(script-fu-menu-register "script-fu-make-brush-rectangular-feathered"
                         "<Brushes>")


(define (script-fu-make-brush-elliptical name width height spacing)
  (let* (
        (img (car (ligma-image-new width height GRAY)))
        (drawable (car (ligma-layer-new img
                                       width height GRAY-IMAGE
                                       "MakeBrush" 100 LAYER-MODE-NORMAL)))
        (filename (string-append ligma-directory
                                 "/brushes/e"
                                 (number->string width)
                                 "x"
                                 (number->string height)
                                 ".gbr"))
        )

    (ligma-context-push)
    (ligma-context-set-antialias TRUE)
    (ligma-context-set-feather FALSE)

    (ligma-image-undo-disable img)
    (ligma-image-insert-layer img drawable 0 0)

    (ligma-context-set-background '(255 255 255))
    (ligma-drawable-fill drawable FILL-BACKGROUND)
    (ligma-context-set-background '(0 0 0))
    (ligma-image-select-ellipse img CHANNEL-OP-REPLACE 0 0 width height)

    (ligma-drawable-edit-fill drawable FILL-BACKGROUND)

    (file-gbr-save 1 img 1 (vector drawable) filename spacing name)
    (ligma-image-delete img)

    (ligma-context-pop)

    (ligma-brushes-refresh)
    (ligma-context-set-brush name)
  )
)

(script-fu-register "script-fu-make-brush-elliptical"
  _"_Elliptical..."
  _"Create an elliptical brush"
  "Seth Burgess <sjburges@ou.edu>"
  "Seth Burgess"
  "1997"
  ""
  SF-STRING     _"Name"    "Ellipse"
  SF-ADJUSTMENT _"Width"   '(20 1 200 1 10 0 1)
  SF-ADJUSTMENT _"Height"  '(20 1 200 1 10 0 1)
  SF-ADJUSTMENT _"Spacing" '(25 1 100 1 10 1 0)
)

(script-fu-menu-register "script-fu-make-brush-elliptical"
                         "<Brushes>")


(define (script-fu-make-brush-elliptical-feathered name
                                                   width height
                                                   feathering spacing)
  (let* (
        (widthplus (+ feathering width)) ; add 3 for blurring
        (heightplus (+ feathering height))
        (img (car (ligma-image-new widthplus heightplus GRAY)))
        (drawable (car (ligma-layer-new img
                                       widthplus heightplus GRAY-IMAGE
                                       "MakeBrush" 100 LAYER-MODE-NORMAL)))
        (filename (string-append ligma-directory
                                 "/brushes/e"
                                 (number->string width)
                                 "x"
                                 (number->string height)
                                 "f"
                                 (number->string feathering)
                                 ".gbr"))
        )

    (ligma-context-push)
    (ligma-context-set-antialias TRUE)

    (ligma-image-undo-disable img)
    (ligma-image-insert-layer img drawable 0 0)

    (ligma-context-set-background '(255 255 255))
    (ligma-drawable-fill drawable FILL-BACKGROUND)

    (cond ((> feathering 0)   ; keep from taking out ligma with stupid entry.
        (ligma-context-set-feather TRUE)
        (ligma-context-set-feather-radius feathering feathering)
        (ligma-image-select-ellipse img CHANNEL-OP-REPLACE
           (/ feathering 2) (/ feathering 2)
           width height))
          ((<= feathering 0)
        (ligma-context-set-feather FALSE)
        (ligma-image-select-ellipse img CHANNEL-OP-REPLACE 0 0 width height)))

    (ligma-context-set-background '(0 0 0))
    (ligma-drawable-edit-fill drawable FILL-BACKGROUND)

    (file-gbr-save 1 img 1 (vector drawable) filename spacing name)
    (ligma-image-delete img)

    (ligma-context-pop)

    (ligma-brushes-refresh)
    (ligma-context-set-brush name)
  )
)

(script-fu-register "script-fu-make-brush-elliptical-feathered"
  _"Elli_ptical, Feathered..."
  _"Create an elliptical brush with feathered edges"
  "Seth Burgess <sjburges@ou.edu>"
  "Seth Burgess"
  "1997"
  ""
  SF-STRING     _"Name"       "Ellipse"
  SF-ADJUSTMENT _"Width"      '(20 1 200 1 10 0 1)
  SF-ADJUSTMENT _"Height"     '(20 1 200 1 10 0 1)
  SF-ADJUSTMENT _"Feathering" '(4 1 100 1 10 0 1)
  SF-ADJUSTMENT _"Spacing"    '(25 1 100 1 10 1 0)
)

(script-fu-menu-register "script-fu-make-brush-elliptical-feathered"
                         "<Brushes>")
