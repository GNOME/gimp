; LIGMA - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; script-fu-paste-as-pattern
; Based on select-to-pattern by Cameron Gregory, http://www.flamingtext.com/
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


(define (script-fu-paste-as-pattern name filename)
  (let* ((pattern-image (car (ligma-edit-paste-as-new-image)))
         (pattern-draw 0)
         (path 0))

    (if (= TRUE (car (ligma-image-is-valid pattern-image)))
      (begin
        (set! pattern-draw (aref (cadr (ligma-image-get-selected-drawables pattern-image)) 0))
        (set! path (string-append ligma-directory
                             "/patterns/"
                             filename
                             (number->string pattern-image)
                             ".pat"))

        (file-pat-save RUN-NONINTERACTIVE
                       pattern-image
                       1 (vector pattern-draw)
                       path
                       name)

        (ligma-image-delete pattern-image)

        (ligma-patterns-refresh)
        (ligma-context-set-pattern name)
      )
      (ligma-message _"There is no image data in the clipboard to paste.")
    )
  )
)

(script-fu-register "script-fu-paste-as-pattern"
  _"Paste as New _Pattern..."
  _"Paste the clipboard contents into a new pattern"
  "Michael Natterer <mitch@ligma.org>"
  "Michael Natterer"
  "2005-09-25"
  ""
  SF-STRING _"_Pattern name" "My Pattern"
  SF-STRING _"_File name"    "mypattern"
)

(script-fu-menu-register "script-fu-paste-as-pattern"
                         "<Image>/Edit/Paste as")
