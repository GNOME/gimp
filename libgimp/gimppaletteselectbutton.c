/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmapaletteselectbutton.c
 * Copyright (C) 2004  Michael Natterer <mitch@ligma.org>
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

#include "libligmawidgets/ligmawidgets.h"

#include "ligma.h"

#include "ligmauitypes.h"
#include "ligmapaletteselectbutton.h"
#include "ligmauimarshal.h"

#include "libligma-intl.h"


/**
 * SECTION: ligmapaletteselectbutton
 * @title: LigmaPaletteSelect
 * @short_description: A button which pops up a palette select dialog.
 *
 * A button which pops up a palette select dialog.
 **/


struct _LigmaPaletteSelectButtonPrivate
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
  PROP_PALETTE_NAME,
  N_PROPS
};


/*  local function prototypes  */

static void   ligma_palette_select_button_finalize     (GObject      *object);

static void   ligma_palette_select_button_set_property (GObject      *object,
                                                       guint         property_id,
                                                       const GValue *value,
                                                       GParamSpec   *pspec);
static void   ligma_palette_select_button_get_property (GObject      *object,
                                                       guint         property_id,
                                                       GValue       *value,
                                                       GParamSpec   *pspec);

static void   ligma_palette_select_button_clicked  (LigmaPaletteSelectButton *button);

static void   ligma_palette_select_button_callback (const gchar *palette_name,
                                                   gboolean     dialog_closing,
                                                   gpointer     user_data);

static void   ligma_palette_select_drag_data_received (LigmaPaletteSelectButton *button,
                                                      GdkDragContext          *context,
                                                      gint                     x,
                                                      gint                     y,
                                                      GtkSelectionData        *selection,
                                                      guint                    info,
                                                      guint                    time);

static GtkWidget * ligma_palette_select_button_create_inside (LigmaPaletteSelectButton *palette_button);


static const GtkTargetEntry target = { "application/x-ligma-palette-name", 0 };

static guint palette_button_signals[LAST_SIGNAL] = { 0 };
static GParamSpec *palette_button_props[N_PROPS] = { NULL, };


G_DEFINE_TYPE_WITH_PRIVATE (LigmaPaletteSelectButton, ligma_palette_select_button,
                            LIGMA_TYPE_SELECT_BUTTON)


static void
ligma_palette_select_button_class_init (LigmaPaletteSelectButtonClass *klass)
{
  GObjectClass          *object_class        = G_OBJECT_CLASS (klass);
  LigmaSelectButtonClass *select_button_class = LIGMA_SELECT_BUTTON_CLASS (klass);

  object_class->finalize     = ligma_palette_select_button_finalize;
  object_class->set_property = ligma_palette_select_button_set_property;
  object_class->get_property = ligma_palette_select_button_get_property;

  select_button_class->select_destroy = ligma_palette_select_destroy;

  klass->palette_set = NULL;

  /**
   * LigmaPaletteSelectButton:title:
   *
   * The title to be used for the palette selection popup dialog.
   *
   * Since: 2.4
   */
  palette_button_props[PROP_TITLE] = g_param_spec_string ("title",
                                                          "Title",
                                                          "The title to be used for the palette selection popup dialog",
                                                          _("Palette Selection"),
                                                          LIGMA_PARAM_READWRITE |
                                                          G_PARAM_CONSTRUCT_ONLY);

  /**
   * LigmaPaletteSelectButton:palette-name:
   *
   * The name of the currently selected palette.
   *
   * Since: 2.4
   */
  palette_button_props[PROP_PALETTE_NAME] = g_param_spec_string ("palette-name",
                                                        "Palette name",
                                                        "The name of the currently selected palette",
                                                        NULL,
                                                        LIGMA_PARAM_READWRITE);

  g_object_class_install_properties (object_class, N_PROPS, palette_button_props);

  /**
   * LigmaPaletteSelectButton::palette-set:
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
                  G_STRUCT_OFFSET (LigmaPaletteSelectButtonClass, palette_set),
                  NULL, NULL,
                  _ligmaui_marshal_VOID__STRING_BOOLEAN,
                  G_TYPE_NONE, 2,
                  G_TYPE_STRING,
                  G_TYPE_BOOLEAN);
}

static void
ligma_palette_select_button_init (LigmaPaletteSelectButton *button)
{
  button->priv = ligma_palette_select_button_get_instance_private (button);

  button->priv->inside = ligma_palette_select_button_create_inside (button);
  gtk_container_add (GTK_CONTAINER (button), button->priv->inside);
}

/**
 * ligma_palette_select_button_new:
 * @title: (nullable): Title of the dialog to use or %NULL to use the default title.
 * @palette_name: (nullable): Initial palette name.
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
ligma_palette_select_button_new (const gchar *title,
                                const gchar *palette_name)
{
  GtkWidget *button;

  if (title)
    button = g_object_new (LIGMA_TYPE_PALETTE_SELECT_BUTTON,
                           "title",        title,
                           "palette-name", palette_name,
                           NULL);
  else
    button = g_object_new (LIGMA_TYPE_PALETTE_SELECT_BUTTON,
                           "palette-name", palette_name,
                           NULL);

  return button;
}

/**
 * ligma_palette_select_button_get_palette:
 * @button: A #LigmaPaletteSelectButton
 *
 * Retrieves the name of currently selected palette.
 *
 * Returns: an internal copy of the palette name which must not be freed.
 *
 * Since: 2.4
 */
const gchar *
ligma_palette_select_button_get_palette (LigmaPaletteSelectButton *button)
{
  g_return_val_if_fail (LIGMA_IS_PALETTE_SELECT_BUTTON (button), NULL);

  return button->priv->palette_name;
}

/**
 * ligma_palette_select_button_set_palette:
 * @button: A #LigmaPaletteSelectButton
 * @palette_name: (nullable): Palette name to set; %NULL means no change.
 *
 * Sets the current palette for the palette select button.
 *
 * Since: 2.4
 */
void
ligma_palette_select_button_set_palette (LigmaPaletteSelectButton *button,
                                        const gchar             *palette_name)
{
  LigmaSelectButton *select_button;

  g_return_if_fail (LIGMA_IS_PALETTE_SELECT_BUTTON (button));

  select_button = LIGMA_SELECT_BUTTON (button);

  if (select_button->temp_callback)
    {
      ligma_palettes_set_popup (select_button->temp_callback, palette_name);
    }
  else
    {
      gchar *name;
      gint   num_colors;

      if (palette_name && *palette_name)
        name = g_strdup (palette_name);
      else
        name = ligma_context_get_palette ();

      if (ligma_palette_get_info (name, &num_colors))
        ligma_palette_select_button_callback (name, FALSE, button);

      g_free (name);
    }
}


/*  private functions  */

static void
ligma_palette_select_button_finalize (GObject *object)
{
  LigmaPaletteSelectButton *button = LIGMA_PALETTE_SELECT_BUTTON (object);

  g_clear_pointer (&button->priv->palette_name, g_free);
  g_clear_pointer (&button->priv->title,        g_free);

  G_OBJECT_CLASS (ligma_palette_select_button_parent_class)->finalize (object);
}

static void
ligma_palette_select_button_set_property (GObject      *object,
                                         guint         property_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  LigmaPaletteSelectButton *button = LIGMA_PALETTE_SELECT_BUTTON (object);

  switch (property_id)
    {
    case PROP_TITLE:
      button->priv->title = g_value_dup_string (value);
      break;

    case PROP_PALETTE_NAME:
      ligma_palette_select_button_set_palette (button,
                                              g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_palette_select_button_get_property (GObject    *object,
                                         guint       property_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  LigmaPaletteSelectButton *button = LIGMA_PALETTE_SELECT_BUTTON (object);

  switch (property_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, button->priv->title);
      break;

    case PROP_PALETTE_NAME:
      g_value_set_string (value, button->priv->palette_name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_palette_select_button_callback (const gchar *palette_name,
                                     gboolean     dialog_closing,
                                     gpointer     user_data)
{
  LigmaPaletteSelectButton *button        = user_data;
  LigmaSelectButton        *select_button = LIGMA_SELECT_BUTTON (button);

  g_free (button->priv->palette_name);
  button->priv->palette_name = g_strdup (palette_name);

  gtk_label_set_text (GTK_LABEL (button->priv->label), palette_name);

  if (dialog_closing)
    select_button->temp_callback = NULL;

  g_signal_emit (button, palette_button_signals[PALETTE_SET], 0,
                 palette_name, dialog_closing);
  g_object_notify_by_pspec (G_OBJECT (button), palette_button_props[PROP_PALETTE_NAME]);
}

static void
ligma_palette_select_button_clicked (LigmaPaletteSelectButton *button)
{
  LigmaSelectButton *select_button = LIGMA_SELECT_BUTTON (button);

  if (select_button->temp_callback)
    {
      /*  calling ligma_palettes_set_popup() raises the dialog  */
      ligma_palettes_set_popup (select_button->temp_callback,
                               button->priv->palette_name);
    }
  else
    {
      select_button->temp_callback =
        ligma_palette_select_new (button->priv->title,
                                 button->priv->palette_name,
                                 ligma_palette_select_button_callback,
                                 button, NULL);
    }
}

static void
ligma_palette_select_drag_data_received (LigmaPaletteSelectButton *button,
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
          pid == ligma_getpid () && name_offset > 0)
        {
          gchar *name = str + name_offset;

          ligma_palette_select_button_set_palette (button, name);
        }
    }

  g_free (str);
}

static GtkWidget *
ligma_palette_select_button_create_inside (LigmaPaletteSelectButton *palette_button)
{
  GtkWidget *button;
  GtkWidget *hbox;
  GtkWidget *image;

  button = gtk_button_new ();

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_container_add (GTK_CONTAINER (button), hbox);

  image = gtk_image_new_from_icon_name (LIGMA_ICON_PALETTE,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

  palette_button->priv->label = gtk_label_new (palette_button->priv->palette_name);
  gtk_box_pack_start (GTK_BOX (hbox), palette_button->priv->label, TRUE, TRUE, 4);

  gtk_widget_show_all (button);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (ligma_palette_select_button_clicked),
                            palette_button);

  gtk_drag_dest_set (GTK_WIDGET (button),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     &target, 1,
                     GDK_ACTION_COPY);

  g_signal_connect_swapped (button, "drag-data-received",
                            G_CALLBACK (ligma_palette_select_drag_data_received),
                            palette_button);

  return button;
}
