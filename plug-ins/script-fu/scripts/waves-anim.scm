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
; waves-anim.scm   version 1.01   1997/12/13
;
; CHANGE-LOG:
; 1.00 - initial release
; 1.01 - some code cleanup, no real changes
;
; Copyright (C) 1997 Sven Neumann <sven@gimp.org>
;
;
; Makes a copy of your image and creates an animation of the active layer
; as if a stone was thrown into the image. The animation may be saved with
; the gif-plug-in.

(define (script-fu-waves-anim img
                              drawable
                              amplitude
                              wavelength
                              num-frames
                              invert)
  (let* ((amplitude (max 0 amplitude))
         (wavelength (max 0 wavelength))
         (num-frames (max 1 num-frames))
         (remaining-frames num-frames)
         (phase 0)
         (phaseshift (/ 360 num-frames))
         (image (car (gimp-image-duplicate img)))
         (source-layer (car (gimp-image-get-active-layer image))))

  (gimp-image-undo-disable image)

  (if (= invert TRUE)
      (set! phaseshift (- 0 phaseshift)))

  (while (> remaining-frames 1)
    (let* (
          (waves-layer (car (gimp-layer-copy source-layer TRUE)))
          (layer-name (string-append "Frame "
                                     (number->string
                                       (- (+ num-frames 2)
                                          remaining-frames) 10
                                       )
                                     " (replace)"))
          )
    (gimp-layer-set-lock-alpha waves-layer FALSE)
    (gimp-image-add-layer image waves-layer -1)
    (gimp-item-set-name waves-layer layer-name)

    (plug-in-waves RUN-NONINTERACTIVE
                   image
                   waves-layer
                   amplitude
                   phase
                   wavelength
                   0
                   FALSE)

    (set! remaining-frames (- remaining-frames 1))
    (set! phase (- phase phaseshift))
    )
  )

  (gimp-item-set-name source-layer "Frame 1")
  (plug-in-waves RUN-NONINTERACTIVE
                 image
                 source-layer
                 amplitude
                 phase
                 wavelength
                 0
                 FALSE)

  (gimp-image-undo-enable image)
  (gimp-display-new image)
  )
)

(script-fu-register "script-fu-waves-anim"
  _"_Waves..."
  _"Create a multi-layer image with an effect like a stone was thrown into the current image"
  "Sven Neumann <sven@gimp.org>"
  "Sven Neumann"
  "1997/13/12"
  "RGB* GRAY*"
  SF-IMAGE       "Image" 0
  SF-DRAWABLE    "Drawable" 0
  SF-ADJUSTMENT _"Amplitude"        '(10 1   101 1 10 1 0)
  SF-ADJUSTMENT _"Wavelength"       '(10 0.1 100 1 10 1 0)
  SF-ADJUSTMENT _"Number of frames" '(6  1   512 1 10 0 1)
  SF-TOGGLE     _"Invert direction" FALSE
)

(script-fu-menu-register "script-fu-waves-anim"
                         "<Image>/Filters/Animation/Animators")
