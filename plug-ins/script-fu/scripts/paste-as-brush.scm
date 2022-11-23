; LIGMA - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; script-fu-paste-as-brush
; Based on select-to-brush by Copyright (c) 1997 Adrian Likins
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


(define (script-fu-paste-as-brush name filename spacing)

  (let* ((brush-image (car (ligma-edit-paste-as-new-image)))
         (brush-draw 0)
         (type 0)
         (path 0))

    (if (= TRUE (car (ligma-image-is-valid brush-image)))
      (begin
        (set! brush-draw (aref (cadr (ligma-image-get-selected-drawables brush-image)) 0))
        (set! type (car (ligma-drawable-type brush-draw)))
        (set! path (string-append ligma-directory
                                  "/brushes/"
                                  filename
                                  (number->string brush-image)
                                  ".gbr"))

        (if (= type GRAYA-IMAGE)
            (begin
                (ligma-context-push)
                (ligma-context-set-background '(255 255 255))
                (set! brush-draw (car (ligma-image-flatten brush-image)))
                (ligma-context-pop)
            )
        )

        (file-gbr-save RUN-NONINTERACTIVE
                       brush-image
                       1 (vector brush-draw)
                       path
                       spacing name)

        (ligma-image-delete brush-image)

        (ligma-brushes-refresh)
        (ligma-context-set-brush name)
      )
      (ligma-message _"There is no image data in the clipboard to paste.")
    )
  )
)

(script-fu-register "script-fu-paste-as-brush"
  _"Paste as New _Brush..."
  _"Paste the clipboard contents into a new brush"
  "Michael Natterer <mitch@ligma.org>"
  "Michael Natterer"
  "2005-09-25"
  ""
  SF-STRING     _"_Brush name" "My Brush"
  SF-STRING     _"_File name"  "mybrush"
  SF-ADJUSTMENT _"_Spacing"    '(25 0 1000 1 2 1 0)
)

(script-fu-menu-register "script-fu-paste-as-brush"
                         "<Image>/Edit/Paste as")
