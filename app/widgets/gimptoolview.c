/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolview.c
 * Copyright (C) 2001-2004 Michael Natterer <mitch@gimp.org>
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

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimptoolinfo.h"

#include "tools/tools-types.h"

#include "tools/gimp-tools.h"
#include "tools/gimpimagemaptool.h"

#include "gimpcontainerview.h"
#include "gimptoolview.h"
#include "gimphelp-ids.h"

#include "gimp-intl.h"


static void   gimp_tool_view_class_init    (GimpToolViewClass   *klass);
static void   gimp_tool_view_init          (GimpToolView        *view);

static void   gimp_tool_view_reset_clicked (GtkWidget           *widget,
                                            GimpToolView        *view);
static void   gimp_tool_view_select_item   (GimpContainerEditor *editor,
                                            GimpViewable        *viewable);
static void   gimp_tool_view_activate_item (GimpContainerEditor *editor,
                                            GimpViewable        *viewable);


static GimpContainerEditorClass *parent_class = NULL;


GType
gimp_tool_view_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpToolViewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_tool_view_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpToolView),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_tool_view_init,
      };

      view_type = g_type_register_static (GIMP_TYPE_CONTAINER_EDITOR,
                                          "GimpToolView",
                                          &view_info, 0);
    }

  return view_type;
}

static void
gimp_tool_view_class_init (GimpToolViewClass *klass)
{
  GimpContainerEditorClass *editor_class = GIMP_CONTAINER_EDITOR_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  editor_class->select_item   = gimp_tool_view_select_item;
  editor_class->activate_item = gimp_tool_view_activate_item;
}

static void
gimp_tool_view_init (GimpToolView *view)
{
}

GtkWidget *
gimp_tool_view_new (GimpViewType     view_type,
                    GimpContainer   *container,
                    GimpContext     *context,
                    gint             preview_size,
                    gint             preview_border_width,
                    GimpMenuFactory *menu_factory)
{
  GimpToolView        *tool_view;
  GimpContainerEditor *editor;

  tool_view = g_object_new (GIMP_TYPE_TOOL_VIEW, NULL);

  if (! gimp_container_editor_construct (GIMP_CONTAINER_EDITOR (tool_view),
                                         view_type,
                                         container,context,
                                         preview_size, preview_border_width,
                                         TRUE, /* reorderable */
                                         menu_factory, "<Tools>",
                                         "/tools-popup"))
    {
      g_object_unref (tool_view);
      return NULL;
    }

  editor = GIMP_CONTAINER_EDITOR (tool_view);

  tool_view->reset_button =
    gimp_editor_add_button (GIMP_EDITOR (editor->view),
                            GIMP_STOCK_RESET,
                            _("Reset tool order and visibility"),
                            NULL,
                            G_CALLBACK (gimp_tool_view_reset_clicked),
                            NULL,
                            editor);

  return GTK_WIDGET (tool_view);
}

static void
gimp_tool_view_reset_clicked (GtkWidget    *widget,
                              GimpToolView *view)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (view);
  GimpContainer       *container;
  GimpContext         *context;
  GList               *list;
  gint                 i = 0;

  container = gimp_container_view_get_container (editor->view);
  context   = gimp_container_view_get_context (editor->view);

  for (list = gimp_tools_get_default_order (context->gimp);
       list;
       list = g_list_next (list))
    {
      GimpObject *object = gimp_container_get_child_by_name (container,
                                                             list->data);

      if (object)
        {
          gimp_container_reorder (container, object, i);

          g_object_set (object, "visible",
                        ! g_type_is_a (GIMP_TOOL_INFO (object)->tool_type,
                                       GIMP_TYPE_IMAGE_MAP_TOOL),
                        NULL);

          i++;
        }
    }
}

static void
gimp_tool_view_select_item (GimpContainerEditor *editor,
                            GimpViewable        *viewable)
{
  if (GIMP_CONTAINER_EDITOR_CLASS (parent_class)->select_item)
    GIMP_CONTAINER_EDITOR_CLASS (parent_class)->select_item (editor, viewable);
}

static void
gimp_tool_view_activate_item (GimpContainerEditor *editor,
				GimpViewable        *viewable)
{
  if (GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item)
    GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item (editor, viewable);
}
