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
; along with this program.  If not, see <http://www.gnu.org/licenses/>.


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
        (theLayer (car (gimp-image-get-active-layer theImage)))
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
                        theImage theFrame    ; mapping
                        1                    ; viewpoint
                        0.5 0.5 2.0          ; object pos
                        0.5 0.5 0.0          ; first axis
                        1.0 0.0 0.0          ; 2nd axis
                        0.0 1.0 0.0          ; axis rotation
                        0.0 (* n ang) 0.0    ; light (type, color)
                        0 '(255 255 255)     ; light position
                        -0.5 -0.5 2.0        ; light direction
                        -1.0 -1.0 1.0  ; material (amb, diff, refl, spec, high)
                        0.3 1.0 0.5 0.0 27.0 ; antialias
                        TRUE                 ; tile
                        FALSE                ; new image
                        FALSE                ; transparency
                        inTransparent        ; radius
                        0.25                 ; unused parameters
                        1.0 1.0 1.0 1.0
                        -1 -1 -1 -1 -1 -1 -1 -1
    )
  )

  (gimp-image-remove-layer theImage theLayer)
  (plug-in-autocrop RUN-NONINTERACTIVE theImage theFrame)

  (if (= inIndex 0)
      ()
      (gimp-image-convert-indexed theImage FS-DITHER MAKE-PALETTE inIndex
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
                         "<Image>/Filters/Animation/Animators")
