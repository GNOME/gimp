; Complete test of PDB

; to run in SF Console:
; (testing:load-test "pdb.scm")
; Expect a report of passed and failed

; This knows the set of files which are tests.
; The test files might be organized in directories in the repo,
; but all flattened into the /tests directory when installed.

; creates images 1-6
(testing:load-test "image-new.scm")
(testing:load-test "image-precision.scm")
(testing:load-test "image-indexed.scm")
(testing:load-test "image-grayscale.scm")
(testing:load-test "image-ops.scm")

(testing:load-test "layer-new.scm") ; image 7 and layer 8
(testing:load-test "layer-ops.scm") ; image 8 and layer 10
(testing:load-test "layer-mask.scm") ; image 9 and layer 12 and layerMask 13, 14
; TODO layer stack ops

(testing:load-test "text-layer-new.scm") ; image 8, layer 15, vectors 16

(testing:load-test "vectors-new.scm") ; image 10 and vectors 19

(testing:load-test "channel-new.scm")
; TODO channel-ops.scm

(testing:load-test "selection.scm")
(testing:load-test "selection-from.scm")

; Test superclass methods.
; Drawable and Item are superclasses
; Testing Drawable and Item uses extant instances;
; must be after instances of subclasses are created.
(testing:load-test "item.scm")
; todo item order

; TODO drawable

(testing:load-test "resource.scm")
(testing:load-test "brush.scm")
; TODO other resources gradient, etc

; TODO edit ops
; TODO undo
; TODO unit
; TODO progress
; pdb
; context
; gimp the class, gimp-get, gimp-parasite

; parasite is not a class, only methods of other classes

(testing:load-test "misc.scm")
(testing:load-test "enums.scm")

; report the result
(testing:report)

; yield the session result
(testing:all-passed?)

