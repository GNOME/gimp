; test Image methods of PDB

; loading this file changes testing state

; depends on fresh GIMP state
; !!!! tests hardcode image ID 1

; Using numeric equality operator '=' on numeric ID's


; method new from fresh GIMP state returns ID 1
(assert '(=
           (car (gimp-image-new 21 22 RGB))
           1 ))

; FUTURE method is_valid on new image yields #t
; method is_valid on new image yields 1 i.e. true
(assert '(=
           (car (gimp-image-id-is-valid 1))
           1))

; Ensure attributes of new image are correct

; method is_dirty on new image is true
(assert '(=
            (car (gimp-image-is-dirty 1))
            1))

; method get_width on new image yields same width given when created
(assert '(=
            (car (gimp-image-get-width 1))
            21))

; method get_height on new image yields same height given when created
(assert '(=
            (car (gimp-image-get-height 1))
            22))

; method get-base-type yields same image type given when created
(assert '(=
            (car (gimp-image-get-base-type 1))
            RGB))

; new image is known to gimp
; Returns (<length> #(1))
(assert '(= (car (gimp-get-images))
            1))


;        new image has no components

; new image has zero layers
(assert '(= (car (gimp-image-get-layers 1))
            0))

; new image has zero vectors
(assert '(= (car (gimp-image-get-vectors 1))
            0))

; new image has no parasites
(assert '(= (length
              (car (gimp-image-get-parasite-list 1)))
            0))





; new image has-a selection
(assert '(gimp-image-get-selection 1))

; new image has no floating selection
(assert '(=
          (car (gimp-image-get-floating-sel 1))
          -1))

; TODO floating-sel-attached-to



; new image has unit having ID 1
(assert '(=
            (car (gimp-image-get-unit 1))
            1))

; new image has name
(assert '(string=?
            (car (gimp-image-get-name 1))
            "[Untitled]"))

; new image has empty metadata string
(assert '(string=?
            (car (gimp-image-get-metadata 1))
            ""))

; has an effective color profile
(assert '(gimp-image-get-effective-color-profile 1))



;        new image has no associated files

; GFile is string in ScriptFu

; no file, xcf file, imported file, or exported file
(assert '(string=? (car (gimp-image-get-file 1)) ""))
(assert '(string=? (car (gimp-image-get-xcf-file 1)) ""))
(assert '(string=? (car (gimp-image-get-imported-file 1)) ""))
(assert '(string=? (car (gimp-image-get-exported-file 1)) ""))



; Test delete method.
; !!! ID 1 is no longer valid

; method delete succeeds on new image
; returns 1 for true.  FUTURE returns #t
(assert `(car (gimp-image-delete 1)))

; ensure id invalid for deleted image
; returns 0 for false.  FUTURE returns #f
(assert `(=
            (car (gimp-image-id-is-valid 1))
            0))

; deleted image is not in gimp
; Returns (<length> #())
; FUTURE Returns empty list '()
(assert '(=
            (car (gimp-get-images))
            0))



; Test abnormal args to image-new


; Dimension zero yields error
; It does NOT yield invalid ID -1
(assert-error '(gimp-image-new 0 0 RGB)
"Procedure execution of gimp-image-new failed on invalid input arguments: Procedure 'gimp-image-new' has been called with value '0' for argument 'width' (#1, type gint). This value is out of range." )




