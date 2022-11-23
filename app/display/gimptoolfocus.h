/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatoolfocus.h
 * Copyright (C) 2020 Ell
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

#ifndef __LIGMA_TOOL_FOCUS_H__
#define __LIGMA_TOOL_FOCUS_H__


#include "ligmatoolwidget.h"


#define LIGMA_TYPE_TOOL_FOCUS            (ligma_tool_focus_get_type ())
#define LIGMA_TOOL_FOCUS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TOOL_FOCUS, LigmaToolFocus))
#define LIGMA_TOOL_FOCUS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TOOL_FOCUS, LigmaToolFocusClass))
#define LIGMA_IS_TOOL_FOCUS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TOOL_FOCUS))
#define LIGMA_IS_TOOL_FOCUS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TOOL_FOCUS))
#define LIGMA_TOOL_FOCUS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TOOL_FOCUS, LigmaToolFocusClass))


typedef struct _LigmaToolFocus        LigmaToolFocus;
typedef struct _LigmaToolFocusPrivate LigmaToolFocusPrivate;
typedef struct _LigmaToolFocusClass   LigmaToolFocusClass;

struct _LigmaToolFocus
{
  LigmaToolWidget        parent_instance;

  LigmaToolFocusPrivate *priv;
};

struct _LigmaToolFocusClass
{
  LigmaToolWidgetClass  parent_class;
};


GType            ligma_tool_focus_get_type (void) G_GNUC_CONST;

LigmaToolWidget * ligma_tool_focus_new      (LigmaDisplayShell *shell);


#endif /* __LIGMA_TOOL_FOCUS_H__ */
