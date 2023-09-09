; Test methods of palette subclass of Resource class

; !!! See also resource.scm

; !!! Testing depends on a fresh install of GIMP.
; A prior testing failure may leave palettees in GIMP.
; The existing palette may have the same name as hard coded in tests.
; In future, will be possible to create new palette with same name as existing.




; setup, not assert
; but tests the -new method
(define testNewPalette (car (gimp-palette-new "testNewPalette")))








;                 attributes of new palette


; gimp-palette-get-background deprecated => gimp-context-get-background
; ditto foreground

; new palette has given name
; !!! Fails if not a fresh install, then name is like "testNewPalette #2"
(assert `(string=?
            (car (gimp-resource-get-name ,testNewPalette))
            "testNewPalette"))

; new palette has zero colors
(assert `(= (car (gimp-palette-get-color-count ,testNewPalette))
            0))

; new palette has empty colormap
; (0 #())
(assert `(= (car (gimp-palette-get-colors ,testNewPalette))
            0))

; new palette has zero columns
; (0 #())
(assert `(= (car (gimp-palette-get-columns ,testNewPalette))
            0))

; new palette is-editable
; method on Resource class
(assert `(= (car (gimp-resource-is-editable ,testNewPalette))
            1))

; can set new palette in context
; Despite having empty colormap
(assert `(gimp-context-set-palette ,testNewPalette))



;             attributes of existing palette

; setup
(define testBearsPalette (car (gimp-palette-get-by-name "Bears")))


; Max size palette is 256

; Bears palette has 256 colors
(assert `(= (car (gimp-palette-get-color-count ,testBearsPalette))
            256))

; Bears palette colormap is size 256
; (256)
(assert `(= (car (gimp-palette-get-color-count ,testBearsPalette))
            256))

; Bears palette colormap array is size 256 vector of 3-tuple lists
; (256 #((8 8 8) ... ))
(assert `(= (vector-length (cadr (gimp-palette-get-colors ,testBearsPalette)))
            256))

; Bears palette has zero columns
; (0 #())
(assert `(= (car (gimp-palette-get-columns ,testBearsPalette))
            0))

; system palette is not editable
(assert `(= (car (gimp-resource-is-editable ,testBearsPalette))
            0))



;              setting attributes of existing palette

; Can not change column count on system palette
(assert-error `(gimp-palette-set-columns ,testBearsPalette 1)
              "Procedure execution of gimp-palette-set-columns failed")


;              add entry to full system palette

; error to add entry to palette which is non-editable and has full colormap
(assert-error `(gimp-palette-add-entry ,testBearsPalette "fooEntryName" "red")
                "Procedure execution of gimp-palette-add-entry failed ")



 ;             setting attributes of new palette

; succeeds
(assert `(gimp-palette-set-columns ,testNewPalette 1))

; effective
(assert `(= (car (gimp-palette-get-columns ,testNewPalette))
            1))


;               adding color "entry" to new palette

; add first entry returns index 0
; result is wrapped (0)
(assert `(= (car (gimp-palette-add-entry ,testNewPalette "fooEntryName" "red"))
            0))

; was effective on color
; FIXME returns ((0 0 0)) which is not "red"
(assert `(equal? (car (gimp-palette-entry-get-color ,testNewPalette 0))
                  (list 0 0 0)))

; was effective on name
(assert `(equal? (car (gimp-palette-entry-get-name ,testNewPalette 0))
                  "fooEntryName"))



;                delete colormap entry

; succeeds
; FIXME: the name seems backward, could be entry-delete
(assert `(gimp-palette-delete-entry  ,testNewPalette 0))
; effective, color count is back to 0
(assert `(= (car (gimp-palette-get-color-count ,testNewPalette))
            0))


;               adding color "entry" to new palette which is full


; TODO locked palette?  See issue about locking palette?





;            delete palette

; can delete a new palette
(assert `(gimp-resource-delete ,testNewPalette))

; delete was effective
; ID is now invalid
(assert `(= (car(gimp-resource-id-is-palette ,testNewPalette))
            0))

; delete was effective
; not findable by name anymore
; If the name DOES exist (because not started fresh) yields "substring out of bounds"
(assert-error `(gimp-palette-get-by-name "testNewPalette")
              "Procedure execution of gimp-palette-get-by-name failed on invalid input arguments: Palette 'testNewPalette' not found")




; see context.scm




;                   test deprecated methods

; These should give warnings in Gimp Error Console.
; Now they are methods on Context, not Palette.

(gimp-palettes-set-palette testBearsPalette)

(gimp-palette-swap-colors)
(gimp-palette-set-foreground "pink")
(gimp-palette-set-background "purple")












