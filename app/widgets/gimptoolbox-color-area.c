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

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdisplay.h"

#include "gimpaction.h"
#include "gimpcolordialog.h"
#include "gimpdialogfactory.h"
#include "gimpfgbgeditor.h"
#include "gimphelp-ids.h"
#include "gimpsessioninfo.h"
#include "gimptoolbox.h"
#include "gimptoolbox-color-area.h"
#include "gimpuimanager.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   color_area_foreground_changed (GimpContext          *context,
                                             GeglColor            *color,
                                             GimpColorDialog      *dialog);
static void   color_area_background_changed (GimpContext          *context,
                                             GeglColor            *color,
                                             GimpColorDialog      *dialog);

static void   color_area_dialog_update      (GimpColorDialog      *dialog,
                                             GeglColor            *color,
                                             GimpColorDialogState  state,
                                             GimpContext          *context);

static void   color_area_color_clicked      (GimpFgBgEditor       *editor,
                                             GimpActiveColor       active_color,
                                             GimpContext          *context);
static void   color_area_color_changed      (GimpContext          *context);
static void   color_area_tooltip            (GimpFgBgEditor       *editor,
                                             GimpFgBgTarget        target,
                                             GtkTooltip           *tooltip,
                                             GimpToolbox          *toolbox);


/*  local variables  */

static GtkWidget       *color_area          = NULL;
static GtkWidget       *color_dialog        = NULL;
static gboolean         color_dialog_active = FALSE;
static GimpActiveColor  edit_color          = GIMP_ACTIVE_COLOR_FOREGROUND;
static GeglColor       *revert_fg           = NULL;
static GeglColor       *revert_bg           = NULL;


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

  gimp_help_set_help_data (color_area, NULL,
                           GIMP_HELP_TOOLBOX_COLOR_AREA);
  g_object_set (color_area, "has-tooltip", TRUE, NULL);

  g_signal_connect (color_area, "color-clicked",
                    G_CALLBACK (color_area_color_clicked),
                    context);
  g_signal_connect_swapped (color_area, "colors-swapped",
                            G_CALLBACK (color_area_color_changed),
                            context);
  g_signal_connect_swapped (color_area, "colors-default",
                            G_CALLBACK (color_area_color_changed),
                            context);
  g_signal_connect_swapped (color_area, "color-dropped",
                            G_CALLBACK (color_area_color_changed),
                            context);

  g_signal_connect (color_area, "tooltip",
                    G_CALLBACK (color_area_tooltip),
                    toolbox);

  return color_area;
}


/*  private functions  */

static void
color_area_foreground_changed (GimpContext     *context,
                               GeglColor       *color,
                               GimpColorDialog *dialog)
{
  if (edit_color == GIMP_ACTIVE_COLOR_FOREGROUND)
    {
      g_signal_handlers_block_by_func (dialog,
                                       color_area_dialog_update,
                                       context);

      gimp_color_dialog_set_color (dialog, color);

      g_signal_handlers_unblock_by_func (dialog,
                                         color_area_dialog_update,
                                         context);
    }
}

static void
color_area_background_changed (GimpContext     *context,
                               GeglColor       *color,
                               GimpColorDialog *dialog)
{
  if (edit_color == GIMP_ACTIVE_COLOR_BACKGROUND)
    {
      g_signal_handlers_block_by_func (dialog,
                                       color_area_dialog_update,
                                       context);

      gimp_color_dialog_set_color (dialog, color);

      g_signal_handlers_unblock_by_func (dialog,
                                         color_area_dialog_update,
                                         context);
    }
}

static void
color_area_dialog_update (GimpColorDialog      *dialog,
                          GeglColor            *color,
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
                                           color_area_background_changed,
                                           dialog);

          gimp_context_set_background (context, color);

          g_signal_handlers_unblock_by_func (context,
                                             color_area_background_changed,
                                             dialog);
        }
      break;

    case GIMP_COLOR_DIALOG_CANCEL:
      gtk_widget_hide (color_dialog);
      color_dialog_active = FALSE;
      gimp_context_set_foreground (context, revert_fg);
      gimp_context_set_background (context, revert_bg);
      break;
    }

  if (gimp_context_get_display (context))
    gimp_display_grab_focus (gimp_context_get_display (context));
}

static void
color_area_color_clicked (GimpFgBgEditor  *editor,
                          GimpActiveColor  active_color,
                          GimpContext     *context)
{
  GeglColor   *color;
  const gchar *title;

  if (! color_dialog_active)
    {
      g_clear_object (&revert_fg);
      revert_fg = gegl_color_duplicate (gimp_context_get_foreground (context));

      g_clear_object (&revert_bg);
      revert_bg = gegl_color_duplicate (gimp_context_get_background (context));
    }

  if (active_color == GIMP_ACTIVE_COLOR_FOREGROUND)
    {
      color = gimp_context_get_foreground (context);
      title = _("Change Foreground Color");
    }
  else
    {
      color = gimp_context_get_background (context);
      title = _("Change Background Color");
    }

  edit_color = active_color;

  if (! color_dialog)
    {
      color_dialog = gimp_color_dialog_new (NULL, context, TRUE,
                                            NULL, NULL, NULL,
                                            GTK_WIDGET (editor),
                                            gimp_dialog_factory_get_singleton (),
                                            "gimp-toolbox-color-dialog",
                                            color, TRUE, FALSE);

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
      gimp_dialog_factory_position_dialog (gimp_dialog_factory_get_singleton (),
                                           "gimp-toolbox-color-dialog",
                                           color_dialog,
                                           gimp_widget_get_monitor (GTK_WIDGET (editor)));
    }

  gtk_window_set_title (GTK_WINDOW (color_dialog), title);
  gimp_color_dialog_set_color (GIMP_COLOR_DIALOG (color_dialog), color);

  gtk_window_present (GTK_WINDOW (color_dialog));
  color_dialog_active = TRUE;
}

static void
color_area_color_changed (GimpContext *context)
{
  if (gimp_context_get_display (context))
    gimp_display_grab_focus (gimp_context_get_display (context));
}

static void
color_area_tooltip (GimpFgBgEditor *editor,
                    GimpFgBgTarget  target,
                    GtkTooltip     *tooltip,
                    GimpToolbox    *toolbox)
{
  GimpUIManager *manager = gimp_dock_get_ui_manager (GIMP_DOCK (toolbox));
  GimpAction    *action  = NULL;
  const gchar   *text    = NULL;

  switch (target)
    {
    case GIMP_FG_BG_TARGET_FOREGROUND:
      text = _("The active foreground color.\n"
               "Click to open the color selection dialog.");
      break;

    case GIMP_FG_BG_TARGET_BACKGROUND:
      text = _("The active background color.\n"
               "Click to open the color selection dialog.");
      break;

    case GIMP_FG_BG_TARGET_SWAP:
      action = gimp_ui_manager_find_action (manager, "context",
                                            "context-colors-swap");
      text = gimp_action_get_tooltip (action);
      break;

    case GIMP_FG_BG_TARGET_DEFAULT:
      action = gimp_ui_manager_find_action (manager, "context",
                                            "context-colors-default");
      text = gimp_action_get_tooltip (action);
      break;

    default:
      break;
    }

  if (text)
    {
      gchar *markup = NULL;

      if (action)
        {
          gchar **accels = gimp_action_get_display_accels (action);

          if (accels && accels[0])
            {
              gchar *escaped = g_markup_escape_text (text, -1);

              markup = g_strdup_printf ("%s  <b>%s</b>", escaped, accels[0]);
              g_free (escaped);
            }

          g_strfreev (accels);
        }

      if (markup)
        {
          gtk_tooltip_set_markup (tooltip, markup);
          g_free (markup);
        }
      else
        {
          gtk_tooltip_set_text (tooltip, text);
        }
    }
}
