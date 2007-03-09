/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbrusheditor.c
 * Copyright 1998 Jay Cox <jaycox@earthlink.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpbrushgenerated.h"
#include "core/gimpcontext.h"

#include "gimpbrusheditor.h"
#include "gimpdocked.h"
#include "gimpview.h"
#include "gimpviewrenderer.h"

#include "gimp-intl.h"


#define BRUSH_VIEW_WIDTH  128
#define BRUSH_VIEW_HEIGHT  96


/*  local function prototypes  */

static void   gimp_brush_editor_docked_iface_init (GimpDockedInterface *face);

static GObject * gimp_brush_editor_constructor (GType              type,
                                                guint              n_params,
                                                GObjectConstructParam *params);

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

  object_class->constructor = gimp_brush_editor_constructor;

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
  GtkWidget *frame;
  GtkWidget *box;
  gint       row = 0;

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (editor), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  editor->view = gimp_view_new_full_by_types (NULL,
                                              GIMP_TYPE_VIEW,
                                              GIMP_TYPE_BRUSH,
                                              BRUSH_VIEW_WIDTH,
                                              BRUSH_VIEW_HEIGHT, 0,
                                              FALSE, FALSE, TRUE);
  gtk_widget_set_size_request (editor->view,
                               BRUSH_VIEW_WIDTH, BRUSH_VIEW_HEIGHT);
  gimp_view_set_expand (GIMP_VIEW (editor->view), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), editor->view);
  gtk_widget_show (editor->view);

  editor->shape_group = NULL;

  /* table for sliders/labels */
  editor->options_table = gtk_table_new (4, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (editor->options_table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (editor->options_table), 2);
  gtk_box_pack_start (GTK_BOX (editor), editor->options_table, FALSE, FALSE, 0);
  gtk_widget_show (editor->options_table);

  /* Stock Box for the brush shape */
  box = gimp_enum_stock_box_new (GIMP_TYPE_BRUSH_GENERATED_SHAPE,
                                 "gimp-shape",
                                 GTK_ICON_SIZE_MENU,
                                 G_CALLBACK (gimp_brush_editor_update_shape),
                                 editor,
                                 &editor->shape_group);
  gimp_table_attach_aligned (GTK_TABLE (editor->options_table),
                             0, row++,
                             _("Shape:"), 0.0, 0.5,
                             box, 2, TRUE);
  gtk_widget_show (box);

  /*  brush radius scale  */
  editor->radius_data =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (editor->options_table),
                                          0, row++,
                                          _("Radius:"), -1, 5,
                                          0.0, 0.1, 1000.0, 0.1, 1.0, 1,
                                          TRUE, 0.0, 0.0,
                                          NULL, NULL));

  gimp_scale_entry_set_logarithmic (GTK_OBJECT (editor->radius_data), TRUE);

  g_signal_connect (editor->radius_data, "value-changed",
                    G_CALLBACK (gimp_brush_editor_update_brush),
                    editor);

  /*  number of spikes  */
  editor->spikes_data =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (editor->options_table),
                                          0, row++,
                                          _("Spikes:"), -1, 5,
                                          2.0, 2.0, 20.0, 1.0, 1.0, 0,
                                          TRUE, 0.0, 0.0,
                                          NULL, NULL));

  g_signal_connect (editor->spikes_data, "value-changed",
                    G_CALLBACK (gimp_brush_editor_update_brush),
                    editor);

  /*  brush hardness scale  */
  editor->hardness_data =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (editor->options_table),
                                          0, row++,
                                          _("Hardness:"), -1, 5,
                                          0.0, 0.0, 1.0, 0.01, 0.1, 2,
                                          TRUE, 0.0, 0.0,
                                          NULL, NULL));

  g_signal_connect (editor->hardness_data, "value-changed",
                    G_CALLBACK (gimp_brush_editor_update_brush),
                    editor);

  /*  brush aspect ratio scale  */
  editor->aspect_ratio_data =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (editor->options_table),
                                          0, row++,
                                          _("Aspect ratio:"), -1, 5,
                                          0.0, 1.0, 20.0, 0.1, 1.0, 1,
                                          TRUE, 0.0, 0.0,
                                          NULL, NULL));

  g_signal_connect (editor->aspect_ratio_data,"value-changed",
                    G_CALLBACK (gimp_brush_editor_update_brush),
                    editor);

  /*  brush angle scale  */
  editor->angle_data =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (editor->options_table),
                                          0, row++,
                                          _("Angle:"), -1, 5,
                                          0.0, 0.0, 180.0, 0.1, 1.0, 1,
                                          TRUE, 0.0, 0.0,
                                          NULL, NULL));

  g_signal_connect (editor->angle_data, "value-changed",
                    G_CALLBACK (gimp_brush_editor_update_brush),
                    editor);

  /*  brush spacing  */
  editor->spacing_data =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (editor->options_table),
                                          0, row++,
                                          _("Spacing:"), -1, 5,
                                          0.0, 1.0, 200.0, 1.0, 10.0, 1,
                                          FALSE, 1.0, 5000.0,
                                          _("Percentage of width of brush"),
                                          NULL));

  g_signal_connect (editor->spacing_data, "value-changed",
                    G_CALLBACK (gimp_brush_editor_update_brush),
                    editor);
}

static GObject *
gimp_brush_editor_constructor (GType                  type,
                               guint                  n_params,
                               GObjectConstructParam *params)
{
  GObject *object;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  gimp_docked_set_show_button_bar (GIMP_DOCKED (object), FALSE);

  return object;
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

  gimp_view_set_viewable (GIMP_VIEW (brush_editor->view),
                          (GimpViewable *) data);

  if (editor->data && GIMP_IS_BRUSH_GENERATED (editor->data))
    {
      GimpBrushGenerated *brush = GIMP_BRUSH_GENERATED (editor->data);

      shape    = gimp_brush_generated_get_shape        (brush);
      radius   = gimp_brush_generated_get_radius       (brush);
      spikes   = gimp_brush_generated_get_spikes       (brush);
      hardness = gimp_brush_generated_get_hardness     (brush);
      ratio    = gimp_brush_generated_get_aspect_ratio (brush);
      angle    = gimp_brush_generated_get_angle        (brush);
      spacing  = gimp_brush_get_spacing                (GIMP_BRUSH (brush));
    }

  gtk_widget_set_sensitive (brush_editor->options_table,
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
  GimpBrushEditor *editor = GIMP_BRUSH_EDITOR (docked);

  parent_docked_iface->set_context (docked, context);

  gimp_view_renderer_set_context (GIMP_VIEW (editor->view)->renderer,
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
  gdouble             radius;
  gint                spikes;
  gdouble             hardness;
  gdouble             ratio;
  gdouble             angle;
  gdouble             spacing;

  if (! GIMP_IS_BRUSH_GENERATED (GIMP_DATA_EDITOR (editor)->data))
    return;

  brush = GIMP_BRUSH_GENERATED (GIMP_DATA_EDITOR (editor)->data);

  radius   = editor->radius_data->value;
  spikes   = ROUND (editor->spikes_data->value);
  hardness = editor->hardness_data->value;
  ratio    = editor->aspect_ratio_data->value;
  angle    = editor->angle_data->value;
  spacing  = editor->spacing_data->value;

  if (radius   != gimp_brush_generated_get_radius       (brush) ||
      spikes   != gimp_brush_generated_get_spikes       (brush) ||
      hardness != gimp_brush_generated_get_hardness     (brush) ||
      ratio    != gimp_brush_generated_get_aspect_ratio (brush) ||
      angle    != gimp_brush_generated_get_angle        (brush) ||
      spacing  != gimp_brush_get_spacing                (GIMP_BRUSH (brush)))
    {
      g_signal_handlers_block_by_func (brush,
                                       gimp_brush_editor_notify_brush,
                                       editor);

      gimp_data_freeze (GIMP_DATA (brush));
      g_object_freeze_notify (G_OBJECT (brush));

      gimp_brush_generated_set_radius       (brush, radius);
      gimp_brush_generated_set_spikes       (brush, spikes);
      gimp_brush_generated_set_hardness     (brush, hardness);
      gimp_brush_generated_set_aspect_ratio (brush, ratio);
      gimp_brush_generated_set_angle        (brush, angle);
      gimp_brush_set_spacing                (GIMP_BRUSH (brush), spacing);

      g_object_thaw_notify (G_OBJECT (brush));
      gimp_data_thaw (GIMP_DATA (brush));

      g_signal_handlers_unblock_by_func (brush,
                                         gimp_brush_editor_notify_brush,
                                         editor);
    }
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
gimp_brush_editor_notify_brush (GimpBrushGenerated   *brush,
                                GParamSpec           *pspec,
                                GimpBrushEditor      *editor)
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

      adj   = editor->radius_data;
      value = brush->radius;
    }
  else if (! strcmp (pspec->name, "radius"))
    {
      adj   = editor->radius_data;
      value = brush->radius;
    }
  else if (! strcmp (pspec->name, "spikes"))
    {
      adj   = editor->spikes_data;
      value = brush->spikes;
    }
  else if (! strcmp (pspec->name, "hardness"))
    {
      adj   = editor->hardness_data;
      value = brush->hardness;
    }
  else if (! strcmp (pspec->name, "angle"))
    {
      adj   = editor->angle_data;
      value = brush->angle;
    }
  else if (! strcmp (pspec->name, "aspect-ratio"))
    {
      adj   = editor->aspect_ratio_data;
      value = brush->aspect_ratio;
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
