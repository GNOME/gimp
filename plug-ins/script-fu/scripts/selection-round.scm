; selection-rounded-rectangle.scm -*-scheme-*-

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

; CHANGE-LOG:
; 1.00 - initial release
; 1.01 - some code cleanup, no real changes
; 1.02 - made script undoable

; 2.00 - ALAN's Branch.  changed name, menu, location, and description
; 2.01 - fixed to work if there was no current selection.
; 2.02 - changed scale to percentages, usability tweaking.
; 2.10 - added concave round edges, updated description.
; 2.11 - tweeked description, changed comments, relinquished any rights.

; Copyright (C) 1997, 1998, Sven Neumann
; Copyright (C) 2004, Alan Horkan.
; Alan Horkan relinquishes all rights to his changes,
; full ownership of this script belongs to Sven Neumann.

(define (script-fu-selection-rounded-rectangle image drawable radius concave)
  (gimp-image-undo-group-start image)

  (if (= (car (gimp-selection-is-empty image)) TRUE) (gimp-selection-all image))
  (let* (
        (radius (/ radius 100)) ; convert from percentages
        (radius (min radius 1.0))
        (radius (max radius 0.0))
        (select-bounds (gimp-selection-bounds image))
        (has-selection (car select-bounds))
        (select-x1 (cadr select-bounds))
        (select-y1 (caddr select-bounds))
        (select-x2 (cadr (cddr select-bounds)))
        (select-y2 (caddr (cddr select-bounds)))
        (select-width (- select-x2 select-x1))
        (select-height (- select-y2 select-y1))
        (cut-radius 0)
        (ellipse-radius 0)
        )

    ;; select to the full bounds of the selection,
    ;; fills in irregular shapes or holes.
    (gimp-rect-select image
              select-x1 select-y1 select-width select-height
              CHANNEL-OP-ADD FALSE 0)

    (if (> select-width select-height)
      (set! cut-radius (trunc (+ 1 (* radius (/ select-height 2)))))
      (set! cut-radius (trunc (+ 1 (* radius (/ select-width 2)))))
    )
    (set! ellipse-radius (* cut-radius 2))

    ;; cut away rounded (concave) corners
    ; top right
    (gimp-ellipse-select image
             (- select-x1 cut-radius)
             (- select-y1 cut-radius)
             (* cut-radius 2)
             (* cut-radius 2)
             CHANNEL-OP-SUBTRACT
             TRUE
             FALSE 0)
    ; lower left
    (gimp-ellipse-select image
             (- select-x1 cut-radius)
             (- select-y2 cut-radius)
             (* cut-radius 2)
             (* cut-radius 2)
             CHANNEL-OP-SUBTRACT
             TRUE
             FALSE 0)
    ; top right
    (gimp-ellipse-select image
             (- select-x2 cut-radius)
             (- select-y1 cut-radius)
             (* cut-radius 2)
             (* cut-radius 2)
             CHANNEL-OP-SUBTRACT
             TRUE
             FALSE 0)
    ; bottom left
    (gimp-ellipse-select image
             (- select-x2 cut-radius)
             (- select-y2 cut-radius)
             (* cut-radius 2)
             (* cut-radius 2)
             CHANNEL-OP-SUBTRACT
             TRUE
             FALSE 0)

    ;; add in rounded (convex) corners
    (if (= concave FALSE)
      (begin
        (gimp-ellipse-select image
                     select-x1
                     select-y1
                     ellipse-radius
                     ellipse-radius
                     CHANNEL-OP-ADD
                     TRUE
                     FALSE 0)
        (gimp-ellipse-select image
                     select-x1
                     (- select-y2 ellipse-radius)
                     ellipse-radius
                     ellipse-radius
                     CHANNEL-OP-ADD
                     TRUE
                     FALSE 0)
        (gimp-ellipse-select image
                     (- select-x2 ellipse-radius)
                     select-y1
                     ellipse-radius
                     ellipse-radius
                     CHANNEL-OP-ADD
                     TRUE
                     FALSE 0)
        (gimp-ellipse-select image
                     (- select-x2 ellipse-radius)
                     (- select-y2 ellipse-radius)
                     ellipse-radius
                     ellipse-radius
                     CHANNEL-OP-ADD
                     TRUE
                     FALSE 0)
      )
    )

    (gimp-image-undo-group-end image)
    (gimp-displays-flush)
  )
)


(define (script-fu-selection-round image drawable radius)
  (script-fu-selection-rounded-rectangle image drawable (* radius 100) FALSE)
)


(script-fu-register "script-fu-selection-rounded-rectangle"
  _"Rounded R_ectangle..."
  _"Round the corners of the current selection"
  "Alan Horkan, Sven Neumann" ; authors
  "Sven Neumann"              ; copyright
  "2004/06/07"
  "*"
  SF-IMAGE       "Image"      0
  SF-DRAWABLE    "Drawable"   0
  SF-ADJUSTMENT _"Radius (%)" '(50 0 100 1 10 0 0)
  SF-TOGGLE     _"Concave"    FALSE
)

(script-fu-register "script-fu-selection-round"
  ""
  "Round the corners of the current selection (deprecated, use Rounded Rectangle)"
  "Sven Neumann"              ; authors
  "Sven Neumann"              ; copyright
  "1998/02/06"
  "*"
  SF-IMAGE       "Image"      0
  SF-DRAWABLE    "Drawable"   0
  SF-ADJUSTMENT  "Relative radius" '(1 0 128 0.1 1 1 1)
)

(script-fu-menu-register "script-fu-selection-rounded-rectangle"
                         "<Image>/Select/Modify")
