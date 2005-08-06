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

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimptoolinfo.h"

#include "gimpforegroundselectoptions.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_BACKGROUND,
  PROP_STROKE_WIDTH
};


static void   gimp_foreground_select_options_class_init   (GimpForegroundSelectOptionsClass *klass);

static void   gimp_foreground_select_options_set_property (GObject         *object,
                                                           guint            property_id,
                                                           const GValue    *value,
                                                           GParamSpec      *pspec);
static void   gimp_foreground_select_options_get_property (GObject         *object,
                                                           guint            property_id,
                                                           GValue          *value,
                                                           GParamSpec      *pspec);


GType
gimp_foreground_select_options_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpForegroundSelectOptionsClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_foreground_select_options_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpForegroundSelectOptions),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) NULL
      };

      type = g_type_register_static (GIMP_TYPE_SELECTION_OPTIONS,
                                     "GimpForegroundSelectOptions",
                                     &info, 0);
    }

  return type;
}

static void
gimp_foreground_select_options_class_init (GimpForegroundSelectOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_foreground_select_options_set_property;
  object_class->get_property = gimp_foreground_select_options_get_property;

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_BACKGROUND,
                                    "background", NULL,
                                    FALSE,
                                    0);
  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_STROKE_WIDTH,
                                "stroke-width", NULL,
                                1, 100, 6,
                                0);
}

static void
gimp_foreground_select_options_set_property (GObject      *object,
                                             guint         property_id,
                                             const GValue *value,
                                             GParamSpec   *pspec)
{
  GimpForegroundSelectOptions *options = GIMP_FOREGROUND_SELECT_OPTIONS (object);

  switch (property_id)
    {
    case PROP_BACKGROUND:
      options->background = g_value_get_boolean (value);
      break;
    case PROP_STROKE_WIDTH:
      options->stroke_width = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_foreground_select_options_get_property (GObject    *object,
                                             guint       property_id,
                                             GValue     *value,
                                             GParamSpec *pspec)
{
  GimpForegroundSelectOptions *options = GIMP_FOREGROUND_SELECT_OPTIONS (object);

  switch (property_id)
    {
    case PROP_BACKGROUND:
      g_value_set_boolean (value, options->background);
      break;
    case PROP_STROKE_WIDTH:
      g_value_set_int (value, options->stroke_width);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
gimp_foreground_select_options_gui (GimpToolOptions *tool_options)
{
  GtkWidget *vbox = gimp_selection_options_gui (tool_options);
  GtkWidget *frame;

  frame = gimp_prop_boolean_radio_frame_new ( G_OBJECT (tool_options),
                                              "background",
                                             _("Interactive refinement"),
                                             _("Mark background"),
                                             _("Mark foreground"));

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  return vbox;
}
