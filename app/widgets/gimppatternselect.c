/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppatternselect.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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
#include "core/gimpparamspecs.h"
#include "core/gimppattern.h"
#include "core/gimptempbuf.h"

#include "pdb/gimppdb.h"

#include "gimpcontainerbox.h"
#include "gimppatternfactoryview.h"
#include "gimppatternselect.h"


static void             gimp_pattern_select_constructed  (GObject        *object);

static GimpValueArray * gimp_pattern_select_run_callback (GimpPdbDialog  *dialog,
                                                          GimpObject     *object,
                                                          gboolean        closing,
                                                          GError        **error);


G_DEFINE_TYPE (GimpPatternSelect, gimp_pattern_select, GIMP_TYPE_PDB_DIALOG)

#define parent_class gimp_pattern_select_parent_class


static void
gimp_pattern_select_class_init (GimpPatternSelectClass *klass)
{
  GObjectClass       *object_class = G_OBJECT_CLASS (klass);
  GimpPdbDialogClass *pdb_class    = GIMP_PDB_DIALOG_CLASS (klass);

  object_class->constructed = gimp_pattern_select_constructed;

  pdb_class->run_callback   = gimp_pattern_select_run_callback;
}

static void
gimp_pattern_select_init (GimpPatternSelect *select)
{
}

static void
gimp_pattern_select_constructed (GObject *object)
{
  GimpPdbDialog *dialog = GIMP_PDB_DIALOG (object);
  GtkWidget     *content_area;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  dialog->view =
    gimp_pattern_factory_view_new (GIMP_VIEW_TYPE_GRID,
                                   dialog->context->gimp->pattern_factory,
                                   dialog->context,
                                   GIMP_VIEW_SIZE_MEDIUM, 1,
                                   dialog->menu_factory);

  gimp_container_box_set_size_request (GIMP_CONTAINER_BOX (GIMP_CONTAINER_EDITOR (dialog->view)->view),
                                       6 * (GIMP_VIEW_SIZE_MEDIUM + 2),
                                       6 * (GIMP_VIEW_SIZE_MEDIUM + 2));

  gtk_container_set_border_width (GTK_CONTAINER (dialog->view), 12);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_box_pack_start (GTK_BOX (content_area), dialog->view, TRUE, TRUE, 0);
  gtk_widget_show (dialog->view);
}

static GimpValueArray *
gimp_pattern_select_run_callback (GimpPdbDialog  *dialog,
                                  GimpObject     *object,
                                  gboolean        closing,
                                  GError        **error)
{
  GimpPattern    *pattern = GIMP_PATTERN (object);
  const Babl     *format;
  gpointer        data;
  GimpArray      *array;
  GimpValueArray *return_vals;

  format = gimp_babl_compat_u8_format (
    gimp_temp_buf_get_format (pattern->mask));
  data   = gimp_temp_buf_lock (pattern->mask, format, GEGL_ACCESS_READ);

  array = gimp_array_new (data,
                          gimp_temp_buf_get_width         (pattern->mask) *
                          gimp_temp_buf_get_height        (pattern->mask) *
                          babl_format_get_bytes_per_pixel (format),
                          TRUE);

  return_vals =
    gimp_pdb_execute_procedure_by_name (dialog->pdb,
                                        dialog->caller_context,
                                        NULL, error,
                                        dialog->callback_name,
                                        G_TYPE_STRING,        gimp_object_get_name (object),
                                        GIMP_TYPE_INT32,      gimp_temp_buf_get_width  (pattern->mask),
                                        GIMP_TYPE_INT32,      gimp_temp_buf_get_height (pattern->mask),
                                        GIMP_TYPE_INT32,      babl_format_get_bytes_per_pixel (gimp_temp_buf_get_format (pattern->mask)),
                                        GIMP_TYPE_INT32,      array->length,
                                        GIMP_TYPE_INT8_ARRAY, array,
                                        GIMP_TYPE_INT32,      closing,
                                        G_TYPE_NONE);

  gimp_array_free (array);

  gimp_temp_buf_unlock (pattern->mask, data);

  return return_vals;
}
