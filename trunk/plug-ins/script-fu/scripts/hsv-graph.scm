;;; hsv-graph.scm -*-scheme-*-
;;; Author: Shuji Narazaki <narazaki@InetQ.or.jp>
;;; Time-stamp: <1998/01/18 05:25:03 narazaki@InetQ.or.jp>
;;; Version: 1.2
; ************************************************************************
; Changed on Feb 4, 1999 by Piet van Oostrum <piet@cs.uu.nl>
; For use with GIMP 1.1.
; All calls to gimp-text-* have been converted to use the *-fontname form.
; The corresponding parameters have been replaced by an SF-FONT parameter.
; ************************************************************************
;;; Code:

(if (not (symbol-bound? 'script-fu-hsv-graph-scale (current-environment)))
    (define script-fu-hsv-graph-scale 1))
(if (not (symbol-bound? 'script-fu-hsv-graph-opacity (current-environment)))
    (define script-fu-hsv-graph-opacity 100))
(if (not (symbol-bound? 'script-fu-hsv-graph-bounds? (current-environment)))
    (define script-fu-hsv-graph-bounds? TRUE))
(if (not (symbol-bound? 'script-fu-hsv-graph-left2right? (current-environment)))
    (define script-fu-hsv-graph-left2right? FALSE))
(if (not (symbol-bound? 'script-fu-hsv-graph-beg-x (current-environment)))
    (define script-fu-hsv-graph-beg-x 0))
(if (not (symbol-bound? 'script-fu-hsv-graph-beg-y (current-environment)))
    (define script-fu-hsv-graph-beg-y 0))
(if (not (symbol-bound? 'script-fu-hsv-graph-end-x (current-environment)))
    (define script-fu-hsv-graph-end-x 1))
(if (not (symbol-bound? 'script-fu-hsv-graph-end-y (current-environment)))
    (define script-fu-hsv-graph-end-y 1))

(define (script-fu-hsv-graph img drawable scale opacity bounds?
                           left2right? beg-x beg-y end-x end-y)
  (define (floor x) (- x (fmod x 1)))
  (define *pos* #f)
  (define (set-point! fvec index x y)
    (aset fvec (* 2 index) x)
    (aset fvec (+ (* 2 index) 1) y)
    fvec
  )

  (define (plot-dot img drawable x y)
    (gimp-pencil drawable 1 (set-point! *pos* 0 x y))
  )

  (define (rgb-to-hsv rgb hsv)
    (let* (
          (red (floor (nth 0 rgb)))
          (green (floor (nth 1 rgb)))
          (blue (floor (nth 2 rgb)))
          (h 0.0)
          (s 0.0)
          (minv (min red (min green blue)))
          (maxv (max red (max green blue)))
          (v maxv)
          (delta 0)
          )
      (if (not (= 0 maxv))
          (set! s (/ (* (- maxv minv) 255.0) maxv))
          (set! s 0.0)
      )
      (if (= 0.0 s)
          (set! h 0.0)
          (begin
            (set! delta (- maxv minv))
            (cond ((= maxv red)
                   (set! h (/ (- green blue) delta)))
                  ((= maxv green)
                   (set! h (+ 2.0 (/ (- blue red) delta))))
                  ((= maxv blue)
                   (set! h (+ 4.0 (/ (- red green) delta)))))
            (set! h (* 42.5 h))
            (if (< h 0.0)
                (set! h (+ h 255.0)))
            (if (< 255 h)
                (set! h (- h 255.0))))
      )
      (set-car! hsv (floor h))
      (set-car! (cdr hsv) (floor s))
      (set-car! (cddr hsv) (floor v))
    )
  )

  ;; segment is
  ;;   filled-index (integer)
  ;;   size as number of points (integer)
  ;;   vector (which size is 2 * size)
  (define (make-segment length x y)
    (if (< 64 length) (set! length 64))
    (if (< length 5)  (set! length 5))
    (let (
         (vec (cons-array (* 2 length) 'double)))
         (aset vec 0 x)
         (aset vec 1 y)
         (list 1 length vec)
    )
  )

  ;; accessors
  (define (segment-filled-size segment) (car segment))
  (define (segment-max-size segment) (cadr segment))
  (define (segment-strokes segment) (caddr segment))

  (define (fill-segment! segment new-x new-y)

    (define (shift-segment! segment)
      (let ((base 0)
            (size (cadr segment))
            (vec (caddr segment))
            (offset 2))
        (while (< base offset)
               (aset vec (* 2 base)
                     (aref vec (* 2 (- size (- offset base)))))
               (aset vec (+ (* 2 base) 1)
                     (aref vec (+ (* 2 (- size (- offset base))) 1)))
               (set! base (+ base 1)))
        (set-car! segment base))
    )

    (let ((base (car segment))
          (size (cadr segment))
          (vec (caddr segment)))
      (if (= base 0)
          (begin
            (shift-segment! segment)
            (set! base (segment-filled-size segment))))
      (if (and (= new-x (aref vec (* 2 (- base 1))))
               (= new-y (aref vec (+ (* 2 (- base 1)) 1))))
          #f
          (begin
            (aset vec (* 2 base) new-x)
            (aset vec (+ (* 2 base) 1) new-y)
            (set! base (+ base 1))
            (if (= base size)
                (begin
                  (set-car! segment 0)
                  #t)
                (begin
                  (set-car! segment base)
                  #f))))
    )
  )

  (define (draw-segment img drawable segment limit rgb)
    (gimp-context-set-foreground rgb)
    (gimp-airbrush drawable 100 (* 2 limit) (segment-strokes segment))
  )

  (define red-color '(255 10 10))
  (define green-color '(10 255 10))
  (define blue-color '(10 10 255))
  (define hue-segment #f)
  (define saturation-segment #f)
  (define value-segment #f)
  (define red-segment #f)
  (define green-segment #f)
  (define blue-segment #f)
  (define border-size 10)

  (define (fill-dot img drawable x y segment color)
    (if (fill-segment! segment x y)
        (begin
          (gimp-context-set-foreground color)
          (draw-segment img drawable segment (segment-max-size segment) color)
          #t)
        #f)
  )

  (define (fill-color-band img drawable x scale x-base y-base color)
    (gimp-context-set-foreground color)
    (gimp-rect-select img (+ x-base (* scale x)) 0 scale y-base CHANNEL-OP-REPLACE FALSE 0)
    (gimp-edit-bucket-fill drawable FG-BUCKET-FILL NORMAL-MODE 100 0 FALSE 0 0)
    (gimp-selection-none img)
  )

  (define (plot-hsv img drawable x scale x-base y-base hsv)
    (let ((real-x (* scale x))
          (h (car hsv))
          (s (cadr hsv))
          (v (caddr hsv)))
      (fill-dot img drawable (+ x-base real-x) (- y-base h)
                hue-segment red-color)
      (fill-dot img drawable (+ x-base real-x) (- y-base s)
                saturation-segment green-color)
      (if (fill-dot img drawable (+ x-base real-x) (- y-base v)
                    value-segment blue-color)
          (gimp-displays-flush)))
  )

  (define (plot-rgb img drawable x scale x-base y-base hsv)
    (let ((real-x (* scale x))
          (h (car hsv))
          (s (cadr hsv))
          (v (caddr hsv)))
      (fill-dot img drawable (+ x-base real-x) (- y-base h)
                red-segment red-color)
      (fill-dot img drawable (+ x-base real-x) (- y-base s)
                green-segment green-color)
      (if (fill-dot img drawable (+ x-base real-x) (- y-base v)
                    blue-segment blue-color)
          (gimp-displays-flush)))
  )

  (define (clamp-value x minv maxv)
    (if (< x minv)
        (set! x minv))
    (if (< maxv x)
        (set! x maxv))
    x
  )

  ;; start of script-fu-hsv-graph
  (if (= TRUE bounds?)
      (if (= TRUE (car (gimp-selection-bounds img)))
          (let ((results (gimp-selection-bounds img)))
            (set! beg-x (nth (if (= TRUE left2right?) 1 3) results))
            (set! beg-y (nth 2 results))
            (set! end-x (nth (if (= TRUE left2right?) 3 1) results))
            (set! end-y (nth 4 results)))
          (let ((offsets (gimp-drawable-offsets drawable)))
            (set! beg-x (if (= TRUE left2right?)
                            (nth 0 offsets)
                            (- (+ (nth 0 offsets)
                                  (car (gimp-drawable-width drawable)))
                               1)))
            (set! beg-y (nth 1 offsets))
            (set! end-x (if (= TRUE left2right?)
                            (- (+ (nth 0 offsets)
                                  (car (gimp-drawable-width drawable)))
                               1)
                            (nth 0 offsets)))
            (set! end-y (- (+ (nth 1 offsets)
                              (car (gimp-drawable-height drawable)))
                           1))))
      (let ((offsets (gimp-drawable-offsets drawable)))
        (set! beg-x (clamp-value beg-x 0
                                 (+ (nth 0 offsets)
                                    (car (gimp-drawable-width drawable)))))
        (set! end-x (clamp-value end-x 0
                                 (+ (nth 0 offsets)
                                    (car (gimp-drawable-width drawable)))))
        (set! beg-y (clamp-value beg-y 0
                                 (+ (nth 1 offsets)
                                    (car (gimp-drawable-height drawable)))))
        (set! end-y (clamp-value end-y 0
                                 (+ (nth 1 offsets)
                                    (car (gimp-drawable-height drawable)))))))
  (set! opacity (clamp-value opacity 0 100))
  (let* ((x-len (- end-x beg-x))
         (y-len (- end-y beg-y))
         (limit (pow (+ (pow x-len 2) (pow y-len 2)) 0.5))
         (gimg-width (* limit scale))
         (gimg-height 256)
         (gimg (car (gimp-image-new (+ (* 2 border-size) gimg-width)
                                    (+ (* 2 border-size) gimg-height) RGB)))
         (bglayer (car (gimp-layer-new gimg
                                       (+ (* 2 border-size) gimg-width)
                                       (+ (* 2 border-size) gimg-height)
                                       1 "Background" 100 NORMAL-MODE)))
         (hsv-layer (car (gimp-layer-new gimg
                                       (+ (* 2 border-size) gimg-width)
                                       (+ (* 2 border-size) gimg-height)
                                      RGBA-IMAGE "HSV Graph" 100 NORMAL-MODE)))
         (rgb-layer (car (gimp-layer-new gimg
                                       (+ (* 2 border-size) gimg-width)
                                       (+ (* 2 border-size) gimg-height)
                                      RGBA-IMAGE "RGB Graph" 100 NORMAL-MODE)))
         (clayer (car (gimp-layer-new gimg gimg-width 40 RGBA-IMAGE
                                       "Color Sampled" opacity NORMAL-MODE)))
         (rgb '(255 255 255))
         (hsv '(254 255 255))
         (x-base border-size)
         (y-base (+ gimg-height border-size))
         (index 0))

    (gimp-context-push)

    (gimp-image-undo-disable gimg)
    (gimp-image-add-layer gimg bglayer -1)
    (gimp-selection-all gimg)
    (gimp-context-set-background '(255 255 255))
    (gimp-edit-fill bglayer BACKGROUND-FILL)
    (gimp-image-add-layer gimg hsv-layer -1)
    (gimp-edit-clear hsv-layer)
    (gimp-image-add-layer gimg rgb-layer -1)
    (gimp-drawable-set-visible rgb-layer FALSE)
    (gimp-edit-clear rgb-layer)
    (gimp-image-add-layer gimg clayer -1)
    (gimp-edit-clear clayer)
    (gimp-layer-translate clayer border-size 0)
    (gimp-selection-none gimg)
    (set! red-segment (make-segment 64 x-base y-base))
    (set! green-segment (make-segment 64 x-base y-base))
    (set! blue-segment (make-segment 64 x-base y-base))
    (set! hue-segment (make-segment 64 x-base y-base))
    (set! saturation-segment (make-segment 64 x-base y-base))
    (set! value-segment (make-segment 64 x-base y-base))
    (gimp-context-set-brush "Circle (01)")
    (gimp-context-set-paint-mode NORMAL-MODE)
    (gimp-context-set-opacity 70)
    (gimp-display-new gimg)
    (while (< index limit)
      (set! rgb (car (gimp-image-pick-color img drawable
                                            (+ beg-x (* x-len (/ index limit)))
                                            (+ beg-y (* y-len (/ index limit)))
                                            TRUE FALSE 0)))
      (fill-color-band gimg clayer index scale x-base 40 rgb)
      (rgb-to-hsv rgb hsv)
      (plot-hsv gimg hsv-layer index scale x-base y-base hsv)
      (plot-rgb gimg rgb-layer index scale x-base y-base rgb)
      (set! index (+ index 1)))
    (mapcar
     (lambda (segment color)
       (if (< 1 (segment-filled-size segment))
        (begin
          (gimp-context-set-foreground color)
          (draw-segment gimg hsv-layer segment (segment-filled-size segment)
                        color))))
     (list hue-segment saturation-segment value-segment)
     (list red-color green-color blue-color))
    (mapcar
     (lambda (segment color)
       (if (< 1 (segment-filled-size segment))
        (begin
          (gimp-context-set-foreground color)
          (draw-segment gimg rgb-layer segment (segment-filled-size segment)
                        color))))
     (list red-segment green-segment blue-segment)
     (list red-color green-color blue-color))
    (gimp-context-set-foreground '(255 255 255))
    (let ((text-layer (car (gimp-text-fontname gimg -1 0 0
                            "Red: Hue, Green: Sat, Blue: Val"
                            1 1 12 PIXELS
                            "Sans")))
          (offset-y (- y-base (car (gimp-drawable-height clayer)))))
      (gimp-layer-set-mode text-layer DIFFERENCE-MODE)
      (gimp-layer-translate clayer 0 offset-y)
      (gimp-layer-translate text-layer border-size (+ offset-y 15)))
    (gimp-image-set-active-layer gimg bglayer)
    (gimp-image-clean-all gimg)
    (gimp-image-undo-enable gimg)

    (set! script-fu-hsv-graph-scale scale)
    (set! script-fu-hsv-graph-opacity opacity)
    (set! script-fu-hsv-graph-bounds? bounds?)
    (set! script-fu-hsv-graph-left2right? left2right?)
    (set! script-fu-hsv-graph-beg-x beg-x)
    (set! script-fu-hsv-graph-beg-y beg-y)
    (set! script-fu-hsv-graph-end-x end-x)
    (set! script-fu-hsv-graph-end-y end-y)
    (gimp-displays-flush)

    (gimp-context-pop)
  )
)

(script-fu-register "script-fu-hsv-graph"
  _"Draw _HSV Graph..."
  _"Create a graph of the Hue, Saturation, and Value distributions"
  "Shuji Narazaki <narazaki@InetQ.or.jp>"
  "Shuji Narazaki"
  "1997"
  "RGB*"
  SF-IMAGE       "Image to analyze"    0
  SF-DRAWABLE    "Drawable to analyze" 0
  SF-ADJUSTMENT _"Graph scale"         '(1 0.1 5 0.1 1 1 1)
  SF-ADJUSTMENT _"BG opacity"          '(100 0 100 1 10 0 1)
  SF-TOGGLE     _"Use selection bounds instead of values below" TRUE
  SF-TOGGLE     _"From top-left to bottom-right"                FALSE
  SF-ADJUSTMENT _"Start X"             '(0 0 5000 1 10 0 1)
  SF-ADJUSTMENT _"Start Y"             '(0 0 5000 1 10 0 1)
  SF-ADJUSTMENT _"End X"               '(1 0 5000 1 10 0 1)
  SF-ADJUSTMENT _"End Y"               '(1 0 5000 1 10 0 1)
)

(script-fu-menu-register "script-fu-hsv-graph"
                         "<Image>/Colors/Info")
