/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "display-types.h"
#include "tools/tools-types.h"

#include "core/gimp.h"
#include "core/gimparea.h"
#include "core/gimpimage.h"
#include "core/gimplist.h"
#include "core/gimpprogress.h"

#include "widgets/gimpwidgets-utils.h"

#include "tools/gimptool.h"
#include "tools/tool_manager.h"

#include "gimpdisplay.h"
#include "gimpdisplay-handlers.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-handlers.h"
#include "gimpdisplayshell-transform.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_ID,
  PROP_IMAGE,
  PROP_SHELL
};


/*  local function prototypes  */

static void     gimp_display_progress_iface_init (GimpProgressInterface *iface);

static void     gimp_display_set_property        (GObject       *object,
                                                  guint          property_id,
                                                  const GValue  *value,
                                                  GParamSpec    *pspec);
static void     gimp_display_get_property        (GObject       *object,
                                                  guint          property_id,
                                                  GValue        *value,
                                                  GParamSpec    *pspec);

static GimpProgress *
                gimp_display_progress_start      (GimpProgress  *progress,
                                                  const gchar   *message,
                                                  gboolean       cancelable);
static void     gimp_display_progress_end        (GimpProgress  *progress);
static gboolean gimp_display_progress_is_active  (GimpProgress  *progress);
static void     gimp_display_progress_set_text   (GimpProgress  *progress,
                                                  const gchar   *message);
static void     gimp_display_progress_set_value  (GimpProgress  *progress,
                                                  gdouble        percentage);
static gdouble  gimp_display_progress_get_value  (GimpProgress  *progress);
static void     gimp_display_progress_pulse      (GimpProgress  *progress);
static guint32  gimp_display_progress_get_window (GimpProgress  *progress);
static void     gimp_display_progress_canceled   (GimpProgress  *progress,
                                                  GimpDisplay   *display);

static void     gimp_display_flush_whenever      (GimpDisplay   *gdisp,
                                                  gboolean       now);
static void     gimp_display_paint_area          (GimpDisplay   *gdisp,
                                                  gint           x,
                                                  gint           y,
                                                  gint           w,
                                                  gint           h);


G_DEFINE_TYPE_WITH_CODE (GimpDisplay, gimp_display, GIMP_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PROGRESS,
                                                gimp_display_progress_iface_init));

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
gimp_display_init (GimpDisplay *gdisp)
{
  gdisp->ID           = 0;

  gdisp->gimage       = NULL;
  gdisp->instance     = 0;

  gdisp->shell        = NULL;

  gdisp->update_areas = NULL;
}

static void
gimp_display_progress_iface_init (GimpProgressInterface *iface)
{
  iface->start      = gimp_display_progress_start;
  iface->end        = gimp_display_progress_end;
  iface->is_active  = gimp_display_progress_is_active;
  iface->set_text   = gimp_display_progress_set_text;
  iface->set_value  = gimp_display_progress_set_value;
  iface->get_value  = gimp_display_progress_get_value;
  iface->pulse      = gimp_display_progress_pulse;
  iface->get_window = gimp_display_progress_get_window;
}

static void
gimp_display_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  GimpDisplay *gdisp = GIMP_DISPLAY (object);

  switch (property_id)
    {
    case PROP_ID:
      gdisp->ID = g_value_get_int (value);
      break;
    case PROP_IMAGE:
    case PROP_SHELL:
      g_assert_not_reached ();
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
  GimpDisplay *gdisp = GIMP_DISPLAY (object);

  switch (property_id)
    {
    case PROP_ID:
      g_value_set_int (value, gdisp->ID);
      break;
    case PROP_IMAGE:
      g_value_set_object (value, gdisp->gimage);
      break;
    case PROP_SHELL:
      g_value_set_object (value, gdisp->shell);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GimpProgress *
gimp_display_progress_start (GimpProgress *progress,
                             const gchar  *message,
                             gboolean      cancelable)
{
  GimpDisplay      *display = GIMP_DISPLAY (progress);
  GimpDisplayShell *shell;

  if (! display->shell)
    return NULL;

  shell = GIMP_DISPLAY_SHELL (display->shell);

  return gimp_progress_start (GIMP_PROGRESS (shell->statusbar),
                              message, cancelable);
}

static void
gimp_display_progress_end (GimpProgress *progress)
{
  GimpDisplay      *display = GIMP_DISPLAY (progress);
  GimpDisplayShell *shell;

  if (! display->shell)
    return;

  shell = GIMP_DISPLAY_SHELL (display->shell);

  gimp_progress_end (GIMP_PROGRESS (shell->statusbar));
}

static gboolean
gimp_display_progress_is_active (GimpProgress *progress)
{
  GimpDisplay      *display = GIMP_DISPLAY (progress);
  GimpDisplayShell *shell;

  if (! display->shell)
    return FALSE;

  shell = GIMP_DISPLAY_SHELL (display->shell);

  return gimp_progress_is_active (GIMP_PROGRESS (shell->statusbar));
}

static void
gimp_display_progress_set_text (GimpProgress *progress,
                                const gchar  *message)
{
  GimpDisplay      *display = GIMP_DISPLAY (progress);
  GimpDisplayShell *shell;

  if (! display->shell)
    return;

  shell = GIMP_DISPLAY_SHELL (display->shell);

  gimp_progress_set_text (GIMP_PROGRESS (shell->statusbar), message);
}

static void
gimp_display_progress_set_value (GimpProgress *progress,
                                 gdouble       percentage)
{
  GimpDisplay      *display = GIMP_DISPLAY (progress);
  GimpDisplayShell *shell;

  if (! display->shell)
    return;

  shell = GIMP_DISPLAY_SHELL (display->shell);

  gimp_progress_set_value (GIMP_PROGRESS (shell->statusbar), percentage);
}

static gdouble
gimp_display_progress_get_value (GimpProgress *progress)
{
  GimpDisplay      *display = GIMP_DISPLAY (progress);
  GimpDisplayShell *shell;

  if (! display->shell)
    return 0.0;

  shell = GIMP_DISPLAY_SHELL (display->shell);

  return gimp_progress_get_value (GIMP_PROGRESS (shell->statusbar));
}

static void
gimp_display_progress_pulse (GimpProgress *progress)
{
  GimpDisplay      *display = GIMP_DISPLAY (progress);
  GimpDisplayShell *shell;

  if (! display->shell)
    return;

  shell = GIMP_DISPLAY_SHELL (display->shell);

  gimp_progress_pulse (GIMP_PROGRESS (shell->statusbar));
}

static guint32
gimp_display_progress_get_window (GimpProgress *progress)
{
  GimpDisplay *display = GIMP_DISPLAY (progress);

  if (! display->shell)
    return 0;

  return (guint32) gimp_window_get_native (GTK_WINDOW (display->shell));
}

static void
gimp_display_progress_canceled (GimpProgress *progress,
                                GimpDisplay  *display)
{
  gimp_progress_cancel (GIMP_PROGRESS (display));
}


/*  public functions  */

GimpDisplay *
gimp_display_new (GimpImage       *gimage,
                  GimpUnit         unit,
                  gdouble          scale,
                  GimpMenuFactory *menu_factory,
                  GimpUIManager   *popup_manager)
{
  GimpDisplay *gdisp;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  /*  If there isn't an interface, never create a gdisplay  */
  if (gimage->gimp->no_interface)
    return NULL;

  gdisp = g_object_new (GIMP_TYPE_DISPLAY,
                        "id", gimage->gimp->next_display_ID++,
                        NULL);

  /*  refs the image  */
  gimp_display_connect (gdisp, gimage);

  /*  create the shell for the image  */
  gdisp->shell = gimp_display_shell_new (gdisp, unit, scale,
                                         menu_factory, popup_manager);
  gtk_widget_show (gdisp->shell);

  g_signal_connect (GIMP_DISPLAY_SHELL (gdisp->shell)->statusbar, "cancel",
                    G_CALLBACK (gimp_display_progress_canceled),
                    gdisp);

  return gdisp;
}

void
gimp_display_delete (GimpDisplay *gdisp)
{
  GimpTool *active_tool;

  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));

  /* remove the display from the list */
  gimp_container_remove (gdisp->gimage->gimp->displays,
                         GIMP_OBJECT (gdisp));

  /*  stop any active tool  */
  tool_manager_control_active (gdisp->gimage->gimp, HALT, gdisp);

  active_tool = tool_manager_get_active (gdisp->gimage->gimp);

  if (active_tool)
    {
      if (active_tool->focus_display == gdisp)
        tool_manager_focus_display_active (gdisp->gimage->gimp, NULL);

      /*  clear out the pointer to this gdisp from the active tool  */
      if (active_tool->gdisp == gdisp)
        {
          active_tool->drawable = NULL;
          active_tool->gdisp    = NULL;
        }
    }

  /*  free the update area lists  */
  gdisp->update_areas = gimp_area_list_free (gdisp->update_areas);

  if (gdisp->shell)
    {
      GtkWidget *shell = gdisp->shell;

      /*  set gdisp->shell to NULL *before* destroying the shell.
       *  all callbacks in gimpdisplayshell-callbacks.c will check
       *  this pointer and do nothing if the shell is in destruction.
       */
      gdisp->shell = NULL;
      gtk_widget_destroy (shell);
    }

  /*  unrefs the gimage  */
  gimp_display_disconnect (gdisp);

  g_object_unref (gdisp);
}

gint
gimp_display_get_ID (GimpDisplay *gdisp)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY (gdisp), -1);

  return gdisp->ID;
}

GimpDisplay *
gimp_display_get_by_ID (Gimp *gimp,
                        gint  ID)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  for (list = GIMP_LIST (gimp->displays)->list;
       list;
       list = g_list_next (list))
    {
      GimpDisplay *gdisp = list->data;

      if (gdisp->ID == ID)
        return gdisp;
    }

  return NULL;
}

void
gimp_display_reconnect (GimpDisplay *gdisp,
                        GimpImage   *gimage)
{
  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  /*  stop any active tool  */
  tool_manager_control_active (gdisp->gimage->gimp, HALT, gdisp);

  gimp_display_shell_disconnect (GIMP_DISPLAY_SHELL (gdisp->shell));

  gimp_display_disconnect (gdisp);

  gimp_display_connect (gdisp, gimage);

  gimp_display_shell_reconnect (GIMP_DISPLAY_SHELL (gdisp->shell));
}

void
gimp_display_update_area (GimpDisplay *gdisp,
                          gboolean     now,
                          gint         x,
                          gint         y,
                          gint         w,
                          gint         h)
{
  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));

  if (now)
    {
      gimp_display_paint_area (gdisp, x, y, w, h);
    }
  else
    {
      GimpArea *area = gimp_area_new (CLAMP (x, 0, gdisp->gimage->width),
                                      CLAMP (y, 0, gdisp->gimage->height),
                                      CLAMP (x + w, 0, gdisp->gimage->width),
                                      CLAMP (y + h, 0, gdisp->gimage->height));

      gdisp->update_areas = gimp_area_list_process (gdisp->update_areas, area);
    }
}

void
gimp_display_flush (GimpDisplay *gdisp)
{
  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));

  gimp_display_flush_whenever (gdisp, FALSE);
}

void
gimp_display_flush_now (GimpDisplay *gdisp)
{
  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));

  gimp_display_flush_whenever (gdisp, TRUE);
}


/*  private functions  */

static void
gimp_display_flush_whenever (GimpDisplay *gdisp,
                             gboolean     now)
{
  if (gdisp->update_areas)
    {
      GSList *list;

      for (list = gdisp->update_areas; list; list = g_slist_next (list))
        {
          GimpArea *area = list->data;

          if ((area->x1 != area->x2) && (area->y1 != area->y2))
            {
              gimp_display_paint_area (gdisp,
                                       area->x1,
                                       area->y1,
                                       (area->x2 - area->x1),
                                       (area->y2 - area->y1));
            }
        }

      gdisp->update_areas = gimp_area_list_free (gdisp->update_areas);
    }

  gimp_display_shell_flush (GIMP_DISPLAY_SHELL (gdisp->shell), now);
}

static void
gimp_display_paint_area (GimpDisplay *gdisp,
                         gint         x,
                         gint         y,
                         gint         w,
                         gint         h)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (gdisp->shell);
  gint              x1, y1, x2, y2;
  gdouble           x1_f, y1_f, x2_f, y2_f;

  /*  Bounds check  */
  x1 = CLAMP (x,     0, gdisp->gimage->width);
  y1 = CLAMP (y,     0, gdisp->gimage->height);
  x2 = CLAMP (x + w, 0, gdisp->gimage->width);
  y2 = CLAMP (y + h, 0, gdisp->gimage->height);
  x = x1;
  y = y1;
  w = (x2 - x1);
  h = (y2 - y1);

  /*  display the area  */
  gimp_display_shell_transform_xy_f (shell, x, y,         &x1_f, &y1_f, FALSE);
  gimp_display_shell_transform_xy_f (shell, x + w, y + h, &x2_f, &y2_f, FALSE);

  /*  make sure to expose a superset of the transformed sub-pixel expose
   *  area, not a subset. bug #126942. --mitch
   */
  x1 = floor (x1_f);
  y1 = floor (y1_f);
  x2 = ceil (x2_f);
  y2 = ceil (y2_f);

  gimp_display_shell_expose_area (shell, x1, y1, x2 - x1, y2 - y1);
}
