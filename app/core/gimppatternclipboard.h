/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapatternclipboard.h
 * Copyright (C) 2006 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_PATTERN_CLIPBOARD_H__
#define __LIGMA_PATTERN_CLIPBOARD_H__


#include "ligmapattern.h"


#define LIGMA_TYPE_PATTERN_CLIPBOARD            (ligma_pattern_clipboard_get_type ())
#define LIGMA_PATTERN_CLIPBOARD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PATTERN_CLIPBOARD, LigmaPatternClipboard))
#define LIGMA_PATTERN_CLIPBOARD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PATTERN_CLIPBOARD, LigmaPatternClipboardClass))
#define LIGMA_IS_PATTERN_CLIPBOARD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PATTERN_CLIPBOARD))
#define LIGMA_IS_PATTERN_CLIPBOARD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PATTERN_CLIPBOARD))
#define LIGMA_PATTERN_CLIPBOARD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PATTERN_CLIPBOARD, LigmaPatternClipboardClass))


typedef struct _LigmaPatternClipboardClass LigmaPatternClipboardClass;

struct _LigmaPatternClipboard
{
  LigmaPattern  parent_instance;

  Ligma        *ligma;
};

struct _LigmaPatternClipboardClass
{
  LigmaPatternClass  parent_class;
};


GType      ligma_pattern_clipboard_get_type (void) G_GNUC_CONST;

LigmaData * ligma_pattern_clipboard_new      (Ligma *ligma);


#endif  /*  __LIGMA_PATTERN_CLIPBOARD_H__  */
