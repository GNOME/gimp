;
; anim_sphere
;
;
; Chris Gutteridge (cjg@ecs.soton.ac.uk)
; At ECS Dept, University of Southampton, England.
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.
; 
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
; 
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


; Define the function:

(define (script-fu-spinning-globe inImage inLayer inFrames inFromLeft inTransparent inIndex inCopy)

        (set! theImage (if (= inCopy TRUE)
                       (car (gimp-channel-ops-duplicate inImage))
                       inImage)
        )
	(set! theLayer (car(gimp-image-get-active-layer theImage)))
	(gimp-layer-add-alpha theLayer)

        (set! n 0)
        (set! ang (* (/ 360 inFrames) (if (= inFromLeft TRUE) 1 -1) ))
(while (> inFrames n)
        (set! n (+ n 1))

        (set! theFrame (car (gimp-layer-copy theLayer FALSE)))
	(gimp-image-add-layer theImage theFrame 0)
        (gimp-layer-set-name theFrame (string-append "Anim Frame: " (number->string (- inFrames n) 10)))
	(plug-in-map-object TRUE theImage theFrame 1
;viewpoint
0.5 0.5 2.0
;obj pos?
0.5 0.5 0
;first axis
1.0 0 0
;2nd axis
0.0 1.0 0.0
; axis rotation
0 (* n ang) 0

; light
0 '(255 255 255)
; light pos
-0.5 -0.5 2.0

; light dir
-1.0 -1.0 1.0

; amb and stuff
.3 1 .5 0 27
TRUE FALSE FALSE inTransparent 0.25)
; end while:
)
	(gimp-image-remove-layer theImage theLayer)
        (plug-in-autocrop TRUE theImage theFrame)

        (if     (= inIndex 0)
                ()
                (gimp-convert-indexed theImage TRUE inIndex)
        )
        (if     (= inCopy TRUE)
                (begin  (gimp-image-clean-all theImage)
                        (gimp-display-new theImage)
                )
                ()
        )
        (gimp-displays-flush)
)

; Register the function with the GIMP:

(script-fu-register
    "script-fu-spinning-globe"
    "<Image>/Script-Fu/Animators/Spinning Globe"
    "foo"
    "Chris Gutteridge"
    "1998, Chris Gutteridge / ECS dept, University of Southampton, England."
    "16th April 1998"
    "RGB RGBA GRAY GRAYA"
    SF-IMAGE "The Image" 0
    SF-DRAWABLE "The Layer" 0
    SF-VALUE "Frames:" "10"
;    SF-ADJUSTMENT "Frames:" '(10 1 256 1 10 0 1)
    SF-TOGGLE "Turn from left to right?" FALSE
    SF-TOGGLE "Transparant Background?" TRUE
    SF-VALUE "Index of n Colors (0 = Remain RGB):" "63"
;    SF-ADJUSTMENT "Index of n Colors (0 = Remain RGB):" '(63 2 256 1 10 0 1)
    SF-TOGGLE "Work on Copy?" TRUE
)
