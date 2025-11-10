/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimageselect.c
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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gegl/gimp-babl-compat.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpparamspecs.h"
#include "core/gimptempbuf.h"

#include "pdb/gimppdb.h"

#include "gimpcontainerbox.h"
#include "gimpcontainerview.h"
#include "gimpimagechooser.h"
#include "gimpimageselect.h"
#include "gimplayermodebox.h"

#include "gimp-intl.h"


static void             gimp_image_select_constructed     (GObject          *object);

static GimpValueArray * gimp_image_select_run_callback    (GimpPdbDialog    *dialog,
                                                           GimpObject       *object,
                                                           gboolean          closing,
                                                           GError          **error);
static GimpObject     * gimp_image_select_get_object      (GimpPdbDialog     *dialog);
static void             gimp_image_select_set_object      (GimpPdbDialog     *dialog,
                                                           GimpObject        *object);


static void             gimp_image_select_activate        (GimpImageSelect   *select);
static void             gimp_image_select_notify_image    (GimpImageSelect   *select);

G_DEFINE_TYPE (GimpImageSelect, gimp_image_select, GIMP_TYPE_PDB_DIALOG)

#define parent_class gimp_image_select_parent_class


static void
gimp_image_select_class_init (GimpImageSelectClass *klass)
{
  GObjectClass       *object_class = G_OBJECT_CLASS (klass);
  GimpPdbDialogClass *pdb_class    = GIMP_PDB_DIALOG_CLASS (klass);

  object_class->constructed = gimp_image_select_constructed;

  pdb_class->run_callback   = gimp_image_select_run_callback;
  pdb_class->set_object     = gimp_image_select_set_object;
  pdb_class->get_object     = gimp_image_select_get_object;
}

static void
gimp_image_select_init (GimpImageSelect *select)
{
}

static void
gimp_image_select_constructed (GObject *object)
{
  GimpPdbDialog   *dialog = GIMP_PDB_DIALOG (object);
  GimpImageSelect *select = GIMP_IMAGE_SELECT (object);
  GtkWidget       *content_area;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  select->chooser = gimp_image_chooser_new (dialog->context,
                                            GIMP_VIEW_SIZE_LARGE, 1);
  gimp_image_chooser_set_image (GIMP_IMAGE_CHOOSER (select->chooser),
                                GIMP_IMAGE (dialog->initial_object));
  g_signal_connect_swapped (select->chooser, "notify::image",
                            G_CALLBACK (gimp_image_select_notify_image),
                            select);
  g_signal_connect_swapped (select->chooser, "activate",
                            G_CALLBACK (gimp_image_select_activate),
                            select);

  gtk_box_pack_start (GTK_BOX (content_area), select->chooser, TRUE, TRUE, 0);
  gtk_widget_set_visible (select->chooser, TRUE);
}

static GimpValueArray *
gimp_image_select_run_callback (GimpPdbDialog  *dialog,
                                GimpObject     *object,
                                gboolean        closing,
                                GError        **error)
{
  GimpImage      *image = GIMP_IMAGE (object);
  GimpValueArray *return_vals;

  return_vals =
    gimp_pdb_execute_procedure_by_name (dialog->pdb,
                                        dialog->caller_context,
                                        NULL, error,
                                        dialog->callback_name,
                                        GIMP_TYPE_IMAGE, image,
                                        G_TYPE_BOOLEAN,  closing,
                                        G_TYPE_NONE);

  return return_vals;
}

static GimpObject *
gimp_image_select_get_object (GimpPdbDialog *dialog)
{
  GimpImageSelect *select = GIMP_IMAGE_SELECT (dialog);

  return (GimpObject *) gimp_image_chooser_get_image (GIMP_IMAGE_CHOOSER (select->chooser));

}

static void
gimp_image_select_set_object (GimpPdbDialog *dialog,
                              GimpObject    *object)
{
  GimpImageSelect *select = GIMP_IMAGE_SELECT (dialog);

  gimp_image_chooser_set_image (GIMP_IMAGE_CHOOSER (select->chooser), GIMP_IMAGE (object));
}


static void
gimp_image_select_activate (GimpImageSelect *select)
{
  gimp_pdb_dialog_run_callback ((GimpPdbDialog **) &select, TRUE);
  gtk_widget_destroy (GTK_WIDGET (select));
}

static void
gimp_image_select_notify_image (GimpImageSelect *select)
{
  gimp_pdb_dialog_run_callback ((GimpPdbDialog **) &select, FALSE);
}
