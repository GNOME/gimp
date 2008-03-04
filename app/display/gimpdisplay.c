/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "display-types.h"
#include "tools/tools-types.h"

#include "file/file-open.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimparea.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimplist.h"
#include "core/gimpprogress.h"

#include "dialogs/tips-parser.h"

/* FIXME */
#include "text/gimpfont-utils.h"
#include "text/gimptext.h"
#include "text/gimptext-compat.h"
#include "text/gimptextlayer.h"

#include "tools/gimptool.h"
#include "tools/tool_manager.h"

#include "gimpdisplay.h"
#include "gimpdisplay-handlers.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-appearance.h"
#include "gimpdisplayshell-handlers.h"
#include "gimpdisplayshell-scale.h"
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

static void     gimp_display_set_property        (GObject             *object,
                                                  guint                property_id,
                                                  const GValue        *value,
                                                  GParamSpec          *pspec);
static void     gimp_display_get_property        (GObject             *object,
                                                  guint                property_id,
                                                  GValue              *value,
                                                  GParamSpec          *pspec);

static GimpProgress *
                gimp_display_progress_start      (GimpProgress        *progress,
                                                  const gchar         *message,
                                                  gboolean             cancelable);
static void     gimp_display_progress_end        (GimpProgress        *progress);
static gboolean gimp_display_progress_is_active  (GimpProgress        *progress);
static void     gimp_display_progress_set_text   (GimpProgress        *progress,
                                                  const gchar         *message);
static void     gimp_display_progress_set_value  (GimpProgress        *progress,
                                                  gdouble              percentage);
static gdouble  gimp_display_progress_get_value  (GimpProgress        *progress);
static void     gimp_display_progress_pulse      (GimpProgress        *progress);
static guint32  gimp_display_progress_get_window (GimpProgress        *progress);
static gboolean gimp_display_progress_message    (GimpProgress        *progress,
                                                  Gimp                *gimp,
                                                  GimpMessageSeverity  severity,
                                                  const gchar         *domain,
                                                  const gchar         *message);
static void     gimp_display_progress_canceled   (GimpProgress        *progress,
                                                  GimpDisplay         *display);

static void     gimp_display_flush_whenever      (GimpDisplay         *display,
                                                  gboolean             now);
static void     gimp_display_paint_area          (GimpDisplay         *display,
                                                  gint                 x,
                                                  gint                 y,
                                                  gint                 w,
                                                  gint                 h);

static void     text_show_tip                    (GimpImage           *image,
                                                  GimpContext         *context);


G_DEFINE_TYPE_WITH_CODE (GimpDisplay, gimp_display, GIMP_TYPE_OBJECT,
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
  display->ID           = 0;

  display->image        = NULL;
  display->instance     = 0;

  display->shell        = NULL;

  display->update_areas = NULL;
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
  iface->message    = gimp_display_progress_message;
}

static void
gimp_display_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  GimpDisplay *display = GIMP_DISPLAY (object);

  switch (property_id)
    {
    case PROP_ID:
      display->ID = g_value_get_int (value);
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
  GimpDisplay *display = GIMP_DISPLAY (object);

  switch (property_id)
    {
    case PROP_ID:
      g_value_set_int (value, display->ID);
      break;
    case PROP_IMAGE:
      g_value_set_object (value, display->image);
      break;
    case PROP_SHELL:
      g_value_set_object (value, display->shell);
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
  GimpDisplay *display = GIMP_DISPLAY (progress);

  if (display->shell)
    return gimp_progress_start (GIMP_PROGRESS (display->shell),
                                message, cancelable);

  return NULL;
}

static void
gimp_display_progress_end (GimpProgress *progress)
{
  GimpDisplay *display = GIMP_DISPLAY (progress);

  if (display->shell)
    gimp_progress_end (GIMP_PROGRESS (display->shell));
}

static gboolean
gimp_display_progress_is_active (GimpProgress *progress)
{
  GimpDisplay *display = GIMP_DISPLAY (progress);

  if (display->shell)
    return gimp_progress_is_active (GIMP_PROGRESS (display->shell));

  return FALSE;
}

static void
gimp_display_progress_set_text (GimpProgress *progress,
                                const gchar  *message)
{
  GimpDisplay *display = GIMP_DISPLAY (progress);

  if (display->shell)
    gimp_progress_set_text (GIMP_PROGRESS (display->shell), message);
}

static void
gimp_display_progress_set_value (GimpProgress *progress,
                                 gdouble       percentage)
{
  GimpDisplay *display = GIMP_DISPLAY (progress);

  if (display->shell)
    gimp_progress_set_value (GIMP_PROGRESS (display->shell), percentage);
}

static gdouble
gimp_display_progress_get_value (GimpProgress *progress)
{
  GimpDisplay *display = GIMP_DISPLAY (progress);

  if (display->shell)
    return gimp_progress_get_value (GIMP_PROGRESS (display->shell));

  return 0.0;
}

static void
gimp_display_progress_pulse (GimpProgress *progress)
{
  GimpDisplay *display = GIMP_DISPLAY (progress);

  if (display->shell)
    gimp_progress_pulse (GIMP_PROGRESS (display->shell));
}

static guint32
gimp_display_progress_get_window (GimpProgress *progress)
{
  GimpDisplay *display = GIMP_DISPLAY (progress);

  if (display->shell)
    return gimp_progress_get_window (GIMP_PROGRESS (display->shell));

  return 0;
}

static gboolean
gimp_display_progress_message (GimpProgress        *progress,
                               Gimp                *gimp,
                               GimpMessageSeverity  severity,
                               const gchar         *domain,
                               const gchar         *message)
{
  GimpDisplay *display = GIMP_DISPLAY (progress);

  if (display->shell)
    return gimp_progress_message (GIMP_PROGRESS (display->shell), gimp,
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
gimp_display_new (GimpImage       *image,
                  GimpUnit         unit,
                  gdouble          scale,
                  GimpMenuFactory *menu_factory,
                  GimpUIManager   *popup_manager)
{
  GimpDisplay *display;
  Gimp        *gimp;
  gint         ID;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  gimp = image->gimp;

  /*  If there isn't an interface, never create a display  */
  if (gimp->no_interface)
    return NULL;

  do
    {
      ID = gimp->next_display_ID++;

      if (gimp->next_display_ID == G_MAXINT)
        gimp->next_display_ID = 1;
    }
  while (gimp_display_get_by_ID (gimp, ID));

  display = g_object_new (GIMP_TYPE_DISPLAY,
                          "id", ID,
                          NULL);

  /*  refs the image  */
  gimp_display_connect (display, image);

  /*  create the shell for the image  */
  display->shell = gimp_display_shell_new (display, unit, scale,
                                           menu_factory, popup_manager);
  gtk_widget_show (display->shell);

  g_signal_connect (GIMP_DISPLAY_SHELL (display->shell)->statusbar, "cancel",
                    G_CALLBACK (gimp_display_progress_canceled),
                    display);

  /* add the display to the list */
  gimp_container_add (gimp->displays, GIMP_OBJECT (display));

  /* if there is a scratch image display, we need to get rid of it,
   * unless of course it is one that we have just created here.
   */
  if (gimp->scratch_image)
    {
      GList *list;

      for (list = GIMP_LIST (gimp->displays)->list; list; list = g_list_next (list))
        {
          GimpDisplay *d = GIMP_DISPLAY (list->data);

          if (gimp_image_is_scratch (d->image) && d != display)
            {
              gimp_display_delete (d);
              gimp->scratch_image = NULL;
              break;
            }
        }
    }

  return display;
}

GimpDisplay *
gimp_scratch_display_new (GimpImage       *image,
                          GimpUnit         unit,
                          gdouble          scale,
                          GimpMenuFactory *menu_factory,
                          GimpUIManager   *popup_manager)
{
  GimpDisplay      *display;
  Gimp             *gimp;
  gint              ID;
  GimpDisplayShell *shell;
  gint              width, height;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  gimp = image->gimp;

  /*  If there isn't an interface, never create a display  */
  if (gimp->no_interface)
    return NULL;

  do
    {
      ID = gimp->next_display_ID++;

      if (gimp->next_display_ID == G_MAXINT)
        gimp->next_display_ID = 1;
    }
  while (gimp_display_get_by_ID (gimp, ID));

  display = g_object_new (GIMP_TYPE_DISPLAY,
                          "id", ID,
                          NULL);

  /*  refs the image  */
  gimp_display_connect (display, image);

  /*  create the shell for the image  */
  display->shell = gimp_display_shell_new (display, unit, scale,
                                           menu_factory, popup_manager);
  shell = GIMP_DISPLAY_SHELL (display->shell);

  gimp_display_shell_set_show_layer      (shell, FALSE);
  gimp_display_shell_set_show_rulers     (shell, FALSE);
  gimp_display_shell_set_show_scrollbars (shell, FALSE);
  gimp_display_shell_set_show_statusbar  (shell, FALSE);

  width  = SCALEX (shell, gimp_image_get_width  (image));
  height = SCALEY (shell, gimp_image_get_height (image));
  gtk_widget_set_size_request (display->shell, width, height);

  gtk_widget_show (display->shell);

  g_signal_connect (GIMP_DISPLAY_SHELL (display->shell)->statusbar, "cancel",
                    G_CALLBACK (gimp_display_progress_canceled),
                    display);

  /* add the display to the list */
  gimp_container_add (gimp->displays, GIMP_OBJECT (display));

  /*FIXME: hack alert */
  if (GIMP_GUI_CONFIG (gimp->config)->show_tips)
    text_show_tip (image, gimp_get_user_context (gimp));

  return display;
}

/*
 * this is a hack, this function should be replaced by something
 * better and go somewhere else
 */
static void
text_show_tip (GimpImage   *image,
               GimpContext *context)
{
  gchar            *filename;
  GList            *tips;
  GError           *error  = NULL;
  Gimp             *gimp   = image->gimp;

  filename = g_build_filename (gimp_data_directory (), "tips",
                               "gimp-tips.xml", NULL);

  tips = gimp_tips_from_file (filename, &error);
  g_free (filename);

  if (tips)
    {
      PangoFontDescription *desc;

      gint           tips_count = g_list_length (tips);
      GimpGuiConfig *config     = GIMP_GUI_CONFIG (gimp->config);
      GList         *current_tip;
      GimpText      *text;
      GimpTip       *tip;
      gchar         *tip_text;
      GimpLayer     *layer;
      gdouble        size;
      GimpRGB        color = {0, 0.2, 0.4, 1.0};
      gchar         *font;
      gint           margin     = 30;
      gdouble        box_height = 200;
      gdouble        box_width;
      gint           layer_height;

      if (config->last_tip >= tips_count || config->last_tip < 0)
        config->last_tip = 0;

      current_tip = g_list_nth (tips, config->last_tip);

      tip = current_tip->data;
      tip_text = g_strconcat ("<span size=\"x-large\"><b>", _("TIP:"), "</b></span> ",
                              tip->thetip, NULL);

      /* the last-shown-tip is saved in sessionrc */
      config->last_tip = g_list_position (tips, current_tip);

      desc = pango_font_description_from_string ("sans 12");
      size = PANGO_PIXELS (pango_font_description_get_size (desc));

      pango_font_description_unset_fields (desc, PANGO_FONT_MASK_SIZE);
      font = gimp_font_util_pango_font_description_to_string (desc);

      pango_font_description_free (desc);

      box_width = gimp_image_get_width (image) - 2 * margin;

      /* FIXME: should calculate appropriate box height */
      text = g_object_new (GIMP_TYPE_TEXT,
                           "text",       tip_text,
                           "font",       font,
                           "font-size",  size,
                           "color",      &color,
                           "box-mode",   GIMP_TEXT_BOX_FIXED,
                           "box-width",  box_width,
                           "box-height", box_height,
                           NULL);

      g_free (font);
      g_free (tip_text);

      layer = gimp_text_layer_new (image, text);

      g_object_unref (text);

      layer_height = GIMP_ITEM (layer)->height;

      GIMP_ITEM (layer)->offset_x = margin;
      GIMP_ITEM (layer)->offset_y = margin;
/*       GIMP_ITEM (layer)->offset_y = (gimp_image_get_height (image) - layer_height) / 2; */

      gimp_image_add_layer (image, layer, -1);
      gimp_image_flatten (image, context);
    }
}


void
gimp_display_delete (GimpDisplay *display)
{
  GimpTool *active_tool;
  Gimp     *gimp;

  g_return_if_fail (GIMP_IS_DISPLAY (display));

  gimp = display->image->gimp;

  /* remove the display from the list */
  gimp_container_remove (gimp->displays,
                         GIMP_OBJECT (display));

  /*  stop any active tool  */
  tool_manager_control_active (gimp, GIMP_TOOL_ACTION_HALT, display);

  active_tool = tool_manager_get_active (gimp);

  if (active_tool && active_tool->focus_display == display)
    tool_manager_focus_display_active (gimp, NULL);

  /*  free the update area lists  */
  gimp_area_list_free (display->update_areas);
  display->update_areas = NULL;

  if (display->shell)
    {
      GtkWidget *shell = display->shell;

      /*  set display->shell to NULL *before* destroying the shell.
       *  all callbacks in gimpdisplayshell-callbacks.c will check
       *  this pointer and do nothing if the shell is in destruction.
       */
      display->shell = NULL;
      gtk_widget_destroy (shell);
    }

  /*  unrefs the image  */
  gimp_display_disconnect (display);

  g_object_unref (display);

  /* make a new scratch image if we need one */
  if (! gimp->exiting && gimp_container_is_empty (gimp->displays))
      file_create_scratch_image (gimp);
}

gint
gimp_display_get_ID (GimpDisplay *display)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), -1);

  return display->ID;
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
      GimpDisplay *display = list->data;

      if (display->ID == ID)
        return display;
    }

  return NULL;
}

void
gimp_display_reconnect (GimpDisplay *display,
                        GimpImage   *image)
{
  GimpImage *old_image;

  g_return_if_fail (GIMP_IS_DISPLAY (display));
  g_return_if_fail (GIMP_IS_IMAGE (image));

  /*  stop any active tool  */
  tool_manager_control_active (display->image->gimp, GIMP_TOOL_ACTION_HALT,
                               display);

  gimp_display_shell_disconnect (GIMP_DISPLAY_SHELL (display->shell));

  old_image = g_object_ref (display->image);

  gimp_display_disconnect (display);
  gimp_display_connect (display, image);

  if (image->gimp->scratch_image == old_image)
    {
      GimpDisplayShell *shell     = GIMP_DISPLAY_SHELL (display->shell);
      gint              n_width;
      gint              n_height;

      gimp_display_shell_set_initial_scale (shell, &n_width, &n_height);
      gtk_widget_set_size_request (shell->canvas, n_width, n_height);
      gimp_display_shell_configure (shell);
      gimp_display_shell_scale_resize (shell, TRUE, TRUE);
    }

  g_object_unref (old_image);

  gimp_display_shell_reconnect (GIMP_DISPLAY_SHELL (display->shell));
}

void
gimp_display_update_area (GimpDisplay *display,
                          gboolean     now,
                          gint         x,
                          gint         y,
                          gint         w,
                          gint         h)
{
  g_return_if_fail (GIMP_IS_DISPLAY (display));

  if (now)
    {
      gimp_display_paint_area (display, x, y, w, h);
    }
  else
    {
      GimpArea *area;
      gint      image_width  = gimp_image_get_width  (display->image);
      gint      image_height = gimp_image_get_height (display->image);

      area = gimp_area_new (CLAMP (x,     0, image_width),
                            CLAMP (y,     0, image_height),
                            CLAMP (x + w, 0, image_width),
                            CLAMP (y + h, 0, image_height));

      display->update_areas = gimp_area_list_process (display->update_areas,
                                                      area);
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
  if (display->update_areas)
    {
      GSList *list;

      for (list = display->update_areas; list; list = g_slist_next (list))
        {
          GimpArea *area = list->data;

          if ((area->x1 != area->x2) && (area->y1 != area->y2))
            {
              gimp_display_paint_area (display,
                                       area->x1,
                                       area->y1,
                                       (area->x2 - area->x1),
                                       (area->y2 - area->y1));
            }
        }

      gimp_area_list_free (display->update_areas);
      display->update_areas = NULL;
    }

  gimp_display_shell_flush (GIMP_DISPLAY_SHELL (display->shell), now);
}

static void
gimp_display_paint_area (GimpDisplay *display,
                         gint         x,
                         gint         y,
                         gint         w,
                         gint         h)
{
  GimpDisplayShell *shell        = GIMP_DISPLAY_SHELL (display->shell);
  gint              image_width  = gimp_image_get_width  (display->image);
  gint              image_height = gimp_image_get_height (display->image);
  gint              x1, y1, x2, y2;
  gdouble           x1_f, y1_f, x2_f, y2_f;

  /*  Bounds check  */
  x1 = CLAMP (x,     0, image_width);
  y1 = CLAMP (y,     0, image_height);
  x2 = CLAMP (x + w, 0, image_width);
  y2 = CLAMP (y + h, 0, image_height);

  x = x1;
  y = y1;
  w = (x2 - x1);
  h = (y2 - y1);

  /*  display the area  */
  gimp_display_shell_transform_xy_f (shell, x,     y,     &x1_f, &y1_f, FALSE);
  gimp_display_shell_transform_xy_f (shell, x + w, y + h, &x2_f, &y2_f, FALSE);

  /*  make sure to expose a superset of the transformed sub-pixel expose
   *  area, not a subset. bug #126942. --mitch
   *
   *  also acommodate for spill introduced by potential box filtering.
   *  (bug #474509). --simon
   */
  x1 = floor (x1_f - 0.5);
  y1 = floor (y1_f - 0.5);
  x2 = ceil (x2_f + 0.5);
  y2 = ceil (y2_f + 0.5);

  gimp_display_shell_expose_area (shell, x1, y1, x2 - x1, y2 - y1);
}
