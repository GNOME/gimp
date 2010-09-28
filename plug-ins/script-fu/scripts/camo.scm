;
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


(define (script-fu-camo-pattern inSize inGrain inColor1 inColor2 inColor3 inSmooth inFlatten)

  (let* (
        (theWidth inSize)
        (theHeight inSize)
        (theImage (car (gimp-image-new theWidth theHeight RGB)))
        (baseLayer (car (gimp-layer-new theImage theWidth theHeight RGBA-IMAGE "Background" 100 NORMAL-MODE)))
        (thickLayer 0)
        (thinLayer 0)
        (theBlur 0)
        )

    (gimp-context-push)

    (gimp-image-insert-layer theImage baseLayer 0)

    (set! thickLayer (car (gimp-layer-new theImage theWidth theHeight RGBA-IMAGE "Camo Thick Layer" 100 NORMAL-MODE)))
    (gimp-image-insert-layer theImage thickLayer 0)

    (set! thinLayer (car (gimp-layer-new theImage theWidth theHeight RGBA-IMAGE "Camo Thin Layer" 100 NORMAL-MODE)))
    (gimp-image-insert-layer theImage thinLayer 0)

    (gimp-selection-all theImage)
    (gimp-context-set-background inColor1)
    (gimp-drawable-fill baseLayer BACKGROUND-FILL)

    (plug-in-solid-noise RUN-NONINTERACTIVE
			 theImage thickLayer 1 0 (rand 65536) 1 inGrain inGrain)
    (plug-in-solid-noise RUN-NONINTERACTIVE
			 theImage thinLayer 1 0 (rand 65536) 1 inGrain inGrain)
    (gimp-threshold thickLayer 127 255)
    (gimp-threshold thinLayer 145 255)

    (set! theBlur (- 16 inGrain))

    (gimp-context-set-background inColor2)
    (gimp-by-color-select thickLayer
			  '(0 0 0) 127 CHANNEL-OP-REPLACE TRUE FALSE 0 FALSE)
    (gimp-edit-clear thickLayer)
    (gimp-selection-invert theImage)
    (gimp-edit-fill thickLayer BACKGROUND-FILL)
    (gimp-selection-none theImage)
    (if (= inSmooth TRUE)
        (script-fu-tile-blur theImage thickLayer theBlur TRUE TRUE FALSE)
    )


    (gimp-context-set-background inColor3)
    (gimp-by-color-select thinLayer '(0 0 0) 127 CHANNEL-OP-REPLACE  TRUE FALSE 0 FALSE)
    (gimp-edit-clear thinLayer)
    (gimp-selection-invert theImage)
    (gimp-edit-fill thinLayer BACKGROUND-FILL)
    (gimp-selection-none theImage)
    (if (= inSmooth TRUE)
        (script-fu-tile-blur theImage thinLayer (/ theBlur 2) TRUE TRUE FALSE)
    )


    (if (= inFlatten TRUE)
        (gimp-image-flatten theImage)
    )

    (gimp-display-new theImage)

    (gimp-context-pop)
  )
)


; Register the function with GIMP:

(script-fu-register
  "script-fu-camo-pattern"
  _"_Camouflage..."
  _"Create an image filled with a camouflage pattern"
  "Chris Gutteridge: cjg@ecs.soton.ac.uk"
  "28th April 1998"
  "Chris Gutteridge / ECS @ University of Southampton, England"
  ""
  SF-ADJUSTMENT _"Image size"    '(256 10 1000 1 10 0 1)
  SF-ADJUSTMENT _"Granularity"   '(7 0 15 1 1 0 0)
  SF-COLOR      _"Color 1"       '(33 100 58)
  SF-COLOR      _"Color 2"       '(170 170 60)
  SF-COLOR      _"Color 3"       '(150 115 100)
  SF-TOGGLE     _"Smooth"        FALSE
  SF-TOGGLE     _"Flatten image" TRUE
)


(script-fu-menu-register "script-fu-camo-pattern"
                         "<Image>/File/Create/Patterns")
