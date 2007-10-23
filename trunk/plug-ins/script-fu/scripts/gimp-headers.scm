; GIMP - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; www.gimp.org web headers
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

(define (script-fu-headers-gimp-org text font font-size text-color high-color side-color shadow-color crop rm-bg index num-colors)
  (let* (
        (img (car (gimp-image-new 256 256 RGB)))
        (text-layer (car (gimp-text-fontname img -1 0 0
                                    text 30 TRUE font-size PIXELS
                                    font)))
        (width (car (gimp-drawable-width text-layer)))
        (height (car (gimp-drawable-height text-layer)))
        (bg-layer (car (gimp-layer-new img width height RGB-IMAGE "Background" 100 NORMAL-MODE)))
        )

    (gimp-context-push)

    (gimp-image-undo-disable img)
    (gimp-image-resize img width height 0 0)
    (gimp-image-add-layer img bg-layer 1)
    (gimp-layer-set-lock-alpha text-layer TRUE)
    (gimp-context-set-background text-color)
    (gimp-edit-fill text-layer BACKGROUND-FILL)

    (gimp-context-set-background '(255 255 255))
    (gimp-edit-fill bg-layer BACKGROUND-FILL)

    (let* (
          (highlight-layer (car (gimp-layer-copy text-layer TRUE)))
          (side-layer (car (gimp-layer-copy text-layer TRUE)))
          (shadow-layer (car (gimp-layer-copy text-layer TRUE)))
          )

      (gimp-image-add-layer img highlight-layer 1)
      (gimp-layer-set-lock-alpha highlight-layer TRUE)

      (gimp-image-add-layer img side-layer 1)
      (gimp-layer-set-lock-alpha side-layer TRUE)

      (gimp-image-add-layer img shadow-layer 1)
      (gimp-layer-set-lock-alpha shadow-layer TRUE)

      (gimp-context-set-background high-color)
      (gimp-edit-fill highlight-layer BACKGROUND-FILL)
      (gimp-layer-translate highlight-layer -1 -1)

      (gimp-context-set-background side-color)
      (gimp-edit-fill side-layer BACKGROUND-FILL)
      (gimp-layer-translate side-layer 1 1)

      (gimp-context-set-background shadow-color)
      (gimp-edit-fill shadow-layer BACKGROUND-FILL)
      (gimp-layer-translate shadow-layer 5 5)

      (gimp-layer-set-lock-alpha shadow-layer FALSE)
      (plug-in-gauss-rle RUN-NONINTERACTIVE img shadow-layer 5 TRUE TRUE)
      (gimp-layer-set-opacity shadow-layer 60)
      (gimp-image-lower-layer img shadow-layer)
      (gimp-image-lower-layer img shadow-layer)
    )


    (set! text-layer (car (gimp-image-flatten img)))
    (gimp-layer-add-alpha text-layer)


    (if (= rm-bg TRUE)
        (begin
          (gimp-by-color-select text-layer '(255 255 255)
                                1 CHANNEL-OP-REPLACE TRUE FALSE 0 FALSE)
          (gimp-edit-clear text-layer)
          (gimp-selection-none img)
        )
    )

    (if (= crop TRUE)
         (plug-in-autocrop RUN-NONINTERACTIVE img text-layer)
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


(define (script-fu-big-header-gimp-org text font font-size text-color
                                       high-color side-color shadow-color
                                       crop rm-bg index num-colors)
  (script-fu-headers-gimp-org (string-append " " text)
                               font font-size
                               text-color high-color side-color shadow-color
                               crop rm-bg index num-colors)
)


(define (script-fu-small-header-gimp-org text font font-size text-color
                                         high-color side-color shadow-color
                                         crop rm-bg index num-colors)
  (script-fu-headers-gimp-org text font
                              font-size text-color high-color
                              side-color shadow-color
                              crop rm-bg index num-colors)
)


(script-fu-register "script-fu-big-header-gimp-org"
  _"_Big Header..."
  _"Create an image of a large header using the gimp.org webpage theme"
  "Adrian Likins & Jens Lautenbacher"
  "Adrian Likins & Jens Lautenbacher"
  "1997"
  ""
  SF-STRING     _"Text"               "gimp.org"
  SF-FONT       _"Font"               "Serif"
  SF-ADJUSTMENT _"Font size (pixels)" '(50 2 1000 1 10 0 1)
  SF-COLOR      _"Text color"         '(82 108 159)
  SF-COLOR      _"Highlight color"    '(190 220 250)
  SF-COLOR      _"Dark color"         '(46 74 92)
  SF-COLOR      _"Shadow color"       "black"
  SF-TOGGLE     _"Autocrop"           TRUE
  SF-TOGGLE     _"Remove background"  TRUE
  SF-TOGGLE     _"Index image"        TRUE
  SF-ADJUSTMENT _"Number of colors"   '(15 2 255 1 10 0 1)
)

(script-fu-menu-register "script-fu-big-header-gimp-org"
                         "<Toolbox>/Xtns/Web Page Themes/Classic.Gimp.Org")

(script-fu-register "script-fu-small-header-gimp-org"
  _"_Small Header..."
  _"Create an image of a small header using the gimp.org webpage theme"
  "Adrian Likins & Jens Lautenbacher"
  "Adrian Likins & Jens Lautenbacher"
  "1997"
  ""
  SF-STRING     _"Text"               "gimp.org"
  SF-FONT       _"Font"               "Sans"
  SF-ADJUSTMENT _"Font size (pixels)" '(24 2 1000 1 10 0 1)
  SF-COLOR      _"Text color"         '(135 220 220)
  SF-COLOR      _"Highlight color"    '(210 240 245)
  SF-COLOR      _"Dark color"         '(46 74 92)
  SF-COLOR      _"Shadow color"       "black"
  SF-TOGGLE     _"Autocrop"           TRUE
  SF-TOGGLE     _"Remove background"  TRUE
  SF-TOGGLE     _"Index image"        TRUE
  SF-ADJUSTMENT _"Number of colors"   '(15 2 255 1 10 0 1)
  SF-ADJUSTMENT _"Select-by-color threshold" '(1 1 256 1 10 0 1)
)

(script-fu-menu-register "script-fu-small-header-gimp-org"
                         "<Toolbox>/Xtns/Web Page Themes/Classic.Gimp.Org")
