/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"

#include "ligmadnd.h"
#include "ligmahelp-ids.h"
#include "ligmatoolbox.h"
#include "ligmatoolbox-image-area.h"
#include "ligmaview.h"
#include "ligmawindowstrategy.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


static void
image_preview_clicked (GtkWidget       *widget,
                       GdkModifierType  state,
                       LigmaToolbox     *toolbox)
{
  LigmaContext *context = ligma_toolbox_get_context (toolbox);

  ligma_window_strategy_show_dockable_dialog (LIGMA_WINDOW_STRATEGY (ligma_get_window_strategy (context->ligma)),
                                             context->ligma,
                                             ligma_dock_get_dialog_factory (LIGMA_DOCK (toolbox)),
                                             ligma_widget_get_monitor (widget),
                                             "ligma-image-list|ligma-image-grid");
}

static void
image_preview_drop_image (GtkWidget    *widget,
                          gint          x,
                          gint          y,
                          LigmaViewable *viewable,
                          gpointer      data)
{
  LigmaContext *context = LIGMA_CONTEXT (data);

  ligma_context_set_image (context, LIGMA_IMAGE (viewable));
}

static void
image_preview_set_viewable (LigmaView     *view,
                            LigmaViewable *old_viewable,
                            LigmaViewable *new_viewable)
{
  if (! old_viewable && new_viewable)
    {
      ligma_dnd_xds_source_add (GTK_WIDGET (view),
                               (LigmaDndDragViewableFunc) ligma_view_get_viewable,
                               NULL);
    }
  else if (old_viewable && ! new_viewable)
    {
      ligma_dnd_xds_source_remove (GTK_WIDGET (view));
    }
}


/*  public functions  */

GtkWidget *
ligma_toolbox_image_area_create (LigmaToolbox *toolbox,
                                gint         width,
                                gint         height)
{
  LigmaContext *context;
  GtkWidget   *image_view;
  gchar       *tooltip;

  g_return_val_if_fail (LIGMA_IS_TOOLBOX (toolbox), NULL);

  context = ligma_toolbox_get_context (toolbox);

  image_view = ligma_view_new_full_by_types (context,
                                            LIGMA_TYPE_VIEW, LIGMA_TYPE_IMAGE,
                                            width, height, 0,
                                            FALSE, TRUE, TRUE);

  g_signal_connect (image_view, "set-viewable",
                    G_CALLBACK (image_preview_set_viewable),
                    NULL);

  ligma_view_set_viewable (LIGMA_VIEW (image_view),
                          LIGMA_VIEWABLE (ligma_context_get_image (context)));

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

  ligma_help_set_help_data (image_view, tooltip,
                           LIGMA_HELP_TOOLBOX_IMAGE_AREA);
  g_free (tooltip);

  g_signal_connect_object (context, "image-changed",
                           G_CALLBACK (ligma_view_set_viewable),
                           image_view, G_CONNECT_SWAPPED);

  g_signal_connect (image_view, "clicked",
                    G_CALLBACK (image_preview_clicked),
                    toolbox);

  ligma_dnd_viewable_dest_add (image_view,
                              LIGMA_TYPE_IMAGE,
                              image_preview_drop_image,
                              context);

  return image_view;
}
