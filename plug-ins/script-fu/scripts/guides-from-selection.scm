;; -*-scheme-*-

(define (script-fu-guides-from-selection image drawable color)
  (let* (
        (boundaries (gimp-selection-bounds image))
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
        (gimp-image-undo-group-start image)

        (gimp-image-add-vguide image x1 color)
        (gimp-image-add-hguide image y1 color)
        (gimp-image-add-vguide image x2 color)
        (gimp-image-add-hguide image y2 color)

        (gimp-image-undo-group-end image)
        (gimp-displays-flush)
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
  SF-COLOR   _"_Guide Color" '(0 204 255)
)

(script-fu-menu-register "script-fu-guides-from-selection"
                         "<Image>/Image/Guides")
