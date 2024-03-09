; test Image set/get-selected-foo methods of PDB

; These methods are not selecting an area in the image.
; They are selecting some of the image's objects
; in a chooser widget of the app.

; !!! Note that user selecting drawables (e.g. layers)
; deselects any previously selected drawables (e.g. channels)
; While user can select a vectors (aka path) concurrently with a drawable.

; Test getter right after setter.
; Test that the ID is the same as passed to setter.
; !!! Also test what should be deselected in some cases.

;             setup

;(define testImage (car (gimp-image-new 21 22 RGB)))

; Load test image that already has drawable
(define testImage (testing:load-test-image "gimp-logo.png"))

(define testLayers (cadr (gimp-image-get-layers testImage )))
; assert testLayers is-a vector of length one
(define testLayer (vector-ref testLayers 0))

; create test channel
; The test image has no channels (RGB are not considered channels.)
(define testChannel (car (gimp-channel-new
            testImage    ; image
            23 24          ; width, height
            "Test Channel" ; name
            50.0           ; opacity
            "red" )))      ; compositing color
; a new channel is not in the image until added
(gimp-image-insert-channel testImage testChannel)
; get a vector that only has the new channel
(define testChannels (cadr (gimp-image-get-channels testImage )))

; create test path
(define testPath (car (gimp-vectors-new
                        testImage
                        "Test Path")))
(gimp-image-insert-vectors testImage testPath 0 0)
(define testVectors (cadr (gimp-image-get-vectors testImage )))



; basic tests that void methods succeed
; when passed a non-empty list of objects

; layer
(assert `(gimp-image-set-selected-layers
            ,testImage
            1 ,testLayers ))
; effective: one layer is selected
(assert `(= (car (gimp-image-get-selected-layers ,testImage))
            1))
; effective: selected layer is the one we just selected
(assert `(= (vector-ref (cadr (gimp-image-get-selected-layers ,testImage)) 0)
            ,testLayer))



; channel
(assert `(gimp-image-set-selected-channels
            ,testImage
            1 ,testChannels ))
; effective: one channel is selected
(assert `(= (car (gimp-image-get-selected-channels ,testImage))
            1))
; effective: selected channel is the one we just selected
(assert `(= (vector-ref (cadr (gimp-image-get-selected-channels ,testImage)) 0)
            ,testChannel))
; !!! Selecting a channel deselects previously selected layer
(assert `(= (car (gimp-image-get-selected-layers ,testImage))
            0))


; vectors
; Failed before #10188 fixed
(assert `(gimp-image-set-selected-vectors
            ,testImage
            1 ,testVectors ))
; Selecting a path means it is selected
(assert `(= (vector-ref (cadr (gimp-image-get-selected-vectors ,testImage)) 0)
            ,testPath))
; Selecting a path does not unselect a drawable i.e. previously selected channel
(assert `(= (vector-ref (cadr (gimp-image-get-selected-channels ,testImage)) 0)
            ,testChannel))


; The generic getter get-selected-drawables
; Returns a homogenous vector of previously selected channels.
(assert `(= (vector-ref (cadr (gimp-image-get-selected-drawables ,testImage)) 0)
            ,testChannel))


; TODO test floating layer???
; The docs mention failing, test what the docs say.


; iError to pass empty vector to setter
(assert-error `(gimp-image-set-selected-layers
                  ,testImage
                  0 #() )
              "Invalid value for argument 2")
(assert-error `(gimp-image-set-selected-channels
                  ,testImage
                  0 #() )
              "Invalid value for argument 2")
(assert-error `(gimp-image-set-selected-vectors
                  ,testImage
                  0 #() )
              "Invalid value for argument 2")
