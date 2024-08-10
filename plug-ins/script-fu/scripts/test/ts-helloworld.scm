#!/usr/bin/env gimp-script-fu-interpreter-3.0

; "Hello, World" Test v1.00 February 29, 2004
; by Kevin Cozens <kcozens@interlog.com>
;
; Creates an image with the text "Hello, World!"
; This was the first TinyScheme based script ever created and run for the
; 2.x version of GIMP.

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
; Tiny-Fu first successfully ran this script at 2:07am on March 6, 2004.

(define (script-fu-helloworld text font size color)
  (let* (
        (width 10)
        (height 10)
        (img (car (gimp-image-new width height RGB)))
        (text-layer)
        )

    (gimp-context-push)

    (gimp-image-undo-disable img)
    (gimp-context-set-foreground color)

    (set! text-layer (car (gimp-text-font img -1 0 0 text 10 TRUE size font)))
    (set! width (car (gimp-drawable-get-width text-layer)))
    (set! height (car (gimp-drawable-get-height text-layer)))
    (gimp-image-resize img width height 0 0)

    (gimp-image-undo-enable img)
    (gimp-display-new img)

    (gimp-context-pop)
  )
)

(script-fu-register "script-fu-helloworld"
    "_Hello World..."
    "Creates an image of a text string."
    "Kevin Cozens <kcozens@interlog.com>"
    "Kevin Cozens"
    "February 29, 2004"
    ""
    SF-STRING     "Text string"         "Hello, World!"
    SF-FONT       "Font"                "Sans"
    SF-ADJUSTMENT "Font size (pixels)"  '(100 2 1000 1 10 0 1)
    SF-COLOR      "Color"               '(0 0 0)
)

(script-fu-menu-register "script-fu-helloworld"
                         "<Image>/Filters/Development/Demos")
