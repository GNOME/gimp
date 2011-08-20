/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsizebox.c
 * Copyright (C) 2004 Sven Neumann <sven@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdio.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpsizebox.h"

#include "gimp-intl.h"


#define SB_WIDTH 8

enum
{
  PROP_0,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_UNIT,
  PROP_XRESOLUTION,
  PROP_YRESOLUTION,
  PROP_RESOLUTION_UNIT,
  PROP_KEEP_ASPECT,
  PROP_EDIT_RESOLUTION
};


#define GIMP_SIZE_BOX_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                        GIMP_TYPE_SIZE_BOX, GimpSizeBoxPrivate))

typedef struct _GimpSizeBoxPrivate GimpSizeBoxPrivate;

struct _GimpSizeBoxPrivate
{
  GimpSizeEntry   *size_entry;
  GimpChainButton *chain_button;
  GtkWidget       *pixel_label;
  GtkWidget       *res_label;
  GimpUnitEntries *unit_entries;
};


static void   gimp_size_box_constructed       (GObject         *object);
static void   gimp_size_box_dispose           (GObject         *object);
static void   gimp_size_box_set_property      (GObject         *object,
                                               guint            property_id,
                                               const GValue    *value,
                                               GParamSpec      *pspec);
static void   gimp_size_box_get_property      (GObject         *object,
                                               guint            property_id,
                                               GValue          *value,
                                               GParamSpec      *pspec);

static void   gimp_size_box_update_size       (GimpSizeBox     *box);
static void   gimp_size_box_update_resolution (GimpSizeBox     *box);


G_DEFINE_TYPE (GimpSizeBox, gimp_size_box, GTK_TYPE_BOX)

#define parent_class gimp_size_box_parent_class


static void
gimp_size_box_class_init (GimpSizeBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_size_box_constructed;
  object_class->dispose      = gimp_size_box_dispose;
  object_class->set_property = gimp_size_box_set_property;
  object_class->get_property = gimp_size_box_get_property;

  g_type_class_add_private (object_class, sizeof (GimpSizeBoxPrivate));

  g_object_class_install_property (object_class, PROP_WIDTH,
                                   g_param_spec_int ("width", NULL, NULL,
                                                     GIMP_MIN_IMAGE_SIZE,
                                                     GIMP_MAX_IMAGE_SIZE,
                                                     256,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_HEIGHT,
                                   g_param_spec_int ("height", NULL, NULL,
                                                     GIMP_MIN_IMAGE_SIZE,
                                                     GIMP_MAX_IMAGE_SIZE,
                                                     256,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_UNIT,
                                   gimp_param_spec_unit ("unit", NULL, NULL,
                                                         TRUE, TRUE,
                                                         GIMP_UNIT_PIXEL,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_XRESOLUTION,
                                   g_param_spec_double ("xresolution",
                                                        NULL, NULL,
                                                        GIMP_MIN_RESOLUTION,
                                                        GIMP_MAX_RESOLUTION,
                                                        72.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_YRESOLUTION,
                                   g_param_spec_double ("yresolution",
                                                        NULL, NULL,
                                                        GIMP_MIN_RESOLUTION,
                                                        GIMP_MAX_RESOLUTION,
                                                        72.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_RESOLUTION_UNIT,
                                   gimp_param_spec_unit ("resolution-unit",
                                                         NULL, NULL,
                                                         FALSE, FALSE,
                                                         GIMP_UNIT_INCH,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_KEEP_ASPECT,
                                   g_param_spec_boolean ("keep-aspect",
                                                         NULL, NULL,
                                                         TRUE,
                                                         GIMP_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_EDIT_RESOLUTION,
                                   g_param_spec_boolean ("edit-resolution",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_size_box_init (GimpSizeBox *box)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (box),
                                  GTK_ORIENTATION_VERTICAL);

  gtk_box_set_spacing (GTK_BOX (box), 6);

  box->size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
}

static void
gimp_size_box_constructed (GObject *object)
{
  GimpSizeBox        *box  = GIMP_SIZE_BOX (object);
  GimpSizeBoxPrivate *priv = GIMP_SIZE_BOX_GET_PRIVATE (box);
  GtkWidget          *vbox;
  GimpUnitEntries    *unit_entries;
  GtkWidget          *hbox;
  GtkWidget          *label;
  GList              *children;
  GList              *list;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  priv->unit_entries = unit_entries = GIMP_UNIT_ENTRIES (
        gimp_prop_coordinates_new  (G_OBJECT (box),
                                    GIMP_UNIT_ENTRIES_WIDTH, GIMP_UNIT_ENTRIES_HEIGHT,
                                    _("Width:"), _("Height:"),
                                    "unit",
                                    box->xresolution, box->yresolution,
                                    TRUE));

  gimp_unit_entries_set_bounds (unit_entries, GIMP_UNIT_PIXEL, 
                                GIMP_UNIT_ENTRIES_WIDTH,  (gdouble) GIMP_MIN_IMAGE_SIZE, (gdouble) GIMP_MAX_IMAGE_SIZE,
                                GIMP_UNIT_ENTRIES_HEIGHT, (gdouble) GIMP_MIN_IMAGE_SIZE, (gdouble) GIMP_MAX_IMAGE_SIZE,
                                NULL);
  gimp_unit_entries_set_pixels (unit_entries,
                                GIMP_UNIT_ENTRIES_WIDTH,  (gdouble) box->width,
                                GIMP_UNIT_ENTRIES_HEIGHT, (gdouble) box->height,
                                NULL);

  priv->chain_button = GIMP_CHAIN_BUTTON (gimp_unit_entries_get_chain_button (unit_entries));

  gtk_box_pack_start (GTK_BOX (hbox), gimp_unit_entries_get_table (unit_entries), FALSE, FALSE, 0);

  children = gtk_container_get_children (GTK_CONTAINER (gimp_unit_entries_get_table (unit_entries)));
  for (list = children; list; list = g_list_next (list))
    if (GTK_IS_LABEL (list->data))
      gtk_size_group_add_widget (box->size_group, list->data);
  g_list_free (children);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_table_attach_defaults (GTK_TABLE (gimp_unit_entries_get_table (unit_entries)), vbox, 4, 6, 2, 3);
  gtk_widget_show (vbox);

  label = gtk_label_new (NULL);
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL,
                             -1);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  priv->pixel_label = label;

  if (box->edit_resolution)
    {
      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
      gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      unit_entries = GIMP_UNIT_ENTRIES (
        gimp_prop_coordinates_new  (G_OBJECT (box),
                                    "xresolution", "yresolution",
                                    _("X resolution:"), _("Y resolution:"),
                                    "resolution-unit",
                                    1, 1,
                                    TRUE));   
                                 
      gimp_unit_entries_set_mode   (unit_entries, GIMP_UNIT_ENTRY_MODE_RESOLUTION);                           
      gimp_unit_entries_set_pixels (unit_entries, 
                                    "xresolution", box->xresolution,
                                    "yresolution", box->yresolution,
                                    NULL);

      gtk_box_pack_start (GTK_BOX (hbox), gimp_unit_entries_get_table (unit_entries), FALSE, FALSE, 0);
      gtk_widget_show (gimp_unit_entries_get_table (unit_entries));

      children = gtk_container_get_children (GTK_CONTAINER (gimp_unit_entries_get_table (unit_entries)));
      for (list = children; list; list = g_list_next (list))
        if (GTK_IS_LABEL (list->data))
          gtk_size_group_add_widget (box->size_group, list->data);
      g_list_free (children);
    }
  else
    {
      label = gtk_label_new (NULL);
      gimp_label_set_attributes (GTK_LABEL (label),
                                 PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL,
                                 -1);
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      priv->res_label = label;
    }

  gimp_size_box_update_size (box);
  gimp_size_box_update_resolution (box);
}

static void
gimp_size_box_dispose (GObject *object)
{
  GimpSizeBox *box = GIMP_SIZE_BOX (object);

  if (box->size_group)
    {
      g_object_unref (box->size_group);
      box->size_group = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_size_box_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  GimpSizeBox        *box  = GIMP_SIZE_BOX (object);
  GimpSizeBoxPrivate *priv = GIMP_SIZE_BOX_GET_PRIVATE (box);

  switch (property_id)
    {
    case PROP_WIDTH:
      box->width = g_value_get_int (value);
      gimp_size_box_update_size (box);
      break;

    case PROP_HEIGHT:
      box->height = g_value_get_int (value);
      gimp_size_box_update_size (box);
      break;

    case PROP_UNIT:
      box->unit = g_value_get_int (value);
      break;

    case PROP_XRESOLUTION:
      box->xresolution = g_value_get_double (value);
      if (priv->unit_entries)
        gimp_unit_adjustment_set_resolution (
            gimp_unit_entries_get_adjustment (priv->unit_entries, GIMP_UNIT_ENTRIES_WIDTH),
            box->xresolution);
      gimp_size_box_update_resolution (box);
      break;

    case PROP_YRESOLUTION:
      box->yresolution = g_value_get_double (value);
      if (priv->unit_entries)
        gimp_unit_adjustment_set_resolution (
            gimp_unit_entries_get_adjustment (priv->unit_entries, GIMP_UNIT_ENTRIES_HEIGHT),
            box->yresolution);
      gimp_size_box_update_resolution (box);
      break;

    case PROP_RESOLUTION_UNIT:
      box->resolution_unit = g_value_get_int (value);
      break;

    case PROP_KEEP_ASPECT:
      if (priv->chain_button)
        gimp_chain_button_set_active (priv->chain_button,
                                      g_value_get_boolean (value));
      break;

    case PROP_EDIT_RESOLUTION:
      box->edit_resolution = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_size_box_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  GimpSizeBox        *box  = GIMP_SIZE_BOX (object);
  GimpSizeBoxPrivate *priv = GIMP_SIZE_BOX_GET_PRIVATE (box);

  switch (property_id)
    {
    case PROP_WIDTH:
      g_value_set_int (value, box->width);
      break;

    case PROP_HEIGHT:
      g_value_set_int (value, box->height);
      break;

    case PROP_UNIT:
      g_value_set_int (value, box->unit);
      break;

    case PROP_XRESOLUTION:
      g_value_set_double (value, box->xresolution);
      break;

    case PROP_YRESOLUTION:
      g_value_set_double (value, box->yresolution);
      break;

    case PROP_RESOLUTION_UNIT:
      g_value_set_int (value, box->resolution_unit);
      break;

    case PROP_KEEP_ASPECT:
      g_value_set_boolean (value,
                           gimp_chain_button_get_active (priv->chain_button));
      break;

    case PROP_EDIT_RESOLUTION:
      g_value_set_boolean (value, box->edit_resolution);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_size_box_update_size (GimpSizeBox *box)
{
  GimpSizeBoxPrivate *priv = GIMP_SIZE_BOX_GET_PRIVATE (box);

  if (priv->pixel_label)
    {
      gchar *text = g_strdup_printf (ngettext ("%d × %d pixel",
                                               "%d × %d pixels", box->height),
                                     box->width, box->height);
      gtk_label_set_text (GTK_LABEL (priv->pixel_label), text);
      g_free (text);
    }
}

static void
gimp_size_box_update_resolution (GimpSizeBox *box)
{
  GimpSizeBoxPrivate *priv = GIMP_SIZE_BOX_GET_PRIVATE (box);

  if (priv->unit_entries)
    {
      gimp_unit_entries_set_pixels (priv->unit_entries, 
                                    GIMP_UNIT_ENTRIES_WIDTH,  (gdouble) box->width,
                                    GIMP_UNIT_ENTRIES_HEIGHT, (gdouble) box->height,
                                    NULL);
    }

  if (priv->res_label)
    {
      gchar *text;
      gint   xres = ROUND (box->xresolution);
      gint   yres = ROUND (box->yresolution);

      if (xres != yres)
        text = g_strdup_printf (_("%d × %d ppi"), xres, yres);
      else
        text = g_strdup_printf (_("%d ppi"), yres);

      gtk_label_set_text (GTK_LABEL (priv->res_label), text);
      g_free (text);
    }
}
