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
                              drawables
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
         (source-layer (vector-ref (car (gimp-image-get-selected-layers image)) 0)))

  (gimp-image-undo-disable image)

  (if (= invert TRUE)
      (set! phaseshift (- 0 phaseshift)))

  (while (> remaining-frames 1)
    (let* (
          (waves-layer (car (gimp-layer-copy source-layer)))
          (layer-name (string-append "Frame "
                                     (number->string
                                       (- (+ num-frames 2)
                                          remaining-frames) 10
                                       )
                                     " (replace)"))
          (phi phase)
          (width (car (gimp-drawable-get-width waves-layer)))
          (height (car (gimp-drawable-get-height waves-layer)))
          (aspect (/ width height))
          )
    (gimp-layer-add-alpha waves-layer)
    (gimp-layer-set-lock-alpha waves-layer FALSE)
    (gimp-image-insert-layer image waves-layer 0 -1)
    (gimp-item-set-name waves-layer layer-name)

    (while (< phi 0)
      (set! phi (+ phi 360.0))
    )
    (set! phi (/ (- (modulo (round phase) 360.0) 180.0) 180.0))
    (if (< aspect 0.1)
      (set! aspect 0.1))
    (if (> aspect 10.0)
      (set! aspect 10.0))
    (gimp-drawable-merge-new-filter waves-layer "gegl:waves" 0 LAYER-MODE-REPLACE 1.0 "amplitude" amplitude "phi" phi
                                                                                      "period" (* wavelength 2.0)
                                                                                      "clamp" TRUE "aspect" aspect)

    (set! remaining-frames (- remaining-frames 1))
    (set! phase (- phase phaseshift))
    )
  )

  (gimp-item-set-name source-layer "Frame 1")

  (let* (
        (phi phase)
        (width (car (gimp-drawable-get-width source-layer)))
        (height (car (gimp-drawable-get-height source-layer)))
        (aspect (/ width height))
        )

    (while (< phi 0)
      (set! phi (+ phi 360.0))
    )
    (set! phi (/ (- (modulo (round phase) 360.0) 180.0) 180.0))

    (if (< aspect 0.1)
      (set! aspect 0.1))
    (if (> aspect 10.0)
      (set! aspect 10.0))
    (gimp-drawable-merge-new-filter source-layer "gegl:waves" 0 LAYER-MODE-REPLACE 1.0 "amplitude" amplitude "phi" phi
                                                                                       "period" (* wavelength 2.0)
                                                                                       "clamp" TRUE "aspect" aspect)
  )

  (gimp-image-undo-enable image)
  (gimp-display-new image)
  )
)

(script-fu-register-filter "script-fu-waves-anim"
  _"_Waves..."
  _"Create a multi-layer image with an effect like a stone was thrown into the current image"
  "Sven Neumann <sven@gimp.org>"
  "Sven Neumann"
  "1997/13/12"
  "RGB* GRAY*"
  SF-ONE-OR-MORE-DRAWABLE
  SF-ADJUSTMENT _"Amplitude"        '(10 1   101 1 10 1 0)
  SF-ADJUSTMENT _"Wavelength"       '(10 0.1 100 1 10 1 0)
  SF-ADJUSTMENT _"Number of frames" '(6  1   512 1 10 0 1)
  SF-TOGGLE     _"Invert direction" FALSE
)

(script-fu-menu-register "script-fu-waves-anim"
                         "<Image>/Filters/Animation/")
