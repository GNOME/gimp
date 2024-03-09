; Test stroke methods of vector class of the PDB

; in the console, this typically works to debug:
; i.e. these are the typical IDs
; (gimp-image-get-vectors 1) => (1 #(3))
; (gimp-vectors-get-strokes 3) => (1 #(2))
;  (gimp-vectors-stroke-get-points 3 2) => (0 6 #(200.0 200.0 200.0 200.0 200.0 200.0) 0)

; setup

(define testImage (testing:load-test-image "gimp-logo.png"))
(define testLayer (vector-ref (cadr (gimp-image-get-layers testImage ))
                                  0))

(define testPath (car (gimp-vectors-new
                        testImage
                        "Test Path")))
(gimp-image-insert-vectors
    testImage
    testPath
    0 0)


; tests


; A stroke has two endpoints.
; Each endpoint as three control points: HAH (sometimes said CAC)
; Where H is a handle, A is an anchor.
; Each control point is a coordinate i.e. pair of floats.
; The count of elements in the array is minimum of 12
; Two endpoints * three control points per endpoint * 2 floats per control point
(assert `(gimp-vectors-stroke-new-from-points
            ,testPath
            VECTORS-STROKE-TYPE-BEZIER
            12 ; count control points, 2*2
            (vector 1 2 3 4 5 6 7 8 9 10 11 12)
            FALSE)) ; not closed

; path now has one stroke
(assert `(= (car (gimp-vectors-get-strokes ,testPath))
            1))



; capture stroke ID
(define testStroke (vector-ref (cadr (gimp-vectors-get-strokes testPath)) 0))

; stroke has correct count of control points
(assert `(gimp-vectors-stroke-get-points
            ,testPath
            ,testStroke))
; returns (type count (x y ...) isClosed)



; operations on stroke

; A Bézier path can be either open or closed.
; An open path has two separate end points.
; A closed path has end points coincide.
; Each Bézier point can have up to two handles,
; the length and direction of which determine the curve of the segments.
; Their end points are called handle points.

; Closing a path does NOT add a new stroke after the given stroke,
; that connects the end of the given stroke
; to the beginning of the first stroke in the path.
; Instead, closing a stroke changes the coords of the end point
; to coincide with the coords of the start point of the stroke.

; close a path does not throw
(assert `(gimp-vectors-stroke-close ,testPath ,testStroke))
; effective: path still has one stroke
(assert `(= (car (gimp-vectors-get-strokes ,testPath))
            1))
; effective: the flag on the stroke says it is closed
; the flag is the fourth element of the returned list.
(assert `(= (cadddr (gimp-vectors-stroke-get-points ,testPath ,testStroke))
            1))
; effective: the coords coincide
; TODO

; transformations
(displayln "stroke transformations")

(assert `(gimp-vectors-stroke-flip ,testPath ,testStroke
            ORIENTATION-HORIZONTAL
            0)) ; axis

(assert `(gimp-vectors-stroke-flip-free ,testPath ,testStroke
            0 0 ; x1,y1 of axis
            100 100 ; x2,y2 of axis
            ))

(assert `(gimp-vectors-stroke-reverse ,testPath ,testStroke))

(assert `(gimp-vectors-stroke-rotate ,testPath ,testStroke
              0 0 ; center x,y
              45)) ; angle

(assert `(gimp-vectors-stroke-scale ,testPath ,testStroke
              10 10)) ; x, y factors

(assert `(gimp-vectors-stroke-translate ,testPath ,testStroke
              10 10)) ; x, y offsets



; stroke attributes

(assert `(gimp-vectors-stroke-get-length ,testPath ,testStroke
            0 ;precision
            ))

(assert `(gimp-vectors-stroke-get-point-at-dist ,testPath ,testStroke
            0 ; along
            0 ; precision
            ))

(displayln "stroke interpolate")
; TODO check isClosed
; !!! yields vector 24k length
; This does NOT add a stroke, only yields data
(assert `(gimp-vectors-stroke-interpolate ,testPath ,testStroke
             0 ; precision
             ))



; add strokes
(displayln "add stroke")

; add a stroke to path by moveto does not throw
; TODO does this start at the end of the path?
(assert `(gimp-vectors-bezier-stroke-new-moveto ,testPath
            200 200 ; x, y of moveto point
          ))
; effective: path now has two stroke
(assert `(= (car (gimp-vectors-get-strokes ,testPath))
            2))

; FIXME: this crashes app when the stroke is closed
;(assert `(gimp-vectors-bezier-stroke-lineto ,testPath ,testStroke
;           256 256 ; x,y of lineto point
;         ))

; todo cubicto, conicto

; todo lineto when no previous moveto???

(assert `(gimp-vectors-bezier-stroke-new-ellipse ,testPath
            200 200 ; x, y of center
            20 20 ; radius in x, y direction
            200 ; angle of x axis in radians
          ))
; effective: path now has three stroke
(assert `(= (car (gimp-vectors-get-strokes ,testPath))
            3))



; remove stroke

(assert `(gimp-vectors-remove-stroke ,testPath ,testStroke))
; effective: path now has two stroke
(assert `(= (car (gimp-vectors-get-strokes ,testPath))
            2))

