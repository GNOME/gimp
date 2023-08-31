; Test methods of Brush subclass of Resource class

; !!! See also resource.scm
; Currently using string names instead of numeric ID

; !!! Testing depends on a fresh install of GIMP.
; A prior testing failure may leave brushes in GIMP.
; The existing brush may have the same name as hard coded in tests.
; In future, will be possible to create new brush with same name as existing.


;      new and delete

; new returns brush of given name
(assert '(string=?
            (car (gimp-brush-new "TestBrushNew"))
            "TestBrushNew"))

; TODO delete
; can delete a new brush
; FAIL _gimp_gp_param_def_to_param_spec: GParamSpec type unsupported 'GimpParamResource'
;(assert '(string=?
;            (car (gimp-resource-delete "TestBrushNew"))
;            "TestBrushNew"))


;       Kind generated vesus raster

; new brush is kind generated
(assert '(equal?
            (car (gimp-brush-is-generated "TestBrushNew"))
            1))

; angle     default is 0
(assert '(=
            (car (gimp-brush-get-angle "TestBrushNew"))
            0))

; aspect-ratio   default  is 1.0
; FIXME: the doc says 0.0
(assert '(=
            (car (gimp-brush-get-aspect-ratio "TestBrushNew"))
            1.0))

; hardness   default is 0.5
; FIXME: the doc says 0
(assert '(=
            (car (gimp-brush-get-hardness "TestBrushNew"))
            0.5))

; shape default is GENERATED-CIRCLE
(assert '(=
            (car (gimp-brush-get-shape "TestBrushNew"))
            BRUSH-GENERATED-CIRCLE))

; spikes default is 2
; FIXME: docs says 0
(assert '(=
            (car (gimp-brush-get-spikes "TestBrushNew"))
            2))

; get-radius    default 5.0
; FIXME: docs says 0
(assert '(=
            (car (gimp-brush-get-radius "TestBrushNew"))
            5.0))


; spacing default 20
; FIXME: docs says 0
(assert '(=
            (car (gimp-brush-get-spacing "TestBrushNew"))
            20))

; get-info returns a list of attributes
; For generated, color bytes is zero
(assert '(equal? (gimp-brush-get-info "TestBrushNew")
                '(11 11 1 0)))

; get-pixels returns a list of attributes
; It is is long so we don't compare.
; This test is just that it doesn't crash or return #f.
(assert '(gimp-brush-get-pixels "TestBrushNew"))





;       Kind non-generated brush

; "z Pepper" is non-generated and is a system brush always installed

;       Certain attributes of non-generated brush yield errors
; angle, aspect-ratio, hardness, shape, spikes, radius

; angle
(assert-error
  '(gimp-brush-get-angle "z Pepper")
  "Procedure execution of gimp-brush-get-angle failed")

; TODO all the other attributes


;        Non-generated brush attributes

; is not generated
(assert '(=
            (car (gimp-brush-is-generated "z Pepper"))
            0))

; spacing
(assert '(=
            (car (gimp-brush-get-spacing "z Pepper"))
            100))

; pixels returns a list of attributes
; FAIL: CRASH Inconsistency detected by ld.so: dl-runtime.c: 63: _dl_fixup: Assertion `ELFW(R_TYPE)(reloc->r_info) == ELF_MACHINE_JMP_SLOT' failed!
; (assert '(gimp-brush-get-pixels "z Pepper"))

; get-info returns a list of attributes
(assert '(equal? (gimp-brush-get-info "z Pepper")
                '(180 220 1 3)))








