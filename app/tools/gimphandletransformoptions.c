/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimppropwidgets.h"
#include "widgets/gimpspinscale.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimphandletransformoptions.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_HANDLE_MODE
};


static void   gimp_handle_transform_options_set_property (GObject         *object,
                                                          guint            property_id,
                                                          const GValue    *value,
                                                          GParamSpec      *pspec);
static void   gimp_handle_transform_options_get_property (GObject         *object,
                                                          guint            property_id,
                                                          GValue          *value,
                                                          GParamSpec      *pspec);


G_DEFINE_TYPE (GimpHandleTransformOptions, gimp_handle_transform_options,
               GIMP_TYPE_TRANSFORM_GRID_OPTIONS)

#define parent_class gimp_handle_transform_options_parent_class


static void
gimp_handle_transform_options_class_init (GimpHandleTransformOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_handle_transform_options_set_property;
  object_class->get_property = gimp_handle_transform_options_get_property;

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_HANDLE_MODE,
                         "handle-mode",
                         _("Handle mode"),
                         _("Handle mode"),
                         GIMP_TYPE_TRANSFORM_HANDLE_MODE,
                         GIMP_HANDLE_MODE_ADD_TRANSFORM,
                         GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_handle_transform_options_init (GimpHandleTransformOptions *options)
{
}

static void
gimp_handle_transform_options_set_property (GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec)
{
  GimpHandleTransformOptions *options = GIMP_HANDLE_TRANSFORM_OPTIONS (object);

  switch (property_id)
    {
    case PROP_HANDLE_MODE:
      options->handle_mode = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_handle_transform_options_get_property (GObject    *object,
                                            guint       property_id,
                                            GValue     *value,
                                            GParamSpec *pspec)
{
  GimpHandleTransformOptions *options = GIMP_HANDLE_TRANSFORM_OPTIONS (object);

  switch (property_id)
    {
    case PROP_HANDLE_MODE:
      g_value_set_enum (value, options->handle_mode);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * gimp_handle_transform_options_gui:
 * @tool_options: a #GimpToolOptions
 *
 * Build the Transform Tool Options.
 *
 * Return value: a container holding the transform tool options
 **/
GtkWidget *
gimp_handle_transform_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = gimp_transform_grid_options_gui (tool_options);
  GtkWidget *frame;
  GtkWidget *button;
  gint       i;

  frame = gimp_prop_enum_radio_frame_new (config, "handle-mode", NULL,
                                          0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /* add modifier to name, add tooltip */
  button = g_object_get_data (G_OBJECT (frame), "radio-button");

  if (GTK_IS_RADIO_BUTTON (button))
    {
      GSList *list = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));

      for (i = g_slist_length (list) - 1 ; list; list = list->next, i--)
        {
          GdkModifierType  shift    = gimp_get_extend_selection_mask ();
          GdkModifierType  ctrl     = gimp_get_constrain_behavior_mask ();
          GdkModifierType  modifier = 0;
          gchar           *tooltip  = "";
          gchar           *tip;
          gchar           *label;

          switch (i)
            {
            case GIMP_HANDLE_MODE_ADD_TRANSFORM:
              modifier = 0;
              tooltip  = _("Add handles and transform the image");
              break;

            case GIMP_HANDLE_MODE_MOVE:
              modifier = shift;
              tooltip  = _("Move transform handles");
              break;

            case GIMP_HANDLE_MODE_REMOVE:
              modifier = ctrl;
              tooltip  = _("Remove transform handles");
              break;
            }

          if (modifier)
            {
              label = g_strdup_printf ("%s (%s)",
                                       gtk_button_get_label (GTK_BUTTON (list->data)),
                                       gimp_get_mod_string (modifier));
              gtk_button_set_label (GTK_BUTTON (list->data), label);
              g_free (label);

              tip = g_strdup_printf ("%s  (%s)",
                                     tooltip, gimp_get_mod_string (modifier));
              gimp_help_set_help_data (list->data, tip, NULL);
              g_free (tip);
            }
          else
            {
              gimp_help_set_help_data (list->data, tooltip, NULL);
            }
        }
    }

  return vbox;
}
