/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbrusheditor.c
 * Copyright 1998 Jay Cox <jaycox@earthlink.net>
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpbrushgenerated.h"
#include "core/gimpcontext.h"

#include "gimpbrusheditor.h"
#include "gimpdocked.h"
#include "gimpspinscale.h"
#include "gimpview.h"
#include "gimpviewrenderer.h"

#include "gimp-intl.h"


#define BRUSH_VIEW_SIZE 96


/*  local function prototypes  */

static void   gimp_brush_editor_docked_iface_init (GimpDockedInterface *face);

static void   gimp_brush_editor_constructed    (GObject            *object);

static void   gimp_brush_editor_set_data       (GimpDataEditor     *editor,
                                                GimpData           *data);

static void   gimp_brush_editor_set_context    (GimpDocked         *docked,
                                                GimpContext        *context);

static void   gimp_brush_editor_update_brush   (GtkAdjustment      *adjustment,
                                                GimpBrushEditor    *editor);
static void   gimp_brush_editor_update_shape   (GtkWidget          *widget,
                                                GimpBrushEditor    *editor);
static void   gimp_brush_editor_notify_brush   (GimpBrushGenerated *brush,
                                                GParamSpec         *pspec,
                                                GimpBrushEditor    *editor);


G_DEFINE_TYPE_WITH_CODE (GimpBrushEditor, gimp_brush_editor,
                         GIMP_TYPE_DATA_EDITOR,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCKED,
                                                gimp_brush_editor_docked_iface_init))

#define parent_class gimp_brush_editor_parent_class

static GimpDockedInterface *parent_docked_iface = NULL;


static void
gimp_brush_editor_class_init (GimpBrushEditorClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpDataEditorClass *editor_class = GIMP_DATA_EDITOR_CLASS (klass);

  object_class->constructed = gimp_brush_editor_constructed;

  editor_class->set_data    = gimp_brush_editor_set_data;
  editor_class->title       = _("Brush Editor");
}

static void
gimp_brush_editor_docked_iface_init (GimpDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (GIMP_TYPE_DOCKED);

  iface->set_context = gimp_brush_editor_set_context;
}

static void
gimp_brush_editor_init (GimpBrushEditor *editor)
{
  GimpDataEditor *data_editor = GIMP_DATA_EDITOR (editor);
  GtkWidget      *frame;
  GtkWidget      *hbox;
  GtkWidget      *label;
  GtkWidget      *box;
  GtkWidget      *scale;

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (editor), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  data_editor->view = gimp_view_new_full_by_types (NULL,
                                                   GIMP_TYPE_VIEW,
                                                   GIMP_TYPE_BRUSH,
                                                   BRUSH_VIEW_SIZE,
                                                   BRUSH_VIEW_SIZE, 0,
                                                   FALSE, FALSE, TRUE);
  gtk_widget_set_size_request (data_editor->view, -1, BRUSH_VIEW_SIZE);
  gimp_view_set_expand (GIMP_VIEW (data_editor->view), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), data_editor->view);
  gtk_widget_show (data_editor->view);

  editor->shape_group = NULL;

  editor->options_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (editor), editor->options_box, FALSE, FALSE, 0);
  gtk_widget_show (editor->options_box);

  /* Stock Box for the brush shape */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (editor->options_box), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Shape:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  box = gimp_enum_icon_box_new (GIMP_TYPE_BRUSH_GENERATED_SHAPE,
                                "gimp-shape",
                                GTK_ICON_SIZE_MENU,
                                G_CALLBACK (gimp_brush_editor_update_shape),
                                editor,
                                &editor->shape_group);
  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 0);
  gtk_widget_show (box);

  /*  brush radius scale  */
  editor->radius_data = gtk_adjustment_new (0.0, 0.1, 1000.0, 0.1, 1.0, 0.0);
  scale = gimp_spin_scale_new (editor->radius_data, _("Radius"), 1);
  gtk_box_pack_start (GTK_BOX (editor->options_box), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  g_signal_connect (editor->radius_data, "value-changed",
                    G_CALLBACK (gimp_brush_editor_update_brush),
                    editor);

  /*  number of spikes  */
  editor->spikes_data = gtk_adjustment_new (2.0, 2.0, 20.0, 1.0, 1.0, 0.0);
  scale = gimp_spin_scale_new (editor->spikes_data, _("Spikes"), 0);
  gtk_box_pack_start (GTK_BOX (editor->options_box), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  g_signal_connect (editor->spikes_data, "value-changed",
                    G_CALLBACK (gimp_brush_editor_update_brush),
                    editor);

  /*  brush hardness scale  */
  editor->hardness_data = gtk_adjustment_new (0.0, 0.0, 1.0, 0.01, 0.1, 0.0);
  scale = gimp_spin_scale_new (editor->hardness_data, _("Hardness"), 2);
  gtk_box_pack_start (GTK_BOX (editor->options_box), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  g_signal_connect (editor->hardness_data, "value-changed",
                    G_CALLBACK (gimp_brush_editor_update_brush),
                    editor);

  /*  brush aspect ratio scale  */
  editor->aspect_ratio_data = gtk_adjustment_new (0.0, 1.0, 20.0, 0.1, 1.0, 0.0);
  scale = gimp_spin_scale_new (editor->aspect_ratio_data, _("Aspect ratio"), 1);
  gtk_box_pack_start (GTK_BOX (editor->options_box), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  g_signal_connect (editor->aspect_ratio_data,"value-changed",
                    G_CALLBACK (gimp_brush_editor_update_brush),
                    editor);

  /*  brush angle scale  */
  editor->angle_data = gtk_adjustment_new (0.0, 0.0, 180.0, 0.1, 1.0, 0.0);
  scale = gimp_spin_scale_new (editor->angle_data, _("Angle"), 1);
  gtk_box_pack_start (GTK_BOX (editor->options_box), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  g_signal_connect (editor->angle_data, "value-changed",
                    G_CALLBACK (gimp_brush_editor_update_brush),
                    editor);

  /*  brush spacing  */
  editor->spacing_data = gtk_adjustment_new (0.0, 1.0, 5000.0, 1.0, 10.0, 0.0);
  scale = gimp_spin_scale_new (editor->spacing_data, _("Spacing"), 1);
  gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (scale), 1.0, 200.0);
  gtk_box_pack_start (GTK_BOX (editor->options_box), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  gimp_help_set_help_data (scale, _("Percentage of width of brush"), NULL);

  g_signal_connect (editor->spacing_data, "value-changed",
                    G_CALLBACK (gimp_brush_editor_update_brush),
                    editor);
}

static void
gimp_brush_editor_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_docked_set_show_button_bar (GIMP_DOCKED (object), FALSE);
}

static void
gimp_brush_editor_set_data (GimpDataEditor *editor,
                            GimpData       *data)
{
  GimpBrushEditor         *brush_editor = GIMP_BRUSH_EDITOR (editor);
  GimpBrushGeneratedShape  shape        = GIMP_BRUSH_GENERATED_CIRCLE;
  gdouble                  radius       = 0.0;
  gint                     spikes       = 2;
  gdouble                  hardness     = 0.0;
  gdouble                  ratio        = 0.0;
  gdouble                  angle        = 0.0;
  gdouble                  spacing      = 0.0;

  if (editor->data)
    g_signal_handlers_disconnect_by_func (editor->data,
                                          gimp_brush_editor_notify_brush,
                                          editor);

  GIMP_DATA_EDITOR_CLASS (parent_class)->set_data (editor, data);

  if (editor->data)
    g_signal_connect (editor->data, "notify",
                      G_CALLBACK (gimp_brush_editor_notify_brush),
                      editor);

  gimp_view_set_viewable (GIMP_VIEW (editor->view), GIMP_VIEWABLE (data));

  if (editor->data)
    {
      spacing = gimp_brush_get_spacing (GIMP_BRUSH (editor->data));

      if (GIMP_IS_BRUSH_GENERATED (editor->data))
        {
          GimpBrushGenerated *brush = GIMP_BRUSH_GENERATED (editor->data);

          shape    = gimp_brush_generated_get_shape        (brush);
          radius   = gimp_brush_generated_get_radius       (brush);
          spikes   = gimp_brush_generated_get_spikes       (brush);
          hardness = gimp_brush_generated_get_hardness     (brush);
          ratio    = gimp_brush_generated_get_aspect_ratio (brush);
          angle    = gimp_brush_generated_get_angle        (brush);
        }
    }

  gtk_widget_set_sensitive (brush_editor->options_box,
                            editor->data_editable);

  gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (brush_editor->shape_group),
                                   shape);

  gtk_adjustment_set_value (brush_editor->radius_data,       radius);
  gtk_adjustment_set_value (brush_editor->spikes_data,       spikes);
  gtk_adjustment_set_value (brush_editor->hardness_data,     hardness);
  gtk_adjustment_set_value (brush_editor->aspect_ratio_data, ratio);
  gtk_adjustment_set_value (brush_editor->angle_data,        angle);
  gtk_adjustment_set_value (brush_editor->spacing_data,      spacing);
}

static void
gimp_brush_editor_set_context (GimpDocked  *docked,
                               GimpContext *context)
{
  GimpDataEditor *data_editor = GIMP_DATA_EDITOR (docked);

  parent_docked_iface->set_context (docked, context);

  gimp_view_renderer_set_context (GIMP_VIEW (data_editor->view)->renderer,
                                  context);
}


/*  public functions  */

GtkWidget *
gimp_brush_editor_new (GimpContext     *context,
                       GimpMenuFactory *menu_factory)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return g_object_new (GIMP_TYPE_BRUSH_EDITOR,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<BrushEditor>",
                       "ui-path",         "/brush-editor-popup",
                       "data-factory",    context->gimp->brush_factory,
                       "context",         context,
                       "data",            gimp_context_get_brush (context),
                       NULL);
}


/*  private functions  */

static void
gimp_brush_editor_update_brush (GtkAdjustment   *adjustment,
                                GimpBrushEditor *editor)
{
  GimpBrushGenerated *brush;
  gdouble             value;

  if (! GIMP_IS_BRUSH_GENERATED (GIMP_DATA_EDITOR (editor)->data))
    return;

  brush = GIMP_BRUSH_GENERATED (GIMP_DATA_EDITOR (editor)->data);

  g_signal_handlers_block_by_func (brush,
                                   gimp_brush_editor_notify_brush,
                                   editor);

  value = gtk_adjustment_get_value (adjustment);

  if (adjustment == editor->radius_data)
    {
      if (value != gimp_brush_generated_get_radius (brush))
        gimp_brush_generated_set_radius (brush, value);
    }
  else if (adjustment == editor->spikes_data)
    {
      if (ROUND (value) != gimp_brush_generated_get_spikes (brush))
        gimp_brush_generated_set_spikes (brush, ROUND (value));
    }
  else if (adjustment == editor->hardness_data)
    {
      if (value != gimp_brush_generated_get_hardness (brush))
        gimp_brush_generated_set_hardness (brush, value);
    }
  else if (adjustment == editor->aspect_ratio_data)
    {
      if (value != gimp_brush_generated_get_aspect_ratio (brush))
        gimp_brush_generated_set_aspect_ratio (brush, value);
    }
  else if (adjustment == editor->angle_data)
    {
      if (value != gimp_brush_generated_get_angle (brush))
        gimp_brush_generated_set_angle (brush, value);
    }
  else if (adjustment == editor->spacing_data)
    {
      if (value != gimp_brush_get_spacing (GIMP_BRUSH (brush)))
        gimp_brush_set_spacing (GIMP_BRUSH (brush), value);
    }

  g_signal_handlers_unblock_by_func (brush,
                                     gimp_brush_editor_notify_brush,
                                     editor);
}

static void
gimp_brush_editor_update_shape (GtkWidget       *widget,
                                GimpBrushEditor *editor)
{
  GimpBrushGenerated *brush;

  if (! GIMP_IS_BRUSH_GENERATED (GIMP_DATA_EDITOR (editor)->data))
    return;

  brush = GIMP_BRUSH_GENERATED (GIMP_DATA_EDITOR (editor)->data);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      GimpBrushGeneratedShape shape;

      shape = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                  "gimp-item-data"));

      if (gimp_brush_generated_get_shape (brush) != shape)
        gimp_brush_generated_set_shape (brush, shape);
    }
}

static void
gimp_brush_editor_notify_brush (GimpBrushGenerated *brush,
                                GParamSpec         *pspec,
                                GimpBrushEditor    *editor)
{
  GtkAdjustment *adj   = NULL;
  gdouble        value = 0.0;

  if (! strcmp (pspec->name, "shape"))
    {
      g_signal_handlers_block_by_func (editor->shape_group,
                                       gimp_brush_editor_update_shape,
                                       editor);

      gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (editor->shape_group),
                                       brush->shape);

      g_signal_handlers_unblock_by_func (editor->shape_group,
                                         gimp_brush_editor_update_shape,
                                         editor);
    }
  else if (! strcmp (pspec->name, "radius"))
    {
      adj   = editor->radius_data;
      value = gimp_brush_generated_get_radius (brush);
    }
  else if (! strcmp (pspec->name, "spikes"))
    {
      adj   = editor->spikes_data;
      value = gimp_brush_generated_get_spikes (brush);
    }
  else if (! strcmp (pspec->name, "hardness"))
    {
      adj   = editor->hardness_data;
      value = gimp_brush_generated_get_hardness (brush);
    }
  else if (! strcmp (pspec->name, "angle"))
    {
      adj   = editor->angle_data;
      value = gimp_brush_generated_get_angle (brush);
    }
  else if (! strcmp (pspec->name, "aspect-ratio"))
    {
      adj   = editor->aspect_ratio_data;
      value = gimp_brush_generated_get_aspect_ratio (brush);
    }
  else if (! strcmp (pspec->name, "spacing"))
    {
      adj   = editor->spacing_data;
      value = gimp_brush_get_spacing (GIMP_BRUSH (brush));
    }

  if (adj)
    {
      g_signal_handlers_block_by_func (adj,
                                       gimp_brush_editor_update_brush,
                                       editor);

      gtk_adjustment_set_value (adj, value);

      g_signal_handlers_unblock_by_func (adj,
                                         gimp_brush_editor_update_brush,
                                         editor);
    }
}
