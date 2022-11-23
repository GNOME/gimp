/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacolorbutton.c
 * Copyright (C) 1999-2001 Sven Neumann
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"

#include "ligmawidgetstypes.h"

#include "ligmacolorarea.h"
#include "ligmacolorbutton.h"
#include "ligmacolornotebook.h"
#include "ligmacolorselection.h"
#include "ligmadialog.h"
#include "ligmahelpui.h"
#include "ligmaicons.h"
#include "ligmawidgets-private.h"

#include "libligma/libligma-intl.h"


/**
 * SECTION: ligmacolorbutton
 * @title: LigmaColorButton
 * @short_description: Widget for selecting a color from a simple button.
 * @see_also: #libligmacolor-ligmacolorspace
 *
 * This widget provides a simple button with a preview showing the
 * color.
 *
 * On click a color selection dialog is opened. Additionally the
 * button supports Drag and Drop and has a right-click menu that
 * allows one to choose the color from the current FG or BG color. If
 * the user changes the color, the "color-changed" signal is emitted.
 **/


#define COLOR_BUTTON_KEY "ligma-color-button"
#define RESPONSE_RESET   1

#define TODOUBLE(i) (i / 65535.0)
#define TOUINT16(d) ((guint16) (d * 65535 + 0.5))


#define LIGMA_COLOR_BUTTON_COLOR_FG    "color-button-use-foreground"
#define LIGMA_COLOR_BUTTON_COLOR_BG    "color-button-use-background"
#define LIGMA_COLOR_BUTTON_COLOR_BLACK "color-button-use-black"
#define LIGMA_COLOR_BUTTON_COLOR_WHITE "color-button-use-white"


enum
{
  COLOR_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_TITLE,
  PROP_COLOR,
  PROP_TYPE,
  PROP_UPDATE,
  PROP_AREA_WIDTH,
  PROP_AREA_HEIGHT,
  PROP_COLOR_CONFIG
};


struct _LigmaColorButtonPrivate
{
  gchar           *title;
  gboolean         continuous_update;

  GtkWidget       *color_area;
  GtkWidget       *dialog;
  GtkWidget       *selection;

  GtkUIManager    *ui_manager;

  LigmaColorConfig *config;
};

#define GET_PRIVATE(obj) (((LigmaColorButton *) (obj))->priv)


static void     ligma_color_button_constructed         (GObject         *object);
static void     ligma_color_button_finalize            (GObject         *object);
static void     ligma_color_button_dispose             (GObject         *object);
static void     ligma_color_button_get_property        (GObject         *object,
                                                       guint            property_id,
                                                       GValue          *value,
                                                       GParamSpec      *pspec);
static void     ligma_color_button_set_property        (GObject         *object,
                                                       guint            property_id,
                                                       const GValue    *value,
                                                       GParamSpec      *pspec);

static gboolean ligma_color_button_button_press        (GtkWidget       *widget,
                                                       GdkEventButton  *bevent);
static void     ligma_color_button_state_flags_changed (GtkWidget       *widget,
                                                       GtkStateFlags    prev_state);
static void     ligma_color_button_clicked             (GtkButton       *button);
static GType    ligma_color_button_get_action_type     (LigmaColorButton *button);

static void     ligma_color_button_dialog_response     (GtkWidget       *dialog,
                                                       gint             response_id,
                                                       LigmaColorButton *button);
static void     ligma_color_button_use_color           (GtkAction       *action,
                                                       LigmaColorButton *button);
static void     ligma_color_button_area_changed        (GtkWidget       *color_area,
                                                       LigmaColorButton *button);
static void     ligma_color_button_selection_changed   (GtkWidget       *selection,
                                                       LigmaColorButton *button);
static void     ligma_color_button_help_func           (const gchar     *help_id,
                                                       gpointer         help_data);


static const GtkActionEntry actions[] =
{
  { "color-button-popup", NULL,
    "Color Button Menu", NULL, NULL,
    NULL
  },

  { LIGMA_COLOR_BUTTON_COLOR_FG, NULL,
    N_("_Foreground Color"), NULL, NULL,
    G_CALLBACK (ligma_color_button_use_color)
  },
  { LIGMA_COLOR_BUTTON_COLOR_BG, NULL,
    N_("_Background Color"), NULL, NULL,
    G_CALLBACK (ligma_color_button_use_color)
  },
  { LIGMA_COLOR_BUTTON_COLOR_BLACK, NULL,
    N_("Blac_k"), NULL, NULL,
    G_CALLBACK (ligma_color_button_use_color)
  },
  { LIGMA_COLOR_BUTTON_COLOR_WHITE, NULL,
    N_("_White"), NULL, NULL,
    G_CALLBACK (ligma_color_button_use_color)
  }
};


G_DEFINE_TYPE_WITH_CODE (LigmaColorButton, ligma_color_button, LIGMA_TYPE_BUTTON,
                         G_ADD_PRIVATE (LigmaColorButton))

#define parent_class ligma_color_button_parent_class

static guint ligma_color_button_signals[LAST_SIGNAL] = { 0 };


static void
ligma_color_button_class_init (LigmaColorButtonClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkButtonClass *button_class = GTK_BUTTON_CLASS (klass);
  LigmaRGB         color;

  parent_class = g_type_class_peek_parent (klass);

  ligma_color_button_signals[COLOR_CHANGED] =
    g_signal_new ("color-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaColorButtonClass, color_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->constructed         = ligma_color_button_constructed;
  object_class->finalize            = ligma_color_button_finalize;
  object_class->dispose             = ligma_color_button_dispose;
  object_class->get_property        = ligma_color_button_get_property;
  object_class->set_property        = ligma_color_button_set_property;

  widget_class->button_press_event  = ligma_color_button_button_press;
  widget_class->state_flags_changed = ligma_color_button_state_flags_changed;

  button_class->clicked             = ligma_color_button_clicked;

  klass->color_changed              = NULL;
  klass->get_action_type            = ligma_color_button_get_action_type;

  ligma_rgba_set (&color, 0.0, 0.0, 0.0, 1.0);

  /**
   * LigmaColorButton:title:
   *
   * The title to be used for the color selection dialog.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_TITLE,
                                   g_param_spec_string ("title",
                                                        "Title",
                                                        "The title to be used for the color selection dialog",
                                                        NULL,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
  /**
   * LigmaColorButton:color:
   *
   * The color displayed in the button's color area.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_COLOR,
                                   ligma_param_spec_rgb ("color",
                                                        "Color",
                                                        "The color displayed in the button's color area",
                                                        TRUE, &color,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
  /**
   * LigmaColorButton:type:
   *
   * The type of the button's color area.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_TYPE,
                                   g_param_spec_enum ("type",
                                                      "Type",
                                                      "The type of the button's color area",
                                                      LIGMA_TYPE_COLOR_AREA_TYPE,
                                                      LIGMA_COLOR_AREA_FLAT,
                                                      LIGMA_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));
  /**
   * LigmaColorButton:continuous-update:
   *
   * The update policy of the color button.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_UPDATE,
                                   g_param_spec_boolean ("continuous-update",
                                                         "Contiguous Update",
                                                         "The update policy of the color button",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
  /**
   * LigmaColorButton:area-width:
   *
   * The minimum width of the button's #LigmaColorArea.
   *
   * Since: 2.8
   */
  g_object_class_install_property (object_class, PROP_AREA_WIDTH,
                                   g_param_spec_int ("area-width",
                                                     "Area Width",
                                                     "The minimum width of the button's LigmaColorArea",
                                                     1, G_MAXINT, 16,
                                                     G_PARAM_WRITABLE |
                                                     G_PARAM_CONSTRUCT));
  /**
   * LigmaColorButton:area-height:
   *
   * The minimum height of the button's #LigmaColorArea.
   *
   * Since: 2.8
   */
  g_object_class_install_property (object_class, PROP_AREA_HEIGHT,
                                   g_param_spec_int ("area-height",
                                                     "Area Height",
                                                     "The minimum height of the button's LigmaColorArea",
                                                     1, G_MAXINT, 16,
                                                     G_PARAM_WRITABLE |
                                                     G_PARAM_CONSTRUCT));
  /**
   * LigmaColorButton:color-config:
   *
   * The #LigmaColorConfig object used for the button's #LigmaColorArea
   * and #LigmaColorSelection.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class, PROP_COLOR_CONFIG,
                                   g_param_spec_object ("color-config",
                                                        "Color Config",
                                                        "The color config object used",
                                                        LIGMA_TYPE_COLOR_CONFIG,
                                                        G_PARAM_READWRITE));
}

static void
ligma_color_button_init (LigmaColorButton *button)
{
  LigmaColorButtonPrivate *priv = ligma_color_button_get_instance_private (button);

  button->priv = priv;

  priv->color_area = g_object_new (LIGMA_TYPE_COLOR_AREA,
                                   "drag-mask", GDK_BUTTON1_MASK,
                                   NULL);

  g_signal_connect (priv->color_area, "color-changed",
                    G_CALLBACK (ligma_color_button_area_changed),
                    button);

  gtk_container_add (GTK_CONTAINER (button), priv->color_area);
  gtk_widget_show (priv->color_area);
}

static void
ligma_color_button_constructed (GObject *object)
{
  LigmaColorButton        *button = LIGMA_COLOR_BUTTON (object);
  LigmaColorButtonClass   *klass  = LIGMA_COLOR_BUTTON_GET_CLASS (object);
  LigmaColorButtonPrivate *priv   = GET_PRIVATE (object);
  GtkActionGroup         *group;
  gint                    i;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  /* right-click opens a popup */
  priv->ui_manager = gtk_ui_manager_new ();

  group = gtk_action_group_new ("color-button");

  for (i = 0; i < G_N_ELEMENTS (actions); i++)
    {
      const gchar *label   = gettext (actions[i].label);
      const gchar *tooltip = gettext (actions[i].tooltip);
      GtkAction   *action;

      action = g_object_new (klass->get_action_type (button),
                             "name",      actions[i].name,
                             "label",     label,
                             "tooltip",   tooltip,
                             "icon-name", actions[i].stock_id,
                             NULL);

      if (actions[i].callback)
        g_signal_connect (action, "activate",
                          actions[i].callback,
                          button);

      gtk_action_group_add_action_with_accel (group, action,
                                              actions[i].accelerator);

      g_object_unref (action);
    }

  gtk_ui_manager_insert_action_group (priv->ui_manager, group, -1);
  g_object_unref (group);

  gtk_ui_manager_add_ui_from_string
    (priv->ui_manager,
     "<ui>\n"
     "  <popup action=\"color-button-popup\">\n"
     "    <menuitem action=\"" LIGMA_COLOR_BUTTON_COLOR_FG "\" />\n"
     "    <menuitem action=\"" LIGMA_COLOR_BUTTON_COLOR_BG "\" />\n"
     "    <separator />\n"
     "    <menuitem action=\"" LIGMA_COLOR_BUTTON_COLOR_BLACK "\" />\n"
     "    <menuitem action=\"" LIGMA_COLOR_BUTTON_COLOR_WHITE "\" />\n"
     "  </popup>\n"
     "</ui>\n",
     -1, NULL);
}

static void
ligma_color_button_finalize (GObject *object)
{
  LigmaColorButtonPrivate *priv = GET_PRIVATE (object);

  g_clear_pointer (&priv->title, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_color_button_dispose (GObject *object)
{
  LigmaColorButton        *button = LIGMA_COLOR_BUTTON (object);
  LigmaColorButtonPrivate *priv   = GET_PRIVATE (button);

  g_clear_pointer (&priv->dialog, gtk_widget_destroy);
  priv->selection = NULL;

  g_clear_pointer (&priv->color_area, gtk_widget_destroy);

  g_clear_object (&priv->ui_manager);

  ligma_color_button_set_color_config (button, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_color_button_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  LigmaColorButtonPrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, priv->title);
      break;

    case PROP_COLOR:
      g_object_get_property (G_OBJECT (priv->color_area), "color", value);
      break;

    case PROP_TYPE:
      g_object_get_property (G_OBJECT (priv->color_area), "type", value);
      break;

    case PROP_UPDATE:
      g_value_set_boolean (value, priv->continuous_update);
      break;

    case PROP_COLOR_CONFIG:
      g_value_set_object (value, priv->config);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_color_button_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  LigmaColorButton        *button = LIGMA_COLOR_BUTTON (object);
  LigmaColorButtonPrivate *priv   = GET_PRIVATE (object);
  gint                    other;

  switch (property_id)
    {
    case PROP_TITLE:
      ligma_color_button_set_title (button, g_value_get_string (value));
      break;

    case PROP_COLOR:
      g_object_set_property (G_OBJECT (priv->color_area), "color", value);
      break;

    case PROP_TYPE:
      g_object_set_property (G_OBJECT (priv->color_area), "type", value);
      break;

    case PROP_UPDATE:
      ligma_color_button_set_update (button, g_value_get_boolean (value));
      break;

    case PROP_AREA_WIDTH:
      gtk_widget_get_size_request (priv->color_area, NULL, &other);
      gtk_widget_set_size_request (priv->color_area,
                                   g_value_get_int (value), other);
      break;

    case PROP_AREA_HEIGHT:
      gtk_widget_get_size_request (priv->color_area, &other, NULL);
      gtk_widget_set_size_request (priv->color_area,
                                   other, g_value_get_int (value));
      break;

    case PROP_COLOR_CONFIG:
      ligma_color_button_set_color_config (button, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
ligma_color_button_button_press (GtkWidget      *widget,
                                GdkEventButton *bevent)
{
  LigmaColorButtonPrivate *priv = GET_PRIVATE (widget);

  if (gdk_event_triggers_context_menu ((GdkEvent *) bevent))
    {
      GtkWidget *menu = gtk_ui_manager_get_widget (priv->ui_manager,
                                                   "/color-button-popup");

      gtk_menu_set_screen (GTK_MENU (menu), gtk_widget_get_screen (widget));

      gtk_menu_popup_at_pointer (GTK_MENU (menu), (GdkEvent *) bevent);
    }

  return GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, bevent);
}

static void
ligma_color_button_state_flags_changed (GtkWidget     *widget,
                                       GtkStateFlags  previous_state)
{
  LigmaColorButtonPrivate *priv = GET_PRIVATE (widget);

  if (! gtk_widget_is_sensitive (widget) && priv->dialog)
    gtk_widget_hide (priv->dialog);

  if (GTK_WIDGET_CLASS (parent_class)->state_flags_changed)
    GTK_WIDGET_CLASS (parent_class)->state_flags_changed (widget,
                                                          previous_state);
}

static void
ligma_color_button_clicked (GtkButton *button)
{
  LigmaColorButton        *color_button = LIGMA_COLOR_BUTTON (button);
  LigmaColorButtonPrivate *priv         = GET_PRIVATE (button);
  LigmaRGB                 color;

  if (! priv->dialog)
    {
      GtkWidget *dialog;

      dialog = priv->dialog =
        ligma_dialog_new (priv->title, "ligma-color-button",
                         gtk_widget_get_toplevel (GTK_WIDGET (button)), 0,
                         ligma_color_button_help_func, NULL,

                         _("_Reset"),  RESPONSE_RESET,
                         _("_Cancel"), GTK_RESPONSE_CANCEL,
                         _("_OK"),     GTK_RESPONSE_OK,

                         NULL);

      g_object_set_data (G_OBJECT (dialog), COLOR_BUTTON_KEY, button);

      ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               RESPONSE_RESET,
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (ligma_color_button_dialog_response),
                        color_button);
      g_signal_connect (dialog, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &priv->dialog);

      priv->selection = ligma_color_selection_new ();
      gtk_container_set_border_width (GTK_CONTAINER (priv->selection), 6);
      ligma_color_selection_set_show_alpha (LIGMA_COLOR_SELECTION (priv->selection),
                                           ligma_color_button_has_alpha (color_button));
      ligma_color_selection_set_config (LIGMA_COLOR_SELECTION (priv->selection),
                                       priv->config);
      gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                          priv->selection, TRUE, TRUE, 0);
      gtk_widget_show (priv->selection);

      g_signal_connect (priv->selection, "color-changed",
                        G_CALLBACK (ligma_color_button_selection_changed),
                        button);
    }

  ligma_color_button_get_color (color_button, &color);

  g_signal_handlers_block_by_func (priv->selection,
                                   ligma_color_button_selection_changed,
                                   button);

  ligma_color_selection_set_color (LIGMA_COLOR_SELECTION (priv->selection), &color);
  ligma_color_selection_set_old_color (LIGMA_COLOR_SELECTION (priv->selection),
                                      &color);

  g_signal_handlers_unblock_by_func (priv->selection,
                                     ligma_color_button_selection_changed,
                                     button);

  gtk_window_present (GTK_WINDOW (priv->dialog));
}

static GType
ligma_color_button_get_action_type (LigmaColorButton *button)
{
  return GTK_TYPE_ACTION;
}


/*  public functions  */

/**
 * ligma_color_button_new:
 * @title:  String that will be used as title for the color_selector.
 * @width:  Width of the colorpreview in pixels.
 * @height: Height of the colorpreview in pixels.
 * @color:  A pointer to a #LigmaRGB color.
 * @type:   The type of transparency to be displayed.
 *
 * Creates a new #LigmaColorButton widget.
 *
 * This returns a button with a preview showing the color.
 * When the button is clicked a GtkColorSelectionDialog is opened.
 * If the user changes the color the new color is written into the
 * array that was used to pass the initial color and the "color-changed"
 * signal is emitted.
 *
 * Returns: Pointer to the new #LigmaColorButton widget.
 **/
GtkWidget *
ligma_color_button_new (const gchar       *title,
                       gint               width,
                       gint               height,
                       const LigmaRGB     *color,
                       LigmaColorAreaType  type)
{
  g_return_val_if_fail (color != NULL, NULL);
  g_return_val_if_fail (width > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);

  return g_object_new (LIGMA_TYPE_COLOR_BUTTON,
                       "title",       title,
                       "type",        type,
                       "color",       color,
                       "area-width",  width,
                       "area-height", height,
                       NULL);
}

/**
 * ligma_color_button_set_title:
 * @button: a #LigmaColorButton.
 * @title:  the new title.
 *
 * Sets the @button dialog's title.
 *
 * Since: 2.10
 **/
void
ligma_color_button_set_title (LigmaColorButton *button,
                             const gchar     *title)
{
  LigmaColorButtonPrivate *priv;

  g_return_if_fail (LIGMA_IS_COLOR_BUTTON (button));
  g_return_if_fail (title != NULL);

  priv = GET_PRIVATE (button);

  g_free (priv->title);
  priv->title = g_strdup (title);

  if (priv->dialog)
    gtk_window_set_title (GTK_WINDOW (priv->dialog), title);

  g_object_notify (G_OBJECT (button), "title");
}

/**
 * ligma_color_button_get_title:
 * @button: a #LigmaColorButton.
 *
 * Returns: The @button dialog's title.
 *
 * Since: 2.10
 **/
const gchar *
ligma_color_button_get_title (LigmaColorButton *button)
{
  LigmaColorButtonPrivate *priv;

  g_return_val_if_fail (LIGMA_IS_COLOR_BUTTON (button), NULL);

  priv = GET_PRIVATE (button);

  return priv->title;
}

/**
 * ligma_color_button_set_color:
 * @button: Pointer to a #LigmaColorButton.
 * @color:  Pointer to the new #LigmaRGB color.
 *
 * Sets the @button to the given @color.
 **/
void
ligma_color_button_set_color (LigmaColorButton *button,
                             const LigmaRGB   *color)
{
  LigmaColorButtonPrivate *priv;

  g_return_if_fail (LIGMA_IS_COLOR_BUTTON (button));
  g_return_if_fail (color != NULL);

  priv = GET_PRIVATE (button);

  ligma_color_area_set_color (LIGMA_COLOR_AREA (priv->color_area), color);

  g_object_notify (G_OBJECT (button), "color");
}

/**
 * ligma_color_button_get_color:
 * @button: Pointer to a #LigmaColorButton.
 * @color:  (out caller-allocates): Pointer to a #LigmaRGB struct
 *          used to return the color.
 *
 * Retrieves the currently set color from the @button.
 **/
void
ligma_color_button_get_color (LigmaColorButton *button,
                             LigmaRGB         *color)
{
  LigmaColorButtonPrivate *priv;

  g_return_if_fail (LIGMA_IS_COLOR_BUTTON (button));
  g_return_if_fail (color != NULL);

  priv = GET_PRIVATE (button);

  ligma_color_area_get_color (LIGMA_COLOR_AREA (priv->color_area), color);
}

/**
 * ligma_color_button_has_alpha:
 * @button: Pointer to a #LigmaColorButton.
 *
 * Checks whether the @buttons shows transparency information.
 *
 * Returns: %TRUE if the @button shows transparency information, %FALSE
 * otherwise.
 **/
gboolean
ligma_color_button_has_alpha (LigmaColorButton *button)
{
  LigmaColorButtonPrivate *priv;

  g_return_val_if_fail (LIGMA_IS_COLOR_BUTTON (button), FALSE);

  priv = GET_PRIVATE (button);

  return ligma_color_area_has_alpha (LIGMA_COLOR_AREA (priv->color_area));
}

/**
 * ligma_color_button_set_type:
 * @button: Pointer to a #LigmaColorButton.
 * @type: the new #LigmaColorAreaType
 *
 * Sets the @button to the given @type. See also ligma_color_area_set_type().
 **/
void
ligma_color_button_set_type (LigmaColorButton   *button,
                            LigmaColorAreaType  type)
{
  LigmaColorButtonPrivate *priv;

  g_return_if_fail (LIGMA_IS_COLOR_BUTTON (button));

  priv = GET_PRIVATE (button);

  ligma_color_area_set_type (LIGMA_COLOR_AREA (priv->color_area), type);

  g_object_notify (G_OBJECT (button), "type");
}

/**
 * ligma_color_button_get_update:
 * @button: A #LigmaColorButton widget.
 *
 * Returns the color button's @continuous_update property.
 *
 * Returns: the @continuous_update property.
 **/
gboolean
ligma_color_button_get_update (LigmaColorButton *button)
{
  LigmaColorButtonPrivate *priv;

  g_return_val_if_fail (LIGMA_IS_COLOR_BUTTON (button), FALSE);

  priv = GET_PRIVATE (button);

  return priv->continuous_update;
}

/**
 * ligma_color_button_set_update:
 * @button:     A #LigmaColorButton widget.
 * @continuous: The new setting of the @continuous_update property.
 *
 * When set to %TRUE, the @button will emit the "color-changed"
 * continuously while the color is changed in the color selection
 * dialog.
 **/
void
ligma_color_button_set_update (LigmaColorButton *button,
                              gboolean         continuous)
{
  LigmaColorButtonPrivate *priv;

  g_return_if_fail (LIGMA_IS_COLOR_BUTTON (button));

  priv = GET_PRIVATE (button);

  if (continuous != priv->continuous_update)
    {
      priv->continuous_update = continuous ? TRUE : FALSE;

      if (priv->selection)
        {
          LigmaRGB color;

          if (priv->continuous_update)
            {
              ligma_color_selection_get_color (LIGMA_COLOR_SELECTION (priv->selection),
                                              &color);
              ligma_color_button_set_color (button, &color);
            }
          else
            {
              ligma_color_selection_get_old_color (LIGMA_COLOR_SELECTION (priv->selection),
                                                  &color);
              ligma_color_button_set_color (button, &color);
            }
        }

      g_object_notify (G_OBJECT (button), "continuous-update");
    }
}

/**
 * ligma_color_button_set_color_config:
 * @button: a #LigmaColorButton widget.
 * @config: a #LigmaColorConfig object.
 *
 * Sets the color management configuration to use with this color button's
 * #LigmaColorArea.
 *
 * Since: 2.10
 */
void
ligma_color_button_set_color_config (LigmaColorButton *button,
                                    LigmaColorConfig *config)
{
  LigmaColorButtonPrivate *priv;

  g_return_if_fail (LIGMA_IS_COLOR_BUTTON (button));
  g_return_if_fail (config == NULL || LIGMA_IS_COLOR_CONFIG (config));

  priv = GET_PRIVATE (button);

  if (g_set_object (&priv->config, config))
    {
      if (priv->color_area)
        ligma_color_area_set_color_config (LIGMA_COLOR_AREA (priv->color_area),
                                          priv->config);

      if (priv->selection)
        ligma_color_selection_set_config (LIGMA_COLOR_SELECTION (priv->selection),
                                         priv->config);
    }
}

/**
 * ligma_color_button_get_ui_manager:
 * @button: a #LigmaColorButton.
 *
 * Returns: (transfer none): The @button's #GtkUIManager.
 *
 * Since: 2.10
 **/
GtkUIManager *
ligma_color_button_get_ui_manager (LigmaColorButton *button)
{
  LigmaColorButtonPrivate *priv;

  g_return_val_if_fail (LIGMA_IS_COLOR_BUTTON (button), NULL);

  priv = GET_PRIVATE (button);

  return priv->ui_manager;
}


/*  private functions  */

static void
ligma_color_button_dialog_response (GtkWidget       *dialog,
                                   gint             response_id,
                                   LigmaColorButton *button)
{
  LigmaColorButtonPrivate *priv = GET_PRIVATE (button);
  LigmaRGB                 color;

  switch (response_id)
    {
    case RESPONSE_RESET:
      ligma_color_selection_reset (LIGMA_COLOR_SELECTION (priv->selection));
      break;

    case GTK_RESPONSE_OK:
      if (! priv->continuous_update)
        {
          ligma_color_selection_get_color (LIGMA_COLOR_SELECTION (priv->selection),
                                          &color);
          ligma_color_button_set_color (button, &color);
        }

      gtk_widget_hide (dialog);
      break;

    default:
      if (priv->continuous_update)
        {
          ligma_color_selection_get_old_color (LIGMA_COLOR_SELECTION (priv->selection),
                                              &color);
          ligma_color_button_set_color (button, &color);
        }

      gtk_widget_hide (dialog);
      break;
    }
}

static void
ligma_color_button_use_color (GtkAction       *action,
                             LigmaColorButton *button)
{
  const gchar *name;
  LigmaRGB      color;

  name = gtk_action_get_name (action);
  ligma_color_button_get_color (button, &color);

  if (! strcmp (name, LIGMA_COLOR_BUTTON_COLOR_FG))
    {
      if (_ligma_get_foreground_func)
        _ligma_get_foreground_func (&color);
      else
        ligma_rgba_set (&color, 0.0, 0.0, 0.0, 1.0);
    }
  else if (! strcmp (name, LIGMA_COLOR_BUTTON_COLOR_BG))
    {
      if (_ligma_get_background_func)
        _ligma_get_background_func (&color);
      else
        ligma_rgba_set (&color, 1.0, 1.0, 1.0, 1.0);
    }
  else if (! strcmp (name, LIGMA_COLOR_BUTTON_COLOR_BLACK))
    {
      ligma_rgba_set (&color, 0.0, 0.0, 0.0, 1.0);
    }
  else if (! strcmp (name, LIGMA_COLOR_BUTTON_COLOR_WHITE))
    {
      ligma_rgba_set (&color, 1.0, 1.0, 1.0, 1.0);
    }

  ligma_color_button_set_color (button, &color);
}

static void
ligma_color_button_area_changed (GtkWidget       *color_area,
                                LigmaColorButton *button)
{
  LigmaColorButtonPrivate *priv = GET_PRIVATE (button);

  if (priv->selection)
    {
      LigmaRGB color;

      ligma_color_button_get_color (button, &color);

      g_signal_handlers_block_by_func (priv->selection,
                                       ligma_color_button_selection_changed,
                                       button);

      ligma_color_selection_set_color (LIGMA_COLOR_SELECTION (priv->selection),
                                      &color);

      g_signal_handlers_unblock_by_func (priv->selection,
                                         ligma_color_button_selection_changed,
                                         button);
    }

  g_signal_emit (button, ligma_color_button_signals[COLOR_CHANGED], 0);
}

static void
ligma_color_button_selection_changed (GtkWidget       *selection,
                                     LigmaColorButton *button)
{
  LigmaColorButtonPrivate *priv = GET_PRIVATE (button);

  if (priv->continuous_update)
    {
      LigmaRGB color;

      ligma_color_selection_get_color (LIGMA_COLOR_SELECTION (selection), &color);

      g_signal_handlers_block_by_func (priv->color_area,
                                       ligma_color_button_area_changed,
                                       button);

      ligma_color_area_set_color (LIGMA_COLOR_AREA (priv->color_area), &color);

      g_signal_handlers_unblock_by_func (priv->color_area,
                                         ligma_color_button_area_changed,
                                         button);

      g_signal_emit (button, ligma_color_button_signals[COLOR_CHANGED], 0);
    }
}

static void
ligma_color_button_help_func (const gchar *help_id,
                             gpointer     help_data)
{
  LigmaColorButton        *button;
  LigmaColorButtonPrivate *priv;
  LigmaColorSelection     *selection;
  LigmaColorNotebook      *notebook;
  LigmaColorSelector      *current;

  button = g_object_get_data (G_OBJECT (help_data), COLOR_BUTTON_KEY);
  priv   = GET_PRIVATE (button);

  selection = LIGMA_COLOR_SELECTION (priv->selection);
  notebook = LIGMA_COLOR_NOTEBOOK (ligma_color_selection_get_notebook (selection));

  current = ligma_color_notebook_get_current_selector (notebook);

  help_id = LIGMA_COLOR_SELECTOR_GET_CLASS (current)->help_id;

  ligma_standard_help_func (help_id, NULL);
}
