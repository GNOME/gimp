/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "gimpdialogfactory.h"
#include "gimpdnd.h"
#include "gimpview.h"
#include "gimptoolbox.h"
#include "gimptoolbox-image-area.h"

#include "gimp-intl.h"


#define CELL_WIDTH       48
#define CELL_HEIGHT      48


static void
image_preview_clicked (GtkWidget       *widget,
                       GdkModifierType  state,
                       GimpToolbox     *toolbox)
{
  gimp_dialog_factory_dialog_raise (GIMP_DOCK (toolbox)->dialog_factory,
                                    gtk_widget_get_screen (widget),
                                    "gimp-image-list|gimp-image-grid", -1);
}

static void
image_preview_drop_image (GtkWidget    *widget,
                          GimpViewable *viewable,
                          gpointer      data)
{
  GimpContext *context = GIMP_CONTEXT (data);

  gimp_context_set_image (context, GIMP_IMAGE (viewable));
}


/*  public functions  */

GtkWidget *
gimp_toolbox_image_area_create (GimpToolbox *toolbox,
                                gint         width,
                                gint         height)
{
  GimpContext *context;
  GtkWidget   *image_view;

  g_return_val_if_fail (GIMP_IS_TOOLBOX (toolbox), NULL);

  context = GIMP_DOCK (toolbox)->context;

  image_view =
    gimp_view_new_full_by_types (GIMP_TYPE_VIEW, GIMP_TYPE_IMAGE,
                                 width, height, 0,
                                 FALSE, TRUE, TRUE);
  gimp_view_set_viewable (GIMP_VIEW (image_view),
                          (GimpViewable *) gimp_context_get_image (context));
  gtk_widget_show (image_view);

  gimp_help_set_help_data (image_view,
                           _("The active image.\n"
                             "Click to open the Image Dialog."), NULL);

  g_signal_connect_object (context, "image_changed",
                           G_CALLBACK (gimp_view_set_viewable),
                           image_view,
                           G_CONNECT_SWAPPED);

  g_signal_connect (image_view, "clicked",
                    G_CALLBACK (image_preview_clicked),
                    toolbox);

  gimp_dnd_viewable_dest_add (image_view,
                              GIMP_TYPE_IMAGE,
                              image_preview_drop_image,
                              context);

  return image_view;
}
