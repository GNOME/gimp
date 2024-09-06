/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbrushselect.c
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
#include "core/gimpbrush.h"
#include "core/gimpparamspecs.h"
#include "core/gimptempbuf.h"

#include "pdb/gimppdb.h"

#include "gimpbrushfactoryview.h"
#include "gimpbrushselect.h"
#include "gimpcontainerbox.h"
#include "gimplayermodebox.h"

#include "gimp-intl.h"


static void             gimp_brush_select_constructed  (GObject         *object);

static GimpValueArray * gimp_brush_select_run_callback (GimpPdbDialog   *dialog,
                                                        GimpObject      *object,
                                                        gboolean         closing,
                                                        GError         **error);


G_DEFINE_TYPE (GimpBrushSelect, gimp_brush_select, GIMP_TYPE_PDB_DIALOG)

#define parent_class gimp_brush_select_parent_class


static void
gimp_brush_select_class_init (GimpBrushSelectClass *klass)
{
  GObjectClass       *object_class = G_OBJECT_CLASS (klass);
  GimpPdbDialogClass *pdb_class    = GIMP_PDB_DIALOG_CLASS (klass);

  object_class->constructed  = gimp_brush_select_constructed;

  pdb_class->run_callback    = gimp_brush_select_run_callback;
}

static void
gimp_brush_select_init (GimpBrushSelect *select)
{
}

static void
gimp_brush_select_constructed (GObject *object)
{
  GimpPdbDialog   *dialog = GIMP_PDB_DIALOG (object);
  GtkWidget       *content_area;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  dialog->view =
    gimp_brush_factory_view_new (GIMP_VIEW_TYPE_GRID,
                                 dialog->context->gimp->brush_factory,
                                 dialog->context,
                                 FALSE,
                                 GIMP_VIEW_SIZE_MEDIUM, 1,
                                 dialog->menu_factory);

  gimp_container_box_set_size_request (GIMP_CONTAINER_BOX (GIMP_CONTAINER_EDITOR (dialog->view)->view),
                                       5 * (GIMP_VIEW_SIZE_MEDIUM + 2),
                                       5 * (GIMP_VIEW_SIZE_MEDIUM + 2));

  gtk_container_set_border_width (GTK_CONTAINER (dialog->view), 12);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_box_pack_start (GTK_BOX (content_area), dialog->view, TRUE, TRUE, 0);
  gtk_widget_show (dialog->view);
}

static GimpValueArray *
gimp_brush_select_run_callback (GimpPdbDialog  *dialog,
                                GimpObject     *object,
                                gboolean        closing,
                                GError        **error)
{
  GimpBrush      *brush = GIMP_BRUSH (object);
  GimpTempBuf    *mask  = gimp_brush_get_mask (brush);
  const Babl     *format;
  gpointer        data;
  GBytes         *bytes;
  GimpValueArray *return_vals;

  format = gimp_babl_compat_u8_mask_format (gimp_temp_buf_get_format (mask));
  data   = gimp_temp_buf_lock (mask, format, GEGL_ACCESS_READ);

  bytes = g_bytes_new_static (data,
                              gimp_temp_buf_get_width         (mask) *
                              gimp_temp_buf_get_height        (mask) *
                              babl_format_get_bytes_per_pixel (format));

  return_vals =
    gimp_pdb_execute_procedure_by_name (dialog->pdb,
                                        dialog->caller_context,
                                        NULL, error,
                                        dialog->callback_name,
                                        GIMP_TYPE_RESOURCE,    object,
                                        G_TYPE_INT,            gimp_brush_get_width  (brush),
                                        G_TYPE_INT,            gimp_brush_get_height (brush),
                                        G_TYPE_BYTES,          bytes,
                                        G_TYPE_BOOLEAN,        closing,
                                        G_TYPE_NONE);

  g_bytes_unref (bytes);

  gimp_temp_buf_unlock (mask, data);

  return return_vals;
}
