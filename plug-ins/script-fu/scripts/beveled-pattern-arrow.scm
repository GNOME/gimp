; GIMP - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; Beveled pattern arrow for web pages
; Copyright (C) 1997 Federico Mena Quintero
; federico@nuclecu.unam.mx
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


(define (script-fu-beveled-pattern-arrow size orientation pattern)

  (define (make-point x y)
    (cons x y)
  )

  (define (point-x p)
    (car p)
  )

  (define (point-y p)
    (cdr p)
  )

  (define (point-list->double-array point-list)
    (let* (
          (how-many (length point-list))
          (a (cons-array (* 2 how-many) 'double))
          (count 0)
          )

      (for-each (lambda (p)
                  (aset a (* count 2) (point-x p))
                  (aset a (+ 1 (* count 2)) (point-y p))
                  (set! count (+ count 1)))
                point-list
      )
      a
    )
  )

  (define (rotate-points points size orientation)
    (map (lambda (p)
           (let ((px (point-x p))
                 (py (point-y p)))
             (cond ((= orientation 0) (make-point px py))           ; right
                   ((= orientation 1) (make-point (- size px) py))  ; left
                   ((= orientation 2) (make-point py (- size px)))  ; up
                   ((= orientation 3) (make-point py px))           ; down
             )
           )
         )
         points
    )
  )

  (define (make-arrow size offset)
    (list (make-point offset offset)
          (make-point (- size offset) (/ size 2))
          (make-point offset (- size offset)))
  )

  ; the main function

  (let* (
        (img (car (gimp-image-new size size RGB)))
        (background (car (gimp-layer-new img size size RGB-IMAGE _"Arrow" 100 NORMAL-MODE)))
        (bumpmap (car (gimp-layer-new img size size RGB-IMAGE _"Bumpmap" 100 NORMAL-MODE)))
        (big-arrow (point-list->double-array (rotate-points (make-arrow size 6) size orientation)))
        (med-arrow (point-list->double-array (rotate-points (make-arrow size 7) size orientation)))
        (small-arrow (point-list->double-array (rotate-points (make-arrow size 8) size orientation)))
        )

    (gimp-context-push)

    (gimp-image-undo-disable img)
    (gimp-image-insert-layer img background -1 -1)
    (gimp-image-insert-layer img bumpmap -1 -1)

    ; Create pattern layer

    (gimp-context-set-background '(0 0 0))
    (gimp-edit-fill background BACKGROUND-FILL)
    (gimp-context-set-pattern pattern)
    (gimp-edit-bucket-fill background PATTERN-BUCKET-FILL NORMAL-MODE 100 0 FALSE 0 0)

    ; Create bumpmap layer

    (gimp-edit-fill bumpmap BACKGROUND-FILL)

    (gimp-context-set-background '(127 127 127))
    (gimp-rect-select img 1 1 (- size 2) (- size 2) CHANNEL-OP-REPLACE FALSE 0)
    (gimp-edit-fill bumpmap BACKGROUND-FILL)

    (gimp-context-set-background '(255 255 255))
    (gimp-rect-select img 2 2 (- size 4) (- size 4) CHANNEL-OP-REPLACE FALSE 0)
    (gimp-edit-fill bumpmap BACKGROUND-FILL)

    (gimp-context-set-background '(127 127 127))
    (gimp-free-select img 6 big-arrow CHANNEL-OP-REPLACE TRUE FALSE 0)
    (gimp-edit-fill bumpmap BACKGROUND-FILL)

    (gimp-context-set-background '(0 0 0))
    (gimp-free-select img 6 med-arrow CHANNEL-OP-REPLACE TRUE FALSE 0)
    (gimp-edit-fill bumpmap BACKGROUND-FILL)

    (gimp-selection-none img)

    ; Bumpmap

    (plug-in-bump-map RUN-NONINTERACTIVE img background bumpmap 135 45 2 0 0 0 0 TRUE FALSE 0)

    ; Darken arrow

    (gimp-context-set-background '(255 255 255))
    (gimp-edit-fill bumpmap BACKGROUND-FILL)

    (gimp-context-set-background '(192 192 192))
    (gimp-free-select img 6 small-arrow CHANNEL-OP-REPLACE TRUE FALSE 0)
    (gimp-edit-fill bumpmap BACKGROUND-FILL)

    (gimp-selection-none img)

    (gimp-layer-set-mode bumpmap MULTIPLY-MODE)

    (gimp-image-flatten img)

    (gimp-image-undo-enable img)
    (gimp-display-new img)

    (gimp-context-pop)
  )
)


(script-fu-register "script-fu-beveled-pattern-arrow"
  _"_Arrow..."
  _"Create a beveled pattern arrow for webpages"
  "Federico Mena Quintero"
  "Federico Mena Quintero"
  "July 1997"
  ""
  SF-ADJUSTMENT _"Size"        '(32 5 150 1 10 0 1)
  SF-OPTION     _"Orientation" '(_"Right" _"Left" _"Up" _"Down")
  SF-PATTERN    _"Pattern"     "Wood"
)

(script-fu-menu-register "script-fu-beveled-pattern-arrow"
                         "<Image>/File/Create/Web Page Themes/Beveled Pattern")
