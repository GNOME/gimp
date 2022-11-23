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

#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmadisplay.h"

#include "ligmaaction.h"
#include "ligmacolordialog.h"
#include "ligmadialogfactory.h"
#include "ligmafgbgeditor.h"
#include "ligmahelp-ids.h"
#include "ligmasessioninfo.h"
#include "ligmatoolbox.h"
#include "ligmatoolbox-color-area.h"
#include "ligmauimanager.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void   color_area_foreground_changed (LigmaContext          *context,
                                             const LigmaRGB        *color,
                                             LigmaColorDialog      *dialog);
static void   color_area_background_changed (LigmaContext          *context,
                                             const LigmaRGB        *color,
                                             LigmaColorDialog      *dialog);

static void   color_area_dialog_update      (LigmaColorDialog      *dialog,
                                             const LigmaRGB        *color,
                                             LigmaColorDialogState  state,
                                             LigmaContext          *context);

static void   color_area_color_clicked      (LigmaFgBgEditor       *editor,
                                             LigmaActiveColor       active_color,
                                             LigmaContext          *context);
static void   color_area_color_changed      (LigmaContext          *context);
static void   color_area_tooltip            (LigmaFgBgEditor       *editor,
                                             LigmaFgBgTarget        target,
                                             GtkTooltip           *tooltip,
                                             LigmaToolbox          *toolbox);


/*  local variables  */

static GtkWidget       *color_area          = NULL;
static GtkWidget       *color_dialog        = NULL;
static gboolean         color_dialog_active = FALSE;
static LigmaActiveColor  edit_color          = LIGMA_ACTIVE_COLOR_FOREGROUND;
static LigmaRGB          revert_fg;
static LigmaRGB          revert_bg;


/*  public functions  */

GtkWidget *
ligma_toolbox_color_area_create (LigmaToolbox *toolbox,
                                gint         width,
                                gint         height)
{
  LigmaContext *context;

  g_return_val_if_fail (LIGMA_IS_TOOLBOX (toolbox), NULL);

  context = ligma_toolbox_get_context (toolbox);

  color_area = ligma_fg_bg_editor_new (context);
  gtk_widget_set_size_request (color_area, width, height);

  ligma_help_set_help_data (color_area, NULL,
                           LIGMA_HELP_TOOLBOX_COLOR_AREA);
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
color_area_foreground_changed (LigmaContext     *context,
                               const LigmaRGB   *color,
                               LigmaColorDialog *dialog)
{
  if (edit_color == LIGMA_ACTIVE_COLOR_FOREGROUND)
    {
      g_signal_handlers_block_by_func (dialog,
                                       color_area_dialog_update,
                                       context);

      /* FIXME this should use LigmaColorDialog API */
      ligma_color_selection_set_color (LIGMA_COLOR_SELECTION (dialog->selection),
                                      color);

      g_signal_handlers_unblock_by_func (dialog,
                                         color_area_dialog_update,
                                         context);
    }
}

static void
color_area_background_changed (LigmaContext     *context,
                               const LigmaRGB   *color,
                               LigmaColorDialog *dialog)
{
  if (edit_color == LIGMA_ACTIVE_COLOR_BACKGROUND)
    {
      g_signal_handlers_block_by_func (dialog,
                                       color_area_dialog_update,
                                       context);

      /* FIXME this should use LigmaColorDialog API */
      ligma_color_selection_set_color (LIGMA_COLOR_SELECTION (dialog->selection),
                                      color);

      g_signal_handlers_unblock_by_func (dialog,
                                         color_area_dialog_update,
                                         context);
    }
}

static void
color_area_dialog_update (LigmaColorDialog      *dialog,
                          const LigmaRGB        *color,
                          LigmaColorDialogState  state,
                          LigmaContext          *context)
{
  switch (state)
    {
    case LIGMA_COLOR_DIALOG_OK:
      gtk_widget_hide (color_dialog);
      color_dialog_active = FALSE;
      /* Fallthrough */

    case LIGMA_COLOR_DIALOG_UPDATE:
      if (edit_color == LIGMA_ACTIVE_COLOR_FOREGROUND)
        {
          g_signal_handlers_block_by_func (context,
                                           color_area_foreground_changed,
                                           dialog);

          ligma_context_set_foreground (context, color);

          g_signal_handlers_unblock_by_func (context,
                                             color_area_foreground_changed,
                                             dialog);
        }
      else
        {
          g_signal_handlers_block_by_func (context,
                                           color_area_background_changed,
                                           dialog);

          ligma_context_set_background (context, color);

          g_signal_handlers_unblock_by_func (context,
                                             color_area_background_changed,
                                             dialog);
        }
      break;

    case LIGMA_COLOR_DIALOG_CANCEL:
      gtk_widget_hide (color_dialog);
      color_dialog_active = FALSE;
      ligma_context_set_foreground (context, &revert_fg);
      ligma_context_set_background (context, &revert_bg);
      break;
    }

  if (ligma_context_get_display (context))
    ligma_display_grab_focus (ligma_context_get_display (context));
}

static void
color_area_color_clicked (LigmaFgBgEditor  *editor,
                          LigmaActiveColor  active_color,
                          LigmaContext     *context)
{
  LigmaRGB      color;
  const gchar *title;

  if (! color_dialog_active)
    {
      ligma_context_get_foreground (context, &revert_fg);
      ligma_context_get_background (context, &revert_bg);
    }

  if (active_color == LIGMA_ACTIVE_COLOR_FOREGROUND)
    {
      ligma_context_get_foreground (context, &color);
      title = _("Change Foreground Color");
    }
  else
    {
      ligma_context_get_background (context, &color);
      title = _("Change Background Color");
    }

  edit_color = active_color;

  if (! color_dialog)
    {
      color_dialog = ligma_color_dialog_new (NULL, context, TRUE,
                                            NULL, NULL, NULL,
                                            GTK_WIDGET (editor),
                                            ligma_dialog_factory_get_singleton (),
                                            "ligma-toolbox-color-dialog",
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
      ligma_dialog_factory_position_dialog (ligma_dialog_factory_get_singleton (),
                                           "ligma-toolbox-color-dialog",
                                           color_dialog,
                                           ligma_widget_get_monitor (GTK_WIDGET (editor)));
    }

  gtk_window_set_title (GTK_WINDOW (color_dialog), title);
  ligma_color_dialog_set_color (LIGMA_COLOR_DIALOG (color_dialog), &color);

  gtk_window_present (GTK_WINDOW (color_dialog));
  color_dialog_active = TRUE;
}

static void
color_area_color_changed (LigmaContext *context)
{
  if (ligma_context_get_display (context))
    ligma_display_grab_focus (ligma_context_get_display (context));
}

static gboolean
accel_find_func (GtkAccelKey *key,
                 GClosure    *closure,
                 gpointer     data)
{
  return (GClosure *) data == closure;
}

static void
color_area_tooltip (LigmaFgBgEditor *editor,
                    LigmaFgBgTarget  target,
                    GtkTooltip     *tooltip,
                    LigmaToolbox    *toolbox)
{
  LigmaUIManager *manager = ligma_dock_get_ui_manager (LIGMA_DOCK (toolbox));
  LigmaAction    *action  = NULL;
  const gchar   *text    = NULL;

  switch (target)
    {
    case LIGMA_FG_BG_TARGET_FOREGROUND:
      text = _("The active foreground color.\n"
               "Click to open the color selection dialog.");
      break;

    case LIGMA_FG_BG_TARGET_BACKGROUND:
      text = _("The active background color.\n"
               "Click to open the color selection dialog.");
      break;

    case LIGMA_FG_BG_TARGET_SWAP:
      action = ligma_ui_manager_find_action (manager, "context",
                                            "context-colors-swap");
      text = ligma_action_get_tooltip (action);
      break;

    case LIGMA_FG_BG_TARGET_DEFAULT:
      action = ligma_ui_manager_find_action (manager, "context",
                                            "context-colors-default");
      text = ligma_action_get_tooltip (action);
      break;

    default:
      break;
    }

  if (text)
    {
      gchar *markup = NULL;

      if (action)
        {
          GtkAccelGroup *accel_group;
          GClosure      *accel_closure;
          GtkAccelKey   *accel_key;

          accel_closure = ligma_action_get_accel_closure (action);
          accel_group   = gtk_accel_group_from_accel_closure (accel_closure);

          accel_key = gtk_accel_group_find (accel_group,
                                            accel_find_func,
                                            accel_closure);

          if (accel_key            &&
              accel_key->accel_key &&
              (accel_key->accel_flags & GTK_ACCEL_VISIBLE))
            {
              gchar *escaped = g_markup_escape_text (text, -1);
              gchar *accel   = gtk_accelerator_get_label (accel_key->accel_key,
                                                          accel_key->accel_mods);

              markup = g_strdup_printf ("%s  <b>%s</b>", escaped, accel);

              g_free (accel);
              g_free (escaped);
            }
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
