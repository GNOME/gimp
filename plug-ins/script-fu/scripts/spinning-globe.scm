;
; anim_sphere
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


; Define the function:

(define (script-fu-spinning-globe inImage
                                  inLayer
                                  inFrames
                                  inFromLeft
                                  inTransparent
                                  inIndex
                                  inCopy)
  (let* (
        (theImage (if (= inCopy TRUE)
                      (car (gimp-image-duplicate inImage))
                      inImage))
        (theLayer (aref (cadr (gimp-image-get-selected-layers theImage)) 0))
        (n 0)
        (ang (* (/ 360 inFrames)
                (if (= inFromLeft TRUE) 1 -1) ))
        (theFrame 0)
        )

  (gimp-layer-add-alpha theLayer)

  (while (> inFrames n)
    (set! n (+ n 1))
    (set! theFrame (car (gimp-layer-copy theLayer FALSE)))
    (gimp-image-insert-layer theImage theFrame 0 0)
    (gimp-item-set-name theFrame
                         (string-append "Anim Frame: "
                                        (number->string (- inFrames n) 10)
                                        " (replace)"))
    (plug-in-map-object RUN-NONINTERACTIVE
                        theImage             ; mapping image
                        1 (vector theFrame)  ; mapping drawables
                        "map-sphere"         ; sphere
                        0.5 0.5 2.0          ; viewpoint
                        0.5 0.5 0.0          ; object pos
                        1.0 0.0 0.0          ; first axis
                        0.0 1.0 0.0          ; 2nd axis
                        0.0 (* n ang) 0.0    ; axis rotation
                        "point-light"        ; light type
                        '(255 255 255)       ; light color
                        -0.5 -0.5 2.0        ; light position
                        -1.0 -1.0 1.0        ; light direction
                        0.3 1.0 0.5 0.0 27.0 ; material (amb, diff, refl, spec, high)
                        TRUE                 ; antialias
                        3.0                  ; depth
                        0.25                 ; threshold
                        FALSE                ; tile
                        FALSE                ; new image
                        FALSE                ; new layer
                        inTransparent        ; transparency
                        0.25                 ; radius
                        1.0 1.0 1.0 1.0      ; unused parameters
                        -1 -1 0.5 0.5 0.5 -1 -1 0.25 0.25
    )
  )

  (gimp-image-remove-layer theImage theLayer)
  (plug-in-autocrop RUN-NONINTERACTIVE theImage theFrame)

  (if (= inIndex 0)
      ()
      (gimp-image-convert-indexed theImage CONVERT-DITHER-FS CONVERT-PALETTE-GENERATE inIndex
                                  FALSE FALSE ""))

  (if (= inCopy TRUE)
    (begin
      (gimp-image-clean-all theImage)
      (gimp-display-new theImage)
    )
  )

  (gimp-displays-flush)
  )
)

(script-fu-register
  "script-fu-spinning-globe"
  _"_Spinning Globe..."
  _"Create an animation by mapping the current image onto a spinning sphere"
  "Chris Gutteridge"
  "1998, Chris Gutteridge / ECS dept, University of Southampton, England."
  "16th April 1998"
  "RGB* GRAY*"
  SF-IMAGE       "The Image"               0
  SF-DRAWABLE    "The Layer"               0
  SF-ADJUSTMENT _"Frames"                  '(10 1 360 1 10 0 1)
  SF-TOGGLE     _"Turn from left to right" FALSE
  SF-TOGGLE     _"Transparent background"  TRUE
  SF-ADJUSTMENT _"Index to n colors (0 = remain RGB)" '(63 0 256 1 10 0 1)
  SF-TOGGLE     _"Work on copy"            TRUE
)

(script-fu-menu-register "script-fu-spinning-globe"
                         "<Image>/Filters/Animation/")
