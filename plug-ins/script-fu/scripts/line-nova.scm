;;; line-nova.scm for ligma-1.1 -*-scheme-*-
;;; Time-stamp: <1998/11/25 13:26:44 narazaki@ligma.org>
;;; Author Shuji Narazaki <narazaki@ligma.org>
;;; Version 0.7

(define (script-fu-line-nova img drw num-of-lines corn-deg offset variation)
  (let* (
        (*points* (cons-array (* 3 2) 'double))
        (modulo fmod)                        ; in R4RS way
        (pi/2 (/ *pi* 2))
        (pi/4 (/ *pi* 4))
        (pi3/4 (* 3 pi/4))
        (pi5/4 (* 5 pi/4))
        (pi3/2 (* 3 pi/2))
        (pi7/4 (* 7 pi/4))
        (2pi (* 2 *pi*))
        (rad/deg (/ 2pi 360))
        (variation/2 (/ variation 2))
        (drw-width (car (ligma-drawable-get-width drw)))
        (drw-height (car (ligma-drawable-get-height drw)))
        (drw-offsets (ligma-drawable-get-offsets drw))
        (old-selection FALSE)
        (radius (max drw-height drw-width))
        (index 0)
        (dir-deg/line (/ 360 num-of-lines))
        (fg-color (car (ligma-context-get-foreground)))
        )
    (ligma-context-push)
    (ligma-context-set-defaults)
    (ligma-context-set-foreground fg-color)

    (define (draw-vector beg-x beg-y direction)

      (define (set-point! index x y)
            (aset *points* (* 2 index) x)
            (aset *points* (+ (* 2 index) 1) y)
      )
      (define (deg->rad rad)
            (* (modulo rad 360) rad/deg)
      )
      (define (set-marginal-point beg-x beg-y direction)
        (let (
             (dir1 (deg->rad (+ direction corn-deg)))
             (dir2 (deg->rad (- direction corn-deg)))
             )

          (define (aux dir index)
                   (set-point! index
                               (+ beg-x (* (cos dir) radius))
                               (+ beg-y (* (sin dir) radius)))
          )

          (aux dir1 1)
          (aux dir2 2)
        )
      )

      (let (
           (dir0 (deg->rad direction))
           (off (+ offset (- (modulo (rand) variation) variation/2)))
           )

        (set-point! 0
                    (+ beg-x (* off (cos dir0)))
                    (+ beg-y (* off (sin dir0)))
        )
        (set-marginal-point beg-x beg-y direction)
        (ligma-image-select-polygon img CHANNEL-OP-ADD 6 *points*)
      )
    )

    (ligma-image-undo-group-start img)

    (set! old-selection
      (if (eq? (car (ligma-selection-is-empty img)) TRUE)
         #f
         (car (ligma-selection-save img))
      )
    )

    (ligma-selection-none img)
    (srand (realtime))
    (while (< index num-of-lines)
      (draw-vector (+ (nth 0 drw-offsets) (/ drw-width 2))
                   (+ (nth 1 drw-offsets) (/ drw-height 2))
                   (* index dir-deg/line)
      )
      (set! index (+ index 1))
    )
    (ligma-drawable-edit-fill drw FILL-FOREGROUND)

    (if old-selection
      (begin
        (ligma-image-select-item img CHANNEL-OP-REPLACE old-selection)
        (ligma-image-remove-channel img old-selection)
      )
    )

    (ligma-image-undo-group-end img)
    (ligma-displays-flush)
    (ligma-context-pop)
  )
)

(script-fu-register "script-fu-line-nova"
  _"Line _Nova..."
  _"Fill a layer with rays emanating outward from its center using the foreground color"
  "Shuji Narazaki <narazaki@ligma.org>"
  "Shuji Narazaki"
  "1997,1998"
  "*"
  SF-IMAGE       "Image"               0
  SF-DRAWABLE    "Drawable"            0
  SF-ADJUSTMENT _"Number of lines"     '(200 40 1000 1 1 0 1)
  SF-ADJUSTMENT _"Sharpness (degrees)" '(1.0 0.0 10.0 0.1 1 1 1)
  SF-ADJUSTMENT _"Offset radius"       '(100 0 2000 1 1 0 1)
  SF-ADJUSTMENT _"Randomness"          '(30 1 2000 1 1 0 1)
)

(script-fu-menu-register "script-fu-line-nova"
                         "<Image>/Filters/Render")
