/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmabrushclipboard.h
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

#ifndef __LIGMA_BRUSH_CLIPBOARD_H__
#define __LIGMA_BRUSH_CLIPBOARD_H__


#include "ligmabrush.h"


#define LIGMA_TYPE_BRUSH_CLIPBOARD            (ligma_brush_clipboard_get_type ())
#define LIGMA_BRUSH_CLIPBOARD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_BRUSH_CLIPBOARD, LigmaBrushClipboard))
#define LIGMA_BRUSH_CLIPBOARD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_BRUSH_CLIPBOARD, LigmaBrushClipboardClass))
#define LIGMA_IS_BRUSH_CLIPBOARD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_BRUSH_CLIPBOARD))
#define LIGMA_IS_BRUSH_CLIPBOARD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_BRUSH_CLIPBOARD))
#define LIGMA_BRUSH_CLIPBOARD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_BRUSH_CLIPBOARD, LigmaBrushClipboardClass))


typedef struct _LigmaBrushClipboardClass LigmaBrushClipboardClass;

struct _LigmaBrushClipboard
{
  LigmaBrush  parent_instance;

  Ligma      *ligma;
  gboolean   mask_only;
};

struct _LigmaBrushClipboardClass
{
  LigmaBrushClass  parent_class;
};


GType      ligma_brush_clipboard_get_type (void) G_GNUC_CONST;

LigmaData * ligma_brush_clipboard_new      (Ligma     *ligma,
                                          gboolean  mask_only);


#endif  /*  __LIGMA_BRUSH_CLIPBOARD_H__  */
