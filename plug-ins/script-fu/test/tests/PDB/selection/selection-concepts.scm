; Test concepts/model of Selection and SelectionMask
;
; Explains methods, coordinate systems, rects/bounds


; Disambiguate word "selection"

; A user of the GUI can "select" an area using a selection Tool.
; The act with a tool and the area the tool shows might be selected
; is distinct from the actual Selection object.
; Also, the methods of the API are similar but distinct from user actions.


; Disambiguate Image and Canvas
;
; The Image is not the set of pixels in all drawables;
; some drawables can be off the Canvas.
; The Canvas is the rectangle that composes and is seen.
; The size of Image is the size of the Canvas, not the size that encloses all drawables.

; Selection
;
; Is a set of pixels.
; The set can be empty, aka Selection is None.
;
; The pixels in the set are a subset of the pixels covered by SelectionMask:
; the pixels in SelectionMask that are non-zero.
;
; Methods to change the Selection usually change the SelectionMask.
; Some methods change the Selection by shape.
; e.g. gimp-image-select-rectangle see GISR
; Some methods make the Selection None and All.


; SelectionMask
;
; Is-a Channel, which is-a Drawable.
; A Drawable is rectangular.
;
; Each image has its own SelectionMask.
; The SelectionMask always exists for an image.
; The coordinates of the SelectionMask are in the Image coordinate system.
; There are no methods for offsetting a channel (setting its UL origin).
; The SelectionMask is the always same size as the Image.
;
; The depth of the SelectionMask is the same as other channels (RGB)
; For example, when the image is 16-bit, the SelectionMask is 16-bit.



; gimp-image-get-selection
;
; GIGS-1 gimp-image-get-selection returns a Channel
; GIGS-2 the SelectionMask is same size as Image, when new image
; GIGS-3 the SelectionMask does not cover parts of layers outside the image
; GIGS-4 the SelectionMask grows to size of a resized enlarged image
; GIGS-5 gimp-item-id-is-selection is True for the SelectionMask

; gimp-image-select-rectangle

; GISR-1 selecting a shape outside the image changes the Selection to None
; GISR-2 selecting a shape outside the image does not change the size of SelectionMask


; gimp-drawable-mask-intersect
;
; Computes the intersection of the SelectionMask drawable with the given drawable.
; The returned bounds are the intersection OR the bounds of the drawable.
; The returned boolean with strange meaning...
; GDMI-1 The boolean is TRUE when Selection is not None and it intersects the drawable.
; GDMI-2 The boolean is TRUE when Selection is None.
; GDMI-3 The boolean is FALSE when Selection is not None and it does not intersect drawable.
; GDMI-4 When Selection is None, bounds of intersection is size of layer,
;   but all corresponding values of the SelectionMask are zero.
; GDMI-5 When Selection is not None, but not covers layer, bounds of intersection is size of intersection.
; GDMI-6 When Selection is not None, but covers layer, bounds of intersection is size of layer,
;    and corresponding values of the SelectionMask are non-zero.


; gimp-selection-value
;
; Returns the value of the SelectionMask at the given coordinates.
;
; GSV-1 When the coordinates are outside the bounds of the image (canvas)
;   the value is zero.


; gimp-drawable-mask-bounds
;
; GDMB
; NOT TESTED, generally low usefulness


; gimp-selection-bounds
;
; Is a method on Selection
;
; Returns:
;    Boolean whether the SelectionMask has any non-zero values, i.e. is-a Selection
;    UL and LR coordinates of a bounding rect of the Selection, in image coordinates,
;       OR the bounding rect of the Image !!!
;
; GSB-1 The bounds of the Selection on a new image without layers is the size of the image.
; GSB-2 A new image without layers has no selection, returned boolean is False.

; The rect need not cover the origin (of the image i.e. canvas).
;
; When the boolean is True, the rect covers all selected pixels inside the image i.e. canvas.
; That rect in the SelectionMask has some values not zero (somewhat selected).
; That rect in the SelectionMask may have some values of 0 (not selected).
; The rect is tight around selected pixels.
; It has no slices that are entirely zero (not selected.)

; When the boolean is False, the rect has no meaning.
; The Rect covers the Image (Canvas.)
; When the boolean is False, gimp-selection-is-empty returns True.


; gimp-drawable-get-offsets
;
; GDGO-1 a drawable can be created with negative offsets from image
;      It can be totally off the image (canvas)




(script-fu-use-v3)


; Testing methods.
; More abstract than Gimp calls.

; assert the SelectionMask is given width
; Requires testSelectionMask is defined globally
(define (testSelectionMaskWidth width)
  (assert (equal? (gimp-drawable-get-width testSelectionMask)
                  width)))

; Assert the Selection is empty.
; Requires testImage is defined globally.
; This tests that selection-is-empty if and only if the boolean
; result of gimp-selection-bounds is #f.
; !!! The bounds returned by gimp-selection-bounds are another matter
; and may be non-zero when Selection is empty.
(define (testSelectionIsEmpty)
   (assert `(gimp-selection-is-empty ,testImage))
   (assert `(not (car (gimp-selection-bounds ,testImage)))))

; Assert the bounds of the Selection
; Requires testImage is defined globally.
; Abstracts away that the bounds are a separate return value of gimp-selection-bounds.
; bounds is-a list of four numbers.
; FIXME this throws "Unbound variable" for the backtick `, so we use explicit quasiquote.
(define (testSelectionBounds bounds)
  (assert (quasiquote (equal? (cdr (gimp-selection-bounds ,testImage))
                   bounds))))


; Image has-as CoordinateSystem
; Origin of the CoordinateSystem is not exposed in the API

(define testImage (gimp-image-new 11 12 RGB))

; New image has the width given at creation.
; The image has no layers; this is independent of layers.
(assert `(= (gimp-image-get-width ,testImage)
            11))


(test! "SelectionMask on new image")

; New image has a SelectionMask.
; Note the API calls it a "selection" !!!
(define testSelectionMask (gimp-image-get-selection testImage))

(test! "GIGS-1 gimp-image-get-selection returns a Channel")
(assert `(gimp-item-id-is-channel ,testSelectionMask))
(test! "GIGS-5 gimp-item-id-is-selection is True for the SelectionMask")
(assert `(gimp-item-id-is-selection ,testSelectionMask))

; But a new image has no layers (not tested.)

; A new image has a SelectionMask but no Selection.
; These are distinct: the Selection is created by a user or script separately.

; In v3 binding is a boolean.
; (In v2 binding, is numeric 0 or 1)
(test! "GSB-2 A new image without layers has no selection, returned boolean is False.")
(assert `(not (car (gimp-selection-bounds ,testImage))))

(test! "GSB-1 Bounds of selection are the image size, when no selection.")
; Cdr (the rest of) gimp-selection-bounds are four values of a Bounds tuple.
; A SelectionMask has the same coordinate system as the Image (same origin.)
; SelectionMask for new image is same size (bounds) as Image.
; It is NOT zero width.
(assert `(equal? (cdr (gimp-selection-bounds ,testImage))
                 '(0 0 11 12)))
;(testSelectionBounds (list 0 0 11 12))

(test! "GIGS-2 the SelectionMask is same size as Image, when new image")
; The offsets are 0,0
(assert (equal? (gimp-drawable-get-offsets testSelectionMask)
                '(0 0)))
; and the width is same as image width
(testSelectionMaskWidth 11)



(test! "selection-none")
(assert `(gimp-selection-none ,testImage))

(testSelectionIsEmpty)

; but does not change the bounds of the Selection
(assert `(equal? (cdr (gimp-selection-bounds ,testImage))
                 '(0 0 11 12)))



(test! "select-all")
(assert `(gimp-selection-all ,testImage))

; after select-all, selection-bounds indicates selection created
(assert `(car (gimp-selection-bounds ,testImage)))
; Remember, the image has no layers, so the selection-bounds are nearly useless
; for operations.

; and the bounds are same as image
(assert `(equal? (cdr (gimp-selection-bounds ,testImage))
                 '(0 0 11 12)))

; and now is-empty is false
(assert `(not (gimp-selection-is-empty ,testImage)))
; but the "selection" is meaningless, no drawing op will have effect
; since there are no layers in the image.


; State: image has no layers, and Selection==All

(test! "select rect outside canvas to upper left, on image with no layers")
; This rect is outside the canvas
(gimp-image-select-rectangle testImage CHANNEL-OP-REPLACE -1 -1 1 1)

; Since outside the canvas, this does not change the SelectionMask size
(test! "GISR-2 selecting a shape outside the image does not change the size of SelectionMask")
(testSelectionMaskWidth 11)

; But it does replace the SelectionMask values and makes the Selection empty
(test! "GISR-1 selecting a shape outside the image changes the Selection to None")
(testSelectionIsEmpty)
; Since Selection is empty, bounds of Selection are same (but meaningless)


(test! "Add layer larger than canvas")
; Adding a layer larger than the current canvas
; does not change the size of the Image
; nor change the Selection
; nor change the SelectionMask
(define testLayer1 (testing:layer-new-inserted testImage))
; The layer is 21,22 and is inserted in image

; The image size has not grown.
(assert `(= (gimp-image-get-width ,testImage)
            11))

; The Selection has not grown; it does not cover the layer.
(assert `(equal? (cdr (gimp-selection-bounds ,testImage))
                 '(0 0 11 12)))

(test! "GIGS-3 the SelectionMask does not cover parts of layers outside the image")
; The layer is width 21 but the SelectionMask is only 11
(testSelectionMaskWidth 11)


(testSelectionIsEmpty)

(assert `(gimp-selection-none ,testImage))
(test! "GDMI-2 When Selection is None, GDMI boolean is TRUE")
(assert `(car (gimp-drawable-mask-intersect ,testLayer1)))

(test! "GDMI-4 When Selection is None, intersection bounds are size of layer.")
; In coordinate system of layer.
(assert `(equal? (cdr (gimp-drawable-mask-intersect ,testLayer1))
                 '(0 0 21 22)))

(assert `(gimp-selection-all ,testImage))
; Selection is All, but the SelectionMask is size of canvas, not the larger layer.

; select all has NOT grown the Selection to size of layer;
; the Selection still only covers the canvas peephole.
(assert `(equal? (cdr (gimp-selection-bounds ,testImage))
                 '(0 0 11 12)))

(test! "GSV-1 value of the selection mask is 'not selected' i.e. 0 when out of bounds of image")
; in the LR quadrant of the layer, but outside the image (canvas.)
(assert `(= (gimp-selection-value ,testImage 11 12)
            0))

(test! "GDMI-1 When is a Selection and intersects layer, GDMI boolean is TRUE")
(assert `(car (gimp-drawable-mask-intersect ,testLayer1)))

(test! "GDMI-5 When Selection is not None, but not covers layer, bounds of intersection is size of intersection")
; In coordinate system of layer.
; !!! Layer is width 21 but SelectionMask is width 11
(assert `(equal? (cdr (gimp-drawable-mask-intersect ,testLayer1))
                 '(0 0 11 12)))


(test! "Resize image (canvas) to layers")
(assert `(gimp-image-resize-to-layers ,testImage))

; The Selection has NOT grown.
; The Selection is still coordinates of the former canvas size,
; which is smaller than the layer size.
(assert `(equal? (cdr (gimp-selection-bounds ,testImage))
                 '(0 0 11 12)))

(test! "GIGS-4 the SelectionMask grows to size of a resized enlarged image")
(testSelectionMaskWidth 21)



(test! "Select all on image with one layer.")
; select all succeeds
(assert `(gimp-selection-all ,testImage))

; The Selection has grown
(assert `(equal? (cdr (gimp-selection-bounds ,testImage))
                 '(0 0 21 22)))



(test! "Intersection of the Selection with the layer")
; Intersection is not empty (#t)
(assert `(car (gimp-drawable-mask-intersect ,testLayer1)))

(test! "GDMI-6 When Selection is not None, but covers layer, bounds of intersection is size of layer.")
; Intersection rect is entire layer
(assert `(equal? (cdr (gimp-drawable-mask-intersect ,testLayer1))
                 '(0 0 21 22)))


; the value of the selection mask is "entirely selected" i.e. 255
; in the LR quadrant of the layer, now inside the image (canvas.)
(assert `(= (gimp-selection-value ,testImage 11 12)
            255))






(test! "Add layer left and above the canvas")
; L2
;   +
;   | L1 |
(define testLayer2 (testing:layer-new-inserted testImage))
(test! "GDGO-1 a drawable can be created with negative offsets from image")
(gimp-layer-set-offsets testLayer2 -21 -22)

; The Selection has NOT grown
(assert `(equal? (cdr (gimp-selection-bounds ,testImage))
                 '(0 0 21 22)))
; Neither the SelectionMask nor the Image is grown to cover the layer off canvas.
; TODO

(test! "Resize image (canvas) to upper left, to TWO layers")
; +       |
; | L2    |
; |    L1 |
(assert `(gimp-image-resize-to-layers ,testImage))

(test! "The Selection HAS NOT grown")
; The origin of the Image has changed;
; and the UL LR of the selection bounds.
(assert `(equal? (cdr (gimp-selection-bounds ,testImage))
                '(21 22 42 44)))

(test! "The SelectionMask is larger")
; The SelectionMask extends from image origin to cover testLayer1,
; which is entirely selected.
(testSelectionMaskWidth 42)
;(assert (equal? (gimp-drawable-get-width testSelectionMask) 42))

(test! "The SelectionMask still intersects testLayer1")
; Intersection is not empty (#t)
(assert `(car (gimp-drawable-mask-intersect ,testLayer1)))

(test! "The intersection has same UL coordinates, in the frame of the Drawable")
; Intersection is width of layer
(assert `(equal? (cdr (gimp-drawable-mask-intersect ,testLayer1))
                 '(0 0 21 22)))
; Adding offsets of the layer to the intersection UL coordinates
; will give coordinates of the intersection UL in the image (canvas)

; Selection does not intersect Layer2.
; (testLayer1 is selected, but not testLayer2.  The SelectionMask covers both.)
(test! "GDMI-3 The boolean is FALSE when Selection is not None and it does not intersect drawable.")
(assert `(not (car (gimp-drawable-mask-intersect ,testLayer2))))


(test! "Select all of image with TWO layers")
(assert `(gimp-selection-all ,testImage))

(test! "The Selection has grown")
(assert `(equal? (cdr (gimp-selection-bounds ,testImage))
                '(0 0 42 44)))

(test! "The SelectionMask has not grown")
(assert `(equal? (cdr (gimp-selection-bounds ,testImage))
                '(0 0 42 44)))

(test! "The intersection with testLayer1 is the same")
; it is still in coordinates of the Drawable.
; testLayer1 UL is at 21,22 of the image and its dimensions are 21,22
(assert `(equal? (cdr (gimp-drawable-mask-intersect ,testLayer1))
                 '(0 0 21 22)))

(test! "The intersection with testLayer2 also is in coordinates of the Drawable")
(assert `(equal? (cdr (gimp-drawable-mask-intersect ,testLayer2))
                 '(0 0 21 22)))



(test! "Select a rect outside the image (canvas) to the upper left")
(gimp-image-select-rectangle testImage CHANNEL-OP-REPLACE -1 -1 1 1)

(test! "GISR-2 selecting a shape outside the image does not change size of SelectionMask")
(testSelectionMaskWidth 42)

(test! "GISR-1 selecting a shape outside the image changes the Selection to None")
; It sets the SelectionMask to all zeroes and makes the Selection empty.
; When a user drags out the same shape, the  shape is visible in the GUI,
; but it is not the selection ()
(testSelectionIsEmpty)

; Since Selection is None, intersection of SelectionMask with drawable is
; still the size of the drawables, in their coordinate systems.
(test! "GDMI-4 When Selection is None, bounds of intersection is size of layer")
(assert `(equal? (cdr (gimp-drawable-mask-intersect ,testLayer1))
                 '(0 0 21 22)))
(assert `(equal? (cdr (gimp-drawable-mask-intersect ,testLayer2))
                 '(0 0 21 22)))


; TODO test shrinking an image to size of a smaller layer.
; expect SelectionMask shrinks

; TODO test resizing image to cover layer to LR of image
; expect SelectionMask grows to image size

; TODO test bit-depth of selection mask
; TODO test valus of selection mask in range 0-255

(gimp-display-new testImage)