/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfgbgeditor.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontext.h"
#include "core/gimpmarshal.h"

#include "gimpdnd.h"
#include "gimpfgbgeditor.h"


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


static void     gimp_fg_bg_editor_set_property    (GObject        *object,
                                                   guint           property_id,
                                                   const GValue   *value,
                                                   GParamSpec     *pspec);
static void     gimp_fg_bg_editor_get_property    (GObject        *object,
                                                   guint           property_id,
                                                   GValue         *value,
                                                   GParamSpec     *pspec);

static void     gimp_fg_bg_editor_destroy         (GtkObject      *object);
static gboolean gimp_fg_bg_editor_expose          (GtkWidget      *widget,
                                                   GdkEventExpose *eevent);
static gboolean gimp_fg_bg_editor_button_press    (GtkWidget      *widget,
                                                   GdkEventButton *bevent);
static gboolean gimp_fg_bg_editor_button_release  (GtkWidget      *widget,
                                                   GdkEventButton *bevent);
static gboolean gimp_fg_bg_editor_drag_motion     (GtkWidget      *widget,
                                                   GdkDragContext *context,
                                                   gint            x,
                                                   gint            y,
                                                   guint           time);

static void     gimp_fg_bg_editor_drag_color      (GtkWidget      *widget,
                                                   GimpRGB        *color,
                                                   gpointer        data);
static void     gimp_fg_bg_editor_drop_color      (GtkWidget      *widget,
                                                   gint            x,
                                                   gint            y,
                                                   const GimpRGB  *color,
                                                   gpointer        data);


G_DEFINE_TYPE (GimpFgBgEditor, gimp_fg_bg_editor, GTK_TYPE_DRAWING_AREA)

#define parent_class gimp_fg_bg_editor_parent_class

static guint  editor_signals[LAST_SIGNAL] = { 0 };


static void
gimp_fg_bg_editor_class_init (GimpFgBgEditorClass *klass)
{
  GObjectClass   *object_class     = G_OBJECT_CLASS (klass);
  GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class     = GTK_WIDGET_CLASS (klass);

  editor_signals[COLOR_CLICKED] =
    g_signal_new ("color-clicked",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpFgBgEditorClass, color_clicked),
                  NULL, NULL,
                  gimp_marshal_VOID__ENUM,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_ACTIVE_COLOR);

  object_class->set_property         = gimp_fg_bg_editor_set_property;
  object_class->get_property         = gimp_fg_bg_editor_get_property;

  gtk_object_class->destroy          = gimp_fg_bg_editor_destroy;

  widget_class->expose_event         = gimp_fg_bg_editor_expose;
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
  editor->context      = NULL;
  editor->active_color = GIMP_ACTIVE_COLOR_FOREGROUND;

  gtk_widget_add_events (GTK_WIDGET (editor),
                         GDK_BUTTON_PRESS_MASK |
                         GDK_BUTTON_RELEASE_MASK);

  gimp_dnd_color_source_add (GTK_WIDGET (editor),
                             gimp_fg_bg_editor_drag_color, NULL);
  gimp_dnd_color_dest_add (GTK_WIDGET (editor),
                           gimp_fg_bg_editor_drop_color, NULL);
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
gimp_fg_bg_editor_destroy (GtkObject *object)
{
  GimpFgBgEditor *editor = GIMP_FG_BG_EDITOR (object);

  if (editor->context)
    gimp_fg_bg_editor_set_context (editor, NULL);

  if (editor->render_buf)
    {
      g_free (editor->render_buf);
      editor->render_buf = NULL;
      editor->render_buf_size = 0;
    }

  if (editor->default_icon)
    {
      g_object_unref (editor->default_icon);
      editor->default_icon = NULL;
    }

  if (editor->swap_icon)
    {
      g_object_unref (editor->swap_icon);
      editor->swap_icon = NULL;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_fg_bg_editor_draw_rect (GimpFgBgEditor *editor,
                             GdkDrawable    *drawable,
                             GdkGC          *gc,
                             gint            x,
                             gint            y,
                             gint            width,
                             gint            height,
                             const GimpRGB  *color)
{
  gint    rowstride;
  guchar  r, g, b;
  gint    xx, yy;
  guchar *bp;

  g_return_if_fail (width > 0 && height > 0);

  gimp_rgb_get_uchar (color, &r, &g, &b);

  rowstride = 3 * ((width + 3) & -4);

  if (! editor->render_buf || editor->render_buf_size < height * rowstride)
    {
      editor->render_buf_size = rowstride * height;

      g_free (editor->render_buf);
      editor->render_buf = g_malloc (editor->render_buf_size);
    }

  bp = editor->render_buf;
  for (xx = 0; xx < width; xx++)
    {
      *bp++ = r;
      *bp++ = g;
      *bp++ = b;
    }

  bp = editor->render_buf;

  for (yy = 1; yy < height; yy++)
    {
      bp += rowstride;
      memcpy (bp, editor->render_buf, rowstride);
    }

  gdk_draw_rgb_image (drawable, gc, x, y, width, height,
                      GDK_RGB_DITHER_MAX,
                      editor->render_buf,
                      rowstride);
}

static gboolean
gimp_fg_bg_editor_expose (GtkWidget      *widget,
                          GdkEventExpose *eevent)
{
  GimpFgBgEditor *editor = GIMP_FG_BG_EDITOR (widget);
  gint            width, height;
  gint            default_w, default_h;
  gint            swap_w, swap_h;
  gint            rect_w, rect_h;
  GimpRGB         color;

  if (! GTK_WIDGET_DRAWABLE (widget))
    return FALSE;

  width  = widget->allocation.width;
  height = widget->allocation.height;

  /*  draw the default colors pixbuf  */
  if (! editor->default_icon)
    editor->default_icon = gtk_widget_render_icon (widget,
                                                   GIMP_STOCK_DEFAULT_COLORS,
                                                   GTK_ICON_SIZE_MENU, NULL);

  default_w = gdk_pixbuf_get_width  (editor->default_icon);
  default_h = gdk_pixbuf_get_height (editor->default_icon);

  if (default_w < width / 2 && default_h < height / 2)
    gdk_draw_pixbuf (widget->window, NULL, editor->default_icon,
                     0, 0, 0, height - default_h, default_w, default_h,
                     GDK_RGB_DITHER_NORMAL, 0, 0);
  else
    default_w = default_h = 0;

  /*  draw the swap colors pixbuf  */
  if (! editor->swap_icon)
    editor->swap_icon = gtk_widget_render_icon (widget,
                                                GIMP_STOCK_SWAP_COLORS,
                                                GTK_ICON_SIZE_MENU, NULL);

  swap_w = gdk_pixbuf_get_width  (editor->swap_icon);
  swap_h = gdk_pixbuf_get_height (editor->swap_icon);

  if (swap_w < width / 2 && swap_h < height / 2)
    gdk_draw_pixbuf (widget->window, NULL, editor->swap_icon,
                     0, 0, width - swap_w, 0, swap_w, swap_h,
                     GDK_RGB_DITHER_NORMAL, 0, 0);
  else
    swap_w = swap_h = 0;

  rect_h = height - MAX (default_h, swap_h) - 2;
  rect_w = width  - MAX (default_w, swap_w) - 4;

  if (rect_h > (height * 3 / 4))
    rect_w = MAX (rect_w - (rect_h - ((height * 3 / 4))),
                  width * 2 / 3);

  editor->rect_width  = rect_w;
  editor->rect_height = rect_h;


  /*  draw the background area  */

  if (editor->context)
    {
      gimp_context_get_background (editor->context, &color);
      gimp_fg_bg_editor_draw_rect (editor,
                                   widget->window,
                                   widget->style->fg_gc[0],
                                   (width - rect_w),
                                   (height - rect_h),
                                   rect_w, rect_h,
                                   &color);
    }

  gtk_paint_shadow (widget->style, widget->window, GTK_STATE_NORMAL,
                    editor->active_color == GIMP_ACTIVE_COLOR_FOREGROUND ?
                    GTK_SHADOW_OUT : GTK_SHADOW_IN,
                    NULL, widget, NULL,
                    (width - rect_w),
                    (height - rect_h),
                    rect_w, rect_h);


  /*  draw the foreground area  */

  if (editor->context)
    {
      gimp_context_get_foreground (editor->context, &color);
      gimp_fg_bg_editor_draw_rect (editor,
                                   widget->window,
                                   widget->style->fg_gc[0],
                                   0, 0,
                                   rect_w, rect_h,
                                   &color);
    }

  gtk_paint_shadow (widget->style, widget->window, GTK_STATE_NORMAL,
                    editor->active_color == GIMP_ACTIVE_COLOR_BACKGROUND ?
                    GTK_SHADOW_OUT : GTK_SHADOW_IN,
                    NULL, widget, NULL,
                    0, 0,
                    rect_w, rect_h);

  return TRUE;
}

static FgBgTarget
gimp_fg_bg_editor_target (GimpFgBgEditor *editor,
                          gint            x,
                          gint            y)
{
  gint width  = GTK_WIDGET (editor)->allocation.width;
  gint height = GTK_WIDGET (editor)->allocation.height;
  gint rect_w = editor->rect_width;
  gint rect_h = editor->rect_height;

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

  if (context == editor->context)
    return;

  if (editor->context)
    {
      g_signal_handlers_disconnect_by_func (editor->context,
                                            gtk_widget_queue_draw,
                                            editor);
      g_object_unref (editor->context);
      editor->context = NULL;
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
    }

  g_object_notify (G_OBJECT (editor), "context");
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
