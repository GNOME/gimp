/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppatternclipboard.h
 * Copyright (C) 2006 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "gimppattern.h"


#define GIMP_TYPE_PATTERN_CLIPBOARD            (gimp_pattern_clipboard_get_type ())
#define GIMP_PATTERN_CLIPBOARD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PATTERN_CLIPBOARD, GimpPatternClipboard))
#define GIMP_PATTERN_CLIPBOARD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PATTERN_CLIPBOARD, GimpPatternClipboardClass))
#define GIMP_IS_PATTERN_CLIPBOARD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PATTERN_CLIPBOARD))
#define GIMP_IS_PATTERN_CLIPBOARD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PATTERN_CLIPBOARD))
#define GIMP_PATTERN_CLIPBOARD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PATTERN_CLIPBOARD, GimpPatternClipboardClass))


typedef struct _GimpPatternClipboardClass GimpPatternClipboardClass;

struct _GimpPatternClipboard
{
  GimpPattern  parent_instance;

  Gimp        *gimp;
};

struct _GimpPatternClipboardClass
{
  GimpPatternClass  parent_class;
};


GType      gimp_pattern_clipboard_get_type (void) G_GNUC_CONST;

GimpData * gimp_pattern_clipboard_new      (Gimp *gimp);
