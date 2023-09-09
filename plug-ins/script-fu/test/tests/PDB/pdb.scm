; Complete test of PDB

; to run in SF Console:
; (testing:load-test "pdb.scm")
; Expect a report of passed and failed

; This knows the set of files which are tests.
; The test files might be organized in directories in the repo,
; but all flattened into the /tests directory when installed.

; images
(testing:load-test "image-new.scm")
(testing:load-test "image-precision.scm")
(testing:load-test "image-indexed.scm")
(testing:load-test "image-grayscale.scm")
(testing:load-test "image-ops.scm")
(testing:load-test "image-layers.scm")

(testing:load-test "layer-new.scm")
(testing:load-test "layer-ops.scm")
(testing:load-test "layer-mask.scm")
; TODO layer stack ops

; Commented out until PDB is fixed
; Known to crash GIMP
;(testing:load-test "text-layer-new.scm")

(testing:load-test "vectors-new.scm")
(testing:load-test "channel-new.scm")
; TODO channel-ops.scm

(testing:load-test "selection.scm")
(testing:load-test "selection-from.scm")

; Test superclass methods.
; Drawable and Item are superclasses
; Testing Drawable and Item uses extant instances;
; must be after instances of subclasses are created.
; commented out until text-get-fontname is fixed
; Known to crash GIMP
;(testing:load-test "item.scm")
; todo item ordering operations

; TODO drawable

; context
(testing:load-test "context-get-set.scm")

; Temporarily commented out until gimpgpparam-body.c is fixed for GimpParamResource
; If you uncomment it, see warnings in stderr
;(testing:load-test "context-resource.scm")

(testing:load-test "resource.scm")
(testing:load-test "brush.scm")
(testing:load-test "palette.scm")
; TODO other resources gradient, etc
(testing:load-test "resource-ops.scm")

(testing:load-test "buffer.scm")

; TODO edit ops
; TODO undo
; TODO progress

; tested in bind-args.scm:
;   unit
;   parasite

; pdb the object
; gimp the class, gimp-get, gimp-parasite


(testing:load-test "misc.scm")
(testing:load-test "enums.scm")
(testing:load-test "refresh.scm")
(testing:load-test "bind-args.scm")

; report the result
(testing:report)

; yield the session overall result
(testing:all-passed?)

