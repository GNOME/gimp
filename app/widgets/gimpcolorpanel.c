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

#include "libgimpcolor/gimpcolor.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "gimpaction.h"
#include "gimpcolordialog.h"
#include "gimpcolorpanel.h"


/*  local function prototypes  */

static void       gimp_color_panel_class_init    (GimpColorPanelClass  *klass);
static void       gimp_color_panel_init          (GimpColorPanel       *panel);

static void       gimp_color_panel_destroy         (GtkObject          *object);
static gboolean   gimp_color_panel_button_press    (GtkWidget          *widget,
                                                    GdkEventButton     *bevent);
static void       gimp_color_panel_clicked         (GtkButton          *button);
static void       gimp_color_panel_color_changed   (GimpColorButton    *button);
static GType      gimp_color_panel_get_action_type (GimpColorButton    *button);

static void       gimp_color_panel_dialog_update   (GimpColorDialog    *dialog,
                                                    const GimpRGB      *color,
                                                    GimpColorDialogState state,
                                                    GimpColorPanel     *panel);


static GimpColorButtonClass *parent_class = NULL;


GType
gimp_color_panel_get_type (void)
{
  static GType panel_type = 0;

  if (! panel_type)
    {
      static const GTypeInfo panel_info =
      {
        sizeof (GimpColorPanelClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_color_panel_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpColorPanel),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_color_panel_init,
      };

      panel_type = g_type_register_static (GIMP_TYPE_COLOR_BUTTON,
                                           "GimpColorPanel",
                                           &panel_info, 0);
    }

  return panel_type;
}

static void
gimp_color_panel_class_init (GimpColorPanelClass *klass)
{
  GtkObjectClass       *object_class       = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass       *widget_class       = GTK_WIDGET_CLASS (klass);
  GtkButtonClass       *button_class       = GTK_BUTTON_CLASS (klass);
  GimpColorButtonClass *color_button_class = GIMP_COLOR_BUTTON_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->destroy               = gimp_color_panel_destroy;
  widget_class->button_press_event    = gimp_color_panel_button_press;
  button_class->clicked               = gimp_color_panel_clicked;
  color_button_class->color_changed   = gimp_color_panel_color_changed;
  color_button_class->get_action_type = gimp_color_panel_get_action_type;
}

static void
gimp_color_panel_init (GimpColorPanel *panel)
{
  panel->context      = NULL;
  panel->color_dialog = NULL;
}

static void
gimp_color_panel_destroy (GtkObject *object)
{
  GimpColorPanel *panel = GIMP_COLOR_PANEL (object);

  if (panel->color_dialog)
    {
      gtk_widget_destroy (panel->color_dialog);
      panel->color_dialog = NULL;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static gboolean
gimp_color_panel_button_press (GtkWidget      *widget,
                               GdkEventButton *bevent)
{
  if (bevent->button == 3)
    {
      GimpColorButton *color_button;
      GimpColorPanel  *color_panel;
      GtkUIManager    *ui_manager;
      GtkActionGroup  *group;
      GtkAction       *action;
      GimpRGB          color;

      color_button = GIMP_COLOR_BUTTON (widget);
      color_panel  = GIMP_COLOR_PANEL (widget);
      ui_manager   = GTK_UI_MANAGER (color_button->popup_menu);

      group = gtk_ui_manager_get_action_groups (ui_manager)->data;

      action = gtk_action_group_get_action (group,
                                            "color-button-use-foreground");
      g_object_set (action, "visible", color_panel->context != NULL, NULL);

      action = gtk_action_group_get_action (group,
                                            "color-button-use-background");
      g_object_set (action, "visible", color_panel->context != NULL, NULL);

      if (color_panel->context)
        {
          action = gtk_action_group_get_action (group,
                                                "color-button-use-foreground");
          gimp_context_get_foreground (color_panel->context, &color);
          g_object_set (action, "color", &color, NULL);

          action = gtk_action_group_get_action (group,
                                                "color-button-use-background");
          gimp_context_get_background (color_panel->context, &color);
          g_object_set (action, "color", &color, NULL);
        }

      action = gtk_action_group_get_action (group, "color-button-use-black");
      gimp_rgba_set (&color, 0.0, 0.0, 0.0, GIMP_OPACITY_OPAQUE);
      g_object_set (action, "color", &color, NULL);

      action = gtk_action_group_get_action (group, "color-button-use-white");
      gimp_rgba_set (&color, 1.0, 1.0, 1.0, GIMP_OPACITY_OPAQUE);
      g_object_set (action, "color", &color, NULL);
    }

  if (GTK_WIDGET_CLASS (parent_class)->button_press_event)
    return GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, bevent);

  return FALSE;
}

static void
gimp_color_panel_clicked (GtkButton *button)
{
  GimpColorPanel *panel = GIMP_COLOR_PANEL (button);
  GimpRGB         color;

  gimp_color_button_get_color (GIMP_COLOR_BUTTON (button), &color);

  if (! panel->color_dialog)
    {
      panel->color_dialog =
	gimp_color_dialog_new (NULL,
                               GIMP_COLOR_BUTTON (button)->title,
                               NULL, NULL,
                               GTK_WIDGET (button),
                               NULL, NULL,
                               (const GimpRGB *) &color,
                               FALSE,
                               gimp_color_button_has_alpha (GIMP_COLOR_BUTTON (button)));

      g_signal_connect (panel->color_dialog, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &panel->color_dialog);

      g_signal_connect (panel->color_dialog, "update",
                        G_CALLBACK (gimp_color_panel_dialog_update),
                        panel);
    }

  gtk_window_present (GTK_WINDOW (panel->color_dialog));
}

static GType
gimp_color_panel_get_action_type (GimpColorButton *button)
{
  return GIMP_TYPE_ACTION;
}


/*  public functions  */

GtkWidget *
gimp_color_panel_new (const gchar       *title,
		      const GimpRGB     *color,
		      GimpColorAreaType  type,
		      gint               width,
		      gint               height)
{
  GimpColorPanel *panel;

  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (color != NULL, NULL);

  panel = g_object_new (GIMP_TYPE_COLOR_PANEL, NULL);

  GIMP_COLOR_BUTTON (panel)->title = g_strdup (title);

  gimp_color_button_set_type (GIMP_COLOR_BUTTON (panel), type);
  gimp_color_button_set_color (GIMP_COLOR_BUTTON (panel), color);
  gtk_widget_set_size_request (GTK_WIDGET (panel), width, height);

  return GTK_WIDGET (panel);
}

static void
gimp_color_panel_color_changed (GimpColorButton *button)
{
  GimpColorPanel *panel = GIMP_COLOR_PANEL (button);
  GimpRGB         color;

  if (panel->color_dialog)
    {
      gimp_color_button_get_color (GIMP_COLOR_BUTTON (button), &color);
      gimp_color_dialog_set_color (GIMP_COLOR_DIALOG (panel->color_dialog),
                                   &color);
    }
}

void
gimp_color_panel_set_context (GimpColorPanel *panel,
                              GimpContext    *context)
{
  g_return_if_fail (GIMP_IS_COLOR_PANEL (panel));
  g_return_if_fail (context == NULL || GIMP_IS_CONTEXT (context));

  panel->context = context;
}


/*  private functions  */

static void
gimp_color_panel_dialog_update (GimpColorDialog      *dialog,
                                const GimpRGB        *color,
                                GimpColorDialogState  state,
                                GimpColorPanel       *panel)
{
  switch (state)
    {
    case GIMP_COLOR_DIALOG_UPDATE:
      break;

    case GIMP_COLOR_DIALOG_OK:
      gimp_color_button_set_color (GIMP_COLOR_BUTTON (panel), color);
      /* Fallthrough */

    case GIMP_COLOR_DIALOG_CANCEL:
      gtk_widget_hide (panel->color_dialog);
    }
}
