/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatoolbutton.h
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

#ifndef __LIGMA_TOOL_BUTTON_H__
#define __LIGMA_TOOL_BUTTON_H__


#define LIGMA_TYPE_TOOL_BUTTON            (ligma_tool_button_get_type ())
#define LIGMA_TOOL_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TOOL_BUTTON, LigmaToolButton))
#define LIGMA_TOOL_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TOOL_BUTTON, LigmaToolButtonClass))
#define LIGMA_IS_TOOL_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, LIGMA_TYPE_TOOL_BUTTON))
#define LIGMA_IS_TOOL_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TOOL_BUTTON))
#define LIGMA_TOOL_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TOOL_BUTTON, LigmaToolButtonClass))


typedef struct _LigmaToolButtonPrivate LigmaToolButtonPrivate;
typedef struct _LigmaToolButtonClass   LigmaToolButtonClass;

struct _LigmaToolButton
{
  GtkToggleToolButton    parent_instance;

  LigmaToolButtonPrivate *priv;
};

struct _LigmaToolButtonClass
{
  GtkToggleToolButtonClass  parent_class;
};


GType          ligma_tool_button_get_type      (void) G_GNUC_CONST;

GtkToolItem  * ligma_tool_button_new           (LigmaToolbox    *toolbox,
                                               LigmaToolItem   *tool_item);

LigmaToolbox  * ligma_tool_button_get_toolbox   (LigmaToolButton *tool_button);

void           ligma_tool_button_set_tool_item (LigmaToolButton *tool_button,
                                               LigmaToolItem   *tool_item);
LigmaToolItem * ligma_tool_button_get_tool_item (LigmaToolButton *tool_button);

LigmaToolInfo * ligma_tool_button_get_tool_info (LigmaToolButton *tool_button);


#endif /* __LIGMA_TOOL_BUTTON_H__ */
