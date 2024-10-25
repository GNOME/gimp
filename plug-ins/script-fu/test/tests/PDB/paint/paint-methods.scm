; test paint-method methods of API

; tests setting a paint-method in context,
; then painting (stroking) with it.


(script-fu-use-v3)

; setup

; an image, drawable, and path
(define testImage (testing:load-test-image-basic-v3))
(define testLayer (vector-ref (gimp-image-get-layers testImage )
                                  0))
(define testPath (gimp-path-new testImage "Test Path"))
; must add to image
(gimp-image-insert-path
                  testImage
                  testPath
                  0 0) ; parent=0 position=0
; Add stroke to path
(gimp-path-stroke-new-from-points
            testPath
            PATH-STROKE-TYPE-BEZIER
            (vector 1 2 3 4 5 6 7 8 9 10 11 12) ; control points
            FALSE) ; not closed



(test! "paint-methods are introspectable to a list of strings")
(assert `(list? (gimp-context-list-paint-methods)))

; setup
(define paintMethods (gimp-context-list-paint-methods))

; TODO
; test their names all have "gimp-" prefix and lower case.

; Test that every returned name is valid to set on the context
; TODO this doesn't seem to work: illegal function
; Probably the assert wrapper screws something up
; (assert `(map gimp-context-set-paint-method ,paintMethods))




 (test! "get/set paint-method on context")

(assert `(gimp-context-set-paint-method "gimp-ink"))

; getter succeeds and setter was effective
(assert `(string=? (gimp-context-get-paint-method)
                  "gimp-ink"))





; Test that all paint-methods seem to work:
;    set context stroke method to paint-method
;    stroke a drawable along a path with the paint method
;       (except some paintMethods not painted with)

(test! "set context to stroke with paint (versus line)")
(assert `(gimp-context-set-stroke-method STROKE-PAINT-METHOD))


(test! "iterate over paintMethods, loosely testing they seem to work")

; test function that paints a path using a paint method.
; paintMethod is string
(define (testPaintMethod paintMethod)
    ; paintMethod can be set on the context
    (gimp-context-set-paint-method paintMethod)

    ; Don't paint with paint methods that need a source image set
    ; The API does not have a way to set source image
    (if (not (or
                (string=? paintMethod "gimp-clone")
                (string=? paintMethod "gimp-heal")
                (string=? paintMethod "gimp-perspective-clone")))
      ; paint with the method, under the test harness
      (begin
        (test! paintMethod)
        (assert `(gimp-drawable-edit-stroke-item ,testLayer ,testPath)))
      ; else skip
      (test! (string-append "Skipping: " paintMethod))
    ))

; apply testPaintMethod to each paintMethod
(for-each
  testPaintMethod
  paintMethods)

(script-fu-use-v2)
