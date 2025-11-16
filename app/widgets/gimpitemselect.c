/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpitemselect.c
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
#include "core/gimpitem.h"
#include "core/gimpparamspecs.h"
#include "core/gimptempbuf.h"

#include "pdb/gimppdb.h"

#include "gimpitemchooser.h"
#include "gimpitemselect.h"
#include "gimpcontainerbox.h"

#include "gimp-intl.h"


static void             gimp_item_select_constructed     (GObject             *object);

static GimpValueArray * gimp_item_select_run_callback    (GimpPdbDialog       *dialog,
                                                          GimpObject          *object,
                                                          gboolean             closing,
                                                          GError             **error);
static GimpObject     * gimp_item_select_get_object      (GimpPdbDialog       *dialog);
static void             gimp_item_select_set_object      (GimpPdbDialog       *dialog,
                                                          GimpObject          *object);

static void             gimp_item_select_activate        (GimpItemSelect  *select);
static void             gimp_item_select_notify_item     (GimpItemSelect  *select);


G_DEFINE_TYPE (GimpItemSelect, gimp_item_select, GIMP_TYPE_PDB_DIALOG)

#define parent_class gimp_item_select_parent_class


static void
gimp_item_select_class_init (GimpItemSelectClass *klass)
{
  GObjectClass       *object_class = G_OBJECT_CLASS (klass);
  GimpPdbDialogClass *pdb_class    = GIMP_PDB_DIALOG_CLASS (klass);

  object_class->constructed  = gimp_item_select_constructed;

  pdb_class->run_callback    = gimp_item_select_run_callback;
  pdb_class->get_object      = gimp_item_select_get_object;
  pdb_class->set_object      = gimp_item_select_set_object;
}

static void
gimp_item_select_init (GimpItemSelect *select)
{
}

static void
gimp_item_select_constructed (GObject *object)
{
  GimpPdbDialog  *dialog = GIMP_PDB_DIALOG (object);
  GimpItemSelect *select = GIMP_ITEM_SELECT (object);
  GtkWidget      *content_area;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  select->chooser = gimp_item_chooser_new (dialog->context, dialog->select_type,
                                           GIMP_VIEW_SIZE_LARGE, 1);
  gimp_item_chooser_set_item (GIMP_ITEM_CHOOSER (select->chooser),
                              GIMP_ITEM (dialog->initial_object));
  g_signal_connect_swapped (select->chooser, "notify::item",
                            G_CALLBACK (gimp_item_select_notify_item),
                            select);
  g_signal_connect_swapped (select->chooser, "activate",
                            G_CALLBACK (gimp_item_select_activate),
                            select);
  gtk_box_pack_start (GTK_BOX (content_area), select->chooser, TRUE, TRUE, 0);
  gtk_widget_set_visible (select->chooser, TRUE);
}

static GimpValueArray *
gimp_item_select_run_callback (GimpPdbDialog  *dialog,
                               GimpObject     *object,
                               gboolean        closing,
                               GError        **error)
{
  GimpItem       *item = GIMP_ITEM (object);
  GimpValueArray *return_vals;

  return_vals =
    gimp_pdb_execute_procedure_by_name (dialog->pdb,
                                        dialog->caller_context,
                                        NULL, error,
                                        dialog->callback_name,
                                        GIMP_TYPE_ITEM, item,
                                        G_TYPE_BOOLEAN, closing,
                                        G_TYPE_NONE);

  return return_vals;
}

static GimpObject *
gimp_item_select_get_object (GimpPdbDialog *dialog)
{
  GimpItemSelect *select = GIMP_ITEM_SELECT (dialog);

  return (GimpObject *) gimp_item_chooser_get_item (GIMP_ITEM_CHOOSER (select->chooser));
}

static void
gimp_item_select_set_object (GimpPdbDialog *dialog,
                                 GimpObject    *object)
{
  GimpItemSelect *select = GIMP_ITEM_SELECT (dialog);

  gimp_item_chooser_set_item (GIMP_ITEM_CHOOSER (select->chooser), GIMP_ITEM (object));
}

static void
gimp_item_select_activate (GimpItemSelect *select)
{
  gimp_pdb_dialog_run_callback ((GimpPdbDialog **) &select, TRUE);
  gtk_widget_destroy (GTK_WIDGET (select));
}

static void
gimp_item_select_notify_item (GimpItemSelect *select)
{
  GtkWidget       *button;
  GimpItemChooser *chooser = GIMP_ITEM_CHOOSER (select->chooser);

  button = gtk_dialog_get_widget_for_response (GTK_DIALOG (select),
                                               GTK_RESPONSE_OK);

  /* Clicking Okay when an image but not an item has been
   * selected causes the widget to "lock up" and not reappear when you
   * click on it again. For now, we'll disable the Okay button until
   * an item is selected */
  if (GIMP_IS_IMAGE (gimp_item_chooser_get_item (chooser)))
    gtk_widget_set_sensitive (button, FALSE);
  else
    gtk_widget_set_sensitive (button, TRUE);

  gimp_pdb_dialog_run_callback ((GimpPdbDialog **) &select, FALSE);
}
