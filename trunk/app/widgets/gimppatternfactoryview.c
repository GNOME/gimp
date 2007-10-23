/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppatternfactoryview.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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
#include "core/gimpdatafactory.h"
#include "core/gimpviewable.h"

#include "gimpcontainerview.h"
#include "gimpeditor.h"
#include "gimppatternfactoryview.h"
#include "gimpviewrenderer.h"


G_DEFINE_TYPE (GimpPatternFactoryView, gimp_pattern_factory_view,
               GIMP_TYPE_DATA_FACTORY_VIEW)


static void
gimp_pattern_factory_view_class_init (GimpPatternFactoryViewClass *klass)
{
}

static void
gimp_pattern_factory_view_init (GimpPatternFactoryView *view)
{
}

GtkWidget *
gimp_pattern_factory_view_new (GimpViewType      view_type,
                               GimpDataFactory  *factory,
                               GimpContext      *context,
                               gint              view_size,
                               gint              view_border_width,
                               GimpMenuFactory  *menu_factory)
{
  GimpPatternFactoryView *factory_view;
  GimpContainerEditor    *editor;

  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH,
                        NULL);

  factory_view = g_object_new (GIMP_TYPE_PATTERN_FACTORY_VIEW, NULL);

  if (! gimp_data_factory_view_construct (GIMP_DATA_FACTORY_VIEW (factory_view),
                                          view_type,
                                          factory,
                                          context,
                                          view_size, view_border_width,
                                          menu_factory, "<Patterns>",
                                          "/patterns-popup",
                                          "patterns"))
    {
      g_object_unref (factory_view);
      return NULL;
    }

  editor = GIMP_CONTAINER_EDITOR (factory_view);

  gimp_editor_add_action_button (GIMP_EDITOR (editor->view),
                                 "patterns", "patterns-open-as-image",
                                 NULL);

  gtk_widget_hide (GIMP_DATA_FACTORY_VIEW (factory_view)->edit_button);
  gtk_widget_hide (GIMP_DATA_FACTORY_VIEW (factory_view)->duplicate_button);

  return GTK_WIDGET (factory_view);
}
