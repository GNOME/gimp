; test Item methods for creating and positioning in GroupLayer

; This is part 2: more tests of more arcane cases

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
(define testGroup2 (gimp-group-layer-new testImage ""))



; tests


(test! "nested GroupLayer")

; group can be inserted in image
; Note parent is zero (NULL)
(assert `(gimp-image-insert-layer
            ,testImage
            ,testGroup
            0  ; parent
            0  ))  ; position within parent

; insertion effective: image now has two root layers, a layer and empty group
(assert `(= (vector-length (gimp-image-get-layers ,testImage))
            2))

; Groups can be nested
(assert `(gimp-image-insert-layer
            ,testImage
            ,testGroup2
            ,testGroup  ; parent
            0  ))  ; position within parent

; insertion effective: image still has only two root layers, a layer and a group
(assert `(= (vector-length (gimp-image-get-layers ,testImage))
            2))




(test! "move item between two groups")

; Initial condition:
; testLayer
; testGroup
;    testGroup2

; testLayer is at the top
(assert `(= (gimp-item-get-parent ,testLayer) -1))

; move testLayer to next level
(assert `(gimp-image-reorder-item ,testImage ,testLayer ,testGroup 0))

; reorder effective: image has one root layer, a group
(assert `(= (vector-length (gimp-image-get-layers ,testImage))
            1))
; effective: testLayer is parented to testGroup
(assert `(= (gimp-item-get-parent ,testLayer) ,testGroup))

; Condition:
; testGroup
;    testLayer
;    testGroup2

; move to a deeper group
(assert `(gimp-image-reorder-item ,testImage ,testLayer ,testGroup2 0))

; effective: testLayer is parented to testGroup2
(assert `(= (gimp-item-get-parent ,testLayer) ,testGroup2))




(test! "Error to create cycles in layer graph")

; Now group2 is in group.  Error to move group into group2, i.e. parent into child
(assert-error `(gimp-image-reorder-item
                  ,testImage
                  ,testGroup  ; moved item
                  ,testGroup2 ; parent
                  0)
              "Procedure execution of gimp-image-reorder-item failed on invalid input arguments:")
; suffix of err msg:
; Item 'Layer Group' (4) must not be an ancestor of 'Layer Group #1' (5)




(test! "Text layers can be reordered")

(define testFont (gimp-context-get-font))

(define testTextLayer
  (gimp-text-font
    testImage
    -1     ; drawable.  -1 means NULL means create new text layer
    0 0   ; coords
    "bar" ; text
    1     ; border size
    1     ; antialias true
    31    ; fontsize
    testFont ))

; text layer is already in image

; Condition:
; testTextLayer
; testGroup
;    testLayer
;    testGroup2

(assert `(= (vector-length (gimp-image-get-layers ,testImage))
            2))
(assert `(= (gimp-image-get-item-position ,testImage ,testTextLayer)
            0))

; move TextLayer to a deeper group
(assert `(gimp-image-reorder-item ,testImage ,testTextLayer ,testGroup2 0))

; effective: TextLayer parent is testGroup2
(assert `(= (gimp-item-get-parent ,testTextLayer) ,testGroup2))





; Channels and Paths are in a separate item tree
; No support for groups of Channels or Paths


(test! "Channels as Item with parent and position")

; Channels are Items but reorder-item only works with subclass Layer
; There is no such thing as a ChannelGroup

; I.E. only layers can be reordered
(define testChannel (gimp-channel-new
            testImage      ; image
            "Test Channel" ; name
            23 24          ; width, height
            50.0           ; opacity
            "red" ))      ; compositing color

; Can insert into image
(assert `(gimp-image-insert-channel
            ,testImage
            ,testChannel
            0            ; parent, moot since channel groups not supported
            0))          ; position in stack

; A channel has a position in its parent
(assert `(= (gimp-image-get-item-position ,testImage ,testChannel)
            0))

; A channel parent is always top level.
; The parent of a channel is not a layer.
(assert `(= (gimp-item-get-parent ,testChannel) -1))



(test! "Channels cannot be reordered into the layer tree")

; Error to parent a channel into the layer tree, into a group layer
(assert-error `(gimp-image-reorder-item
                  ,testImage
                  ,testChannel ; moved item
                  ,testGroup2  ; parent
                  0)
              "Procedure execution of gimp-image-reorder-item failed on invalid input arguments:")
; suffix of err message
; Items 'Test Channel' (6) and 'Layer Group #1' (5) cannot be used because they are not part of the same item tree



(test! "Path layers as items with parent and position.")

(define testPath (gimp-path-new
                        testImage
                        "Test Path"))
; a new Path is not in the image
(assert `(= (vector-length (gimp-image-get-paths ,testImage))
            0))

; !!! Even before inserted in image, path has parent but not position!!!

; Path has parent
(assert `(= (gimp-item-get-parent ,testPath)
            -1))

; Error to get position of Path not inserted in image
(assert-error `(gimp-image-get-item-position ,testImage ,testPath)
              "Procedure execution of gimp-image-get-item-position failed on invalid input arguments:")
; suffix of err msg
; Item 'Test Path' (8) cannot be used because it has not been added to an image

(assert  `(gimp-image-insert-path
                  ,testImage
                  ,testPath
                  0 0)) ; parent=0 position=0

; A Path's parent is always top level.
; The parent of a Path is not a layer.
(assert `(= (gimp-item-get-parent ,testPath)
            -1))

; A Path has a position in its parent, after Path was inserted
(assert `(= (gimp-image-get-item-position ,testImage ,testPath)
            0))



(test! "Path layers cannot be reordered into layer tree")

; Error to parent a Path into the layer tree, into a group layer
(assert-error `(gimp-image-reorder-item
                  ,testImage
                  ,testPath ; moved item
                  ,testGroup2  ; parent
                  0)
              "Procedure execution of gimp-image-reorder-item failed on invalid input arguments:")
; suffix of err msg
; Items 'Test Path' (8) and 'Layer Group #1' (5) cannot be used because they are not part of the same item tree




(script-fu-use-v2)
