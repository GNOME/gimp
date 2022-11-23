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

#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "config/ligmacoreconfig.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"

#include "ligmaaction.h"
#include "ligmaactiongroup.h"
#include "ligmaactionimpl.h"
#include "ligmacolordialog.h"
#include "ligmacolorpanel.h"


#define RGBA_EPSILON 1e-6

enum
{
  RESPONSE,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void       ligma_color_panel_dispose         (GObject            *object);

static gboolean   ligma_color_panel_button_press    (GtkWidget          *widget,
                                                    GdkEventButton     *bevent);

static void       ligma_color_panel_clicked         (GtkButton          *button);

static void       ligma_color_panel_color_changed   (LigmaColorButton    *button);
static GType      ligma_color_panel_get_action_type (LigmaColorButton    *button);

static void       ligma_color_panel_dialog_update   (LigmaColorDialog    *dialog,
                                                    const LigmaRGB      *color,
                                                    LigmaColorDialogState state,
                                                    LigmaColorPanel     *panel);


G_DEFINE_TYPE (LigmaColorPanel, ligma_color_panel, LIGMA_TYPE_COLOR_BUTTON)

#define parent_class ligma_color_panel_parent_class

static guint color_panel_signals[LAST_SIGNAL] = { 0, };


static void
ligma_color_panel_class_init (LigmaColorPanelClass *klass)
{
  GObjectClass         *object_class       = G_OBJECT_CLASS (klass);
  GtkWidgetClass       *widget_class       = GTK_WIDGET_CLASS (klass);
  GtkButtonClass       *button_class       = GTK_BUTTON_CLASS (klass);
  LigmaColorButtonClass *color_button_class = LIGMA_COLOR_BUTTON_CLASS (klass);

  object_class->dispose               = ligma_color_panel_dispose;

  widget_class->button_press_event    = ligma_color_panel_button_press;

  button_class->clicked               = ligma_color_panel_clicked;

  color_button_class->color_changed   = ligma_color_panel_color_changed;
  color_button_class->get_action_type = ligma_color_panel_get_action_type;

  color_panel_signals[RESPONSE] =
    g_signal_new ("response",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaColorPanelClass, response),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_COLOR_DIALOG_STATE);
}

static void
ligma_color_panel_init (LigmaColorPanel *panel)
{
  panel->context      = NULL;
  panel->color_dialog = NULL;
}

static void
ligma_color_panel_dispose (GObject *object)
{
  LigmaColorPanel *panel = LIGMA_COLOR_PANEL (object);

  if (panel->color_dialog)
    {
      gtk_widget_destroy (panel->color_dialog);
      panel->color_dialog = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static gboolean
ligma_color_panel_button_press (GtkWidget      *widget,
                               GdkEventButton *bevent)
{
  if (gdk_event_triggers_context_menu ((GdkEvent *) bevent))
    {
      LigmaColorButton *color_button;
      LigmaColorPanel  *color_panel;
      GtkUIManager    *ui_manager;
      LigmaActionGroup *group;
      LigmaAction      *action;
      LigmaRGB          color;

      color_button = LIGMA_COLOR_BUTTON (widget);
      color_panel  = LIGMA_COLOR_PANEL (widget);
      ui_manager   = ligma_color_button_get_ui_manager (color_button);

      group = gtk_ui_manager_get_action_groups (ui_manager)->data;

      action = ligma_action_group_get_action (group,
                                             "color-button-use-foreground");
      ligma_action_set_visible (action, color_panel->context != NULL);

      action = ligma_action_group_get_action (group,
                                             "color-button-use-background");
      ligma_action_set_visible (action, color_panel->context != NULL);

      if (color_panel->context)
        {
          action = ligma_action_group_get_action (group,
                                                 "color-button-use-foreground");
          ligma_context_get_foreground (color_panel->context, &color);
          g_object_set (action, "color", &color, NULL);

          action = ligma_action_group_get_action (group,
                                                 "color-button-use-background");
          ligma_context_get_background (color_panel->context, &color);
          g_object_set (action, "color", &color, NULL);
        }

      action = ligma_action_group_get_action (group, "color-button-use-black");
      ligma_rgba_set (&color, 0.0, 0.0, 0.0, LIGMA_OPACITY_OPAQUE);
      g_object_set (action, "color", &color, NULL);

      action = ligma_action_group_get_action (group, "color-button-use-white");
      ligma_rgba_set (&color, 1.0, 1.0, 1.0, LIGMA_OPACITY_OPAQUE);
      g_object_set (action, "color", &color, NULL);
    }

  if (GTK_WIDGET_CLASS (parent_class)->button_press_event)
    return GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, bevent);

  return FALSE;
}

static void
ligma_color_panel_clicked (GtkButton *button)
{
  LigmaColorPanel *panel = LIGMA_COLOR_PANEL (button);
  LigmaRGB         color;

  ligma_color_button_get_color (LIGMA_COLOR_BUTTON (button), &color);

  if (! panel->color_dialog)
    {
      LigmaColorButton *color_button = LIGMA_COLOR_BUTTON (button);

      panel->color_dialog =
        ligma_color_dialog_new (NULL, panel->context, TRUE,
                               ligma_color_button_get_title (color_button),
                               NULL, NULL,
                               GTK_WIDGET (button),
                               NULL, NULL,
                               &color,
                               ligma_color_button_get_update (color_button),
                               ligma_color_button_has_alpha (color_button));

      g_signal_connect (panel->color_dialog, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &panel->color_dialog);

      g_signal_connect (panel->color_dialog, "update",
                        G_CALLBACK (ligma_color_panel_dialog_update),
                        panel);
    }
  else
    {
      ligma_color_dialog_set_color (LIGMA_COLOR_DIALOG (panel->color_dialog),
                                   &color);
    }

  gtk_window_present (GTK_WINDOW (panel->color_dialog));
}

static GType
ligma_color_panel_get_action_type (LigmaColorButton *button)
{
  return LIGMA_TYPE_ACTION_IMPL;
}


/*  public functions  */

GtkWidget *
ligma_color_panel_new (const gchar       *title,
                      const LigmaRGB     *color,
                      LigmaColorAreaType  type,
                      gint               width,
                      gint               height)
{
  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (color != NULL, NULL);
  g_return_val_if_fail (width > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);

  return g_object_new (LIGMA_TYPE_COLOR_PANEL,
                       "title",       title,
                       "type",        type,
                       "color",       color,
                       "area-width",  width,
                       "area-height", height,
                       NULL);
}

static void
ligma_color_panel_color_changed (LigmaColorButton *button)
{
  LigmaColorPanel *panel = LIGMA_COLOR_PANEL (button);
  LigmaRGB         color;

  if (panel->color_dialog)
    {
      LigmaRGB dialog_color;

      ligma_color_button_get_color (LIGMA_COLOR_BUTTON (button), &color);
      ligma_color_dialog_get_color (LIGMA_COLOR_DIALOG (panel->color_dialog),
                                   &dialog_color);

      if (ligma_rgba_distance (&color, &dialog_color) > RGBA_EPSILON ||
          color.a != dialog_color.a)
        {
          ligma_color_dialog_set_color (LIGMA_COLOR_DIALOG (panel->color_dialog),
                                       &color);
        }
    }
}

void
ligma_color_panel_set_context (LigmaColorPanel *panel,
                              LigmaContext    *context)
{
  g_return_if_fail (LIGMA_IS_COLOR_PANEL (panel));
  g_return_if_fail (context == NULL || LIGMA_IS_CONTEXT (context));

  panel->context = context;

  if (context)
    ligma_color_button_set_color_config (LIGMA_COLOR_BUTTON (panel),
                                        context->ligma->config->color_management);
}

void
ligma_color_panel_dialog_response (LigmaColorPanel       *panel,
                                  LigmaColorDialogState  state)
{
  g_return_if_fail (LIGMA_IS_COLOR_PANEL (panel));
  g_return_if_fail (state == LIGMA_COLOR_DIALOG_OK ||
                    state == LIGMA_COLOR_DIALOG_CANCEL);

  if (panel->color_dialog && gtk_widget_get_visible (panel->color_dialog))
    ligma_color_panel_dialog_update (NULL, NULL, state, panel);
}


/*  private functions  */

static void
ligma_color_panel_dialog_update (LigmaColorDialog      *dialog,
                                const LigmaRGB        *color,
                                LigmaColorDialogState  state,
                                LigmaColorPanel       *panel)
{
  switch (state)
    {
    case LIGMA_COLOR_DIALOG_UPDATE:
      if (ligma_color_button_get_update (LIGMA_COLOR_BUTTON (panel)))
        ligma_color_button_set_color (LIGMA_COLOR_BUTTON (panel), color);
      break;

    case LIGMA_COLOR_DIALOG_OK:
    case LIGMA_COLOR_DIALOG_CANCEL:
      /* LigmaColorDialog returns the appropriate color (new one or
       * original one if process cancelled.
       */
      ligma_color_button_set_color (LIGMA_COLOR_BUTTON (panel), color);
      gtk_widget_hide (panel->color_dialog);

      g_signal_emit (panel, color_panel_signals[RESPONSE], 0,
                     state);
      break;
    }
}
