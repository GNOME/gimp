; test methods of drawable


; set/get tested elsewhere
; kernel operations on drawable as set of pixels tested elsewhere

; setup
(define testImage (testing:load-test-image "gimp-logo.png"))
; Wilber has one layer
(define testDrawable (vector-ref (cadr (gimp-image-get-layers testImage)) 0))



; get bounding box of intersection of selection mask with drawable
; bounding box in x, y, width, heigth format
(assert `(gimp-drawable-mask-bounds ,testDrawable))
; bounding box in upper left x,y lower right x,y
(assert `(gimp-drawable-mask-intersect ,testDrawable))



(assert `(gimp-drawable-free-shadow ,testDrawable))
(assert `(gimp-drawable-merge-filters ,testDrawable))

; update a region (AKA invalidate)
; forces redraw of any display
(assert `(gimp-drawable-update
            ,testDrawable
            -1 -1 ; origin of region
            2147483648 2147483648 ; width height of region
          ))

; FIXME: throws CRITICAL and sometimes crashes
;(assert `(gimp-drawable-merge-shadow
;            ,testDrawable
;            1 ; push merge to undo stack
;          ))

(assert `(gimp-drawable-offset
            ,testDrawable
            1 ; wrap around or fill
            OFFSET-WRAP-AROUND ; OffsetType
            -2147483648 -2147483648 ; x, y
            ))



; thumbnails

(assert `(gimp-drawable-thumbnail
            ,testDrawable
            1 1 ; thumbnail width height
          ))

(assert `(gimp-drawable-sub-thumbnail
            ,testDrawable
            1 1 ; origin
            2 2 ; width, height
            2 2 ; thumbnail width height
          ))


;gimp-drawable-extract-component is tested drawable-ops

;gimp-layer-new-from-drawable is tested w layer
