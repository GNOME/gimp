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

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "widgets/gimppropwidgets.h"
#include "widgets/gimpwidgets-constructors.h"
#include "widgets/gimpwidgets-utils.h"

#include "core/gimptooloptions.h"
#include "gimppaintselectoptions.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_MODE,
  PROP_STROKE_WIDTH,
  PROP_SHOW_SCRIBBLES,
};


static void   gimp_paint_select_options_set_property      (GObject      *object,
                                                           guint         property_id,
                                                           const GValue *value,
                                                           GParamSpec   *pspec);
static void   gimp_paint_select_options_get_property      (GObject      *object,
                                                           guint         property_id,
                                                           GValue       *value,
                                                           GParamSpec   *pspec);


G_DEFINE_TYPE (GimpPaintSelectOptions, gimp_paint_select_options,
               GIMP_TYPE_TOOL_OPTIONS)


static void
gimp_paint_select_options_class_init (GimpPaintSelectOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_paint_select_options_set_property;
  object_class->get_property = gimp_paint_select_options_get_property;

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_MODE,
                         "mode",
                         _("Mode"),
                         _("Paint over areas to mark pixels for "
                           "inclusion or exclusion from selection"),
                         GIMP_TYPE_PAINT_SELECT_MODE,
                         GIMP_PAINT_SELECT_MODE_ADD,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_INT  (object_class, PROP_STROKE_WIDTH,
                         "stroke-width",
                         _("Stroke width"),
                         _("Size of the brush used for refinements"),
                         1, 6000, 50,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN  (object_class, PROP_SHOW_SCRIBBLES,
                         "show-scribbles",
                         _("Show scribbles"),
                         _("Show scribbles"),
                         FALSE,
                         GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_paint_select_options_init (GimpPaintSelectOptions *options)
{
}

static void
gimp_paint_select_options_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  GimpPaintSelectOptions *options = GIMP_PAINT_SELECT_OPTIONS (object);

  switch (property_id)
    {
    case PROP_MODE:
      options->mode = g_value_get_enum (value);
      break;

    case PROP_STROKE_WIDTH:
      options->stroke_width = g_value_get_int (value);
      break;

    case PROP_SHOW_SCRIBBLES:
      options->show_scribbles = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_paint_select_options_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  GimpPaintSelectOptions *options = GIMP_PAINT_SELECT_OPTIONS (object);

  switch (property_id)
    {
    case PROP_MODE:
      g_value_set_enum (value, options->mode);
      break;

    case PROP_STROKE_WIDTH:
      g_value_set_int (value, options->stroke_width);
      break;

    case PROP_SHOW_SCRIBBLES:
      g_value_set_boolean (value, options->show_scribbles);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_paint_select_options_reset_stroke_width (GtkWidget       *button,
                                              GimpToolOptions *tool_options)
{
  g_object_set (tool_options, "stroke-width", 10, NULL);
}

GtkWidget *
gimp_paint_select_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = gimp_tool_options_gui (tool_options);
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *frame;
  GList     *children;
  GList     *radio_children;
  GList     *list;
  GtkWidget *scale;
  gint       i;

  frame = gimp_prop_enum_radio_frame_new (config, "mode", NULL,
                                          0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  /* add modifier to tooltips */
  children = gtk_container_get_children (GTK_CONTAINER (frame));
  radio_children = gtk_container_get_children (GTK_CONTAINER (children->data));
  for (list = radio_children, i = 0; list; list = list->next, i++)
    {
      GtkWidget   *button   = list->data;
      const gchar *modifier = NULL;
      const gchar *label    = NULL;

      if (i == 0)
        modifier = gimp_get_mod_string (gimp_get_extend_selection_mask ());
      else if (i == 1)
        modifier = gimp_get_mod_string (gimp_get_modify_selection_mask ());

      if (! modifier)
        continue;

      label = gtk_button_get_label (GTK_BUTTON (button));

      if (label)
        {
          gchar *tip = g_strdup_printf ("%s  <b>%s</b>", label, modifier);

          gimp_help_set_help_data_with_markup (button, tip, NULL);

          g_free (tip);
        }
    }
  g_list_free (children);
  g_list_free (radio_children);
  g_list_free (list);

  /* stroke width */
  scale = gimp_prop_spin_scale_new (config, "stroke-width",
                                    1.0, 10.0, 2);
  gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (scale), 1.0, 1000.0);
  gimp_spin_scale_set_gamma (GIMP_SPIN_SCALE (scale), 1.7);
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);

  button = gimp_icon_button_new (GIMP_ICON_RESET, NULL);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_image_set_from_icon_name (GTK_IMAGE (gtk_bin_get_child (GTK_BIN (button))),
                                GIMP_ICON_RESET, GTK_ICON_SIZE_MENU);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (gimp_paint_select_options_reset_stroke_width),
                    tool_options);

  gimp_help_set_help_data (button,
                           _("Reset stroke width native size"), NULL);

  /* show scribbles */
  button = gimp_prop_check_button_new (config, "show-scribbles", "Show scribbles");
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  return vbox;
}
