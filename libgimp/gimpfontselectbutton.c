/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpfontselectbutton.c
 * Copyright (C) 2003  Sven Neumann  <sven@gimp.org>
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

#include "gimp.h"
#include "gimpui.h"
#include "gimpuimarshal.h"

#include "libgimp-intl.h"


#define GIMP_FONT_SELECT_BUTTON_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIMP_TYPE_FONT_SELECT_BUTTON, GimpFontSelectButtonPrivate))

struct _GimpFontSelectButtonPrivate
{
  gchar               *title;
  gchar               *font_name;      /* Local copy */

  GtkWidget           *inside;
  GtkWidget           *label;

  const gchar         *temp_font_callback;
};

enum
{
  FONT_SET,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_TITLE,
  PROP_FONT_NAME
};


/*  local function prototypes  */

static void   gimp_font_select_button_set_property (GObject      *object,
                                                    guint         property_id,
                                                    const GValue *value,
                                                    GParamSpec   *pspec);
static void   gimp_font_select_button_get_property (GObject      *object,
                                                    guint         property_id,
                                                    GValue       *value,
                                                    GParamSpec   *pspec);

static void   gimp_font_select_button_destroy  (GtkObject   *object);
static void   gimp_font_select_button_clicked  (GtkButton   *button);
static void   gimp_font_select_button_callback (const gchar *name,
                                                gboolean     closing,
                                                gpointer     data);

static void   gimp_font_select_drag_data_received (GtkWidget        *widget,
                                                   GdkDragContext   *context,
                                                   gint              x,
                                                   gint              y,
                                                   GtkSelectionData *selection,
                                                   guint             info,
                                                   guint             time);

static GtkWidget * gimp_font_select_button_create_inside (GimpFontSelectButton *button);


static const GtkTargetEntry target = { "application/x-gimp-font-name", 0 };

static guint font_button_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE(GimpFontSelectButton,
              gimp_font_select_button,
              GTK_TYPE_BUTTON);

static void
gimp_font_select_button_class_init (GimpFontSelectButtonClass *klass)
{
  GObjectClass   *gobject_class = G_OBJECT_CLASS (klass);
  GtkObjectClass *object_class  = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class  = GTK_WIDGET_CLASS (klass);
  GtkButtonClass *button_class  = GTK_BUTTON_CLASS (klass);

  gobject_class->set_property = gimp_font_select_button_set_property;
  gobject_class->get_property = gimp_font_select_button_get_property;

  object_class->destroy = gimp_font_select_button_destroy;

  widget_class->drag_data_received = gimp_font_select_drag_data_received;

  button_class->clicked = gimp_font_select_button_clicked;

  klass->font_set = NULL;

  /**
   * GimpFontSelectButton:title:
   *
   * The title to be used for the font selection dialog.
   *
   * Since: GIMP 2.4
   */
  g_object_class_install_property (gobject_class, PROP_TITLE,
                                   g_param_spec_string ("title", NULL, NULL,
                                                        _("Font Selection"),
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  /**
   * GimpFontSelectButton:font-name:
   *
   * The name of the currently selected font.
   *
   * Since: 2.4
   */
  g_object_class_install_property (gobject_class, PROP_FONT_NAME,
                                   g_param_spec_string ("font-name", NULL, NULL,
                                                        _("Sans"),
                                                        G_PARAM_READWRITE));

  /**
   * GimpFontSelectButton::font-set:
   * @widget: the object which received the signal.
   * @font_name: the name of the currently selected font.
   * @dialog_closing: whether the dialog was closed or not.
   *
   * The ::font-set signal is emitted when the user selects a font.
   *
   * Since: 2.4
   */
  font_button_signals[FONT_SET] =
    g_signal_new ("font-set",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpFontSelectButtonClass, font_set),
                  NULL, NULL,
                  _gimpui_marshal_VOID__STRING_BOOLEAN,
                  G_TYPE_NONE, 2,
                  G_TYPE_STRING,
                  G_TYPE_BOOLEAN);

  g_type_class_add_private (object_class, sizeof (GimpFontSelectButtonPrivate));
}

static void
gimp_font_select_button_init (GimpFontSelectButton *button)
{
  button->priv = GIMP_FONT_SELECT_BUTTON_GET_PRIVATE (button);

  button->priv->title = g_strdup(_("Font Selection"));
  button->priv->font_name = g_strdup(_("Sans"));
  button->priv->temp_font_callback = NULL;

  button->priv->inside = gimp_font_select_button_create_inside (button);
  gtk_container_add (GTK_CONTAINER (button), button->priv->inside);

  gtk_drag_dest_set (GTK_WIDGET (button),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     &target, 1,
                     GDK_ACTION_COPY);
}

/**
 * gimp_font_select_button_new:
 * @title:     Title of the dialog to use or %NULL means to use the default
 *             title.
 * @font_name: Initial font name.
 *
 * Creates a new #GtkWidget that completely controls the selection of
 * a font.  This widget is suitable for placement in a table in a
 * plug-in dialog.
 *
 * Returns: A #GtkWidget that you can use in your UI.
 */
GtkWidget *
gimp_font_select_button_new (const gchar *title,
                             const gchar *font_name)
{
  GtkWidget *button;

  if (title)
    button = g_object_new (GIMP_TYPE_FONT_SELECT_BUTTON,
                           "title",     title,
                           "font-name", font_name,
                           NULL);
  else
    button = g_object_new (GIMP_TYPE_FONT_SELECT_BUTTON,
                           "font-name", font_name,
                           NULL);

  return button;
}

/**
 * gimp_font_select_button_close_popup:
 * @button: A #GimpFontSelectButton
 *
 * Closes the popup window associated with @button.
 */
void
gimp_font_select_button_close_popup (GimpFontSelectButton *button)
{
  g_return_if_fail (GIMP_IS_FONT_SELECT_BUTTON (button));

  if (button->priv->temp_font_callback)
    {
      gimp_font_select_destroy (button->priv->temp_font_callback);
      button->priv->temp_font_callback = NULL;
    }
}

/**
 * gimp_font_select_button_get_font_name:
 * @button: A #GimpFontSelectButton
 *
 * Retrieves the name of currently selected font.
 *
 * Returns: an internal copy of the font name which must not be freed.
 *
 * Since: 2.4
 */
G_CONST_RETURN gchar *
gimp_font_select_button_get_font_name (GimpFontSelectButton *button)
{
  g_return_val_if_fail (GIMP_IS_FONT_SELECT_BUTTON (button), NULL);

  return button->priv->font_name;
}

/**
 * gimp_font_select_button_set_font_name:
 * @button: A #GimpFontSelectButton
 * @font_name: Font name to set; %NULL means no change.
 *
 * Sets the current font for the font select button.
 *
 * Since: 2.4
 */
void
gimp_font_select_button_set_font_name (GimpFontSelectButton *button,
                                       const gchar          *font_name)
{
  g_return_if_fail (GIMP_IS_FONT_SELECT_BUTTON (button));

  if (button->priv->temp_font_callback)
    gimp_fonts_set_popup (button->priv->temp_font_callback, font_name);
  else
    gimp_font_select_button_callback (font_name, FALSE, button);
}


/*  private functions  */

static void
gimp_font_select_button_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  GimpFontSelectButton *button = GIMP_FONT_SELECT_BUTTON (object);

  switch (property_id)
    {
    case PROP_TITLE:
      g_free (button->priv->title);
      button->priv->title = g_value_dup_string (value);
      g_object_notify (object, "title");
      break;

    case PROP_FONT_NAME:
      gimp_font_select_button_set_font_name (button,
                                             g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_font_select_button_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  GimpFontSelectButton *button = GIMP_FONT_SELECT_BUTTON (object);

  switch (property_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, button->priv->title);
      break;

    case PROP_FONT_NAME:
      g_value_set_string (value, button->priv->font_name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_font_select_button_callback (const gchar *name,
                                  gboolean     closing,
                                  gpointer     data)
{
  GimpFontSelectButton *button = GIMP_FONT_SELECT_BUTTON (data);

  g_free (button->priv->font_name);
  button->priv->font_name = g_strdup (name);

  gtk_label_set_text (GTK_LABEL (button->priv->label), name);

  if (closing)
    button->priv->temp_font_callback = NULL;

  g_signal_emit (button, font_button_signals[FONT_SET], 0, name, closing);
  g_object_notify (G_OBJECT (button), "font-name");
}

static void
gimp_font_select_button_clicked (GtkButton *button)
{
  GimpFontSelectButton *font_button = GIMP_FONT_SELECT_BUTTON (button);

  if (font_button->priv->temp_font_callback)
    {
      /*  calling gimp_fonts_set_popup() raises the dialog  */
      gimp_fonts_set_popup (font_button->priv->temp_font_callback,
                            font_button->priv->font_name);
    }
  else
    {
      font_button->priv->temp_font_callback =
        gimp_font_select_new (font_button->priv->title,
                              font_button->priv->font_name,
                              gimp_font_select_button_callback,
                              button);
    }
}

static void
gimp_font_select_button_destroy (GtkObject *object)
{
  GimpFontSelectButton *button = GIMP_FONT_SELECT_BUTTON (object);

  if (button->priv->temp_font_callback)
    {
      gimp_font_select_destroy (button->priv->temp_font_callback);
      button->priv->temp_font_callback = NULL;
    }

  g_free (button->priv->title);
  button->priv->title = NULL;

  g_free (button->priv->font_name);
  button->priv->font_name = NULL;

  GTK_OBJECT_CLASS (gimp_font_select_button_parent_class)->destroy (object);
}

static void
gimp_font_select_drag_data_received (GtkWidget        *widget,
                                     GdkDragContext   *context,
                                     gint              x,
                                     gint              y,
                                     GtkSelectionData *selection,
                                     guint             info,
                                     guint             time)
{
  gchar *str;

  if ((selection->format != 8) || (selection->length < 1))
    {
      g_warning ("Received invalid font data!");
      return;
    }

  str = g_strndup ((const gchar *) selection->data, selection->length);

  if (g_utf8_validate (str, -1, NULL))
    {
      gint     pid;
      gpointer unused;
      gint     name_offset = 0;

      if (sscanf (str, "%i:%p:%n", &pid, &unused, &name_offset) >= 2 &&
          pid == gimp_getpid () && name_offset > 0)
        {
          gchar *name = str + name_offset;

          gimp_font_select_button_set_font_name (GIMP_FONT_SELECT_BUTTON (widget),
                                                 name);
        }
    }

  g_free (str);
}

static GtkWidget *
gimp_font_select_button_create_inside (GimpFontSelectButton *button)
{
  GtkWidget *hbox;
  GtkWidget *image;

  gtk_widget_push_composite_child ();

  hbox = gtk_hbox_new (FALSE, 4);

  image = gtk_image_new_from_stock (GIMP_STOCK_FONT, GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

  button->priv->label = gtk_label_new (button->priv->font_name);
  gtk_box_pack_start (GTK_BOX (hbox), button->priv->label, TRUE, TRUE, 4);

  gtk_widget_show_all (hbox);

  gtk_widget_pop_composite_child ();

  return hbox;
}
