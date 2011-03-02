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
; slide.scm   version 0.41   2004/03/28
;
; CHANGE-LOG:
; 0.20 - first public release
; 0.30 - some code cleanup
;        now uses the rotate plug-in to improve speed
; 0.40 - changes to work with gimp-1.1
;        if the image was rotated, rotate the whole thing back when finished
; 0.41 - changes to work with gimp-2.0, slightly correct text offsets,
;        Nils Philippsen <nphilipp@redhat.com> 2004/03/28
;
; !still in development!
; TODO: - change the script so that the film is rotated, not the image
;       - antialiasing
;       - make 'add background' an option
;       - ?
;
; Copyright (C) 1997-1999 Sven Neumann <sven@gimp.org>
;
; makes your picture look like a slide
;
; The script works on RGB and grayscale images that contain only
; one layer. The image is cropped to fit into an aspect ratio of 1:1,5.
; It creates a copy of the image or can optionally work on the original.
; The script uses the current background color to create a background
; layer.


(define (script-fu-slide img
                         drawable
                         text
                         number
                         fontname
                         font-color
                         work-on-copy)

  (define (crop width height ratio)
    (if (>= width (* ratio height))
        (* ratio height)
        width
    )
  )

  (let* (
        (type (car (gimp-drawable-type-with-alpha drawable)))
        (image (cond ((= work-on-copy TRUE)
                      (car (gimp-image-duplicate img)))
                     ((= work-on-copy FALSE)
                      img)))
        (owidth (car (gimp-image-width image)))
        (oheight (car (gimp-image-height image)))
        (ratio (if (>= owidth oheight) (/ 3 2)
                                       (/ 2 3)))
        (crop-width (crop owidth oheight ratio))
        (crop-height (/ crop-width ratio))
        (width (* (max crop-width crop-height) 1.05))
        (height (* (min crop-width crop-height) 1.5))
        (hole-width (/ width 20))
        (hole-space (/ width 8))
        (hole-height (/ width 12))
        (hole-radius (/ hole-width 4))
        (hole-start (- (/ (rand 1000) 1000) 0.5))
        (film-layer (car (gimp-layer-new image
                                         width
                                         height
                                         type
                                         "Film"
                                         100
                                         NORMAL-MODE)))
        (bg-layer (car (gimp-layer-new image
                                       width
                                       height
                                       type
                                       "Background"
                                       100
                                       NORMAL-MODE)))
        (pic-layer (car (gimp-image-get-active-drawable image)))
        (numbera (string-append number "A"))
        )

  (gimp-context-push)

  (gimp-image-undo-disable image)

; add an alpha channel to the image
  (gimp-layer-add-alpha pic-layer)

; crop, resize and eventually rotate the image
  (gimp-image-crop image
                   crop-width
                   crop-height
                   (/ (- owidth crop-width) 2)
                   (/ (- oheight crop-height) 2))
  (gimp-image-resize image
                     width
                     height
                     (/ (- width crop-width) 2)
                     (/ (- height crop-height) 2))
  (if (< ratio 1)
      (plug-in-rotate RUN-NONINTERACTIVE image pic-layer 1 FALSE)
  )

; add the background layer
  (gimp-drawable-fill bg-layer BACKGROUND-FILL)
  (gimp-image-insert-layer image bg-layer 0 -1)

; add the film layer
  (gimp-context-set-background '(0 0 0))
  (gimp-drawable-fill film-layer BACKGROUND-FILL)
  (gimp-image-insert-layer image film-layer 0 -1)

; add the text
  (gimp-context-set-foreground font-color)
  (gimp-floating-sel-anchor (car (gimp-text-fontname image
                                            film-layer
                                            (+ hole-start (* -0.25 width))
                                            (* 0.01 height)
                                            text
                                            0
                                            TRUE
                                            (* 0.040 height) PIXELS fontname)))
  (gimp-floating-sel-anchor (car (gimp-text-fontname image
                                            film-layer
                                            (+ hole-start (* 0.75 width))
                                            (* 0.01 height)
                                            text
                                            0
                                            TRUE
                                            (* 0.040 height) PIXELS
                                            fontname )))
  (gimp-floating-sel-anchor (car (gimp-text-fontname image
                                            film-layer
                                            (+ hole-start (* 0.35 width))
                                            0.0
                                            number
                                            0
                                            TRUE
                                            (* 0.050 height) PIXELS
                                            fontname )))
  (gimp-floating-sel-anchor (car (gimp-text-fontname image
                                            film-layer
                                            (+ hole-start (* 0.35 width))
                                            (* 0.94 height)
                                            number
                                            0
                                            TRUE
                                            (* 0.050 height) PIXELS
                                            fontname )))
  (gimp-floating-sel-anchor (car (gimp-text-fontname image
                                            film-layer
                                            (+ hole-start (* 0.85 width))
                                            (* 0.945 height)
                                            numbera
                                            0
                                            TRUE
                                            (* 0.045 height) PIXELS
                                            fontname )))

; create a mask for the holes and cut them out
  (let* (
        (film-mask (car (gimp-layer-create-mask film-layer ADD-WHITE-MASK)))
        (hole hole-start)
        (top-y (* height 0.06))
        (bottom-y (* height 0.855))
        )

    (gimp-layer-add-mask film-layer film-mask)

    (gimp-selection-none image)
    (while (< hole 8)
           (gimp-rect-select image
                             (* hole-space hole)
                             top-y
                             hole-width
                             hole-height
                             CHANNEL-OP-ADD
                             FALSE
                             0)
           (gimp-rect-select image
                             (* hole-space hole)
                             bottom-y
                             hole-width
                             hole-height
                             CHANNEL-OP-ADD
                             FALSE
                             0)
           (set! hole (+ hole 1))
    )

    (gimp-context-set-foreground '(0 0 0))
    (gimp-edit-fill film-mask BACKGROUND-FILL)
    (gimp-selection-none image)
    (plug-in-gauss-rle RUN-NONINTERACTIVE image film-mask hole-radius TRUE TRUE)
    (gimp-threshold film-mask 127 255)

    (gimp-layer-remove-mask film-layer MASK-APPLY)
  )

; reorder the layers
  (gimp-image-raise-layer image pic-layer)
  (gimp-image-raise-layer image pic-layer)

; eventually rotate the whole thing back
  (if (< ratio 1)
      (plug-in-rotate RUN-NONINTERACTIVE image pic-layer 3 TRUE)
  )

; clean up after the script
  (gimp-selection-none image)
  (gimp-image-undo-enable image)
  (if (= work-on-copy TRUE)
      (gimp-display-new image)
  )

  (gimp-displays-flush)

  (gimp-context-pop)
  )
)

(script-fu-register "script-fu-slide"
  _"_Slide..."
  _"Add a slide-film like frame, sprocket holes, and labels to an image"
  "Sven Neumann <sven@gimp.org>"
  "Sven Neumann"
  "2004/03/28"
  "RGB GRAY"
  SF-IMAGE     "Image"         0
  SF-DRAWABLE  "Drawable"      0
  SF-STRING   _"Text"          "GIMP"
  SF-STRING   _"Number"        "32"
  SF-FONT     _"Font"          "Serif"
  SF-COLOR    _"Font color"    '(255 180 0)
  SF-TOGGLE   _"Work on copy"  TRUE
)

(script-fu-menu-register "script-fu-slide"
                         "<Image>/Filters/Decor")
