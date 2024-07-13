; test Item methods for creating and positioning in GroupLayer




; setup
(define testImage (testing:load-test-image "gimp-logo.png"))
(displayln testImage)
(define testLayer (vector-ref (cadr (gimp-image-get-layers testImage ))
                              0))

; !!! Note that load-test-image is not script-fu-v3 compatible, already unpacks
(script-fu-use-v3)
; The rest of the test script is v3, no need for car to unpack single values


; group-new not throw
; This is setup, not an assert, because we need to capture the group's ID
; Note the ID is not wrapped in list
(define testGroup (gimp-group-layer-new testImage))


; tests

; new group is not in the image until inserted
; length of list of layers is one, the background
(assert `(= (car (gimp-image-get-layers ,testImage))
            1))

(test! "insert GroupLayer")
; group can be inserted in image
; Note group is zero (NULL)
(assert `(gimp-image-insert-layer
            ,testImage
            ,testGroup
            0  ; parent
            0  ))  ; position within parent

; insertion effective: image now has two layers
(assert `(= (car (gimp-image-get-layers ,testImage))
            2))

; TODO Error to insert layer into invalid parent


; Effective: newly created item is-a group
(assert `(gimp-item-id-is-group-layer ,testGroup))

; Newly created group has no children
; vector length is zero
(assert `(= (car (gimp-item-get-children ,testGroup))
            0))

; Newly created group is at the top i.e. position 0 of the root level
(assert `(= (gimp-image-get-item-position ,testImage ,testGroup)
            0))


(test! "Add children to GroupLayer")
; Note add child is "reorder"

; Layer is initially not in the new group
; -1 is ScriptFu NULL
(assert `(= (gimp-item-get-parent ,testLayer)
            -1))

; Add child does not throw
(assert `(gimp-image-reorder-item ,testImage ,testLayer ,testGroup 0))

; Add child is effective, group now has children
(assert `(= (car (gimp-item-get-children ,testGroup))
            1))

; Add child is effective, now child's parent is the group
(assert `(= (gimp-item-get-parent ,testLayer)
            ,testGroup))

; !!! The added child is still at position 0 of it's level
; It's level is the group, not the root
(assert `(= (gimp-image-get-item-position ,testImage ,testLayer)
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
(assert `(= (gimp-item-get-parent ,testLayer)
            ,testGroup))


(test! "merge GroupLayer")
; Deprecated: image-merge-layer-group
; gimp-group-layer-merge converts item from group to normal layer
; FIXME (script-fu:397): LibGimpConfig-WARNING **: 18:02:38.773: gimp_config_class_init:
; not supported: GimpParamGroupLayer (group-layer | GimpGroupLayer)
(assert `(gimp-group-layer-merge ,testGroup))
; group layer ID is now not a group
(assert `(not (gimp-item-id-is-group-layer ,testGroup)))
; the new layer is at the same position in the root level as the group was
; TODO we didn't capture the new layer ID



(script-fu-use-v2)