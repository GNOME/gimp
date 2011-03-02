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


(define (script-fu-render-map inSize
                              inGrain
                              inGrad
                              inWiden)

  (let* (
        (theWidth inSize)
        (theHeight inSize)
        (theImage (car(gimp-image-new theWidth theHeight RGB)))
        (theLayer 0)
        (thinLayer 0)
        )

  (gimp-context-push)

  (gimp-selection-all theImage)

  (set! theLayer (car (gimp-layer-new theImage theWidth theHeight
                                      RGBA-IMAGE
                                      "I've got more rubber ducks than you!"
                                      100 NORMAL-MODE)))
  (gimp-image-insert-layer theImage theLayer 0 0)
  (plug-in-solid-noise RUN-NONINTERACTIVE
		       theImage theLayer 1 0 (rand 65536)
                       inGrain inGrain inGrain)

  (if (= inWiden 1)
      (begin
        (set! thinLayer (car (gimp-layer-new theImage theWidth theHeight
                                             RGBA-IMAGE "Camo Thin Layer"
                                             100 NORMAL-MODE)))
        (gimp-image-insert-layer theImage thinLayer 0 0)

        (let ((theBigGrain (min 15 (* 2 inGrain))))
          (plug-in-solid-noise RUN-NONINTERACTIVE
			       theImage thinLayer 1 0 (rand 65536)
                               theBigGrain theBigGrain theBigGrain))

        (gimp-context-set-background '(255 255 255))
        (gimp-context-set-foreground '(0 0 0))

        (let ((theMask (car (gimp-layer-create-mask thinLayer 0))))
          (gimp-layer-add-mask thinLayer theMask)

          (gimp-edit-blend theMask FG-BG-RGB-MODE NORMAL-MODE
                           GRADIENT-LINEAR 100 0 REPEAT-TRIANGULAR FALSE
                           FALSE 0 0 TRUE
                           0 0 0 (/ theHeight 2)))

        (set! theLayer (car(gimp-image-flatten theImage)))))

  (gimp-selection-none theImage)
  (gimp-context-set-gradient inGrad)
  (plug-in-gradmap RUN-NONINTERACTIVE theImage theLayer)

  (gimp-display-new theImage)

  (gimp-context-pop)
  )
)

(script-fu-register "script-fu-render-map"
  _"Render _Map..."
  _"Create an image filled with an Earth-like map pattern"
  "Chris Gutteridge: cjg@ecs.soton.ac.uk"
  "28th April 1998"
  "Chris Gutteridge / ECS @ University of Southampton, England"
  ""
  SF-ADJUSTMENT _"Image size"       '(256 0 2048 1 10 0 0)
  SF-ADJUSTMENT _"Granularity"      '(4 0 15 1 1 0 0)
  SF-GRADIENT   _"Gradient"         "Land and Sea"
  SF-TOGGLE     _"Gradient reverse" FALSE
  SF-OPTION     _"Behavior"         '(_"Tile" _"Detail in Middle")
)

(script-fu-menu-register "script-fu-render-map"
                         "<Image>/File/Create/Patterns")
