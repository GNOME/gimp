/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimprectangleoptions.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_HIGHLIGHT,
  PROP_CONSTRAINT,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_ASPECT,
};


static void   gimp_rectangle_options_class_init (GimpRectangleOptionsClass *options_class);

static void   gimp_rectangle_options_set_property (GObject         *object,
                                                   guint            property_id,
                                                   const GValue    *value,
                                                   GParamSpec      *pspec);
static void   gimp_rectangle_options_get_property (GObject         *object,
                                                   guint            property_id,
                                                   GValue          *value,
                                                   GParamSpec      *pspec);


static GimpToolOptionsClass *parent_class = NULL;


GType
gimp_rectangle_options_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpRectangleOptionsClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_rectangle_options_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpRectangleOptions),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) NULL
      };

      type = g_type_register_static (GIMP_TYPE_SELECTION_OPTIONS,
                                     "GimpRectangleOptions",
                                     &info, 0);
    }

  return type;
}

static void
gimp_rectangle_options_class_init (GimpRectangleOptionsClass *klass)
{
  GObjectClass         *object_class  = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->set_property = gimp_rectangle_options_set_property;
  object_class->get_property = gimp_rectangle_options_get_property;

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_HIGHLIGHT,
                                    "highlight",
                                    N_("Highlight rectangle"),
                                    TRUE,
                                    0);

  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_WIDTH,
                                   "width", N_("Width"),
                                   0.0, GIMP_MAX_IMAGE_SIZE, 1.0,
                                   0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_HEIGHT,
                                   "height", N_("Height"),
                                   0.0, GIMP_MAX_IMAGE_SIZE, 1.0,
                                   0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_ASPECT,
                                   "aspect", N_("Aspect"),
                                   0.0, GIMP_MAX_IMAGE_SIZE, 1.0,
                                   0);
}

static void
gimp_rectangle_options_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GimpRectangleOptions *options = GIMP_RECTANGLE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_HIGHLIGHT:
      options->highlight = g_value_get_boolean (value);
      break;

    case PROP_WIDTH:
      options->width = g_value_get_double (value);
      break;

    case PROP_HEIGHT:
      options->height = g_value_get_double (value);
      break;

    case PROP_ASPECT:
      options->aspect = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_rectangle_options_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  GimpRectangleOptions *options = GIMP_RECTANGLE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_HIGHLIGHT:
      g_value_set_boolean (value, options->highlight);
      break;

    case PROP_WIDTH:
      g_value_set_double (value, options->width);
      break;

    case PROP_HEIGHT:
      g_value_set_double (value, options->height);
      break;

    case PROP_ASPECT:
      g_value_set_double (value, options->aspect);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
gimp_rectangle_options_gui (GimpToolOptions *tool_options)
{
  GObject     *config  = G_OBJECT (tool_options);
  GtkWidget   *vbox;
  GtkWidget   *button;
  GtkWidget   *controls_container;

  vbox = gimp_selection_options_gui (tool_options);

  /*  the highlight toggle button  */
  button = gimp_prop_check_button_new (config, "highlight",
                                       _("Highlight"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  controls_container = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), controls_container, FALSE, FALSE, 0);
  gtk_widget_show (controls_container);
  g_object_set_data (G_OBJECT (tool_options),
                     "controls-container", controls_container);

  return vbox;
}
