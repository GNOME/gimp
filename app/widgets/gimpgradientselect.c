/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpgradientselect.c
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

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpgradient.h"
#include "core/gimpparamspecs.h"

#include "pdb/gimppdb.h"

#include "gimpcontainerbox.h"
#include "gimpdatafactoryview.h"
#include "gimpgradientselect.h"


enum
{
  PROP_0,
  PROP_SAMPLE_SIZE
};


static void             gimp_gradient_select_constructed  (GObject        *object);
static void             gimp_gradient_select_set_property (GObject        *object,
                                                           guint           property_id,
                                                           const GValue   *value,
                                                           GParamSpec     *pspec);

static GimpValueArray * gimp_gradient_select_run_callback (GimpPdbDialog  *dialog,
                                                           GimpObject     *object,
                                                           gboolean        closing,
                                                           GError        **error);


G_DEFINE_TYPE (GimpGradientSelect, gimp_gradient_select,
               GIMP_TYPE_PDB_DIALOG)

#define parent_class gimp_gradient_select_parent_class


static void
gimp_gradient_select_class_init (GimpGradientSelectClass *klass)
{
  GObjectClass       *object_class = G_OBJECT_CLASS (klass);
  GimpPdbDialogClass *pdb_class    = GIMP_PDB_DIALOG_CLASS (klass);

  object_class->constructed  = gimp_gradient_select_constructed;
  object_class->set_property = gimp_gradient_select_set_property;

  pdb_class->run_callback    = gimp_gradient_select_run_callback;

  g_object_class_install_property (object_class, PROP_SAMPLE_SIZE,
                                   g_param_spec_int ("sample-size", NULL, NULL,
                                                     0, 10000, 84,
                                                     GIMP_PARAM_WRITABLE |
                                                     G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_gradient_select_init (GimpGradientSelect *select)
{
}

static void
gimp_gradient_select_constructed (GObject *object)
{
  GimpPdbDialog *dialog = GIMP_PDB_DIALOG (object);
  GtkWidget     *content_area;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  dialog->view =
    gimp_data_factory_view_new (GIMP_VIEW_TYPE_LIST,
                                dialog->context->gimp->gradient_factory,
                                dialog->context,
                                GIMP_VIEW_SIZE_MEDIUM, 1,
                                dialog->menu_factory, "<Gradients>",
                                "/gradients-popup",
                                "gradients");

  gimp_container_box_set_size_request (GIMP_CONTAINER_BOX (GIMP_CONTAINER_EDITOR (dialog->view)->view),
                                       6 * (GIMP_VIEW_SIZE_MEDIUM + 2),
                                       6 * (GIMP_VIEW_SIZE_MEDIUM + 2));

  gtk_container_set_border_width (GTK_CONTAINER (dialog->view), 12);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_box_pack_start (GTK_BOX (content_area), dialog->view, TRUE, TRUE, 0);
  gtk_widget_show (dialog->view);
}

static void
gimp_gradient_select_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpGradientSelect *select = GIMP_GRADIENT_SELECT (object);

  switch (property_id)
    {
    case PROP_SAMPLE_SIZE:
      select->sample_size = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GimpValueArray *
gimp_gradient_select_run_callback (GimpPdbDialog  *dialog,
                                   GimpObject     *object,
                                   gboolean        closing,
                                   GError        **error)
{
  GimpGradient        *gradient = GIMP_GRADIENT (object);
  GimpGradientSegment *seg      = NULL;
  gdouble             *values, *pv;
  gdouble              pos, delta;
  GimpRGB              color;
  gint                 i;
  GimpArray           *array;
  GimpValueArray      *return_vals;

  i      = GIMP_GRADIENT_SELECT (dialog)->sample_size;
  pos    = 0.0;
  delta  = 1.0 / (i - 1);

  values = g_new (gdouble, 4 * i);
  pv     = values;

  while (i--)
    {
      seg = gimp_gradient_get_color_at (gradient, dialog->caller_context,
                                        seg, pos, FALSE,
                                        GIMP_GRADIENT_BLEND_RGB_PERCEPTUAL,
                                        &color);

      *pv++ = color.r;
      *pv++ = color.g;
      *pv++ = color.b;
      *pv++ = color.a;

      pos += delta;
    }

  array = gimp_array_new ((guint8 *) values,
                          GIMP_GRADIENT_SELECT (dialog)->sample_size * 4 *
                          sizeof (gdouble),
                          TRUE);
  array->static_data = FALSE;

  return_vals =
    gimp_pdb_execute_procedure_by_name (dialog->pdb,
                                        dialog->caller_context,
                                        NULL, error,
                                        dialog->callback_name,
                                        G_TYPE_STRING,         gimp_object_get_name (object),
                                        GIMP_TYPE_INT32,       array->length / sizeof (gdouble),
                                        GIMP_TYPE_FLOAT_ARRAY, array,
                                        GIMP_TYPE_INT32,       closing,
                                        G_TYPE_NONE);

  gimp_array_free (array);

  return return_vals;
}
