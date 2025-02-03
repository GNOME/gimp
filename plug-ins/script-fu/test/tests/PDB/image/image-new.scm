; test Image methods of PDB

; loading this file changes testing state

; Using numeric equality operator '=' on numeric ID's

; Returned values not wrapped in lists.
(script-fu-use-v3)



;            setup

; method new from fresh GIMP state returns ID 1
(define testImage (gimp-image-new 21 22 RGB))





; method is_valid on new image yields 1 i.e. true
(assert `(gimp-image-id-is-valid ,testImage))


(test! "attributes of new image")

; method is_dirty on new image is true
(assert `(gimp-image-is-dirty ,testImage))

; method get_width on new image yields same width given when created
(assert `(=
            (gimp-image-get-width ,testImage)
            21))

; method get_height on new image yields same height given when created
(assert `(=
            (gimp-image-get-height ,testImage)
            22))

; method get-base-type yields same image type given when created
(assert `(=
            (gimp-image-get-base-type ,testImage)
            RGB))

; new image is known to gimp.
; Returns (<length> #(1))
; Test that the length is 1.
; !!! This is sensitive to retest, if a test leaves images.
(assert `(= (vector-length (gimp-get-images))
             1))


(test! "new image has few components")

; !!!!
; New image has one drawable, the selection mask.
; Note there is no gimp-image-get-drawables
; FIXME: this is susceptible to test order:
; subsequent images will have different ID for selection mask.
(assert `(gimp-item-id-is-valid 1))
; Item 1 is the Selection Mask.
(assert `(string=? (gimp-item-get-name 1)
                   "Selection Mask"))



; new image has zero layers
(assert `(= (vector-length (gimp-image-get-layers ,testImage))
            0))

; new image has zero paths
(assert `(= (vector-length (gimp-image-get-paths ,testImage))
            0))

; new image has no parasites
; returns a GStrv i.e. just a list
(assert `(= (length
              (gimp-image-get-parasite-list ,testImage))
            0))




(test! "new image has selection")

(assert `(gimp-image-get-selection ,testImage))

; new image has no floating selection
(assert `(=
          (gimp-image-get-floating-sel ,testImage)
          -1))



; new image has unit having ID 1
(assert `(=
            (gimp-image-get-unit ,testImage)
            1))

; new image has name
(assert `(string=?
            (gimp-image-get-name ,testImage)
            "[Untitled]"))

; since 3.0rc image-get-metadata private to libgimp
; new image has empty metadata string
;(assert `(string=?
;            (gimp-image-get-metadata ,testImage)
;            ""))


; since 3.0rc image-get-metadata private to libgimp
;(test! "new image has an effective color profile")
;(assert `(gimp-image-get-effective-color-profile ,testImage))



(test! "new image has no associated files")

; GFile is string in ScriptFu

; no file, xcf file, imported file, or exported file
(assert `(string=? (gimp-image-get-file     ,testImage)
                    ""))
(assert `(string=? (gimp-image-get-xcf-file ,testImage)
                    ""))
(assert `(string=? (gimp-image-get-imported-file ,testImage)
                   ""))
(assert `(string=? (gimp-image-get-exported-file ,testImage)
                   ""))



(test! "image delete method")

; !!! ID 1 is no longer valid

; method delete succeeds on new image
; returns 1 for true in v2.  returns #t in v3
(assert `(gimp-image-delete ,testImage))

; ensure id invalid for deleted image
; returns 0 for false in v2.  returns #f in v3
(assert `(not (gimp-image-id-is-valid ,testImage)))


; deleted image is not in gimp
; Returns (<length> #())
; FUTURE Returns empty list `()
(assert `(=
            (vector-length (gimp-get-images))
            0))
; !!! This only passes when testing is from fresh Gimp restart


(test! "abnormal args to image-new")


; Dimension zero yields error
; It does NOT yield invalid ID -1
(assert-error `(gimp-image-new 0 0 RGB)
               "argument 1 in call to gimp-image-new has value 0 out of range: 1 to 524288")
; Not this: "Invalid value for argument 0")

; Since 3.0, parameter validation catches this earlier.
; Formerly,  "Procedure execution of gimp-image-new failed on invalid input arguments: "
; "Procedure 'gimp-image-new' has been called with value '0' for argument 'width' (#1, type gint)."))
; " This value is out of range."

(script-fu-use-v2)


