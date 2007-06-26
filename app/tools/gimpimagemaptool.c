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

#include <errno.h>

#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-pick-color.h"
#include "core/gimpimagemap.h"
#include "core/gimppickable.h"
#include "core/gimpprogress.h"
#include "core/gimpprojection.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimptooldialog.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"

#include "gimpcoloroptions.h"
#include "gimpimagemaptool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void     gimp_image_map_tool_finalize   (GObject          *object);

static gboolean gimp_image_map_tool_initialize (GimpTool         *tool,
                                                GimpDisplay      *display,
                                                GError          **error);
static void     gimp_image_map_tool_control    (GimpTool         *tool,
                                                GimpToolAction    action,
                                                GimpDisplay      *display);
static gboolean gimp_image_map_tool_key_press  (GimpTool         *tool,
                                                GdkEventKey      *kevent,
                                                GimpDisplay      *display);

static gboolean gimp_image_map_tool_pick_color (GimpColorTool    *color_tool,
                                                gint              x,
                                                gint              y,
                                                GimpImageType    *sample_type,
                                                GimpRGB          *color,
                                                gint             *color_index);
static void     gimp_image_map_tool_map        (GimpImageMapTool *im_tool);
static void     gimp_image_map_tool_dialog     (GimpImageMapTool *im_tool);
static void     gimp_image_map_tool_reset      (GimpImageMapTool *im_tool);

static void     gimp_image_map_tool_flush      (GimpImageMap     *image_map,
                                                GimpImageMapTool *im_tool);

static void     gimp_image_map_tool_response   (GtkWidget        *widget,
                                                gint              response_id,
                                                GimpImageMapTool *im_tool);

static void     gimp_image_map_tool_load_clicked     (GtkWidget        *widget,
                                                      GimpImageMapTool *tool);
static void     gimp_image_map_tool_load_ext_clicked (GtkWidget        *widget,
                                                      GdkModifierType   state,
                                                      GimpImageMapTool *tool);
static void     gimp_image_map_tool_save_clicked     (GtkWidget        *widget,
                                                      GimpImageMapTool *tool);
static void     gimp_image_map_tool_save_ext_clicked (GtkWidget        *widget,
                                                      GdkModifierType   state,
                                                      GimpImageMapTool *tool);

static void     gimp_image_map_tool_settings_dialog  (GimpImageMapTool *im_tool,
                                                      const gchar      *title,
                                                      gboolean          save);
static void     gimp_image_map_tool_notify_preview   (GObject          *config,
                                                      GParamSpec       *pspec,
                                                      GimpImageMapTool *im_tool);


G_DEFINE_TYPE (GimpImageMapTool, gimp_image_map_tool, GIMP_TYPE_COLOR_TOOL)

#define parent_class gimp_image_map_tool_parent_class


static void
gimp_image_map_tool_class_init (GimpImageMapToolClass *klass)
{
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  GimpToolClass      *tool_class       = GIMP_TOOL_CLASS (klass);
  GimpColorToolClass *color_tool_class = GIMP_COLOR_TOOL_CLASS (klass);

  object_class->finalize   = gimp_image_map_tool_finalize;

  tool_class->initialize   = gimp_image_map_tool_initialize;
  tool_class->control      = gimp_image_map_tool_control;
  tool_class->key_press    = gimp_image_map_tool_key_press;

  color_tool_class->pick   = gimp_image_map_tool_pick_color;

  klass->shell_desc        = NULL;
  klass->settings_name     = NULL;
  klass->load_dialog_title = NULL;
  klass->load_button_tip   = NULL;
  klass->save_dialog_title = NULL;
  klass->save_button_tip   = NULL;

  klass->map               = NULL;
  klass->dialog            = NULL;
  klass->reset             = NULL;
  klass->settings_load     = NULL;
  klass->settings_save     = NULL;
}

static void
gimp_image_map_tool_init (GimpImageMapTool *image_map_tool)
{
  GimpTool *tool = GIMP_TOOL (image_map_tool);

  gimp_tool_control_set_scroll_lock (tool->control, TRUE);
  gimp_tool_control_set_preserve    (tool->control, FALSE);
  gimp_tool_control_set_dirty_mask  (tool->control,
                                     GIMP_DIRTY_IMAGE           |
                                     GIMP_DIRTY_IMAGE_STRUCTURE |
                                     GIMP_DIRTY_DRAWABLE        |
                                     GIMP_DIRTY_SELECTION);

  image_map_tool->drawable  = NULL;
  image_map_tool->image_map = NULL;

  image_map_tool->shell       = NULL;
  image_map_tool->main_vbox   = NULL;
  image_map_tool->load_button = NULL;
  image_map_tool->save_button = NULL;
}

static void
gimp_image_map_tool_finalize (GObject *object)
{
  GimpImageMapTool *image_map_tool = GIMP_IMAGE_MAP_TOOL (object);

  if (image_map_tool->shell)
    {
      gtk_widget_destroy (image_map_tool->shell);
      image_map_tool->shell       = NULL;
      image_map_tool->main_vbox   = NULL;
      image_map_tool->load_button = NULL;
      image_map_tool->save_button = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

#define RESPONSE_RESET 1

static gboolean
gimp_image_map_tool_initialize (GimpTool     *tool,
                                GimpDisplay  *display,
                                GError      **error)
{
  GimpImageMapTool *image_map_tool = GIMP_IMAGE_MAP_TOOL (tool);
  GimpToolInfo     *tool_info      = tool->tool_info;
  GimpDrawable     *drawable;

  /*  set display so the dialog can be hidden on display destruction  */
  tool->display = display;

  if (! image_map_tool->shell)
    {
      GimpImageMapToolClass *klass;
      GtkWidget             *shell;
      GtkWidget             *vbox;
      GtkWidget             *toggle;
      const gchar           *stock_id;

      klass = GIMP_IMAGE_MAP_TOOL_GET_CLASS (image_map_tool);

      stock_id = gimp_viewable_get_stock_id (GIMP_VIEWABLE (tool_info));

      image_map_tool->shell = shell =
        gimp_tool_dialog_new (tool_info,
                              display->shell,
                              klass->shell_desc,

                              GIMP_STOCK_RESET, RESPONSE_RESET,
                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                              GTK_STOCK_OK,     GTK_RESPONSE_OK,

                              NULL);

      gtk_dialog_set_alternative_button_order (GTK_DIALOG (shell),
                                               RESPONSE_RESET,
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      g_signal_connect_object (shell, "response",
                               G_CALLBACK (gimp_image_map_tool_response),
                               G_OBJECT (image_map_tool), 0);

      image_map_tool->main_vbox = vbox = gtk_vbox_new (FALSE, 6);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
      gtk_container_add (GTK_CONTAINER (GTK_DIALOG (shell)->vbox), vbox);

      /*  The preview toggle  */
      toggle = gimp_prop_check_button_new (G_OBJECT (tool_info->tool_options),
                                           "preview",
                                           _("_Preview"));

      gtk_box_pack_end (GTK_BOX (image_map_tool->main_vbox), toggle,
                        FALSE, FALSE, 0);
      gtk_widget_show (toggle);

      g_signal_connect_object (tool_info->tool_options, "notify::preview",
                               G_CALLBACK (gimp_image_map_tool_notify_preview),
                               image_map_tool, 0);

      if (klass->load_dialog_title)
        {
          image_map_tool->load_button =
            g_object_new (GIMP_TYPE_BUTTON,
                          "label",         GTK_STOCK_OPEN,
                          "use-stock",     TRUE,
                          "use-underline", TRUE,
                          NULL);

          g_signal_connect (image_map_tool->load_button, "clicked",
                            G_CALLBACK (gimp_image_map_tool_load_clicked),
                            image_map_tool);
          g_signal_connect (image_map_tool->load_button, "extended-clicked",
                            G_CALLBACK (gimp_image_map_tool_load_ext_clicked),
                            image_map_tool);

          if (klass->load_button_tip)
            {
              gchar *str = g_strdup_printf ("%s\n"
                                            "(%s)  %s",
                                            klass->load_button_tip,
                                            gimp_get_mod_string (GDK_SHIFT_MASK),
                                            _("Quick Load"));

              gimp_help_set_help_data (image_map_tool->load_button, str, NULL);
              g_free (str);
            }
        }

      if (klass->save_dialog_title)
        {
          image_map_tool->save_button =
            g_object_new (GIMP_TYPE_BUTTON,
                          "label",         GTK_STOCK_SAVE,
                          "use-stock",     TRUE,
                          "use-underline", TRUE,
                          NULL);

          g_signal_connect (image_map_tool->save_button, "clicked",
                            G_CALLBACK (gimp_image_map_tool_save_clicked),
                            image_map_tool);
          g_signal_connect (image_map_tool->save_button, "extended-clicked",
                            G_CALLBACK (gimp_image_map_tool_save_ext_clicked),
                            image_map_tool);

          if (klass->save_button_tip)
            {
              gchar *str = g_strdup_printf ("%s\n"
                                            "(%s)  %s",
                                            klass->save_button_tip,
                                            gimp_get_mod_string (GDK_SHIFT_MASK),
                                            _("Quick Save"));

              gimp_help_set_help_data (image_map_tool->save_button, str, NULL);
              g_free (str);
            }
        }

      /*  Fill in subclass widgets  */
      gimp_image_map_tool_dialog (image_map_tool);

      gtk_widget_show (vbox);
    }

  drawable = gimp_image_active_drawable (display->image);

  gimp_viewable_dialog_set_viewable (GIMP_VIEWABLE_DIALOG (image_map_tool->shell),
                                     GIMP_VIEWABLE (drawable),
                                     GIMP_CONTEXT (tool_info->tool_options));

  gtk_widget_show (image_map_tool->shell);

  image_map_tool->drawable  = drawable;
  image_map_tool->image_map = gimp_image_map_new (drawable, tool_info->blurb);

  g_signal_connect (image_map_tool->image_map, "flush",
                    G_CALLBACK (gimp_image_map_tool_flush),
                    image_map_tool);

  return TRUE;
}

static void
gimp_image_map_tool_control (GimpTool       *tool,
                             GimpToolAction  action,
                             GimpDisplay    *display)
{
  GimpImageMapTool *image_map_tool = GIMP_IMAGE_MAP_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      if (image_map_tool->shell)
        gtk_dialog_response (GTK_DIALOG (image_map_tool->shell),
                             GTK_RESPONSE_CANCEL);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static gboolean
gimp_image_map_tool_key_press (GimpTool    *tool,
                               GdkEventKey *kevent,
                               GimpDisplay *display)
{
  GimpImageMapTool *image_map_tool = GIMP_IMAGE_MAP_TOOL (tool);

  if (display == tool->display)
    {
      switch (kevent->keyval)
        {
        case GDK_KP_Enter:
        case GDK_Return:
          gimp_image_map_tool_response (NULL, GTK_RESPONSE_OK, image_map_tool);
          return TRUE;

        case GDK_Delete:
        case GDK_BackSpace:
          gimp_image_map_tool_response (NULL, RESPONSE_RESET, image_map_tool);
          return TRUE;

        case GDK_Escape:
          gimp_image_map_tool_response (NULL, GTK_RESPONSE_CANCEL, image_map_tool);
          return TRUE;
        }
    }

  return FALSE;
}

static gboolean
gimp_image_map_tool_pick_color (GimpColorTool *color_tool,
                                gint           x,
                                gint           y,
                                GimpImageType *sample_type,
                                GimpRGB       *color,
                                gint          *color_index)
{
  GimpImageMapTool *tool = GIMP_IMAGE_MAP_TOOL (color_tool);
  gint              off_x, off_y;

  gimp_item_offsets (GIMP_ITEM (tool->drawable), &off_x, &off_y);

  *sample_type = gimp_drawable_type (tool->drawable);

  return gimp_pickable_pick_color (GIMP_PICKABLE (tool->image_map),
                                   x - off_x,
                                   y - off_y,
                                   color_tool->options->sample_average,
                                   color_tool->options->average_radius,
                                   color, color_index);
}

static void
gimp_image_map_tool_map (GimpImageMapTool *tool)
{
  GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool)->map (tool);
}

static void
gimp_image_map_tool_dialog (GimpImageMapTool *tool)
{
  GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool)->dialog (tool);
}

static void
gimp_image_map_tool_reset (GimpImageMapTool *tool)
{
  GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool)->reset (tool);
}

static gboolean
gimp_image_map_tool_settings_load (GimpImageMapTool  *tool,
                                   gpointer           file,
                                   GError           **error)
{
  GimpImageMapToolClass *tool_class = GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool);

  g_return_val_if_fail (tool_class->settings_load != NULL, FALSE);

  if (tool_class->settings_load (tool, file, error))
    {
      gimp_image_map_tool_preview (tool);
      return TRUE;
    }

  return FALSE;
}

static gboolean
gimp_image_map_tool_settings_save (GimpImageMapTool *tool,
                                   gpointer          file)
{
  GimpImageMapToolClass *tool_class = GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool);

  g_return_val_if_fail (tool_class->settings_save != NULL, FALSE);

  return tool_class->settings_save (tool, file);
}

static void
gimp_image_map_tool_flush (GimpImageMap     *image_map,
                           GimpImageMapTool *image_map_tool)
{
  GimpTool *tool = GIMP_TOOL (image_map_tool);

  gimp_projection_flush_now (tool->display->image->projection);
  gimp_display_flush_now (tool->display);
}

static void
gimp_image_map_tool_response (GtkWidget        *widget,
                              gint              response_id,
                              GimpImageMapTool *image_map_tool)
{
  GimpTool *tool = GIMP_TOOL (image_map_tool);

  switch (response_id)
    {
    case RESPONSE_RESET:
      gimp_image_map_tool_reset (image_map_tool);
      gimp_image_map_tool_preview (image_map_tool);
      break;

    case GTK_RESPONSE_OK:
      gimp_dialog_factory_hide_dialog (image_map_tool->shell);

      if (image_map_tool->image_map)
        {
          GimpImageMapOptions *options = GIMP_IMAGE_MAP_TOOL_GET_OPTIONS (tool);

          gimp_tool_control_set_preserve (tool->control, TRUE);

          if (! options->preview)
            gimp_image_map_tool_map (image_map_tool);

          gimp_image_map_commit (image_map_tool->image_map);
          g_object_unref (image_map_tool->image_map);
          image_map_tool->image_map = NULL;

          gimp_tool_control_set_preserve (tool->control, FALSE);

          gimp_viewable_invalidate_preview (GIMP_VIEWABLE (image_map_tool->drawable));
          gimp_image_flush (tool->display->image);
        }

      tool->display  = NULL;
      tool->drawable = NULL;
      break;

    default:
      gimp_dialog_factory_hide_dialog (image_map_tool->shell);

      if (image_map_tool->image_map)
        {
          gimp_tool_control_set_preserve (tool->control, TRUE);

          gimp_image_map_abort (image_map_tool->image_map);
          g_object_unref (image_map_tool->image_map);
          image_map_tool->image_map = NULL;

          gimp_tool_control_set_preserve (tool->control, FALSE);

          gimp_viewable_invalidate_preview (GIMP_VIEWABLE (image_map_tool->drawable));
          gimp_image_flush (tool->display->image);
        }

      tool->display  = NULL;
      tool->drawable = NULL;
      break;
    }
}

static void
gimp_image_map_tool_notify_preview (GObject          *config,
                                    GParamSpec       *pspec,
                                    GimpImageMapTool *image_map_tool)
{
  GimpTool            *tool    = GIMP_TOOL (image_map_tool);
  GimpImageMapOptions *options = GIMP_IMAGE_MAP_OPTIONS (config);

  if (options->preview)
    {
      gimp_tool_control_set_preserve (tool->control, TRUE);

      gimp_image_map_tool_map (image_map_tool);

      gimp_tool_control_set_preserve (tool->control, FALSE);
    }
  else
    {
      if (image_map_tool->image_map)
        {
          gimp_tool_control_set_preserve (tool->control, TRUE);

          gimp_image_map_clear (image_map_tool->image_map);

          gimp_tool_control_set_preserve (tool->control, FALSE);

          gimp_image_map_tool_flush (image_map_tool->image_map,
                                     image_map_tool);
        }
    }
}

void
gimp_image_map_tool_preview (GimpImageMapTool *image_map_tool)
{
  GimpTool            *tool;
  GimpImageMapOptions *options;

  g_return_if_fail (GIMP_IS_IMAGE_MAP_TOOL (image_map_tool));

  tool    = GIMP_TOOL (image_map_tool);
  options = GIMP_IMAGE_MAP_TOOL_GET_OPTIONS (tool);

  if (options->preview)
    {
      gimp_tool_control_set_preserve (tool->control, TRUE);

      gimp_image_map_tool_map (image_map_tool);

      gimp_tool_control_set_preserve (tool->control, FALSE);
    }
}

static void
gimp_image_map_tool_load_save (GimpImageMapTool *tool,
                               const gchar      *filename,
                               gboolean          save)
{
  FILE   *file;
  GError *error = NULL;

  file = g_fopen (filename, save ? "wt" : "rt");

  if (! file)
    {
      const gchar *format = save ?
        _("Could not open '%s' for writing: %s") :
        _("Could not open '%s' for reading: %s");

      gimp_message (GIMP_TOOL (tool)->tool_info->gimp, G_OBJECT (tool->shell),
                    GIMP_MESSAGE_ERROR,
                    format,
                    gimp_filename_to_utf8 (filename),
                    g_strerror (errno));
      return;
    }

  g_object_set (GIMP_TOOL_GET_OPTIONS (tool),
                "settings", filename,
                NULL);

  if (save)
    {
      if (gimp_image_map_tool_settings_save (tool, file))
        {
          gchar *name = g_filename_display_name (filename);

          gimp_message (GIMP_TOOL (tool)->tool_info->gimp,
                        G_OBJECT (GIMP_TOOL (tool)->display),
                        GIMP_MESSAGE_INFO,
                        _("Settings saved to '%s'"),
                        name);
          g_free (name);
        }
    }
  else if (! gimp_image_map_tool_settings_load (tool, file, &error))
    {
      gimp_message (GIMP_TOOL (tool)->tool_info->gimp, G_OBJECT (tool->shell),
                    GIMP_MESSAGE_ERROR,
                    _("Error reading '%s': %s"),
                    gimp_filename_to_utf8 (filename),
                    error->message);
      g_error_free (error);
    }

  fclose (file);
}

static void
settings_dialog_response (GtkWidget        *dialog,
                          gint              response_id,
                          GimpImageMapTool *tool)
{
  gboolean save;

  save = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (dialog), "save"));

  if (response_id == GTK_RESPONSE_OK)
    {
      gchar *filename;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      gimp_image_map_tool_load_save (tool, filename, save);

      g_free (filename);
    }

  if (save)
    gtk_widget_set_sensitive (tool->load_button, TRUE);
  else
    gtk_widget_set_sensitive (tool->save_button, TRUE);

  gtk_widget_destroy (dialog);
}

static void
gimp_image_map_tool_load_clicked (GtkWidget        *widget,
                                  GimpImageMapTool *tool)
{
  GimpImageMapToolClass *klass = GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool);

  gimp_image_map_tool_settings_dialog (tool, klass->load_dialog_title, FALSE);
}

static void
gimp_image_map_tool_load_ext_clicked (GtkWidget        *widget,
                                      GdkModifierType   state,
                                      GimpImageMapTool *tool)
{
  if (state & GDK_SHIFT_MASK)
    {
      gchar *filename;

      g_object_get (GIMP_TOOL_GET_OPTIONS (tool),
                    "settings", &filename,
                    NULL);

      if (filename)
        {
          gimp_image_map_tool_load_save (tool, filename, FALSE);
          g_free (filename);
        }
      else
        {
          gimp_image_map_tool_load_clicked (widget, tool);
        }
    }
}

static void
gimp_image_map_tool_save_clicked (GtkWidget        *widget,
                                  GimpImageMapTool *tool)
{
  GimpImageMapToolClass *klass = GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool);

  gimp_image_map_tool_settings_dialog (tool, klass->save_dialog_title, TRUE);
}

static void
gimp_image_map_tool_save_ext_clicked (GtkWidget        *widget,
                                      GdkModifierType   state,
                                      GimpImageMapTool *tool)
{
  if (state & GDK_SHIFT_MASK)
    {
      gchar *filename;

      g_object_get (GIMP_TOOL_GET_OPTIONS (tool),
                    "settings", &filename,
                    NULL);

      if (filename)
        {
          gimp_image_map_tool_load_save (tool, filename, TRUE);
          g_free (filename);
        }
      else
        {
          gimp_image_map_tool_save_clicked (widget, tool);
        }
    }
}

static void
gimp_image_map_tool_settings_dialog (GimpImageMapTool *tool,
                                     const gchar      *title,
                                     gboolean          save)
{
  GimpImageMapOptions *options = GIMP_IMAGE_MAP_TOOL_GET_OPTIONS (tool);
  GtkFileChooser      *chooser;
  const gchar         *settings_name;
  gchar               *folder;

  settings_name = GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool)->settings_name;

  g_return_if_fail (settings_name != NULL);

  if (tool->settings_dialog)
    {
      gtk_window_present (GTK_WINDOW (tool->settings_dialog));
      return;
    }

  if (save)
    gtk_widget_set_sensitive (tool->load_button, FALSE);
  else
    gtk_widget_set_sensitive (tool->save_button, FALSE);

  tool->settings_dialog =
    gtk_file_chooser_dialog_new (title, GTK_WINDOW (tool->shell),
                                 save ?
                                 GTK_FILE_CHOOSER_ACTION_SAVE :
                                 GTK_FILE_CHOOSER_ACTION_OPEN,

                                 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                 save ? GTK_STOCK_SAVE : GTK_STOCK_OPEN,
                                 GTK_RESPONSE_OK,

                                 NULL);

  chooser = GTK_FILE_CHOOSER (tool->settings_dialog);

  g_object_set_data (G_OBJECT (chooser), "save", GINT_TO_POINTER (save));

  gtk_window_set_role (GTK_WINDOW (chooser), "gimp-load-save-settings");
  gtk_window_set_position (GTK_WINDOW (chooser), GTK_WIN_POS_MOUSE);

  g_object_add_weak_pointer (G_OBJECT (chooser),
                             (gpointer) &tool->settings_dialog);

  gtk_window_set_destroy_with_parent (GTK_WINDOW (chooser), TRUE);

  gtk_dialog_set_default_response (GTK_DIALOG (chooser), GTK_RESPONSE_OK);

  if (save)
    gtk_file_chooser_set_do_overwrite_confirmation (chooser, TRUE);

  g_signal_connect (chooser, "response",
                    G_CALLBACK (settings_dialog_response),
                    tool);
  g_signal_connect (chooser, "delete-event",
                    G_CALLBACK (gtk_true),
                    NULL);

  folder = g_build_filename (gimp_directory (), settings_name, NULL);
  gtk_file_chooser_add_shortcut_folder (chooser, folder, NULL);

  if (options->settings)
    gtk_file_chooser_set_filename (chooser, options->settings);
  else
    gtk_file_chooser_set_current_folder (chooser, folder);

  g_free (folder);

  gimp_help_connect (tool->settings_dialog, gimp_standard_help_func,
                     GIMP_TOOL (tool)->tool_info->help_id, NULL);

  gtk_widget_show (tool->settings_dialog);
}
