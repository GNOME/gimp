; test item methods of PDB

; Define function that is a sequence of tests.
; Iterate over items of different types, applying test function.
;    1. test attributes of a minimal item
;    2. test transformations of item

; Test of gimp-item-is-<ItemType> are elsewhere

; Test of tree/group (raise/lower, reorder) are elsewhere




; Implementation of test:
; function using assert must backquote ` and unquote , item.



; Test methods of bare, minimal item.
(define (test-bare-item item)
  ; item is a numeric ID is valid
  (assert `(gimp-item-id-is-valid ,item))

  ; item is not a group (can have no children)
  (assert `(= (car (gimp-item-is-group ,item))
              0))

  ; item has no color tag
  (assert `(= (car (gimp-item-get-color-tag ,item))
              COLOR-TAG-NONE))

  ; item is not expanded
  (assert `(= (car (gimp-item-get-expanded ,item))
              0))

  ; item has name, tattoo
  ; Test does not check returned value
  (assert `(gimp-item-get-name ,item))
  (assert `(gimp-item-get-tattoo ,item))

  ; item has no parasites, yields no list of string
  ; !!! C GStrv marshaled to empty list
  ; Scheme null? tests for empty list
  (assert `(null? (car (gimp-item-get-parasite-list ,item))))

  ; item has no parent
  ; yields -1 for NULL ID
  (assert `(= (car (gimp-item-get-parent ,item))
              -1))

  ; item has-a image
  ; Test does not compare item ID
  (assert `(gimp-item-get-image ,item))

  ; item's content, position, visibility is not locked
  (assert `(= (car (gimp-item-get-lock-content ,item))
              0))
  (assert `(= (car (gimp-item-get-lock-position ,item))
              0))
  (assert `(= (car (gimp-item-get-lock-visibility ,item))
              0))
)


; Test methods of image,item
(define (test-item-in-image image item)
  ; item can produce a selection
  (assert `(gimp-image-select-item
              ,image
              CHANNEL-OP-ADD
              ,item))
)



; !!! GimpParasite does not have method new in PDB.
; But you can create one in ScriptFu as (list "name" <flags> "data")
; <flags>
; 0 - Not persistent and not UNDOable
; 1 - Persistent and not UNDOable
; 2 - Not persistent and UNDOable
; 3 - Persistent and UNDOable

; https://www.gimpusers.com/forums/gimp-user/12970-how-are-parasites-represented-in-script-fu
; https://www.mail-archive.com/gimp-user@lists.xcf.berkeley.edu/msg20099.html

; A returned parasite in ScriptFu is-a list (list "name" <flags> "data")

; You can use this in testing but requires (quote ,testParasite) ???
;(define testParasite (list "Parasite New" 1 "Parasite Data"))

(define (test-item-parasite item)

  ; not has-a parasite
  ; !!! procedure expected to fail when no parasite
  (assert-error `(gimp-item-get-parasite
                  ,item
                  "Test Parasite") ; name
                "Procedure execution of gimp-item-get-parasite failed")

  ; can attach parasite
  (assert `(gimp-item-attach-parasite
              ,item
              (list "Parasite New" 1 "Parasite Data")))
  ; attach was effective: now item has parasite
  ; and its name is as previously given
  (assert `(string=?
              ; !!! Parasite is list in list, and first element is name
              (caar (gimp-item-get-parasite
                        ,item
                        "Parasite New")) ; name
              "Parasite New"))

  ; can detach parasite
  (assert `(gimp-item-detach-parasite
              ,item
              "Parasite New"))
  ; detach was effective
  (assert-error `(gimp-item-get-parasite
                  ,item
                  "Test Parasite") ; name
                "Procedure execution of gimp-item-get-parasite failed")
)


; use image,item instance extant from previous tests.

; text layer
(test-bare-item 15)
(test-item-in-image 8 15)
(test-item-parasite 15)

; layer
(test-bare-item 12)
(test-item-in-image 9 12)
(test-item-parasite 12)

; layerMask
(test-bare-item 14)
(test-item-in-image 9 14)
(test-item-parasite 14)

; vectors
; ID 16 is also a vectors named "foo"
(test-bare-item 19)
(test-item-in-image 10 19)
(test-item-parasite 19)

; channel
(test-bare-item 20)
(test-item-in-image 10 20)
(test-item-parasite 20)

; selection
(test-bare-item 18)
(test-item-in-image 10 18)
(test-item-parasite 18)

; TODO other item types e.g. ?

; gimp-image-get-item-position
; gimp-image-raise-item
; gimp-image-raise-item-to-top
; lower
; reorder
