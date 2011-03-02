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
;
; round-corners.scm   version 1.02   1999/12/21
;
; CHANGE-LOG:
; 1.00 - initial release
; 1.01 - some code cleanup, no real changes
;
; Copyright (C) 1997-1999 Sven Neumann <sven@gimp.org>
;
;
; Rounds the corners of an image, optionally adding a drop-shadow and
; a background layer
;
; The script works on RGB and grayscale images that contain only
; one layer. It creates a copy of the image or can optionally work
; on the original. The script uses the current background color to
; create a background layer. It makes a call to the script drop-shadow.
;
; This script is derived from my script add-shadow, which has become
; obsolete now.



(define (script-fu-round-corners img
                                 drawable
                                 radius
                                 shadow-toggle
                                 shadow-x
                                 shadow-y
                                 shadow-blur
                                 background-toggle
                                 work-on-copy)
  (let* ((shadow-blur (abs shadow-blur))
         (radius (abs radius))
         (diam (* 2 radius))
         (width (car (gimp-image-width img)))
         (height (car (gimp-image-height img)))
         (type (car (gimp-drawable-type-with-alpha drawable)))
         (image (cond ((= work-on-copy TRUE)
                       (car (gimp-image-duplicate img)))
                      ((= work-on-copy FALSE)
                       img)))
         (pic-layer (car (gimp-image-get-active-drawable image))))

  (if (= work-on-copy TRUE)
      (gimp-image-undo-disable image)
      (gimp-image-undo-group-start image)
  )

  ; add an alpha channel to the image
  (gimp-layer-add-alpha pic-layer)

  ; round the edges
  (gimp-selection-none image)
  (gimp-rect-select image 0 0 radius radius CHANNEL-OP-ADD 0 0)
  (gimp-ellipse-select image 0 0 diam diam CHANNEL-OP-SUBTRACT TRUE 0 0)
  (gimp-rect-select image (- width radius) 0 radius radius CHANNEL-OP-ADD 0 0)
  (gimp-ellipse-select image (- width diam) 0 diam diam CHANNEL-OP-SUBTRACT TRUE 0 0)
  (gimp-rect-select image 0 (- height radius) radius radius CHANNEL-OP-ADD 0 0)
  (gimp-ellipse-select image 0 (- height diam) diam diam CHANNEL-OP-SUBTRACT TRUE 0 0)
  (gimp-rect-select image (- width radius) (- height radius)
                    radius radius CHANNEL-OP-ADD 0 0)
  (gimp-ellipse-select image (- width diam) (- height diam)
                       diam diam CHANNEL-OP-SUBTRACT TRUE 0 0)
  (gimp-edit-clear pic-layer)
  (gimp-selection-none image)

  ; optionally add a shadow
  (if (= shadow-toggle TRUE)
      (begin
        (script-fu-drop-shadow image
                               pic-layer
                               shadow-x
                               shadow-y
                               shadow-blur
                               '(0 0 0)
                               80
                               TRUE)
        (set! width (car (gimp-image-width image)))
        (set! height (car (gimp-image-height image)))))

  ; optionally add a background
  (if (= background-toggle TRUE)
      (let* ((bg-layer (car (gimp-layer-new image
                                            width
                                            height
                                            type
                                            "Background"
                                            100
                                            NORMAL-MODE))))
        (gimp-drawable-fill bg-layer BACKGROUND-FILL)
        (gimp-image-insert-layer image bg-layer 0 -1)
        (gimp-image-raise-layer image pic-layer)
        (if (= shadow-toggle TRUE)
            (gimp-image-lower-layer image bg-layer))))

; clean up after the script
  (if (= work-on-copy TRUE)
      (gimp-image-undo-enable image)
      (gimp-image-undo-group-end image)
  )

  (if (= work-on-copy TRUE)
      (gimp-display-new image))
  (gimp-displays-flush))
)

(script-fu-register "script-fu-round-corners"
  _"_Round Corners..."
  _"Round the corners of an image and optionally add a drop-shadow and background"
  "Sven Neumann <sven@gimp.org>"
  "Sven Neumann"
  "1999/12/21"
  "RGB GRAY"
  SF-IMAGE      "Image"            0
  SF-DRAWABLE   "Drawable"         0
  SF-ADJUSTMENT _"Edge radius"     '(15 0 4096 1 10 0 1)
  SF-TOGGLE     _"Add drop-shadow" TRUE
  SF-ADJUSTMENT _"Shadow X offset" '(8 -4096 4096 1 10 0 1)
  SF-ADJUSTMENT _"Shadow Y offset" '(8 -4096 4096 1 10 0 1)
  SF-ADJUSTMENT _"Blur radius"     '(15 0 1024 1 10 0 1)
  SF-TOGGLE     _"Add background"  TRUE
  SF-TOGGLE     _"Work on copy"    TRUE
)

(script-fu-menu-register "script-fu-round-corners"
                         "<Image>/Filters/Decor")
