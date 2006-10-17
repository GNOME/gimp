; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; gimp-online.scm
; Copyright (C) 2003  Henrik Brix Andersen <brix@gimp.org>
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

(define (gimp-online-docs-web-site)
  (plug-in-web-browser "http://docs.gimp.org")
)

(define (gimp-help-2-concepts-usage)
  (gimp-help "" "gimp-concepts-usage")
)

(define (gimp-help-2-using-docks)
  (gimp-help "" "gimp-using-docks")
)

(define (gimp-help-2-using-simpleobjects)
  (gimp-help "" "gimp-using-simpleobjects")
)

(define (gimp-help-2-using-selections)
  (gimp-help "" "gimp-using-selections")
)

(define (gimp-help-2-using-fileformats)
  (gimp-help "" "gimp-using-fileformats")
)

(define (gimp-help-2-using-photography)
  (gimp-help "" "gimp-using-photography")
)

(define (gimp-help-2-using-web)
  (gimp-help "" "gimp-using-web")
)

(define (gimp-help-2-concepts-paths)
  (gimp-help "" "gimp-concepts-paths")
)

; shortcuts to help topics
(script-fu-register "gimp-help-2-concepts-paths"
   _"Using _Paths"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@gimp.org>"
    "Roman Joost <romanofski@gimp.org>"
    "2006"
    ""
)
(script-fu-menu-register "gimp-help-2-concepts-paths"
			 "<Toolbox>/Help/User Manual")

(script-fu-register "gimp-help-2-using-web"
   _"_Preparing your Images for the Web"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@gimp.org>"
    "Roman Joost <romanofski@gimp.org>"
    "2006"
    ""
)
(script-fu-menu-register "gimp-help-2-using-web"
			 "<Toolbox>/Help/User Manual")

(script-fu-register "gimp-help-2-using-photography"
   _"_Working with Digital Camera Photos"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@gimp.org>"
    "Roman Joost <romanofski@gimp.org>"
    "2006"
    ""
)
(script-fu-menu-register "gimp-help-2-using-photography"
			 "<Toolbox>/Help/User Manual")

(script-fu-register "gimp-help-2-using-fileformats"
   _"Create, Open and Save _Files"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@gimp.org>"
    "Roman Joost <romanofski@gimp.org>"
    "2006"
    ""
)
(script-fu-menu-register "gimp-help-2-using-fileformats"
			 "<Toolbox>/Help/User Manual")

(script-fu-register "gimp-help-2-concepts-usage"
   _"_Basic Concepts"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@gimp.org>"
    "Roman Joost <romanofski@gimp.org>"
    "2006"
    ""
)
(script-fu-menu-register "gimp-help-2-concepts-usage"
			 "<Toolbox>/Help/User Manual")

(script-fu-register "gimp-help-2-using-docks"
   _"How to Use _Dialogs"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@gimp.org>"
    "Roman Joost <romanofski@gimp.org>"
    "2006"
    ""
)
(script-fu-menu-register "gimp-help-2-using-docks"
			 "<Toolbox>/Help/User Manual")

(script-fu-register "gimp-help-2-using-simpleobjects"
   _"Drawing _Simple Objects"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@gimp.org>"
    "Roman Joost <romanofski@gimp.org>"
    "2006"
    ""
)
(script-fu-menu-register "gimp-help-2-using-simpleobjects"
			 "<Toolbox>/Help/User Manual")

(script-fu-register "gimp-help-2-using-selections"
   _"Create and Use _Selections"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@gimp.org>"
    "Roman Joost <romanofski@gimp.org>"
    "2006"
    ""
)
(script-fu-menu-register "gimp-help-2-using-simpleobjects"
			 "<Toolbox>/Help/User Manual")


;; Links to GIMP related web sites

(define (gimp-online-main-web-site)
  (plug-in-web-browser "http://www.gimp.org/")
)

(define (gimp-online-developer-web-site)
  (plug-in-web-browser "http://developer.gimp.org/")
)

(define (gimp-online-plug-in-web-site)
  (plug-in-web-browser "http://registry.gimp.org/")
)

(script-fu-register "gimp-online-main-web-site"
   _"_Main Web Site"
   _"Bookmark to the GIMP web site"
    "Henrik Brix Andersen <brix@gimp.org>"
    "Henrik Brix Andersen <brix@gimp.org>"
    "2003"
    ""
)

(script-fu-menu-register "gimp-online-main-web-site"
                         "<Toolbox>/Help/GIMP Online")

(script-fu-register "gimp-online-developer-web-site"
   _"_Developer Web Site"
   _"Bookmark to the GIMP web site"
    "Henrik Brix Andersen <brix@gimp.org>"
    "Henrik Brix Andersen <brix@gimp.org>"
    "2003"
    ""
)

(script-fu-menu-register "gimp-online-developer-web-site"
                         "<Toolbox>/Help/GIMP Online")

(script-fu-register "gimp-online-docs-web-site"
   _"_User Manual Web Site"
   _"Bookmark to the GIMP web site"
    "Roman Joost <romanofski@gimp.org>"
    "Roman Joost <romanofski@gimp.org>"
    "2006"
    ""
)

(script-fu-register "gimp-online-plug-in-web-site"
   _"Plug-in _Registry"
   _"Bookmark to the GIMP web site"
    "Henrik Brix Andersen <brix@gimp.org>"
    "Henrik Brix Andersen <brix@gimp.org>"
    "2003"
    ""
)

(script-fu-menu-register "gimp-online-plug-in-web-site"
                         "<Toolbox>/Help/GIMP Online")
