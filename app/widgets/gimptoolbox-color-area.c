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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "gimpcolordialog.h"
#include "gimpdialogfactory.h"
#include "gimpfgbgeditor.h"
#include "gimpsessioninfo.h"
#include "gimptoolbox.h"
#include "gimptoolbox-color-area.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   color_area_foreground_changed (GimpContext          *context,
                                             const GimpRGB        *color,
                                             GimpColorDialog      *dialog);
static void   color_area_background_changed (GimpContext          *context,
                                             const GimpRGB        *color,
                                             GimpColorDialog      *dialog);

static void   color_area_dialog_update      (GimpColorDialog      *dialog,
                                             const GimpRGB        *color,
                                             GimpColorDialogState  state,
                                             GimpContext          *context);

static void   color_area_color_clicked      (GimpFgBgEditor       *editor,
                                             GimpActiveColor       active_color,
                                             GimpContext          *context);


/*  local variables  */

static GtkWidget       *color_area          = NULL;
static GtkWidget       *color_dialog        = NULL;
static gboolean         color_dialog_active = FALSE;
static GimpActiveColor  edit_color          = GIMP_ACTIVE_COLOR_FOREGROUND;
static GimpRGB          revert_fg;
static GimpRGB          revert_bg;


/*  public functions  */

GtkWidget *
gimp_toolbox_color_area_create (GimpToolbox *toolbox,
                                gint         width,
                                gint         height)
{
  GimpContext *context;

  g_return_val_if_fail (GIMP_IS_TOOLBOX (toolbox), NULL);

  context = gimp_toolbox_get_context (toolbox);

  color_area = gimp_fg_bg_editor_new (context);
  gtk_widget_set_size_request (color_area, width, height);
  gtk_widget_add_events (color_area,
                         GDK_ENTER_NOTIFY_MASK |
                         GDK_LEAVE_NOTIFY_MASK);

  gimp_help_set_help_data
    (color_area, _("Foreground & background colors.\n"
                   "The black and white squares reset colors.\n"
                   "The arrows swap colors.\n"
                   "Click to open the color selection dialog."), NULL);

  g_signal_connect (color_area, "color-clicked",
                    G_CALLBACK (color_area_color_clicked),
                    context);

  return color_area;
}


/*  private functions  */

static void
color_area_foreground_changed (GimpContext     *context,
                               const GimpRGB   *color,
                               GimpColorDialog *dialog)
{
  if (edit_color == GIMP_ACTIVE_COLOR_FOREGROUND)
    {
      g_signal_handlers_block_by_func (dialog,
                                       color_area_dialog_update,
                                       context);

      /* FIXME this should use GimpColorDialog API */
      gimp_color_selection_set_color (GIMP_COLOR_SELECTION (dialog->selection),
                                      color);

      g_signal_handlers_unblock_by_func (dialog,
                                         color_area_dialog_update,
                                         context);
    }
}

static void
color_area_background_changed (GimpContext     *context,
                               const GimpRGB   *color,
                               GimpColorDialog *dialog)
{
  if (edit_color == GIMP_ACTIVE_COLOR_BACKGROUND)
    {
      g_signal_handlers_block_by_func (dialog,
                                       color_area_dialog_update,
                                       context);

      /* FIXME this should use GimpColorDialog API */
      gimp_color_selection_set_color (GIMP_COLOR_SELECTION (dialog->selection),
                                      color);

      g_signal_handlers_unblock_by_func (dialog,
                                         color_area_dialog_update,
                                         context);
    }
}

static void
color_area_dialog_update (GimpColorDialog      *dialog,
                          const GimpRGB        *color,
                          GimpColorDialogState  state,
                          GimpContext          *context)
{
  switch (state)
    {
    case GIMP_COLOR_DIALOG_OK:
      gtk_widget_hide (color_dialog);
      color_dialog_active = FALSE;
      /* Fallthrough */

    case GIMP_COLOR_DIALOG_UPDATE:
      if (edit_color == GIMP_ACTIVE_COLOR_FOREGROUND)
        {
          g_signal_handlers_block_by_func (context,
                                           color_area_foreground_changed,
                                           dialog);

          gimp_context_set_foreground (context, color);

          g_signal_handlers_unblock_by_func (context,
                                             color_area_foreground_changed,
                                             dialog);
        }
      else
        {
          g_signal_handlers_block_by_func (context,
                                           color_area_foreground_changed,
                                           dialog);

          gimp_context_set_background (context, color);

          g_signal_handlers_unblock_by_func (context,
                                             color_area_foreground_changed,
                                             dialog);
        }
      break;

    case GIMP_COLOR_DIALOG_CANCEL:
      gtk_widget_hide (color_dialog);
      color_dialog_active = FALSE;
      gimp_context_set_foreground (context, &revert_fg);
      gimp_context_set_background (context, &revert_bg);
      break;
    }
}

static void
color_area_color_clicked (GimpFgBgEditor  *editor,
                          GimpActiveColor  active_color,
                          GimpContext     *context)
{
  GimpRGB      color;
  const gchar *title;

  if (! color_dialog_active)
    {
      gimp_context_get_foreground (context, &revert_fg);
      gimp_context_get_background (context, &revert_bg);
    }

  if (active_color == GIMP_ACTIVE_COLOR_FOREGROUND)
    {
      gimp_context_get_foreground (context, &color);
      title = _("Change Foreground Color");
    }
  else
    {
      gimp_context_get_background (context, &color);
      title = _("Change Background Color");
    }

  edit_color = active_color;

  if (! color_dialog)
    {
      color_dialog = gimp_color_dialog_new (NULL, context,
                                            NULL, NULL, NULL,
                                            GTK_WIDGET (editor),
                                            gimp_dialog_factory_get_singleton (),
                                            "gimp-toolbox-color-dialog",
                                            &color,
                                            TRUE, FALSE);

      g_signal_connect_object (color_dialog, "update",
                               G_CALLBACK (color_area_dialog_update),
                               G_OBJECT (context), 0);

      g_signal_connect_object (context, "foreground-changed",
                               G_CALLBACK (color_area_foreground_changed),
                               G_OBJECT (color_dialog), 0);
      g_signal_connect_object (context, "background-changed",
                               G_CALLBACK (color_area_background_changed),
                               G_OBJECT (color_dialog), 0);
    }
  else if (! gtk_widget_get_visible (color_dialog))
    {
      /*  See https://gitlab.gnome.org/GNOME/gimp/issues/1093
       *
       *  We correctly position all newly created dialog via
       *  gimp_dialog_factory_add_dialog(), but the color dialog is
       *  special, it's never destroyed but created only once per
       *  session. On re-showing, whatever window managing magic kicks
       *  in and the dialog sometimes goes where it shouldn't.
       *
       *  The code below belongs into GimpDialogFactory, perhaps a new
       *  function gimp_dialog_factory_position_dialog() and does the
       *  same positioning logic as add_dialog().
       */
      GimpDialogFactory *dialog_factory = gimp_dialog_factory_get_singleton ();
      GimpSessionInfo   *info;

      info = gimp_dialog_factory_find_session_info (dialog_factory,
                                                    "gimp-toolbox-color-dialog");

      if (gimp_session_info_get_widget (info) == color_dialog)
        {
          GdkScreen     *screen  = gtk_widget_get_screen (GTK_WIDGET (editor));
          gint           monitor = gimp_widget_get_monitor (GTK_WIDGET (editor));
          GimpGuiConfig *gui_config;

          gui_config = GIMP_GUI_CONFIG (context->gimp->config);

          gimp_session_info_apply_geometry (info,
                                            screen, monitor,
                                            gui_config->restore_monitor);
        }
    }

  gtk_window_set_title (GTK_WINDOW (color_dialog), title);
  gimp_color_dialog_set_color (GIMP_COLOR_DIALOG (color_dialog), &color);

  gtk_window_present (GTK_WINDOW (color_dialog));
  color_dialog_active = TRUE;
}
