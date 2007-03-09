/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdasheditor.c
 * Copyright (C) 2003 Simon Budig  <simon@gimp.org>
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

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"

#include "widgets-types.h"

#include "core/gimpdashpattern.h"
#include "core/gimpstrokeoptions.h"

#include "gimpdasheditor.h"


#define MIN_WIDTH          64
#define MIN_HEIGHT         20

#define DEFAULT_N_SEGMENTS 24


enum
{
  PROP_0,
  PROP_STROKE_OPTIONS,
  PROP_N_SEGMENTS,
  PROP_LENGTH
};


static void gimp_dash_editor_finalize           (GObject        *object);
static void gimp_dash_editor_set_property       (GObject        *object,
                                                 guint           property_id,
                                                 const GValue   *value,
                                                 GParamSpec     *pspec);
static void gimp_dash_editor_get_property       (GObject        *object,
                                                 guint           property_id,
                                                 GValue         *value,
                                                 GParamSpec     *pspec);

static void gimp_dash_editor_size_request       (GtkWidget      *widget,
                                                 GtkRequisition *requisition);
static gboolean gimp_dash_editor_expose         (GtkWidget      *widget,
                                                 GdkEventExpose *event);
static gboolean gimp_dash_editor_button_press   (GtkWidget      *widget,
                                                 GdkEventButton *bevent);
static gboolean gimp_dash_editor_button_release (GtkWidget      *widget,
                                                 GdkEventButton *bevent);
static gboolean gimp_dash_editor_motion_notify  (GtkWidget      *widget,
                                                 GdkEventMotion *bevent);

/* helper function */
static void update_segments_from_options        (GimpDashEditor *editor);
static void update_options_from_segments        (GimpDashEditor *editor);
static void update_blocksize                    (GimpDashEditor *editor);
static gint dash_x_to_index                     (GimpDashEditor *editor,
                                                 gint            x);


G_DEFINE_TYPE (GimpDashEditor, gimp_dash_editor, GTK_TYPE_DRAWING_AREA)

#define parent_class gimp_dash_editor_parent_class


static void
gimp_dash_editor_class_init (GimpDashEditorClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize     = gimp_dash_editor_finalize;
  object_class->get_property = gimp_dash_editor_get_property;
  object_class->set_property = gimp_dash_editor_set_property;

  widget_class->size_request         = gimp_dash_editor_size_request;
  widget_class->expose_event         = gimp_dash_editor_expose;
  widget_class->button_press_event   = gimp_dash_editor_button_press;
  widget_class->button_release_event = gimp_dash_editor_button_release;
  widget_class->motion_notify_event  = gimp_dash_editor_motion_notify;

  g_object_class_install_property (object_class, PROP_STROKE_OPTIONS,
                                   g_param_spec_object ("stroke-options",
                                                        NULL, NULL,
                                                        GIMP_TYPE_STROKE_OPTIONS,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_N_SEGMENTS,
                                   g_param_spec_int ("n-segments",
                                                     NULL, NULL,
                                                     2, 120, DEFAULT_N_SEGMENTS,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_LENGTH,
                                   g_param_spec_double ("dash-length",
                                                        NULL, NULL,
                                                        0.0, 2000.0,
                                                        0.5 * DEFAULT_N_SEGMENTS,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gimp_dash_editor_init (GimpDashEditor *editor)
{
  editor->segments       = NULL;
  editor->block_width    = 6;
  editor->block_height   = 6;
  editor->edit_mode      = TRUE;
  editor->edit_button_x0 = 0;

  gtk_widget_add_events (GTK_WIDGET (editor),
                         GDK_BUTTON_PRESS_MASK   |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_BUTTON1_MOTION_MASK);
}

static void
gimp_dash_editor_finalize (GObject *object)
{
  GimpDashEditor *editor = GIMP_DASH_EDITOR (object);

  if (editor->stroke_options)
    {
      g_object_unref (editor->stroke_options);
      editor->stroke_options = NULL;
    }

  if (editor->segments)
    {
      g_free (editor->segments);
      editor->segments = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_dash_editor_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpDashEditor *editor = GIMP_DASH_EDITOR (object);

  switch (property_id)
    {
    case PROP_STROKE_OPTIONS:
      g_return_if_fail (editor->stroke_options == NULL);

      editor->stroke_options = GIMP_STROKE_OPTIONS (g_value_dup_object (value));
      g_signal_connect_object (editor->stroke_options, "notify::dash-info",
                               G_CALLBACK (update_segments_from_options),
                               editor, G_CONNECT_SWAPPED);
      break;

    case PROP_N_SEGMENTS:
      editor->n_segments = g_value_get_int (value);

      if (editor->segments)
        g_free (editor->segments);
      editor->segments = g_new0 (gboolean, editor->n_segments);
      break;

    case PROP_LENGTH:
      editor->dash_length = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }

  update_segments_from_options (editor);
}

static void
gimp_dash_editor_get_property (GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  GimpDashEditor *editor = GIMP_DASH_EDITOR (object);

  switch (property_id)
    {
    case PROP_STROKE_OPTIONS:
      g_value_set_object (value, editor->stroke_options);
      break;
    case PROP_N_SEGMENTS:
      g_value_set_int (value, editor->n_segments);
      break;
    case PROP_LENGTH:
      g_value_set_double (value, editor->dash_length);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_dash_editor_size_request (GtkWidget      *widget,
                               GtkRequisition *requisition)
{
  GimpDashEditor *editor = GIMP_DASH_EDITOR (widget);

  requisition->width  = MAX (editor->block_width * editor->n_segments + 20,
                             MIN_WIDTH);
  requisition->height = MAX (editor->block_height + 10, MIN_HEIGHT);
}

static gboolean
gimp_dash_editor_expose (GtkWidget      *widget,
                         GdkEventExpose *event)
{
  GimpDashEditor *editor = GIMP_DASH_EDITOR (widget);
  gint            x, index, w, h;

  update_blocksize (editor);

  /*  Draw the background  */
  gdk_draw_rectangle (widget->window,
                      widget->style->base_gc[GTK_STATE_NORMAL], TRUE,
                      0, 0,
                      widget->allocation.width,
                      widget->allocation.height);

  w = editor->block_width;
  h = editor->block_height;

  editor->x0 = (widget->allocation.width - w * editor->n_segments) / 2;
  editor->y0 = (widget->allocation.height - h) / 2;

  x = editor->x0 % w;

  if (x > 0)
    x -= w;

  for (; x < widget->allocation.width + w; x += w)
    {
      index = dash_x_to_index (editor, x);

      if (x < editor->x0 || x >= editor->x0 + editor->n_segments * w)
        {
          if (editor->segments[index])
            {
              gdk_draw_rectangle (widget->window,
                                  widget->style->text_aa_gc[GTK_STATE_NORMAL],
                                  TRUE,
                                  x, editor->y0,
                                  w, h);
            }
        }
      else
        {
          if (editor->segments[index])
            {
              gdk_draw_rectangle (widget->window,
                                  widget->style->text_gc[GTK_STATE_NORMAL],
                                  TRUE,
                                  x, editor->y0,
                                  w, h);
            }

          if (editor->n_segments % 4 == 0 &&
              (index + 1) % (editor->n_segments / 4) == 0)
            {
              gdk_draw_line (widget->window,
                             widget->style->text_aa_gc[GTK_STATE_NORMAL],
                             x + w - 1, editor->y0 - 2,
                             x + w - 1, editor->y0 + h + 1);
            }
          else if (index % 2 == 1)
            {
              gdk_draw_line (widget->window,
                             widget->style->text_aa_gc[GTK_STATE_NORMAL],
                             x + w - 1, editor->y0 + 1,
                             x + w - 1, editor->y0 + h - 2);
            }
          else
            {
              gdk_draw_line (widget->window,
                             widget->style->text_aa_gc[GTK_STATE_NORMAL],
                             x + w - 1, editor->y0 + h / 2 - 1,
                             x + w - 1, editor->y0 + h / 2);
            }
        }
    }

  gdk_draw_line (widget->window,
                 widget->style->text_aa_gc[GTK_STATE_NORMAL],
                 editor->x0 - 1, editor->y0 - 1,
                 editor->x0 - 1, editor->y0 + h);

  return FALSE;
}

static gboolean
gimp_dash_editor_button_press (GtkWidget      *widget,
                               GdkEventButton *bevent)
{
  GimpDashEditor *editor = GIMP_DASH_EDITOR (widget);
  gint            index;

  if (bevent->button == 1 && bevent->type == GDK_BUTTON_PRESS)
    {
      gdk_pointer_grab (widget->window, FALSE,
                        GDK_BUTTON_RELEASE_MASK | GDK_BUTTON1_MOTION_MASK,
                        NULL, NULL, bevent->time);
      index = dash_x_to_index (editor, bevent->x);

      editor->edit_mode = ! editor->segments [index];
      editor->edit_button_x0 = bevent->x;

      editor->segments [index] = editor->edit_mode;

      gtk_widget_queue_draw (widget);
    }

  return TRUE;
}

static gboolean
gimp_dash_editor_button_release (GtkWidget      *widget,
                                 GdkEventButton *bevent)
{
  GimpDashEditor *editor = GIMP_DASH_EDITOR (widget);

  if (bevent->button == 1)
    {
      gdk_display_pointer_ungrab (gtk_widget_get_display (GTK_WIDGET (editor)),
                                  bevent->time);

      update_options_from_segments (editor);
    }

  return TRUE;
}

static gboolean
gimp_dash_editor_motion_notify (GtkWidget      *widget,
                                GdkEventMotion *mevent)
{
  GimpDashEditor *editor = GIMP_DASH_EDITOR (widget);
  gint            x, index;

  index = dash_x_to_index (editor, mevent->x);
  editor->segments [index] = editor->edit_mode;

  if (mevent->x > editor->edit_button_x0)
    {
      for (x = editor->edit_button_x0; x < mevent->x; x += editor->block_width)
        {
          index = dash_x_to_index (editor, x);
          editor->segments[index] = editor->edit_mode;
        }
    }

  if (mevent->x < editor->edit_button_x0)
    {
      for (x = editor->edit_button_x0; x > mevent->x; x -= editor->block_width)
        {
          index = dash_x_to_index (editor, x);
          editor->segments[index] = editor->edit_mode;
        }
    }

  gtk_widget_queue_draw (widget);

  return TRUE;
}

GtkWidget *
gimp_dash_editor_new (GimpStrokeOptions *stroke_options)
{
  g_return_val_if_fail (GIMP_IS_STROKE_OPTIONS (stroke_options), NULL);

  return g_object_new (GIMP_TYPE_DASH_EDITOR,
                       "stroke-options", stroke_options,
                       NULL);
}

void
gimp_dash_editor_shift_right (GimpDashEditor *editor)
{
  gboolean swap;
  gint     i;

  g_return_if_fail (GIMP_IS_DASH_EDITOR (editor));
  g_return_if_fail (editor->n_segments > 0);

  swap = editor->segments[editor->n_segments - 1];
  for (i = editor->n_segments - 1; i > 0; i--)
    editor->segments[i] = editor->segments[i-1];
  editor->segments[0] = swap;

  update_options_from_segments (editor);
}

void
gimp_dash_editor_shift_left (GimpDashEditor *editor)
{
  gboolean swap;
  gint     i;

  g_return_if_fail (GIMP_IS_DASH_EDITOR (editor));
  g_return_if_fail (editor->n_segments > 0);

  swap = editor->segments[0];
  for (i = 1; i < editor->n_segments; i++)
    editor->segments[i-1] = editor->segments[i];
  editor->segments[editor->n_segments - 1] = swap;

  update_options_from_segments (editor);
}

static void
update_segments_from_options (GimpDashEditor *editor)
{
  if (editor->stroke_options == NULL || editor->segments == NULL)
    return;

  g_return_if_fail (GIMP_IS_STROKE_OPTIONS (editor->stroke_options));

  gtk_widget_queue_draw (GTK_WIDGET (editor));

  gimp_dash_pattern_fill_segments (editor->stroke_options->dash_info,
                                   editor->segments, editor->n_segments);
}

static void
update_options_from_segments (GimpDashEditor *editor)
{
  GArray *pattern = gimp_dash_pattern_new_from_segments (editor->segments,
                                                         editor->n_segments,
                                                         editor->dash_length);

  gimp_stroke_options_set_dash_pattern (editor->stroke_options,
                                        GIMP_DASH_CUSTOM, pattern);
}

static void
update_blocksize (GimpDashEditor *editor)
{
  GtkWidget *widget = GTK_WIDGET (editor);

  editor->block_height = 6;

  editor->block_width = MAX (ROUND (editor->dash_length /
                                    editor->n_segments * editor->block_height),
                             4);
  editor->block_height = MIN (ROUND (((float) editor->block_width) *
                                     editor->n_segments / editor->dash_length),
                              widget->allocation.height - 4);
}

static gint
dash_x_to_index (GimpDashEditor *editor,
                 gint            x)
{
  gint index = x - editor->x0;

  while (index < 0)
    index += editor->n_segments * editor->block_width;

  index = (index / editor->block_width) % editor->n_segments;

  return index;
}
