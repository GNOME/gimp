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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-colormap.h"
#include "core/gimpmarshal.h"
#include "core/gimppalette.h"

#include "gimpdnd.h"
#include "gimpfgbgeditor.h"
#include "gimpwidgets-utils.h"

#define CHANNEL_EPSILON 1e-3

enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_ACTIVE_COLOR
};

enum
{
  COLOR_CLICKED,
  TOOLTIP,
  LAST_SIGNAL
};


static void     gimp_fg_bg_editor_dispose           (GObject          *object);
static void     gimp_fg_bg_editor_set_property      (GObject          *object,
                                                     guint             property_id,
                                                     const GValue     *value,
                                                     GParamSpec       *pspec);
static void     gimp_fg_bg_editor_get_property      (GObject          *object,
                                                     guint             property_id,
                                                     GValue           *value,
                                                     GParamSpec       *pspec);

static GtkSizeRequestMode
                gimp_fg_bg_editor_get_request_mode  (GtkWidget        *widget);
static void
   gimp_fg_bg_editor_get_preferred_width_for_height (GtkWidget        *widget,
                                                     gint              height,
                                                     gint             *minimum_width,
                                                     gint             *natural_width);

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
static gboolean gimp_fg_bg_editor_query_tooltip     (GtkWidget        *widget,
                                                     gint              x,
                                                     gint              y,
                                                     gboolean          keyboard_mode,
                                                     GtkTooltip       *tooltip);

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

static void     gimp_fg_bg_editor_image_changed     (GimpFgBgEditor   *editor,
                                                     GimpImage        *image);

static void     gimp_fg_bg_editor_draw_color_frame  (GimpFgBgEditor   *editor,
                                                     cairo_t          *cr,
                                                     const GimpRGB    *color,
                                                     gint              x,
                                                     gint              y,
                                                     gint              width,
                                                     gint              height,
                                                     gint              corner_dx,
                                                     gint              corner_dy);

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
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_ACTIVE_COLOR);

  editor_signals[TOOLTIP] =
    g_signal_new ("tooltip",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpFgBgEditorClass, tooltip),
                  NULL, NULL,
                  gimp_marshal_VOID__INT_OBJECT,
                  G_TYPE_NONE, 2,
                  G_TYPE_INT,
                  GTK_TYPE_TOOLTIP);

  object_class->dispose              = gimp_fg_bg_editor_dispose;
  object_class->set_property         = gimp_fg_bg_editor_set_property;
  object_class->get_property         = gimp_fg_bg_editor_get_property;

  widget_class->get_request_mode               = gimp_fg_bg_editor_get_request_mode;
  widget_class->get_preferred_width_for_height = gimp_fg_bg_editor_get_preferred_width_for_height;
  widget_class->style_updated                  = gimp_fg_bg_editor_style_updated;
  widget_class->draw                           = gimp_fg_bg_editor_draw;
  widget_class->button_press_event             = gimp_fg_bg_editor_button_press;
  widget_class->button_release_event           = gimp_fg_bg_editor_button_release;
  widget_class->drag_motion                    = gimp_fg_bg_editor_drag_motion;
  widget_class->query_tooltip                  = gimp_fg_bg_editor_query_tooltip;

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

  gtk_widget_class_set_css_name (widget_class, "GimpFgBgEditor");
}

static void
gimp_fg_bg_editor_init (GimpFgBgEditor *editor)
{
  editor->active_color = GIMP_ACTIVE_COLOR_FOREGROUND;

  gtk_widget_set_can_focus (GTK_WIDGET (editor), FALSE);
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
                             NULL, NULL);

  gtk_widget_set_size_request (GTK_WIDGET (editor), 32, 24);
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

static GtkSizeRequestMode
gimp_fg_bg_editor_get_request_mode (GtkWidget *widget)
{
  return GTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT;
}

static void
gimp_fg_bg_editor_get_preferred_width_for_height (GtkWidget *widget,
                                                  gint       height,
                                                  gint      *minimum_width,
                                                  gint      *natural_width)
{
  *minimum_width = height;
  *natural_width = height;
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
  GimpFgBgEditor  *editor = GIMP_FG_BG_EDITOR (widget);
  GtkStyleContext *style  = gtk_widget_get_style_context (widget);
  GtkBorder        border;
  GtkBorder        padding;
  GdkRectangle     rect;
  gint             scale_factor;
  gint             width, height;
  gint             default_w, default_h;
  gint             swap_w, swap_h;
  GimpRGB          color;

  gtk_style_context_save (style);

  width  = gtk_widget_get_allocated_width  (widget);
  height = gtk_widget_get_allocated_height (widget);

  gtk_style_context_get_border (style, gtk_style_context_get_state (style),
                                &border);
  gtk_style_context_get_padding (style, gtk_style_context_get_state (style),
                                 &padding);

  border.left   += padding.left;
  border.right  += padding.right;
  border.top    += padding.top;
  border.bottom += padding.bottom;

  scale_factor = gtk_widget_get_scale_factor (widget);

  /*  draw the default colors pixbuf  */
  if (! editor->default_icon)
    editor->default_icon = gimp_widget_load_icon (widget,
                                                  GIMP_ICON_COLORS_DEFAULT, 12);

  default_w = gdk_pixbuf_get_width  (editor->default_icon) / scale_factor;
  default_h = gdk_pixbuf_get_height (editor->default_icon) / scale_factor;

  if (default_w < width / 2 && default_h < height / 2)
    {
      cairo_surface_t *surface;

      surface = gdk_cairo_surface_create_from_pixbuf (editor->default_icon,
                                                      scale_factor, NULL);
      cairo_set_source_surface (cr, surface,
                                border.left,
                                height - border.bottom - default_h);
      cairo_surface_destroy (surface);

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

  swap_w = gdk_pixbuf_get_width  (editor->swap_icon) / scale_factor;
  swap_h = gdk_pixbuf_get_height (editor->swap_icon) / scale_factor;

  if (swap_w < width / 2 && swap_h < height / 2)
    {
      cairo_surface_t *surface;

      surface = gdk_cairo_surface_create_from_pixbuf (editor->swap_icon,
                                                      scale_factor, NULL);
      cairo_set_source_surface (cr, surface,
                                width - border.right - swap_w,
                                border.top);
      cairo_surface_destroy (surface);

      cairo_paint (cr);
    }
  else
    {
      swap_w = swap_h = 0;
    }

  rect.width  = width  - MAX (default_w, swap_w) - 4 - border.top  - border.bottom;
  rect.height = height - MAX (default_h, swap_h) - 2 - border.left - border.right;

  if (rect.height > (height * 3 / 4))
    rect.width = MAX (rect.width - (rect.height - ((height * 3 / 4))),
                      width * 2 / 3);

  editor->rect_width  = rect.width;
  editor->rect_height = rect.height;

  if (! editor->transform)
    gimp_fg_bg_editor_create_transform (editor);

  if (editor->context)
    {
      /*  draw the background frame  */
      gimp_context_get_background (editor->context, &color);
      rect.x = width  - rect.width  - border.right;
      rect.y = height - rect.height - border.bottom;
      gimp_fg_bg_editor_draw_color_frame (editor, cr, &color,
                                          rect.x,     rect.y,
                                          rect.width, rect.height,
                                          +1,         +1);

      /*  draw the foreground frame  */
      gimp_context_get_foreground (editor->context, &color);
      rect.x = border.left;
      rect.y = border.top;
      gimp_fg_bg_editor_draw_color_frame (editor, cr, &color,
                                          rect.x,     rect.y,
                                          rect.width, rect.height,
                                          +1,         +1);
    }

  gtk_style_context_restore (style);

  return TRUE;
}

static GimpFgBgTarget
gimp_fg_bg_editor_target (GimpFgBgEditor *editor,
                          gint            x,
                          gint            y)
{
  GtkWidget       *widget = GTK_WIDGET (editor);
  GtkStyleContext *style  = gtk_widget_get_style_context (widget);
  GtkBorder        border;
  GtkBorder        padding;
  gint             width;
  gint             height;
  gint             rect_w = editor->rect_width;
  gint             rect_h = editor->rect_height;
  gint             button_width;
  gint             button_height;

  width  = gtk_widget_get_allocated_width  (widget);
  height = gtk_widget_get_allocated_height (widget);

  gtk_style_context_get_border (style, gtk_style_context_get_state (style),
                                &border);
  gtk_style_context_get_padding (style, gtk_style_context_get_state (style),
                                 &padding);

  border.left   += padding.left;
  border.right  += padding.right;
  border.top    += padding.top;
  border.bottom += padding.bottom;

  button_width  = width  - border.left - border.right  - rect_w;
  button_height = height - border.top  - border.bottom - rect_h;

  if (x > border.left          &&
      x < border.left + rect_w &&
      y > border.top           &&
      y < border.top + rect_h)
    {
      return GIMP_FG_BG_TARGET_FOREGROUND;
    }
  else if (x > width  - border.right - rect_w  &&
           x < width  - border.right           &&
           y > height - border.bottom - rect_h &&
           y < height - border.bottom)
    {
      return GIMP_FG_BG_TARGET_BACKGROUND;
    }
  else if (x > border.left                &&
           x < border.left + button_width &&
           y > border.top + rect_h        &&
           y < height - border.bottom)
    {
      return GIMP_FG_BG_TARGET_DEFAULT;
    }
  else if (x > border.left + rect_w &&
           x < width - border.right &&
           y > border.top           &&
           y < border.top + button_height)
    {
      return GIMP_FG_BG_TARGET_SWAP;
    }

  return GIMP_FG_BG_TARGET_INVALID;
}

static gboolean
gimp_fg_bg_editor_button_press (GtkWidget      *widget,
                                GdkEventButton *bevent)
{
  GimpFgBgEditor *editor = GIMP_FG_BG_EDITOR (widget);

  if (bevent->button == 1 && bevent->type == GDK_BUTTON_PRESS)
    {
      GimpFgBgTarget target = gimp_fg_bg_editor_target (editor,
                                                        bevent->x, bevent->y);

      editor->click_target = GIMP_FG_BG_TARGET_INVALID;

      switch (target)
        {
        case GIMP_FG_BG_TARGET_FOREGROUND:
          if (editor->active_color != GIMP_ACTIVE_COLOR_FOREGROUND)
            gimp_fg_bg_editor_set_active (editor,
                                          GIMP_ACTIVE_COLOR_FOREGROUND);
          editor->click_target = GIMP_FG_BG_TARGET_FOREGROUND;
          break;

        case GIMP_FG_BG_TARGET_BACKGROUND:
          if (editor->active_color != GIMP_ACTIVE_COLOR_BACKGROUND)
            gimp_fg_bg_editor_set_active (editor,
                                          GIMP_ACTIVE_COLOR_BACKGROUND);
          editor->click_target = GIMP_FG_BG_TARGET_BACKGROUND;
          break;

        case GIMP_FG_BG_TARGET_SWAP:
          if (editor->context)
            gimp_context_swap_colors (editor->context);
          break;

        case GIMP_FG_BG_TARGET_DEFAULT:
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
      GimpFgBgTarget target = gimp_fg_bg_editor_target (editor,
                                                        bevent->x, bevent->y);

      if (target == editor->click_target)
        {
          switch (target)
            {
            case GIMP_FG_BG_TARGET_FOREGROUND:
              g_signal_emit (editor, editor_signals[COLOR_CLICKED], 0,
                             GIMP_ACTIVE_COLOR_FOREGROUND);
              break;

            case GIMP_FG_BG_TARGET_BACKGROUND:
              g_signal_emit (editor, editor_signals[COLOR_CLICKED], 0,
                             GIMP_ACTIVE_COLOR_BACKGROUND);
              break;

            default:
              break;
            }
        }

      editor->click_target = GIMP_FG_BG_TARGET_INVALID;
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
  GimpFgBgTarget  target = gimp_fg_bg_editor_target (editor, x, y);

  if (target == GIMP_FG_BG_TARGET_FOREGROUND ||
      target == GIMP_FG_BG_TARGET_BACKGROUND)
    {
      gdk_drag_status (context, GDK_ACTION_COPY, time);

      return TRUE;
    }

  gdk_drag_status (context, 0, time);

  return FALSE;
}

static gboolean
gimp_fg_bg_editor_query_tooltip (GtkWidget  *widget,
                                 gint        x,
                                 gint        y,
                                 gboolean    keyboard_mode,
                                 GtkTooltip *tooltip)
{
  if (! keyboard_mode)
    {
      GimpFgBgEditor *editor = GIMP_FG_BG_EDITOR (widget);
      GimpFgBgTarget  target = gimp_fg_bg_editor_target (editor, x, y);

      if (target != GIMP_FG_BG_TARGET_INVALID)
        {
          g_signal_emit (widget, editor_signals[TOOLTIP], 0,
                         target, tooltip);

          return TRUE;
        }
    }

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
          g_signal_handlers_disconnect_by_func (editor->context,
                                                G_CALLBACK (gimp_fg_bg_editor_image_changed),
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
          g_signal_connect_swapped (context, "image-changed",
                                    G_CALLBACK (gimp_fg_bg_editor_image_changed),
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
        case GIMP_FG_BG_TARGET_FOREGROUND:
          gimp_context_set_foreground (editor->context, color);
          break;

        case GIMP_FG_BG_TARGET_BACKGROUND:
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

static void
gimp_fg_bg_editor_image_changed (GimpFgBgEditor *editor,
                                 GimpImage      *image)
{
  gtk_widget_queue_draw (GTK_WIDGET (editor));

  if (editor->active_image)
    {
      g_signal_handlers_disconnect_by_func (editor->active_image,
                                            G_CALLBACK (gtk_widget_queue_draw),
                                            editor);
      if (gimp_image_get_base_type (editor->active_image) == GIMP_INDEXED)
        {
          GimpPalette *palette;

          palette = gimp_image_get_colormap_palette (editor->active_image);
          g_signal_handlers_disconnect_by_func (palette,
                                                G_CALLBACK (gtk_widget_queue_draw),
                                                editor);
        }
    }
  editor->active_image = image;
  if (image)
    {
      g_signal_connect_swapped (image, "notify::base-type",
                                G_CALLBACK (gtk_widget_queue_draw),
                                editor);
      g_signal_connect_swapped (image, "colormap-changed",
                                G_CALLBACK (gtk_widget_queue_draw),
                                editor);

      if (gimp_image_get_base_type (image) == GIMP_INDEXED)
        {
          GimpPalette *palette;

          palette = gimp_image_get_colormap_palette (editor->active_image);
          g_signal_connect_swapped (palette, "dirty",
                                    G_CALLBACK (gtk_widget_queue_draw),
                                    editor);
        }
    }
}

static void
gimp_fg_bg_editor_draw_color_frame (GimpFgBgEditor *editor,
                                    cairo_t        *cr,
                                    const GimpRGB  *color,
                                    gint            x,
                                    gint            y,
                                    gint            width,
                                    gint            height,
                                    gint            corner_dx,
                                    gint            corner_dy)
{
  GimpPalette       *colormap_palette = NULL;
  GimpImageBaseType  base_type        = GIMP_RGB;
  GimpRGB            transformed_color;

  if (editor->active_image)
    {
      base_type = gimp_image_get_base_type (editor->active_image);

      if (base_type == GIMP_INDEXED)
        {
          colormap_palette = gimp_image_get_colormap_palette (
            editor->active_image);
        }
    }

  if (editor->transform)
    {
      gimp_color_transform_process_pixels (editor->transform,
                                           babl_format ("R'G'B'A double"),
                                           color,
                                           babl_format ("R'G'B'A double"),
                                           &transformed_color,
                                           1);
    }
  else
    {
      transformed_color = *color;
    }

  cairo_save (cr);

  gimp_cairo_set_source_rgb (cr, &transformed_color);

  cairo_rectangle (cr, x, y, width, height);
  cairo_fill (cr);

  if (editor->color_config &&
      /* Common out-of-gamut case */
      ((color->r < 0.0 || color->r > 1.0 ||
        color->g < 0.0 || color->g > 1.0 ||
        color->b < 0.0 || color->b > 1.0) ||
       /* Indexed images */
       (colormap_palette &&
        ! gimp_palette_find_entry (colormap_palette, color, NULL)) ||
       /* Grayscale images */
       (base_type == GIMP_GRAY &&
        (ABS (color->r - color->g) > CHANNEL_EPSILON ||
         ABS (color->r - color->b) > CHANNEL_EPSILON ||
         ABS (color->g - color->b) > CHANNEL_EPSILON))))
    {
      gint    corner_x = x + 0.5 * (1.0 + corner_dx) * width;
      gint    corner_y = y + 0.5 * (1.0 + corner_dy) * height;
      gint    side     = MIN (width, height) * 2 / 3;
      GimpRGB out_of_gamut_color;

      cairo_move_to (cr, corner_x, corner_y);
      cairo_line_to (cr, corner_x + side * corner_dx, corner_y);
      cairo_line_to (cr, corner_x, corner_y + side * corner_dy);
      cairo_close_path (cr);

      gimp_color_config_get_out_of_gamut_color (editor->color_config,
                                                &out_of_gamut_color);
      gimp_cairo_set_source_rgb (cr, &out_of_gamut_color);
      cairo_fill (cr);
    }

  cairo_set_line_width (cr, 1.0);

  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
  cairo_rectangle (cr, x + 0.5, y + 0.5, width - 1.0, height - 1.0);
  cairo_stroke (cr);

  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_rectangle (cr, x + 1.5, y + 1.5, width - 3.0, height - 3.0);
  cairo_stroke (cr);

  cairo_restore (cr);
}
