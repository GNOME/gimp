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



(script-fu-use-v3)

(test! "refresh resources")

; always succeeds
; Returns #t
(assert `(gimp-brushes-refresh))
(assert `(gimp-dynamics-refresh))
(assert `(gimp-fonts-refresh))
(assert `(gimp-gradients-refresh))
(assert `(gimp-palettes-refresh))
(assert `(gimp-patterns-refresh))


(test!  "list resources")

; always succeeds
; Takes an optional regex string.
; Returns a vector of object ID's.
; !!! The name says its a list, but its an array
(assert `(vector? (gimp-brushes-get-list "")))
(assert `(vector? (gimp-fonts-get-list "")))
(assert `(vector? (gimp-gradients-get-list "")))
(assert `(vector? (gimp-palettes-get-list "")))
(assert `(vector? (gimp-patterns-get-list "")))


;            listing app's collection of things not resources
; But taking a regex

; Returns list of names, not a vector of object ID's
(assert `(list? (gimp-dynamics-get-name-list "")))
(assert `(list? (gimp-buffers-get-name-list "")))


;           listing app's other collections not resources
; Not taking a regex

; FIXME the naming does not follow the pattern, should be plural parasites
; Not: (gimp-parasites-get-list "")
(assert `(list? (gimp-get-parasite-list)))

; the app, images, vectors, drawables, items
; can all have parasites.
; Tested elsewhere.

(test! "get images")

; gimp-get-images does not follow the pattern:
; it doesn't take a regex
; and it returns a vector of image objects #()
(assert `(vector? (gimp-get-images)))


(script-fu-use-v2)