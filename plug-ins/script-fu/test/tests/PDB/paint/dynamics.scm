; Test methods related to paint dynamics



; setup

; an image, drawable, and path
(define testImage (testing:load-test-image "gimp-logo.png"))
(define testLayer (vector-ref (car (gimp-image-get-layers testImage ))
                                  0))
(define testPath (car (gimp-path-new testImage "Test Path")))
; must add to image
(gimp-image-insert-path
                  testImage
                  testPath
                  0 0) ; parent=0 position=0
; Add stroke to path
; TODO enum still named "VECTORS" => PATH
(gimp-path-stroke-new-from-points
            testPath
            PATH-STROKE-TYPE-BEZIER
            (vector 1 2 83 84 5 6 7 8 9 10 11 12) ; control points
            FALSE) ; not closed

; make test harder by using float precision
(gimp-image-convert-precision testImage PRECISION-DOUBLE-NON-LINEAR)
; ensure testing is stroking with paint (versus line)
(gimp-context-set-stroke-method STROKE-PAINT-METHOD)
; ensure testing is painting with paintbrush (versus pencil, airbrush, etc.)
(gimp-context-set-paint-method "gimp-paintbrush")
; make test harder by using a big, color brush
(gimp-context-set-brush (car (gimp-brush-get-by-name "Wilber")))




; methods of the gimp module

; introspection: gimp module returns list of names of dynamics
; second arg is a regex
; FORMERLY get-list
(assert `(list? (gimp-dynamics-get-name-list "")))

; refresh: gimp module will load newly installed dynamics
; method is void and should never fail.
(assert `(gimp-dynamics-refresh))

; TODO install a new dynamic and test that refresh is effective






; context setting

; the dynamics setting defaults to true
; !!! test requires freshly installed GIMP OR no prior testing
(assert-PDB-true `(gimp-context-are-dynamics-enabled))

; the dynamics-enabled setting can be set to false
; TODO #f instead of 0
(assert `(gimp-context-enable-dynamics 0))
; setting to false was effective
(assert-PDB-false `(gimp-context-are-dynamics-enabled))
; restore to enabled for further testing
(assert `(gimp-context-enable-dynamics 1))


; the dynamics setting can be set to the name of a dynamics
(assert `(gimp-context-set-dynamics-name "Tilt Angle"))
; setting was effective
; formerly context-[set,get]-dynamics
(assert `(string=? (car (gimp-context-get-dynamics-name))
                   "Tilt Angle"))



;  TODO test all the dynamics seems to work

; Test that all dynamics seem to work:
;    set context to dynamics
;    stroke a drawable along a path with current brush and dynamics


(define dynamicsList (car (gimp-dynamics-get-name-list "")))

(define (testDynamics dynamics)
    ; Test that every dynamics can be set on the context
    (gimp-context-set-dynamics-name dynamics)

    (display dynamics)
    ; paint with paintbrush and dynamics, under the test harness
    (assert `(gimp-drawable-edit-stroke-item ,testLayer ,testPath))
   )

; apply testDynamics to each dynamics kind.
; This is not a difficult test since the stroke is uniform w/r to dynamics.
; The stroke does not vary by e.g. pressure.
(for-each
  testDynamics
  dynamicsList)

;(gimp-display-new testImage)
