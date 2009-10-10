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

#include "gimp-intl.h"

#include "gimpmenufactory.h"
#include "gimpdynamicseditor.h"
#include "core/gimpdynamics.h"

#include "core/gimptoolinfo.h"
#include "tools/gimptooloptions-gui.h"
#include "gimppropwidgets.h"

static GObject     * get_config_value (GimpDynamicsEditor *editor);

/*  local function prototypes  */

static void   gimp_dynamics_editor_docked_iface_init (GimpDockedInterface *face);

static GObject * gimp_dynamics_editor_constructor  (GType              type,
                                                    guint              n_params,
                                                    GObjectConstructParam *params);

static void   gimp_dynamics_editor_set_data        (GimpDataEditor     *editor,
                                                    GimpData           *data);

static void   gimp_dynamics_editor_set_context     (GimpDocked         *docked,
                                                    GimpContext        *context);

static void   gimp_dynamics_editor_notify_dynamics (GimpDynamics  *options,
                                                    GParamSpec           *pspec,
                                                    GimpDynamicsEditor      *editor);

static void   dynamics_output_maping_row_gui       (GObject     *config,
                                                    const gchar *property_name_part,
                                                    const gchar *property_label,
                                                    GtkTable    *table,
                                                    gint         row,
                                                    GtkWidget   *labels[]);

static GtkWidget * dynamics_check_button_new       (GObject     *config,
                                                    const gchar *property_name,
                                                    GtkTable    *table,
                                                    gint         column,
                                                    gint         row);


static void    dynamics_check_button_size_allocate (GtkWidget     *toggle,
                                                    GtkAllocation *allocation,
                                                    GtkWidget     *label);

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
                                        NULL);

  GimpDynamics     *dynamics    = editor->dynamics_model;

  GtkWidget        *frame;
  GtkWidget        *box;

  GtkWidget        *vbox;
  GtkWidget        *table;
  GtkWidget        *label;
  gint              n_dynamics         = 0;
  GtkWidget        *dynamics_labels[6];
  GObject          *config = G_OBJECT(dynamics);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (data_editor), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  /*gboolean  pressure;
  gboolean  velocity;
  gboolean  direction;
  gboolean  tilt;
  gboolean  random;
  gboolean  fade;
*/

  dynamics_labels[n_dynamics] = gtk_label_new (_("Pressure"));
  n_dynamics++;

  dynamics_labels[n_dynamics] = gtk_label_new (_("Velocity"));
  n_dynamics++;

  dynamics_labels[n_dynamics] = gtk_label_new (_("Direction"));
  n_dynamics++;

  dynamics_labels[n_dynamics] = gtk_label_new (_("Tilt"));
  n_dynamics++;

  dynamics_labels[n_dynamics] = gtk_label_new (_("Random"));
  n_dynamics++;

  dynamics_labels[n_dynamics] = gtk_label_new (_("Fade"));
  n_dynamics++;

  /* NB: When adding new dynamics driver, increase size of the
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


      table = gtk_table_new (9, n_dynamics + 2, FALSE);
      gtk_container_add (GTK_CONTAINER (inner_frame), table);
      gtk_widget_show (table);


      dynamics_output_maping_row_gui(G_OBJECT(editor->dynamics_model),
                                     "opacity",
                                     _("Opacity"),
                                     table,
                                     1,
                                     dynamics_labels);

      dynamics_output_maping_row_gui(G_OBJECT(editor->dynamics_model),
                                     "hardness",
                                     _("Hardness"),
                                     table,
                                     2,
                                     dynamics_labels);

      dynamics_output_maping_row_gui(G_OBJECT(editor->dynamics_model),
                                     "rate",
                                     _("Rate"),
                                     table,
                                     3,
                                     dynamics_labels);

      dynamics_output_maping_row_gui(G_OBJECT(editor->dynamics_model),
                                     "size",
                                     _("Size"),
                                     table,
                                     4,
                                     dynamics_labels);

      dynamics_output_maping_row_gui(G_OBJECT(editor->dynamics_model),
                                     "aspect-ratio",
                                     _("Aspect ratio"),
                                     table,
                                     5,
                                     dynamics_labels);

      dynamics_output_maping_row_gui(G_OBJECT(editor->dynamics_model),
                                     "color",
                                     _("Color"),
                                     table,
                                     6,
                                     dynamics_labels);

      dynamics_output_maping_row_gui(G_OBJECT(editor->dynamics_model),
                                     "angle",
                                     _("Angle"),
                                     table,
                                     7,
                                     dynamics_labels);

      dynamics_output_maping_row_gui(G_OBJECT(editor->dynamics_model),
                                     "jitter",
                                     _("Jitter"),
                                     table,
                                     8,
                                     dynamics_labels);
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

static void
dynamics_output_maping_row_gui(GObject     *config,
                               const gchar *property_name_part,
                               const gchar *property_label,
                               GtkTable    *table,
                               gint         row,
                               GtkWidget   *labels[])
{
      GtkWidget        *label;
      GtkWidget        *button;
      gint              column=1;

      label = gtk_label_new (property_label);
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row+1,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (label);

/*gboolean  pressure;
  gboolean  velocity;
  gboolean  direction;
  gboolean  tilt;
  gboolean  random;
  gboolean  fade;
*/

      button = dynamics_check_button_new (config,  g_strconcat("pressure-", property_name_part, NULL),
                                          table, column, row);
      g_signal_connect (button, "size-allocate",
                        G_CALLBACK (dynamics_check_button_size_allocate),
                        labels[column - 1]);
      column++;

      button = dynamics_check_button_new (config,  g_strconcat("velocity-", property_name_part, NULL),
                                          table, column, row);
      g_signal_connect (button, "size-allocate",
                        G_CALLBACK (dynamics_check_button_size_allocate),
                        labels[column - 1]);
      column++;

      button = dynamics_check_button_new (config,  g_strconcat("direction-", property_name_part, NULL),
                                          table, column, row);
      g_signal_connect (button, "size-allocate",
                        G_CALLBACK (dynamics_check_button_size_allocate),
                        labels[column - 1]);
      column++;

      button = dynamics_check_button_new (config,  g_strconcat("tilt-", property_name_part, NULL),
                                          table, column, row);
      g_signal_connect (button, "size-allocate",
                        G_CALLBACK (dynamics_check_button_size_allocate),
                        labels[column - 1]);
      column++;

      button = dynamics_check_button_new (config,  g_strconcat("random-", property_name_part, NULL),
                                          table, column, row);
      g_signal_connect (button, "size-allocate",
                        G_CALLBACK (dynamics_check_button_size_allocate),
                        labels[column - 1]);
      column++;

      button = dynamics_check_button_new (config,  g_strconcat("fading-", property_name_part, NULL),
                                          table, column, row);
      g_signal_connect (button, "size-allocate",
                        G_CALLBACK (dynamics_check_button_size_allocate),
                        labels[column - 1]);
      column++;

}

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
