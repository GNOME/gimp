/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpImagePropView
 * Copyright (C) 2005  Michael Natterer <mitch@gimp.org>
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

#include "core/gimpcontainer.h"
#include "core/gimpimage.h"
#include "core/gimpunit.h"

#include "gimpimagepropview.h"
#include "gimppropwidgets.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_IMAGE
};


static void      gimp_image_prop_view_class_init   (GimpImagePropViewClass *klass);
static void      gimp_image_prop_view_init         (GimpImagePropView      *view);

static GObject * gimp_image_prop_view_constructor  (GType              type,
                                                    guint              n_params,
                                                    GObjectConstructParam *params);
static void      gimp_image_prop_view_set_property (GObject           *object,
                                                    guint              property_id,
                                                    const GValue      *value,
                                                    GParamSpec        *pspec);
static void      gimp_image_prop_view_get_property (GObject           *object,
                                                    guint              property_id,
                                                    GValue            *value,
                                                    GParamSpec        *pspec);

static void      gimp_image_prop_view_update       (GimpImagePropView *view);


static GtkTableClass *parent_class = NULL;


GType
gimp_image_prop_view_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpImagePropViewClass),
        NULL,           /* base_init      */
        NULL,           /* base_finalize  */
        (GClassInitFunc) gimp_image_prop_view_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpImagePropView),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_image_prop_view_init
      };

      view_type = g_type_register_static (GTK_TYPE_TABLE,
                                          "GimpImagePropView",
                                          &view_info, 0);
    }

  return view_type;
}

static void
gimp_image_prop_view_class_init (GimpImagePropViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor  = gimp_image_prop_view_constructor;
  object_class->set_property = gimp_image_prop_view_set_property;
  object_class->get_property = gimp_image_prop_view_get_property;

  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_object ("image", NULL, NULL,
                                                        GIMP_TYPE_IMAGE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_image_prop_view_init (GimpImagePropView *view)
{
  gtk_table_set_col_spacing (GTK_TABLE (view), 0, 6);
  gtk_table_set_row_spacings (GTK_TABLE (view), 4);
}

static void
gimp_image_prop_view_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpImagePropView *view = GIMP_IMAGE_PROP_VIEW (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      view->image = GIMP_IMAGE (g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_image_prop_view_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpImagePropView *view = GIMP_IMAGE_PROP_VIEW (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      g_value_set_object (value, view->image);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GObject *
gimp_image_prop_view_constructor (GType                  type,
                                  guint                  n_params,
                                  GObjectConstructParam *params)
{
  GimpImagePropView *view;
  GObject           *object;
  GtkWidget         *label;
  gint               row = 0;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  view = GIMP_IMAGE_PROP_VIEW (object);

  g_assert (view->image != NULL);

  view->pixel_size_label = label = gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gimp_table_attach_aligned (GTK_TABLE (view), 0, row++,
                             _("Pixel dimensions:"),
                             0.0, 0.5, label,
                             1, FALSE);

  view->print_size_label = label = gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gimp_table_attach_aligned (GTK_TABLE (view), 0, row++,
                             _("Print size:"),
                             0.0, 0.5, label,
                             1, FALSE);

  view->resolution_label = label = gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gimp_table_attach_aligned (GTK_TABLE (view), 0, row++,
                             _("Resolution:"),
                             0.0, 0.5, label,
                             1, FALSE);

  view->colorspace_label = label = gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gimp_table_attach_aligned (GTK_TABLE (view), 0, row++,
                             _("Color space:"),
                             0.0, 0.5, label,
                             1, FALSE);

  view->memsize_label =label =  gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gimp_table_attach_aligned (GTK_TABLE (view), 0, row++,
                             _("Size in memory:"),
                             0.0, 0.5, label,
                             1, FALSE);

  view->layers_label = label = gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gimp_table_attach_aligned (GTK_TABLE (view), 0, row++,
                             _("Number of layers:"),
                             0.0, 0.5, label,
                             1, FALSE);

  view->channels_label = label = gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gimp_table_attach_aligned (GTK_TABLE (view), 0, row++,
                             _("Number of channels:"),
                             0.0, 0.5, label,
                             1, FALSE);

  view->vectors_label = label = gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gimp_table_attach_aligned (GTK_TABLE (view), 0, row++,
                             _("Number of paths:"),
                             0.0, 0.5, label,
                             1, FALSE);

  g_signal_connect_object (view->image, "size-changed",
                           G_CALLBACK (gimp_image_prop_view_update),
                           G_OBJECT (view),
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (view->image, "resolution-changed",
                           G_CALLBACK (gimp_image_prop_view_update),
                           G_OBJECT (view),
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (view->image, "unit-changed",
                           G_CALLBACK (gimp_image_prop_view_update),
                           G_OBJECT (view),
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (view->image, "mode-changed",
                           G_CALLBACK (gimp_image_prop_view_update),
                           G_OBJECT (view),
                           G_CONNECT_SWAPPED);

  gimp_image_prop_view_update (view);

  return object;
}


/*  public functions  */

GtkWidget *
gimp_image_prop_view_new (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return g_object_new (GIMP_TYPE_IMAGE_PROP_VIEW,
                       "image", image,
                       NULL);
}


/*  private functions  */

static void
gimp_image_prop_view_update (GimpImagePropView *view)
{
  GimpImage         *image = view->image;
  GimpImageBaseType  type;
  GimpUnit           unit;
  gdouble            unit_factor;
  gint               unit_digits;
  gchar              format_buf[32];
  gchar              buf[256];

  /*  pixel size  */
  g_snprintf (buf, sizeof (buf), _("%d x %d pixels"),
              image->width, image->height);
  gtk_label_set_text (GTK_LABEL (view->pixel_size_label), buf);

  /*  print size  */
#if 0
  unit = GIMP_DISPLAY_SHELL (gdisp->shell)->unit;

  if (unit == GIMP_UNIT_PIXEL)
#endif
    unit = gimp_image_get_unit (image);

  unit_factor = _gimp_unit_get_factor (image->gimp, unit);
  unit_digits = _gimp_unit_get_digits (image->gimp, unit);

  g_snprintf (format_buf, sizeof (format_buf), "%%.%df x %%.%df %s",
              unit_digits + 1, unit_digits + 1,
              _gimp_unit_get_plural (image->gimp, unit));
  g_snprintf (buf, sizeof (buf), format_buf,
              image->width  * unit_factor / image->xresolution,
              image->height * unit_factor / image->yresolution);
  gtk_label_set_text (GTK_LABEL (view->print_size_label), buf);

  /*  resolution  */
  unit = gimp_image_get_unit (image);
  unit_factor = _gimp_unit_get_factor (image->gimp, unit);

  g_snprintf (format_buf, sizeof (format_buf), _("pixels/%s"),
              _gimp_unit_get_abbreviation (image->gimp, unit));
  g_snprintf (buf, sizeof (buf), _("%g x %g %s"),
              image->xresolution / unit_factor,
              image->yresolution / unit_factor,
              unit == GIMP_UNIT_INCH ? _("dpi") : format_buf);
  gtk_label_set_text (GTK_LABEL (view->resolution_label), buf);

  /*  color type  */
  type = gimp_image_base_type (image);

  switch (type)
    {
    case GIMP_RGB:
      g_snprintf (buf, sizeof (buf), "%s", _("RGB Color"));
      break;
    case GIMP_GRAY:
      g_snprintf (buf, sizeof (buf), "%s", _("Grayscale"));
      break;
    case GIMP_INDEXED:
      g_snprintf (buf, sizeof (buf), "%s (%d %s)",
                  _("Indexed Color"), image->num_cols, _("colors"));
      break;
    }

  gtk_label_set_text (GTK_LABEL (view->colorspace_label), buf);

  /*  size in memory  */
  {
    GimpObject *object = GIMP_OBJECT (image);
    gchar      *str;

    str = gimp_memsize_to_string (gimp_object_get_memsize (object, NULL));
    gtk_label_set_text (GTK_LABEL (view->memsize_label), str);
    g_free (str);
  }

  /*  number of layers  */
  g_snprintf (buf, sizeof (buf), "%d",
              gimp_container_num_children (image->layers));
  gtk_label_set_text (GTK_LABEL (view->layers_label), buf);

  /*  number of channels  */
  g_snprintf (buf, sizeof (buf), "%d",
              gimp_container_num_children (image->channels));
  gtk_label_set_text (GTK_LABEL (view->channels_label), buf);

  /*  number of vectors  */
  g_snprintf (buf, sizeof (buf), "%d",
              gimp_container_num_children (image->vectors));
  gtk_label_set_text (GTK_LABEL (view->vectors_label), buf);
}
