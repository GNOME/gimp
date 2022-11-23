;; -*-scheme-*-

(define (script-fu-guides-from-selection image drawable)
  (let* (
        (boundaries (ligma-selection-bounds image))
        ;; non-empty INT32 TRUE if there is a selection
        (selection (car boundaries))
        (x1 (cadr boundaries))
        (y1 (caddr boundaries))
        (x2 (cadr (cddr boundaries)))
        (y2 (caddr (cddr boundaries)))
        )

    ;; need to check for a selection or we get guides right at edges of the image
    (if (= selection TRUE)
      (begin
        (ligma-image-undo-group-start image)

        (ligma-image-add-vguide image x1)
        (ligma-image-add-hguide image y1)
        (ligma-image-add-vguide image x2)
        (ligma-image-add-hguide image y2)

        (ligma-image-undo-group-end image)
        (ligma-displays-flush)
      )
    )
  )
)

(script-fu-register "script-fu-guides-from-selection"
  _"New Guides from _Selection"
  _"Create four guides around the bounding box of the current selection"
  "Alan Horkan"
  "Alan Horkan, 2004.  Public Domain."
  "2004-08-13"
  "*"
  SF-IMAGE    "Image"    0
  SF-DRAWABLE "Drawable" 0
)

(script-fu-menu-register "script-fu-guides-from-selection"
                         "<Image>/Image/Guides")
