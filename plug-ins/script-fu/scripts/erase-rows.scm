(define (script-fu-erase-rows img drawable orientation which type)
  (script-fu-erase-nth-rows img drawable orientation which type 2)
)

(define (script-fu-erase-nth-rows img drawable orientation offset type nth)
  (let* (
        (width (car (gimp-drawable-get-width drawable)))
        (height (car (gimp-drawable-get-height drawable)))
        (position-x (car (gimp-drawable-get-offsets drawable)))
        (position-y (cadr (gimp-drawable-get-offsets drawable)))
        )

    (gimp-context-push)
    (gimp-context-set-paint-mode LAYER-MODE-NORMAL)
    (gimp-context-set-opacity 100.0)
    (gimp-context-set-feather FALSE)
    (gimp-image-undo-group-start img)
    (letrec ((loop (lambda (i max)
                     (if (< i max)
                         (begin
                           (if (= orientation 0)
                               (gimp-image-select-rectangle img CHANNEL-OP-REPLACE position-x (+ i position-y) width 1)
                               (gimp-image-select-rectangle img CHANNEL-OP-REPLACE (+ i position-x) position-y 1 height))
                           (if (= type 0)
                               (gimp-drawable-edit-clear drawable)
                               (gimp-drawable-edit-fill drawable FILL-BACKGROUND))
                           (loop (+ i nth) max))))))
      (loop offset
            (if (= orientation 0)
                height
                width)
      )
    )
    (gimp-selection-none img)
    (gimp-image-undo-group-end img)
    (gimp-context-pop)
    (gimp-displays-flush)
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
