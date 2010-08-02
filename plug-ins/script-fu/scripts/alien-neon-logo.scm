; GIMP - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; alien-neon-logo.scm - creates multiple outlines around the letters
; Copyright (C) 1999-2000 Raphael Quinet <quinet@gamers.org>
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
; 1999-12-01 First version.
; 2000-02-19 Do not discard the layer mask so that it can still be edited.
; 2000-03-08 Adapted the script to my gimp-edit-fill changes.
; 2000-04-02 Split the script in two parts: one using text, one using alpha.
; 2000-05-29 More modifications for "Alpha to Logo" using a separate function.
;
; To do: use a channel mask for creating the bands, instead of working in the
; image.  gimp-invert would then work on one grayscale channel instead of
; wasting CPU cycles on three identical R, G, B channels.
;

(define (apply-alien-neon-logo-effect img
                                      logo-layer
                                      fg-color
                                      bg-color
                                      band-size
                                      gap-size
                                      num-bands
                                      do-fade)
  (let* (
        (fade-size (- (* (+ band-size gap-size) num-bands) 1))
        (width (car (gimp-drawable-width logo-layer)))
        (height (car (gimp-drawable-height logo-layer)))
        (bg-layer (car (gimp-layer-new img width height RGB-IMAGE "Background" 100 NORMAL-MODE)))
        (bands-layer (car (gimp-layer-new img width height RGBA-IMAGE "Bands" 100 NORMAL-MODE)))
        )

    (gimp-context-push)

    (script-fu-util-image-resize-from-layer img logo-layer)
    (script-fu-util-image-add-layers img bands-layer bg-layer)
    (gimp-selection-none img)
    (gimp-context-set-background bg-color)
    (gimp-edit-fill bg-layer BACKGROUND-FILL)
    (gimp-context-set-background '(0 0 0))
    (gimp-edit-fill bands-layer BACKGROUND-FILL)
    ; The text layer is never shown: it is only used to create a selection
    (gimp-selection-layer-alpha logo-layer)
    (gimp-context-set-foreground '(255 255 255))
    (gimp-edit-fill bands-layer FOREGROUND-FILL)

    ; Create multiple outlines by growing and inverting the selection
    ; The bands are black and white because they will be used as a mask.
    (while (> num-bands 0)
      (gimp-selection-grow img band-size)
      (gimp-invert bands-layer)
      (gimp-selection-grow img gap-size)
      (gimp-invert bands-layer)
      (set! num-bands (- num-bands 1))
    )

    ; The fading effect is obtained by masking the image with a gradient.
    ; The gradient is created by filling a bordered selection (white->black).
    (if (= do-fade TRUE)
        (let ((bands-layer-mask (car (gimp-layer-create-mask bands-layer
                                                             ADD-BLACK-MASK))))
          (gimp-layer-add-mask bands-layer bands-layer-mask)
          (gimp-selection-layer-alpha logo-layer)
          (gimp-selection-border img fade-size)
          (gimp-edit-fill bands-layer-mask FOREGROUND-FILL)
          (gimp-layer-remove-mask bands-layer MASK-APPLY)))

    ; Transfer the resulting grayscale bands into the layer mask.
    (let ((bands-layer-mask (car (gimp-layer-create-mask bands-layer
                                                         ADD-BLACK-MASK))))
      (gimp-layer-add-mask bands-layer bands-layer-mask)
      (gimp-selection-none img)
      (gimp-edit-copy bands-layer)
      (gimp-floating-sel-anchor (car (gimp-edit-paste bands-layer-mask
                                                      FALSE))))

    ; Fill the layer with the foreground color.  The areas that are not
    ; masked become visible.
    (gimp-context-set-foreground fg-color)
    (gimp-edit-fill bands-layer FOREGROUND-FILL)
    ;; (gimp-layer-remove-mask bands-layer MASK-APPLY)

    ; Clean up and exit.
    (gimp-item-set-visible logo-layer 0)
    (gimp-image-set-active-layer img bands-layer)
    (gimp-displays-flush)

    (gimp-context-pop)
  )
)


(define (script-fu-alien-neon-logo-alpha img
                                         logo-layer
                                         fg-color
                                         bg-color
                                         band-size
                                         gap-size
                                         num-bands
                                         do-fade)
 (begin
    (gimp-image-undo-group-start img)
    (apply-alien-neon-logo-effect img logo-layer fg-color bg-color
                                  band-size gap-size num-bands do-fade)
    (gimp-image-undo-group-end img)
    (gimp-displays-flush)
  )
)

(script-fu-register "script-fu-alien-neon-logo-alpha"
    _"Alien _Neon..."
    _"Add psychedelic outlines to the selected region (or alpha)"
    "Raphael Quinet (quinet@gamers.org)"
    "Raphael Quinet"
    "1999-2000"
    "RGBA"
    SF-IMAGE      "Image"             0
    SF-DRAWABLE   "Drawable"          0
    SF-COLOR      _"Glow color"       "green"
    SF-COLOR      _"Background color" "black"
    SF-ADJUSTMENT _"Width of bands"   '(2 1 60 1 10 0 0)
    SF-ADJUSTMENT _"Width of gaps"    '(2 1 60 1 10 0 0)
    SF-ADJUSTMENT _"Number of bands"  '(7 1 100 1 10 0 1)
    SF-TOGGLE     _"Fade away"        TRUE
)

(script-fu-menu-register "script-fu-alien-neon-logo-alpha"
                         "<Image>/Filters/Alpha to Logo")


(define (script-fu-alien-neon-logo text
                                   size
                                   fontname
                                   fg-color
                                   bg-color
                                   band-size
                                   gap-size
                                   num-bands
                                   do-fade)
  (let* (
        (img (car (gimp-image-new 256 256 RGB)))
        (fade-size (- (* (+ band-size gap-size) num-bands) 1))
        (text-layer (car (gimp-text-fontname img -1 0 0 text (+ fade-size 10) TRUE size PIXELS fontname)))
        )
    (gimp-image-undo-disable img)
    (apply-alien-neon-logo-effect img text-layer fg-color bg-color
                                  band-size gap-size num-bands do-fade)
    (gimp-image-undo-enable img)
    (gimp-display-new img)
  )
)

(script-fu-register "script-fu-alien-neon-logo"
  _"Alien _Neon..."
  _"Create a logo with psychedelic outlines around the text"
  "Raphael Quinet (quinet@gamers.org)"
  "Raphael Quinet"
  "1999-2000"
  ""
  SF-STRING     _"Text"               "GIMP"
  SF-ADJUSTMENT _"Font size (pixels)" '(150 2 1000 1 10 0 1)
  SF-FONT       _"Font"               "Blippo"
  SF-COLOR      _"Glow color"         "green"
  SF-COLOR      _"Background color"   "black"
  SF-ADJUSTMENT _"Width of bands"     '(2 1 60 1 10 0 0)
  SF-ADJUSTMENT _"Width of gaps"      '(2 1 60 1 10 0 0)
  SF-ADJUSTMENT _"Number of bands"    '(7 1 100 1 10 0 1)
  SF-TOGGLE     _"Fade away" TRUE
)

(script-fu-menu-register "script-fu-alien-neon-logo"
                         "<Image>/File/Create/Logos")
