;
; old-photo
;
;
; Chris Gutteridge (cjg@ecs.soton.ac.uk)
; At ECS Dept, University of Southampton, England.

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
; Branko Collin <collin@xs4all.nl> added the possibility to change
; the border size in October 2001.

; Define the function:

(define (script-fu-old-photo inImage inLayer inDefocus inBorderSize inSepia inMottle inCopy)
  (let (
       (theImage (if (= inCopy TRUE) (car (ligma-image-duplicate inImage)) inImage))
       (theLayer 0)
       (theWidth 0)
       (theHeight 0)
       )
  (if (= inCopy TRUE)
     (ligma-image-undo-disable theImage)
     (ligma-image-undo-group-start theImage)
  )

  (ligma-selection-all theImage)

  (set! theLayer (car (ligma-image-flatten theImage)))
  (if (= inDefocus TRUE)
      (plug-in-gauss-rle RUN-NONINTERACTIVE theImage theLayer 1.5 TRUE TRUE)
  )
  (if (> inBorderSize 0)
      (script-fu-fuzzy-border theImage theLayer '(255 255 255)
                              inBorderSize TRUE 8 FALSE 100 FALSE TRUE )
  )
  (set! theLayer (car (ligma-image-flatten theImage)))

  (if (= inSepia TRUE)
      (begin (ligma-drawable-desaturate theLayer DESATURATE-LIGHTNESS)
             (ligma-drawable-brightness-contrast theLayer -0.078125 -0.15625)
             (ligma-drawable-color-balance theLayer TRANSFER-SHADOWS TRUE 30 0 -30)
      )
  )
  (set! theWidth (car (ligma-image-get-width theImage)))
  (set! theHeight (car (ligma-image-get-height theImage)))
  (if (= inMottle TRUE)
      (let (
	    (mLayer (car (ligma-layer-new theImage theWidth theHeight
					 RGBA-IMAGE "Mottle"
					 100 LAYER-MODE-DARKEN-ONLY)))
	    )

             (ligma-image-insert-layer theImage mLayer 0 0)
             (ligma-selection-all theImage)
             (ligma-drawable-edit-clear mLayer)
             (ligma-selection-none theImage)
             (plug-in-noisify RUN-NONINTERACTIVE theImage mLayer TRUE 0 0 0 0.5)
             (plug-in-gauss-rle RUN-NONINTERACTIVE theImage mLayer 5 TRUE TRUE)
             (set! theLayer (car (ligma-image-flatten theImage)))
      )
  )
  (ligma-selection-none theImage)

  (if (= inCopy TRUE)
      (begin  (ligma-image-clean-all theImage)
              (ligma-display-new theImage)
              (ligma-image-undo-enable theImage)
      )
      (ligma-image-undo-group-end theImage)
  )

  (ligma-displays-flush theImage)
  )
)

(script-fu-register "script-fu-old-photo"
  _"_Old Photo..."
  _"Make an image look like an old photo"
  "Chris Gutteridge"
  "1998, Chris Gutteridge / ECS dept, University of Southampton, England."
  "16th April 1998"
  "RGB* GRAY*"
  SF-IMAGE      "The image"     0
  SF-DRAWABLE   "The layer"     0
  SF-TOGGLE     _"Defocus"      TRUE
  SF-ADJUSTMENT _"Border size"  '(20 0 300 1 10 0 1)
     ; since this plug-in uses the fuzzy-border plug-in, I used the
     ; values of the latter, with the exception of the initial value
     ; and the 'minimum' value.
  SF-TOGGLE     _"Sepia"        TRUE
  SF-TOGGLE     _"Mottle"       FALSE
  SF-TOGGLE     _"Work on copy" TRUE
)

(script-fu-menu-register "script-fu-old-photo"
                         "<Image>/Filters/Decor")
