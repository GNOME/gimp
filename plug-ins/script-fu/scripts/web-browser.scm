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


(define (gimp-online-main-web-site)
  (plug-in-web-browser "http://www.gimp.org"))

(define (gimp-online-developer-web-site)
  (plug-in-web-browser "http://developer.gimp.org"))

(define (gimp-online-plug-in-web-site)
  (plug-in-web-browser "http://registry.gimp.org"))


(script-fu-register "gimp-online-main-web-site"
                    _"_Main Web Site"
                    "Link to http://www.gimp.org"
		    "Henrik Brix Andersen <brix@gimp.org>"
		    "Henrik Brix Andersen <brix@gimp.org>"
		    "2003"
		    "")

(script-fu-menu-register "gimp-online-main-web-site"
			 "<Toolbox>/Help/GIMP Online")


(script-fu-register "gimp-online-developer-web-site"
                    _"_Developer Web Site"
                    "Link to http://developer.gimp.org"
		    "Henrik Brix Andersen <brix@gimp.org>"
		    "Henrik Brix Andersen <brix@gimp.org>"
		    "2003"
		    "")

(script-fu-menu-register "gimp-online-developer-web-site"
			 "<Toolbox>/Help/GIMP Online")


(script-fu-register "gimp-online-plug-in-web-site"
                    _"Plug-in _Registry"
                    "Link to http://registry.gimp.org"
		    "Henrik Brix Andersen <brix@gimp.org>"
		    "Henrik Brix Andersen <brix@gimp.org>"
		    "2003"
		    "")

(script-fu-menu-register "gimp-online-plug-in-web-site"
			 "<Toolbox>/Help/GIMP Online")
