/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsizebox.c
 * Copyright (C) 2004 Sven Neumann <sven@gimp.org>
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
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpconfig-params.h"

#include "gimppropwidgets.h"
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


#define GIMP_SIZE_BOX_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIMP_TYPE_SIZE_BOX, GimpSizeBoxPrivate))

typedef struct _GimpSizeBoxPrivate GimpSizeBoxPrivate;

struct _GimpSizeBoxPrivate
{
  GimpSizeEntry   *entry;
  GimpChainButton *chain;
  GtkWidget       *pixel_label;
  GtkWidget       *res_label;
  gdouble          aspect;
};


static void      gimp_size_box_class_init    (GimpSizeBoxClass      *klass);

static GObject * gimp_size_box_constructor   (GType                  type,
                                              guint                  n_params,
                                              GObjectConstructParam *params);

static void      gimp_size_box_init           (GimpSizeBox     *box);

static void      gimp_size_box_set_property   (GObject         *object,
                                               guint            property_id,
                                               const GValue    *value,
                                               GParamSpec      *pspec);
static void      gimp_size_box_get_property   (GObject         *object,
                                               guint            property_id,
                                               GValue          *value,
                                               GParamSpec      *pspec);

static void      gimp_size_box_update_size       (GimpSizeBox *box);
static void      gimp_size_box_update_resolution (GimpSizeBox *box);


static GtkVBoxClass *parent_class = NULL;


GType
gimp_size_box_get_type (void)
{
  static GType box_type = 0;

  if (! box_type)
    {
      static const GTypeInfo box_info =
      {
        sizeof (GimpSizeBoxClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_size_box_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpSizeBox),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_size_box_init
      };

      box_type = g_type_register_static (GTK_TYPE_VBOX,
                                         "GimpSizeBox",
                                         &box_info, 0);
    }

  return box_type;
}

static void
gimp_size_box_class_init (GimpSizeBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor  = gimp_size_box_constructor;
  object_class->set_property = gimp_size_box_set_property;
  object_class->get_property = gimp_size_box_get_property;

  g_type_class_add_private (object_class, sizeof (GimpSizeBoxPrivate));

  g_object_class_install_property (object_class, PROP_WIDTH,
                                   g_param_spec_int ("width", NULL, NULL,
                                                     GIMP_MIN_IMAGE_SIZE,
                                                     GIMP_MAX_IMAGE_SIZE,
                                                     256,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_HEIGHT,
                                   g_param_spec_int ("height", NULL, NULL,
                                                     GIMP_MIN_IMAGE_SIZE,
                                                     GIMP_MAX_IMAGE_SIZE,
                                                     256,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_UNIT,
                                   gimp_param_spec_unit ("unit", NULL, NULL,
                                                         TRUE, TRUE,
                                                         GIMP_UNIT_PIXEL,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_XRESOLUTION,
                                   g_param_spec_double ("xresolution",
                                                        NULL, NULL,
                                                        GIMP_MIN_RESOLUTION,
                                                        GIMP_MAX_RESOLUTION,
                                                        72.0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_YRESOLUTION,
                                   g_param_spec_double ("yresolution",
                                                        NULL, NULL,
                                                        GIMP_MIN_RESOLUTION,
                                                        GIMP_MAX_RESOLUTION,
                                                        72.0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_RESOLUTION_UNIT,
                                   gimp_param_spec_unit ("resolution-unit",
                                                         NULL, NULL,
                                                         FALSE, FALSE,
                                                         GIMP_UNIT_INCH,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_KEEP_ASPECT,
                                   g_param_spec_boolean ("keep-aspect",
                                                         NULL, NULL,
                                                         TRUE,
                                                         G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_EDIT_RESOLUTION,
                                   g_param_spec_boolean ("edit-resolution",
                                                         NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_size_box_init (GimpSizeBox *box)
{
  gtk_box_set_spacing (GTK_BOX (box), 6);
}

static GObject *
gimp_size_box_constructor (GType                  type,
                           guint                  n_params,
                           GObjectConstructParam *params)
{
  GObject            *object;
  GimpSizeBox        *box;
  GtkWidget          *entry;
  GtkWidget          *table;
  GtkWidget          *hbox;
  GtkWidget          *width;
  GtkWidget          *height;
  GtkWidget          *chain;
  GtkWidget          *vbox;
  GtkWidget          *label;
  GtkObject          *adjustment;
  GimpSizeBoxPrivate *priv;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  box = GIMP_SIZE_BOX (object);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_box_pack_start (GTK_BOX (box), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  width = gimp_spin_button_new (&adjustment,
                                1, 1, 1, 1, 10, 0,
                                1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (width), SB_WIDTH);

  height = gimp_spin_button_new (&adjustment,
                                 1, 1, 1, 1, 10, 0,
                                 1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (height), SB_WIDTH);

  entry = gimp_size_entry_new (0, box->unit,_("%p"),
                               TRUE, TRUE, FALSE, SB_WIDTH,
                               GIMP_SIZE_ENTRY_UPDATE_SIZE);

  label = gtk_label_new_with_mnemonic (_("_Width:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), width);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new_with_mnemonic (_("H_eight:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), height);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), hbox, 1, 2, 0, 2);
  gtk_widget_show (hbox);

  gtk_table_set_row_spacing (GTK_TABLE (entry), 0, 2);
  gtk_table_set_col_spacing (GTK_TABLE (entry), 1, 2);
  gtk_table_set_col_spacing (GTK_TABLE (entry), 2, 2);

  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
  gtk_widget_show (entry);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (entry),
			     GTK_SPIN_BUTTON (height), NULL);
  gtk_table_attach_defaults (GTK_TABLE (entry), height, 0, 1, 1, 2);
  gtk_widget_show (height);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (entry),
			     GTK_SPIN_BUTTON (width), NULL);
  gtk_table_attach_defaults (GTK_TABLE (entry), width, 0, 1, 0, 1);
  gtk_widget_show (width);

  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (entry), 0, 0, box->width);
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (entry), 1, 0, box->height);

  gimp_prop_coordinates_connect (G_OBJECT (box),
                                 "width", "height", "unit",
                                 entry, NULL,
                                 box->xresolution, box->yresolution);

  chain = gimp_chain_button_new (GIMP_CHAIN_RIGHT);
  gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (chain), TRUE);
  gtk_table_attach_defaults (GTK_TABLE (entry), chain, 1, 2, 0, 2);
  gtk_widget_show (chain);

  gimp_help_set_help_data (GIMP_CHAIN_BUTTON (chain)->button,
                           _("Keep aspect ratio"), NULL);

  vbox = gtk_vbox_new (2, FALSE);
  gtk_table_attach_defaults (GTK_TABLE (table), vbox, 1, 3, 2, 3);
  gtk_widget_show (vbox);

  label = gtk_label_new (NULL);
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL,
                             -1);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  priv = GIMP_SIZE_BOX_GET_PRIVATE (box);

  priv->entry       = GIMP_SIZE_ENTRY (entry);
  priv->chain       = GIMP_CHAIN_BUTTON (chain);
  priv->pixel_label = label;
  priv->aspect      = (gdouble) box->width / (gdouble) box->height;

  if (box->edit_resolution)
    {
      priv->res_label = NULL;
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

  return object;
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
      if (priv->chain && gimp_chain_button_get_active (priv->chain))
        {
          gint  height = ROUND ((gdouble) box->width / priv->aspect);

          if (box->height != height)
            gimp_size_entry_set_refval (priv->entry, 1, height);
        }
      gimp_size_box_update_size (box);
      break;

    case PROP_HEIGHT:
      box->height = g_value_get_int (value);
      if (priv->chain && gimp_chain_button_get_active (priv->chain))
        {
          gint  width = ROUND ((gdouble) box->height * priv->aspect);

          if (box->width != width)
            gimp_size_entry_set_refval (priv->entry, 0, width);
        }
      gimp_size_box_update_size (box);
      break;

    case PROP_UNIT:
      box->unit = g_value_get_int (value);
      break;

    case PROP_XRESOLUTION:
      box->xresolution = g_value_get_double (value);
      if (priv->entry)
        gimp_size_entry_set_resolution (priv->entry, 0, box->xresolution, TRUE);
      gimp_size_box_update_resolution (box);
      break;

    case PROP_YRESOLUTION:
      box->yresolution = g_value_get_double (value);
      if (priv->entry)
        gimp_size_entry_set_resolution (priv->entry, 1, box->yresolution, TRUE);
      gimp_size_box_update_resolution (box);
      break;

    case PROP_RESOLUTION_UNIT:
      box->resolution_unit = g_value_get_int (value);
      break;

    case PROP_KEEP_ASPECT:
      if (priv->chain)
        gimp_chain_button_set_active (priv->chain,
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
                           gimp_chain_button_get_active (priv->chain));
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
      gchar *text;

      text = g_strdup_printf (_("%d x %d pixels"), box->width, box->height);
      gtk_label_set_text (GTK_LABEL (priv->pixel_label), text);
      g_free (text);
    }
}

static void
gimp_size_box_update_resolution (GimpSizeBox *box)
{
  GimpSizeBoxPrivate *priv = GIMP_SIZE_BOX_GET_PRIVATE (box);

  if (priv->res_label)
    {
      gchar *text;

      if ((gint) box->xresolution != (gint) box->yresolution)
        text = g_strdup_printf (_("%d x %d dpi"),
                                (gint) box->xresolution,
                                (gint) box->yresolution);
      else
        text = g_strdup_printf (_("%d dpi"),
                                (gint) box->yresolution);

      gtk_label_set_text (GTK_LABEL (priv->res_label), text);
      g_free (text);
    }
}
