;; -*-scheme-*-

;;  No copyright.  Public Domain.

(define (script-fu-guides-from-rule image
                                    drawables
                                    rule)
  (let* (
        (width (car (gimp-image-get-width image)))
      	(height (car (gimp-image-get-height image)))
        (SQRT5 2.236067977)
        )

    (gimp-image-undo-group-start image)

    (if (= rule 0)          ; centre lines
        (begin
            (gimp-image-add-hguide image (/ height 2))
            (gimp-image-add-vguide image (/ width 2))
        )
    )

    (if (= rule 1)          ; rule of thirds
        (begin
            (gimp-image-add-hguide image (/ height 3))
            (gimp-image-add-vguide image (/ width 3))
            (gimp-image-add-hguide image (/ (* height 2) 3))
            (gimp-image-add-vguide image (/ (* width 2) 3))
        )
    )

    (if (= rule 2)          ; rule of fifths
        (begin
            (gimp-image-add-hguide image (/ height 5))
            (gimp-image-add-vguide image (/ width 5))
            (gimp-image-add-hguide image (/ (* height 2) 5))
            (gimp-image-add-vguide image (/ (* width 2) 5))
            (gimp-image-add-hguide image (/ (* height 3) 5))
            (gimp-image-add-vguide image (/ (* width 3) 5))
            (gimp-image-add-hguide image (/ (* height 4) 5))
            (gimp-image-add-vguide image (/ (* width 4) 5))
        )
    )

    (if (= rule 3)          ; golden sections
        (begin
            (gimp-image-add-hguide image (/ (* height (+ 1 SQRT5)) (+ 3 SQRT5)))
            (gimp-image-add-hguide image (/ (* height 2) (+ 3 SQRT5)))
            (gimp-image-add-vguide image (/ (* width (+ 1 SQRT5)) (+ 3 SQRT5)))
            (gimp-image-add-vguide image (/ (* width 2) (+ 3 SQRT5)))
        )
    )

    (gimp-image-undo-group-end image)
    (gimp-displays-flush)
  )
)

(script-fu-register-filter "script-fu-guides-from-rule"
  _"New Guides from Rule..."
  _"Add guides based on a selected composition rule"
  "Richard McLean"
  "Richard McLean, 2024"
  "February 2024"
  "*"
  SF-ONE-OR-MORE-DRAWABLE  ; doesn't matter how many drawables are selected
  SF-OPTION     _"R_ule"       '(_"Center lines"
                                 _"Rule of thirds"
                                 _"Rule of fifths"
                                 _"Golden sections")
)

(script-fu-menu-register "script-fu-guides-from-rule"
                         "<Image>/Image/Guides")
