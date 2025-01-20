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
                                 drawables
                                 radius
                                 shadow-toggle
                                 shadow-x
                                 shadow-y
                                 shadow-blur
                                 background-toggle
                                 work-on-copy)
  (let* ((drawable (vector-ref drawables 0))
         (shadow-blur (abs shadow-blur))
         (radius (abs radius))
         (diam (* 2 radius))
         (width (car (gimp-image-get-width img)))
         (height (car (gimp-image-get-height img)))
         (image (cond ((= work-on-copy TRUE)
                       (car (gimp-image-duplicate img)))
                      ((= work-on-copy FALSE)
                       img)))
         ; active drawable is not necessarily the active layer
         (pic-layer (vector-ref (car (gimp-image-get-selected-layers image)) 0))
         (type (car (gimp-drawable-type-with-alpha pic-layer)))
        )

  (gimp-context-push)
  (gimp-context-set-defaults)

  (if (= work-on-copy TRUE)
      (gimp-image-undo-disable image)
      (gimp-image-undo-group-start image)
  )

  ; add an alpha channel to the image
  (gimp-layer-add-alpha pic-layer)

  ; round the edges
  (gimp-selection-none image)
  (gimp-image-select-rectangle image CHANNEL-OP-ADD 0 0 radius radius)
  (gimp-image-select-ellipse image CHANNEL-OP-SUBTRACT  0 0 diam diam)
  (gimp-image-select-rectangle image CHANNEL-OP-ADD (- width radius) 0 radius radius)
  (gimp-image-select-ellipse image CHANNEL-OP-SUBTRACT (- width diam) 0 diam diam)
  (gimp-image-select-rectangle image CHANNEL-OP-ADD 0 (- height radius) radius radius)
  (gimp-image-select-ellipse image CHANNEL-OP-SUBTRACT 0 (- height diam) diam diam)
  (gimp-image-select-rectangle image CHANNEL-OP-ADD (- width radius) (- height radius)
                    radius radius)
  (gimp-image-select-ellipse image CHANNEL-OP-SUBTRACT (- width diam) (- height diam)
                       diam diam)
  (gimp-drawable-edit-clear pic-layer)
  (gimp-selection-none image)

  ; optionally add a shadow
  (if (= shadow-toggle TRUE)
      (begin
        (script-fu-drop-shadow image
                               (vector pic-layer)
                               shadow-x
                               shadow-y
                               shadow-blur
                               '(0 0 0)
                               80
                               TRUE)
        (set! width (car (gimp-image-get-width image)))
        (set! height (car (gimp-image-get-height image)))))

  ; optionally add a background
  (if (= background-toggle TRUE)
      (let* ((bg-layer (car (gimp-layer-new image
                                            "Background"
                                            width
                                            height
                                            type
                                            100
                                            LAYER-MODE-NORMAL))))
        (gimp-drawable-fill bg-layer FILL-BACKGROUND)
        (gimp-image-insert-layer image bg-layer 0 -1)
        (gimp-image-raise-item image pic-layer)
        (if (= shadow-toggle TRUE)
            (gimp-image-lower-item image bg-layer))))

; clean up after the script
  (if (= work-on-copy TRUE)
      (gimp-image-undo-enable image)
      (gimp-image-undo-group-end image)
  )

  (if (= work-on-copy TRUE)
      (gimp-display-new image))
  (gimp-context-pop)
  (gimp-displays-flush))
)

(script-fu-register-filter "script-fu-round-corners"
  _"_Round Corners..."
  _"Round the corners of an image and optionally add a drop-shadow and background"
  "Sven Neumann <sven@gimp.org>"
  "Sven Neumann"
  "1999/12/21"
  "RGB* GRAY*"
  SF-ONE-OR-MORE-DRAWABLE
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
