; GIMP - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; gimp-online.scm
; Copyright (C) 2003  Henrik Brix Andersen <brix@gimp.org>
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.

(define (gimp-online-docs-web-site)
  (plug-in-web-browser #:url "https://docs.gimp.org/")
)

(define (gimp-help-main)
  (gimp-help "" "gimp-main")
)

(define (gimp-help-concepts-usage)
  (gimp-help "" "gimp-concepts-usage")
)

(define (gimp-help-using-docks)
  (gimp-help "" "gimp-concepts-docks")
)

(define (gimp-help-using-simpleobjects)
  (gimp-help "" "gimp-using-simpleobjects")
)

(define (gimp-help-using-selections)
  (gimp-help "" "gimp-using-selections")
)

(define (gimp-help-using-fileformats)
  (gimp-help "" "gimp-using-fileformats")
)

(define (gimp-help-using-photography)
  (gimp-help "" "gimp-using-photography")
)

(define (gimp-help-using-web)
  (gimp-help "" "gimp-using-web")
)

(define (gimp-help-concepts-paths)
  (gimp-help "" "gimp-concepts-paths")
)


; shortcuts to help topics
(script-fu-register-procedure "gimp-help-concepts-paths"
   _"_Using Paths"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@gimp.org>"
    "2006"
)

(script-fu-menu-register "gimp-help-concepts-paths"
			 "<Image>/Help/User Manual")


(script-fu-register-procedure "gimp-help-using-web"
   _"_Preparing your Images for the Web"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@gimp.org>"
    "2006"
)

(script-fu-menu-register "gimp-help-using-web"
			 "<Image>/Help/User Manual")


(script-fu-register-procedure "gimp-help-using-photography"
   _"_Working with Digital Camera Photos"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@gimp.org>"
    "2006"
)

(script-fu-menu-register "gimp-help-using-photography"
			 "<Image>/Help/User Manual")


(script-fu-register-procedure "gimp-help-using-fileformats"
   _"Create, Open and Save _Files"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@gimp.org>"
    "2006"
)

(script-fu-menu-register "gimp-help-using-fileformats"
			 "<Image>/Help/User Manual")


(script-fu-register-procedure "gimp-help-concepts-usage"
   _"_Basic Concepts"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@gimp.org>"
    "2006"
)

(script-fu-menu-register "gimp-help-concepts-usage"
			 "<Image>/Help/User Manual")


(script-fu-register-procedure "gimp-help-using-docks"
   _"How to Use _Dialogs"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@gimp.org>"
    "2006"
)

(script-fu-menu-register "gimp-help-using-docks"
			 "<Image>/Help/User Manual")


(script-fu-register-procedure "gimp-help-using-simpleobjects"
   _"Drawing _Simple Objects"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@gimp.org>"
    "2006"
)

(script-fu-menu-register "gimp-help-using-simpleobjects"
			 "<Image>/Help/User Manual")


(script-fu-register-procedure "gimp-help-using-selections"
   _"_Create and Use Selections"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@gimp.org>"
    "2006"
)

(script-fu-menu-register "gimp-help-using-selections"
			 "<Image>/Help/User Manual")

(script-fu-register-procedure "gimp-help-main"
   _"_Table of Contents"
   _"Bookmark to the user manual"
    "Alx Sa"
    "2023"
)

(script-fu-menu-register "gimp-help-main"
			 "<Image>/Help/User Manual/[Table of Contents]")


;; Links to GIMP related web sites

(define (gimp-online-main-web-site)
  (plug-in-web-browser #:url "https://www.gimp.org/")
)

(define (gimp-online-developer-web-site)
  (plug-in-web-browser #:url "https://developer.gimp.org/")
)

(define (gimp-online-roadmap)
  (plug-in-web-browser #:url "https://developer.gimp.org/core/roadmap/")
)

(define (gimp-online-bugs-features)
  (plug-in-web-browser #:url "https://gitlab.gnome.org/GNOME/gimp/issues")
)

; (define (gimp-online-plug-in-web-site)
;   (plug-in-web-browser #:url "https://registry.gimp.org/")
; )


(script-fu-register-procedure "gimp-online-main-web-site"
   _"_Main Web Site"
   _"Bookmark to the GIMP web site"
    "Henrik Brix Andersen <brix@gimp.org>"
    "2003"
)

(script-fu-menu-register "gimp-online-main-web-site"
                         "<Image>/Help/GIMP Online")


(script-fu-register-procedure "gimp-online-developer-web-site"
   _"_Developer Web Site"
   _"Bookmark to the GIMP web site"
    "Henrik Brix Andersen <brix@gimp.org>"
    "2003"
)

(script-fu-menu-register "gimp-online-developer-web-site"
                         "<Image>/Help/GIMP Online")


(script-fu-register-procedure "gimp-online-roadmap"
   _"_Roadmaps"
   _"Bookmark to the roadmaps of GIMP"
    "Alexandre Prokoudine <alexandre.prokoudine@gmail.com>"
    "2018"
)

(script-fu-menu-register "gimp-online-roadmap"
                         "<Image>/Help/GIMP Online")


(script-fu-register-procedure "gimp-online-bugs-features"
   _"_Bug Reports and Feature Requests"
   _"Bookmark to the bug tracker of GIMP"
    "Alexandre Prokoudine <alexandre.prokoudine@gmail.com>"
    "2018"
)

(script-fu-menu-register "gimp-online-bugs-features"
                         "<Image>/Help/GIMP Online")


(script-fu-register-procedure "gimp-online-docs-web-site"
   _"_User Manual Web Site"
   _"Bookmark to the GIMP web site"
    "Roman Joost <romanofski@gimp.org>"
    "2006"
)

(script-fu-menu-register "gimp-online-docs-web-site"
                         "<Image>/Help/GIMP Online")


; (script-fu-register-procedure "gimp-online-plug-in-web-site"
;    _"Plug-in _Registry"
;    _"Bookmark to the GIMP web site"
;     "Henrik Brix Andersen <brix@gimp.org>"
;     "2003"
; )

; (script-fu-menu-register "gimp-online-plug-in-web-site"
;                          "<Image>/Help/GIMP Online")
