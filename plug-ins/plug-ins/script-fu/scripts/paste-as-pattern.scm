; GIMP - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; script-fu-paste-as-pattern
; Based on select-to-pattern by Cameron Gregory, http://www.flamingtext.com/
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


(define (script-fu-paste-as-pattern name filename)
  (let* ((pattern-image (car (gimp-edit-paste-as-new)))
         (pattern-draw 0)
         (path 0))

    (if (= TRUE (car (gimp-image-is-valid pattern-image)))
      (begin
        (set! pattern-draw (car (gimp-image-get-active-drawable pattern-image)))
        (set! path (string-append gimp-directory
                             "/patterns/"
                             filename
                             (number->string pattern-image)
                             ".pat"))
       
        (file-pat-save RUN-NONINTERACTIVE
                       pattern-image pattern-draw path path
                       name)
       
        (gimp-image-delete pattern-image)
       
        (gimp-patterns-refresh)
        (gimp-context-set-pattern name)
      )
      (gimp-message _"There is no image data in the clipboard to paste.")
    )
  )
)

(script-fu-register "script-fu-paste-as-pattern"
  _"New _Pattern..."
  _"Paste the clipboard contents into a new pattern"
  "Michael Natterer <mitch@gimp.org>"
  "Michael Natterer"
  "2005-09-25"
  ""
  SF-STRING _"Pattern name" "My Pattern"
  SF-STRING _"File name"    "mypattern"
)

(script-fu-menu-register "script-fu-paste-as-pattern"
                         "<Image>/Edit/Paste as")
