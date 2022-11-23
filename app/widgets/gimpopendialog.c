/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaopendialog.c
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmaimage.h"

#include "ligmahelp-ids.h"
#include "ligmaopendialog.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void   ligma_open_dialog_dispose (GObject *object);


G_DEFINE_TYPE (LigmaOpenDialog, ligma_open_dialog,
               LIGMA_TYPE_FILE_DIALOG)

#define parent_class ligma_open_dialog_parent_class


/*  private functions  */

static void
ligma_open_dialog_class_init (LigmaOpenDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = ligma_open_dialog_dispose;
}

static void
ligma_open_dialog_init (LigmaOpenDialog *dialog)
{
}

static void
ligma_open_dialog_dispose (GObject *object)
{
  ligma_open_dialog_set_image (LIGMA_OPEN_DIALOG (object), NULL, FALSE);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}


/*  public functions  */

GtkWidget *
ligma_open_dialog_new (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  return g_object_new (LIGMA_TYPE_OPEN_DIALOG,
                       "ligma",                  ligma,
                       "title",                 _("Open Image"),
                       "role",                  "ligma-file-open",
                       "help-id",               LIGMA_HELP_FILE_OPEN,
                       "ok-button-label",       _("_Open"),

                       "automatic-label",       _("Automatically Detected"),
                       "automatic-help-id",     LIGMA_HELP_FILE_OPEN_BY_EXTENSION,

                       "action",                GTK_FILE_CHOOSER_ACTION_OPEN,
                       "file-procs",            LIGMA_FILE_PROCEDURE_GROUP_OPEN,
                       "file-procs-all-images", LIGMA_FILE_PROCEDURE_GROUP_NONE,
                       "file-filter-label",     NULL,
                       NULL);
}

void
ligma_open_dialog_set_image (LigmaOpenDialog *dialog,
                            LigmaImage      *image,
                            gboolean        open_as_layers)
{
  LigmaFileDialog *file_dialog;

  g_return_if_fail (LIGMA_IS_OPEN_DIALOG (dialog));
  g_return_if_fail (image == NULL || LIGMA_IS_IMAGE (image));

  file_dialog = LIGMA_FILE_DIALOG (dialog);

  if (file_dialog->image)
    {
      g_object_remove_weak_pointer (G_OBJECT (file_dialog->image),
                                    (gpointer *) &file_dialog->image);
    }

  file_dialog->image     = image;
  dialog->open_as_layers = open_as_layers;

  if (file_dialog->image)
    {
      g_object_add_weak_pointer (G_OBJECT (file_dialog->image),
                                 (gpointer *) &file_dialog->image);
    }
}
