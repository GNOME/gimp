; test Image methods of PDB

; loading this file changes testing state

; Using numeric equality operator '=' on numeric ID's


;            setup

; method new from fresh GIMP state returns ID 1
(define testImage (car (gimp-image-new 21 22 RGB)))





; method is_valid on new image yields 1 i.e. true
(assert-PDB-true `(gimp-image-id-is-valid ,testImage))


; Ensure attributes of new image are correct

; method is_dirty on new image is true
(assert-PDB-true `(gimp-image-is-dirty ,testImage))

; method get_width on new image yields same width given when created
(assert `(=
            (car (gimp-image-get-width ,testImage))
            21))

; method get_height on new image yields same height given when created
(assert `(=
            (car (gimp-image-get-height ,testImage))
            22))

; method get-base-type yields same image type given when created
(assert `(=
            (car (gimp-image-get-base-type ,testImage))
            RGB))

; new image is known to gimp.
; Returns (<length> #(1))
; Test that the length is 1.
; !!! This is sensitive to retest, if a test leaves images.
(assert `(= (car (gimp-get-images))
             1))


;        new image has few components

; !!!!
; New image has one drawable, the selection mask.
; Note there is no gimp-image-get-drawables
; FIXME: this is susceptible to test order:
; subsequent images will have different ID for selection mask.
(assert-PDB-true `(gimp-item-id-is-valid 1))
; Item 1 is the Selection Mask.
(assert `(string=? (car (gimp-item-get-name 1))
                   "Selection Mask"))



; new image has zero layers
(assert `(= (car (gimp-image-get-layers ,testImage))
            0))

; new image has zero paths
(assert `(= (car (gimp-image-get-paths ,testImage))
            0))

; new image has no parasites
(assert `(= (length
              (car (gimp-image-get-parasite-list ,testImage)))
            0))





; new image has-a selection
(assert `(gimp-image-get-selection ,testImage))

; new image has no floating selection
(assert `(=
          (car (gimp-image-get-floating-sel ,testImage))
          -1))

; TODO floating-sel-attached-to



; new image has unit having ID 1
(assert `(=
            (car (gimp-image-get-unit ,testImage))
            1))

; new image has name
(assert `(string=?
            (car (gimp-image-get-name ,testImage))
            "[Untitled]"))

; new image has empty metadata string
(assert `(string=?
            (car (gimp-image-get-metadata ,testImage))
            ""))

; has an effective color profile
(assert `(gimp-image-get-effective-color-profile ,testImage))



;        new image has no associated files

; GFile is string in ScriptFu

; no file, xcf file, imported file, or exported file
(assert `(string=? (car (gimp-image-get-file     ,testImage)) ""))
(assert `(string=? (car (gimp-image-get-xcf-file ,testImage)) ""))
(assert `(string=? (car (gimp-image-get-imported-file ,testImage)) ""))
(assert `(string=? (car (gimp-image-get-exported-file ,testImage)) ""))



; Test delete method.
; !!! ID 1 is no longer valid

; method delete succeeds on new image
; returns 1 for true.  FUTURE returns #t
(assert `(car (gimp-image-delete ,testImage)))

; ensure id invalid for deleted image
; returns 0 for false.  FUTURE returns #f
(assert `(=
            (car (gimp-image-id-is-valid ,testImage))
            0))

; deleted image is not in gimp
; Returns (<length> #())
; FUTURE Returns empty list `()
(assert `(=
            (car (gimp-get-images))
            0))
; !!! This only passes when testing is from fresh Gimp restart


; Test abnormal args to image-new


; Dimension zero yields error
; It does NOT yield invalid ID -1
(assert-error `(gimp-image-new 0 0 RGB)
              "Invalid value 0 for argument 0: expected value between 1 and 524288")

; Since 3.0, parameter validation catches this earlier.
; Formerly,  "Procedure execution of gimp-image-new failed on invalid input arguments: "
; "Procedure 'gimp-image-new' has been called with value '0' for argument 'width' (#1, type gint)."))
; " This value is out of range."




