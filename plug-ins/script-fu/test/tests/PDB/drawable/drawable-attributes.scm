; test get/set attributes of drawable

; The test script uses v3 binding of return values
(script-fu-use-v3)


; setup

(define testImage (testing:load-test-image-basic-v3))
; Loaded "gimp-logo.png" i.e. Wilber having one layer
(define testDrawable (vector-ref (gimp-image-get-layers testImage) 0))




; a drawable is represented by an ID
; As an item, it is type Drawable
(assert `(gimp-item-id-is-drawable ,testDrawable))




(test! "getters of Drawable")

; only testing getters that are not of the superclass Item

; bytes per pixel
(assert `(number? (gimp-drawable-get-bpp ,testDrawable)))
; height and width are single numbers
(assert `(number? (gimp-drawable-get-height ,testDrawable)))
(assert `(number? (gimp-drawable-get-width ,testDrawable)))
; offset is list of two numbers
(assert `(list? (gimp-drawable-get-offsets ,testDrawable)))

; since 3.0rc2 drawable-get-format is private to libgimp
; formats are strings encoded for babl
; (assert `(string? (gimp-drawable-get-format ,testDrawable)))

; Since 3.0rc2, this is private to libgimp
;(assert `(string? (gimp-drawable-get-thumbnail-format ,testDrawable)))


; the test drawable has transparency
; FUTURE: inconsistent naming, should be gimp-drawable-get-alpha?
(assert `(gimp-drawable-has-alpha ,testDrawable))

; the test drawable has image base type RGB
(assert `(gimp-drawable-is-rgb ,testDrawable))
(assert `(not (gimp-drawable-is-gray ,testDrawable)))
(assert `(not (gimp-drawable-is-indexed ,testDrawable)))

; the test drawable has type RGBA
(assert `(= (gimp-drawable-type ,testDrawable)
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


(script-fu-use-v2)
