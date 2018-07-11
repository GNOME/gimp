/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpextensiondetails.c
 * Copyright (C) 2018 Jehan <jehan@gimp.org>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpextension.h"
#include "core/gimpextensionmanager.h"

#include "gimpextensiondetails.h"

#include "gimp-intl.h"

struct _GimpExtensionDetailsPrivate
{
  GimpExtension *extension;
};

G_DEFINE_TYPE (GimpExtensionDetails, gimp_extension_details, GTK_TYPE_FRAME)

#define parent_class gimp_extension_details_parent_class


static void
gimp_extension_details_class_init (GimpExtensionDetailsClass *klass)
{
  g_type_class_add_private (klass, sizeof (GimpExtensionDetailsPrivate));
}

static void
gimp_extension_details_init (GimpExtensionDetails *details)
{
  gtk_frame_set_label_align (GTK_FRAME (details), 0.5, 1.0);
  details->p = G_TYPE_INSTANCE_GET_PRIVATE (details,
                                            GIMP_TYPE_EXTENSION_DETAILS,
                                            GimpExtensionDetailsPrivate);
}

GtkWidget *
gimp_extension_details_new (void)
{
  return g_object_new (GIMP_TYPE_EXTENSION_DETAILS, NULL);
}

void
gimp_extension_details_set (GimpExtensionDetails *details,
                            GimpExtension        *extension)
{
  g_return_if_fail (GIMP_IS_EXTENSION (extension));

  if (details->p->extension)
    g_object_unref (details->p->extension);
  details->p->extension  = g_object_ref (extension);

  gtk_container_foreach (GTK_CONTAINER (details),
                         (GtkCallback) gtk_widget_destroy,
                         NULL);

  gtk_frame_set_label (GTK_FRAME (details),
                       extension ? gimp_extension_get_name (extension) : NULL);
}
