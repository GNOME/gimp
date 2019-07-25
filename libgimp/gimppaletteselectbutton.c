/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppaletteselectbutton.c
 * Copyright (C) 2004  Michael Natterer <mitch@gimp.org>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gimp.h"

#include "gimpuitypes.h"
#include "gimppaletteselectbutton.h"
#include "gimpuimarshal.h"

#include "libgimp-intl.h"


/**
 * SECTION: gimppaletteselectbutton
 * @title: GimpPaletteSelect
 * @short_description: A button which pops up a palette select dialog.
 *
 * A button which pops up a palette select dialog.
 **/


#define GIMP_PALETTE_SELECT_BUTTON_GET_PRIVATE(obj) ((GimpPaletteSelectButtonPrivate *) gimp_palette_select_button_get_instance_private ((GimpPaletteSelectButton *) (obj)))

typedef struct _GimpPaletteSelectButtonPrivate GimpPaletteSelectButtonPrivate;

struct _GimpPaletteSelectButtonPrivate
{
  gchar     *title;

  gchar     *palette_name;      /* Local copy */

  GtkWidget *inside;
  GtkWidget *label;
};

enum
{
  PALETTE_SET,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_TITLE,
  PROP_PALETTE_NAME
};


/*  local function prototypes  */

static void   gimp_palette_select_button_finalize     (GObject      *object);

static void   gimp_palette_select_button_set_property (GObject      *object,
                                                       guint         property_id,
                                                       const GValue *value,
                                                       GParamSpec   *pspec);
static void   gimp_palette_select_button_get_property (GObject      *object,
                                                       guint         property_id,
                                                       GValue       *value,
                                                       GParamSpec   *pspec);

static void   gimp_palette_select_button_clicked  (GimpPaletteSelectButton *button);

static void   gimp_palette_select_button_callback (const gchar *palette_name,
                                                   gboolean     dialog_closing,
                                                   gpointer     user_data);

static void   gimp_palette_select_drag_data_received (GimpPaletteSelectButton *button,
                                                      GdkDragContext          *context,
                                                      gint                     x,
                                                      gint                     y,
                                                      GtkSelectionData        *selection,
                                                      guint                    info,
                                                      guint                    time);

static GtkWidget * gimp_palette_select_button_create_inside (GimpPaletteSelectButton *palette_button);


static const GtkTargetEntry target = { "application/x-gimp-palette-name", 0 };

static guint palette_button_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE_WITH_PRIVATE (GimpPaletteSelectButton, gimp_palette_select_button,
                            GIMP_TYPE_SELECT_BUTTON)


static void
gimp_palette_select_button_class_init (GimpPaletteSelectButtonClass *klass)
{
  GObjectClass          *object_class        = G_OBJECT_CLASS (klass);
  GimpSelectButtonClass *select_button_class = GIMP_SELECT_BUTTON_CLASS (klass);

  object_class->finalize     = gimp_palette_select_button_finalize;
  object_class->set_property = gimp_palette_select_button_set_property;
  object_class->get_property = gimp_palette_select_button_get_property;

  select_button_class->select_destroy = gimp_palette_select_destroy;

  klass->palette_set = NULL;

  /**
   * GimpPaletteSelectButton:title:
   *
   * The title to be used for the palette selection popup dialog.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_TITLE,
                                   g_param_spec_string ("title",
                                                        "Title",
                                                        "The title to be used for the palette selection popup dialog",
                                                        _("Palette Selection"),
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  /**
   * GimpPaletteSelectButton:palette-name:
   *
   * The name of the currently selected palette.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_PALETTE_NAME,
                                   g_param_spec_string ("palette-name",
                                                        "Palette name",
                                                        "The name of the currently selected palette",
                                                        NULL,
                                                        GIMP_PARAM_READWRITE));

  /**
   * GimpPaletteSelectButton::palette-set:
   * @widget: the object which received the signal.
   * @palette_name: the name of the currently selected palette.
   * @dialog_closing: whether the dialog was closed or not.
   *
   * The ::palette-set signal is emitted when the user selects a palette.
   *
   * Since: 2.4
   */
  palette_button_signals[PALETTE_SET] =
    g_signal_new ("palette-set",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpPaletteSelectButtonClass, palette_set),
                  NULL, NULL,
                  _gimpui_marshal_VOID__STRING_BOOLEAN,
                  G_TYPE_NONE, 2,
                  G_TYPE_STRING,
                  G_TYPE_BOOLEAN);
}

static void
gimp_palette_select_button_init (GimpPaletteSelectButton *button)
{
  GimpPaletteSelectButtonPrivate *priv;

  priv = GIMP_PALETTE_SELECT_BUTTON_GET_PRIVATE (button);

  priv->palette_name = NULL;

  priv->inside = gimp_palette_select_button_create_inside (button);
  gtk_container_add (GTK_CONTAINER (button), priv->inside);
}

/**
 * gimp_palette_select_button_new:
 * @title:        Title of the dialog to use or %NULL to use the default title.
 * @palette_name: Initial palette name.
 *
 * Creates a new #GtkWidget that completely controls the selection of
 * a palette.  This widget is suitable for placement in a table in a
 * plug-in dialog.
 *
 * Returns: A #GtkWidget that you can use in your UI.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_palette_select_button_new (const gchar *title,
                                const gchar *palette_name)
{
  GtkWidget *button;

  if (title)
    button = g_object_new (GIMP_TYPE_PALETTE_SELECT_BUTTON,
                           "title",        title,
                           "palette-name", palette_name,
                           NULL);
  else
    button = g_object_new (GIMP_TYPE_PALETTE_SELECT_BUTTON,
                           "palette-name", palette_name,
                           NULL);

  return button;
}

/**
 * gimp_palette_select_button_get_palette:
 * @button: A #GimpPaletteSelectButton
 *
 * Retrieves the name of currently selected palette.
 *
 * Returns: an internal copy of the palette name which must not be freed.
 *
 * Since: 2.4
 */
const gchar *
gimp_palette_select_button_get_palette (GimpPaletteSelectButton *button)
{
  GimpPaletteSelectButtonPrivate *priv;

  g_return_val_if_fail (GIMP_IS_PALETTE_SELECT_BUTTON (button), NULL);

  priv = GIMP_PALETTE_SELECT_BUTTON_GET_PRIVATE (button);
  return priv->palette_name;
}

/**
 * gimp_palette_select_button_set_palette:
 * @button: A #GimpPaletteSelectButton
 * @palette_name: Palette name to set; %NULL means no change.
 *
 * Sets the current palette for the palette select button.
 *
 * Since: 2.4
 */
void
gimp_palette_select_button_set_palette (GimpPaletteSelectButton *button,
                                        const gchar             *palette_name)
{
  GimpSelectButton *select_button;

  g_return_if_fail (GIMP_IS_PALETTE_SELECT_BUTTON (button));

  select_button = GIMP_SELECT_BUTTON (button);

  if (select_button->temp_callback)
    {
      gimp_palettes_set_popup (select_button->temp_callback, palette_name);
    }
  else
    {
      gchar *name;
      gint   num_colors;

      if (palette_name && *palette_name)
        name = g_strdup (palette_name);
      else
        name = gimp_context_get_palette ();

      if (gimp_palette_get_info (name, &num_colors))
        gimp_palette_select_button_callback (name, FALSE, button);

      g_free (name);
    }
}


/*  private functions  */

static void
gimp_palette_select_button_finalize (GObject *object)
{
  GimpPaletteSelectButtonPrivate *priv;

  priv = GIMP_PALETTE_SELECT_BUTTON_GET_PRIVATE (object);

  g_clear_pointer (&priv->palette_name, g_free);
  g_clear_pointer (&priv->title,        g_free);

  G_OBJECT_CLASS (gimp_palette_select_button_parent_class)->finalize (object);
}

static void
gimp_palette_select_button_set_property (GObject      *object,
                                         guint         property_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  GimpPaletteSelectButton        *button;
  GimpPaletteSelectButtonPrivate *priv;

  button = GIMP_PALETTE_SELECT_BUTTON (object);
  priv = GIMP_PALETTE_SELECT_BUTTON_GET_PRIVATE (button);

  switch (property_id)
    {
    case PROP_TITLE:
      priv->title = g_value_dup_string (value);
      break;
    case PROP_PALETTE_NAME:
      gimp_palette_select_button_set_palette (button,
                                              g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_palette_select_button_get_property (GObject    *object,
                                         guint       property_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  GimpPaletteSelectButton        *button;
  GimpPaletteSelectButtonPrivate *priv;

  button = GIMP_PALETTE_SELECT_BUTTON (object);
  priv = GIMP_PALETTE_SELECT_BUTTON_GET_PRIVATE (button);

  switch (property_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, priv->title);
      break;
    case PROP_PALETTE_NAME:
      g_value_set_string (value, priv->palette_name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_palette_select_button_callback (const gchar *palette_name,
                                     gboolean     dialog_closing,
                                     gpointer     user_data)
{
  GimpPaletteSelectButton        *button;
  GimpPaletteSelectButtonPrivate *priv;
  GimpSelectButton               *select_button;

  button = GIMP_PALETTE_SELECT_BUTTON (user_data);

  priv = GIMP_PALETTE_SELECT_BUTTON_GET_PRIVATE (button);
  select_button = GIMP_SELECT_BUTTON (button);

  g_free (priv->palette_name);
  priv->palette_name = g_strdup (palette_name);

  gtk_label_set_text (GTK_LABEL (priv->label), palette_name);

  if (dialog_closing)
    select_button->temp_callback = NULL;

  g_signal_emit (button, palette_button_signals[PALETTE_SET], 0,
                 palette_name, dialog_closing);
  g_object_notify (G_OBJECT (button), "palette-name");
}

static void
gimp_palette_select_button_clicked (GimpPaletteSelectButton *button)
{
  GimpPaletteSelectButtonPrivate *priv;
  GimpSelectButton               *select_button;

  priv = GIMP_PALETTE_SELECT_BUTTON_GET_PRIVATE (button);
  select_button = GIMP_SELECT_BUTTON (button);

  if (select_button->temp_callback)
    {
      /*  calling gimp_palettes_set_popup() raises the dialog  */
      gimp_palettes_set_popup (select_button->temp_callback,
                               priv->palette_name);
    }
  else
    {
      select_button->temp_callback =
        gimp_palette_select_new (priv->title, priv->palette_name,
                                 gimp_palette_select_button_callback,
                                 button);
    }
}

static void
gimp_palette_select_drag_data_received (GimpPaletteSelectButton *button,
                                        GdkDragContext          *context,
                                        gint                     x,
                                        gint                     y,
                                        GtkSelectionData        *selection,
                                        guint                    info,
                                        guint                    time)
{
  gint   length = gtk_selection_data_get_length (selection);
  gchar *str;

  if (gtk_selection_data_get_format (selection) != 8 || length < 1)
    {
      g_warning ("%s: received invalid palette data", G_STRFUNC);
      return;
    }

  str = g_strndup ((const gchar *) gtk_selection_data_get_data (selection),
                   length);

  if (g_utf8_validate (str, -1, NULL))
    {
      gint     pid;
      gpointer unused;
      gint     name_offset = 0;

      if (sscanf (str, "%i:%p:%n", &pid, &unused, &name_offset) >= 2 &&
          pid == gimp_getpid () && name_offset > 0)
        {
          gchar *name = str + name_offset;

          gimp_palette_select_button_set_palette (button, name);
        }
    }

  g_free (str);
}

static GtkWidget *
gimp_palette_select_button_create_inside (GimpPaletteSelectButton *palette_button)
{
  GtkWidget                      *button;
  GtkWidget                      *hbox;
  GtkWidget                      *image;
  GimpPaletteSelectButtonPrivate *priv;

  priv = GIMP_PALETTE_SELECT_BUTTON_GET_PRIVATE (palette_button);

  gtk_widget_push_composite_child ();

  button = gtk_button_new ();

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_container_add (GTK_CONTAINER (button), hbox);

  image = gtk_image_new_from_icon_name (GIMP_ICON_PALETTE,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

  priv->label = gtk_label_new (priv->palette_name);
  gtk_box_pack_start (GTK_BOX (hbox), priv->label, TRUE, TRUE, 4);

  gtk_widget_show_all (button);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (gimp_palette_select_button_clicked),
                            palette_button);

  gtk_drag_dest_set (GTK_WIDGET (button),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     &target, 1,
                     GDK_ACTION_COPY);

  g_signal_connect_swapped (button, "drag-data-received",
                            G_CALLBACK (gimp_palette_select_drag_data_received),
                            palette_button);

  gtk_widget_pop_composite_child ();

  return button;
}
