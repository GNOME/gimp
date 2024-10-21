; Set Colormap v1.1  September 29, 2004
; by Kevin Cozens <kcozens@interlog.com>
;
; Change the colormap of an image to the colors in a specified palette.
; Included is script-fu-make-cmap-array (available for use in scripts) which
; returns a GBytes containing the colors from a specified palette.
; This array can be used as the cmap argument for gimp-image-set-colormap.

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

(define (script-fu-make-cmap-array palette)
  (let* (
        (num-colors (car (gimp-palette-get-color-count palette)))
        ; cons-array is not Scheme standard
        ; but was in SIOD and is in script-fu-compat.init
        (cmap (cons-array (* num-colors 3) 'byte))
        (color 0)
        (i 0)
        )

    (while (< i num-colors)
      (set! color (car (gimp-palette-entry-get-color palette i)))
      (vector-set! cmap (* i 3) (car color))
      (vector-set! cmap (+ (* i 3) 1) (cadr color))
      (vector-set! cmap (+ (* i 3) 2) (caddr color))
      (set! i (+ i 1))
    )

    cmap
  )
)

(define (script-fu-set-cmap img drawables palette)
  (gimp-image-set-palette img palette)
  (gimp-displays-flush)
)

(script-fu-register-filter "script-fu-set-cmap"
    _"Se_t Colormap..."
    _"Change the colormap of an image to the colors in a specified palette."
    "Kevin Cozens <kcozens@interlog.com>"
    "Kevin Cozens"
    "September 29, 2004"
    "INDEXED*"
    SF-ONE-OR-MORE-DRAWABLE
    SF-PALETTE  _"Palette"  "Default"
)

(script-fu-menu-register "script-fu-set-cmap" "<Image>/Colors/Map/[Colormap]")
