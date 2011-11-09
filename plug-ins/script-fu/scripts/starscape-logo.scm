;  Nova Starscape
;  Create a text effect that simulates an eerie alien glow around text

(define (apply-starscape-logo-effect img logo-layer size glow-color)

  (define (find-blend-coords w h)
    (let* (
          (denom (+ (/ w h) (/ h w)))
          (bx    (/ (* -2 h) denom))
          (by    (/ (* -2 w) denom))
          )
      (cons bx by)
    )
  )

  (define (find-nova-x-coord drawable x1 x2 y)
    (let* (
          (x 0)
          (alpha 3)
          (range (- x2 x1))
          (min-clearance 5)
          (val '())
          (val-left '())
          (val-right '())
          (val-top '())
          (val-bottom '())
          (limit 100)
          (clearance 0)
          )

      (while (and (= clearance 0) (> limit 0))
         (set! x (+ (rand range) x1))
         (set! val (cadr (gimp-drawable-get-pixel drawable x y)))
         (set! val-left (cadr (gimp-drawable-get-pixel drawable (- x min-clearance) y)))
         (set! val-right (cadr (gimp-drawable-get-pixel drawable (+ x min-clearance) y)))
         (set! val-top (cadr (gimp-drawable-get-pixel drawable x (- y min-clearance))))
         (set! val-bottom (cadr (gimp-drawable-get-pixel drawable x (+ y min-clearance))))
         (if (and (= (aref val alpha) 0) (= (aref val-left alpha) 0)
                  (= (aref val-right alpha) 0) (= (aref val-top alpha) 0)
                  (= (aref val-bottom alpha) 0)
             )
             (set! clearance 1)
             (set! limit (- limit 1))
         )
      )
      x
    )
  )

  (let* (
        (border (/ size 4))
        (grow (/ size 30))
        (offx (* size 0.03))
        (offy (* size 0.02))
        (feather (/ size 4))
        (shadow-feather (/ size 25))
        (width (car (gimp-drawable-width logo-layer)))
        (height (car (gimp-drawable-height logo-layer)))
        (w (* (/ (- width (* border 2)) 2.0) 0.75))
        (h (* (/ (- height (* border 2)) 2.0) 0.75))
        (novay (* height 0.3))
        (novax (find-nova-x-coord logo-layer (* width 0.2) (* width 0.8) novay))
        (novaradius (/ (min height width) 7.0))
        (cx (/ width 2.0))
        (cy (/ height 2.0))
        (bx (+ cx (car (find-blend-coords w h))))
        (by (+ cy (cdr (find-blend-coords w h))))
        (bg-layer (car (gimp-layer-new img width height RGB-IMAGE "Background" 100 NORMAL-MODE)))
        (glow-layer (car (gimp-layer-new img width height RGBA-IMAGE "Glow" 100 NORMAL-MODE)))
        (shadow-layer (car (gimp-layer-new img width height RGBA-IMAGE "Drop Shadow" 100 NORMAL-MODE)))
        (bump-channel (car (gimp-channel-new img width height "Bump Map" 50 '(0 0 0))))
        )

    (gimp-context-push)
    (gimp-context-set-defaults)

    (gimp-selection-none img)
    (script-fu-util-image-resize-from-layer img logo-layer)
    (script-fu-util-image-add-layers img shadow-layer glow-layer bg-layer)
    (gimp-image-insert-channel img bump-channel 0 0)
    (gimp-layer-set-lock-alpha logo-layer TRUE)

    (gimp-context-set-background '(0 0 0))
    (gimp-edit-fill bg-layer BACKGROUND-FILL)
    (gimp-edit-clear shadow-layer)
    (gimp-edit-clear glow-layer)

    (gimp-image-select-item img CHANNEL-OP-REPLACE logo-layer)
    (gimp-selection-grow img grow)
    (gimp-selection-feather img feather)
    (gimp-context-set-background glow-color)
    (gimp-selection-feather img feather)
    (gimp-edit-fill glow-layer BACKGROUND-FILL)

    (gimp-image-select-item img CHANNEL-OP-REPLACE logo-layer)
    (gimp-selection-feather img shadow-feather)
    (gimp-context-set-background '(0 0 0))
    (gimp-selection-translate img offx offy)
    (gimp-edit-fill shadow-layer BACKGROUND-FILL)

    (gimp-selection-none img)
    (gimp-context-set-background '(31 31 31))
    (gimp-context-set-foreground '(255 255 255))

    (gimp-edit-blend logo-layer FG-BG-RGB-MODE NORMAL-MODE
             GRADIENT-BILINEAR 100 0 REPEAT-NONE FALSE
             FALSE 0 0 TRUE
             cx cy bx by)

    (plug-in-nova RUN-NONINTERACTIVE img glow-layer novax novay glow-color novaradius 100 0)

    (gimp-selection-all img)
    (gimp-context-set-pattern "Stone")
    (gimp-edit-bucket-fill bump-channel PATTERN-BUCKET-FILL NORMAL-MODE 100 0 FALSE 0 0)
    (plug-in-bump-map RUN-NONINTERACTIVE img logo-layer bump-channel
              135.0 45.0 4 0 0 0 0 FALSE FALSE 0)
    (gimp-image-remove-channel img bump-channel)
    (gimp-selection-none img)

    (gimp-context-pop)
  )
)

(define (script-fu-starscape-logo text size fontname glow-color)
  (let* (
        (img (car (gimp-image-new 256 256 RGB)))
        (border (/ size 4))
        (text-layer (car (gimp-text-fontname img -1 0 0 text border
                                             TRUE size PIXELS fontname)))
        )
    (gimp-image-undo-disable img)
    (apply-starscape-logo-effect img text-layer size glow-color)
    (gimp-image-undo-enable img)
    (gimp-display-new img)
  )
)

(script-fu-register "script-fu-starscape-logo"
  _"Sta_rscape..."
  _"Create a logo using a rock-like texture, a nova glow, and shadow"
  "Spencer Kimball"
  "Spencer Kimball"
  "1997"
  ""
  SF-STRING     _"Text"               "Nova"
  SF-ADJUSTMENT _"Font size (pixels)" '(150 1 1000 1 10 0 1)
  SF-FONT       _"Font"               "Engraver"
  SF-COLOR      _"Glow color"         '(28 65 188)
)

(script-fu-menu-register "script-fu-starscape-logo"
                         "<Image>/File/Create/Logos")
