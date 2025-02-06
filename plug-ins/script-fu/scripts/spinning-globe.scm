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
        (theLayer (vector-ref (car (gimp-image-get-selected-layers theImage)) 0))
        (n 0)
        (ang (* (/ 360 inFrames)
                (if (= inFromLeft TRUE) 1 -1) ))
        (theFrame 0)
        )

  (gimp-layer-add-alpha theLayer)

  (while (> inFrames n)
    (set! n (+ n 1))
    (set! theFrame (car (gimp-layer-copy theLayer)))
    (gimp-image-insert-layer theImage theFrame 0 0)
    (gimp-item-set-name theFrame
                         (string-append "Anim Frame: "
                                        (number->string (- inFrames n) 10)
                                        " (replace)"))
    ;(gimp-message "Now call map-object")
    (plug-in-map-object #:run-mode              RUN-NONINTERACTIVE
                        #:image                 theImage              ; mapping image
                        #:drawables             (vector theFrame)     ; mapping drawables
                        #:map-type              "map-sphere"          ; sphere
                        #:viewpoint-x           0.5                   ; viewpoint
                        #:viewpoint-y           0.5
                        #:viewpoint-z           2.0
                        #:position-x            0.5                   ; object pos
                        #:position-y            0.5
                        #:position-z            0.0
                        #:first-axis-x          1.0                   ; first axis
                        #:first-axis-y          0.0
                        #:first-axis-z          0.0
                        #:second-axis-x         0.0                   ; 2nd axis
                        #:second-axis-y         1.0
                        #:second-axis-z         0.0
                        #:rotation-angle-x      0.0                   ; axis rotation
                        #:rotation-angle-y      (* n ang)
                        #:rotation-angle-z      0.0
                        #:light-type            "point-light"         ; light type
                        #:light-color           '(255 255 255)        ; light color
                        #:light-position-x      -0.5                  ; light position
                        #:light-position-y      -0.5
                        #:light-position-z       2.0
                        #:light-direction-x     -1.0                  ; light direction
                        #:light-direction-y     -1.0
                        #:light-direction-z      1.0
                        #:ambient-intensity      0.3                  ; material (amb, diff, refl, spec, high)
                        #:diffuse-intensity      1.0
                        #:diffuse-reflectivity   0.5
                        #:specular-reflectivity  0.0
                        #:highlight              27.0
                        #:antialiasing           TRUE                 ; antialias
                        #:depth                  3.0                  ; depth
                        #:threshold              0.25                 ; threshold
                        #:tiled                  FALSE                ; tile
                        #:new-image              FALSE                ; new image
                        #:new-layer              FALSE                ; new layer
                        #:transparent-background inTransparent        ; transparency
                        #:sphere-radius          0.25                 ; radius
    )
  )

  (gimp-image-remove-layer theImage theLayer)
  (gimp-image-autocrop theImage theFrame)

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

(script-fu-register-filter
  "script-fu-spinning-globe"
  _"_Spinning Globe..."
  _"Create an animation by mapping the current image onto a spinning sphere"
  "Chris Gutteridge"
  "1998, Chris Gutteridge / ECS dept, University of Southampton, England."
  "16th April 1998"
  "RGB* GRAY*"
  SF-ONE-OR-MORE-DRAWABLE
  SF-ADJUSTMENT _"Frames"                  '(10 1 360 1 10 0 1)
  SF-TOGGLE     _"Turn from left to right" FALSE
  SF-TOGGLE     _"Transparent background"  TRUE
  SF-ADJUSTMENT _"Index to n colors (0 = remain RGB)" '(63 0 256 1 10 0 1)
  SF-TOGGLE     _"Work on copy"            TRUE
)

(script-fu-menu-register "script-fu-spinning-globe"
                         "<Image>/Filters/Animation/")
