/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "display-types.h"
#include "tools/tools-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpprogress.h"

#include "widgets/gimpdialogfactory.h"

#include "tools/gimptool.h"
#include "tools/tool_manager.h"

#include "gimpdisplay.h"
#include "gimpdisplay-handlers.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-expose.h"
#include "gimpdisplayshell-handlers.h"
#include "gimpdisplayshell-icon.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpdisplayshell-scrollbars.h"
#include "gimpdisplayshell-transform.h"
#include "gimpimagewindow.h"

#include "gimp-intl.h"


#define FLUSH_NOW_INTERVAL      (G_TIME_SPAN_SECOND / 60)

#define PAINT_AREA_CHUNK_WIDTH  32
#define PAINT_AREA_CHUNK_HEIGHT 32


enum
{
  PROP_0,
  PROP_ID,
  PROP_GIMP,
  PROP_IMAGE,
  PROP_SHELL
};


typedef struct _GimpDisplayPrivate GimpDisplayPrivate;

struct _GimpDisplayPrivate
{
  gint            ID;           /*  unique identifier for this display  */

  GimpImage      *image;        /*  pointer to the associated image     */
  gint            instance;     /*  the instance # of this display as
                                 *  taken from the image at creation    */

  GeglRectangle   bounding_box;

  GtkWidget      *shell;
  cairo_region_t *update_region;

  guint64         last_flush_now;
};

#define GIMP_DISPLAY_GET_PRIVATE(display) \
        ((GimpDisplayPrivate *) gimp_display_get_instance_private ((GimpDisplay *) (display)))


/*  local function prototypes  */

static void     gimp_display_progress_iface_init  (GimpProgressInterface *iface);

static void     gimp_display_set_property           (GObject             *object,
                                                     guint                property_id,
                                                     const GValue        *value,
                                                     GParamSpec          *pspec);
static void     gimp_display_get_property           (GObject             *object,
                                                     guint                property_id,
                                                     GValue              *value,
                                                     GParamSpec          *pspec);

static GimpProgress * gimp_display_progress_start   (GimpProgress        *progress,
                                                     gboolean             cancellable,
                                                     const gchar         *message);
static void     gimp_display_progress_end           (GimpProgress        *progress);
static gboolean gimp_display_progress_is_active     (GimpProgress        *progress);
static void     gimp_display_progress_set_text      (GimpProgress        *progress,
                                                     const gchar         *message);
static void     gimp_display_progress_set_value     (GimpProgress        *progress,
                                                     gdouble              percentage);
static gdouble  gimp_display_progress_get_value     (GimpProgress        *progress);
static void     gimp_display_progress_pulse         (GimpProgress        *progress);
static guint32  gimp_display_progress_get_window_id (GimpProgress        *progress);
static gboolean gimp_display_progress_message       (GimpProgress        *progress,
                                                     Gimp                *gimp,
                                                     GimpMessageSeverity  severity,
                                                     const gchar         *domain,
                                                     const gchar         *message);
static void     gimp_display_progress_canceled      (GimpProgress        *progress,
                                                     GimpDisplay         *display);

static void     gimp_display_flush_whenever         (GimpDisplay         *display,
                                                     gboolean             now);
static void     gimp_display_paint_area             (GimpDisplay         *display,
                                                     gint                 x,
                                                     gint                 y,
                                                     gint                 w,
                                                     gint                 h);


G_DEFINE_TYPE_WITH_CODE (GimpDisplay, gimp_display, GIMP_TYPE_OBJECT,
                         G_ADD_PRIVATE (GimpDisplay)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PROGRESS,
                                                gimp_display_progress_iface_init))

#define parent_class gimp_display_parent_class


static void
gimp_display_class_init (GimpDisplayClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_display_set_property;
  object_class->get_property = gimp_display_get_property;

  g_object_class_install_property (object_class, PROP_ID,
                                   g_param_spec_int ("id",
                                                     NULL, NULL,
                                                     0, G_MAXINT, 0,
                                                     GIMP_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp",
                                                        NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_object ("image",
                                                        NULL, NULL,
                                                        GIMP_TYPE_IMAGE,
                                                        GIMP_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_SHELL,
                                   g_param_spec_object ("shell",
                                                        NULL, NULL,
                                                        GIMP_TYPE_DISPLAY_SHELL,
                                                        GIMP_PARAM_READABLE));
}

static void
gimp_display_init (GimpDisplay *display)
{
}

static void
gimp_display_progress_iface_init (GimpProgressInterface *iface)
{
  iface->start         = gimp_display_progress_start;
  iface->end           = gimp_display_progress_end;
  iface->is_active     = gimp_display_progress_is_active;
  iface->set_text      = gimp_display_progress_set_text;
  iface->set_value     = gimp_display_progress_set_value;
  iface->get_value     = gimp_display_progress_get_value;
  iface->pulse         = gimp_display_progress_pulse;
  iface->get_window_id = gimp_display_progress_get_window_id;
  iface->message       = gimp_display_progress_message;
}

static void
gimp_display_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  GimpDisplay        *display = GIMP_DISPLAY (object);
  GimpDisplayPrivate *private = GIMP_DISPLAY_GET_PRIVATE (display);

  switch (property_id)
    {
    case PROP_GIMP:
      {
        gint ID;

        display->gimp = g_value_get_object (value); /* don't ref the gimp */
        display->config = GIMP_DISPLAY_CONFIG (display->gimp->config);

        do
          {
            ID = display->gimp->next_display_ID++;

            if (display->gimp->next_display_ID == G_MAXINT)
              display->gimp->next_display_ID = 1;
          }
        while (gimp_display_get_by_ID (display->gimp, ID));

        private->ID = ID;
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_display_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  GimpDisplay        *display = GIMP_DISPLAY (object);
  GimpDisplayPrivate *private = GIMP_DISPLAY_GET_PRIVATE (display);

  switch (property_id)
    {
    case PROP_ID:
      g_value_set_int (value, private->ID);
      break;

    case PROP_GIMP:
      g_value_set_object (value, display->gimp);
      break;

    case PROP_IMAGE:
      g_value_set_object (value, private->image);
      break;

    case PROP_SHELL:
      g_value_set_object (value, private->shell);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GimpProgress *
gimp_display_progress_start (GimpProgress *progress,
                             gboolean      cancellable,
                             const gchar  *message)
{
  GimpDisplay        *display = GIMP_DISPLAY (progress);
  GimpDisplayPrivate *private = GIMP_DISPLAY_GET_PRIVATE (display);

  if (private->shell)
    return gimp_progress_start (GIMP_PROGRESS (private->shell), cancellable,
                                "%s", message);

  return NULL;
}

static void
gimp_display_progress_end (GimpProgress *progress)
{
  GimpDisplay        *display = GIMP_DISPLAY (progress);
  GimpDisplayPrivate *private = GIMP_DISPLAY_GET_PRIVATE (display);

  if (private->shell)
    gimp_progress_end (GIMP_PROGRESS (private->shell));
}

static gboolean
gimp_display_progress_is_active (GimpProgress *progress)
{
  GimpDisplay        *display = GIMP_DISPLAY (progress);
  GimpDisplayPrivate *private = GIMP_DISPLAY_GET_PRIVATE (display);

  if (private->shell)
    return gimp_progress_is_active (GIMP_PROGRESS (private->shell));

  return FALSE;
}

static void
gimp_display_progress_set_text (GimpProgress *progress,
                                const gchar  *message)
{
  GimpDisplay        *display = GIMP_DISPLAY (progress);
  GimpDisplayPrivate *private = GIMP_DISPLAY_GET_PRIVATE (display);

  if (private->shell)
    gimp_progress_set_text_literal (GIMP_PROGRESS (private->shell), message);
}

static void
gimp_display_progress_set_value (GimpProgress *progress,
                                 gdouble       percentage)
{
  GimpDisplay        *display = GIMP_DISPLAY (progress);
  GimpDisplayPrivate *private = GIMP_DISPLAY_GET_PRIVATE (display);

  if (private->shell)
    gimp_progress_set_value (GIMP_PROGRESS (private->shell), percentage);
}

static gdouble
gimp_display_progress_get_value (GimpProgress *progress)
{
  GimpDisplay        *display = GIMP_DISPLAY (progress);
  GimpDisplayPrivate *private = GIMP_DISPLAY_GET_PRIVATE (display);

  if (private->shell)
    return gimp_progress_get_value (GIMP_PROGRESS (private->shell));

  return 0.0;
}

static void
gimp_display_progress_pulse (GimpProgress *progress)
{
  GimpDisplay        *display = GIMP_DISPLAY (progress);
  GimpDisplayPrivate *private = GIMP_DISPLAY_GET_PRIVATE (display);

  if (private->shell)
    gimp_progress_pulse (GIMP_PROGRESS (private->shell));
}

static guint32
gimp_display_progress_get_window_id (GimpProgress *progress)
{
  GimpDisplay        *display = GIMP_DISPLAY (progress);
  GimpDisplayPrivate *private = GIMP_DISPLAY_GET_PRIVATE (display);

  if (private->shell)
    return gimp_progress_get_window_id (GIMP_PROGRESS (private->shell));

  return 0;
}

static gboolean
gimp_display_progress_message (GimpProgress        *progress,
                               Gimp                *gimp,
                               GimpMessageSeverity  severity,
                               const gchar         *domain,
                               const gchar         *message)
{
  GimpDisplay        *display = GIMP_DISPLAY (progress);
  GimpDisplayPrivate *private = GIMP_DISPLAY_GET_PRIVATE (display);

  if (private->shell)
    return gimp_progress_message (GIMP_PROGRESS (private->shell), gimp,
                                  severity, domain, message);

  return FALSE;
}

static void
gimp_display_progress_canceled (GimpProgress *progress,
                                GimpDisplay  *display)
{
  gimp_progress_cancel (GIMP_PROGRESS (display));
}


/*  public functions  */

GimpDisplay *
gimp_display_new (Gimp              *gimp,
                  GimpImage         *image,
                  GimpUnit           unit,
                  gdouble            scale,
                  GimpUIManager     *popup_manager,
                  GimpDialogFactory *dialog_factory,
                  GdkScreen         *screen,
                  gint               monitor)
{
  GimpDisplay        *display;
  GimpDisplayPrivate *private;
  GimpImageWindow    *window = NULL;
  GimpDisplayShell   *shell;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (image == NULL || GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);

  /*  If there isn't an interface, never create a display  */
  if (gimp->no_interface)
    return NULL;

  display = g_object_new (GIMP_TYPE_DISPLAY,
                          "gimp", gimp,
                          NULL);

  private = GIMP_DISPLAY_GET_PRIVATE (display);

  /*  refs the image  */
  if (image)
    gimp_display_set_image (display, image);

  /*  get an image window  */
  if (GIMP_GUI_CONFIG (display->config)->single_window_mode)
    {
      GimpDisplay *active_display;

      active_display = gimp_context_get_display (gimp_get_user_context (gimp));

      if (! active_display)
        {
          active_display =
            GIMP_DISPLAY (gimp_container_get_first_child (gimp->displays));
        }

      if (active_display)
        {
          GimpDisplayShell *shell = gimp_display_get_shell (active_display);

          window = gimp_display_shell_get_window (shell);
        }
    }

  if (! window)
    {
      window = gimp_image_window_new (gimp,
                                      private->image,
                                      dialog_factory,
                                      screen,
                                      monitor);
    }

  /*  create the shell for the image  */
  private->shell = gimp_display_shell_new (display, unit, scale,
                                           popup_manager,
                                           screen,
                                           monitor);

  shell = gimp_display_get_shell (display);

  gimp_display_update_bounding_box (display);

  gimp_image_window_add_shell (window, shell);
  gimp_display_shell_present (shell);

  /* make sure the docks are visible, in case all other image windows
   * are iconified, see bug #686544.
   */
  gimp_dialog_factory_show_with_display (dialog_factory);

  g_signal_connect (gimp_display_shell_get_statusbar (shell), "cancel",
                    G_CALLBACK (gimp_display_progress_canceled),
                    display);

  /* add the display to the list */
  gimp_container_add (gimp->displays, GIMP_OBJECT (display));

  return display;
}

/**
 * gimp_display_delete:
 * @display:
 *
 * Closes the display and removes it from the display list. You should
 * not call this function directly, use gimp_display_close() instead.
 */
void
gimp_display_delete (GimpDisplay *display)
{
  GimpDisplayPrivate *private;
  GimpTool           *active_tool;

  g_return_if_fail (GIMP_IS_DISPLAY (display));

  private = GIMP_DISPLAY_GET_PRIVATE (display);

  /* remove the display from the list */
  gimp_container_remove (display->gimp->displays, GIMP_OBJECT (display));

  /*  unrefs the image  */
  gimp_display_set_image (display, NULL);

  active_tool = tool_manager_get_active (display->gimp);

  if (active_tool && active_tool->focus_display == display)
    tool_manager_focus_display_active (display->gimp, NULL);

  if (private->shell)
    {
      GimpDisplayShell *shell  = gimp_display_get_shell (display);
      GimpImageWindow  *window = gimp_display_shell_get_window (shell);

      /*  set private->shell to NULL *before* destroying the shell.
       *  all callbacks in gimpdisplayshell-callbacks.c will check
       *  this pointer and do nothing if the shell is in destruction.
       */
      private->shell = NULL;

      if (window)
        {
          if (gimp_image_window_get_n_shells (window) > 1)
            {
              g_object_ref (shell);

              gimp_image_window_remove_shell (window, shell);
              gtk_widget_destroy (GTK_WIDGET (shell));

              g_object_unref (shell);
            }
          else
            {
              gimp_image_window_destroy (window);
            }
        }
      else
        {
          g_object_unref (shell);
        }
    }

  g_object_unref (display);
}

/**
 * gimp_display_close:
 * @display:
 *
 * Closes the display. If this is the last display, it will remain
 * open, but without an image.
 */
void
gimp_display_close (GimpDisplay *display)
{
  g_return_if_fail (GIMP_IS_DISPLAY (display));

  if (gimp_container_get_n_children (display->gimp->displays) > 1)
    {
      gimp_display_delete (display);
    }
  else
    {
      gimp_display_empty (display);
    }
}

gint
gimp_display_get_ID (GimpDisplay *display)
{
  GimpDisplayPrivate *private;

  g_return_val_if_fail (GIMP_IS_DISPLAY (display), -1);

  private = GIMP_DISPLAY_GET_PRIVATE (display);

  return private->ID;
}

GimpDisplay *
gimp_display_get_by_ID (Gimp *gimp,
                        gint  ID)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  for (list = gimp_get_display_iter (gimp);
       list;
       list = g_list_next (list))
    {
      GimpDisplay *display = list->data;

      if (gimp_display_get_ID (display) == ID)
        return display;
    }

  return NULL;
}

/**
 * gimp_display_get_action_name:
 * @display:
 *
 * Returns: The action name for the given display. The action name
 * depends on the display ID. The result must be freed with g_free().
 **/
gchar *
gimp_display_get_action_name (GimpDisplay *display)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), NULL);

  return g_strdup_printf ("windows-display-%04d",
                          gimp_display_get_ID (display));
}

Gimp *
gimp_display_get_gimp (GimpDisplay *display)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), NULL);

  return display->gimp;
}

GimpImage *
gimp_display_get_image (GimpDisplay *display)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), NULL);

  return GIMP_DISPLAY_GET_PRIVATE (display)->image;
}

void
gimp_display_set_image (GimpDisplay *display,
                        GimpImage   *image)
{
  GimpDisplayPrivate *private;
  GimpImage          *old_image = NULL;
  GimpDisplayShell   *shell;

  g_return_if_fail (GIMP_IS_DISPLAY (display));
  g_return_if_fail (image == NULL || GIMP_IS_IMAGE (image));

  private = GIMP_DISPLAY_GET_PRIVATE (display);

  shell = gimp_display_get_shell (display);

  if (private->image)
    {
      /*  stop any active tool  */
      tool_manager_control_active (display->gimp, GIMP_TOOL_ACTION_HALT,
                                   display);

      gimp_display_shell_disconnect (shell);

      gimp_display_disconnect (display);

      g_clear_pointer (&private->update_region, cairo_region_destroy);

      gimp_image_dec_display_count (private->image);

      /*  set private->image before unrefing because there may be code
       *  that listens for image removals and then iterates the
       *  display list to find a valid display.
       */
      old_image = private->image;

#if 0
      g_print ("%s: image->ref_count before unrefing: %d\n",
               G_STRFUNC, G_OBJECT (old_image)->ref_count);
#endif
    }

  private->image = image;

  if (image)
    {
#if 0
      g_print ("%s: image->ref_count before refing: %d\n",
               G_STRFUNC, G_OBJECT (image)->ref_count);
#endif

      g_object_ref (image);

      private->instance = gimp_image_get_instance_count (image);
      gimp_image_inc_instance_count (image);

      gimp_image_inc_display_count (image);

      gimp_display_connect (display);

      if (shell)
        gimp_display_shell_connect (shell);
    }

  if (old_image)
    g_object_unref (old_image);

  gimp_display_update_bounding_box (display);

  if (shell)
    {
      if (image)
        gimp_display_shell_reconnect (shell);
      else
        gimp_display_shell_icon_update (shell);
    }

  if (old_image != image)
    g_object_notify (G_OBJECT (display), "image");
}

gint
gimp_display_get_instance (GimpDisplay *display)
{
  GimpDisplayPrivate *private;

  g_return_val_if_fail (GIMP_IS_DISPLAY (display), 0);

  private = GIMP_DISPLAY_GET_PRIVATE (display);

  return private->instance;
}

GimpDisplayShell *
gimp_display_get_shell (GimpDisplay *display)
{
  GimpDisplayPrivate *private;

  g_return_val_if_fail (GIMP_IS_DISPLAY (display), NULL);

  private = GIMP_DISPLAY_GET_PRIVATE (display);

  return GIMP_DISPLAY_SHELL (private->shell);
}

void
gimp_display_empty (GimpDisplay *display)
{
  GimpDisplayPrivate *private;
  GList              *iter;

  g_return_if_fail (GIMP_IS_DISPLAY (display));

  private = GIMP_DISPLAY_GET_PRIVATE (display);

  g_return_if_fail (GIMP_IS_IMAGE (private->image));

  for (iter = display->gimp->context_list; iter; iter = g_list_next (iter))
    {
      GimpContext *context = iter->data;

      if (gimp_context_get_display (context) == display)
        gimp_context_set_image (context, NULL);
    }

  gimp_display_set_image (display, NULL);

  gimp_display_shell_empty (gimp_display_get_shell (display));
}

void
gimp_display_fill (GimpDisplay *display,
                   GimpImage   *image,
                   GimpUnit     unit,
                   gdouble      scale)
{
  GimpDisplayPrivate *private;

  g_return_if_fail (GIMP_IS_DISPLAY (display));
  g_return_if_fail (GIMP_IS_IMAGE (image));

  private = GIMP_DISPLAY_GET_PRIVATE (display);

  g_return_if_fail (private->image == NULL);

  gimp_display_set_image (display, image);

  gimp_display_shell_fill (gimp_display_get_shell (display),
                           image, unit, scale);
}

void
gimp_display_update_bounding_box (GimpDisplay *display)
{
  GimpDisplayPrivate *private;
  GimpDisplayShell   *shell;
  GeglRectangle       bounding_box = {};

  g_return_if_fail (GIMP_IS_DISPLAY (display));

  private = GIMP_DISPLAY_GET_PRIVATE (display);
  shell   = gimp_display_get_shell (display);

  if (shell)
    {
      bounding_box = gimp_display_shell_get_bounding_box (shell);

      if (! gegl_rectangle_equal (&bounding_box, &private->bounding_box))
        {
          GeglRectangle diff_rects[4];
          gint          n_diff_rects;
          gint          i;

          n_diff_rects = gegl_rectangle_subtract (diff_rects,
                                                  &private->bounding_box,
                                                  &bounding_box);

          for (i = 0; i < n_diff_rects; i++)
            {
              gimp_display_paint_area (display,
                                       diff_rects[i].x,     diff_rects[i].y,
                                       diff_rects[i].width, diff_rects[i].height);
            }

          private->bounding_box = bounding_box;

          gimp_display_shell_scroll_clamp_and_update (shell);
          gimp_display_shell_scrollbars_update (shell);
        }
    }
  else
    {
      private->bounding_box = bounding_box;
    }
}

void
gimp_display_update_area (GimpDisplay *display,
                          gboolean     now,
                          gint         x,
                          gint         y,
                          gint         w,
                          gint         h)
{
  GimpDisplayPrivate *private;

  g_return_if_fail (GIMP_IS_DISPLAY (display));

  private = GIMP_DISPLAY_GET_PRIVATE (display);

  if (now)
    {
      gimp_display_paint_area (display, x, y, w, h);
    }
  else
    {
      cairo_rectangle_int_t rect;
      gint                  image_width;
      gint                  image_height;

      image_width  = gimp_image_get_width  (private->image);
      image_height = gimp_image_get_height (private->image);

      rect.x      = CLAMP (x,     0, image_width);
      rect.y      = CLAMP (y,     0, image_height);
      rect.width  = CLAMP (x + w, 0, image_width)  - rect.x;
      rect.height = CLAMP (y + h, 0, image_height) - rect.y;

      if (private->update_region)
        cairo_region_union_rectangle (private->update_region, &rect);
      else
        private->update_region = cairo_region_create_rectangle (&rect);
    }
}

void
gimp_display_flush (GimpDisplay *display)
{
  g_return_if_fail (GIMP_IS_DISPLAY (display));

  gimp_display_flush_whenever (display, FALSE);
}

void
gimp_display_flush_now (GimpDisplay *display)
{
  g_return_if_fail (GIMP_IS_DISPLAY (display));

  gimp_display_flush_whenever (display, TRUE);
}


/*  private functions  */

static void
gimp_display_flush_whenever (GimpDisplay *display,
                             gboolean     now)
{
  GimpDisplayPrivate *private = GIMP_DISPLAY_GET_PRIVATE (display);

  if (private->update_region)
    {
      gint n_rects = cairo_region_num_rectangles (private->update_region);
      gint i;

      for (i = 0; i < n_rects; i++)
        {
          cairo_rectangle_int_t rect;

          cairo_region_get_rectangle (private->update_region,
                                      i, &rect);

          gimp_display_paint_area (display,
                                   rect.x,
                                   rect.y,
                                   rect.width,
                                   rect.height);
        }

      g_clear_pointer (&private->update_region, cairo_region_destroy);
    }

  if (now)
    {
      guint64 now = g_get_monotonic_time ();

      if ((now - private->last_flush_now) > FLUSH_NOW_INTERVAL)
        {
          gimp_display_shell_flush (gimp_display_get_shell (display), TRUE);

          private->last_flush_now = now;
        }
    }
  else
    {
      gimp_display_shell_flush (gimp_display_get_shell (display), now);
    }
}

static void
gimp_display_paint_area (GimpDisplay *display,
                         gint         x,
                         gint         y,
                         gint         w,
                         gint         h)
{
  GimpDisplayPrivate *private = GIMP_DISPLAY_GET_PRIVATE (display);
  GimpDisplayShell   *shell   = gimp_display_get_shell (display);
  GeglRectangle       rect;
  gint                x1, y1, x2, y2;
  gdouble             x1_f, y1_f, x2_f, y2_f;

  if (! gegl_rectangle_intersect (&rect,
                                  &private->bounding_box,
                                  GEGL_RECTANGLE (x, y, w, h)))
    {
      return;
    }

  /*  display the area  */
  gimp_display_shell_transform_bounds (shell,
                                       rect.x,
                                       rect.y,
                                       rect.x + rect.width,
                                       rect.y + rect.height,
                                       &x1_f, &y1_f, &x2_f, &y2_f);

  /*  make sure to expose a superset of the transformed sub-pixel expose
   *  area, not a subset. bug #126942. --mitch
   *
   *  also accommodate for spill introduced by potential box filtering.
   *  (bug #474509). --simon
   */
  x1 = floor (x1_f - 0.5);
  y1 = floor (y1_f - 0.5);
  x2 = ceil (x2_f + 0.5);
  y2 = ceil (y2_f + 0.5);

  /*  align transformed area to a coarse grid, to simplify the
   *  invalidated area
   */
  x1 = floor ((gdouble) x1 / PAINT_AREA_CHUNK_WIDTH)  * PAINT_AREA_CHUNK_WIDTH;
  y1 = floor ((gdouble) y1 / PAINT_AREA_CHUNK_HEIGHT) * PAINT_AREA_CHUNK_HEIGHT;
  x2 = ceil  ((gdouble) x2 / PAINT_AREA_CHUNK_WIDTH)  * PAINT_AREA_CHUNK_WIDTH;
  y2 = ceil  ((gdouble) y2 / PAINT_AREA_CHUNK_HEIGHT) * PAINT_AREA_CHUNK_HEIGHT;

  gimp_display_shell_expose_area (shell, x1, y1, x2 - x1, y2 - y1);
}
