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

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
//#include "core/gimpbrushgenerated.h"
#include "core/gimpcontext.h"

#include "gimpdocked.h"
#include "gimpview.h"
#include "gimpviewrenderer.h"

#include "gimp-intl.h"

#include "gimpmenufactory.h"
#include "widgets/gimpdynamicseditor.h"

#include "core/gimpbrush.h"
//To do:
// discard unneeded ones.
// needs to be fixed to gimppaintdynamics.h when that works.

/*
#include "paint/gimppaintoptions.h"

#include "core/gimptoolinfo.h"

#include "widgets/gimppropwidgets.h"
#include "widgets/gimpviewablebox.h"
#include "widgets/gimpwidgets-utils.h"

#include "tools/gimpairbrushtool.h"
#include "tools/gimpclonetool.h"
#include "tools/gimpconvolvetool.h"
#include "tools/gimpdodgeburntool.h"
#include "tools/gimperasertool.h"
#include "tools/gimphealtool.h"
#include "tools/gimpinktool.h"
#include "tools/gimppaintoptions-gui.h"
#include "tools/gimppenciltool.h"
#include "tools/gimpperspectiveclonetool.h"
#include "tools/gimpsmudgetool.h"
#include "tools/gimptooloptions-gui.h"

*/


/*  local function prototypes  */

static void   gimp_dynamics_editor_docked_iface_init (GimpDockedInterface *face);

static GObject * gimp_dynamics_editor_constructor (GType              type,
                                                   guint              n_params,
                                                   GObjectConstructParam *params);

//static void   gimp_dynamics_editor_set_data       (GimpDataEditor     *editor,
//                                                   GimpData           *data);

static void   gimp_dynamics_editor_set_context    (GimpDocked         *docked,
                                                   GimpContext        *context);

/*dynamics options gui*/
/*
static gboolean    tool_has_opacity_dynamics      (GType       tool_type);
static gboolean    tool_has_hardness_dynamics     (GType       tool_type);
static gboolean    tool_has_rate_dynamics         (GType       tool_type);
static gboolean    tool_has_size_dynamics         (GType       tool_type);
static gboolean    tool_has_color_dynamics        (GType       tool_type);
static gboolean    tool_has_angle_dynamics        (GType       tool_type);
static gboolean    tool_has_aspect_ratio_dynamics (GType       tool_type);

static void        pressure_options_gui  (GimpPaintOptions *paint_options,
                                          GType             tool_type,
                                          GtkTable         *table,
                                          gint              row,
                                          GtkWidget        *labels[]);
static void        velocity_options_gui  (GimpPaintOptions *paint_options,
                                          GType             tool_type,
                                          GtkTable         *table,
                                          gint              row);
static void        direction_options_gui (GimpPaintOptions *paint_options,
                                          GType             tool_type,
                                          GtkTable         *table,
                                          gint              row);
static void        tilt_options_gui      (GimpPaintOptions *paint_options,
                                          GType             tool_type,
                                          GtkTable         *table,
                                          gint              row);
static void        random_options_gui    (GimpPaintOptions *paint_options,
                                          GType             tool_type,
                                          GtkTable         *table,
                                          gint              row);

static void        fading_options_gui    (GimpPaintOptions *paint_options,
                                          GType             tool_type,
                                          GtkTable         *table,
                                          gint              row);
*/


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

  //editor_class->set_data    = gimp_dynamics_editor_set_data;
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

/*
To do:
look at other init for dataeditors
look at how to move gui codes from paintopitons to here
*/


static void
gimp_dynamics_editor_init (GimpDynamicsEditor *editor)
{
  GimpDataEditor *data_editor = GIMP_DATA_EDITOR (editor);

  GtkWidget      *frame;
  //GtkWidget      *box;
  //gint            row = 0;

  //add a frame
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (editor), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  data_editor->view = gimp_view_new_full_by_types (NULL,
                                                   GIMP_TYPE_VIEW,
                                                   GIMP_TYPE_BRUSH,
                                                   DYNAMICS_VIEW_SIZE,
                                                   DYNAMICS_VIEW_SIZE, 0,
                                                   FALSE, FALSE, TRUE);
  gtk_widget_set_size_request (data_editor->view, -1, DYNAMICS_VIEW_SIZE);
  gimp_view_set_expand (GIMP_VIEW (data_editor->view), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), data_editor->view);
  gtk_widget_show (data_editor->view);

  //editor->shape_group = NULL;

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

  return g_object_new (GIMP_TYPE_DYNAMICS_EDITOR,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<DynamicsEditor>",
                       "ui-path",         "/dynamics-editor-popup",
                       "data-factory",    context->gimp->brush_factory,
                       "context",         context,
                       "data",            gimp_context_get_brush (context),
                       NULL);

/*
  return g_object_new (GIMP_TYPE_DYNAMICS_EDITOR,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<DynamicsEditor>",
                       "ui-path",         "/dynamics-editor-popup",
                       "data-factory",    context->gimp->brush_factory,
                       "context",         context,
                       "data",            gimp_context_get_brush (context),
                       NULL);
*/
}



/*  private functions  */
/*
static gboolean
tool_has_opacity_dynamics (GType tool_type)
{
  return (g_type_is_a (tool_type, GIMP_TYPE_PAINTBRUSH_TOOL) ||
          tool_type == GIMP_TYPE_CLONE_TOOL             ||
          tool_type == GIMP_TYPE_HEAL_TOOL              ||
          tool_type == GIMP_TYPE_PERSPECTIVE_CLONE_TOOL ||
          tool_type == GIMP_TYPE_DODGE_BURN_TOOL        ||
          tool_type == GIMP_TYPE_ERASER_TOOL);
}

static gboolean
tool_has_hardness_dynamics (GType tool_type)
{
  return (tool_type == GIMP_TYPE_AIRBRUSH_TOOL          ||
          tool_type == GIMP_TYPE_CLONE_TOOL             ||
          tool_type == GIMP_TYPE_HEAL_TOOL              ||
          tool_type == GIMP_TYPE_PERSPECTIVE_CLONE_TOOL ||
          tool_type == GIMP_TYPE_CONVOLVE_TOOL          ||
          tool_type == GIMP_TYPE_ERASER_TOOL            ||
          tool_type == GIMP_TYPE_DODGE_BURN_TOOL        ||
          tool_type == GIMP_TYPE_PAINTBRUSH_TOOL        ||
          tool_type == GIMP_TYPE_SMUDGE_TOOL);
}

static gboolean
tool_has_rate_dynamics (GType tool_type)
{
  return (tool_type == GIMP_TYPE_AIRBRUSH_TOOL          ||
          tool_type == GIMP_TYPE_CONVOLVE_TOOL          ||
          tool_type == GIMP_TYPE_SMUDGE_TOOL);
}

static gboolean
tool_has_size_dynamics (GType tool_type)
{
  return (g_type_is_a (tool_type, GIMP_TYPE_PAINTBRUSH_TOOL) ||
          tool_type == GIMP_TYPE_CLONE_TOOL             ||
          tool_type == GIMP_TYPE_HEAL_TOOL              ||
          tool_type == GIMP_TYPE_PERSPECTIVE_CLONE_TOOL ||
          tool_type == GIMP_TYPE_CONVOLVE_TOOL          ||
          tool_type == GIMP_TYPE_DODGE_BURN_TOOL        ||
          tool_type == GIMP_TYPE_ERASER_TOOL);
}

static gboolean
tool_has_aspect_ratio_dynamics (GType tool_type)
{
  return (g_type_is_a (tool_type, GIMP_TYPE_PAINTBRUSH_TOOL) ||
          tool_type == GIMP_TYPE_CLONE_TOOL             ||
          tool_type == GIMP_TYPE_HEAL_TOOL              ||
          tool_type == GIMP_TYPE_PERSPECTIVE_CLONE_TOOL ||
          tool_type == GIMP_TYPE_CONVOLVE_TOOL          ||
          tool_type == GIMP_TYPE_DODGE_BURN_TOOL        ||
          tool_type == GIMP_TYPE_ERASER_TOOL);
}

static gboolean
tool_has_angle_dynamics (GType tool_type)
{
  return (g_type_is_a (tool_type, GIMP_TYPE_PAINTBRUSH_TOOL));
}

static gboolean
tool_has_color_dynamics (GType tool_type)
{
  return (g_type_is_a (tool_type, GIMP_TYPE_PAINTBRUSH_TOOL));
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

static void
pressure_options_gui (GimpPaintOptions *paint_options,
                      GType             tool_type,
                      GtkTable         *table,
                      gint              row,
                      GtkWidget        *labels[])
{
  GObject   *config = G_OBJECT (paint_options);
  GtkWidget *button;
  gint       column = 1;
  GtkWidget *scalebutton;

  if (tool_has_opacity_dynamics (tool_type))
    {
      button = dynamics_check_button_new (config, "pressure-opacity",
                                          table, column, row);
      g_signal_connect (button, "size-allocate",
                        G_CALLBACK (dynamics_check_button_size_allocate),
                        labels[column - 1]);
      column++;
    }

  if (tool_has_hardness_dynamics (tool_type))
    {
      button = dynamics_check_button_new (config, "pressure-hardness",
                                          table, column, row);
      g_signal_connect (button, "size-allocate",
                        G_CALLBACK (dynamics_check_button_size_allocate),
                        labels[column - 1]);
      column++;
    }

  if (tool_has_rate_dynamics (tool_type))
    {
      button = dynamics_check_button_new (config, "pressure-rate",
                                          table, column, row);
      g_signal_connect (button, "size-allocate",
                        G_CALLBACK (dynamics_check_button_size_allocate),
                        labels[column - 1]);
      column++;
    }

  if (tool_has_size_dynamics (tool_type))
    {
      if (tool_type != GIMP_TYPE_AIRBRUSH_TOOL)
        button = dynamics_check_button_new (config, "pressure-size",
                                            table, column, row);
      else
        button = dynamics_check_button_new (config, "pressure-inverse-size",
                                            table, column, row);

      g_signal_connect (button, "size-allocate",
                        G_CALLBACK (dynamics_check_button_size_allocate),
                        labels[column - 1]);
      column++;
    }

  if (tool_has_aspect_ratio_dynamics (tool_type))
    {
      button = dynamics_check_button_new (config, "pressure-aspect_ratio",
                                          table, column, row);

      g_signal_connect (button, "size-allocate",
                        G_CALLBACK (dynamics_check_button_size_allocate),
                        labels[column - 1]);
      column++;
     }

  if (tool_has_angle_dynamics (tool_type))
    {
      button = dynamics_check_button_new (config, "pressure-angle",
                                          table, column, row);
      g_signal_connect (button, "size-allocate",
                        G_CALLBACK (dynamics_check_button_size_allocate),
                        labels[column - 1]);
      column++;
    }

  if (tool_has_color_dynamics (tool_type))
    {
      button = dynamics_check_button_new (config, "pressure-color",
                                          table, column, row);
      g_signal_connect (button, "size-allocate",
                        G_CALLBACK (dynamics_check_button_size_allocate),
                        labels[column - 1]);
      column++;
    }

   scalebutton = gimp_prop_scale_button_new (config, "pressure-prescale");
   gtk_table_attach (table, scalebutton, column, column + 1, row, row + 1,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
   gtk_widget_show (scalebutton);
}

static void
velocity_options_gui (GimpPaintOptions *paint_options,
                      GType             tool_type,
                      GtkTable         *table,
                      gint              row)
{
  GObject   *config = G_OBJECT (paint_options);
  gint       column = 1;
  GtkWidget *scalebutton;

  if (tool_has_opacity_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "velocity-opacity",
                                 table, column++, row);
    }

  if (tool_has_hardness_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "velocity-hardness",
                                 table, column++, row);
    }

  if (tool_has_rate_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "velocity-rate",
                                 table, column++, row);
    }

  if (tool_has_size_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "velocity-size",
                                 table, column++, row);
    }

  if (tool_has_aspect_ratio_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "velocity-aspect-ratio",
                                 table, column++, row);
    }


  if (tool_has_angle_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "velocity-angle",
                                 table, column++, row);
    }

  if (tool_has_color_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "velocity-color",
                                 table, column++, row);
    }

  scalebutton = gimp_prop_scale_button_new (config, "velocity-prescale");
  gtk_table_attach (table, scalebutton, column, column + 1, row, row + 1,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (scalebutton);
}

static void
direction_options_gui (GimpPaintOptions *paint_options,
                       GType             tool_type,
                       GtkTable         *table,
                       gint              row)
{
  GObject   *config = G_OBJECT (paint_options);
  gint       column = 1;
  GtkWidget *scalebutton;

  if (tool_has_opacity_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "direction-opacity",
                                 table, column++, row);
    }

  if (tool_has_hardness_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "direction-hardness",
                                 table, column++, row);
    }

  if (tool_has_rate_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "direction-rate",
                                 table, column++, row);
    }

  if (tool_has_size_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "direction-size",
                                 table, column++, row);
    }

  if (tool_has_aspect_ratio_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "direction-aspect-ratio",
                                 table, column++, row);
    }

  if (tool_has_angle_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "direction-angle",
                                 table, column++, row);
    }

  if (tool_has_color_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "direction-color",
                                 table, column++, row);
    }

  scalebutton = gimp_prop_scale_button_new (config, "direction-prescale");
  gtk_table_attach (table, scalebutton, column, column + 1, row, row + 1,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (scalebutton);
}


static void
tilt_options_gui (GimpPaintOptions *paint_options,
                       GType             tool_type,
                       GtkTable         *table,
                       gint              row)
{
  GObject   *config = G_OBJECT (paint_options);
  gint       column = 1;
  GtkWidget *scalebutton;

  if (tool_has_opacity_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "tilt-opacity",
                                 table, column++, row);
    }

  if (tool_has_hardness_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "tilt-hardness",
                                 table, column++, row);
    }

  if (tool_has_rate_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "tilt-rate",
                                 table, column++, row);
    }

  if (tool_has_size_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "tilt-size",
                                 table, column++, row);
    }

if (tool_has_aspect_ratio_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "tilt-aspect-ratio",
                                 table, column++, row);
    }

  if (tool_has_angle_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "tilt-angle",
                                 table, column++, row);
    }

  if (tool_has_color_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "tilt-color",
                                 table, column++, row);
    }

  scalebutton = gimp_prop_scale_button_new (config, "tilt-prescale");
  gtk_table_attach (table, scalebutton, column, column + 1, row, row + 1,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (scalebutton);
}

static void
random_options_gui (GimpPaintOptions *paint_options,
                    GType             tool_type,
                    GtkTable         *table,
                    gint              row)
{
  GObject   *config = G_OBJECT (paint_options);
  gint       column = 1;
  GtkWidget *scalebutton;

  if (tool_has_opacity_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "random-opacity",
                                 table, column++, row);
    }

  if (tool_has_hardness_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "random-hardness",
                                 table, column++, row);
    }

  if (tool_has_rate_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "random-rate",
                                 table, column++, row);
    }

  if (tool_has_size_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "random-size",
                                 table, column++, row);
    }

  if (tool_has_aspect_ratio_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "random-aspect-ratio",
                                 table, column++, row);
    }

  if (tool_has_angle_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "random-angle",
                                 table, column++, row);
    }

  if (tool_has_color_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "random-color",
                                 table, column++, row);
    }

   scalebutton = gimp_prop_scale_button_new (config, "random-prescale");
   gtk_table_attach (table, scalebutton, column, column + 1, row, row + 1,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
   gtk_widget_show (scalebutton);
}

static void
fading_options_gui (GimpPaintOptions *paint_options,
                    GType             tool_type,
                    GtkTable         *table,
                    gint              row)
{
  GObject   *config = G_OBJECT (paint_options);
  gint       column = 1;
  GtkWidget *scalebutton;

  if (tool_has_opacity_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "fading-opacity",
                                 table, column++, row);
    }

  if (tool_has_hardness_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "fading-hardness",
                                 table, column++, row);
    }

  if (tool_has_rate_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "fading-rate",
                                 table, column++, row);
    }

  if (tool_has_size_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "fading-size",
                                 table, column++, row);
    }

  if (tool_has_aspect_ratio_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "fading-aspect-ratio",
                                 table, column++, row);
    }

  if (tool_has_angle_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "fading-angle",
                                 table, column++, row);
    }

  if (tool_has_color_dynamics (tool_type))
    {
      dynamics_check_button_new (config, "fading-color",
                                 table, column++, row);
    }

   scalebutton = gimp_prop_scale_button_new (config, "fading-prescale");
   gtk_table_attach (table, scalebutton, column, column + 1, row, row + 1,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
   gtk_widget_show (scalebutton);
}

*/
/*
GtkWidget *
gimp_paint_options_gui (GimpToolOptions *tool_options)
{

  GObject          *config  = G_OBJECT (tool_options);
  GimpPaintOptions *options = GIMP_PAINT_OPTIONS (tool_options);
  GtkWidget        *vbox    = gimp_tool_options_gui (tool_options);
  GtkWidget        *frame;
  GtkWidget        *table;
  GtkWidget        *menu;
  GtkWidget        *label;
  GtkWidget        *button;
  GtkWidget        *incremental_toggle = NULL;
  gint              table_row          = 0;
  gint              n_dynamics         = 0;
  GtkWidget        *dynamics_labels[7];
  GType             tool_type;
}
*/
