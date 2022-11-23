/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * ligmadeviceinfoeditor.c
 * Copyright (C) 2010 Michael Natterer <mitch@ligma.org>
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

#include "core/ligmacurve.h"

#include "ligmacurveview.h"
#include "ligmadeviceinfo.h"
#include "ligmadeviceinfoeditor.h"

#include "ligma-intl.h"


#define CURVE_SIZE   256
#define CURVE_BORDER   4


enum
{
  PROP_0,
  PROP_INFO
};

enum
{
  AXIS_COLUMN_INDEX,
  AXIS_COLUMN_NAME,
  AXIS_COLUMN_INPUT_NAME,
  AXIS_N_COLUMNS
};

enum
{
  INPUT_COLUMN_INDEX,
  INPUT_COLUMN_NAME,
  INPUT_N_COLUMNS
};

enum
{
  KEY_COLUMN_INDEX,
  KEY_COLUMN_NAME,
  KEY_COLUMN_KEY,
  KEY_COLUMN_MASK,
  KEY_N_COLUMNS
};


typedef struct _LigmaDeviceInfoEditorPrivate LigmaDeviceInfoEditorPrivate;

struct _LigmaDeviceInfoEditorPrivate
{
  LigmaDeviceInfo *info;

  GtkWidget      *vbox;

  GtkListStore   *input_store;

  GtkListStore   *axis_store;

  GtkWidget      *notebook;
};

#define LIGMA_DEVICE_INFO_EDITOR_GET_PRIVATE(editor) \
        ((LigmaDeviceInfoEditorPrivate *) ligma_device_info_editor_get_instance_private ((LigmaDeviceInfoEditor *) (editor)))


static void   ligma_device_info_editor_constructed   (GObject              *object);
static void   ligma_device_info_editor_finalize      (GObject              *object);
static void   ligma_device_info_editor_set_property  (GObject              *object,
                                                     guint                 property_id,
                                                     const GValue         *value,
                                                     GParamSpec           *pspec);
static void   ligma_device_info_editor_get_property  (GObject              *object,
                                                     guint                 property_id,
                                                     GValue               *value,
                                                     GParamSpec           *pspec);

static void   ligma_device_info_editor_set_axes      (LigmaDeviceInfoEditor *editor);

static void   ligma_device_info_editor_axis_changed  (GtkCellRendererCombo *combo,
                                                     const gchar          *path_string,
                                                     GtkTreeIter          *new_iter,
                                                     LigmaDeviceInfoEditor *editor);
static void   ligma_device_info_editor_axis_selected (GtkTreeSelection     *selection,
                                                     LigmaDeviceInfoEditor *editor);

static void   ligma_device_info_editor_curve_reset   (GtkWidget            *button,
                                                     LigmaCurve            *curve);

static gboolean ligma_device_info_editor_foreach     (GtkTreeModel         *model,
                                                     GtkTreePath          *path,
                                                     GtkTreeIter          *iter,
                                                     gpointer              data);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaDeviceInfoEditor, ligma_device_info_editor,
                            GTK_TYPE_BOX)

#define parent_class ligma_device_info_editor_parent_class


static void
ligma_device_info_editor_class_init (LigmaDeviceInfoEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = ligma_device_info_editor_constructed;
  object_class->finalize     = ligma_device_info_editor_finalize;
  object_class->set_property = ligma_device_info_editor_set_property;
  object_class->get_property = ligma_device_info_editor_get_property;

  g_object_class_install_property (object_class, PROP_INFO,
                                   g_param_spec_object ("info",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_DEVICE_INFO,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_device_info_editor_init (LigmaDeviceInfoEditor *editor)
{
  LigmaDeviceInfoEditorPrivate *private;

  private = LIGMA_DEVICE_INFO_EDITOR_GET_PRIVATE (editor);

  gtk_orientable_set_orientation (GTK_ORIENTABLE (editor),
                                  GTK_ORIENTATION_HORIZONTAL);

  gtk_box_set_spacing (GTK_BOX (editor), 12);

  private->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_box_pack_start (GTK_BOX (editor), private->vbox, TRUE, TRUE, 0);
  gtk_widget_show (private->vbox);

  private->axis_store = gtk_list_store_new (AXIS_N_COLUMNS,
                                            /* Axis index. */
                                            G_TYPE_INT,
                                            /* Axis name.  */
                                            G_TYPE_STRING,
                                            /* Input name. */
                                            G_TYPE_STRING);

}

static void
ligma_device_info_editor_constructed (GObject *object)
{
  LigmaDeviceInfoEditor        *editor  = LIGMA_DEVICE_INFO_EDITOR (object);
  LigmaDeviceInfoEditorPrivate *private;
  GtkWidget                   *frame;
  GtkWidget                   *frame2;
  GtkWidget                   *grid;
  GtkWidget                   *label;
  GtkWidget                   *combo;
  GtkWidget                   *view;
  GtkTreeSelection            *sel;
  GtkCellRenderer             *cell;
  GtkTreeIter                  axis_iter;
  gboolean                     has_axes = FALSE;

  gint                         n_axes;
  gint                         i;

  private = LIGMA_DEVICE_INFO_EDITOR_GET_PRIVATE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_DEVICE_INFO (private->info));

  /*  the axes  */

  n_axes = ligma_device_info_get_n_axes (private->info);
  for (i = 0; i < n_axes; i++)
    {
      const gchar *axis_name;
      GdkAxisUse   use       = i + 1;
      gboolean     has_curve = FALSE;

      if (ligma_device_info_get_device (private->info, NULL))
        {
          if (ligma_device_info_ignore_axis (private->info, i))
            /* Some axis are apparently returned by the driver, yet
             * should be ignored. We just pass these.
             */
            continue;

          use = ligma_device_info_get_axis_use (private->info, i);
          has_curve = (ligma_device_info_get_curve (private->info, use) != NULL);
        }

      axis_name = ligma_device_info_get_axis_name (private->info, i);
      gtk_list_store_insert_with_values (private->axis_store,
                                         /* Set the initially selected
                                          * axis to an axis with curve
                                          * if available, or the first
                                          * row otherwise.
                                          */
                                         (! has_axes || has_curve) ? &axis_iter : NULL,
                                         -1,
                                         AXIS_COLUMN_INDEX, i,
                                         AXIS_COLUMN_NAME,  gettext (axis_name),
                                         -1);
      has_axes = TRUE;
    }

  if (has_axes)
    {
      /* The list of axes of an input device */

      frame = ligma_frame_new (_("Axes"));
      gtk_box_pack_start (GTK_BOX (private->vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      frame2 = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame2), GTK_SHADOW_IN);
      gtk_container_add (GTK_CONTAINER (frame), frame2);
      gtk_widget_show (frame2);

      view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (private->axis_store));
      g_object_unref (private->axis_store);

      gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);

      gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
                                                   -1, NULL,
                                                   gtk_cell_renderer_text_new (),
                                                   "text", AXIS_COLUMN_NAME,
                                                   NULL);

      private->input_store = gtk_list_store_new (INPUT_N_COLUMNS,
                                                 /* Axis real index.    */
                                                 G_TYPE_INT,
                                                 /* Axis printed index. */
                                                 G_TYPE_STRING);

      cell = gtk_cell_renderer_combo_new ();
      g_object_set (cell,
                    "mode",        GTK_CELL_RENDERER_MODE_EDITABLE,
                    "editable",    TRUE,
                    "model",       private->input_store,
                    "text-column", INPUT_COLUMN_NAME,
                    "has-entry",   FALSE,
                    NULL);

      g_object_unref (private->input_store);

      g_signal_connect (cell, "changed",
                        G_CALLBACK (ligma_device_info_editor_axis_changed),
                        editor);

      gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
                                                   -1, NULL,
                                                   cell,
                                                   "text", AXIS_COLUMN_INPUT_NAME,
                                                   NULL);

      gtk_container_add (GTK_CONTAINER (frame2), view);
      gtk_widget_show (view);

      sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
      gtk_tree_selection_set_mode (sel, GTK_SELECTION_BROWSE);
    }
  else
    {
      g_clear_object (&private->axis_store);
    }

  /*  general device information  */

  frame = ligma_frame_new (_("General"));
  gtk_box_pack_start (GTK_BOX (private->vbox), frame, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (private->vbox), frame, 0);
  gtk_widget_show (frame);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 4);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  combo = ligma_prop_enum_combo_box_new (G_OBJECT (private->info), "mode",
                                        0, 0);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("_Mode:"), 0.0, 0.5,
                            combo, 1);

  label = ligma_prop_enum_label_new (G_OBJECT (private->info), "source");
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("Source:"), 0.0, 0.5,
                            label, 1);

  label = ligma_prop_label_new (G_OBJECT (private->info), "vendor-id");
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 2,
                            _("Vendor ID:"), 0.0, 0.5,
                            label, 1);

  label = ligma_prop_label_new (G_OBJECT (private->info), "product-id");
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 3,
                            _("Product ID:"), 0.0, 0.5,
                            label, 1);

  label = ligma_prop_enum_label_new (G_OBJECT (private->info), "tool-type");
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 4,
                            _("Tool type:"), 0.0, 0.5,
                            label, 1);

  label = ligma_prop_label_new (G_OBJECT (private->info), "tool-serial");
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 5,
                            _("Tool serial:"), 0.0, 0.5,
                            label, 1);

  label = ligma_prop_label_new (G_OBJECT (private->info), "tool-hardware-id");
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 6,
                            _("Tool hardware ID:"), 0.0, 0.5,
                            label, 1);

  if (has_axes)
    {
      /*  the axes  */

      for (i = -1; i < n_axes; i++)
        {
          gchar name[16];

          if (i == -1)
            g_snprintf (name, sizeof (name), _("none"));
          else
            g_snprintf (name, sizeof (name), "%d", i + 1);

          gtk_list_store_insert_with_values (private->input_store, NULL, -1,
                                             INPUT_COLUMN_INDEX, i,
                                             INPUT_COLUMN_NAME,  name,
                                             -1);
        }

      ligma_device_info_editor_set_axes (editor);

      /*  the curves  */

      private->notebook = gtk_notebook_new ();
      gtk_notebook_set_show_border (GTK_NOTEBOOK (private->notebook), FALSE);
      gtk_notebook_set_show_tabs (GTK_NOTEBOOK (private->notebook), FALSE);
      gtk_box_pack_start (GTK_BOX (editor), private->notebook, TRUE, TRUE, 0);
      gtk_widget_show (private->notebook);

      for (i = GDK_AXIS_X; i < GDK_AXIS_LAST; i++)
        {
          LigmaCurve *curve;
          gchar     *title;

          /* e.g. "Pressure Curve" for mapping input device axes */
          title = g_strdup_printf (_("%s Curve"),
                                   gettext (ligma_device_info_get_axis_name (private->info, i - 1)));

          frame = ligma_frame_new (title);
          gtk_notebook_append_page (GTK_NOTEBOOK (private->notebook), frame, NULL);
          gtk_widget_show (frame);

          g_free (title);

          curve = ligma_device_info_get_curve (private->info, i);

          if (curve)
            {
              GtkWidget *vbox;
              GtkWidget *hbox;
              GtkWidget *view;
              GtkWidget *label;
              GtkWidget *combo;
              GtkWidget *button;

              vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
              gtk_box_set_spacing (GTK_BOX (vbox), 6);
              gtk_container_add (GTK_CONTAINER (frame), vbox);
              gtk_widget_show (vbox);

              frame = gtk_frame_new (NULL);
              gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
              gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
              gtk_widget_show (frame);

              view = ligma_curve_view_new ();
              g_object_set (view,
                            "ligma",         LIGMA_TOOL_PRESET (private->info)->ligma,
                            "border-width", CURVE_BORDER,
                            NULL);
              gtk_widget_set_size_request (view,
                                           CURVE_SIZE + CURVE_BORDER * 2,
                                           CURVE_SIZE + CURVE_BORDER * 2);
              gtk_container_add (GTK_CONTAINER (frame), view);
              gtk_widget_show (view);

              ligma_curve_view_set_curve (LIGMA_CURVE_VIEW (view), curve, NULL);

              hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
              gtk_box_set_spacing (GTK_BOX (hbox), 6);
              gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
              gtk_widget_show (hbox);

              label = gtk_label_new_with_mnemonic (_("Curve _type:"));
              gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
              gtk_widget_show (label);

              combo = ligma_prop_enum_combo_box_new (G_OBJECT (curve),
                                                    "curve-type", 0, 0);
              ligma_enum_combo_box_set_icon_prefix (LIGMA_ENUM_COMBO_BOX (combo),
                                                   "ligma-curve");
              gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);

              gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

              button = gtk_button_new_with_mnemonic (_("_Reset Curve"));
              gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
              gtk_widget_show (button);

              g_signal_connect (button, "clicked",
                                G_CALLBACK (ligma_device_info_editor_curve_reset),
                                curve);
            }
          else
            {
              GtkWidget *label;
              gchar     *string;

              string = g_strdup_printf (_("The axis '%s' has no curve"),
                                        gettext (ligma_device_info_get_axis_name (private->info, i - 1)));

              label = gtk_label_new (string);
              gtk_container_add (GTK_CONTAINER (frame), label);
              gtk_widget_show (label);

              g_free (string);
            }
        }

      /* Callback when selecting an axis. */

      g_signal_connect (sel, "changed",
                        G_CALLBACK (ligma_device_info_editor_axis_selected),
                        editor);

      if (has_axes)
        gtk_tree_selection_select_iter (sel, &axis_iter);
    }
}

static void
ligma_device_info_editor_finalize (GObject *object)
{
  LigmaDeviceInfoEditorPrivate *private;

  private = LIGMA_DEVICE_INFO_EDITOR_GET_PRIVATE (object);

  g_clear_object (&private->info);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_device_info_editor_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  LigmaDeviceInfoEditorPrivate *private;

  private = LIGMA_DEVICE_INFO_EDITOR_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_INFO:
      private->info = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_device_info_editor_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  LigmaDeviceInfoEditorPrivate *private;

  private = LIGMA_DEVICE_INFO_EDITOR_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_INFO:
      g_value_set_object (value, private->info);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_device_info_editor_set_axes (LigmaDeviceInfoEditor *editor)
{
  LigmaDeviceInfoEditorPrivate *private;

  private = LIGMA_DEVICE_INFO_EDITOR_GET_PRIVATE (editor);

  gtk_tree_model_foreach (GTK_TREE_MODEL (private->axis_store),
                          ligma_device_info_editor_foreach, private);
}

static void
ligma_device_info_editor_axis_changed (GtkCellRendererCombo  *combo,
                                      const gchar           *path_string,
                                      GtkTreeIter           *new_iter,
                                      LigmaDeviceInfoEditor  *editor)
{
  LigmaDeviceInfoEditorPrivate *private;
  GtkTreePath                 *path;
  GtkTreeIter                  new_use_iter;

  private = LIGMA_DEVICE_INFO_EDITOR_GET_PRIVATE (editor);

  path = gtk_tree_path_new_from_string (path_string);

  if (gtk_tree_model_get_iter (GTK_TREE_MODEL (private->axis_store),
                               &new_use_iter, path))
    {
      gint       axis_index;
      GdkAxisUse new_use  = GDK_AXIS_IGNORE;
      GdkAxisUse old_use  = GDK_AXIS_IGNORE;
      gint       new_axis = -1;
      gint       old_axis = -1;
      gint       n_axes;
      gint       i;

      gtk_tree_model_get (GTK_TREE_MODEL (private->axis_store), &new_use_iter,
                          AXIS_COLUMN_INDEX, &axis_index,
                          -1);
      new_use = axis_index + 1;

      gtk_tree_model_get (GTK_TREE_MODEL (private->input_store), new_iter,
                          INPUT_COLUMN_INDEX, &new_axis,
                          -1);

      n_axes = ligma_device_info_get_n_axes (private->info);

      for (i = 0; i < n_axes; i++)
        if (ligma_device_info_get_axis_use (private->info, i) == new_use)
          {
            old_axis = i;
            break;
          }

      if (new_axis == old_axis)
        goto out;

      if (new_axis != -1)
        old_use = ligma_device_info_get_axis_use (private->info, new_axis);

      /* we must always have an x and a y axis */
      if ((new_axis == -1 && (new_use == GDK_AXIS_X ||
                              new_use == GDK_AXIS_Y)) ||
          (old_axis == -1 && (old_use == GDK_AXIS_X ||
                              old_use == GDK_AXIS_Y)))
        {
          /* do nothing */
        }
      else
        {
          if (new_axis != -1)
            ligma_device_info_set_axis_use (private->info, new_axis, new_use);

          if (old_axis != -1)
            ligma_device_info_set_axis_use (private->info, old_axis, old_use);

          ligma_device_info_editor_set_axes (editor);
        }
    }

 out:
  gtk_tree_path_free (path);
}

static void
ligma_device_info_editor_axis_selected (GtkTreeSelection     *selection,
                                       LigmaDeviceInfoEditor *editor)
{
  LigmaDeviceInfoEditorPrivate *private;
  GtkTreeIter                  iter;

  private = LIGMA_DEVICE_INFO_EDITOR_GET_PRIVATE (editor);

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      gint axis_index;

      gtk_tree_model_get (GTK_TREE_MODEL (private->axis_store), &iter,
                          AXIS_COLUMN_INDEX, &axis_index,
                          -1);

      gtk_notebook_set_current_page (GTK_NOTEBOOK (private->notebook),
                                     axis_index);
    }
}

static void
ligma_device_info_editor_curve_reset (GtkWidget *button,
                                     LigmaCurve *curve)
{
  ligma_curve_reset (curve, TRUE);
}

static gboolean
ligma_device_info_editor_foreach (GtkTreeModel *model,
                                 GtkTreePath *path,
                                 GtkTreeIter *iter,
                                 gpointer data)
{
  LigmaDeviceInfoEditorPrivate *private = data;
  gchar                        input_name[16];
  gint                         n_axes;
  gint                         axe_index;
  gint                         i;

  n_axes = ligma_device_info_get_n_axes (private->info);

  gtk_tree_model_get (model, iter,
                      AXIS_COLUMN_INDEX, &axe_index,
                      -1);

  for (i = 0; i < n_axes; i++)
    {
      if (ligma_device_info_get_axis_use (private->info, i) == axe_index + 1)
        break;
    }

  if (i == n_axes)
    i = -1;

  if (i == -1)
    g_snprintf (input_name, sizeof (input_name), _("none"));
  else
    g_snprintf (input_name, sizeof (input_name), "%d", i + 1);

  gtk_list_store_set (private->axis_store,
                      iter,
                      AXIS_COLUMN_INPUT_NAME, input_name,
                      -1);

  /* Continue to walk the tree. */
  return FALSE;
}


/*  public functions  */

GtkWidget *
ligma_device_info_editor_new (LigmaDeviceInfo *info)
{
  g_return_val_if_fail (LIGMA_IS_DEVICE_INFO (info), NULL);

  return g_object_new (LIGMA_TYPE_DEVICE_INFO_EDITOR,
                       "info", info,
                       NULL);
}
