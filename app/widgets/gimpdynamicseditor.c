/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
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

#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmadynamics.h"

#include "ligmadocked.h"
#include "ligmadynamicseditor.h"
#include "ligmadynamicsoutputeditor.h"
#include "ligmamenufactory.h"
#include "ligmapropwidgets.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void   ligma_dynamics_editor_constructed     (GObject            *object);
static void   ligma_dynamics_editor_finalize        (GObject            *object);

static void   ligma_dynamics_editor_set_data        (LigmaDataEditor     *editor,
                                                    LigmaData           *data);

static void   ligma_dynamics_editor_notify_model    (LigmaDynamics       *options,
                                                    const GParamSpec   *pspec,
                                                    LigmaDynamicsEditor *editor);
static void   ligma_dynamics_editor_notify_data     (LigmaDynamics       *options,
                                                    const GParamSpec   *pspec,
                                                    LigmaDynamicsEditor *editor);

static void   ligma_dynamics_editor_add_icon_editor (LigmaDynamics       *dynamics,
                                                    Ligma               *ligma,
                                                    GtkWidget          *vbox);

static void   ligma_dynamics_editor_add_output_row  (GObject     *config,
                                                    const gchar *row_label,
                                                    GtkGrid     *grid,
                                                    gint         row);

static void ligma_dynamics_editor_init_output_editors (LigmaDynamics *dynamics,
                                                      GtkWidget    *view_selector,
                                                      GtkWidget    *notebook,
                                                      GtkWidget    *check_grid);

static GtkWidget * dynamics_check_button_new       (GObject     *config,
                                                    const gchar *property_name,
                                                    GtkGrid     *grid,
                                                    gint         column,
                                                    gint         row);

static void      ligma_dynamics_editor_view_changed (GtkComboBox *combo,
                                                    GtkWidget   *notebook);

G_DEFINE_TYPE_WITH_CODE (LigmaDynamicsEditor, ligma_dynamics_editor,
                         LIGMA_TYPE_DATA_EDITOR,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_DOCKED, NULL))

#define parent_class ligma_dynamics_editor_parent_class


static void
ligma_dynamics_editor_class_init (LigmaDynamicsEditorClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  LigmaDataEditorClass *editor_class = LIGMA_DATA_EDITOR_CLASS (klass);

  object_class->constructed = ligma_dynamics_editor_constructed;
  object_class->finalize    = ligma_dynamics_editor_finalize;

  editor_class->set_data    = ligma_dynamics_editor_set_data;
  editor_class->title       = _("Paint Dynamics Editor");
}

static void
ligma_dynamics_editor_init (LigmaDynamicsEditor *editor)
{
  LigmaDataEditor *data_editor = LIGMA_DATA_EDITOR (editor);

  editor->dynamics_model = g_object_new (LIGMA_TYPE_DYNAMICS, NULL);

  g_signal_connect (editor->dynamics_model, "notify",
                    G_CALLBACK (ligma_dynamics_editor_notify_model),
                    editor);

  editor->view_selector =
    ligma_enum_combo_box_new (LIGMA_TYPE_DYNAMICS_OUTPUT_TYPE);
  gtk_box_pack_start (GTK_BOX (data_editor), editor->view_selector,
                      FALSE, FALSE, 0);
  gtk_widget_show (editor->view_selector);

  editor->notebook = gtk_notebook_new ();
  gtk_notebook_set_show_border (GTK_NOTEBOOK (editor->notebook), FALSE);
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (editor->notebook), FALSE);
  gtk_box_pack_start (GTK_BOX (editor), editor->notebook, TRUE, TRUE, 0);
  gtk_widget_show (editor->notebook);
}

static void
ligma_dynamics_editor_constructed (GObject *object)
{
  LigmaDataEditor     *data_editor = LIGMA_DATA_EDITOR (object);
  LigmaDynamicsEditor *editor      = LIGMA_DYNAMICS_EDITOR (object);
  LigmaDynamics       *dynamics    = editor->dynamics_model;
  GtkWidget          *input_labels[7];
  GtkWidget          *vbox;
  GtkWidget          *icon_box;
  GtkWidget          *grid;
  gint                n_inputs    = G_N_ELEMENTS (input_labels);
  gint                i;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_notebook_append_page (GTK_NOTEBOOK (editor->notebook),
                            vbox, NULL);
  gtk_widget_show (vbox);

  icon_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), icon_box, FALSE, FALSE, 0);
  gtk_widget_show (icon_box);

  ligma_dynamics_editor_add_icon_editor (dynamics,
                                        data_editor->context->ligma,
                                        vbox);

  grid = gtk_grid_new ();
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  ligma_dynamics_editor_init_output_editors (dynamics,
                                            editor->view_selector,
                                            editor->notebook,
                                            grid);

  input_labels[0] = gtk_label_new (_("Pressure"));
  input_labels[1] = gtk_label_new (_("Velocity"));
  input_labels[2] = gtk_label_new (_("Direction"));
  input_labels[3] = gtk_label_new (_("Tilt"));
  input_labels[4] = gtk_label_new (_("Wheel/Rotation"));
  input_labels[5] = gtk_label_new (_("Random"));
  input_labels[6] = gtk_label_new (_("Fade"));

  for (i = 0; i < n_inputs; i++)
    {
      gtk_label_set_angle (GTK_LABEL (input_labels[i]), 90);
      gtk_label_set_yalign (GTK_LABEL (input_labels[i]), 1.0);

      gtk_grid_attach (GTK_GRID (grid), input_labels[i], i + 1, 0, 1, 1);
      gtk_widget_show (input_labels[i]);
    }

  ligma_int_combo_box_prepend (LIGMA_INT_COMBO_BOX (editor->view_selector),
                              LIGMA_INT_STORE_VALUE,     -1,
                              LIGMA_INT_STORE_LABEL,     _("Mapping matrix"),
                              LIGMA_INT_STORE_USER_DATA, vbox,
                              -1);

  ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (editor->view_selector), -1);

  ligma_docked_set_show_button_bar (LIGMA_DOCKED (object), FALSE);
}

static void
ligma_dynamics_editor_finalize (GObject *object)
{
  LigmaDynamicsEditor *editor = LIGMA_DYNAMICS_EDITOR (object);

  g_clear_object (&editor->dynamics_model);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_dynamics_editor_set_data (LigmaDataEditor *editor,
                               LigmaData       *data)
{
  LigmaDynamicsEditor *dynamics_editor = LIGMA_DYNAMICS_EDITOR (editor);

  if (editor->data)
    g_signal_handlers_disconnect_by_func (editor->data,
                                          ligma_dynamics_editor_notify_data,
                                          editor);

  LIGMA_DATA_EDITOR_CLASS (parent_class)->set_data (editor, data);

  if (editor->data)
    {
      g_signal_handlers_block_by_func (dynamics_editor->dynamics_model,
                                       ligma_dynamics_editor_notify_model,
                                       editor);

      ligma_config_copy (LIGMA_CONFIG (editor->data),
                        LIGMA_CONFIG (dynamics_editor->dynamics_model),
                        LIGMA_CONFIG_PARAM_SERIALIZE);

      g_signal_handlers_unblock_by_func (dynamics_editor->dynamics_model,
                                         ligma_dynamics_editor_notify_model,
                                         editor);

      g_signal_connect (editor->data, "notify",
                        G_CALLBACK (ligma_dynamics_editor_notify_data),
                        editor);
    }

  gtk_widget_set_sensitive (dynamics_editor->notebook, editor->data_editable);
}


/*  public functions  */

GtkWidget *
ligma_dynamics_editor_new (LigmaContext     *context,
                          LigmaMenuFactory *menu_factory)
{
  g_return_val_if_fail (LIGMA_IS_MENU_FACTORY (menu_factory), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  return g_object_new (LIGMA_TYPE_DYNAMICS_EDITOR,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<DynamicsEditor>",
                       "ui-path",         "/dynamics-editor-popup",
                       "data-factory",    context->ligma->dynamics_factory,
                       "context",         context,
                       "data",            ligma_context_get_dynamics (context),
                       NULL);
}


/*  private functions  */

static void
ligma_dynamics_editor_notify_model (LigmaDynamics       *options,
                                   const GParamSpec   *pspec,
                                   LigmaDynamicsEditor *editor)
{
  LigmaDataEditor *data_editor = LIGMA_DATA_EDITOR (editor);

  if (data_editor->data)
    {
      g_signal_handlers_block_by_func (data_editor->data,
                                       ligma_dynamics_editor_notify_data,
                                       editor);

      ligma_config_copy (LIGMA_CONFIG (editor->dynamics_model),
                        LIGMA_CONFIG (data_editor->data),
                        LIGMA_CONFIG_PARAM_SERIALIZE);

      g_signal_handlers_unblock_by_func (data_editor->data,
                                         ligma_dynamics_editor_notify_data,
                                         editor);
    }
}

static void
ligma_dynamics_editor_notify_data (LigmaDynamics       *options,
                                  const GParamSpec   *pspec,
                                  LigmaDynamicsEditor *editor)
{
  LigmaDataEditor *data_editor = LIGMA_DATA_EDITOR (editor);

  g_signal_handlers_block_by_func (editor->dynamics_model,
                                   ligma_dynamics_editor_notify_model,
                                   editor);

  ligma_config_copy (LIGMA_CONFIG (data_editor->data),
                    LIGMA_CONFIG (editor->dynamics_model),
                    LIGMA_CONFIG_PARAM_SERIALIZE);

  g_signal_handlers_unblock_by_func (editor->dynamics_model,
                                     ligma_dynamics_editor_notify_model,
                                     editor);
}

static void
ligma_dynamics_editor_add_icon_editor (LigmaDynamics *dynamics,
                                      Ligma         *ligma,
                                      GtkWidget    *vbox)
{
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *button;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Icon:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  button = ligma_prop_icon_picker_new (LIGMA_VIEWABLE (dynamics), ligma);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
}

static void
ligma_dynamics_editor_add_output_row (GObject     *config,
                                     const gchar *row_label,
                                     GtkGrid     *grid,
                                     gint         row)
{
  GtkWidget *label;
  gint       column = 1;

  label = gtk_label_new (row_label);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (grid, label, 0, row, 1, 1);
  gtk_widget_show (label);

  dynamics_check_button_new (config, "use-pressure",
                             grid, column, row);
  column++;

  dynamics_check_button_new (config, "use-velocity",
                             grid, column, row);
  column++;

  dynamics_check_button_new (config, "use-direction",
                             grid, column, row);
  column++;

  dynamics_check_button_new (config,  "use-tilt",
                             grid, column, row);
  column++;

  dynamics_check_button_new (config,  "use-wheel",
                             grid, column, row);
  column++;

  dynamics_check_button_new (config, "use-random",
                             grid, column, row);
  column++;

  dynamics_check_button_new (config, "use-fade",
                             grid, column, row);
  column++;
}

static GtkWidget *
dynamics_check_button_new (GObject     *config,
                           const gchar *property_name,
                           GtkGrid     *grid,
                           gint         column,
                           gint         row)
{
  GtkWidget *button;

  button = ligma_prop_check_button_new (config, property_name, NULL);
  gtk_widget_destroy (gtk_bin_get_child (GTK_BIN (button)));
  gtk_grid_attach (grid, button, column, row, 1, 1);

  return button;
}

static void
ligma_dynamics_editor_init_output_editors (LigmaDynamics *dynamics,
                                          GtkWidget    *view_selector,
                                          GtkWidget    *notebook,
                                          GtkWidget    *check_grid)
{
  GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (view_selector));
  LigmaIntStore *list  = LIGMA_INT_STORE (model);
  GtkTreeIter   iter;
  gboolean      iter_valid;
  gint          i;

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter), i = 1;
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter), i++)
    {
      gint                output_type;
      gchar              *label;
      LigmaDynamicsOutput *output;
      GtkWidget          *output_editor;

      gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
                          LIGMA_INT_STORE_VALUE, &output_type,
                          LIGMA_INT_STORE_LABEL, &label,
                          -1);

      output = ligma_dynamics_get_output (dynamics, output_type);

      output_editor = ligma_dynamics_output_editor_new (output);

      gtk_notebook_append_page (GTK_NOTEBOOK (notebook), output_editor, NULL);
      gtk_widget_show (output_editor);

      gtk_list_store_set (GTK_LIST_STORE (list), &iter,
                          LIGMA_INT_STORE_USER_DATA, output_editor,
                          -1);

      ligma_dynamics_editor_add_output_row (G_OBJECT (output),
                                           label,
                                           GTK_GRID (check_grid),
                                           i);

      g_free (label);
  }

  g_signal_connect (G_OBJECT (view_selector), "changed",
                    G_CALLBACK (ligma_dynamics_editor_view_changed),
                    notebook);
}

static void
ligma_dynamics_editor_view_changed (GtkComboBox *combo,
                                   GtkWidget   *notebook)
{
  GtkTreeModel *model = gtk_combo_box_get_model (combo);
  GtkTreeIter   iter;
  gpointer      widget;
  gint          page;

  gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter);

  gtk_tree_model_get (model, &iter,
                      LIGMA_INT_STORE_USER_DATA, &widget,
                      -1);
  page = gtk_notebook_page_num (GTK_NOTEBOOK (notebook), widget);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), page);
}
