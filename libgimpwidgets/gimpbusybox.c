/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpbusybox.c
 * Copyright (C) 2018 Ell
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "gimpwidgetstypes.h"

#include "gimpbusybox.h"
#include "gimpwidgetsutils.h"


/**
 * SECTION: gimpbusybox
 * @title: GimpBusyBox
 * @short_description: A widget indicating an ongoing operation
 *
 * #GimpBusyBox displays a styled message, providing indication of
 * an ongoing operation.
 **/


enum
{
  PROP_0,
  PROP_MESSAGE
};


struct _GimpBusyBox
{
  GtkBox    parent_instance;

  GtkLabel *label;
};


/*  local function prototypes  */

static void   gimp_busy_box_set_property (GObject      *object,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec);
static void   gimp_busy_box_get_property (GObject      *object,
                                          guint         property_id,
                                          GValue       *value,
                                          GParamSpec   *pspec);


G_DEFINE_TYPE (GimpBusyBox, gimp_busy_box, GTK_TYPE_BOX)

#define parent_class gimp_busy_box_parent_class


/*  private functions  */


static void
gimp_busy_box_class_init (GimpBusyBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_busy_box_set_property;
  object_class->get_property = gimp_busy_box_get_property;

  /**
   * GimpBusyBox:message:
   *
   * Specifies the displayed message.
   *
   * Since: 2.10.4
   **/
  g_object_class_install_property (object_class, PROP_MESSAGE,
                                   g_param_spec_string ("message",
                                                        "Message",
                                                        "The message to display",
                                                        NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gimp_busy_box_init (GimpBusyBox *box)
{
  GtkWidget *spinner;
  GtkWidget *label;

  gtk_widget_set_halign (GTK_WIDGET (box), GTK_ALIGN_CENTER);
  gtk_widget_set_valign (GTK_WIDGET (box), GTK_ALIGN_CENTER);
  gtk_box_set_spacing (GTK_BOX (box), 8);

  /* the spinner */
  spinner = gtk_spinner_new ();
  gtk_spinner_start (GTK_SPINNER (spinner));
  gtk_box_pack_start (GTK_BOX (box), spinner, FALSE, FALSE, 0);
  gtk_widget_show (spinner);

  /* the label */
  label = gtk_label_new (NULL);
  box->label = GTK_LABEL (label);
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
}

static void
gimp_busy_box_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  GimpBusyBox *box = GIMP_BUSY_BOX (object);

  switch (property_id)
    {
    case PROP_MESSAGE:
      gtk_label_set_text (box->label, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_busy_box_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpBusyBox *box = GIMP_BUSY_BOX (object);

  switch (property_id)
    {
    case PROP_MESSAGE:
      g_value_set_string (value, gtk_label_get_text (box->label));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */


/**
 * gimp_busy_box_new:
 * @message: (allow-none): the displayed message, or %NULL
 *
 * Creates a new #GimpBusyBox widget.
 *
 * Returns: A pointer to the new #GimpBusyBox widget.
 *
 * Since: 2.10.4
 **/
GtkWidget *
gimp_busy_box_new (const gchar *message)
{
  if (message == NULL)
    message = "";

  return g_object_new (GIMP_TYPE_BUSY_BOX,
                       "message", message,
                       NULL);
}

/**
 * gimp_busy_box_set_message:
 * @box:     a #GimpBusyBox
 * @message: the displayed message
 *
 * Sets the displayed message og @box to @message.
 *
 * Since: 2.10.4
 **/
void
gimp_busy_box_set_message (GimpBusyBox *box,
                           const gchar *message)
{
  g_return_if_fail (GIMP_IS_BUSY_BOX (box));
  g_return_if_fail (message != NULL);

  g_object_set (box,
                "message", message,
                NULL);
}

/**
 * gimp_busy_box_get_message:
 * @box: a #GimpBusyBox
 *
 * Returns the displayed message of @box.
 *
 * Returns: The displayed message.
 *
 * Since: 2.10.4
 **/
const gchar *
gimp_busy_box_get_message (GimpBusyBox *box)
{
  g_return_val_if_fail (GIMP_IS_BUSY_BOX (box), NULL);

  return gtk_label_get_text (box->label);
}
