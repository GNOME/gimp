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
; chalk.scm  version 0.11  10/10/97
;
; Copyright (C) 1997 Manish Singh <msingh@uclink4.berkeley.edu>
;
; Makes a logo with a chalk-like text effect.

(define (apply-chalk-logo-effect img
                                 logo-layer
                                 bg-color)
  (let* (
        (width (car (gimp-drawable-width logo-layer)))
        (height (car (gimp-drawable-height logo-layer)))
        (bg-layer (car (gimp-layer-new img
                                       width height RGB-IMAGE
                                       "Background" 100 NORMAL-MODE)))
        )

    (gimp-context-push)

    (gimp-selection-none img)
    (script-fu-util-image-resize-from-layer img logo-layer)
    (script-fu-util-image-add-layers img bg-layer)
    (gimp-context-set-background bg-color)
    (gimp-edit-fill bg-layer BACKGROUND-FILL)

    ; the actual effect
    (gimp-layer-set-lock-alpha logo-layer FALSE)
    (plug-in-gauss-rle RUN-NONINTERACTIVE img logo-layer 2.0 1 1)
    (plug-in-spread RUN-NONINTERACTIVE img logo-layer 5.0 5.0)
    (plug-in-ripple RUN-NONINTERACTIVE img logo-layer 27 2 0 0 0 TRUE TRUE)
    (plug-in-ripple RUN-NONINTERACTIVE img logo-layer 27 2 1 0 0 TRUE TRUE)

    ; sobel doesn't work on a layer with transparency, so merge layers:
    (let ((logo-layer
           (car (gimp-image-merge-down img logo-layer EXPAND-AS-NECESSARY))))
      (plug-in-sobel RUN-NONINTERACTIVE img logo-layer TRUE TRUE TRUE)
      (gimp-levels logo-layer 0 0 120 3.5 0 255)

      ; work-around for sobel edge detect screw-up (why does this happen?)
      ; the top line of the image has some garbage instead of the bgcolor
      (gimp-rect-select img 0 0 width 1 CHANNEL-OP-ADD FALSE 0)
      (gimp-edit-clear logo-layer)
      )

    (gimp-selection-none img)

    (gimp-context-pop)
  )
)


(define (script-fu-chalk-logo-alpha img
                                    logo-layer
                                    bg-color)
  (begin
    (gimp-image-undo-group-start img)
    (apply-chalk-logo-effect img logo-layer bg-color)
    (gimp-image-undo-group-end img)
    (gimp-displays-flush)
  )
)

(script-fu-register "script-fu-chalk-logo-alpha"
  _"_Chalk..."
  _"Create a chalk drawing effect for the selected region (or alpha)"
  "Manish Singh <msingh@uclink4.berkeley.edu>"
  "Manish Singh"
  "October 1997"
  "RGBA"
  SF-IMAGE     "Image"            0
  SF-DRAWABLE  "Drawable"         0
  SF-COLOR    _"Background color" "black"
)

(script-fu-menu-register "script-fu-chalk-logo-alpha"
                         "<Image>/Filters/Alpha to Logo")


(define (script-fu-chalk-logo text
                              size
                              font
                              bg-color
                              chalk-color)
  (let* (
        (img (car (gimp-image-new 256 256 RGB)))
        (border (/ size 4))
        (text-layer (car (gimp-text-fontname img -1 0 0 text border TRUE size PIXELS font)))
        )

    (gimp-context-push)

    (gimp-image-undo-disable img)
    (gimp-context-set-foreground chalk-color)
    (gimp-layer-set-lock-alpha text-layer TRUE)
    (gimp-edit-fill text-layer FOREGROUND-FILL)
    (apply-chalk-logo-effect img text-layer bg-color)
    (gimp-image-undo-enable img)
    (gimp-display-new img)

    (gimp-context-pop)
  )
)

(script-fu-register "script-fu-chalk-logo"
  _"_Chalk..."
  _"Create a logo resembling chalk scribbled on a blackboard"
  "Manish Singh <msingh@uclink4.berkeley.edu>"
  "Manish Singh"
  "October 1997"
  ""
  SF-STRING     _"Text"               "CHALK"
  SF-ADJUSTMENT _"Font size (pixels)" '(150 2 1000 1 10 0 1)
  SF-FONT       _"Font"               "Sans"
  SF-COLOR      _"Background color"   "black"
  SF-COLOR      _"Chalk color"        "white"
)

(script-fu-menu-register "script-fu-chalk-logo"
                         "<Image>/File/Create/Logos")
