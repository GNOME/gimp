; GIMP - The GNU Image Manipulation Program
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
  (script-fu-use-v3)
  (let* ((pattern-image (gimp-edit-paste-as-new-image))
         (path 0))

    (if (gimp-image-id-is-valid pattern-image)
      (begin
        (set! path (string-append gimp-directory
                             "/patterns/"
                             filename
                             (number->string pattern-image)
                             ".pat"))

        (file-pat-export RUN-NONINTERACTIVE
                       pattern-image
                       path
                       -1  ; NULL ExportOptions
                       name)

        (gimp-image-delete pattern-image)

        (gimp-patterns-refresh)
        (gimp-context-set-pattern (gimp-pattern-get-by-name name))
      )
      (gimp-message _"There is no image data in the clipboard to paste.")
    )
  )
)

(script-fu-register-procedure "script-fu-paste-as-pattern"
  _"Paste as New _Pattern..."
  _"Paste the clipboard contents into a new pattern"
  "Michael Natterer <mitch@gimp.org>"
  "2005-09-25"
  SF-STRING _"_Pattern name" _"My Pattern"
  SF-STRING _"_File name"    _"mypattern"
)

(script-fu-menu-register "script-fu-paste-as-pattern"
                         "<Image>/Edit/Paste as")
