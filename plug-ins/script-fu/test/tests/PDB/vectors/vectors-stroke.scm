; Test stroke methods of Path class of the PDB

; in the console, this typically works to debug:
; i.e. these are the typical IDs
; (gimp-image-get-paths 1) => (1 #(3))
; (gimp-path-get-strokes 3) => (1 #(2))
;  (gimp-path-stroke-get-points 3 2) => (0 6 #(200.0 200.0 200.0 200.0 200.0 200.0) 0)

; setup

(define testImage (testing:load-test-image "gimp-logo.png"))
(define testLayer (vector-ref (cadr (gimp-image-get-layers testImage ))
                                  0))

(define testPath (car (gimp-path-new
                        testImage
                        "Test Path")))
(gimp-image-insert-path
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
(assert `(gimp-path-stroke-new-from-points
            ,testPath
            VECTORS-STROKE-TYPE-BEZIER
            12 ; count control points, 2*2
            (vector 1 2 3 4 5 6 7 8 9 10 11 12)
            FALSE)) ; not closed

; path now has one stroke
(assert `(= (car (gimp-path-get-strokes ,testPath))
            1))



; capture stroke ID
(define testStroke (vector-ref (cadr (gimp-path-get-strokes testPath)) 0))

; stroke has correct count of control points
(assert `(gimp-path-stroke-get-points
            ,testPath
            ,testStroke))
; TODO check the returned values
; returns (type count (x y ...) isClosed)



(test! "close operation on stroke")

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
(assert `(gimp-path-stroke-close ,testPath ,testStroke))
; effective: path still has one stroke
(assert `(= (car (gimp-path-get-strokes ,testPath))
            1))
; effective: the flag on the stroke says it is closed
; the flag is the fourth element of the returned list.
(assert `(= (cadddr (gimp-path-stroke-get-points ,testPath ,testStroke))
            1))
; effective: the coords coincide
; TODO


(test! "stroke transformations")

(assert `(gimp-path-stroke-flip ,testPath ,testStroke
            ORIENTATION-HORIZONTAL
            0)) ; axis

(assert `(gimp-path-stroke-flip-free ,testPath ,testStroke
            0 0 ; x1,y1 of axis
            100 100 ; x2,y2 of axis
            ))

(assert `(gimp-path-stroke-reverse ,testPath ,testStroke))

(assert `(gimp-path-stroke-rotate ,testPath ,testStroke
              0 0 ; center x,y
              45)) ; angle

(assert `(gimp-path-stroke-scale ,testPath ,testStroke
              10 10)) ; x, y factors

(assert `(gimp-path-stroke-translate ,testPath ,testStroke
              10 10)) ; x, y offsets



(test! "stroke attributes")

(assert `(gimp-path-stroke-get-length ,testPath ,testStroke
            0 ;precision
            ))

(assert `(gimp-path-stroke-get-point-at-dist ,testPath ,testStroke
            0 ; along
            0 ; precision
            ))

(test! "stroke interpolate")
; TODO check isClosed
; !!! yields vector 24k length
; This does NOT add a stroke, only yields data
(assert `(gimp-path-stroke-interpolate ,testPath ,testStroke
             0 ; precision
             ))



(test! "add stroke")

; add a stroke to path by moveto does not throw
; TODO does this start at the end of the path?
(assert `(gimp-path-bezier-stroke-new-moveto ,testPath
            200 200 ; x, y of moveto point
          ))
; effective: path now has two stroke
(assert `(= (car (gimp-path-get-strokes ,testPath))
            2))

; FIXME: this crashes app when the stroke is closed
;(assert `(gimp-path-bezier-stroke-lineto ,testPath ,testStroke
;           256 256 ; x,y of lineto point
;         ))


; TODO error case: lineto when no previous moveto???

(assert `(gimp-path-bezier-stroke-new-ellipse ,testPath
            200 200 ; x, y of center
            20 20 ; radius in x, y direction
            200 ; angle of x axis in radians
          ))
; effective: path now has three stroke
(assert `(= (car (gimp-path-get-strokes ,testPath))
            3))

(test! "add strokeconicto")
; simple test, I don't really know what the effect is.
; Extending a stroke, or adding another stroke?
; Should it fail if the given stroke is not a bezier?
; Isn't every stroke a bezier?
(assert `(gimp-path-bezier-stroke-conicto
            ,testPath
            ,testStroke
            200 200 ; x, y of control point
            20 20   ; x, y of end point
          ))
; effective: path now has three stroke
(assert `(= (car (gimp-path-get-strokes ,testPath))
            3))

(test! "add strokecubicto")
; simple test, I don't really know what the effect is.
; See above, conicto.
; Can you extend the same stroke twice?
(assert `(gimp-path-bezier-stroke-cubicto
            ,testPath
            ,testStroke
            200 200 ; x, y of control point
            20 20   ; x, y of end point
          ))
; effective: path now has three stroke
(assert `(= (car (gimp-path-get-strokes ,testPath))
            3))



(test! "remove stroke")

(assert `(gimp-path-remove-stroke ,testPath ,testStroke))
; effective: path now has two stroke
(assert `(= (car (gimp-path-get-strokes ,testPath))
            2))

