; test get/set attributes of drawable


; setup

(define testImage (testing:load-test-image "gimp-logo.png"))
; Wilber has one layer
; cadr is vector, first element is a drawable
(define testDrawable (vector-ref (cadr (gimp-image-get-layers testImage)) 0))




; a drawable is represented by an ID
; As an item, it is type Drawable
(assert-PDB-true `(gimp-item-id-is-drawable ,testDrawable))




; getters

; only testing getters that are not of the superclass Item

; Returned single values are wrapped in a list.

; bytes per pixel
(assert `(number? (car (gimp-drawable-get-bpp ,testDrawable))))
; height and width are single numbers
(assert `(number? (car (gimp-drawable-get-height ,testDrawable))))
(assert `(number? (car (gimp-drawable-get-width ,testDrawable))))
; offset is list of two numbers
(assert `(list? (gimp-drawable-get-offsets ,testDrawable)))
; formats are strings encoded for babl
(assert `(string? (car (gimp-drawable-get-format ,testDrawable))))
(assert `(string? (car (gimp-drawable-get-thumbnail-format ,testDrawable))))


; the test drawable has transparency
; FUTURE: inconsistent naming, should be gimp-drawable-get-alpha?
(assert-PDB-true `(gimp-drawable-has-alpha ,testDrawable))

; the test drawable has image base type RGB
(assert-PDB-true `(gimp-drawable-is-rgb ,testDrawable))
(assert-PDB-false `(gimp-drawable-is-gray ,testDrawable))
(assert-PDB-false `(gimp-drawable-is-indexed ,testDrawable))

; the test drawable has type RGBA
(assert `(= (car (gimp-drawable-type ,testDrawable))
            RGBA-IMAGE))


; These are deprecated.
; Scripts should use superclass gimp-item-get.
; Which are tested elsewhere
;(assert `(gimp-drawable-get-image ,testDrawable))
;(assert `(gimp-drawable-get-name ,testDrawable))
;(assert `(gimp-drawable-get-tattoo ,testDrawable))
; the test drawable is visible
;(assert-PDB-true `(gimp-drawable-get-visible ,testDrawable))




; TODO setters

