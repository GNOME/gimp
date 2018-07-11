/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "gimpdnd.h"
#include "gimpview.h"
#include "gimptoolbox.h"
#include "gimptoolbox-image-area.h"
#include "gimpwindowstrategy.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


static void
image_preview_clicked (GtkWidget       *widget,
                       GdkModifierType  state,
                       GimpToolbox     *toolbox)
{
  GimpContext *context = gimp_toolbox_get_context (toolbox);

  gimp_window_strategy_show_dockable_dialog (GIMP_WINDOW_STRATEGY (gimp_get_window_strategy (context->gimp)),
                                             context->gimp,
                                             gimp_dock_get_dialog_factory (GIMP_DOCK (toolbox)),
                                             gtk_widget_get_screen (widget),
                                             gimp_widget_get_monitor (widget),
                                             "gimp-image-list|gimp-image-grid");
}

static void
image_preview_drop_image (GtkWidget    *widget,
                          gint          x,
                          gint          y,
                          GimpViewable *viewable,
                          gpointer      data)
{
  GimpContext *context = GIMP_CONTEXT (data);

  gimp_context_set_image (context, GIMP_IMAGE (viewable));
}

static void
image_preview_set_viewable (GimpView     *view,
                            GimpViewable *old_viewable,
                            GimpViewable *new_viewable)
{
  if (! old_viewable && new_viewable)
    {
      gimp_dnd_xds_source_add (GTK_WIDGET (view),
                               (GimpDndDragViewableFunc) gimp_view_get_viewable,
                               NULL);
    }
  else if (old_viewable && ! new_viewable)
    {
      gimp_dnd_xds_source_remove (GTK_WIDGET (view));
    }
}

/*  public functions  */

GtkWidget *
gimp_toolbox_image_area_create (GimpToolbox *toolbox,
                                gint         width,
                                gint         height)
{
  GimpContext *context;
  GtkWidget   *image_view;
  gchar       *tooltip;

  g_return_val_if_fail (GIMP_IS_TOOLBOX (toolbox), NULL);

  context = gimp_toolbox_get_context (toolbox);

  image_view = gimp_view_new_full_by_types (context,
                                            GIMP_TYPE_VIEW, GIMP_TYPE_IMAGE,
                                            width, height, 0,
                                            FALSE, TRUE, TRUE);

  g_signal_connect (image_view, "set-viewable",
                    G_CALLBACK (image_preview_set_viewable),
                    NULL);

  gimp_view_set_viewable (GIMP_VIEW (image_view),
                          GIMP_VIEWABLE (gimp_context_get_image (context)));

  gtk_widget_show (image_view);

#ifdef GDK_WINDOWING_X11
  tooltip = g_strdup_printf ("%s\n%s",
                             _("The active image.\n"
                               "Click to open the Image Dialog."),
                             _("Drag to an XDS enabled file-manager to "
                               "save the image."));
#else
  tooltip = g_strdup (_("The active image.\n"
                        "Click to open the Image Dialog."));
#endif

  gimp_help_set_help_data (image_view, tooltip, NULL);
  g_free (tooltip);

  g_signal_connect_object (context, "image-changed",
                           G_CALLBACK (gimp_view_set_viewable),
                           image_view, G_CONNECT_SWAPPED);

  g_signal_connect (image_view, "clicked",
                    G_CALLBACK (image_preview_clicked),
                    toolbox);

  gimp_dnd_viewable_dest_add (image_view,
                              GIMP_TYPE_IMAGE,
                              image_preview_drop_image,
                              context);

  return image_view;
}
