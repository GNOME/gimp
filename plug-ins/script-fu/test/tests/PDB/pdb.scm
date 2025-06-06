; Complete test of PDB

; to run in SF Console:
; (testing:load-test "pdb.scm")
; Expect a report of passed and failed

; This knows the set of files which are tests.
; The test files might be organized in directories in the repo,
; but all flattened into the /tests directory when installed.

(testing:load-test "image-new.scm")
(testing:load-test "image-precision.scm")
(testing:load-test "image-indexed.scm")
(testing:load-test "image-grayscale.scm")
(testing:load-test "image-ops.scm")
(testing:load-test "image-layers.scm")
(testing:load-test "image-set-selected.scm")
(testing:load-test "image-mode.scm")
(testing:load-test "image-color-profile.scm")
(testing:load-test "image-sample-points.scm")

(testing:load-test "paint.scm")
(testing:load-test "paint-methods.scm")
(testing:load-test "dynamics.scm")

(testing:load-test "layer-new.scm")
(testing:load-test "layer-ops.scm")
(testing:load-test "layer-mask.scm")
; TODO layer stack ops, moving up and down?

; TODO broken until ScriptFu marshalls GimpUnit
; and it crosses the wire?
; (testing:load-test "text-layer-new.scm")

(testing:load-test "vectors-new.scm")
(testing:load-test "vectors-stroke.scm")

(testing:load-test "selection.scm")
(testing:load-test "selection-from.scm")
(testing:load-test "selection-by.scm")
(testing:load-test "selection-by-shape.scm")
; TODO test floating-sel- methods

(testing:load-test "channel-new.scm")
(testing:load-test "channel-attributes.scm")
(testing:load-test "channel-ops.scm")
; TODO channels to selection

(testing:load-test "color.scm")

; Test superclass methods.
; Drawable and Item are superclasses

; Testing Drawable and Item uses extant instances;
; must be after instances of subclasses are created.
(testing:load-test "item.scm")
(testing:load-test "item-position.scm")
(testing:load-test "item-layer-group.scm")
(testing:load-test "item-layer-group-2.scm")

(testing:load-test "drawable.scm")
(testing:load-test "drawable-attributes.scm")
(testing:load-test "drawable-ops.scm")

; context
(testing:load-test "context-get-set.scm")
(testing:load-test "context-resource.scm")
(testing:load-test "context-stack.scm")

(testing:load-test "resource.scm")
(testing:load-test "brush.scm")
(testing:load-test "palette.scm")
; TODO other resources gradient, etc
(testing:load-test "resource-ops.scm")

; clipboards
; Order important: edit.scm requires empty clipboard

(testing:load-test "edit.scm")
(testing:load-test "edit-multi-layer.scm")
(testing:load-test "buffer.scm")
(testing:load-test "edit-cut.scm")

(testing:load-test "file.scm")

; gimp module, gimp-get methods

; Since 3.0rc2 private to libgimp
; gimp PDB as a queryable store i.e. database
; (testing:load-test "PDB.scm")
; test methods on PDBProcedure
;(testing:load-test "procedures.scm")

; test gimp as a refreshable set of installed resources
(testing:load-test "refresh.scm")

; test methods on DrawableFilter
(testing:load-test "filter.scm")
(testing:load-test "filter-ops.scm")

; Only run when not headless i.e.
; when testing is interactive using SF Console
(testing:load-test "display.scm")

; TODO undo
; TODO progress

; tested in bind-args.scm:
;   unit
;   parasite

; gimp-parasite

; uncategorized tests
(testing:load-test "misc.scm")
(testing:load-test "enums.scm")
(testing:load-test "bind-args.scm")
(testing:load-test "pixel.scm")
(testing:load-test "named-args.scm")

; tested last, random and time-consuming
(testing:load-test "file-export.scm")

; Don't routinely test the PDB API for NDE filters, it is long:
; (testing:load-test "gegl.scm")

; report the result
(testing:report)

; yield the session overall result
(testing:all-passed?)

