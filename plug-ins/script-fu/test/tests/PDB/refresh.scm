; test refresh methods

; make the app read resources from configuration files

; methods of the app
; the app manages collections of resources
; app can refresh and list the resources.

; A collection is named by the plural of the singular element,
; i.e. brushes is a collection of brush.




; Deprecations:
; gimp-palette-refresh
; gimp-brushes-list => gimp-brushes-get-list etc.
; gimp-parasite-list => gimp-get-parasite-list


;                      refresh

; always succeeds
; FIXME but wraps result in list (#t)
(assert `(car (gimp-brushes-refresh)))
(assert `(car (gimp-dynamics-refresh)))
(assert `(car (gimp-fonts-refresh)))
(assert `(car (gimp-gradients-refresh)))
(assert `(car (gimp-palettes-refresh)))
(assert `(car (gimp-patterns-refresh)))


;                      list

; always succeeds
; take an optional regex string
(assert `(list? (car (gimp-brushes-get-list ""))))
(assert `(list? (car (gimp-dynamics-get-list ""))))
(assert `(list? (car (gimp-fonts-get-list ""))))
(assert `(list? (car (gimp-gradients-get-list ""))))
(assert `(list? (car (gimp-palettes-get-list ""))))
(assert `(list? (car (gimp-patterns-get-list ""))))


;            listing app's collection of things not resources
; But taking a regex

(assert `(list? (car (gimp-buffers-get-list ""))))


;           listing app's other collections not resources
; Not taking a regex

; FIXME the naming does not follow the pattern, should be plural parasites
; Not: (gimp-parasites-get-list "")
(assert `(list? (car (gimp-get-parasite-list))))

; the app, images, vectors, drawables, items
; can all have parasites.
; Tested elsewhere.


; gimp-get-images does not follow the pattern:
; it doesn't take a regex
; and it returns a vector of image objects (0 #())
(assert `(vector? (cadr (gimp-get-images))))



