/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpvectorstreeview.c
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
#include "core/gimpimage.h"

#include "vectors/gimpvectors.h"

#include "gimpcontainerview.h"
#include "gimpvectorstreeview.h"
#include "gimpdnd.h"
#include "gimphelp-ids.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


static void   gimp_vectors_tree_view_class_init (GimpVectorsTreeViewClass *klass);
static void   gimp_vectors_tree_view_init       (GimpVectorsTreeView      *view);

static GObject * gimp_vectors_tree_view_constructor (GType                  type,
                                                     guint                  n_params,
                                                     GObjectConstructParam *params);


static GimpItemTreeViewClass *parent_class = NULL;


GType
gimp_vectors_tree_view_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpVectorsTreeViewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_vectors_tree_view_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpVectorsTreeView),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_vectors_tree_view_init,
      };

      view_type = g_type_register_static (GIMP_TYPE_ITEM_TREE_VIEW,
                                          "GimpVectorsTreeView",
                                          &view_info, 0);
    }

  return view_type;
}

static void
gimp_vectors_tree_view_class_init (GimpVectorsTreeViewClass *klass)
{
  GObjectClass          *object_class    = G_OBJECT_CLASS (klass);
  GimpItemTreeViewClass *item_view_class = GIMP_ITEM_TREE_VIEW_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor        = gimp_vectors_tree_view_constructor;

  item_view_class->get_container   = gimp_image_get_vectors;
  item_view_class->get_active_item = (GimpGetItemFunc) gimp_image_get_active_vectors;
  item_view_class->set_active_item = (GimpSetItemFunc) gimp_image_set_active_vectors;
  item_view_class->reorder_item    = (GimpReorderItemFunc) gimp_image_position_vectors;
  item_view_class->add_item        = (GimpAddItemFunc) gimp_image_add_vectors;
  item_view_class->remove_item     = (GimpRemoveItemFunc) gimp_image_remove_vectors;

  item_view_class->edit_desc               = _("Edit Path Attributes");
  item_view_class->edit_help_id            = GIMP_HELP_PATH_EDIT;
  item_view_class->new_desc                = _("New Path\n%s New Path Dialog");
  item_view_class->new_help_id             = GIMP_HELP_PATH_NEW;
  item_view_class->duplicate_desc          = _("Duplicate Path");
  item_view_class->duplicate_help_id       = GIMP_HELP_PATH_DUPLICATE;
  item_view_class->delete_desc             = _("Delete Path");
  item_view_class->delete_help_id          = GIMP_HELP_PATH_DELETE;
  item_view_class->raise_desc              = _("Raise Path");
  item_view_class->raise_help_id           = GIMP_HELP_PATH_RAISE;
  item_view_class->raise_to_top_desc       = _("Raise Path to Top");
  item_view_class->raise_to_top_help_id    = GIMP_HELP_PATH_RAISE_TO_TOP;
  item_view_class->lower_desc              = _("Lower Path");
  item_view_class->lower_help_id           = GIMP_HELP_PATH_LOWER;
  item_view_class->lower_to_bottom_desc    = _("Lower Path to Bottom");
  item_view_class->lower_to_bottom_help_id = GIMP_HELP_PATH_LOWER_TO_BOTTOM;
  item_view_class->reorder_desc            = _("Reorder Path");
}

static void
gimp_vectors_tree_view_init (GimpVectorsTreeView *view)
{
}

static GObject *
gimp_vectors_tree_view_constructor (GType                  type,
                                    guint                  n_params,
                                    GObjectConstructParam *params)
{
  GObject             *object;
  GimpEditor          *editor;
  GimpVectorsTreeView *view;
  gchar               *str;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  editor = GIMP_EDITOR (object);
  view   = GIMP_VECTORS_TREE_VIEW (object);

  /*  Hide basically useless Edit button  */

  gtk_widget_hide (GIMP_ITEM_TREE_VIEW (view)->edit_button);

  str = g_strdup_printf (_("Path to Selection\n"
                           "%s  Add\n"
                           "%s  Subtract\n"
                           "%s%s%s  Intersect"),
                         gimp_get_mod_name_shift (),
                         gimp_get_mod_name_control (),
                         gimp_get_mod_name_shift (),
                         gimp_get_mod_separator (),
                         gimp_get_mod_name_control ());

  view->toselection_button =
    gimp_editor_add_action_button (editor, "vectors",
                                   "vectors-selection-replace",
                                   "vectors-selection-intersect",
                                   GDK_SHIFT_MASK | GDK_CONTROL_MASK,
                                   "vectors-selection-subtract",
                                   GDK_CONTROL_MASK,
                                   "vectors-selection-add",
                                   GDK_SHIFT_MASK,
                                   NULL);

  gimp_help_set_help_data (view->toselection_button, str,
                           GIMP_HELP_PATH_SELECTION_REPLACE);

  g_free (str);

  str = g_strdup_printf (_("Selection to Path\n"
                           "%s  Advanced Options"),
                         gimp_get_mod_name_shift ());

  view->tovectors_button =
    gimp_editor_add_action_button (editor, "vectors",
                                   "vectors-selection-to-vectors",
                                   "vectors-selection-to-vectors-advanced",
                                   GDK_SHIFT_MASK,
                                   NULL);

  gimp_help_set_help_data (view->tovectors_button, str,
                           GIMP_HELP_SELECTION_TO_PATH);

  g_free (str);

  view->stroke_button =
    gimp_editor_add_action_button (editor, "vectors",
                                   "vectors-stroke", NULL);

  gtk_box_reorder_child (GTK_BOX (editor->button_box),
			 view->toselection_button, 5);
  gtk_box_reorder_child (GTK_BOX (editor->button_box),
			 view->tovectors_button, 6);
  gtk_box_reorder_child (GTK_BOX (editor->button_box),
			 view->stroke_button, 7);

  gimp_container_view_enable_dnd (GIMP_CONTAINER_VIEW (editor),
				  GTK_BUTTON (view->toselection_button),
				  GIMP_TYPE_VECTORS);
  gimp_container_view_enable_dnd (GIMP_CONTAINER_VIEW (editor),
				  GTK_BUTTON (view->stroke_button),
				  GIMP_TYPE_VECTORS);

  return object;
}
