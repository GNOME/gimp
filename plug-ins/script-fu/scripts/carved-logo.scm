;  CARVE-TEXT
;   Carving, embossing, & stamping
;   Process taken from "The Photoshop 3 WOW! Book"
;   http://www.peachpit.com


(define (carve-brush brush-size)
  (cond ((<= brush-size 5) "Circle (05)")
        ((<= brush-size 7) "Circle (07)")
        ((<= brush-size 9) "Circle (09)")
        ((<= brush-size 11) "Circle (11)")
        ((<= brush-size 13) "Circle (13)")
        ((<= brush-size 15) "Circle (15)")
        ((<= brush-size 17) "Circle (17)")
        ((> brush-size 17) "Circle (19)")))

(define (carve-scale val scale)
  (* (sqrt val) scale))

(define (calculate-inset-gamma img layer)
  (let* ((stats (gimp-histogram layer 0 0 255))
         (mean (car stats)))
    (cond ((< mean 127) (+ 1.0 (* 0.5 (/ (- 127 mean) 127.0))))
          ((>= mean 127) (- 1.0 (* 0.5 (/ (- mean 127) 127.0)))))))

(define (script-fu-carved-logo text size font bg-img carve-raised padding)
  (let* (
        (img (car (gimp-file-load 1 bg-img bg-img)))
        (offx (carve-scale size 0.33))
        (offy (carve-scale size 0.25))
        (feather (carve-scale size 0.3))
        (brush-size (carve-scale size 0.3))
        (b-size (+ (carve-scale size 0.5) padding))
        (layer1 (car (gimp-image-get-active-drawable img)))
        (mask-layer (car (gimp-text-fontname img -1 0 0 text b-size TRUE size PIXELS font)))
        (width (car (gimp-drawable-width mask-layer)))
        (height (car (gimp-drawable-height mask-layer)))
        (mask-fs 0)
        (mask (car (gimp-channel-new img width height "Engraving Mask" 50 '(0 0 0))))
        (inset-gamma (calculate-inset-gamma img layer1))
        (mask-fat 0)
        (mask-emboss 0)
        (mask-highlight 0)
        (mask-shadow 0)
        (shadow-layer 0)
        (highlight-layer 0)
        (cast-shadow-layer 0)
        (csl-mask 0)
        (inset-layer 0)
        (il-mask 0)
        )

    (gimp-context-push)

    (gimp-image-undo-disable img)

    (gimp-image-set-filename img "")

    (gimp-image-insert-channel img mask -1 0)

    (gimp-layer-set-lock-alpha mask-layer TRUE)
    (gimp-context-set-background '(255 255 255))
    (gimp-edit-fill mask-layer BACKGROUND-FILL)
    (gimp-context-set-background '(0 0 0))
    (gimp-edit-fill mask BACKGROUND-FILL)

    (plug-in-tile RUN-NONINTERACTIVE img layer1 width height FALSE)

    (gimp-edit-copy mask-layer)
    (set! mask-fs (car (gimp-edit-paste mask FALSE)))
    (gimp-floating-sel-anchor mask-fs)
    (if (= carve-raised TRUE)
        (gimp-invert mask)
    )

    (gimp-image-remove-layer img mask-layer)

    (set! mask-fat (car (gimp-channel-copy mask)))
    (gimp-image-insert-channel img mask-fat -1 0)
    (gimp-item-to-selection mask-fat CHANNEL-OP-REPLACE)
    (gimp-context-set-brush (carve-brush brush-size))
    (gimp-context-set-foreground '(255 255 255))
    (gimp-edit-stroke mask-fat)
    (gimp-selection-none img)

    (set! mask-emboss (car (gimp-channel-copy mask-fat)))
    (gimp-image-insert-channel img mask-emboss -1 0)
    (plug-in-gauss-rle RUN-NONINTERACTIVE img mask-emboss feather TRUE TRUE)
    (plug-in-emboss RUN-NONINTERACTIVE img mask-emboss 315.0 45.0 7 TRUE)

    (gimp-context-set-background '(180 180 180))
    (gimp-item-to-selection mask-fat CHANNEL-OP-REPLACE)
    (gimp-selection-invert img)
    (gimp-edit-fill mask-emboss BACKGROUND-FILL)
    (gimp-item-to-selection mask CHANNEL-OP-REPLACE)
    (gimp-edit-fill mask-emboss BACKGROUND-FILL)
    (gimp-selection-none img)

    (set! mask-highlight (car (gimp-channel-copy mask-emboss)))
    (gimp-image-insert-channel img mask-highlight -1 0)
    (gimp-levels mask-highlight 0 180 255 1.0 0 255)

    (set! mask-shadow mask-emboss)
    (gimp-levels mask-shadow 0 0 180 1.0 0 255)

    (gimp-edit-copy mask-shadow)
    (set! shadow-layer (car (gimp-edit-paste layer1 FALSE)))
    (gimp-floating-sel-to-layer shadow-layer)
    (gimp-layer-set-mode shadow-layer MULTIPLY-MODE)

    (gimp-edit-copy mask-highlight)
    (set! highlight-layer (car (gimp-edit-paste shadow-layer FALSE)))
    (gimp-floating-sel-to-layer highlight-layer)
    (gimp-layer-set-mode highlight-layer SCREEN-MODE)

    (gimp-edit-copy mask)
    (set! cast-shadow-layer (car (gimp-edit-paste highlight-layer FALSE)))
    (gimp-floating-sel-to-layer cast-shadow-layer)
    (gimp-layer-set-mode cast-shadow-layer MULTIPLY-MODE)
    (gimp-layer-set-opacity cast-shadow-layer 75)
    (plug-in-gauss-rle RUN-NONINTERACTIVE img cast-shadow-layer feather TRUE TRUE)
    (gimp-layer-translate cast-shadow-layer offx offy)

    (set! csl-mask (car (gimp-layer-create-mask cast-shadow-layer ADD-BLACK-MASK)))
    (gimp-layer-add-mask cast-shadow-layer csl-mask)
    (gimp-item-to-selection mask CHANNEL-OP-REPLACE)
    (gimp-context-set-background '(255 255 255))
    (gimp-edit-fill csl-mask BACKGROUND-FILL)

    (set! inset-layer (car (gimp-layer-copy layer1 TRUE)))
    (gimp-image-insert-layer img inset-layer -1 1)

    (set! il-mask (car (gimp-layer-create-mask inset-layer ADD-BLACK-MASK)))
    (gimp-layer-add-mask inset-layer il-mask)
    (gimp-item-to-selection mask CHANNEL-OP-REPLACE)
    (gimp-context-set-background '(255 255 255))
    (gimp-edit-fill il-mask BACKGROUND-FILL)
    (gimp-selection-none img)

    (gimp-levels inset-layer 0 0 255 inset-gamma 0 255)

    (gimp-image-remove-channel img mask)
    (gimp-image-remove-channel img mask-fat)
    (gimp-image-remove-channel img mask-highlight)
    (gimp-image-remove-channel img mask-shadow)

    (gimp-item-set-name layer1 _"Carved Surface")
    (gimp-item-set-name shadow-layer _"Bevel Shadow")
    (gimp-item-set-name highlight-layer _"Bevel Highlight")
    (gimp-item-set-name cast-shadow-layer _"Cast Shadow")
    (gimp-item-set-name inset-layer _"Inset")

    (gimp-display-new img)
    (gimp-image-undo-enable img)

    (gimp-context-pop)
  )
)

(script-fu-register "script-fu-carved-logo"
  _"Carved..."
  _"Create a logo with text raised above or carved in to the specified background image"
  "Spencer Kimball"
  "Spencer Kimball"
  "1997"
  ""
  SF-STRING     _"Text" "Marble"
  SF-ADJUSTMENT _"Font size (pixels)" '(100 2 1000 1 10 0 1)
  SF-FONT       _"Font" "Engraver"
  SF-FILENAME   _"Background Image"
      (string-append gimp-data-directory
                     "/scripts/images/texture3.jpg")
  SF-TOGGLE     _"Carve raised text" FALSE
  SF-ADJUSTMENT _"Padding around text" '(10 0 1000 1 10 0 1)
)

(script-fu-menu-register "script-fu-carved-logo"
                         "<Image>/File/Create/Logos")
