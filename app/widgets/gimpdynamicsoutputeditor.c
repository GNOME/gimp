/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpdynamicsoutputeditor.c
 * Copyright (C) 2010 Alexia Death
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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"
#include "libgimpcolor/gimpcolor.h"

#include "widgets-types.h"

#include "core/gimpcurve.h"
#include "core/gimpdynamicsoutput.h"

#include "gimpcurveview.h"
#include "gimpdynamicsoutputeditor.h"

#include "gimp-intl.h"


#define CURVE_SIZE   185
#define CURVE_BORDER   4


enum
{
  PROP_0,
  PROP_OUTPUT
};

enum
{
  INPUT_COLUMN_INDEX,
  INPUT_COLUMN_USE_INPUT,
  INPUT_COLUMN_NAME,
  INPUT_N_COLUMNS
};

enum
{
  INPUT_PRESSURE,
  INPUT_VELOCITY,
  INPUT_DIRECTION,
  INPUT_TILT,
  INPUT_RANDOM,
  INPUT_FADE,
  N_INPUTS
};

typedef struct _GimpDynamicsOutputEditorPrivate GimpDynamicsOutputEditorPrivate;

struct _GimpDynamicsOutputEditorPrivate
{
  GimpDynamicsOutput *output;

  GtkWidget      *vbox;

  GtkWidget      *notebook;

  GtkWidget      *curve_view;

  GtkListStore   *input_list;
};

#define GIMP_DYNAMICS_OUTPUT_EDITOR_GET_PRIVATE(editor) \
        G_TYPE_INSTANCE_GET_PRIVATE (editor, \
                                     GIMP_TYPE_DYNAMICS_OUTPUT_EDITOR, \
                                     GimpDynamicsOutputEditorPrivate)


static GObject * gimp_dynamics_output_editor_constructor  (GType                  type,
                                                           guint                  n_params,
                                                           GObjectConstructParam *params);
static void      gimp_dynamics_output_editor_finalize     (GObject               *object);
static void      gimp_dynamics_output_editor_set_property (GObject               *object,
                                                           guint                  property_id,
                                                           const GValue          *value,
                                                           GParamSpec            *pspec);
static void      gimp_dynamics_output_editor_get_property (GObject               *object,
                                                           guint                  property_id,
                                                           GValue                *value,
                                                           GParamSpec            *pspec);

static void     gimp_dynamics_output_editor_curve_reset   (GtkWidget             *button,
                                                           GimpCurve             *curve);

static void    gimp_dynamics_output_editor_input_selected (GtkTreeSelection *selection,
                                                           GimpDynamicsOutputEditor *editor);

static void     gimp_dynamics_output_editor_input_toggled (GtkWidget *cell,
                                                           GimpDynamicsOutputEditor *editor);

G_DEFINE_TYPE (GimpDynamicsOutputEditor, gimp_dynamics_output_editor,
               GTK_TYPE_VBOX)

#define parent_class gimp_dynamics_output_editor_parent_class

static void
gimp_dynamics_output_editor_class_init (GimpDynamicsOutputEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructor  = gimp_dynamics_output_editor_constructor;
  object_class->finalize     = gimp_dynamics_output_editor_finalize;
  object_class->set_property = gimp_dynamics_output_editor_set_property;
  object_class->get_property = gimp_dynamics_output_editor_get_property;

  g_object_class_install_property (object_class, PROP_OUTPUT,
                                   g_param_spec_object ("output",
                                                        NULL, NULL,
                                                        GIMP_TYPE_DYNAMICS_OUTPUT,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (object_class, sizeof (GimpDynamicsOutputEditorPrivate));
}

static void
gimp_dynamics_output_editor_init (GimpDynamicsOutputEditor *editor)
{
  GimpDynamicsOutputEditorPrivate *private;

  private = GIMP_DYNAMICS_OUTPUT_EDITOR_GET_PRIVATE (editor);

  gtk_box_set_spacing (GTK_BOX (editor), 6);
}

static GObject *
gimp_dynamics_output_editor_constructor (GType                   type,
                                         guint                   n_params,
                                         GObjectConstructParam  *params)
{
  GObject                         *object;
  GimpDynamicsOutputEditor        *editor;
  GimpDynamicsOutputEditorPrivate *private;
  GtkWidget                       *label;
  GtkWidget                       *view;
  GtkWidget                       *button;
  GtkCellRenderer                 *cell;
  GtkTreeSelection                *tree_sel;
  GtkTreeIter                      iter = {0};

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  editor  = GIMP_DYNAMICS_OUTPUT_EDITOR (object);
  private = GIMP_DYNAMICS_OUTPUT_EDITOR_GET_PRIVATE (object);

  g_assert (GIMP_IS_DYNAMICS_OUTPUT (private->output));

  label = gtk_label_new ("This is pressure curve.");
  gtk_box_pack_start (GTK_BOX (editor), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  private->curve_view = gimp_curve_view_new ();
  g_object_set (private->curve_view,
                "border-width", CURVE_BORDER,
                NULL);
  gtk_widget_set_size_request (private->curve_view,
                               CURVE_SIZE + CURVE_BORDER * 2,
                               CURVE_SIZE + CURVE_BORDER * 2);
  gtk_box_pack_start (GTK_BOX (editor), private->curve_view, TRUE, TRUE, 0);
  gtk_widget_show (private->curve_view);

  gimp_curve_view_set_curve (GIMP_CURVE_VIEW (private->curve_view),
                             private->output->pressure_curve);

  button = gtk_button_new_with_mnemonic (_("_Reset Curve"));
  gtk_box_pack_start (GTK_BOX (editor), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (gimp_dynamics_output_editor_curve_reset),
                    private->output->pressure_curve);

  private->input_list = gtk_list_store_new (INPUT_N_COLUMNS,
                                            G_TYPE_INT,
                                            G_TYPE_BOOLEAN,
                                            G_TYPE_STRING);

  gtk_list_store_insert_with_values (private->input_list,
                                     &iter, INPUT_PRESSURE,
                                     INPUT_COLUMN_INDEX, INPUT_PRESSURE,
                                     INPUT_COLUMN_USE_INPUT, private->output->use_pressure,
                                     INPUT_COLUMN_NAME,  _("Pressure"),
                                     -1);

  gtk_list_store_insert_with_values (private->input_list,
                                     &iter, INPUT_VELOCITY,
                                     INPUT_COLUMN_INDEX, INPUT_VELOCITY,
                                     INPUT_COLUMN_USE_INPUT, private->output->use_velocity,
                                     INPUT_COLUMN_NAME,  _("Velocity"),
                                     -1);

  gtk_list_store_insert_with_values (private->input_list,
                                     &iter, INPUT_DIRECTION,
                                     INPUT_COLUMN_INDEX, INPUT_DIRECTION,
                                     INPUT_COLUMN_USE_INPUT, private->output->use_direction,
                                     INPUT_COLUMN_NAME,  _("Direction"),
                                     -1);

  gtk_list_store_insert_with_values (private->input_list,
                                     &iter, INPUT_TILT,
                                     INPUT_COLUMN_INDEX, INPUT_TILT,
                                     INPUT_COLUMN_USE_INPUT, private->output->use_tilt,
                                     INPUT_COLUMN_NAME,  _("Tilt"),
                                     -1);

  gtk_list_store_insert_with_values (private->input_list,
                                     &iter, INPUT_RANDOM,
                                     INPUT_COLUMN_INDEX, INPUT_RANDOM,
                                     INPUT_COLUMN_USE_INPUT, private->output->use_random,
                                     INPUT_COLUMN_NAME,  _("Random"),
                                     -1);

  gtk_list_store_insert_with_values (private->input_list,
                                     &iter, INPUT_FADE,
                                     INPUT_COLUMN_INDEX, INPUT_FADE,
                                     INPUT_COLUMN_USE_INPUT, private->output->use_fade,
                                     INPUT_COLUMN_NAME,  _("Fade"),
                                     -1);

  view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (private->input_list));
  g_object_unref (private->input_list);

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);


  cell = gtk_cell_renderer_toggle_new ();

  g_object_set (cell,
                "mode",     GTK_CELL_RENDERER_MODE_ACTIVATABLE,
                "activatable", TRUE,
                NULL);

  g_signal_connect(G_OBJECT(cell),
                  "toggled",
                  G_CALLBACK(gimp_dynamics_output_editor_input_toggled),
                  editor);

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
                                               -1, NULL,
                                               cell,
                                               "active", INPUT_COLUMN_USE_INPUT,
                                               NULL);

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
                                               -1, NULL,
                                               gtk_cell_renderer_text_new (),
                                               "text", INPUT_COLUMN_NAME,
                                               NULL);

  gtk_box_pack_start (GTK_BOX (editor), view, FALSE, FALSE, 0);
  gtk_widget_show (view);

  tree_sel = gtk_tree_view_get_selection(GTK_TREE_VIEW (view));
  gtk_tree_selection_set_mode(tree_sel,
                              GTK_SELECTION_BROWSE);


  g_signal_connect(G_OBJECT(tree_sel),
                  "changed",
                  G_CALLBACK(gimp_dynamics_output_editor_input_selected),
                  editor);

  return object;
}

static void
gimp_dynamics_output_editor_finalize (GObject *object)
{
  GimpDynamicsOutputEditorPrivate *private;

  private = GIMP_DYNAMICS_OUTPUT_EDITOR_GET_PRIVATE (object);

  if (private->output)
    {
      g_object_unref (private->output);
      private->output = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_dynamics_output_editor_set_property (GObject      *object,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  GimpDynamicsOutputEditorPrivate *private;

  private = GIMP_DYNAMICS_OUTPUT_EDITOR_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_OUTPUT:
      private->output = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_dynamics_output_editor_get_property (GObject    *object,
                                          guint       property_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  GimpDynamicsOutputEditorPrivate *private;

  private = GIMP_DYNAMICS_OUTPUT_EDITOR_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_OUTPUT:
      g_value_set_object (value, private->output);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


static void
gimp_dynamics_output_editor_curve_reset (GtkWidget *button,
                                         GimpCurve *curve)
{
  gimp_curve_reset (curve, TRUE);
}

static void
gimp_dynamics_output_editor_input_selected (GtkTreeSelection *selection,
                                            GimpDynamicsOutputEditor *editor)
{
  GimpDynamicsOutputEditorPrivate *private;
  GtkTreeModel     *model;
  GtkTreeIter       iter;
  GimpRGB       bg_color;

  gimp_rgb_set (&bg_color,   0.5, 0.5, 0.5);

  private = GIMP_DYNAMICS_OUTPUT_EDITOR_GET_PRIVATE (editor);

  if (gtk_tree_selection_get_selected(selection, &model, &iter))
  {
    gint input;

    gtk_tree_model_get (model, &iter,
                        INPUT_COLUMN_INDEX, &input,
                        -1);

    gimp_curve_view_remove_all_backgrounds (GIMP_CURVE_VIEW (private->curve_view));

    if (input == INPUT_PRESSURE)
      {
        gimp_curve_view_set_curve (GIMP_CURVE_VIEW (private->curve_view),
                                   private->output->pressure_curve);
      }
    else if (private->output->use_pressure)
      {
        gimp_curve_view_add_background (GIMP_CURVE_VIEW (private->curve_view),
                                        private->output->pressure_curve,
                                        &bg_color);
      }

    if (input == INPUT_VELOCITY)
      {
        gimp_curve_view_set_curve (GIMP_CURVE_VIEW (private->curve_view),
                                   private->output->velocity_curve);
      }
    else if (private->output->use_velocity)
      {
        gimp_curve_view_add_background (GIMP_CURVE_VIEW (private->curve_view),
                                        private->output->velocity_curve,
                                        &bg_color);
      }
    if (input == INPUT_DIRECTION)
      {
        gimp_curve_view_set_curve (GIMP_CURVE_VIEW (private->curve_view),
                                   private->output->direction_curve);
      }
    else if (private->output->use_direction)
      {
        gimp_curve_view_add_background (GIMP_CURVE_VIEW (private->curve_view),
                                        private->output->direction_curve,
                                        &bg_color);
      }
    if (input == INPUT_TILT)
      {
        gimp_curve_view_set_curve (GIMP_CURVE_VIEW (private->curve_view),
                                   private->output->tilt_curve);
      }
    else if (private->output->use_tilt)
      {
        gimp_curve_view_add_background (GIMP_CURVE_VIEW (private->curve_view),
                                        private->output->tilt_curve,
                                        &bg_color);
      }
    if (input == INPUT_RANDOM)
      {
        gimp_curve_view_set_curve (GIMP_CURVE_VIEW (private->curve_view),
                                   private->output->random_curve);
      }
    else if (private->output->use_random)
      {
        gimp_curve_view_add_background (GIMP_CURVE_VIEW (private->curve_view),
                                        private->output->random_curve,
                                        &bg_color);
      }

    if (input == INPUT_FADE) {
        gimp_curve_view_set_curve (GIMP_CURVE_VIEW (private->curve_view),
                                   private->output->fade_curve);
        }
    else if (private->output->use_fade) {
        gimp_curve_view_add_background (GIMP_CURVE_VIEW (private->curve_view),
                                        private->output->fade_curve,
                                        &bg_color);
      }
    gtk_widget_queue_draw (private->curve_view);
  }

}

static void
gimp_dynamics_output_editor_input_toggled (GtkWidget *cell,
                                           GimpDynamicsOutputEditor *editor)
{
  printf("Input toggled\n");
}

/*  public functions  */

GtkWidget *
gimp_dynamics_output_editor_new (GimpDynamicsOutput *output)
{
  g_return_val_if_fail (GIMP_IS_DYNAMICS_OUTPUT (output), NULL);

  return g_object_new (GIMP_TYPE_DYNAMICS_OUTPUT_EDITOR,
                       "output", output,
                       NULL);
}
