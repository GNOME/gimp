/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmagradientselect.c
 * Copyright (C) 2004 Michael Natterer <mitch@ligma.org>
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
#include "core/ligmacontext.h"
#include "core/ligmagradient.h"
#include "core/ligmaparamspecs.h"

#include "pdb/ligmapdb.h"

#include "ligmacontainerbox.h"
#include "ligmadatafactoryview.h"
#include "ligmagradientselect.h"


enum
{
  PROP_0,
  PROP_SAMPLE_SIZE
};


static void             ligma_gradient_select_constructed  (GObject        *object);
static void             ligma_gradient_select_set_property (GObject        *object,
                                                           guint           property_id,
                                                           const GValue   *value,
                                                           GParamSpec     *pspec);

static LigmaValueArray * ligma_gradient_select_run_callback (LigmaPdbDialog  *dialog,
                                                           LigmaObject     *object,
                                                           gboolean        closing,
                                                           GError        **error);


G_DEFINE_TYPE (LigmaGradientSelect, ligma_gradient_select,
               LIGMA_TYPE_PDB_DIALOG)

#define parent_class ligma_gradient_select_parent_class


static void
ligma_gradient_select_class_init (LigmaGradientSelectClass *klass)
{
  GObjectClass       *object_class = G_OBJECT_CLASS (klass);
  LigmaPdbDialogClass *pdb_class    = LIGMA_PDB_DIALOG_CLASS (klass);

  object_class->constructed  = ligma_gradient_select_constructed;
  object_class->set_property = ligma_gradient_select_set_property;

  pdb_class->run_callback    = ligma_gradient_select_run_callback;

  g_object_class_install_property (object_class, PROP_SAMPLE_SIZE,
                                   g_param_spec_int ("sample-size", NULL, NULL,
                                                     0, 10000, 84,
                                                     LIGMA_PARAM_WRITABLE |
                                                     G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_gradient_select_init (LigmaGradientSelect *select)
{
}

static void
ligma_gradient_select_constructed (GObject *object)
{
  LigmaPdbDialog *dialog = LIGMA_PDB_DIALOG (object);
  GtkWidget     *content_area;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  dialog->view =
    ligma_data_factory_view_new (LIGMA_VIEW_TYPE_LIST,
                                dialog->context->ligma->gradient_factory,
                                dialog->context,
                                LIGMA_VIEW_SIZE_MEDIUM, 1,
                                dialog->menu_factory, "<Gradients>",
                                "/gradients-popup",
                                "gradients");

  ligma_container_box_set_size_request (LIGMA_CONTAINER_BOX (LIGMA_CONTAINER_EDITOR (dialog->view)->view),
                                       6 * (LIGMA_VIEW_SIZE_MEDIUM + 2),
                                       6 * (LIGMA_VIEW_SIZE_MEDIUM + 2));

  gtk_container_set_border_width (GTK_CONTAINER (dialog->view), 12);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_box_pack_start (GTK_BOX (content_area), dialog->view, TRUE, TRUE, 0);
  gtk_widget_show (dialog->view);
}

static void
ligma_gradient_select_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  LigmaGradientSelect *select = LIGMA_GRADIENT_SELECT (object);

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

static LigmaValueArray *
ligma_gradient_select_run_callback (LigmaPdbDialog  *dialog,
                                   LigmaObject     *object,
                                   gboolean        closing,
                                   GError        **error)
{
  LigmaGradient        *gradient = LIGMA_GRADIENT (object);
  LigmaGradientSegment *seg      = NULL;
  gdouble             *values, *pv;
  gdouble              pos, delta;
  LigmaRGB              color;
  gint                 i;
  LigmaArray           *array;
  LigmaValueArray      *return_vals;

  i      = LIGMA_GRADIENT_SELECT (dialog)->sample_size;
  pos    = 0.0;
  delta  = 1.0 / (i - 1);

  values = g_new (gdouble, 4 * i);
  pv     = values;

  while (i--)
    {
      seg = ligma_gradient_get_color_at (gradient, dialog->caller_context,
                                        seg, pos, FALSE,
                                        LIGMA_GRADIENT_BLEND_RGB_PERCEPTUAL,
                                        &color);

      *pv++ = color.r;
      *pv++ = color.g;
      *pv++ = color.b;
      *pv++ = color.a;

      pos += delta;
    }

  array = ligma_array_new ((guint8 *) values,
                          LIGMA_GRADIENT_SELECT (dialog)->sample_size * 4 *
                          sizeof (gdouble),
                          TRUE);
  array->static_data = FALSE;

  return_vals =
    ligma_pdb_execute_procedure_by_name (dialog->pdb,
                                        dialog->caller_context,
                                        NULL, error,
                                        dialog->callback_name,
                                        G_TYPE_STRING,         ligma_object_get_name (object),
                                        G_TYPE_INT,            array->length / sizeof (gdouble),
                                        LIGMA_TYPE_FLOAT_ARRAY, array->data,
                                        G_TYPE_BOOLEAN,        closing,
                                        G_TYPE_NONE);

  ligma_array_free (array);

  return return_vals;
}
