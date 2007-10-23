; GIMP - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; "Copy Visible" -- copy the visible selection so that it can be pasted easily
; Copyright (C) 2004 Raphaël Quinet, Adrian Likins, Sven Neumann
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
; Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
;
; 2004-04-14 This script was almost rewritten from scratch
;     by Raphaël Quinet <raphael@gimp.org>
;     see also http://bugzilla.gnome.org/show_bug.cgi?id=139989
;
; The code is new but the API is the same as in the previous version:
; "Copy Visible"  version 0.11 01/24/98
;     by Adrian Likins <adrian@gimp.org>
;   _heavily_ based on:
;        cyn-merge.scm   version 0.02   10/10/97
;        Copyright (C) 1997 Sven Neumann (neumanns@uni-duesseldorf.de)
;
; Removed all code and made it a backward-compat wrapper around
;     (gimp-edit-copy-visible)
;     2004-12-12 Michael Natterer <mitch@gimp.org>
;

(define (script-fu-copy-visible image drawable)
  (gimp-edit-copy-visible image)
)

(script-fu-register "script-fu-copy-visible"
  "Copy Visible"
  "This procedure is deprecated! use \'gimp-edit-copy-visible\' instead."
  ""
  ""
  ""
  "RGB* INDEXED* GRAY*"
  SF-IMAGE    "Image"    0
  SF-DRAWABLE "Drawable" 0
)
