/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfgbgeditor.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpmarshal.h"

#include "gimpdnd.h"
#include "gimpfgbgeditor.h"
#include "gimpwidgets-utils.h"


enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_ACTIVE_COLOR
};

enum
{
  COLOR_CLICKED,
  LAST_SIGNAL
};

typedef enum
{
  INVALID_AREA,
  FOREGROUND_AREA,
  BACKGROUND_AREA,
  SWAP_AREA,
  DEFAULT_AREA
} FgBgTarget;


static void     gimp_fg_bg_editor_dispose           (GObject          *object);
static void     gimp_fg_bg_editor_set_property      (GObject          *object,
                                                     guint             property_id,
                                                     const GValue     *value,
                                                     GParamSpec       *pspec);
static void     gimp_fg_bg_editor_get_property      (GObject          *object,
                                                     guint             property_id,
                                                     GValue           *value,
                                                     GParamSpec       *pspec);

static void     gimp_fg_bg_editor_style_updated     (GtkWidget        *widget);
static gboolean gimp_fg_bg_editor_draw              (GtkWidget        *widget,
                                                     cairo_t          *cr);
static gboolean gimp_fg_bg_editor_button_press      (GtkWidget        *widget,
                                                     GdkEventButton   *bevent);
static gboolean gimp_fg_bg_editor_button_release    (GtkWidget        *widget,
                                                     GdkEventButton   *bevent);
static gboolean gimp_fg_bg_editor_drag_motion       (GtkWidget        *widget,
                                                     GdkDragContext   *context,
                                                     gint              x,
                                                     gint              y,
                                                     guint             time);

static void     gimp_fg_bg_editor_drag_color        (GtkWidget        *widget,
                                                     GimpRGB          *color,
                                                     gpointer          data);
static void     gimp_fg_bg_editor_drop_color        (GtkWidget        *widget,
                                                     gint              x,
                                                     gint              y,
                                                     const GimpRGB    *color,
                                                     gpointer          data);

static void     gimp_fg_bg_editor_create_transform  (GimpFgBgEditor   *editor);
static void     gimp_fg_bg_editor_destroy_transform (GimpFgBgEditor   *editor);


G_DEFINE_TYPE (GimpFgBgEditor, gimp_fg_bg_editor, GTK_TYPE_EVENT_BOX)

#define parent_class gimp_fg_bg_editor_parent_class

static guint  editor_signals[LAST_SIGNAL] = { 0 };


static void
gimp_fg_bg_editor_class_init (GimpFgBgEditorClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  editor_signals[COLOR_CLICKED] =
    g_signal_new ("color-clicked",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpFgBgEditorClass, color_clicked),
                  NULL, NULL,
                  gimp_marshal_VOID__ENUM,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_ACTIVE_COLOR);

  object_class->dispose              = gimp_fg_bg_editor_dispose;
  object_class->set_property         = gimp_fg_bg_editor_set_property;
  object_class->get_property         = gimp_fg_bg_editor_get_property;

  widget_class->style_updated        = gimp_fg_bg_editor_style_updated;
  widget_class->draw                 = gimp_fg_bg_editor_draw;
  widget_class->button_press_event   = gimp_fg_bg_editor_button_press;
  widget_class->button_release_event = gimp_fg_bg_editor_button_release;
  widget_class->drag_motion          = gimp_fg_bg_editor_drag_motion;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        GIMP_TYPE_CONTEXT,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_ACTIVE_COLOR,
                                   g_param_spec_enum ("active-color",
                                                      NULL, NULL,
                                                      GIMP_TYPE_ACTIVE_COLOR,
                                                      GIMP_ACTIVE_COLOR_FOREGROUND,
                                                      GIMP_PARAM_READWRITE));
}

static void
gimp_fg_bg_editor_init (GimpFgBgEditor *editor)
{
  editor->active_color = GIMP_ACTIVE_COLOR_FOREGROUND;

  gtk_event_box_set_visible_window (GTK_EVENT_BOX (editor), FALSE);

  gtk_widget_add_events (GTK_WIDGET (editor),
                         GDK_BUTTON_PRESS_MASK |
                         GDK_BUTTON_RELEASE_MASK);

  gimp_dnd_color_source_add (GTK_WIDGET (editor),
                             gimp_fg_bg_editor_drag_color, NULL);
  gimp_dnd_color_dest_add (GTK_WIDGET (editor),
                           gimp_fg_bg_editor_drop_color, NULL);

  gimp_widget_track_monitor (GTK_WIDGET (editor),
                             G_CALLBACK (gimp_fg_bg_editor_destroy_transform),
                             NULL);
}

static void
gimp_fg_bg_editor_dispose (GObject *object)
{
  GimpFgBgEditor *editor = GIMP_FG_BG_EDITOR (object);

  if (editor->context)
    gimp_fg_bg_editor_set_context (editor, NULL);

  g_clear_object (&editor->default_icon);
  g_clear_object (&editor->swap_icon);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_fg_bg_editor_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpFgBgEditor *editor = GIMP_FG_BG_EDITOR (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      gimp_fg_bg_editor_set_context (editor, g_value_get_object (value));
      break;
    case PROP_ACTIVE_COLOR:
      gimp_fg_bg_editor_set_active (editor, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_fg_bg_editor_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpFgBgEditor *editor = GIMP_FG_BG_EDITOR (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, editor->context);
      break;
    case PROP_ACTIVE_COLOR:
      g_value_set_enum (value, editor->active_color);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_fg_bg_editor_style_updated (GtkWidget *widget)
{
  GimpFgBgEditor *editor = GIMP_FG_BG_EDITOR (widget);

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  g_clear_object (&editor->default_icon);
  g_clear_object (&editor->swap_icon);
}

static gboolean
gimp_fg_bg_editor_draw (GtkWidget *widget,
                        cairo_t   *cr)
{
  GimpFgBgEditor *editor = GIMP_FG_BG_EDITOR (widget);
  GtkStyle       *style  = gtk_widget_get_style (widget);
  GtkAllocation   allocation;
  gint            width, height;
  gint            default_w, default_h;
  gint            swap_w, swap_h;
  gint            rect_w, rect_h;
  GimpRGB         color;
  GimpRGB         transformed_color;

  if (! gtk_widget_is_drawable (widget))
    return FALSE;

  gtk_widget_get_allocation (widget, &allocation);

  width  = allocation.width;
  height = allocation.height;

  cairo_translate (cr, allocation.x, allocation.y);

  /*  draw the default colors pixbuf  */
  if (! editor->default_icon)
    editor->default_icon = gimp_widget_load_icon (widget,
                                                  GIMP_ICON_COLORS_DEFAULT, 12);

  default_w = gdk_pixbuf_get_width  (editor->default_icon);
  default_h = gdk_pixbuf_get_height (editor->default_icon);

  if (default_w < width / 2 && default_h < height / 2)
    {
      gdk_cairo_set_source_pixbuf (cr, editor->default_icon,
                                   0, height - default_h);
      cairo_paint (cr);
    }
  else
    {
      default_w = default_h = 0;
    }

  /*  draw the swap colors pixbuf  */
  if (! editor->swap_icon)
    editor->swap_icon = gimp_widget_load_icon (widget,
                                               GIMP_ICON_COLORS_SWAP, 12);

  swap_w = gdk_pixbuf_get_width  (editor->swap_icon);
  swap_h = gdk_pixbuf_get_height (editor->swap_icon);

  if (swap_w < width / 2 && swap_h < height / 2)
    {
      gdk_cairo_set_source_pixbuf (cr, editor->swap_icon,
                                   width - swap_w, 0);
      cairo_paint (cr);
    }
  else
    {
      swap_w = swap_h = 0;
    }

  rect_h = height - MAX (default_h, swap_h) - 2;
  rect_w = width  - MAX (default_w, swap_w) - 4;

  if (rect_h > (height * 3 / 4))
    rect_w = MAX (rect_w - (rect_h - ((height * 3 / 4))),
                  width * 2 / 3);

  editor->rect_width  = rect_w;
  editor->rect_height = rect_h;


  if (! editor->transform)
    gimp_fg_bg_editor_create_transform (editor);

  /*  draw the background area  */

  if (editor->context)
    {
      gimp_context_get_background (editor->context, &color);

      if (editor->transform)
        gimp_color_transform_process_pixels (editor->transform,
                                             babl_format ("R'G'B'A double"),
                                             &color,
                                             babl_format ("R'G'B'A double"),
                                             &transformed_color,
                                             1);
      else
        transformed_color = color;

      gimp_cairo_set_source_rgb (cr, &transformed_color);

      cairo_rectangle (cr,
                       width - rect_w,
                       height - rect_h,
                       rect_w,
                       rect_h);
      cairo_fill (cr);

      if (editor->color_config &&
          (color.r < 0.0 || color.r > 1.0 ||
           color.g < 0.0 || color.g > 1.0 ||
           color.b < 0.0 || color.b > 1.0))
        {
          gint side = MIN (rect_w, rect_h) * 2 / 3;

          cairo_move_to (cr, width, height);
          cairo_line_to (cr, width - side, height);
          cairo_line_to (cr, width, height - side);
          cairo_line_to (cr, width, height);

          gimp_cairo_set_source_rgb (cr,
                                     &editor->color_config->out_of_gamut_color);
          cairo_fill (cr);
        }
    }

  gtk_paint_shadow (style, cr, GTK_STATE_NORMAL,
                    editor->active_color == GIMP_ACTIVE_COLOR_FOREGROUND ?
                    GTK_SHADOW_OUT : GTK_SHADOW_IN,
                    widget, NULL,
                    allocation.x + (width - rect_w),
                    allocation.y + (height - rect_h),
                    rect_w, rect_h);


  /*  draw the foreground area  */

  if (editor->context)
    {
      gimp_context_get_foreground (editor->context, &color);

      if (editor->transform)
        gimp_color_transform_process_pixels (editor->transform,
                                             babl_format ("R'G'B'A double"),
                                             &color,
                                             babl_format ("R'G'B'A double"),
                                             &transformed_color,
                                             1);
      else
        transformed_color = color;

      gimp_cairo_set_source_rgb (cr, &transformed_color);

      cairo_rectangle (cr,
                       0, 0,
                       rect_w, rect_h);
      cairo_fill (cr);

      if (editor->color_config &&
          (color.r < 0.0 || color.r > 1.0 ||
           color.g < 0.0 || color.g > 1.0 ||
           color.b < 0.0 || color.b > 1.0))
        {
          gint side = MIN (rect_w, rect_h) * 2 / 3;

          cairo_move_to (cr, 0, 0);
          cairo_line_to (cr, 0, side);
          cairo_line_to (cr, side, 0);
          cairo_line_to (cr, 0, 0);

          gimp_cairo_set_source_rgb (cr,
                                     &editor->color_config->out_of_gamut_color);
          cairo_fill (cr);
        }
    }

  gtk_paint_shadow (style, cr, GTK_STATE_NORMAL,
                    editor->active_color == GIMP_ACTIVE_COLOR_BACKGROUND ?
                    GTK_SHADOW_OUT : GTK_SHADOW_IN,
                    widget, NULL,
                    allocation.x,
                    allocation.y,
                    rect_w, rect_h);

  return TRUE;
}

static FgBgTarget
gimp_fg_bg_editor_target (GimpFgBgEditor *editor,
                          gint            x,
                          gint            y)
{
  GtkAllocation allocation;
  gint          width;
  gint          height;
  gint          rect_w = editor->rect_width;
  gint          rect_h = editor->rect_height;

  gtk_widget_get_allocation (GTK_WIDGET (editor), &allocation);

  width  = allocation.width;
  height = allocation.height;

  if (x > 0 && x < rect_w && y > 0 && y < rect_h)
    return FOREGROUND_AREA;
  else if (x > (width - rect_w)  && x < width  &&
           y > (height - rect_h) && y < height)
    return BACKGROUND_AREA;
  else if (x > 0      && x < (width - rect_w) &&
           y > rect_h && y < height)
    return DEFAULT_AREA;
  else if (x > rect_w && x < width &&
           y > 0      && y < (height - rect_h))
    return SWAP_AREA;

  return INVALID_AREA;
}

static gboolean
gimp_fg_bg_editor_button_press (GtkWidget      *widget,
                                GdkEventButton *bevent)
{
  GimpFgBgEditor *editor = GIMP_FG_BG_EDITOR (widget);

  if (bevent->button == 1 && bevent->type == GDK_BUTTON_PRESS)
    {
      FgBgTarget target = gimp_fg_bg_editor_target (editor,
                                                    bevent->x, bevent->y);

      editor->click_target = INVALID_AREA;

      switch (target)
        {
        case FOREGROUND_AREA:
          if (editor->active_color != GIMP_ACTIVE_COLOR_FOREGROUND)
            gimp_fg_bg_editor_set_active (editor,
                                          GIMP_ACTIVE_COLOR_FOREGROUND);
          editor->click_target = FOREGROUND_AREA;
          break;

        case BACKGROUND_AREA:
          if (editor->active_color != GIMP_ACTIVE_COLOR_BACKGROUND)
            gimp_fg_bg_editor_set_active (editor,
                                          GIMP_ACTIVE_COLOR_BACKGROUND);
          editor->click_target = BACKGROUND_AREA;
          break;

        case SWAP_AREA:
          if (editor->context)
            gimp_context_swap_colors (editor->context);
          break;

        case DEFAULT_AREA:
          if (editor->context)
            gimp_context_set_default_colors (editor->context);
          break;

        default:
          break;
        }
    }

  return FALSE;
}

static gboolean
gimp_fg_bg_editor_button_release (GtkWidget      *widget,
                                  GdkEventButton *bevent)
{
  GimpFgBgEditor *editor = GIMP_FG_BG_EDITOR (widget);

  if (bevent->button == 1)
    {
      FgBgTarget target = gimp_fg_bg_editor_target (editor,
                                                    bevent->x, bevent->y);

      if (target == editor->click_target)
        {
          switch (target)
            {
            case FOREGROUND_AREA:
              g_signal_emit (editor, editor_signals[COLOR_CLICKED], 0,
                             GIMP_ACTIVE_COLOR_FOREGROUND);
              break;

            case BACKGROUND_AREA:
              g_signal_emit (editor, editor_signals[COLOR_CLICKED], 0,
                             GIMP_ACTIVE_COLOR_BACKGROUND);
              break;

            default:
              break;
            }
        }

      editor->click_target = INVALID_AREA;
    }

  return FALSE;
}

static gboolean
gimp_fg_bg_editor_drag_motion (GtkWidget      *widget,
                               GdkDragContext *context,
                               gint            x,
                               gint            y,
                               guint           time)
{
  GimpFgBgEditor *editor = GIMP_FG_BG_EDITOR (widget);
  FgBgTarget      target = gimp_fg_bg_editor_target (editor, x, y);

  if (target == FOREGROUND_AREA || target == BACKGROUND_AREA)
    {
      gdk_drag_status (context, GDK_ACTION_COPY, time);

      return TRUE;
    }

  gdk_drag_status (context, 0, time);

  return FALSE;
}


/*  public functions  */

GtkWidget *
gimp_fg_bg_editor_new (GimpContext *context)
{
  g_return_val_if_fail (context == NULL || GIMP_IS_CONTEXT (context), NULL);

  return g_object_new (GIMP_TYPE_FG_BG_EDITOR,
                       "context", context,
                       NULL);
}

void
gimp_fg_bg_editor_set_context (GimpFgBgEditor *editor,
                               GimpContext    *context)
{
  g_return_if_fail (GIMP_IS_FG_BG_EDITOR (editor));
  g_return_if_fail (context == NULL || GIMP_IS_CONTEXT (context));

  if (context != editor->context)
    {
      if (editor->context)
        {
          g_signal_handlers_disconnect_by_func (editor->context,
                                                gtk_widget_queue_draw,
                                                editor);
          g_object_unref (editor->context);

          g_signal_handlers_disconnect_by_func (editor->color_config,
                                                gimp_fg_bg_editor_destroy_transform,
                                                editor);
          g_clear_object (&editor->color_config);
        }

      editor->context = context;

      if (context)
        {
          g_object_ref (context);

          g_signal_connect_swapped (context, "foreground-changed",
                                    G_CALLBACK (gtk_widget_queue_draw),
                                    editor);
          g_signal_connect_swapped (context, "background-changed",
                                    G_CALLBACK (gtk_widget_queue_draw),
                                    editor);

          editor->color_config = g_object_ref (context->gimp->config->color_management);

          g_signal_connect_swapped (editor->color_config, "notify",
                                    G_CALLBACK (gimp_fg_bg_editor_destroy_transform),
                                    editor);
        }

      gimp_fg_bg_editor_destroy_transform (editor);

      g_object_notify (G_OBJECT (editor), "context");
    }
}

void
gimp_fg_bg_editor_set_active (GimpFgBgEditor  *editor,
                              GimpActiveColor  active)
{
  g_return_if_fail (GIMP_IS_FG_BG_EDITOR (editor));

  editor->active_color = active;
  gtk_widget_queue_draw (GTK_WIDGET (editor));
  g_object_notify (G_OBJECT (editor), "active-color");
}


/*  private functions  */

static void
gimp_fg_bg_editor_drag_color (GtkWidget *widget,
                              GimpRGB   *color,
                              gpointer   data)
{
  GimpFgBgEditor *editor = GIMP_FG_BG_EDITOR (widget);

  if (editor->context)
    {
      switch (editor->active_color)
        {
        case GIMP_ACTIVE_COLOR_FOREGROUND:
          gimp_context_get_foreground (editor->context, color);
          break;

        case GIMP_ACTIVE_COLOR_BACKGROUND:
          gimp_context_get_background (editor->context, color);
          break;
        }
    }
}

static void
gimp_fg_bg_editor_drop_color (GtkWidget     *widget,
                              gint           x,
                              gint           y,
                              const GimpRGB *color,
                              gpointer       data)
{
  GimpFgBgEditor *editor = GIMP_FG_BG_EDITOR (widget);

  if (editor->context)
    {
      switch (gimp_fg_bg_editor_target (editor, x, y))
        {
        case FOREGROUND_AREA:
          gimp_context_set_foreground (editor->context, color);
          break;

        case BACKGROUND_AREA:
          gimp_context_set_background (editor->context, color);
          break;

        default:
          break;
        }
    }
}

static void
gimp_fg_bg_editor_create_transform (GimpFgBgEditor *editor)
{
  if (editor->color_config)
    {
      static GimpColorProfile *profile = NULL;

      if (G_UNLIKELY (! profile))
        profile = gimp_color_profile_new_rgb_srgb ();

      editor->transform =
        gimp_widget_get_color_transform (GTK_WIDGET (editor),
                                         editor->color_config,
                                         profile,
                                         babl_format ("R'G'B'A double"),
                                         babl_format ("R'G'B'A double"));
    }
}

static void
gimp_fg_bg_editor_destroy_transform (GimpFgBgEditor *editor)
{
  g_clear_object (&editor->transform);

  gtk_widget_queue_draw (GTK_WIDGET (editor));
}
