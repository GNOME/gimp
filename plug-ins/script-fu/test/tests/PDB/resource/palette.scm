; Test methods of palette subclass of Resource class

; !!! See also resource.scm
; Currently using string names instead of numeric ID

; !!! Testing depends on a fresh install of GIMP.
; A prior testing failure may leave palettees in GIMP.
; The existing palette may have the same name as hard coded in tests.
; In future, will be possible to create new palette with same name as existing.


;      new and delete

; new returns palette of given name
(assert '(string=?
            (car (gimp-palette-new "testPaletteNew"))
            "testPaletteNew"))

; TODO delete
; can delete a new palette
; FAIL _gimp_gp_param_def_to_param_spec: GParamSpec type unsupported 'GimpParamResource'
;(assert '(string=?
;            (car (gimp-resource-delete "testPaletteNew"))
;            "testPaletteNew"))



;                 attributes of new palette


; gimp-palette-get-background deprecated => gimp-context-get-background
; ditto foreground

; new palette has zero colors
(assert '(= (car (gimp-palette-get-color-count "testPaletteNew"))
            0))

; new palette has empty colormap
; (0 #())
(assert '(= (car (gimp-palette-get-colors "testPaletteNew"))
            0))

; new palette has zero columns
; (0 #())
(assert '(= (car (gimp-palette-get-columns "testPaletteNew"))
            0))

; TODO is editable resource-is-editable



;             attributes of existing palette

; Max size palette 256

; Bears palette has 256 colors
(assert '(= (car (gimp-palette-get-color-count "Bears"))
            256))

; Bears palette colormap is size 256
; (256)
(assert '(= (car (gimp-palette-get-color-count "Bears"))
            256))

; Bears palette colormap array is size 256 vector of 3-tuple lists
; (256 #((8 8 8) ... ))
(assert '(= (vector-length (cadr (gimp-palette-get-colors "Bears")))
            256))

; Bears palette has zero columns
; (0 #())
(assert (= (car (gimp-palette-get-columns "Bears"))
            0))

; TODO is not editable resource-is-editable



;              setting attributes of existing palette

; Can not change column count on system palette
(assert-error `(gimp-palette-set-columns "Bears" 1)
              "Procedure execution of gimp-palette-set-columns failed")


 ;             setting attributes of new palette

; succeeds
(assert `(gimp-palette-set-columns "testPaletteNew" 1))

; effective
(assert `(= (car (gimp-palette-get-columns "testPaletteNew"))
            1))


;               adding color "entry" to new palette

; add first entry returns index 0
; result is wrapped (0)
(assert `(= (car (gimp-palette-add-entry "testPaletteNew" "fooEntryName" "red"))
            0))

; was effective on color
; FIXME returns ((0 0 0)) which is not "red"
(assert `(equal? (car (gimp-palette-entry-get-color "testPaletteNew" 0))
                  (list 0 0 0)))

; was effective on name
(assert `(equal? (car (gimp-palette-entry-get-name "testPaletteNew" 0))
                  "fooEntryName"))



;             delete entry

; succeeds
; FIXME: the name seems backward, could be entry-delete
(assert `(gimp-palette-delete-entry  "testPaletteNew" 0))
; effective, color count is back to 0
(assert '(= (car (gimp-palette-get-color-count "testPaletteNew"))
            0))


;               adding color "entry" to new palette which is full
;               adding color "entry" to system palette
; TODO

; TODO locked palette?  See issue about locking palette?




; see context.scm

; same as context-set-palette ?
;gimp-palettes-set-palette deprecated











