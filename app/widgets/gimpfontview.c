/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfontview.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"

#include "gimpcontainerview.h"
#include "gimpeditor.h"
#include "gimpfontview.h"
#include "gimphelp-ids.h"
#include "gimpuimanager.h"

#include "gimp-intl.h"


static void   gimp_font_view_activate_item (GimpContainerEditor *editor,
                                            GimpViewable        *viewable);


G_DEFINE_TYPE (GimpFontView, gimp_font_view, GIMP_TYPE_CONTAINER_EDITOR)

#define parent_class gimp_font_view_parent_class


static void
gimp_font_view_class_init (GimpFontViewClass *klass)
{
  GimpContainerEditorClass *editor_class = GIMP_CONTAINER_EDITOR_CLASS (klass);

  editor_class->activate_item = gimp_font_view_activate_item;
}

static void
gimp_font_view_init (GimpFontView *view)
{
  view->refresh_button = NULL;
}

GtkWidget *
gimp_font_view_new (GimpViewType     view_type,
                    GimpContainer   *container,
                    GimpContext     *context,
                    gint             view_size,
                    gint             view_border_width,
                    GimpMenuFactory *menu_factory)
{
  GimpFontView        *font_view;
  GimpContainerEditor *editor;

  font_view = g_object_new (GIMP_TYPE_FONT_VIEW, NULL);

  if (! gimp_container_editor_construct (GIMP_CONTAINER_EDITOR (font_view),
                                         view_type,
                                         container,context,
                                         view_size, view_border_width,
                                         menu_factory, "<Fonts>",
                                         "/fonts-popup"))
    {
      g_object_unref (font_view);
      return NULL;
    }

  editor = GIMP_CONTAINER_EDITOR (font_view);

  gimp_container_view_set_reorderable (GIMP_CONTAINER_VIEW (editor->view),
                                       FALSE);

  font_view->refresh_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "fonts",
                                   "fonts-refresh", NULL);

  gimp_ui_manager_update (GIMP_EDITOR (editor->view)->ui_manager, editor);

  return GTK_WIDGET (font_view);
}

static void
gimp_font_view_activate_item (GimpContainerEditor *editor,
                                GimpViewable        *viewable)
{
  if (GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item)
    GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item (editor, viewable);
}
