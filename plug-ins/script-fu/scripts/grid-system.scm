;;; grid-system.scm -*-scheme-*-
;;; Time-stamp: <1998/01/20 23:22:02 narazaki@InetQ.or.jp>
;;; This file is a part of:
;;;   GIMP (Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis)
;;; Author: Shuji Narazaki (narazaki@InetQ.or.jp)
;;; Version 0.6

;;; Code:
(if (not (symbol-bound? 'script-fu-grid-system-x-divides (current-environment)))
    (define script-fu-grid-system-x-divides "'(1 g 1)"))
(if (not (symbol-bound? 'script-fu-grid-system-y-divides (current-environment)))
    (define script-fu-grid-system-y-divides "'(1 g 1)"))

(define (script-fu-grid-system img drw x-divides-orig y-divides-orig)
  (define (update-segment! s x0 y0 x1 y1)
    (aset s 0 x0)
    (aset s 1 y0)
    (aset s 2 x1)
    (aset s 3 y1))
  (define (map proc seq)
    (if (null? seq)
        '()
        (cons (proc (car seq))
              (map proc (cdr seq)))))
  (define (convert-g l)
    (cond ((null? l) '())
    ((eq? (car l) 'g) (cons 1.618 (convert-g (cdr l))))
    ((eq? (car l) '1/g) (cons 0.618 (convert-g (cdr l))))
    ('else (cons (car l) (convert-g (cdr l))))))
  (define (wrap-list l)
    (define (wrap-object obj)
      (cond ((number? obj) (string-append (number->string obj) " "))
      ((eq? obj 'g) "g ")
      (eq? obj '1/g) "1/g "))
    (string-append "'(" (apply string-append (map wrap-object l)) ")"))
  (let* ((drw-width (car (gimp-drawable-width drw)))
   (drw-height (car (gimp-drawable-height drw)))
   (drw-offset-x (nth 0 (gimp-drawable-offsets drw)))
   (drw-offset-y (nth 1 (gimp-drawable-offsets drw)))
   (grid-layer #f)
   (segment (cons-array 4 'double))
   (stepped-x 0)
   (stepped-y 0)
   (temp 0)
   (total-step-x 0)
   (total-step-y 0)
   (x-divides (convert-g x-divides-orig))
   (y-divides (convert-g y-divides-orig))
   (total-step-x (apply + x-divides))
   (total-step-y (apply + y-divides)))

    (gimp-image-undo-group-start img)

    (set! grid-layer (car (gimp-layer-copy drw TRUE)))
    (gimp-image-add-layer img grid-layer 0)
    (gimp-edit-clear grid-layer)
    (gimp-item-set-name grid-layer "Grid Layer")

    (while (not (null? (cdr x-divides)))
      (set! stepped-x (+ stepped-x (car x-divides)))
      (set! temp (* drw-width (/ stepped-x total-step-x)))
      (set! x-divides (cdr x-divides))
      (update-segment! segment
           (+ drw-offset-x temp) drw-offset-y
           (+ drw-offset-x temp) (+ drw-offset-y drw-height))
      (gimp-pencil grid-layer 4 segment))

    (while (not (null? (cdr y-divides)))
      (set! stepped-y (+ stepped-y (car y-divides)))
      (set! temp (* drw-height (/ stepped-y total-step-y)))
      (set! y-divides (cdr y-divides))
      (update-segment! segment
           drw-offset-x (+ drw-offset-y temp)
           (+ drw-offset-x drw-width) (+ drw-offset-y temp))
      (gimp-pencil grid-layer 4 segment))

    (gimp-image-undo-group-end img)

    (set! script-fu-grid-system-x-divides (wrap-list x-divides-orig))
    (set! script-fu-grid-system-y-divides (wrap-list y-divides-orig))
    (gimp-displays-flush)))

(script-fu-register "script-fu-grid-system"
  _"_Grid..."
  _"Draw a grid as specified by the lists of X and Y locations using the current brush"
  "Shuji Narazaki <narazaki@InetQ.or.jp>"
  "Shuji Narazaki"
  "1997"
  "RGB*, INDEXED*, GRAY*"
  SF-IMAGE     "Image to use"          0
  SF-DRAWABLE  "Drawable to draw grid" 0
  SF-VALUE    _"X divisions"           script-fu-grid-system-x-divides
  SF-VALUE    _"Y divisions"           script-fu-grid-system-y-divides
)

