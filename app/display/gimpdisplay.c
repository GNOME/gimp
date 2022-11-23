/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "display-types.h"
#include "tools/tools-types.h"

#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmaprogress.h"

#include "widgets/ligmadialogfactory.h"

#include "tools/ligmatool.h"
#include "tools/tool_manager.h"

#include "ligmadisplay.h"
#include "ligmadisplay-handlers.h"
#include "ligmadisplayshell.h"
#include "ligmadisplayshell-expose.h"
#include "ligmadisplayshell-handlers.h"
#include "ligmadisplayshell-render.h"
#include "ligmadisplayshell-scroll.h"
#include "ligmadisplayshell-scrollbars.h"
#include "ligmadisplayshell-title.h"
#include "ligmadisplayshell-transform.h"
#include "ligmaimagewindow.h"

#include "ligma-intl.h"


#define PAINT_AREA_CHUNK_WIDTH  32
#define PAINT_AREA_CHUNK_HEIGHT 32


enum
{
  PROP_0,
  PROP_IMAGE,
  PROP_SHELL
};


struct _LigmaDisplayImplPrivate
{
  LigmaImage      *image;        /*  pointer to the associated image     */
  gint            instance;     /*  the instance # of this display as
                                 *  taken from the image at creation    */

  GeglRectangle   bounding_box;

  GtkWidget      *shell;
  cairo_region_t *update_region;
};


/*  local function prototypes  */

static void     ligma_display_progress_iface_init  (LigmaProgressInterface *iface);

static void     ligma_display_set_property           (GObject             *object,
                                                     guint                property_id,
                                                     const GValue        *value,
                                                     GParamSpec          *pspec);
static void     ligma_display_get_property           (GObject             *object,
                                                     guint                property_id,
                                                     GValue              *value,
                                                     GParamSpec          *pspec);

static gboolean ligma_display_impl_present           (LigmaDisplay         *display);
static gboolean ligma_display_impl_grab_focus        (LigmaDisplay         *display);

static LigmaProgress * ligma_display_progress_start   (LigmaProgress        *progress,
                                                     gboolean             cancellable,
                                                     const gchar         *message);
static void     ligma_display_progress_end           (LigmaProgress        *progress);
static gboolean ligma_display_progress_is_active     (LigmaProgress        *progress);
static void     ligma_display_progress_set_text      (LigmaProgress        *progress,
                                                     const gchar         *message);
static void     ligma_display_progress_set_value     (LigmaProgress        *progress,
                                                     gdouble              percentage);
static gdouble  ligma_display_progress_get_value     (LigmaProgress        *progress);
static void     ligma_display_progress_pulse         (LigmaProgress        *progress);
static guint32  ligma_display_progress_get_window_id (LigmaProgress        *progress);
static gboolean ligma_display_progress_message       (LigmaProgress        *progress,
                                                     Ligma                *ligma,
                                                     LigmaMessageSeverity  severity,
                                                     const gchar         *domain,
                                                     const gchar         *message);
static void     ligma_display_progress_canceled      (LigmaProgress        *progress,
                                                     LigmaDisplay         *display);

static void     ligma_display_flush_update_region    (LigmaDisplay         *display);
static void     ligma_display_paint_area             (LigmaDisplay         *display,
                                                     gint                 x,
                                                     gint                 y,
                                                     gint                 w,
                                                     gint                 h);


G_DEFINE_TYPE_WITH_CODE (LigmaDisplayImpl, ligma_display_impl, LIGMA_TYPE_DISPLAY,
                         G_ADD_PRIVATE (LigmaDisplayImpl)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_PROGRESS,
                                                ligma_display_progress_iface_init))

#define parent_class ligma_display_impl_parent_class


static void
ligma_display_impl_class_init (LigmaDisplayImplClass *klass)
{
  GObjectClass     *object_class  = G_OBJECT_CLASS (klass);
  LigmaDisplayClass *display_class = LIGMA_DISPLAY_CLASS (klass);

  object_class->set_property = ligma_display_set_property;
  object_class->get_property = ligma_display_get_property;

  display_class->present     = ligma_display_impl_present;
  display_class->grab_focus  = ligma_display_impl_grab_focus;

  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_object ("image",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_IMAGE,
                                                        LIGMA_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_SHELL,
                                   g_param_spec_object ("shell",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_DISPLAY_SHELL,
                                                        LIGMA_PARAM_READABLE));
}

static void
ligma_display_impl_init (LigmaDisplayImpl *display)
{
  display->priv = ligma_display_impl_get_instance_private (display);
}

static void
ligma_display_progress_iface_init (LigmaProgressInterface *iface)
{
  iface->start         = ligma_display_progress_start;
  iface->end           = ligma_display_progress_end;
  iface->is_active     = ligma_display_progress_is_active;
  iface->set_text      = ligma_display_progress_set_text;
  iface->set_value     = ligma_display_progress_set_value;
  iface->get_value     = ligma_display_progress_get_value;
  iface->pulse         = ligma_display_progress_pulse;
  iface->get_window_id = ligma_display_progress_get_window_id;
  iface->message       = ligma_display_progress_message;
}

static void
ligma_display_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_display_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  LigmaDisplayImpl *display = LIGMA_DISPLAY_IMPL (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      g_value_set_object (value, display->priv->image);
      break;

    case PROP_SHELL:
      g_value_set_object (value, display->priv->shell);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
ligma_display_impl_present (LigmaDisplay *display)
{
  ligma_display_shell_present (ligma_display_get_shell (display));

  return TRUE;
}

static gboolean
ligma_display_impl_grab_focus (LigmaDisplay *display)
{
  LigmaDisplayImpl *display_impl = LIGMA_DISPLAY_IMPL (display);

  if (display_impl->priv->shell && ligma_display_get_image (display))
    {
      LigmaImageWindow *image_window;

      image_window = ligma_display_shell_get_window (ligma_display_get_shell (display));

      ligma_display_present (display);
      gtk_widget_grab_focus (ligma_window_get_primary_focus_widget (LIGMA_WINDOW (image_window)));

      return TRUE;
    }

  return FALSE;
}

static LigmaProgress *
ligma_display_progress_start (LigmaProgress *progress,
                             gboolean      cancellable,
                             const gchar  *message)
{
  LigmaDisplayImpl *display = LIGMA_DISPLAY_IMPL (progress);

  if (display->priv->shell)
    return ligma_progress_start (LIGMA_PROGRESS (display->priv->shell),
                                cancellable,
                                "%s", message);

  return NULL;
}

static void
ligma_display_progress_end (LigmaProgress *progress)
{
  LigmaDisplayImpl *display = LIGMA_DISPLAY_IMPL (progress);

  if (display->priv->shell)
    ligma_progress_end (LIGMA_PROGRESS (display->priv->shell));
}

static gboolean
ligma_display_progress_is_active (LigmaProgress *progress)
{
  LigmaDisplayImpl *display = LIGMA_DISPLAY_IMPL (progress);

  if (display->priv->shell)
    return ligma_progress_is_active (LIGMA_PROGRESS (display->priv->shell));

  return FALSE;
}

static void
ligma_display_progress_set_text (LigmaProgress *progress,
                                const gchar  *message)
{
  LigmaDisplayImpl *display = LIGMA_DISPLAY_IMPL (progress);

  if (display->priv->shell)
    ligma_progress_set_text_literal (LIGMA_PROGRESS (display->priv->shell),
                                    message);
}

static void
ligma_display_progress_set_value (LigmaProgress *progress,
                                 gdouble       percentage)
{
  LigmaDisplayImpl *display = LIGMA_DISPLAY_IMPL (progress);

  if (display->priv->shell)
    ligma_progress_set_value (LIGMA_PROGRESS (display->priv->shell),
                             percentage);
}

static gdouble
ligma_display_progress_get_value (LigmaProgress *progress)
{
  LigmaDisplayImpl *display = LIGMA_DISPLAY_IMPL (progress);

  if (display->priv->shell)
    return ligma_progress_get_value (LIGMA_PROGRESS (display->priv->shell));

  return 0.0;
}

static void
ligma_display_progress_pulse (LigmaProgress *progress)
{
  LigmaDisplayImpl *display = LIGMA_DISPLAY_IMPL (progress);

  if (display->priv->shell)
    ligma_progress_pulse (LIGMA_PROGRESS (display->priv->shell));
}

static guint32
ligma_display_progress_get_window_id (LigmaProgress *progress)
{
  LigmaDisplayImpl *display = LIGMA_DISPLAY_IMPL (progress);

  if (display->priv->shell)
    return ligma_progress_get_window_id (LIGMA_PROGRESS (display->priv->shell));

  return 0;
}

static gboolean
ligma_display_progress_message (LigmaProgress        *progress,
                               Ligma                *ligma,
                               LigmaMessageSeverity  severity,
                               const gchar         *domain,
                               const gchar         *message)
{
  LigmaDisplayImpl *display = LIGMA_DISPLAY_IMPL (progress);

  if (display->priv->shell)
    return ligma_progress_message (LIGMA_PROGRESS (display->priv->shell), ligma,
                                  severity, domain, message);

  return FALSE;
}

static void
ligma_display_progress_canceled (LigmaProgress *progress,
                                LigmaDisplay  *display)
{
  ligma_progress_cancel (LIGMA_PROGRESS (display));
}


/*  public functions  */

LigmaDisplay *
ligma_display_new (Ligma              *ligma,
                  LigmaImage         *image,
                  LigmaUnit           unit,
                  gdouble            scale,
                  LigmaUIManager     *popup_manager,
                  LigmaDialogFactory *dialog_factory,
                  GdkMonitor        *monitor)
{
  LigmaDisplay            *display;
  LigmaDisplayImplPrivate *private;
  LigmaImageWindow        *window = NULL;
  LigmaDisplayShell       *shell;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (image == NULL || LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GDK_IS_MONITOR (monitor), NULL);

  /*  If there isn't an interface, never create a display  */
  if (ligma->no_interface)
    return NULL;

  display = g_object_new (LIGMA_TYPE_DISPLAY_IMPL,
                          "ligma", ligma,
                          NULL);

  private = LIGMA_DISPLAY_IMPL (display)->priv;

  /*  refs the image  */
  if (image)
    ligma_display_set_image (display, image);

  /*  get an image window  */
  if (LIGMA_GUI_CONFIG (display->config)->single_window_mode)
    {
      LigmaDisplay *active_display;

      active_display = ligma_context_get_display (ligma_get_user_context (ligma));

      if (! active_display)
        {
          active_display =
            LIGMA_DISPLAY (ligma_container_get_first_child (ligma->displays));
        }

      if (active_display)
        {
          LigmaDisplayShell *shell = ligma_display_get_shell (active_display);

          window = ligma_display_shell_get_window (shell);
        }
    }

  if (! window)
    {
      window = ligma_image_window_new (ligma,
                                      private->image,
                                      dialog_factory,
                                      monitor);
    }

  /*  create the shell for the image  */
  private->shell = ligma_display_shell_new (display, unit, scale,
                                           popup_manager,
                                           monitor);

  shell = ligma_display_get_shell (display);

  ligma_display_update_bounding_box (display);

  ligma_image_window_add_shell (window, shell);
  ligma_display_shell_present (shell);

  /* make sure the docks are visible, in case all other image windows
   * are iconified, see bug #686544.
   */
  ligma_dialog_factory_show_with_display (dialog_factory);

  g_signal_connect (ligma_display_shell_get_statusbar (shell), "cancel",
                    G_CALLBACK (ligma_display_progress_canceled),
                    display);

  /* add the display to the list */
  ligma_container_add (ligma->displays, LIGMA_OBJECT (display));

  return display;
}

/**
 * ligma_display_delete:
 * @display:
 *
 * Closes the display and removes it from the display list. You should
 * not call this function directly, use ligma_display_close() instead.
 */
void
ligma_display_delete (LigmaDisplay *display)
{
  LigmaDisplayImplPrivate *private;
  LigmaTool               *active_tool;

  g_return_if_fail (LIGMA_IS_DISPLAY (display));

  private = LIGMA_DISPLAY_IMPL (display)->priv;

  /* remove the display from the list */
  ligma_container_remove (display->ligma->displays, LIGMA_OBJECT (display));

  /*  unrefs the image  */
  ligma_display_set_image (display, NULL);

  active_tool = tool_manager_get_active (display->ligma);

  if (active_tool && active_tool->focus_display == display)
    tool_manager_focus_display_active (display->ligma, NULL);

  if (private->shell)
    {
      LigmaDisplayShell *shell  = ligma_display_get_shell (display);
      LigmaImageWindow  *window = ligma_display_shell_get_window (shell);

      /*  set private->shell to NULL *before* destroying the shell.
       *  all callbacks in ligmadisplayshell-callbacks.c will check
       *  this pointer and do nothing if the shell is in destruction.
       */
      private->shell = NULL;

      if (window)
        {
          if (ligma_image_window_get_n_shells (window) > 1)
            {
              g_object_ref (shell);

              ligma_image_window_remove_shell (window, shell);
              gtk_widget_destroy (GTK_WIDGET (shell));

              g_object_unref (shell);
            }
          else
            {
              ligma_image_window_destroy (window);
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
 * ligma_display_close:
 * @display:
 *
 * Closes the display. If this is the last display, it will remain
 * open, but without an image.
 */
void
ligma_display_close (LigmaDisplay *display)
{
  g_return_if_fail (LIGMA_IS_DISPLAY (display));

  if (ligma_container_get_n_children (display->ligma->displays) > 1)
    {
      ligma_display_delete (display);
    }
  else
    {
      ligma_display_empty (display);
    }
}

/**
 * ligma_display_get_action_name:
 * @display:
 *
 * Returns: The action name for the given display. The action name
 * depends on the display ID. The result must be freed with g_free().
 **/
gchar *
ligma_display_get_action_name (LigmaDisplay *display)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY (display), NULL);

  return g_strdup_printf ("windows-display-%04d",
                          ligma_display_get_id (display));
}

LigmaImage *
ligma_display_get_image (LigmaDisplay *display)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY (display), NULL);

  return LIGMA_DISPLAY_IMPL (display)->priv->image;
}

void
ligma_display_set_image (LigmaDisplay *display,
                        LigmaImage   *image)
{
  LigmaDisplayImplPrivate *private;
  LigmaImage              *old_image = NULL;
  LigmaDisplayShell       *shell;

  g_return_if_fail (LIGMA_IS_DISPLAY (display));
  g_return_if_fail (image == NULL || LIGMA_IS_IMAGE (image));

  private = LIGMA_DISPLAY_IMPL (display)->priv;

  shell = ligma_display_get_shell (display);

  if (private->image)
    {
      /*  stop any active tool  */
      tool_manager_control_active (display->ligma, LIGMA_TOOL_ACTION_HALT,
                                   display);

      ligma_display_shell_disconnect (shell);

      ligma_display_disconnect (display);

      g_clear_pointer (&private->update_region, cairo_region_destroy);

      ligma_image_dec_display_count (private->image);

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

      private->instance = ligma_image_get_instance_count (image);
      ligma_image_inc_instance_count (image);

      ligma_image_inc_display_count (image);

      ligma_display_connect (display);

      if (shell)
        ligma_display_shell_connect (shell);
    }

  if (old_image)
    g_object_unref (old_image);

  ligma_display_update_bounding_box (display);

  if (shell)
    {
      if (image)
        {
          ligma_display_shell_reconnect (shell);
        }
      else
        {
          ligma_display_shell_title_update (shell);
        }
    }

  if (old_image != image)
    g_object_notify (G_OBJECT (display), "image");
}

gint
ligma_display_get_instance (LigmaDisplay *display)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY (display), 0);

  return LIGMA_DISPLAY_IMPL (display)->priv->instance;
}

LigmaDisplayShell *
ligma_display_get_shell (LigmaDisplay *display)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY (display), NULL);

  return LIGMA_DISPLAY_SHELL (LIGMA_DISPLAY_IMPL (display)->priv->shell);
}

void
ligma_display_empty (LigmaDisplay *display)
{
  LigmaDisplayImplPrivate *private;
  GList                 *iter;

  g_return_if_fail (LIGMA_IS_DISPLAY (display));

  private = LIGMA_DISPLAY_IMPL (display)->priv;

  g_return_if_fail (LIGMA_IS_IMAGE (private->image));

  for (iter = display->ligma->context_list; iter; iter = g_list_next (iter))
    {
      LigmaContext *context = iter->data;

      if (ligma_context_get_display (context) == display)
        ligma_context_set_image (context, NULL);
    }

  ligma_display_set_image (display, NULL);

  ligma_display_shell_empty (ligma_display_get_shell (display));
}

void
ligma_display_fill (LigmaDisplay *display,
                   LigmaImage   *image,
                   LigmaUnit     unit,
                   gdouble      scale)
{
  LigmaDisplayImplPrivate *private;

  g_return_if_fail (LIGMA_IS_DISPLAY (display));
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  private = LIGMA_DISPLAY_IMPL (display)->priv;

  g_return_if_fail (private->image == NULL);

  ligma_display_set_image (display, image);

  ligma_display_shell_fill (ligma_display_get_shell (display),
                           image, unit, scale);
}

void
ligma_display_update_bounding_box (LigmaDisplay *display)
{
  LigmaDisplayImplPrivate *private;
  LigmaDisplayShell       *shell;
  GeglRectangle           bounding_box = {};

  g_return_if_fail (LIGMA_IS_DISPLAY (display));

  private = LIGMA_DISPLAY_IMPL (display)->priv;
  shell   = ligma_display_get_shell (display);

  if (shell)
    {
      bounding_box = ligma_display_shell_get_bounding_box (shell);

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
              ligma_display_paint_area (display,
                                       diff_rects[i].x,     diff_rects[i].y,
                                       diff_rects[i].width, diff_rects[i].height);
            }

          private->bounding_box = bounding_box;

          ligma_display_shell_scroll_clamp_and_update (shell);
          ligma_display_shell_scrollbars_update (shell);
        }
    }
  else
    {
      private->bounding_box = bounding_box;
    }
}

void
ligma_display_update_area (LigmaDisplay *display,
                          gboolean     now,
                          gint         x,
                          gint         y,
                          gint         w,
                          gint         h)
{
  LigmaDisplayImplPrivate *private;

  g_return_if_fail (LIGMA_IS_DISPLAY (display));

  private = LIGMA_DISPLAY_IMPL (display)->priv;

  if (now)
    {
      ligma_display_paint_area (display, x, y, w, h);
    }
  else
    {
      cairo_rectangle_int_t rect;
      gint                  image_width;
      gint                  image_height;

      image_width  = ligma_image_get_width  (private->image);
      image_height = ligma_image_get_height (private->image);

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
ligma_display_flush (LigmaDisplay *display)
{
  g_return_if_fail (LIGMA_IS_DISPLAY (display));

  /* FIXME: we can end up being called during shell construction if "show all"
   * is enabled by default, in which case the shell's display pointer is still
   * NULL
   */
  if (ligma_display_get_shell (display))
    {
      ligma_display_flush_update_region (display);

      ligma_display_shell_flush (ligma_display_get_shell (display));
    }
}

void
ligma_display_flush_now (LigmaDisplay *display)
{
  g_return_if_fail (LIGMA_IS_DISPLAY (display));

  ligma_display_flush_update_region (display);
}


/*  private functions  */

static void
ligma_display_flush_update_region (LigmaDisplay *display)
{
  LigmaDisplayImplPrivate *private = LIGMA_DISPLAY_IMPL (display)->priv;

  if (private->update_region)
    {
      gint n_rects = cairo_region_num_rectangles (private->update_region);
      gint i;

      for (i = 0; i < n_rects; i++)
        {
          cairo_rectangle_int_t rect;

          cairo_region_get_rectangle (private->update_region,
                                      i, &rect);

          ligma_display_paint_area (display,
                                   rect.x,
                                   rect.y,
                                   rect.width,
                                   rect.height);
        }

      g_clear_pointer (&private->update_region, cairo_region_destroy);
    }
}

static void
ligma_display_paint_area (LigmaDisplay *display,
                         gint         x,
                         gint         y,
                         gint         w,
                         gint         h)
{
  LigmaDisplayImplPrivate *private = LIGMA_DISPLAY_IMPL (display)->priv;
  LigmaDisplayShell       *shell   = ligma_display_get_shell (display);
  GeglRectangle           rect;
  gint                    x1, y1, x2, y2;
  gdouble                 x1_f, y1_f, x2_f, y2_f;

  if (! gegl_rectangle_intersect (&rect,
                                  &private->bounding_box,
                                  GEGL_RECTANGLE (x, y, w, h)))
    {
      return;
    }

  /*  display the area  */
  ligma_display_shell_transform_bounds (shell,
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

  ligma_display_shell_expose_area (shell, x1, y1, x2 - x1, y2 - y1);

  ligma_display_shell_render_invalidate_area (shell, x1, y1, x2 - x1, y2 - y1);
}
