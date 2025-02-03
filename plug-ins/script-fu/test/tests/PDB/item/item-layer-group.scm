; test Item methods for creating and positioning in GroupLayer


; The test script uses v3 binding of return values
(script-fu-use-v3)


; setup
(define testImage (testing:load-test-image-basic-v3 "gimp-logo.png"))
(displayln testImage)
(define testLayer (vector-ref (gimp-image-get-layers testImage )
                              0))
; gimp-logo.png has only one layer

; another layer for reorder tests
(define testLayer2 (testing:layer-new testImage))


; group-new not throw
; This is setup, not an assert, because we need to capture the group's ID
; Note the ID is not wrapped in list
(define testGroup (gimp-group-layer-new testImage ""))




; tests


(test! "new GroupLayer")

; group-layer-new was called above, not tested explicitly

; Name of the new group is a fabricated name
(assert `(string=? (gimp-item-get-name ,testGroup)
                   "Layer Group"))

; new group is not in the image until inserted.
; length of list of layers is one, the background.
(assert `(= (vector-length (gimp-image-get-layers ,testImage))
            1))

; Effective: new item is-a group
(assert `(gimp-item-id-is-group-layer ,testGroup))

; New group has no children
; vector length is zero
(assert `(= (vector-length (gimp-item-get-children ,testGroup))
            0))

; New group has no position until added to image
(assert-error `(gimp-image-get-item-position ,testImage ,testGroup)
               "Procedure execution of gimp-image-get-item-position failed on invalid input arguments:")
; Full text:
; Procedure execution of gimp-image-get-item-position failed on invalid input arguments:
; Item 'Layer Group' (3) cannot be used because it has not been added to an image


(test! "insert GroupLayer into image")

; group can be inserted in image
; Note group is zero (NULL)
(assert `(gimp-image-insert-layer
            ,testImage
            ,testGroup
            0  ; parent
            0  ))  ; position within parent

; insertion effective: image now has two layers, a layer and empty group
(assert `(= (vector-length (gimp-image-get-layers ,testImage))
            2))

; New group, when added to image, is at the top i.e. position 0 of the root level
(assert `(= (gimp-image-get-item-position ,testImage ,testGroup)
            0))

; Error to insert twice in the same position as already is
(assert-error `(gimp-image-insert-layer
                  ,testImage
                  ,testGroup
                  0  ; parent
                  0  )  ; position within parent
              "Procedure execution of gimp-image-insert-layer failed on invalid input arguments: ")
;  "Item 'LayerGroup' (3) has already been added to an image"

; Error to insert layer into invalid parent
; FIXME: we don't test this because:
; thereafter GIMP erroneously thinks the item has been added to some image.
; GIMP could be fixed so that even after this error, the item can be inserted
; into a valid parent i.e. 0
;(assert-error `(gimp-image-insert-layer
;                  ,testImage
;                  ,testGroup
;                  666  ; parent
;                  0  )  ; position within parent
;              "Procedure execution of gimp-image-insert-layer failed on invalid input arguments: ")




; !!! image-reorder-item has many purposes:
; 1) move child into group (from another level)
; 2) reorder i.e. move child within group


(test! "Add child to GroupLayer")
; Note testLayer already is in the image, but in the top (root) group

; Layer is initially at root, not in the new group
; -1 is ScriptFu NULL
(assert `(= (gimp-item-get-parent ,testLayer)
            -1))

; error to move layer item into self, i.e. an item not a group
(assert-error `(gimp-image-reorder-item ,testImage ,testLayer ,testLayer 0)
              "Procedure execution of gimp-image-reorder-item failed on invalid input arguments:")
; Item 'Background' (2) cannot be used because it is not a group item

; Not an error to move item into a group at a position larger than size of group
; The group layer is now empty.
(assert `(gimp-image-reorder-item ,testImage ,testLayer ,testGroup 666))
(assert `(= (gimp-image-get-item-position ,testImage ,testLayer)
            0))

; Not an error to move item into its current group at its current position
(assert `(gimp-image-reorder-item ,testImage ,testLayer ,testGroup 0))

; Add child is effective, group now has children
(assert `(= (vector-length (gimp-item-get-children ,testGroup))
            1))

; Add child is effective, now child's parent is the group
(assert `(= (gimp-item-get-parent ,testLayer)
            ,testGroup))

; !!! The added child is still at position 0 of it's level
; It's level is the group, not the root
(assert `(= (gimp-image-get-item-position ,testImage ,testLayer)
            0))

; TODO reparenting has no effect on which layer is selected




(test! "Remove child from GroupLayer")

(assert `(gimp-image-reorder-item
             ,testImage
             ,testLayer
             -1 ; parent 0 or -1 means remove
             0))

; effective: group has no items
(assert `(= (vector-length (gimp-item-get-children ,testGroup))
            0))

; effective: now child's parent is NULL i.e. -1  i.e. the top level
(assert `(= (gimp-item-get-parent ,testLayer)
            -1))

; restore test conditions, add child back to group
(assert `(gimp-image-reorder-item ,testImage ,testLayer ,testGroup 0))



(test! "order within a group")

; initial conditions:
; testLayer2 not in the image
; image
;   testGroup
;      testLayer

(assert `(= (vector-length (gimp-item-get-children ,testGroup))
            1))

; insert testLayer2 in the image
(assert `(gimp-image-insert-layer
            ,testImage
            ,testLayer2
            -1  ; parent is top level
            0  ))  ; position within parent

; The image now has two layers
; !!! This only returns the root layers
(assert `(= (vector-length (gimp-image-get-layers ,testImage))
            2))

; TODO Add testLayer2 to the image, and simultaneously to the group
; gimp-image-insert-layer

; Not an error to reorder into existing parent, position
(assert `(gimp-image-reorder-item ,testImage ,testLayer2
            -1 ; root, layer is already at root
            0)) ; position, layer is already at position 0

; move testLayer2 from top level into group
(assert `(gimp-image-reorder-item ,testImage ,testLayer2 ,testGroup 0))

; effective: image now has only one root layer, the group
(assert `(= (vector-length (gimp-image-get-layers ,testImage))
            1))

; effective: group now has two items
(assert `(= (vector-length (gimp-item-get-children ,testGroup))
            2))

; effective: testLayer moved down, is now at position 1 within group
(assert `(= (gimp-image-get-item-position ,testImage ,testLayer)
            1))

; Reorder within group, testLayer to position top
(assert `(gimp-image-reorder-item ,testImage ,testLayer ,testGroup 0))

; effective: testLayer is now at position 0 within group
(assert `(= (gimp-image-get-item-position ,testImage ,testLayer)
            0))

; effective: testLayer2 is now at position 1 within group
(assert `(= (gimp-image-get-item-position ,testImage ,testLayer2)
            1))



(test! "stack order of a group")

; A group can be raised and lowered.
; The group is already at both top and bottom of root level,
; so this is not a rigorous test.
(assert `(gimp-image-lower-item-to-bottom ,testImage ,testGroup))

; Raising a child does not take it out of it's group,
; the raising is within its level.
(assert `(gimp-image-raise-item-to-top ,testImage ,testLayer))
(assert `(= (gimp-item-get-parent ,testLayer)
            ,testGroup))



(test! "merge GroupLayer")
; Deprecated: image-merge-layer-group
; gimp-group-layer-merge converts item from group to normal layer
(assert `(gimp-group-layer-merge ,testGroup))
; group layer ID is now not a group
(assert `(not (gimp-item-id-is-group-layer ,testGroup)))

; the ID of the group layer is now invalid
(assert `(not (gimp-item-id-is-valid ,testGroup)))

; The image now has one layer, the merged group.
; It formerly had one group with two layers.
(assert `(= (vector-length (gimp-image-get-layers ,testImage))
            1))

(define mergedLayer (vector-ref (gimp-image-get-layers testImage) 0))

; the merged layer is not a group layer anymore
(assert `(not (gimp-item-id-is-group-layer ,mergedLayer)))

; !!! But the name sort of suggests it is
(assert `(string=? (gimp-item-get-name ,mergedLayer)
                   "Layer Group"))




(script-fu-use-v2)
