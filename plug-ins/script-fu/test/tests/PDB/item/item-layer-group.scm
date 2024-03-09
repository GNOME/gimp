; test Item methods for creating and positioning in layer groups



; setup
(define testImage (testing:load-test-image "gimp-logo.png"))
(define testLayer (vector-ref (cadr (gimp-image-get-layers testImage ))
                              0))

; group-new not throw
; This is setup, not an assert, because we need to capture the group's ID
; Note the ID is wrapped in list: (3)
(define testGroup (car (gimp-layer-group-new testImage)))




; tests

; new group is not in the image until inserted
(assert `(= (car (gimp-image-get-layers ,testImage))
            1))

; group can be inserted in image
; Note group is zero (NULL)
(assert `(gimp-image-insert-layer
            ,testImage
            ,testGroup
            0  ; parent
            0  ))  ; position within parent

; TODO Error to insert layer into invalid parent

; Effective: newly created item is-a group
(assert-PDB-true `(gimp-item-is-group ,testGroup))

; Newly created group has no children
; vector length is zero
(assert `(= (car (gimp-item-get-children ,testGroup))
            0))

; Newly created group is at the top i.e. position 0 of the root level
(assert `(= (car (gimp-image-get-item-position ,testImage ,testGroup))
            0))



; Add children
; Note add child is "reorder"

; Layer is initially not in the new group
; -1 is ScriptFu NULL
(assert `(= (car (gimp-item-get-parent ,testLayer))
            -1))

; Add child does not throw
(assert `(gimp-image-reorder-item ,testImage ,testLayer ,testGroup 0))

; Add child is effective, group now has children
(assert `(= (car (gimp-item-get-children ,testGroup))
            1))

; Add child is effective, now child's parent is the group
(assert `(= (car (gimp-item-get-parent ,testLayer))
            ,testGroup))

; !!! The added child is still at position 0 of it's level
; It's level is the group, not the root
(assert `(= (car (gimp-image-get-item-position ,testImage ,testLayer))
            0))

; TODO reparenting has no effect on which layer is selected




; stack order group

; A group can be raised and lowered.
; The group is already at both top and bottom of root level,
; so this is not a rigorous test.
(assert `(gimp-image-lower-item-to-bottom ,testImage ,testGroup))

; Raising a child does not take it out of it's group,
; the raising is within its level
(assert `(gimp-image-raise-item-to-top ,testImage ,testLayer))
(assert `(= (car (gimp-item-get-parent ,testLayer))
            ,testGroup))


; TODO gimp-image-merge-layer-group
; merge-layer-group means the item is not a group any longer
; the new layer is at the same position in the root level as the group was

