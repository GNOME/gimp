; LIGMA - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; ligma-online.scm
; Copyright (C) 2003  Henrik Brix Andersen <brix@ligma.org>
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

(define (ligma-online-docs-web-site)
  (plug-in-web-browser "https://docs.ligma.org/")
)

(define (ligma-help-concepts-usage)
  (ligma-help "" "ligma-concepts-usage")
)

(define (ligma-help-using-docks)
  (ligma-help "" "ligma-concepts-docks")
)

(define (ligma-help-using-simpleobjects)
  (ligma-help "" "ligma-using-simpleobjects")
)

(define (ligma-help-using-selections)
  (ligma-help "" "ligma-using-selections")
)

(define (ligma-help-using-fileformats)
  (ligma-help "" "ligma-using-fileformats")
)

(define (ligma-help-using-photography)
  (ligma-help "" "ligma-using-photography")
)

(define (ligma-help-using-web)
  (ligma-help "" "ligma-using-web")
)

(define (ligma-help-concepts-paths)
  (ligma-help "" "ligma-concepts-paths")
)


; shortcuts to help topics
(script-fu-register "ligma-help-concepts-paths"
   _"Using _Paths"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@ligma.org>"
    "Roman Joost <romanofski@ligma.org>"
    "2006"
    ""
)

(script-fu-menu-register "ligma-help-concepts-paths"
			 "<Image>/Help/User Manual")


(script-fu-register "ligma-help-using-web"
   _"_Preparing your Images for the Web"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@ligma.org>"
    "Roman Joost <romanofski@ligma.org>"
    "2006"
    ""
)

(script-fu-menu-register "ligma-help-using-web"
			 "<Image>/Help/User Manual")


(script-fu-register "ligma-help-using-photography"
   _"_Working with Digital Camera Photos"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@ligma.org>"
    "Roman Joost <romanofski@ligma.org>"
    "2006"
    ""
)

(script-fu-menu-register "ligma-help-using-photography"
			 "<Image>/Help/User Manual")


(script-fu-register "ligma-help-using-fileformats"
   _"Create, Open and Save _Files"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@ligma.org>"
    "Roman Joost <romanofski@ligma.org>"
    "2006"
    ""
)

(script-fu-menu-register "ligma-help-using-fileformats"
			 "<Image>/Help/User Manual")


(script-fu-register "ligma-help-concepts-usage"
   _"_Basic Concepts"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@ligma.org>"
    "Roman Joost <romanofski@ligma.org>"
    "2006"
    ""
)

(script-fu-menu-register "ligma-help-concepts-usage"
			 "<Image>/Help/User Manual")


(script-fu-register "ligma-help-using-docks"
   _"How to Use _Dialogs"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@ligma.org>"
    "Roman Joost <romanofski@ligma.org>"
    "2006"
    ""
)

(script-fu-menu-register "ligma-help-using-docks"
			 "<Image>/Help/User Manual")


(script-fu-register "ligma-help-using-simpleobjects"
   _"Drawing _Simple Objects"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@ligma.org>"
    "Roman Joost <romanofski@ligma.org>"
    "2006"
    ""
)

(script-fu-menu-register "ligma-help-using-simpleobjects"
			 "<Image>/Help/User Manual")


(script-fu-register "ligma-help-using-selections"
   _"Create and Use _Selections"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@ligma.org>"
    "Roman Joost <romanofski@ligma.org>"
    "2006"
    ""
)

(script-fu-menu-register "ligma-help-using-simpleobjects"
			 "<Image>/Help/User Manual")


;; Links to LIGMA related web sites

(define (ligma-online-main-web-site)
  (plug-in-web-browser "https://www.ligma.org/")
)

(define (ligma-online-developer-web-site)
  (plug-in-web-browser "https://developer.ligma.org/")
)

(define (ligma-online-roadmap)
  (plug-in-web-browser "https://wiki.ligma.org/wiki/Roadmap")
)

(define (ligma-online-wiki)
  (plug-in-web-browser "https://wiki.ligma.org/wiki/Main_Page")
)

(define (ligma-online-bugs-features)
  (plug-in-web-browser "https://gitlab.gnome.org/GNOME/ligma/issues")
)

; (define (ligma-online-plug-in-web-site)
;   (plug-in-web-browser "https://registry.ligma.org/")
; )


(script-fu-register "ligma-online-main-web-site"
   _"_Main Web Site"
   _"Bookmark to the LIGMA web site"
    "Henrik Brix Andersen <brix@ligma.org>"
    "Henrik Brix Andersen <brix@ligma.org>"
    "2003"
    ""
)

(script-fu-menu-register "ligma-online-main-web-site"
                         "<Image>/Help/LIGMA Online")


(script-fu-register "ligma-online-developer-web-site"
   _"_Developer Web Site"
   _"Bookmark to the LIGMA web site"
    "Henrik Brix Andersen <brix@ligma.org>"
    "Henrik Brix Andersen <brix@ligma.org>"
    "2003"
    ""
)

(script-fu-menu-register "ligma-online-developer-web-site"
                         "<Image>/Help/LIGMA Online")


(script-fu-register "ligma-online-roadmap"
   _"_Roadmap"
   _"Bookmark to the roadmap of LIGMA"
    "Alexandre Prokoudine <alexandre.prokoudine@gmail.com>"
    "Alexandre Prokoudine <alexandre.prokoudine@gmail.com>"
    "2018"
    ""
)

(script-fu-menu-register "ligma-online-roadmap"
                         "<Image>/Help/LIGMA Online")


(script-fu-register "ligma-online-wiki"
   _"_Wiki"
   _"Bookmark to the wiki of LIGMA"
    "Alexandre Prokoudine <alexandre.prokoudine@gmail.com>"
    "Alexandre Prokoudine <alexandre.prokoudine@gmail.com>"
    "2018"
    ""
)

(script-fu-menu-register "ligma-online-wiki"
                         "<Image>/Help/LIGMA Online")


(script-fu-register "ligma-online-bugs-features"
   _"_Bug Reports and Feature Requests"
   _"Bookmark to the bug tracker of LIGMA"
    "Alexandre Prokoudine <alexandre.prokoudine@gmail.com>"
    "Alexandre Prokoudine <alexandre.prokoudine@gmail.com>"
    "2018"
    ""
)

(script-fu-menu-register "ligma-online-bugs-features"
                         "<Image>/Help")


(script-fu-register "ligma-online-docs-web-site"
   _"_User Manual Web Site"
   _"Bookmark to the LIGMA web site"
    "Roman Joost <romanofski@ligma.org>"
    "Roman Joost <romanofski@ligma.org>"
    "2006"
    ""
)

(script-fu-menu-register "ligma-online-docs-web-site"
                         "<Image>/Help/LIGMA Online")


; (script-fu-register "ligma-online-plug-in-web-site"
;    _"Plug-in _Registry"
;    _"Bookmark to the LIGMA web site"
;     "Henrik Brix Andersen <brix@ligma.org>"
;     "Henrik Brix Andersen <brix@ligma.org>"
;     "2003"
;     ""
; )

; (script-fu-menu-register "ligma-online-plug-in-web-site"
;                          "<Image>/Help/LIGMA Online")
