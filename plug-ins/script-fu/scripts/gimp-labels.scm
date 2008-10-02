; GIMP - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; www.gimp.org web labels
; Copyright (c) 1997 Adrian Likins
; aklikins@eos.ncsu.edu
;
; based on a idea by jtl (Jens  Lautenbacher)
; and improved by jtl
;
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
;
;  **NOTE**  This fonts use some very common fonts, that are typically
;  bitmap fonts on most installations. If you want better quality versions
;  you need to grab the urw font package form www.gimp.org/fonts.html
;  and install as indicated. This will replace the some current bitmap fonts
;  with higher quality vector fonts. This is how the actual www.gimp.org
;  logos were created.
;
; ************************************************************************
; Changed on Feb 4, 1999 by Piet van Oostrum <piet@cs.uu.nl>
; For use with GIMP 1.1.
; All calls to gimp-text-* have been converted to use the *-fontname form.
; The corresponding parameters have been replaced by an SF-FONT parameter.
; ************************************************************************


(define (script-fu-labels-gimp-org text font font-size text-color
                                   shadow-color bg-color rm-bg index
                                   num-colors color-thresh yoff xoff height)
  (let* (
        (img (car (gimp-image-new 125 height RGB)))
        (text-layer (car (gimp-text-fontname img -1 xoff yoff text 0
                                    TRUE font-size PIXELS
                                    font)))
        (bg-layer (car (gimp-layer-new  img 125 height
                                        RGB-IMAGE "Background" 100 NORMAL-MODE)))
        (shadow-layer (car (gimp-layer-copy text-layer TRUE)))
        )

    (gimp-context-push)

    (gimp-image-undo-disable img)
    (gimp-image-add-layer img shadow-layer 1)
    (gimp-image-add-layer img bg-layer 2)

    (gimp-layer-set-lock-alpha text-layer TRUE)
    (gimp-layer-set-lock-alpha shadow-layer TRUE)

    (gimp-context-set-background text-color)
    (gimp-edit-fill text-layer BACKGROUND-FILL)

    (gimp-context-set-background bg-color)
    (gimp-edit-fill bg-layer BACKGROUND-FILL)

    (gimp-context-set-background shadow-color)
    (gimp-edit-fill shadow-layer BACKGROUND-FILL)
    (gimp-layer-translate shadow-layer 1 1)

    (set! text-layer (car (gimp-image-flatten img)))
    (gimp-layer-add-alpha text-layer)

    (if (= rm-bg TRUE)
        (begin
          (gimp-by-color-select text-layer bg-color
                                color-thresh CHANNEL-OP-REPLACE TRUE FALSE 0 FALSE)
          (gimp-edit-clear text-layer)
          (gimp-selection-none img)
        )
    )

    (if (= index TRUE)
        (gimp-image-convert-indexed img FS-DITHER MAKE-PALETTE num-colors
                                    FALSE FALSE "")
    )

    (gimp-image-undo-enable img)
    (gimp-display-new img)

    (gimp-context-pop)
  )
)


;;;(define (script-fu-tube-button-label-gimp-org text rm-bg index)
;;;  (script-fu-labels-gimp-org text "nimbus sans" 14 "medium" "r" "normal" '(151 177 192) '(0 0 0) '(255 255 255) rm-bg index 15 1 8 0 30))
;;;
;;;(define (script-fu-tube-subbutton-label-gimp-org text rm-bg index)
;;;  (script-fu-labels-gimp-org text "nimbus sans" 12 "medium" "r" "normal" '(151 177 192) '(0 0 0) '(255 255 255) rm-bg index 15 1 7 0 24))
;;;
;;;(define (script-fu-tube-subsubbutton-label-gimp-org text rm-bg index)
;;;  (script-fu-labels-gimp-org text "nimbus sans" 10 "medium" "r" "normal" '(151 177 192) '(0 0 0) '(255 255 255) rm-bg index 15 1 6 0 18))
;;;

(define (script-fu-tube-button-label-gimp-org text rm-bg index)
  (script-fu-labels-gimp-org text "helvetica" 14 '(86 114 172) '(255 255 255) '(255 255 255) rm-bg index 15 1 8 0 30)
)

(define (script-fu-tube-subbutton-label-gimp-org text rm-bg index)
  (script-fu-labels-gimp-org text "helvetica" 12 '(86 114 172) '(255 255 255) '(255 255 255) rm-bg index 15 1 7 10 24)
)

(define (script-fu-tube-subsubbutton-label-gimp-org text rm-bg index)
  (script-fu-labels-gimp-org text "helvetica" 10 '(86 114 172) '(255 255 255) '(255 255 255) rm-bg index 15 1 6 20 18)
)


(script-fu-register "script-fu-tube-button-label-gimp-org"
  _"_Tube Button Label..."
  _"Create an image of a Tube Button Label using the gimp.org webpage theme"
  "Adrian Likins & Jens Lautenbacher"
  "Adrian Likins & Jens Lautenbacher"
  "1997"
  ""
  SF-STRING _"Text"              "?"
  SF-TOGGLE _"Remove background" TRUE
  SF-TOGGLE _"Index image"       TRUE
)

(script-fu-menu-register "script-fu-tube-button-label-gimp-org"
                         "<Image>/File/Create/Web Page Themes/Classic.Gimp.Org")


(script-fu-register "script-fu-tube-subbutton-label-gimp-org"
  _"T_ube Sub-Button Label..."
  _"Create an image of a second level Tube Button Label using the gimp.org webpage theme"
  "Adrian Likins & Jens Lautenbacher"
  "Adrian Likins & Jens Lautenbacher"
  "1997"
  ""
  SF-STRING _"Text"              "?"
  SF-TOGGLE _"Remove background" TRUE
  SF-TOGGLE _"Index image"       TRUE
)

(script-fu-menu-register "script-fu-tube-subbutton-label-gimp-org"
                         "<Image>/File/Create/Web Page Themes/Classic.Gimp.Org")


(script-fu-register "script-fu-tube-subsubbutton-label-gimp-org"
  _"Tub_e Sub-Sub-Button Label..."
  _"Create an image of a third level Tube Button Label using the gimp.org webpage theme"
  "Adrian Likins & Jens Lautenbacher"
  "Adrian Likins & Jens Lautenbacher"
  "1997"
  ""
  SF-STRING _"Text"              "?"
  SF-TOGGLE _"Remove background" TRUE
  SF-TOGGLE _"Index image"       TRUE
)

(script-fu-menu-register "script-fu-tube-subsubbutton-label-gimp-org"
                         "<Image>/File/Create/Web Page Themes/Classic.Gimp.Org")


(script-fu-register "script-fu-labels-gimp-org"
  _"_General Tube Labels..."
  _"Create an image of a Tube Button Label Header using the gimp.org webpage theme"
  "Adrian Likins & Jens Lautenbacher"
  "Adrian Likins & Jens Lautenbacher"
  "1997"
  ""
  SF-STRING     _"Text"               "Gimp.Org"
  SF-FONT       _"Font"               "Sans"
  SF-ADJUSTMENT _"Font size (pixels)" '(18 2 1000 1 10 0 1)
  SF-COLOR      _"Text color"         '(130 165 235)
  SF-COLOR      _"Shadow color"       "black"
  SF-COLOR      _"Background color"   "white"
  SF-TOGGLE     _"Remove background"  TRUE
  SF-TOGGLE     _"Index image"        TRUE
  SF-ADJUSTMENT _"Number of colors"   '(15 2 255 1 10 0 1)
  SF-ADJUSTMENT _"Select-by-color threshold" '(1 1 256 1 10 0 1)
  SF-ADJUSTMENT _"Offset X"           '(8 0 50 1 10 0 1)
  SF-ADJUSTMENT _"Offset Y"           '(0 0 50 1 10 0 1)
  SF-ADJUSTMENT _"Height"             '(30 2 1000 1 10 0 1)
)

(script-fu-menu-register "script-fu-labels-gimp-org"
                         "<Image>/File/Create/Web Page Themes/Classic.Gimp.Org")
