;; text-circle.scm -- a script for GIMP
;; Author: Shuji Narazaki <narazaki@gimp.org>
;; Time-stamp: <1998/11/25 13:26:51 narazaki@gimp.org>
;; Version 2.5
;; Thanks:
;;   jseymour@jimsun.LinxNet.com (Jim Seymour)
;;   Sven Neumann <neumanns@uni-duesseldorf.de>
;;
;; Modified June 24, 2005 by Kevin Cozens
;; Incorporated changes made by Daniel P. Stasinski in his text-circle2.scm
;; script. The letters are now placed properly for both positive and negative
;; fill angles.

(if (not (symbol-bound? 'script-fu-text-circle-debug? (current-environment)))
    (define script-fu-text-circle-debug? #f)
)

(define (script-fu-text-circle text radius start-angle fill-angle
                               font-size antialias font-name)

  (define (wrap-string str)
    (string-append "\"" str "\"")
  )
  (define (white-space-string? str)
    (or (equal? " " str) (equal? "\t" str))
  )

  (let* (
        (drawable-size (* 2.0 (+ radius (* 2 font-size))))
        (script-fu-text-circle-debug? #f)
        (img (car (gimp-image-new drawable-size drawable-size RGB)))
        (BG-layer (car (gimp-layer-new img drawable-size drawable-size
                       RGBA-IMAGE "background" 100 NORMAL-MODE)))
        (merged-layer #f)
        (char-num (string-length text))
        (radian-step 0)
        (rad-90 (/ *pi* 2))
        (center-x (/ drawable-size 2))
        (center-y center-x)
        (font-infos (gimp-text-get-extents-fontname "lAgy" font-size
                                PIXELS font-name))
        (desc (nth 3 font-infos))
        (start-angle-rad (* (/ (modulo start-angle 360) 360) 2 *pi*))
        (angle-list #f)
        (letter "")
        (new-layer #f)
        (index 0)
        (ndx 0)
        (ndx-start 0)
        (ndx-step 1)
        (ccw 0)
        (fill-angle-rad 0)
        (rot-op 0)
        (radian-step 0)
        )

    (gimp-image-undo-disable img)
    (gimp-image-insert-layer img BG-layer 0 0)
    (gimp-edit-fill BG-layer BACKGROUND-FILL)

    ;; change units
    (if (< fill-angle 0)
        (begin
          (set! ccw 1)
          (set! fill-angle (abs fill-angle))
          (set! start-angle-rad (* (/ (modulo (+ (- start-angle fill-angle) 360) 360) 360) 2 *pi*))
          (set! ndx-start (- char-num 1))
          (set! ndx-step -1)
        )
    )

    (set! fill-angle-rad (* (/ fill-angle 360) 2 *pi*))
    (set! radian-step (/ fill-angle-rad char-num))

    ;; make width-list
    ;;  In a situation,
    ;; (car (gimp-drawable-width (car (gimp-text ...)))
    ;; != (car (gimp-text-get-extent ...))
    ;; Thus, I changed to gimp-text from gimp-text-get-extent at 2.2 !!
    (let (
         (temp-list '())
         (temp-str #f)
         (temp-layer #f)
         (scale 0)
         (temp #f)
         )
      (set! ndx ndx-start)
      (set! index 0)
      (while (< index char-num)
        (set! temp-str (substring text ndx (+ ndx 1)))
        (if (white-space-string? temp-str)
            (set! temp-str "x")
        )
        (set! temp-layer (car (gimp-text-fontname img -1 0 0
                              temp-str
                              1 antialias
                              font-size PIXELS
                              font-name)))
        (set! temp-list (cons (car (gimp-drawable-width temp-layer)) temp-list))
        (gimp-image-remove-layer img temp-layer)
        (set! ndx (+ ndx ndx-step))
        (set! index (+ index 1))
      )
      (set! angle-list (nreverse temp-list))
      (set! temp 0)
      (set! angle-list
        (mapcar
          (lambda (angle)
            (let ((tmp temp))
              (set! temp (+ angle temp))
              (+ tmp (/ angle 2))
            )
          )
          angle-list
        )
      )
      (set! scale (/ fill-angle-rad temp))
      (set! angle-list (mapcar (lambda (angle) (* scale angle)) angle-list))
    )
    (set! ndx ndx-start)
    (set! index 0)
    (while (< index char-num)
      (set! letter (substring text ndx (+ ndx 1)))
      (if (not (white-space-string? letter))
        ;; Running gimp-text with " " causes an error!
        (let* (
              (new-layer (car (gimp-text-fontname img -1 0 0
                                       letter
                                       1 antialias
                                       font-size PIXELS
                                       font-name)))
              (width (car (gimp-drawable-width new-layer)))
              (height (car (gimp-drawable-height new-layer)))
              (rotate-radius (- (/ height 2) desc))
              (angle (+ start-angle-rad (- (nth index angle-list) rad-90)))
              )

          (gimp-layer-resize new-layer width height 0 0)
          (set! width (car (gimp-drawable-width new-layer)))
          (if (not script-fu-text-circle-debug?)
            (begin
              (if (= ccw 0)
                  (set! rot-op (if (< 0 fill-angle-rad) + -))
                  (set! rot-op (if (> 0 fill-angle-rad) + -))
              )
              (gimp-drawable-transform-rotate-default new-layer
                       (rot-op angle rad-90)
                       TRUE 0 0
                       TRUE FALSE)
              (gimp-layer-translate new-layer
                   (+ center-x
                      (* radius (cos angle))
                      (* rotate-radius
                         (cos (if (< 0 fill-angle-rad)
                                  angle
                                  (+ angle *pi*)
                              )
                         )
                      )
                      (- (/ width 2))
                   )
                   (+ center-y
                      (* radius (sin angle))
                      (* rotate-radius
                         (sin (if (< 0 fill-angle-rad)
                                  angle
                                  (+ angle *pi*)
                              )
                         )
                      )
                      (- (/ height 2))
                   )
              )
            )
          )
        )
      )
      (set! ndx (+ ndx ndx-step))
      (set! index (+ index 1))
    )

    (gimp-item-set-visible BG-layer 0)
    (if (not script-fu-text-circle-debug?)
      (begin
        (set! merged-layer
                (car (gimp-image-merge-visible-layers img CLIP-TO-IMAGE)))
        (gimp-item-set-name merged-layer
                     (if (< (string-length text) 16)
                         (wrap-string text)
                         "Text Circle"
                     )
        )
      )
    )
    (gimp-item-set-visible BG-layer 1)
    (gimp-image-undo-enable img)
    (gimp-image-clean-all img)
    (gimp-display-new img)
    (gimp-displays-flush)
  )
)

(script-fu-register "script-fu-text-circle"
  _"Text C_ircle..."
  _"Create a logo by rendering the specified text along the perimeter of a circle"
  "Shuji Narazaki <narazaki@gimp.org>"
  "Shuji Narazaki"
  "1997-1998"
  ""
  SF-STRING     _"Text" "The GNU Image Manipulation Program Version 2.0 "
  SF-ADJUSTMENT _"Radius"             '(80 1 8000 1 1 0 1)
  SF-ADJUSTMENT _"Start angle"        '(0 -180 180 1 1 0 1)
  SF-ADJUSTMENT _"Fill angle"         '(360 -360 360 1 1 0 1)
  SF-ADJUSTMENT _"Font size (pixels)" '(18 1 1000 1 1 0 1)
  SF-TOGGLE     _"Antialias"          TRUE
  SF-FONT       _"Font"               "Sans"
)

(script-fu-menu-register "script-fu-text-circle"
                         "<Image>/File/Create/Logos")
