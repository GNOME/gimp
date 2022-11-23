(define (script-fu-erase-rows img drawable orientation which type)
  (script-fu-erase-nth-rows img drawable orientation which type 2)
)

(define (script-fu-erase-nth-rows img drawable orientation offset type nth)
  (let* (
        (width (car (ligma-drawable-get-width drawable)))
        (height (car (ligma-drawable-get-height drawable)))
        (position-x (car (ligma-drawable-get-offsets drawable)))
        (position-y (cadr (ligma-drawable-get-offsets drawable)))
        )

    (ligma-context-push)
    (ligma-context-set-paint-mode LAYER-MODE-NORMAL)
    (ligma-context-set-opacity 100.0)
    (ligma-context-set-feather FALSE)
    (ligma-image-undo-group-start img)
    (letrec ((loop (lambda (i max)
                     (if (< i max)
                         (begin
                           (if (= orientation 0)
                               (ligma-image-select-rectangle img CHANNEL-OP-REPLACE position-x (+ i position-y) width 1)
                               (ligma-image-select-rectangle img CHANNEL-OP-REPLACE (+ i position-x) position-y 1 height))
                           (if (= type 0)
                               (ligma-drawable-edit-clear drawable)
                               (ligma-drawable-edit-fill drawable FILL-BACKGROUND))
                           (loop (+ i nth) max))))))
      (loop offset
            (if (= orientation 0)
                height
                width)
      )
    )
    (ligma-selection-none img)
    (ligma-image-undo-group-end img)
    (ligma-context-pop)
    (ligma-displays-flush)
  )
)

(script-fu-register "script-fu-erase-nth-rows"
  _"_Erase Every Nth Row..."
  _"Erase every nth row or column"
  "Federico Mena Quintero, Nikc M. (Altered)"
  "Federico Mena Quintero"
  "June 1997, February 2020"
  "RGB* GRAY* INDEXED*"
  SF-IMAGE       "Image"      0
  SF-DRAWABLE    "Drawable"   0
  SF-OPTION     _"Rows/cols"  '(_"Rows" _"Columns")
  SF-ADJUSTMENT  "Offset"     '(0 0 1024 1 10 0 SF-SPINNER)
  SF-OPTION     _"Erase/fill" '(_"Erase" _"Fill with BG")
  SF-ADJUSTMENT  "Skip by"    '(1 1 1024 1 10 0 SF-SPINNER)
)

(script-fu-register "script-fu-erase-rows"
  _"_Erase Every Other Row..."
  _"Erase every other row or column"
  "Federico Mena Quintero"
  "Federico Mena Quintero"
  "June 1997"
  "RGB* GRAY* INDEXED*"
  SF-IMAGE    "Image"      0
  SF-DRAWABLE "Drawable"   0
  SF-OPTION  _"Rows/cols"  '(_"Rows" _"Columns")
  SF-OPTION  _"Even/odd"   '(_"Even" _"Odd")
  SF-OPTION  _"Erase/fill" '(_"Erase" _"Fill with BG")
)

; (script-fu-menu-register "script-fu-erase-rows"
;                          "<Image>/Filters/Distorts")
