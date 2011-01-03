/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolorbutton.c
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
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "gimpwidgetstypes.h"

#include "gimpcolorarea.h"
#include "gimpcolorbutton.h"
#include "gimpcolornotebook.h"
#include "gimpcolorselection.h"
#include "gimpdialog.h"
#include "gimphelpui.h"
#include "gimpicons.h"
#include "gimpwidgets-private.h"
#include "gimp3migration.h"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimpcolorbutton
 * @title: GimpColorButton
 * @short_description: Widget for selecting a color from a simple button.
 * @see_also: #libgimpcolor-gimpcolorspace
 *
 * This widget provides a simple button with a preview showing the
 * color.
 *
 * On click a color selection dialog is opened. Additionally the
 * button supports Drag and Drop and has a right-click menu that
 * allows one to choose the color from the current FG or BG color. If
 * the user changes the color, the "color-changed" signal is emitted.
 **/


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

enum
{
  PROP_0,
  PROP_TITLE,
  PROP_COLOR,
  PROP_TYPE,
  PROP_UPDATE,
  PROP_AREA_WIDTH,
  PROP_AREA_HEIGHT
};


typedef struct _GimpColorButtonPrivate GimpColorButtonPrivate;

struct _GimpColorButtonPrivate
{
  gchar        *title;
  gboolean      continuous_update;

  GtkWidget    *color_area;
  GtkWidget    *dialog;

  GtkUIManager *ui_manager;
};

#define GET_PRIVATE(obj) G_TYPE_INSTANCE_GET_PRIVATE (obj, \
                                                      GIMP_TYPE_COLOR_BUTTON, \
                                                      GimpColorButtonPrivate)


static void     gimp_color_button_class_init     (GimpColorButtonClass *klass);
static void     gimp_color_button_init           (GimpColorButton      *button,
                                                  GimpColorButtonClass *klass);

static void     gimp_color_button_finalize            (GObject         *object);
static void     gimp_color_button_dispose             (GObject         *object);
static void     gimp_color_button_get_property        (GObject         *object,
                                                       guint            property_id,
                                                       GValue          *value,
                                                       GParamSpec      *pspec);
static void     gimp_color_button_set_property        (GObject         *object,
                                                       guint            property_id,
                                                       const GValue    *value,
                                                       GParamSpec      *pspec);

static gboolean gimp_color_button_button_press        (GtkWidget       *widget,
                                                       GdkEventButton  *bevent);
static void     gimp_color_button_state_flags_changed (GtkWidget       *widget,
                                                       GtkStateFlags    prev_state);
static void     gimp_color_button_clicked             (GtkButton       *button);
static GType    gimp_color_button_get_action_type     (GimpColorButton *button);

static void     gimp_color_button_dialog_response     (GtkWidget       *dialog,
                                                       gint             response_id,
                                                       GimpColorButton *button);
static void     gimp_color_button_use_color           (GtkAction       *action,
                                                       GimpColorButton *button);
static void     gimp_color_button_area_changed        (GtkWidget       *color_area,
                                                       GimpColorButton *button);
static void     gimp_color_button_selection_changed   (GtkWidget       *selection,
                                                       GimpColorButton *button);
static void     gimp_color_button_help_func           (const gchar     *help_id,
                                                       gpointer         help_data);


static const GtkActionEntry actions[] =
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

  if (! button_type)
    {
      const GTypeInfo button_info =
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
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkButtonClass *button_class = GTK_BUTTON_CLASS (klass);
  GimpRGB         color;

  parent_class = g_type_class_peek_parent (klass);

  gimp_color_button_signals[COLOR_CHANGED] =
    g_signal_new ("color-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColorButtonClass, color_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->finalize            = gimp_color_button_finalize;
  object_class->dispose             = gimp_color_button_dispose;
  object_class->get_property        = gimp_color_button_get_property;
  object_class->set_property        = gimp_color_button_set_property;

  widget_class->button_press_event  = gimp_color_button_button_press;
  widget_class->state_flags_changed = gimp_color_button_state_flags_changed;

  button_class->clicked             = gimp_color_button_clicked;

  klass->color_changed              = NULL;
  klass->get_action_type            = gimp_color_button_get_action_type;

  gimp_rgba_set (&color, 0.0, 0.0, 0.0, 1.0);

  /**
   * GimpColorButton:title:
   *
   * The title to be used for the color selection dialog.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_TITLE,
                                   g_param_spec_string ("title", NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
  /**
   * GimpColorButton:color:
   *
   * The color displayed in the button's color area.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_COLOR,
                                   gimp_param_spec_rgb ("color", NULL, NULL,
                                                        TRUE, &color,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
  /**
   * GimpColorButton:type:
   *
   * The type of the button's color area.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_TYPE,
                                   g_param_spec_enum ("type", NULL, NULL,
                                                      GIMP_TYPE_COLOR_AREA_TYPE,
                                                      GIMP_COLOR_AREA_FLAT,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));
  /**
   * GimpColorButton:continuous-update:
   *
   * The update policy of the color button.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_UPDATE,
                                   g_param_spec_boolean ("continuous-update",
                                                         NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  /**
   * GimpColorButton:area-width:
   *
   * The minimum width of the button's #GimpColorArea.
   *
   * Since: 2.8
   */
  g_object_class_install_property (object_class, PROP_AREA_WIDTH,
                                   g_param_spec_int ("area-width",
                                                     NULL, NULL,
                                                     1, G_MAXINT, 16,
                                                     G_PARAM_WRITABLE |
                                                     G_PARAM_CONSTRUCT));

  /**
   * GimpColorButton:area-height:
   *
   * The minimum height of the button's #GimpColorArea.
   *
   * Since: 2.8
   */
  g_object_class_install_property (object_class, PROP_AREA_HEIGHT,
                                   g_param_spec_int ("area-height",
                                                     NULL, NULL,
                                                     1, G_MAXINT, 16,
                                                     G_PARAM_WRITABLE |
                                                     G_PARAM_CONSTRUCT));

  g_type_class_add_private (object_class, sizeof (GimpColorButtonPrivate));
}

static void
gimp_color_button_init (GimpColorButton      *button,
                        GimpColorButtonClass *klass)
{
  GimpColorButtonPrivate *priv = GET_PRIVATE (button);
  GtkActionGroup         *group;
  gint                    i;

  priv->title  = NULL;
  priv->dialog = NULL;

  priv->color_area = g_object_new (GIMP_TYPE_COLOR_AREA,
                                   "drag-mask", GDK_BUTTON1_MASK,
                                   NULL);

  g_signal_connect (priv->color_area, "color-changed",
                    G_CALLBACK (gimp_color_button_area_changed),
                    button);

  gtk_container_add (GTK_CONTAINER (button), priv->color_area);
  gtk_widget_show (priv->color_area);

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
gimp_color_button_finalize (GObject *object)
{
  GimpColorButtonPrivate *priv = GET_PRIVATE (object);

  if (priv->title)
    {
      g_free (priv->title);
      priv->title = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_color_button_dispose (GObject *object)
{
  GimpColorButtonPrivate *priv = GET_PRIVATE (object);

  if (priv->dialog)
    {
      gtk_widget_destroy (priv->dialog);
      priv->dialog = NULL;
    }

  if (priv->color_area)
    {
      gtk_widget_destroy (priv->color_area);
      priv->color_area = NULL;
    }

  if (priv->ui_manager)
    {
      g_object_unref (priv->ui_manager);
      priv->ui_manager = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_color_button_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpColorButtonPrivate *priv = GET_PRIVATE (object);

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

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_color_button_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpColorButton        *button = GIMP_COLOR_BUTTON (object);
  GimpColorButtonPrivate *priv   = GET_PRIVATE (object);
  gint                    other;

  switch (property_id)
    {
    case PROP_TITLE:
      gimp_color_button_set_title (button, g_value_get_string (value));
      break;

    case PROP_COLOR:
      g_object_set_property (G_OBJECT (priv->color_area), "color", value);
      break;

    case PROP_TYPE:
      g_object_set_property (G_OBJECT (priv->color_area), "type", value);
      break;

    case PROP_UPDATE:
      gimp_color_button_set_update (button, g_value_get_boolean (value));
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

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_color_button_button_press (GtkWidget      *widget,
                                GdkEventButton *bevent)
{
  GimpColorButtonPrivate *priv = GET_PRIVATE (widget);

  if (gdk_event_triggers_context_menu ((GdkEvent *) bevent))
    {
      GtkWidget *menu = gtk_ui_manager_get_widget (priv->ui_manager,
                                                   "/color-button-popup");

      gtk_menu_set_screen (GTK_MENU (menu), gtk_widget_get_screen (widget));

      gtk_menu_popup (GTK_MENU (menu),
                      NULL, NULL, NULL, NULL,
                      bevent->button, bevent->time);
    }

  return GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, bevent);
}

static void
gimp_color_button_state_flags_changed (GtkWidget     *widget,
                                       GtkStateFlags  previous_state)
{
  GimpColorButtonPrivate *priv = GET_PRIVATE (widget);

  if (! gtk_widget_is_sensitive (widget) && priv->dialog)
    gtk_widget_hide (priv->dialog);

  if (GTK_WIDGET_CLASS (parent_class)->state_flags_changed)
    GTK_WIDGET_CLASS (parent_class)->state_flags_changed (widget,
                                                          previous_state);
}

static void
gimp_color_button_clicked (GtkButton *button)
{
  GimpColorButton        *color_button = GIMP_COLOR_BUTTON (button);
  GimpColorButtonPrivate *priv      = GET_PRIVATE (button);
  GtkWidget              *selection;
  GimpRGB                 color;

  if (! priv->dialog)
    {
      GtkWidget *dialog;

      dialog = priv->dialog =
        gimp_dialog_new (priv->title, COLOR_SELECTION_KEY,
                         gtk_widget_get_toplevel (GTK_WIDGET (button)), 0,
                         gimp_color_button_help_func, NULL,

                         _("_Reset"),  RESPONSE_RESET,
                         _("_Cancel"), GTK_RESPONSE_CANCEL,
                         _("_OK"),     GTK_RESPONSE_OK,

                         NULL);

      gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               RESPONSE_RESET,
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (gimp_color_button_dialog_response),
                        color_button);
      g_signal_connect (dialog, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &priv->dialog);

      selection = gimp_color_selection_new ();
      gtk_container_set_border_width (GTK_CONTAINER (selection), 6);
      gimp_color_selection_set_show_alpha (GIMP_COLOR_SELECTION (selection),
                                           gimp_color_button_has_alpha (color_button));
      gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                          selection, TRUE, TRUE, 0);
      gtk_widget_show (selection);

      g_signal_connect (selection, "color-changed",
                        G_CALLBACK (gimp_color_button_selection_changed),
                        button);

      g_object_set_data (G_OBJECT (priv->dialog), COLOR_SELECTION_KEY,
                         selection);
    }
  else
    {
      selection = g_object_get_data (G_OBJECT (priv->dialog),
                                     COLOR_SELECTION_KEY);
    }

  gimp_color_button_get_color (color_button, &color);

  g_signal_handlers_block_by_func (selection,
                                   gimp_color_button_selection_changed,
                                   button);

  gimp_color_selection_set_color (GIMP_COLOR_SELECTION (selection), &color);
  gimp_color_selection_set_old_color (GIMP_COLOR_SELECTION (selection),
                                      &color);

  g_signal_handlers_unblock_by_func (selection,
                                     gimp_color_button_selection_changed,
                                     button);

  gtk_window_present (GTK_WINDOW (priv->dialog));
}

static GType
gimp_color_button_get_action_type (GimpColorButton *button)
{
  return GTK_TYPE_ACTION;
}


/*  public functions  */

/**
 * gimp_color_button_new:
 * @title:  String that will be used as title for the color_selector.
 * @width:  Width of the colorpreview in pixels.
 * @height: Height of the colorpreview in pixels.
 * @color:  A pointer to a #GimpRGB color.
 * @type:   The type of transparency to be displayed.
 *
 * Creates a new #GimpColorButton widget.
 *
 * This returns a button with a preview showing the color.
 * When the button is clicked a GtkColorSelectionDialog is opened.
 * If the user changes the color the new color is written into the
 * array that was used to pass the initial color and the "color-changed"
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
  g_return_val_if_fail (color != NULL, NULL);
  g_return_val_if_fail (width > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);

  return g_object_new (GIMP_TYPE_COLOR_BUTTON,
                       "title",       title,
                       "type",        type,
                       "color",       color,
                       "area-width",  width,
                       "area-height", height,
                       NULL);
}

/**
 * gimp_color_button_set_title:
 * @button: a #GimpColorButton.
 * @color:  the new title.
 *
 * Sets the @button dialog's title.
 *
 * Since: GIMP 2.10
 **/
void
gimp_color_button_set_title (GimpColorButton *button,
                             const gchar     *title)
{
  GimpColorButtonPrivate *priv;

  g_return_if_fail (GIMP_IS_COLOR_BUTTON (button));
  g_return_if_fail (title != NULL);

  priv = GET_PRIVATE (button);

  g_free (priv->title);
  priv->title = g_strdup (title);

  if (priv->dialog)
    gtk_window_set_title (GTK_WINDOW (priv->dialog), title);

  g_object_notify (G_OBJECT (button), "title");
}

/**
 * gimp_color_button_get_title:
 * @button: a #GimpColorButton.
 *
 * Returns: The @button dialog's title.
 *
 * Since: GIMP 2.10
 **/
const gchar *
gimp_color_button_get_title (GimpColorButton *button)
{
  GimpColorButtonPrivate *priv;

  g_return_val_if_fail (GIMP_IS_COLOR_BUTTON (button), NULL);

  priv = GET_PRIVATE (button);

  return priv->title;
}

/**
 * gimp_color_button_set_color:
 * @button: Pointer to a #GimpColorButton.
 * @color:  Pointer to the new #GimpRGB color.
 *
 * Sets the @button to the given @color.
 **/
void
gimp_color_button_set_color (GimpColorButton *button,
                             const GimpRGB   *color)
{
  GimpColorButtonPrivate *priv;

  g_return_if_fail (GIMP_IS_COLOR_BUTTON (button));
  g_return_if_fail (color != NULL);

  priv = GET_PRIVATE (button);

  gimp_color_area_set_color (GIMP_COLOR_AREA (priv->color_area), color);

  g_object_notify (G_OBJECT (button), "color");
}

/**
 * gimp_color_button_get_color:
 * @button: Pointer to a #GimpColorButton.
 * @color:  Pointer to a #GimpRGB struct used to return the color.
 *
 * Retrieves the currently set color from the @button.
 **/
void
gimp_color_button_get_color (GimpColorButton *button,
                             GimpRGB         *color)
{
  GimpColorButtonPrivate *priv;

  g_return_if_fail (GIMP_IS_COLOR_BUTTON (button));
  g_return_if_fail (color != NULL);

  priv = GET_PRIVATE (button);

  gimp_color_area_get_color (GIMP_COLOR_AREA (priv->color_area), color);
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
  GimpColorButtonPrivate *priv;

  g_return_val_if_fail (GIMP_IS_COLOR_BUTTON (button), FALSE);

  priv = GET_PRIVATE (button);

  return gimp_color_area_has_alpha (GIMP_COLOR_AREA (priv->color_area));
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
  GimpColorButtonPrivate *priv;

  g_return_if_fail (GIMP_IS_COLOR_BUTTON (button));

  priv = GET_PRIVATE (button);

  gimp_color_area_set_type (GIMP_COLOR_AREA (priv->color_area), type);

  g_object_notify (G_OBJECT (button), "type");
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
  GimpColorButtonPrivate *priv;

  g_return_val_if_fail (GIMP_IS_COLOR_BUTTON (button), FALSE);

  priv = GET_PRIVATE (button);

  return priv->continuous_update;
}

/**
 * gimp_color_button_set_update:
 * @button:     A #GimpColorButton widget.
 * @continuous: The new setting of the @continuous_update property.
 *
 * When set to #TRUE, the @button will emit the "color-changed"
 * continuously while the color is changed in the color selection
 * dialog.
 **/
void
gimp_color_button_set_update (GimpColorButton *button,
                              gboolean         continuous)
{
  GimpColorButtonPrivate *priv;

  g_return_if_fail (GIMP_IS_COLOR_BUTTON (button));

  priv = GET_PRIVATE (button);

  if (continuous != priv->continuous_update)
    {
      priv->continuous_update = continuous ? TRUE : FALSE;

      if (priv->dialog)
        {
          GimpColorSelection *selection;
          GimpRGB             color;

          selection = g_object_get_data (G_OBJECT (priv->dialog),
                                         COLOR_SELECTION_KEY);

          if (priv->continuous_update)
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

      g_object_notify (G_OBJECT (button), "continuous-update");
    }
}

/**
 * gimp_color_button_set_color_config:
 * @button: a #GimpColorButton widget.
 * @config: a #GimpColorConfig object.
 *
 * Sets the color management configuration to use with this color button's
 * #GimpColorArea.
 *
 * Since: 2.10
 */
void
gimp_color_button_set_color_config (GimpColorButton *button,
                                    GimpColorConfig *config)
{
  GimpColorButtonPrivate *priv;

  g_return_if_fail (GIMP_IS_COLOR_BUTTON (button));
  g_return_if_fail (config == NULL || GIMP_IS_COLOR_CONFIG (config));

  priv = GET_PRIVATE (button);

  gimp_color_area_set_color_config (GIMP_COLOR_AREA (priv->color_area),
                                    config);
}

/**
 * gimp_color_button_get_ui_manager:
 * @button: a #GimpColorButton.
 *
 * Returns: The @button's #GtkUIManager.
 *
 * Since: GIMP 2.10
 * Since: GIMP 3.0
 **/
GtkUIManager *
gimp_color_button_get_ui_manager (GimpColorButton *button)
{
  GimpColorButtonPrivate *priv;

  g_return_val_if_fail (GIMP_IS_COLOR_BUTTON (button), NULL);

  priv = GET_PRIVATE (button);

  return priv->ui_manager;
}


/*  private functions  */

static void
gimp_color_button_dialog_response (GtkWidget       *dialog,
                                   gint             response_id,
                                   GimpColorButton *button)
{
  GimpColorButtonPrivate *priv = GET_PRIVATE (button);
  GtkWidget              *selection;
  GimpRGB                 color;

  selection = g_object_get_data (G_OBJECT (dialog), COLOR_SELECTION_KEY);

  switch (response_id)
    {
    case RESPONSE_RESET:
      gimp_color_selection_reset (GIMP_COLOR_SELECTION (selection));
      break;

    case GTK_RESPONSE_OK:
      if (! priv->continuous_update)
        {
          gimp_color_selection_get_color (GIMP_COLOR_SELECTION (selection),
                                          &color);
          gimp_color_button_set_color (button, &color);
        }

      gtk_widget_hide (dialog);
      break;

    default:
      if (priv->continuous_update)
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
  GimpColorButtonPrivate *priv = GET_PRIVATE (button);

  if (priv->dialog)
    {
      GimpColorSelection *selection;
      GimpRGB             color;

      selection = g_object_get_data (G_OBJECT (priv->dialog),
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
  GimpColorButtonPrivate *priv = GET_PRIVATE (button);

  if (priv->continuous_update)
    {
      GimpRGB color;

      gimp_color_selection_get_color (GIMP_COLOR_SELECTION (selection), &color);

      g_signal_handlers_block_by_func (priv->color_area,
                                       gimp_color_button_area_changed,
                                       button);

      gimp_color_area_set_color (GIMP_COLOR_AREA (priv->color_area), &color);

      g_signal_handlers_unblock_by_func (priv->color_area,
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
  GimpColorSelector  *current;

  selection = g_object_get_data (G_OBJECT (help_data), COLOR_SELECTION_KEY);

  notebook = GIMP_COLOR_NOTEBOOK (gimp_color_selection_get_notebook (selection));

  current = gimp_color_notebook_get_current_selector (notebook);

  help_id = GIMP_COLOR_SELECTOR_GET_CLASS (current)->help_id;

  gimp_standard_help_func (help_id, NULL);
}
