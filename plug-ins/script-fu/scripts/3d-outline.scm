; 3d-outlined-patterned-shadowed-and-bump-mapped-logo :)
; creates outlined border of a text with patterns
;
; GIMP - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; 3d-outline creates outlined border of a text with patterns
; Copyright (C) 1998 Hrvoje Horvat
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

(define (apply-3d-outline-logo-effect img
                                      logo-layer
                                      text-pattern
                                      outline-blur-radius
                                      shadow-blur-radius
                                      bump-map-blur-radius
                                      noninteractive
                                      s-offset-x
                                      s-offset-y)
  (let* (
        (width (car (gimp-drawable-width logo-layer)))
        (height (car (gimp-drawable-height logo-layer)))
        (bg-layer (car (gimp-layer-new img width height
                                       RGB-IMAGE "Background" 100 NORMAL-MODE)))
        (pattern-layer (car (gimp-layer-new img width height
                                       RGBA-IMAGE "Pattern" 100 NORMAL-MODE)))
        (alpha-layer 0)
        (shadow-layer 0)
        (pattern-mask 0)
        (floating-sel 0)
        )

    (gimp-context-push)

    (gimp-selection-none img)
    (script-fu-util-image-resize-from-layer img logo-layer)
    (script-fu-util-image-add-layers img pattern-layer bg-layer)
    (gimp-context-set-background '(255 255 255))
    (gimp-edit-fill bg-layer BACKGROUND-FILL)
    (gimp-edit-clear pattern-layer)
    (gimp-layer-set-lock-alpha logo-layer TRUE)
    (gimp-context-set-foreground '(0 0 0))
    (gimp-edit-fill logo-layer FOREGROUND-FILL)
    (gimp-layer-set-lock-alpha logo-layer FALSE)
    (plug-in-gauss-iir RUN-NONINTERACTIVE img logo-layer outline-blur-radius TRUE TRUE)

    (gimp-item-set-visible pattern-layer FALSE)
    (set! alpha-layer (car (gimp-image-merge-visible-layers img CLIP-TO-IMAGE)))
    (plug-in-edge RUN-NONINTERACTIVE img alpha-layer 2 1 0)
    (gimp-item-set-name alpha-layer "Bump map")
    (set! shadow-layer (car (gimp-layer-copy alpha-layer TRUE)))
    (gimp-item-set-name shadow-layer "Edges")
    (script-fu-util-image-add-layers img shadow-layer)
    (plug-in-gauss-iir RUN-NONINTERACTIVE img alpha-layer bump-map-blur-radius TRUE TRUE)

    (gimp-selection-all img)
    (gimp-context-set-pattern text-pattern)
    (gimp-edit-bucket-fill pattern-layer
                           PATTERN-BUCKET-FILL NORMAL-MODE 100 0 FALSE 0 0)
    (plug-in-bump-map noninteractive img pattern-layer alpha-layer
                      110.0 45.0 4 0 0 0 0 TRUE FALSE 0)

    (set! pattern-mask (car (gimp-layer-create-mask pattern-layer ADD-ALPHA-MASK)))
    (gimp-layer-add-mask pattern-layer pattern-mask)

    (gimp-selection-all img)
    (gimp-edit-copy shadow-layer)
    (set! floating-sel (car (gimp-edit-paste pattern-mask FALSE)))
    (gimp-floating-sel-anchor floating-sel)

    (gimp-layer-remove-mask pattern-layer MASK-APPLY)
    (gimp-invert shadow-layer)
    (gimp-item-set-name shadow-layer "Drop shadow")
    (plug-in-gauss-iir RUN-NONINTERACTIVE img shadow-layer shadow-blur-radius TRUE TRUE)

    (gimp-drawable-offset shadow-layer
                          FALSE OFFSET-BACKGROUND s-offset-x s-offset-y)

    (gimp-item-set-visible alpha-layer FALSE)
    (gimp-item-set-visible pattern-layer TRUE)
    ;;(set! final (car (gimp-image-flatten img)))

    (gimp-context-pop)
  )
)

(define (script-fu-3d-outline-logo-alpha img
                                         logo-layer
                                         text-pattern
                                         outline-blur-radius
                                         shadow-blur-radius
                                         bump-map-blur-radius
                                         noninteractive
                                         s-offset-x
                                         s-offset-y)
  (begin
    (gimp-image-undo-group-start img)
    (apply-3d-outline-logo-effect img logo-layer text-pattern
                                  outline-blur-radius shadow-blur-radius
                                  bump-map-blur-radius noninteractive
                                  s-offset-x s-offset-y)
    (gimp-image-undo-group-end img)
    (gimp-displays-flush)
  )
)

(script-fu-register "script-fu-3d-outline-logo-alpha"
  _"3D _Outline..."
  _"Outline the selected region (or alpha) with a pattern and add a drop shadow"
  "Hrvoje Horvat (hhorvat@open.hr)"
  "Hrvoje Horvat"
  "07 April, 1998"
  "RGBA"
  SF-IMAGE       "Image"               0
  SF-DRAWABLE    "Drawable"            0
  SF-PATTERN    _"Pattern"             "Parque #1"
  SF-ADJUSTMENT _"Outline blur radius" '(5 1 200 1 10 0 1)
  SF-ADJUSTMENT _"Shadow blur radius"  '(10 1 200 1 10 0 1)
  SF-ADJUSTMENT _"Bumpmap (alpha layer) blur radius" '(5 1 200 1 10 0 1)
  SF-TOGGLE     _"Default bumpmap settings" TRUE
  SF-ADJUSTMENT _"Shadow X offset"     '(0 0 200 1 5 0 1)
  SF-ADJUSTMENT _"Shadow Y offset"     '(0 0 200 1 5 0 1)
)

(script-fu-menu-register "script-fu-3d-outline-logo-alpha"
                         "<Image>/Filters/Alpha to Logo")


(define (script-fu-3d-outline-logo text-pattern
                                   text
                                   size
                                   font
                                   outline-blur-radius
                                   shadow-blur-radius
                                   bump-map-blur-radius
                                   noninteractive
                                   s-offset-x
                                   s-offset-y)
  (let* (
        (img (car (gimp-image-new 256 256 RGB)))
        (text-layer (car (gimp-text-fontname img -1 0 0 text 30 TRUE size PIXELS font)))
        )
    (gimp-image-undo-disable img)
    (apply-3d-outline-logo-effect img text-layer text-pattern
                                  outline-blur-radius shadow-blur-radius
                                  bump-map-blur-radius noninteractive
                                  s-offset-x s-offset-y)
    (gimp-image-undo-enable img)
    (gimp-display-new img)
  )
)

(script-fu-register "script-fu-3d-outline-logo"
  _"3D _Outline..."
  _"Create a logo with outlined text and a drop shadow"
  "Hrvoje Horvat (hhorvat@open.hr)"
  "Hrvoje Horvat"
  "07 April, 1998"
  ""
  SF-PATTERN    _"Pattern"             "Parque #1"
  SF-STRING     _"Text"                "GIMP"
  SF-ADJUSTMENT _"Font size (pixels)"  '(100 2 1000 1 10 0 1)
  SF-FONT       _"Font"                "RoostHeavy"
  SF-ADJUSTMENT _"Outline blur radius" '(5 1 200 1 10 0 1)
  SF-ADJUSTMENT _"Shadow blur radius"  '(10 1 200 1 10 0 1)
  SF-ADJUSTMENT _"Bumpmap (alpha layer) blur radius" '(5 1 200 1 10 0 1)
  SF-TOGGLE     _"Default bumpmap settings" TRUE
  SF-ADJUSTMENT _"Shadow X offset"     '(0 0 200 1 5 0 1)
  SF-ADJUSTMENT _"Shadow Y offset"     '(0 0 200 1 5 0 1)
)

(script-fu-menu-register "script-fu-3d-outline-logo"
                         "<Image>/File/Create/Logos")
