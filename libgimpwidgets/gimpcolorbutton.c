/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolorbutton.c
 * Copyright (C) 1999-2001 Sven Neumann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"

#include "gimpwidgetstypes.h"

#include "gimpcolorarea.h"
#include "gimpcolorbutton.h"
#include "gimpcolornotebook.h"
#include "gimpcolorselection.h"
#include "gimpdialog.h"
#include "gimphelpui.h"
#include "gimpstock.h"
#include "gimpwidgets-private.h"

#include "libgimp/libgimp-intl.h"


#define COLOR_SELECTION_KEY "gimp-color-selection"
#define RESPONSE_RESET      1

#define TODOUBLE(i) (i / 65535.0)
#define TOUINT16(d) ((guint16) (d * 65535 + 0.5))


#define GIMP_COLOR_BUTTON_COLOR_FG    "color-button-use-foreground"
#define GIMP_COLOR_BUTTON_COLOR_BG    "color-button-use-background"
#define GIMP_COLOR_BUTTON_COLOR_BLACK "color-button-use-black"
#define GIMP_COLOR_BUTTON_COLOR_WHITE "color-button-use-white"


enum
{
  COLOR_CHANGED,
  LAST_SIGNAL
};


static void     gimp_color_button_class_init   (GimpColorButtonClass *klass);
static void     gimp_color_button_init         (GimpColorButton      *button,
                                                GimpColorButtonClass *klass);

static void     gimp_color_button_destroy           (GtkObject       *object);

static gboolean gimp_color_button_button_press      (GtkWidget       *widget,
                                                     GdkEventButton  *bevent);
static void     gimp_color_button_state_changed     (GtkWidget       *widget,
                                                     GtkStateType     prev_state);
static void     gimp_color_button_clicked           (GtkButton       *button);
static GType    gimp_color_button_get_action_type   (GimpColorButton *button);

static void     gimp_color_button_dialog_response   (GtkWidget       *dialog,
                                                     gint             response_id,
                                                     GimpColorButton *button);
static void     gimp_color_button_use_color         (GtkAction       *action,
                                                     GimpColorButton *button);
static void     gimp_color_button_area_changed      (GtkWidget       *color_area,
                                                     GimpColorButton *button);
static void     gimp_color_button_selection_changed (GtkWidget       *selection,
                                                     GimpColorButton *button);
static void     gimp_color_button_help_func         (const gchar     *help_id,
                                                     gpointer         help_data);


static GtkActionEntry actions[] =
{
  { "color-button-popup", NULL,
    "Color Button Menu", NULL, NULL,
    NULL
  },

  { GIMP_COLOR_BUTTON_COLOR_FG, NULL,
    N_("_Foreground Color"), NULL, NULL,
    G_CALLBACK (gimp_color_button_use_color)
  },
  { GIMP_COLOR_BUTTON_COLOR_BG, NULL,
    N_("_Background Color"), NULL, NULL,
    G_CALLBACK (gimp_color_button_use_color)
  },
  { GIMP_COLOR_BUTTON_COLOR_BLACK, NULL,
    N_("Blac_k"), NULL, NULL,
    G_CALLBACK (gimp_color_button_use_color)
  },
  { GIMP_COLOR_BUTTON_COLOR_WHITE, NULL,
    N_("_White"), NULL, NULL,
    G_CALLBACK (gimp_color_button_use_color)
  }
};

static guint   gimp_color_button_signals[LAST_SIGNAL] = { 0 };

static GimpButtonClass * parent_class = NULL;


GType
gimp_color_button_get_type (void)
{
  static GType button_type = 0;

  if (!button_type)
    {
      static const GTypeInfo button_info =
      {
        sizeof (GimpColorButtonClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_color_button_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpColorButton),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_color_button_init,
      };

      button_type = g_type_register_static (GIMP_TYPE_BUTTON,
                                            "GimpColorButton",
                                            &button_info, 0);
    }

  return button_type;
}

static void
gimp_color_button_class_init (GimpColorButtonClass *klass)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkButtonClass *button_class = GTK_BUTTON_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gimp_color_button_signals[COLOR_CHANGED] =
    g_signal_new ("color_changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColorButtonClass, color_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->destroy            = gimp_color_button_destroy;

  widget_class->button_press_event = gimp_color_button_button_press;
  widget_class->state_changed      = gimp_color_button_state_changed;

  button_class->clicked            = gimp_color_button_clicked;

  klass->color_changed             = NULL;
  klass->get_action_type           = gimp_color_button_get_action_type;
}

static void
gimp_color_button_init (GimpColorButton      *button,
                        GimpColorButtonClass *klass)
{
  GtkActionGroup *group;
  GtkUIManager   *ui_manager;
  GimpRGB         color;
  gint            i;

  button->continuous_update = FALSE;
  button->title             = NULL;
  button->dialog            = NULL;

  gimp_rgba_set (&color, 0.0, 0.0, 0.0, 1.0);
  button->color_area = gimp_color_area_new (&color, FALSE, GDK_BUTTON2_MASK);
  g_signal_connect (button->color_area, "color_changed",
                    G_CALLBACK (gimp_color_button_area_changed),
                    button);

  gtk_container_add (GTK_CONTAINER (button), button->color_area);
  gtk_widget_show (button->color_area);

  /* right-click opens a popup */
  button->popup_menu = ui_manager = gtk_ui_manager_new ();

  group = gtk_action_group_new ("color-button");

  for (i = 0; i < G_N_ELEMENTS (actions); i++)
    {
      const gchar *label   = gettext (actions[i].label);
      const gchar *tooltip = gettext (actions[i].tooltip);
      GtkAction   *action;

      action = g_object_new (klass->get_action_type (button),
                             "name",     actions[i].name,
                             "label",    label,
                             "tooltip",  tooltip,
                             "stock-id", actions[i].stock_id,
                             NULL);

      if (actions[i].callback)
        g_signal_connect (action, "activate",
                          actions[i].callback,
                          button);

      gtk_action_group_add_action_with_accel (group, action,
                                              actions[i].accelerator);

      g_object_unref (action);
    }

  gtk_ui_manager_insert_action_group (ui_manager, group, -1);
  g_object_unref (group);

  gtk_ui_manager_add_ui_from_string
    (ui_manager,
     "<ui>\n"
     "  <popup action=\"color-button-popup\">\n"
     "    <menuitem action=\"" GIMP_COLOR_BUTTON_COLOR_FG "\" />\n"
     "    <menuitem action=\"" GIMP_COLOR_BUTTON_COLOR_BG "\" />\n"
     "    <separator />\n"
     "    <menuitem action=\"" GIMP_COLOR_BUTTON_COLOR_BLACK "\" />\n"
     "    <menuitem action=\"" GIMP_COLOR_BUTTON_COLOR_WHITE "\" />\n"
     "  </popup>\n"
     "</ui>\n",
     -1, NULL);
}

static void
gimp_color_button_destroy (GtkObject *object)
{
  GimpColorButton *button = GIMP_COLOR_BUTTON (object);

  if (button->title)
    {
      g_free (button->title);
      button->title = NULL;
    }

  if (button->dialog)
    {
      gtk_widget_destroy (button->dialog);
      button->dialog = NULL;
    }

  if (button->color_area)
    {
      gtk_widget_destroy (button->color_area);
      button->color_area = NULL;
    }

  if (button->popup_menu)
    {
      g_object_unref (button->popup_menu);
      button->popup_menu = NULL;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static gboolean
gimp_color_button_button_press (GtkWidget      *widget,
                                GdkEventButton *bevent)
{
  GimpColorButton *button = GIMP_COLOR_BUTTON (widget);

  if (bevent->button == 3)
    {
      GtkWidget *menu = gtk_ui_manager_get_widget (button->popup_menu,
                                                   "/color-button-popup");

      gtk_menu_set_screen (GTK_MENU (menu), gtk_widget_get_screen (widget));

      gtk_menu_popup (GTK_MENU (menu),
                      NULL, NULL, NULL, NULL,
                      bevent->button, bevent->time);
    }

  if (GTK_WIDGET_CLASS (parent_class)->button_press_event)
    return GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, bevent);

  return FALSE;
}

static void
gimp_color_button_state_changed (GtkWidget    *widget,
                                 GtkStateType  prev_state)
{
  g_return_if_fail (GIMP_IS_COLOR_BUTTON (widget));

  if (! GTK_WIDGET_IS_SENSITIVE (widget) && GIMP_COLOR_BUTTON (widget)->dialog)
    gtk_widget_hide (GIMP_COLOR_BUTTON (widget)->dialog);

  if (GTK_WIDGET_CLASS (parent_class)->state_changed)
    GTK_WIDGET_CLASS (parent_class)->state_changed (widget, prev_state);
}

static void
gimp_color_button_clicked (GtkButton *button)
{
  GimpColorButton *color_button = GIMP_COLOR_BUTTON (button);

  if (! color_button->dialog)
    {
      GtkWidget *dialog;
      GtkWidget *selection;
      GimpRGB    color;

      gimp_color_button_get_color (color_button, &color);

      dialog = color_button->dialog =
        gimp_dialog_new (color_button->title, COLOR_SELECTION_KEY,
                         GTK_WIDGET (button), 0,
                         gimp_color_button_help_func, NULL,

                         GIMP_STOCK_RESET, RESPONSE_RESET,
                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                         GTK_STOCK_OK,     GTK_RESPONSE_OK,

                         NULL);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (gimp_color_button_dialog_response),
                        color_button);
      g_signal_connect (dialog, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &color_button->dialog);

      selection = gimp_color_selection_new ();
      gtk_container_set_border_width (GTK_CONTAINER (selection), 6);
      gimp_color_selection_set_show_alpha (GIMP_COLOR_SELECTION (selection),
                                           gimp_color_button_has_alpha (color_button));
      gimp_color_selection_set_color (GIMP_COLOR_SELECTION (selection), &color);
      gimp_color_selection_set_old_color (GIMP_COLOR_SELECTION (selection),
                                          &color);
      gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), selection);
      gtk_widget_show (selection);

      g_signal_connect (selection, "color_changed",
                        G_CALLBACK (gimp_color_button_selection_changed),
                        button);

      g_object_set_data (G_OBJECT (color_button->dialog), COLOR_SELECTION_KEY,
                         selection);
    }

  gtk_window_present (GTK_WINDOW (color_button->dialog));
}

static GType
gimp_color_button_get_action_type (GimpColorButton *button)
{
  return GTK_TYPE_ACTION;
}


/*  public functions  */

/**
 * gimp_color_button_new:
 * @title: String that will be used as title for the color_selector.
 * @width: Width of the colorpreview in pixels.
 * @height: Height of the colorpreview in pixels.
 * @color: A pointer to a #GimpRGB color.
 * @type:
 *
 * Creates a new #GimpColorButton widget.
 *
 * This returns a button with a preview showing the color.
 * When the button is clicked a GtkColorSelectionDialog is opened.
 * If the user changes the color the new color is written into the
 * array that was used to pass the initial color and the "color_changed"
 * signal is emitted.
 *
 * Returns: Pointer to the new #GimpColorButton widget.
 **/
GtkWidget *
gimp_color_button_new (const gchar       *title,
                       gint               width,
                       gint               height,
                       const GimpRGB     *color,
                       GimpColorAreaType  type)
{
  GimpColorButton *button;

  g_return_val_if_fail (color != NULL, NULL);

  button = g_object_new (GIMP_TYPE_COLOR_BUTTON, NULL);

  button->title = g_strdup (title);

  gtk_widget_set_size_request (GTK_WIDGET (button->color_area), width, height);

  gimp_color_area_set_type (GIMP_COLOR_AREA (button->color_area), type);
  gimp_color_area_set_color (GIMP_COLOR_AREA (button->color_area), color);

  return GTK_WIDGET (button);
}

/**
 * gimp_color_button_set_color:
 * @button: Pointer to a #GimpColorButton.
 * @color: Pointer to the new #GimpRGB color.
 *
 * Sets the @button to the given @color.
 **/
void
gimp_color_button_set_color (GimpColorButton *button,
                             const GimpRGB   *color)
{
  g_return_if_fail (GIMP_IS_COLOR_BUTTON (button));
  g_return_if_fail (color != NULL);

  gimp_color_area_set_color (GIMP_COLOR_AREA (button->color_area), color);
}

/**
 * gimp_color_button_get_color:
 * @button: Pointer to a #GimpColorButton.
 * @color: Pointer to a #GimpRGB struct used to return the color.
 *
 * Retrieves the currently set color from the @button.
 **/
void
gimp_color_button_get_color (GimpColorButton *button,
                             GimpRGB         *color)
{
  g_return_if_fail (GIMP_IS_COLOR_BUTTON (button));
  g_return_if_fail (color != NULL);

  gimp_color_area_get_color (GIMP_COLOR_AREA (button->color_area), color);
}

/**
 * gimp_color_button_has_alpha:
 * @button: Pointer to a #GimpColorButton.
 *
 * Checks whether the @buttons shows transparency information.
 *
 * Returns: %TRUE if the @button shows transparency information, %FALSE
 * otherwise.
 **/
gboolean
gimp_color_button_has_alpha (GimpColorButton *button)
{
  g_return_val_if_fail (GIMP_IS_COLOR_BUTTON (button), FALSE);

  return gimp_color_area_has_alpha (GIMP_COLOR_AREA (button->color_area));
}

/**
 * gimp_color_button_set_type:
 * @button: Pointer to a #GimpColorButton.
 * @type: the new #GimpColorAreaType
 *
 * Sets the @button to the given @type. See also gimp_color_area_set_type().
 **/
void
gimp_color_button_set_type (GimpColorButton   *button,
                            GimpColorAreaType  type)
{
  g_return_if_fail (GIMP_IS_COLOR_BUTTON (button));

  gimp_color_area_set_type (GIMP_COLOR_AREA (button->color_area), type);
}

/**
 * gimp_color_button_get_update:
 * @button: A #GimpColorButton widget.
 *
 * Returns the color button's @continuous_update property.
 *
 * Return value: the @continuous_update property.
 **/
gboolean
gimp_color_button_get_update (GimpColorButton *button)
{
  g_return_val_if_fail (GIMP_IS_COLOR_BUTTON (button), FALSE);

  return button->continuous_update;
}

/**
 * gimp_color_button_set_update:
 * @button:     A #GimpColorButton widget.
 * @continuous: The new setting of the @continuous_update property.
 *
 * When set to #TRUE, the @button will emit the "color_changed"
 * continuously while the color is changed in the color selection
 * dialog.
 **/
void
gimp_color_button_set_update (GimpColorButton *button,
                              gboolean         continuous)
{
  g_return_if_fail (GIMP_IS_COLOR_BUTTON (button));

  if (continuous != button->continuous_update)
    {
      button->continuous_update = continuous ? TRUE : FALSE;

      if (button->dialog)
        {
          GimpColorSelection *selection;
          GimpRGB             color;

          selection = g_object_get_data (G_OBJECT (button->dialog),
                                         COLOR_SELECTION_KEY);

          if (button->continuous_update)
            {
              gimp_color_selection_get_color (selection, &color);
              gimp_color_button_set_color (button, &color);
            }
          else
            {
              gimp_color_selection_get_old_color (selection, &color);
              gimp_color_button_set_color (button, &color);
            }
        }
    }
}


/*  private functions  */

static void
gimp_color_button_dialog_response (GtkWidget       *dialog,
                                   gint             response_id,
                                   GimpColorButton *button)
{
  GtkWidget *selection;
  GimpRGB    color;

  selection = g_object_get_data (G_OBJECT (dialog), COLOR_SELECTION_KEY);

  switch (response_id)
    {
    case RESPONSE_RESET:
      gimp_color_selection_reset (GIMP_COLOR_SELECTION (selection));
      break;

    case GTK_RESPONSE_OK:
      if (! button->continuous_update)
        {
          gimp_color_selection_get_color (GIMP_COLOR_SELECTION (selection),
                                          &color);
          gimp_color_button_set_color (button, &color);
        }

      gtk_widget_hide (dialog);
      break;

    default:
      if (button->continuous_update)
        {
          gimp_color_selection_get_old_color (GIMP_COLOR_SELECTION (selection),
                                              &color);
          gimp_color_button_set_color (button, &color);
        }

      gtk_widget_hide (dialog);
      break;
    }
}

static void
gimp_color_button_use_color (GtkAction       *action,
                             GimpColorButton *button)
{
  const gchar *name;
  GimpRGB      color;

  name = gtk_action_get_name (action);
  gimp_color_button_get_color (button, &color);

  if (! strcmp (name, GIMP_COLOR_BUTTON_COLOR_FG))
    {
      if (_gimp_get_foreground_func)
        _gimp_get_foreground_func (&color);
      else
        gimp_rgba_set (&color, 0.0, 0.0, 0.0, 1.0);
    }
  else if (! strcmp (name, GIMP_COLOR_BUTTON_COLOR_BG))
    {
      if (_gimp_get_background_func)
        _gimp_get_background_func (&color);
      else
        gimp_rgba_set (&color, 1.0, 1.0, 1.0, 1.0);
    }
  else if (! strcmp (name, GIMP_COLOR_BUTTON_COLOR_BLACK))
    {
      gimp_rgba_set (&color, 0.0, 0.0, 0.0, 1.0);
    }
  else if (! strcmp (name, GIMP_COLOR_BUTTON_COLOR_WHITE))
    {
      gimp_rgba_set (&color, 1.0, 1.0, 1.0, 1.0);
    }

  gimp_color_button_set_color (button, &color);
}

static void
gimp_color_button_area_changed (GtkWidget       *color_area,
                                GimpColorButton *button)
{
  if (button->dialog)
    {
      GimpColorSelection *selection;
      GimpRGB             color;

      selection = g_object_get_data (G_OBJECT (button->dialog),
                                     COLOR_SELECTION_KEY);

      gimp_color_button_get_color (button, &color);

      g_signal_handlers_block_by_func (selection,
                                       gimp_color_button_selection_changed,
                                       button);

      gimp_color_selection_set_color (selection, &color);

      g_signal_handlers_unblock_by_func (selection,
                                         gimp_color_button_selection_changed,
                                         button);
    }

  g_signal_emit (button, gimp_color_button_signals[COLOR_CHANGED], 0);
}

static void
gimp_color_button_selection_changed (GtkWidget       *selection,
                                     GimpColorButton *button)
{
  if (button->continuous_update)
    {
      GimpRGB color;

      gimp_color_selection_get_color (GIMP_COLOR_SELECTION (selection), &color);

      g_signal_handlers_block_by_func (button->color_area,
                                       gimp_color_button_area_changed,
                                       button);

      gimp_color_area_set_color (GIMP_COLOR_AREA (button->color_area), &color);

      g_signal_handlers_unblock_by_func (button->color_area,
                                         gimp_color_button_area_changed,
                                         button);

      g_signal_emit (button, gimp_color_button_signals[COLOR_CHANGED], 0);
    }
}

static void
gimp_color_button_help_func (const gchar *help_id,
                             gpointer     help_data)
{
  GimpColorSelection *selection;
  GimpColorNotebook  *notebook;

  selection = g_object_get_data (G_OBJECT (help_data), COLOR_SELECTION_KEY);

  notebook = GIMP_COLOR_NOTEBOOK (selection->notebook);

  help_id = GIMP_COLOR_SELECTOR_GET_CLASS (notebook->cur_page)->help_id;

  gimp_standard_help_func (help_id, NULL);
}
