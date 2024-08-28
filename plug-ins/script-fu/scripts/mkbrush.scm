; GIMP - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; Make-Brush - a script for the script-fu program
; by Seth Burgess 1997 <sjburges@ou.edu>
;
; 18-Dec-2000 fixed to work with the new convention (not inverted) of
;             gbr saver (jtl@gimp.org)
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
        (img (car (gimp-image-new width height GRAY)))
        (drawable (car (gimp-layer-new img
                                       width height GRAY-IMAGE
                                       "MakeBrush" 100 LAYER-MODE-NORMAL)))
        (filename (string-append gimp-directory
                                 "/brushes/r"
                                 (number->string width)
                                 "x"
                                 (number->string height)
                                 ".gbr"))
        )

    (gimp-context-push)
    (gimp-context-set-defaults)

    (gimp-image-undo-disable img)
    (gimp-image-insert-layer img drawable 0 0)

    (gimp-context-set-background '(255 255 255))
    (gimp-drawable-fill drawable FILL-BACKGROUND)

    (gimp-image-select-rectangle img CHANNEL-OP-REPLACE 0 0 width height)

    (gimp-context-set-background '(0 0 0))
    (gimp-drawable-edit-fill drawable FILL-BACKGROUND)

    ; -1: NULL export_options
    (file-gbr-export 1 img filename -1 spacing name)
    (gimp-image-delete img)

    (gimp-context-pop)

    (gimp-brushes-refresh)
    (gimp-context-set-brush (car (gimp-brush-get-by-name name)))
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
                         "<Brushes>/Brushes Menu")


(define (script-fu-make-brush-rectangular-feathered name width height
                                                    feathering spacing)
  (let* (
        (widthplus (+ width feathering))
        (heightplus (+ height feathering))
        (img (car (gimp-image-new widthplus heightplus GRAY)))
        (drawable (car (gimp-layer-new img
                                       widthplus heightplus GRAY-IMAGE
                                       "MakeBrush" 100 LAYER-MODE-NORMAL)))
        (filename (string-append gimp-directory
                                 "/brushes/r"
                                 (number->string width)
                                 "x"
                                 (number->string height)
                                 "f"
                                 (number->string feathering)
                                 ".gbr"))
        )

    (gimp-context-push)
    (gimp-context-set-paint-mode LAYER-MODE-NORMAL)
    (gimp-context-set-opacity 100.0)

    (gimp-image-undo-disable img)
    (gimp-image-insert-layer img drawable 0 0)

    (gimp-context-set-background '(255 255 255))
    (gimp-drawable-fill drawable FILL-BACKGROUND)

    (cond
      ((< 0 feathering)
       (gimp-context-set-feather TRUE)
       (gimp-context-set-feather-radius feathering feathering)
       (gimp-image-select-rectangle img CHANNEL-OP-REPLACE
           (/ feathering 2) (/ feathering 2) width height))
      ((>= 0 feathering)
      (gimp-context-set-feather FALSE)
      (gimp-image-select-rectangle img CHANNEL-OP-REPLACE 0 0 width height))
    )

    (gimp-context-set-background '(0 0 0))
    (gimp-drawable-edit-fill drawable FILL-BACKGROUND)

    (file-gbr-export 1 img filename -1 spacing name)
    (gimp-image-delete img)

    (gimp-context-pop)

    (gimp-brushes-refresh)
    (gimp-context-set-brush (car (gimp-brush-get-by-name name)))
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
                         "<Brushes>/Brushes Menu")


(define (script-fu-make-brush-elliptical name width height spacing)
  (let* (
        (img (car (gimp-image-new width height GRAY)))
        (drawable (car (gimp-layer-new img
                                       width height GRAY-IMAGE
                                       "MakeBrush" 100 LAYER-MODE-NORMAL)))
        (filename (string-append gimp-directory
                                 "/brushes/e"
                                 (number->string width)
                                 "x"
                                 (number->string height)
                                 ".gbr"))
        )

    (gimp-context-push)
    (gimp-context-set-antialias TRUE)
    (gimp-context-set-feather FALSE)

    (gimp-image-undo-disable img)
    (gimp-image-insert-layer img drawable 0 0)

    (gimp-context-set-background '(255 255 255))
    (gimp-drawable-fill drawable FILL-BACKGROUND)
    (gimp-context-set-background '(0 0 0))
    (gimp-image-select-ellipse img CHANNEL-OP-REPLACE 0 0 width height)

    (gimp-drawable-edit-fill drawable FILL-BACKGROUND)

    (file-gbr-export 1 img filename -1 spacing name)
    (gimp-image-delete img)

    (gimp-context-pop)

    (gimp-brushes-refresh)
    (gimp-context-set-brush (car (gimp-brush-get-by-name name)))
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
                         "<Brushes>/Brushes Menu")


(define (script-fu-make-brush-elliptical-feathered name
                                                   width height
                                                   feathering spacing)
  (let* (
        (widthplus (+ feathering width)) ; add 3 for blurring
        (heightplus (+ feathering height))
        (img (car (gimp-image-new widthplus heightplus GRAY)))
        (drawable (car (gimp-layer-new img
                                       widthplus heightplus GRAY-IMAGE
                                       "MakeBrush" 100 LAYER-MODE-NORMAL)))
        (filename (string-append gimp-directory
                                 "/brushes/e"
                                 (number->string width)
                                 "x"
                                 (number->string height)
                                 "f"
                                 (number->string feathering)
                                 ".gbr"))
        )

    (gimp-context-push)
    (gimp-context-set-antialias TRUE)

    (gimp-image-undo-disable img)
    (gimp-image-insert-layer img drawable 0 0)

    (gimp-context-set-background '(255 255 255))
    (gimp-drawable-fill drawable FILL-BACKGROUND)

    (cond ((> feathering 0)   ; keep from taking out gimp with stupid entry.
        (gimp-context-set-feather TRUE)
        (gimp-context-set-feather-radius feathering feathering)
        (gimp-image-select-ellipse img CHANNEL-OP-REPLACE
           (/ feathering 2) (/ feathering 2)
           width height))
          ((<= feathering 0)
        (gimp-context-set-feather FALSE)
        (gimp-image-select-ellipse img CHANNEL-OP-REPLACE 0 0 width height)))

    (gimp-context-set-background '(0 0 0))
    (gimp-drawable-edit-fill drawable FILL-BACKGROUND)

    (file-gbr-export 1 img filename -1 spacing name)
    (gimp-image-delete img)

    (gimp-context-pop)

    (gimp-brushes-refresh)
    (gimp-context-set-brush (car (gimp-brush-get-by-name name)))
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
                         "<Brushes>/Brushes Menu")
