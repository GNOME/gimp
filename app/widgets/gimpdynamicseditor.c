/* GIMP - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */



#define DYNAMICS_VIEW_SIZE 96

#define DEFAULT_PRESSURE_OPACITY       TRUE
#define DEFAULT_PRESSURE_HARDNESS      FALSE

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "libgimpconfig/gimpconfig.h"

#include "gimpdocked.h"
#include "gimpview.h"
#include "gimpviewrenderer.h"

#include "gimp-intl.h"

#include "gimpmenufactory.h"
#include "gimpdynamicseditor.h"
#include "core/gimpdynamics.h"

#include "core/gimptoolinfo.h"
#include "tools/gimptooloptions-gui.h"
#include "gimppropwidgets.h"

static void        pressure_options_gui  (GObject          *config,
                                          GtkTable         *table,
                                          gint              row,
                                          GtkWidget        *labels[]);
static void        velocity_options_gui  (GObject          *config,
                                          GtkTable         *table,
                                          gint              row);

static void        direction_options_gui  (GObject          *config,
                                          GtkTable         *table,
                                          gint              row);

static void        tilt_options_gui      (GObject          *config,
                                          GtkTable         *table,
                                          gint              row);

static void        random_options_gui    (GObject          *config,
                                          GtkTable         *table,
                                          gint              row);

static void        fading_options_gui    (GObject          *config,
                                          GtkTable         *table,
                                          gint              row);


static GObject     * get_config_value (GimpDynamicsEditor *editor);
/*  local function prototypes  */

static void   gimp_dynamics_editor_docked_iface_init (GimpDockedInterface *face);

static GObject * gimp_dynamics_editor_constructor (GType              type,
                                                   guint              n_params,
                                                   GObjectConstructParam *params);

static void   gimp_dynamics_editor_set_data       (GimpDataEditor     *editor,
                                                   GimpData           *data);

static void   gimp_dynamics_editor_set_context    (GimpDocked         *docked,
                                                   GimpContext        *context);
/*
static void   gimp_dynamics_editor_update_dynamics(GtkAdjustment      *adjustment,
                                                   GimpDynamicsEditor    *editor);*/

static void   gimp_dynamics_editor_notify_dynamics (GimpDynamics  *options,
                                                    GParamSpec           *pspec,
                                                    GimpDynamicsEditor      *editor);

G_DEFINE_TYPE_WITH_CODE (GimpDynamicsEditor, gimp_dynamics_editor,
                         GIMP_TYPE_DATA_EDITOR,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCKED,
                                                gimp_dynamics_editor_docked_iface_init))

#define parent_class gimp_dynamics_editor_parent_class

static GimpDockedInterface *parent_docked_iface = NULL;


static void
gimp_dynamics_editor_class_init (GimpDynamicsEditorClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpDataEditorClass *editor_class = GIMP_DATA_EDITOR_CLASS (klass);

  object_class->constructor = gimp_dynamics_editor_constructor;

  editor_class->set_data    = gimp_dynamics_editor_set_data;
  editor_class->title       = _("Dynamics Editor");
}

static void
gimp_dynamics_editor_docked_iface_init (GimpDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (GIMP_TYPE_DOCKED);

  iface->set_context = gimp_dynamics_editor_set_context;
}



static GObject *
gimp_dynamics_editor_constructor (GType                  type,
                                  guint                  n_params,
                                  GObjectConstructParam *params)
{
  GObject *object;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  gimp_docked_set_show_button_bar (GIMP_DOCKED (object), FALSE);

  return object;
}

static void
gimp_dynamics_editor_set_data (GimpDataEditor *editor,
                               GimpData       *data)
{
  GimpDynamicsEditor         *dynamics_editor = GIMP_DYNAMICS_EDITOR (editor);


  if (editor->data)
    g_signal_handlers_disconnect_by_func (editor->data,
                                          gimp_dynamics_editor_notify_dynamics,
                                          editor);

  GIMP_DATA_EDITOR_CLASS (parent_class)->set_data (editor, data);

  if (editor->data)
    g_signal_connect (editor->data, "notify",
                      G_CALLBACK (gimp_dynamics_editor_notify_dynamics),
                      editor);

  gimp_view_set_viewable (GIMP_VIEW (editor->view), GIMP_VIEWABLE (data));

}


static void
gimp_dynamics_editor_notify_dynamics (GimpDynamics  *options,
                                      GParamSpec           *pspec,
                                      GimpDynamicsEditor      *editor)
{
  GimpDataEditor *data_editor = GIMP_DATA_EDITOR (editor);

  g_return_val_if_fail (GIMP_IS_CONTEXT (data_editor->context), NULL);

  GimpDynamics *context_dyn = gimp_get_user_context(data_editor->context->gimp)->dynamics;

  g_return_val_if_fail (GIMP_IS_DYNAMICS (context_dyn), NULL);

  gimp_config_copy(options, context_dyn, 0);

}




static void
gimp_dynamics_editor_set_context (GimpDocked  *docked,
                                  GimpContext *context)
{
  GimpDataEditor *data_editor = GIMP_DATA_EDITOR (docked);

  parent_docked_iface->set_context (docked, context);

  gimp_view_renderer_set_context (GIMP_VIEW (data_editor->view)->renderer,
                                  context);
}

/*  public functions  */


GtkWidget *
gimp_dynamics_editor_new (GimpContext     *context,
                          GimpMenuFactory *menu_factory)
{
  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (menu_factory), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  GimpDynamicsEditor *editor;
  editor = g_object_new (GIMP_TYPE_DYNAMICS_EDITOR,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<DynamicsEditor>",
                       "ui-path",         "/dynamics-editor-popup",
                       "data-factory",    context->gimp->dynamics_factory,
                       "context",         context,
                       "data",            gimp_context_get_dynamics (context),
                       NULL);


  g_signal_connect (editor->dynamics_model, "notify",
                    G_CALLBACK (gimp_dynamics_editor_notify_dynamics),
                    editor);

  return editor;
}


static GObject *
get_config_value (GimpDynamicsEditor *editor)
{

  g_return_val_if_fail (GIMP_IS_DYNAMICS_EDITOR (editor), NULL);

  GObject  *config  = G_OBJECT (editor);

  return config;
}


static void
gimp_dynamics_editor_init (GimpDynamicsEditor *editor)
{
  GimpDataEditor   *data_editor = GIMP_DATA_EDITOR (editor);
  editor->dynamics_model = g_object_new(GIMP_TYPE_DYNAMICS,
                                        "name", "Default",
                                        NULL);

  GimpDynamics     *dynamics    = editor->dynamics_model;

  GtkWidget        *frame;
  GtkWidget        *box;

  GtkWidget        *vbox;
  GtkWidget        *table;
  GtkWidget        *label;
  gint              n_dynamics         = 0;
  GtkWidget        *dynamics_labels[7];
  GObject          *config = G_OBJECT(dynamics);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (editor), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  data_editor->view = gimp_view_new_full_by_types (NULL,
                                                   GIMP_TYPE_VIEW,
                                                   GIMP_TYPE_DYNAMICS,
                                                   DYNAMICS_VIEW_SIZE,
                                                   DYNAMICS_VIEW_SIZE, 0,
                                                   FALSE, FALSE, TRUE);

  gtk_widget_set_size_request (data_editor->view, -1, DYNAMICS_VIEW_SIZE);
  gimp_view_set_expand (GIMP_VIEW (data_editor->view), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), data_editor->view);
  gtk_widget_show (data_editor->view);



  vbox = gtk_vbox_new (FALSE, 6);
  //gtk_container_add (GTK_CONTAINER (data_editor->view), vbox);
  gtk_box_pack_start (GTK_BOX (data_editor), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  //n_dynamics = 5;

  dynamics_labels[n_dynamics] = gtk_label_new (_("Opacity"));
  n_dynamics++;

  dynamics_labels[n_dynamics] = gtk_label_new (_("Hardness"));
  n_dynamics++;

  dynamics_labels[n_dynamics] = gtk_label_new (_("Rate"));
  n_dynamics++;

  dynamics_labels[n_dynamics] = gtk_label_new (_("Size"));
  n_dynamics++;


  dynamics_labels[n_dynamics] = gtk_label_new (_("Aspect ratio"));
  n_dynamics++;

  dynamics_labels[n_dynamics] = gtk_label_new (_("Angle"));
  n_dynamics++;

  dynamics_labels[n_dynamics] = gtk_label_new (_("Color"));
  n_dynamics++;


  /* NB: When adding new dynamics, increase size of the
   * dynamics_labels[] array
   */

  if (n_dynamics > 0)
    {
      GtkWidget *inner_frame;
      GtkWidget *fixed;
      gint       i;
      gboolean   rtl = gtk_widget_get_direction (vbox) == GTK_TEXT_DIR_RTL;


      inner_frame = gimp_frame_new (NULL);
      gtk_container_add (GTK_CONTAINER (vbox), inner_frame);
      gtk_widget_show (inner_frame);


      table = gtk_table_new (7, n_dynamics + 2, FALSE);
      gtk_container_add (GTK_CONTAINER (inner_frame), table);
      gtk_widget_show (table);

      label = gtk_label_new (_("Pressure:"));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (label);

      label = gtk_label_new (_("Velocity:"));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (label);

      label = gtk_label_new (_("Direction:"));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (label);

      label = gtk_label_new (_("Tilt:"));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 4, 5,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (label);

      label = gtk_label_new (_("Random:"));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 5, 6,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (label);

      label = gtk_label_new (_("Fading:"));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 6, 7,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (label);



     pressure_options_gui (config,
                            GTK_TABLE (table), 1,
                            dynamics_labels);

      velocity_options_gui (config, GTK_TABLE (table), 2);

      direction_options_gui (config, GTK_TABLE (table), 3);

      tilt_options_gui (config, GTK_TABLE (table), 4);

      random_options_gui (config, GTK_TABLE (table), 5);

      fading_options_gui (config, GTK_TABLE (table), 6);


      fixed = gtk_fixed_new ();
      gtk_table_attach (GTK_TABLE (table), fixed, 0, n_dynamics + 2, 0, 1,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (fixed);

      for (i = 0; i < n_dynamics; i++)
        {
          gtk_label_set_angle (GTK_LABEL (dynamics_labels[i]),
                               90);
          gtk_misc_set_alignment (GTK_MISC (dynamics_labels[i]), 1.0, 1.0);
          gtk_fixed_put (GTK_FIXED (fixed), dynamics_labels[i], 0, 0);
          gtk_widget_show (dynamics_labels[i]);
        }

    }
}

/*  private functions  */

static GtkWidget *
dynamics_check_button_new (GObject     *config,
                           const gchar *property_name,
                           GtkTable    *table,
                           gint         column,
                           gint         row)
{
  GtkWidget *button;

  button = gimp_prop_check_button_new (config, property_name, NULL);
  gtk_table_attach (table, button, column, column + 1, row, row + 1,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (button);

  return button;
}

static void
dynamics_check_button_size_allocate (GtkWidget     *toggle,
                                     GtkAllocation *allocation,
                                     GtkWidget     *label)
{
  GtkWidget *fixed = label->parent;
  gint       x, y;

  if (gtk_widget_get_direction (label) == GTK_TEXT_DIR_LTR)
    x = allocation->x;
  else
    x = allocation->x + allocation->width - label->allocation.width;

  x -= fixed->allocation.x;

  y = fixed->allocation.height - label->allocation.height;

  gtk_fixed_move (GTK_FIXED (fixed), label, x, y);
}

static void
pressure_options_gui (GObject          *config,
                      GtkTable         *table,
                      gint              row,
                      GtkWidget        *labels[])
{
  GtkWidget *button;
  gint       column = 1;
  GtkWidget *scalebutton;

      button = dynamics_check_button_new (config, "pressure-opacity",
                                          table, column, row);
      g_signal_connect (button, "size-allocate",
                        G_CALLBACK (dynamics_check_button_size_allocate),
                        labels[column - 1]);
      column++;


      button = dynamics_check_button_new (config, "pressure-hardness",
                                          table, column, row);
      g_signal_connect (button, "size-allocate",
                        G_CALLBACK (dynamics_check_button_size_allocate),
                        labels[column - 1]);
      column++;

      button = dynamics_check_button_new (config, "pressure-rate",
                                          table, column, row);
      g_signal_connect (button, "size-allocate",
                        G_CALLBACK (dynamics_check_button_size_allocate),
                        labels[column - 1]);
      column++;

      button = dynamics_check_button_new (config, "pressure-size",
                                            table, column, row);

      g_signal_connect (button, "size-allocate",
                        G_CALLBACK (dynamics_check_button_size_allocate),
                        labels[column - 1]);
      column++;

      button = dynamics_check_button_new (config, "pressure-aspect_ratio",
                                          table, column, row);

      g_signal_connect (button, "size-allocate",
                        G_CALLBACK (dynamics_check_button_size_allocate),
                        labels[column - 1]);
      column++;

      button = dynamics_check_button_new (config, "pressure-angle",
                                          table, column, row);
      g_signal_connect (button, "size-allocate",
                        G_CALLBACK (dynamics_check_button_size_allocate),
                        labels[column - 1]);
      column++;

      button = dynamics_check_button_new (config, "pressure-color",
                                          table, column, row);
      g_signal_connect (button, "size-allocate",
                        G_CALLBACK (dynamics_check_button_size_allocate),
                        labels[column - 1]);
      column++;

}

static void
velocity_options_gui (GObject          *config,
                      GtkTable         *table,
                      gint              row)
{
  GtkWidget *button;
  gint       column = 1;
  GtkWidget *scalebutton;

      button = dynamics_check_button_new (config, "velocity-opacity",
                                          table, column, row);
      column++;


      button = dynamics_check_button_new (config, "velocity-hardness",
                                          table, column, row);
      column++;

      button = dynamics_check_button_new (config, "velocity-rate",
                                          table, column, row);
      column++;

      button = dynamics_check_button_new (config, "velocity-size",
                                            table, column, row);
      column++;

      button = dynamics_check_button_new (config, "velocity-aspect_ratio",
                                          table, column, row);
      column++;

      button = dynamics_check_button_new (config, "velocity-angle",
                                          table, column, row);
      column++;

      button = dynamics_check_button_new (config, "velocity-color",
                                          table, column, row);
      column++;

}

static void
direction_options_gui (GObject          *config,
                      GtkTable         *table,
                      gint              row)
{
  GtkWidget *button;
  gint       column = 1;
  GtkWidget *scalebutton;

      button = dynamics_check_button_new (config, "direction-opacity",
                                          table, column, row);
      column++;


      button = dynamics_check_button_new (config, "direction-hardness",
                                          table, column, row);
      column++;

      button = dynamics_check_button_new (config, "direction-rate",
                                          table, column, row);
      column++;

      button = dynamics_check_button_new (config, "direction-size",
                                            table, column, row);
      column++;

      button = dynamics_check_button_new (config, "direction-aspect_ratio",
                                          table, column, row);
      column++;

      button = dynamics_check_button_new (config, "direction-angle",
                                          table, column, row);
      column++;

      button = dynamics_check_button_new (config, "direction-color",
                                          table, column, row);
      column++;

}

static void
tilt_options_gui (GObject          *config,
                      GtkTable         *table,
                      gint              row)
{
  GtkWidget *button;
  gint       column = 1;
  GtkWidget *scalebutton;

      button = dynamics_check_button_new (config, "tilt-opacity",
                                          table, column, row);
      column++;


      button = dynamics_check_button_new (config, "tilt-hardness",
                                          table, column, row);
      column++;

      button = dynamics_check_button_new (config, "tilt-rate",
                                          table, column, row);
      column++;

      button = dynamics_check_button_new (config, "tilt-size",
                                            table, column, row);
      column++;

      button = dynamics_check_button_new (config, "tilt-aspect_ratio",
                                          table, column, row);
      column++;

      button = dynamics_check_button_new (config, "tilt-angle",
                                          table, column, row);
      column++;

      button = dynamics_check_button_new (config, "tilt-color",
                                          table, column, row);
      column++;

}

static void
random_options_gui (GObject          *config,
                      GtkTable         *table,
                      gint              row)
{
  GtkWidget *button;
  gint       column = 1;
  GtkWidget *scalebutton;

      button = dynamics_check_button_new (config, "random-opacity",
                                          table, column, row);
      column++;


      button = dynamics_check_button_new (config, "random-hardness",
                                          table, column, row);
      column++;

      button = dynamics_check_button_new (config, "random-rate",
                                          table, column, row);
      column++;

      button = dynamics_check_button_new (config, "random-size",
                                            table, column, row);
      column++;

      button = dynamics_check_button_new (config, "random-aspect_ratio",
                                          table, column, row);
      column++;

      button = dynamics_check_button_new (config, "random-angle",
                                          table, column, row);
      column++;

      button = dynamics_check_button_new (config, "random-color",
                                          table, column, row);
      column++;

}

static void
fading_options_gui (GObject          *config,
                      GtkTable         *table,
                      gint              row)
{
  GtkWidget *button;
  gint       column = 1;
  GtkWidget *scalebutton;

      button = dynamics_check_button_new (config, "fading-opacity",
                                          table, column, row);
      column++;


      button = dynamics_check_button_new (config, "fading-hardness",
                                          table, column, row);
      column++;

      button = dynamics_check_button_new (config, "fading-rate",
                                          table, column, row);
      column++;

      button = dynamics_check_button_new (config, "fading-size",
                                            table, column, row);
      column++;

      button = dynamics_check_button_new (config, "fading-aspect_ratio",
                                          table, column, row);
      column++;

      button = dynamics_check_button_new (config, "fading-angle",
                                          table, column, row);
      column++;

      button = dynamics_check_button_new (config, "fading-color",
                                          table, column, row);
      column++;

}
