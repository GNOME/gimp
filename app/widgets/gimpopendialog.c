/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpopendialog.c
 * Copyright (C) 2015 Jehan <jehan@girinstud.io>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"

#include "plug-in/gimppluginmanager.h"

#include "gimpfiledialog.h"
#include "gimphelp-ids.h"
#include "gimpopendialog.h"

#include "gimp-intl.h"


G_DEFINE_TYPE (GimpOpenDialog, gimp_open_dialog,
               GIMP_TYPE_FILE_DIALOG)

#define parent_class gimp_open_dialog_parent_class


static void
gimp_open_dialog_class_init (GimpOpenDialogClass *klass)
{
}

static void
gimp_open_dialog_init (GimpOpenDialog *dialog)
{
}


/*  public functions  */

GtkWidget *
gimp_open_dialog_new (Gimp *gimp)
{
  GimpOpenDialog *dialog;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  dialog = g_object_new (GIMP_TYPE_OPEN_DIALOG,
                         "gimp",                      gimp,
                         "title",                     _("Open Image"),
                         "role",                      "gimp-file-open",
                         "help-id",                   GIMP_HELP_FILE_OPEN,
                         "stock-id",                  GTK_STOCK_OPEN,

                         "automatic-label",           _("Automatically Detected"),
                         "automatic-help-id",         GIMP_HELP_FILE_OPEN_BY_EXTENSION,

                         "action",                    GTK_FILE_CHOOSER_ACTION_OPEN,
                         "file-procs",                gimp->plug_in_manager->load_procs,
                         "file-procs-all-images",     NULL,
                         "file-filter-label",         NULL,
                         "local-only",                FALSE,
                         "do-overwrite-confirmation", TRUE,
                         NULL);

  return GTK_WIDGET (dialog);
}

void
gimp_open_dialog_set_image (GimpOpenDialog *dialog,
                            GimpImage      *image,
                            gboolean        open_as_layers)
{
  g_return_if_fail (GIMP_IS_OPEN_DIALOG (dialog));
  g_return_if_fail (image == NULL || GIMP_IS_IMAGE (image));

  GIMP_FILE_DIALOG (dialog)->image = image;
  dialog->open_as_layers           = open_as_layers;
}
