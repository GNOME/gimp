; test paint-method methods of API

; tests setting a paint-method in context,
; then painting (stroking) with it.



; setup

; an image, drawable, and path
(define testImage (testing:load-test-image "gimp-logo.png"))
(define testLayer (vector-ref (cadr (gimp-image-get-layers testImage ))
                                  0))
(define testPath (car (gimp-vectors-new testImage "Test Path")))
; must add to image
(gimp-image-insert-vectors
                  testImage
                  testPath
                  0 0) ; parent=0 position=0
; Add stroke to path
(gimp-vectors-stroke-new-from-points
            testPath
            VECTORS-STROKE-TYPE-BEZIER
            12 ; count control points, 2*2
            (vector 1 2 3 4 5 6 7 8 9 10 11 12)
            FALSE) ; not closed



; paint-methods are introspectable to a list
(assert `(list? (gimp-context-list-paint-methods)))

; TODO
; test their names all have "gimp-" prefix and lower case.

; Test that every returned name is valid to set on the context
(define paintMethods (car (gimp-context-list-paint-methods)))
; TODO this doesn't seem to work: illegal function
; Probably the assert wrapper screws something up
; (assert `(map gimp-context-set-paint-method ,paintMethods))




; paint-method get/set on context

(assert `(gimp-context-set-paint-method "gimp-ink"))

; getter succeeds and setter was effective
(assert `(string=? (car (gimp-context-get-paint-method))
                  "gimp-ink"))





; Test that all paint-methods seem to work:
;    set context stroke method to paint-method
;    stroke a drawable along a path with the paint method
;       (except some paintMethods not painted with)

; set context to stroke with paint (versus line)
(assert `(gimp-context-set-stroke-method STROKE-PAINT-METHOD))

; iterate over paintMethods, testing that they seem to work

; test function that paints a path using a paint method.
; paintMethod is string
(define (testPaintMethod paintMethod)
    ; Test that every paintMethod can be set on the context
    (gimp-context-set-paint-method paintMethod)

    ; Don't paint with paint methods that need a source image set
    ; The API does not have a way to set source image
    ; TODO this is still failing with "Set a source first"
    (if (not (or
                (string=? paintMethod "gimp-clone")
                (string=? paintMethod "gimp-heal")
                (string=? paintMethod "gimp-perspective-clone")))
      (display paintMethod)
      ; paint with the method, under the test harness
      (assert `(gimp-drawable-edit-stroke-item ,testLayer ,testPath))
    ))

; apply testPaintMethod to each paintMethod
(for-each
  testPaintMethod
  paintMethods)
