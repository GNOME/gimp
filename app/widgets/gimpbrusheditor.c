/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmabrusheditor.c
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmabrushgenerated.h"
#include "core/ligmacontext.h"

#include "ligmabrusheditor.h"
#include "ligmadocked.h"
#include "ligmaview.h"
#include "ligmaviewrenderer.h"

#include "ligma-intl.h"


#define BRUSH_VIEW_SIZE 96


/*  local function prototypes  */

static void   ligma_brush_editor_docked_iface_init (LigmaDockedInterface *face);

static void   ligma_brush_editor_constructed    (GObject            *object);

static void   ligma_brush_editor_set_data       (LigmaDataEditor     *editor,
                                                LigmaData           *data);

static void   ligma_brush_editor_set_context    (LigmaDocked         *docked,
                                                LigmaContext        *context);

static void   ligma_brush_editor_update_brush   (GtkAdjustment      *adjustment,
                                                LigmaBrushEditor    *editor);
static void   ligma_brush_editor_update_shape   (GtkWidget          *widget,
                                                LigmaBrushEditor    *editor);
static void   ligma_brush_editor_notify_brush   (LigmaBrushGenerated *brush,
                                                GParamSpec         *pspec,
                                                LigmaBrushEditor    *editor);


G_DEFINE_TYPE_WITH_CODE (LigmaBrushEditor, ligma_brush_editor,
                         LIGMA_TYPE_DATA_EDITOR,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_DOCKED,
                                                ligma_brush_editor_docked_iface_init))

#define parent_class ligma_brush_editor_parent_class

static LigmaDockedInterface *parent_docked_iface = NULL;


static void
ligma_brush_editor_class_init (LigmaBrushEditorClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  LigmaDataEditorClass *editor_class = LIGMA_DATA_EDITOR_CLASS (klass);

  object_class->constructed = ligma_brush_editor_constructed;

  editor_class->set_data    = ligma_brush_editor_set_data;
  editor_class->title       = _("Brush Editor");
}

static void
ligma_brush_editor_docked_iface_init (LigmaDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (LIGMA_TYPE_DOCKED);

  iface->set_context = ligma_brush_editor_set_context;
}

static void
ligma_brush_editor_init (LigmaBrushEditor *editor)
{
  LigmaDataEditor *data_editor = LIGMA_DATA_EDITOR (editor);
  GtkWidget      *frame;
  GtkWidget      *hbox;
  GtkWidget      *label;
  GtkWidget      *box;
  GtkWidget      *scale;

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (editor), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  data_editor->view = ligma_view_new_full_by_types (NULL,
                                                   LIGMA_TYPE_VIEW,
                                                   LIGMA_TYPE_BRUSH,
                                                   BRUSH_VIEW_SIZE,
                                                   BRUSH_VIEW_SIZE, 0,
                                                   FALSE, FALSE, TRUE);
  gtk_widget_set_size_request (data_editor->view, -1, BRUSH_VIEW_SIZE);
  ligma_view_set_expand (LIGMA_VIEW (data_editor->view), TRUE);
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

  box = ligma_enum_icon_box_new (LIGMA_TYPE_BRUSH_GENERATED_SHAPE,
                                "ligma-shape",
                                GTK_ICON_SIZE_MENU,
                                G_CALLBACK (ligma_brush_editor_update_shape),
                                editor, NULL,
                                &editor->shape_group);
  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 0);
  gtk_widget_show (box);

  /*  brush radius scale  */
  editor->radius_data = gtk_adjustment_new (0.0, 0.1, 1000.0, 1.0, 10.0, 0.0);
  scale = ligma_spin_scale_new (editor->radius_data, _("Radius"), 1);
  gtk_box_pack_start (GTK_BOX (editor->options_box), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  g_signal_connect (editor->radius_data, "value-changed",
                    G_CALLBACK (ligma_brush_editor_update_brush),
                    editor);

  /*  number of spikes  */
  editor->spikes_data = gtk_adjustment_new (2.0, 2.0, 20.0, 1.0, 1.0, 0.0);
  scale = ligma_spin_scale_new (editor->spikes_data, _("Spikes"), 0);
  gtk_box_pack_start (GTK_BOX (editor->options_box), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  g_signal_connect (editor->spikes_data, "value-changed",
                    G_CALLBACK (ligma_brush_editor_update_brush),
                    editor);

  /*  brush hardness scale  */
  editor->hardness_data = gtk_adjustment_new (0.0, 0.0, 1.0, 0.01, 0.1, 0.0);
  scale = ligma_spin_scale_new (editor->hardness_data, _("Hardness"), 2);
  gtk_box_pack_start (GTK_BOX (editor->options_box), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  g_signal_connect (editor->hardness_data, "value-changed",
                    G_CALLBACK (ligma_brush_editor_update_brush),
                    editor);

  /*  brush aspect ratio scale  */
  editor->aspect_ratio_data = gtk_adjustment_new (0.0, 1.0, 20.0, 0.1, 1.0, 0.0);
  scale = ligma_spin_scale_new (editor->aspect_ratio_data, _("Aspect ratio"), 1);
  gtk_box_pack_start (GTK_BOX (editor->options_box), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  g_signal_connect (editor->aspect_ratio_data,"value-changed",
                    G_CALLBACK (ligma_brush_editor_update_brush),
                    editor);

  /*  brush angle scale  */
  editor->angle_data = gtk_adjustment_new (0.0, 0.0, 180.0, 0.1, 1.0, 0.0);
  scale = ligma_spin_scale_new (editor->angle_data, _("Angle"), 1);
  gtk_box_pack_start (GTK_BOX (editor->options_box), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  g_signal_connect (editor->angle_data, "value-changed",
                    G_CALLBACK (ligma_brush_editor_update_brush),
                    editor);

  /*  brush spacing  */
  editor->spacing_data = gtk_adjustment_new (0.0, 1.0, 5000.0, 1.0, 10.0, 0.0);
  scale = ligma_spin_scale_new (editor->spacing_data, _("Spacing"), 1);
  ligma_spin_scale_set_scale_limits (LIGMA_SPIN_SCALE (scale), 1.0, 200.0);
  gtk_box_pack_start (GTK_BOX (editor->options_box), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  ligma_help_set_help_data (scale, _("Percentage of width of brush"), NULL);

  g_signal_connect (editor->spacing_data, "value-changed",
                    G_CALLBACK (ligma_brush_editor_update_brush),
                    editor);
}

static void
ligma_brush_editor_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_docked_set_show_button_bar (LIGMA_DOCKED (object), FALSE);
}

static void
ligma_brush_editor_set_data (LigmaDataEditor *editor,
                            LigmaData       *data)
{
  LigmaBrushEditor         *brush_editor = LIGMA_BRUSH_EDITOR (editor);
  LigmaBrushGeneratedShape  shape        = LIGMA_BRUSH_GENERATED_CIRCLE;
  gdouble                  radius       = 0.0;
  gint                     spikes       = 2;
  gdouble                  hardness     = 0.0;
  gdouble                  ratio        = 0.0;
  gdouble                  angle        = 0.0;
  gdouble                  spacing      = 0.0;

  if (editor->data)
    g_signal_handlers_disconnect_by_func (editor->data,
                                          ligma_brush_editor_notify_brush,
                                          editor);

  LIGMA_DATA_EDITOR_CLASS (parent_class)->set_data (editor, data);

  if (editor->data)
    g_signal_connect (editor->data, "notify",
                      G_CALLBACK (ligma_brush_editor_notify_brush),
                      editor);

  ligma_view_set_viewable (LIGMA_VIEW (editor->view), LIGMA_VIEWABLE (data));

  if (editor->data)
    {
      spacing = ligma_brush_get_spacing (LIGMA_BRUSH (editor->data));

      if (LIGMA_IS_BRUSH_GENERATED (editor->data))
        {
          LigmaBrushGenerated *brush = LIGMA_BRUSH_GENERATED (editor->data);

          shape    = ligma_brush_generated_get_shape        (brush);
          radius   = ligma_brush_generated_get_radius       (brush);
          spikes   = ligma_brush_generated_get_spikes       (brush);
          hardness = ligma_brush_generated_get_hardness     (brush);
          ratio    = ligma_brush_generated_get_aspect_ratio (brush);
          angle    = ligma_brush_generated_get_angle        (brush);
        }
    }

  gtk_widget_set_sensitive (brush_editor->options_box,
                            editor->data_editable);

  ligma_int_radio_group_set_active (GTK_RADIO_BUTTON (brush_editor->shape_group),
                                   shape);

  gtk_adjustment_set_value (brush_editor->radius_data,       radius);
  gtk_adjustment_set_value (brush_editor->spikes_data,       spikes);
  gtk_adjustment_set_value (brush_editor->hardness_data,     hardness);
  gtk_adjustment_set_value (brush_editor->aspect_ratio_data, ratio);
  gtk_adjustment_set_value (brush_editor->angle_data,        angle);
  gtk_adjustment_set_value (brush_editor->spacing_data,      spacing);
}

static void
ligma_brush_editor_set_context (LigmaDocked  *docked,
                               LigmaContext *context)
{
  LigmaDataEditor *data_editor = LIGMA_DATA_EDITOR (docked);

  parent_docked_iface->set_context (docked, context);

  ligma_view_renderer_set_context (LIGMA_VIEW (data_editor->view)->renderer,
                                  context);
}


/*  public functions  */

GtkWidget *
ligma_brush_editor_new (LigmaContext     *context,
                       LigmaMenuFactory *menu_factory)
{
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  return g_object_new (LIGMA_TYPE_BRUSH_EDITOR,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<BrushEditor>",
                       "ui-path",         "/brush-editor-popup",
                       "data-factory",    context->ligma->brush_factory,
                       "context",         context,
                       "data",            ligma_context_get_brush (context),
                       NULL);
}


/*  private functions  */

static void
ligma_brush_editor_update_brush (GtkAdjustment   *adjustment,
                                LigmaBrushEditor *editor)
{
  LigmaBrushGenerated *brush;
  gdouble             value;

  if (! LIGMA_IS_BRUSH_GENERATED (LIGMA_DATA_EDITOR (editor)->data))
    return;

  brush = LIGMA_BRUSH_GENERATED (LIGMA_DATA_EDITOR (editor)->data);

  g_signal_handlers_block_by_func (brush,
                                   ligma_brush_editor_notify_brush,
                                   editor);

  value = gtk_adjustment_get_value (adjustment);

  if (adjustment == editor->radius_data)
    {
      if (value != ligma_brush_generated_get_radius (brush))
        ligma_brush_generated_set_radius (brush, value);
    }
  else if (adjustment == editor->spikes_data)
    {
      if (ROUND (value) != ligma_brush_generated_get_spikes (brush))
        ligma_brush_generated_set_spikes (brush, ROUND (value));
    }
  else if (adjustment == editor->hardness_data)
    {
      if (value != ligma_brush_generated_get_hardness (brush))
        ligma_brush_generated_set_hardness (brush, value);
    }
  else if (adjustment == editor->aspect_ratio_data)
    {
      if (value != ligma_brush_generated_get_aspect_ratio (brush))
        ligma_brush_generated_set_aspect_ratio (brush, value);
    }
  else if (adjustment == editor->angle_data)
    {
      if (value != ligma_brush_generated_get_angle (brush))
        ligma_brush_generated_set_angle (brush, value);
    }
  else if (adjustment == editor->spacing_data)
    {
      if (value != ligma_brush_get_spacing (LIGMA_BRUSH (brush)))
        ligma_brush_set_spacing (LIGMA_BRUSH (brush), value);
    }

  g_signal_handlers_unblock_by_func (brush,
                                     ligma_brush_editor_notify_brush,
                                     editor);
}

static void
ligma_brush_editor_update_shape (GtkWidget       *widget,
                                LigmaBrushEditor *editor)
{
  LigmaBrushGenerated *brush;

  if (! LIGMA_IS_BRUSH_GENERATED (LIGMA_DATA_EDITOR (editor)->data))
    return;

  brush = LIGMA_BRUSH_GENERATED (LIGMA_DATA_EDITOR (editor)->data);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      LigmaBrushGeneratedShape shape;

      shape = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                  "ligma-item-data"));

      if (ligma_brush_generated_get_shape (brush) != shape)
        ligma_brush_generated_set_shape (brush, shape);
    }
}

static void
ligma_brush_editor_notify_brush (LigmaBrushGenerated *brush,
                                GParamSpec         *pspec,
                                LigmaBrushEditor    *editor)
{
  GtkAdjustment *adj   = NULL;
  gdouble        value = 0.0;

  if (! strcmp (pspec->name, "shape"))
    {
      g_signal_handlers_block_by_func (editor->shape_group,
                                       ligma_brush_editor_update_shape,
                                       editor);

      ligma_int_radio_group_set_active (GTK_RADIO_BUTTON (editor->shape_group),
                                       brush->shape);

      g_signal_handlers_unblock_by_func (editor->shape_group,
                                         ligma_brush_editor_update_shape,
                                         editor);
    }
  else if (! strcmp (pspec->name, "radius"))
    {
      adj   = editor->radius_data;
      value = ligma_brush_generated_get_radius (brush);
    }
  else if (! strcmp (pspec->name, "spikes"))
    {
      adj   = editor->spikes_data;
      value = ligma_brush_generated_get_spikes (brush);
    }
  else if (! strcmp (pspec->name, "hardness"))
    {
      adj   = editor->hardness_data;
      value = ligma_brush_generated_get_hardness (brush);
    }
  else if (! strcmp (pspec->name, "angle"))
    {
      adj   = editor->angle_data;
      value = ligma_brush_generated_get_angle (brush);
    }
  else if (! strcmp (pspec->name, "aspect-ratio"))
    {
      adj   = editor->aspect_ratio_data;
      value = ligma_brush_generated_get_aspect_ratio (brush);
    }
  else if (! strcmp (pspec->name, "spacing"))
    {
      adj   = editor->spacing_data;
      value = ligma_brush_get_spacing (LIGMA_BRUSH (brush));
    }

  if (adj)
    {
      g_signal_handlers_block_by_func (adj,
                                       ligma_brush_editor_update_brush,
                                       editor);

      gtk_adjustment_set_value (adj, value);

      g_signal_handlers_unblock_by_func (adj,
                                         ligma_brush_editor_update_brush,
                                         editor);
    }
}
