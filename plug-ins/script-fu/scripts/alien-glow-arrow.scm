; GIMP - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; Alien Glow themed arrows for web pages
; Copyright (c) 1997 Adrian Likins
; aklikins@eos.ncsu.edu
;
;
; Based on code from
; Federico Mena Quintero
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

(define (script-fu-alien-glow-right-arrow size
                                          orientation
                                          glow-color
                                          bg-color
                                          flatten)

  ; some local helper functions, better to not define globally,
  ; since otherwise the definitions could be clobbered by other scripts.
  (define (map proc seq)
    (if (null? seq)
        '()
        (cons (proc (car seq))
              (map proc (cdr seq))
        )
    )
  )

  (define (for-each proc seq)
    (if (not (null? seq))
        (begin
          (proc (car seq))
          (for-each proc (cdr seq))
        )
    )
  )

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
    (define (convert points array pos)
      (if (not (null? points))
          (begin
            (aset array (* 2 pos) (point-x (car points)))
            (aset array (+ 1 (* 2 pos)) (point-y (car points)))
            (convert (cdr points) array (+ pos 1))
          )
      )
    )

    (let* (
          (how-many (length point-list))
          (a (cons-array (* 2 how-many) 'double))
          )
      (convert point-list a 0)
      a
    )
  )

  (define (make-arrow size
                      offset)
    (list (make-point offset offset)
          (make-point (- size offset) (/ size 2))
          (make-point offset (- size offset))
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


  ; the main function

  (let* (
        (img (car (gimp-image-new size size RGB)))
        (grow-amount (/ size 12))
        (blur-radius (/ size 3))
        (offset (/ size 6))
        (ruler-layer (car (gimp-layer-new img
                                          size size RGBA-IMAGE
                                          _"Arrow" 100 NORMAL-MODE)))
        (glow-layer (car (gimp-layer-new img
                                         size size RGBA-IMAGE
                                         _"Alien Glow" 100 NORMAL-MODE)))
        (bg-layer (car (gimp-layer-new img
                                       size size RGB-IMAGE
                                       _"Background" 100 NORMAL-MODE)))
        (big-arrow (point-list->double-array
                    (rotate-points (make-arrow size offset)
                                    size orientation)))
        )

    (gimp-context-push)

    (gimp-image-undo-disable img)
    ;(gimp-image-resize img (+ length height) (+ height height) 0 0)
    (gimp-image-insert-layer img bg-layer 0 1)
    (gimp-image-insert-layer img glow-layer 0 -1)
    (gimp-image-insert-layer img ruler-layer 0 -1)

    (gimp-edit-clear glow-layer)
    (gimp-edit-clear ruler-layer)

    (gimp-free-select img 6 big-arrow CHANNEL-OP-REPLACE TRUE FALSE 0)

    (gimp-context-set-foreground '(103 103 103))
    (gimp-context-set-background '(0 0 0))

    (gimp-edit-blend ruler-layer FG-BG-RGB-MODE NORMAL-MODE
                     GRADIENT-SHAPEBURST-ANGULAR 100 0 REPEAT-NONE FALSE
                     FALSE 0 0 TRUE
                     0 0 size size)

    (gimp-selection-grow img grow-amount)
    (gimp-context-set-foreground glow-color)
    (gimp-edit-fill glow-layer FOREGROUND-FILL)

    (gimp-selection-none img)


    (plug-in-gauss-rle RUN-NONINTERACTIVE img glow-layer blur-radius TRUE TRUE)

    (gimp-context-set-background bg-color)
    (gimp-edit-fill bg-layer BACKGROUND-FILL)

    (if (= flatten TRUE)
        (gimp-image-flatten img)
    )
    (gimp-image-undo-enable img)
    (gimp-display-new img)

    (gimp-context-pop)
  )
)

(script-fu-register "script-fu-alien-glow-right-arrow"
  _"_Arrow..."
  _"Create an arrow graphic with an eerie glow for web pages"
  "Adrian Likins"
  "Adrian Likins"
  "1997"
  ""
  SF-ADJUSTMENT _"Size"             '(32 5 150 1 10 0 1)
  SF-OPTION     _"Orientation"      '(_"Right" _"Left" _"Up" _"Down")
  SF-COLOR      _"Glow color"       '(63 252 0)
  SF-COLOR      _"Background color" "black"
  SF-TOGGLE     _"Flatten image"    TRUE
)

(script-fu-menu-register "script-fu-alien-glow-right-arrow"
                         "<Image>/File/Create/Web Page Themes/Alien Glow")
