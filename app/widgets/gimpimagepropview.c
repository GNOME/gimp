/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpImagePropView
 * Copyright (C) 2005  Michael Natterer <mitch@gimp.org>
 * Copyright (C) 2006  Sven Neumann <sven@gimp.org>
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
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpimage.h"
#include "core/gimpimage-colormap.h"
#include "core/gimpimage-undo.h"
#include "core/gimppalette.h"
#include "core/gimpundostack.h"
#include "core/gimp-utils.h"

#include "plug-in/gimppluginmanager-file.h"
#include "plug-in/gimppluginprocedure.h"

#include "gimpimagepropview.h"
#include "gimppropwidgets.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_IMAGE
};


static void        gimp_image_prop_view_constructed  (GObject           *object);
static void        gimp_image_prop_view_set_property (GObject           *object,
                                                      guint              property_id,
                                                      const GValue      *value,
                                                      GParamSpec        *pspec);
static void        gimp_image_prop_view_get_property (GObject           *object,
                                                      guint              property_id,
                                                      GValue            *value,
                                                      GParamSpec        *pspec);

static GtkWidget * gimp_image_prop_view_add_label    (GtkGrid           *grid,
                                                      gint               row,
                                                      const gchar       *text);
static void        gimp_image_prop_view_undo_event   (GimpImage         *image,
                                                      GimpUndoEvent      event,
                                                      GimpUndo          *undo,
                                                      GimpImagePropView *view);
static void        gimp_image_prop_view_update       (GimpImagePropView *view);
static void        gimp_image_prop_view_file_update  (GimpImagePropView *view);
static void        gimp_image_prop_view_realize      (GimpImagePropView *view,
                                                      gpointer           user_data);


G_DEFINE_TYPE (GimpImagePropView, gimp_image_prop_view, GTK_TYPE_GRID)

#define parent_class gimp_image_prop_view_parent_class


static void
gimp_image_prop_view_class_init (GimpImagePropViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_image_prop_view_constructed;
  object_class->set_property = gimp_image_prop_view_set_property;
  object_class->get_property = gimp_image_prop_view_get_property;

  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_object ("image", NULL, NULL,
                                                        GIMP_TYPE_IMAGE,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_image_prop_view_init (GimpImagePropView *view)
{
  GtkGrid *grid = GTK_GRID (view);
  gint     row = 0;

  gtk_grid_set_column_spacing (grid, 6);
  gtk_grid_set_row_spacing (grid, 3);

  view->pixel_size_label =
    gimp_image_prop_view_add_label (grid, row++, _("Size in pixels:"));

  view->print_size_label =
    gimp_image_prop_view_add_label (grid, row++, _("Print size:"));

  view->resolution_label =
    gimp_image_prop_view_add_label (grid, row++, _("Resolution:"));

  view->colorspace_label =
    gimp_image_prop_view_add_label (grid, row++, _("Color space:"));

  view->precision_label =
    gimp_image_prop_view_add_label (grid, row++, _("Precision:"));

  gtk_widget_set_margin_bottom (view->precision_label, 12);

  view->filename_label =
    gimp_image_prop_view_add_label (grid, row++, _("File Name:"));

  gtk_label_set_ellipsize (GTK_LABEL (view->filename_label),
                           PANGO_ELLIPSIZE_MIDDLE);
  /* See gimp_image_prop_view_realize(). */
  gtk_label_set_max_width_chars (GTK_LABEL (view->filename_label), 25);

  view->filesize_label =
    gimp_image_prop_view_add_label (grid, row++, _("File Size:"));

  view->filetype_label =
    gimp_image_prop_view_add_label (grid, row++, _("File Type:"));

  gtk_widget_set_margin_bottom (view->filetype_label, 12);

  view->memsize_label =
    gimp_image_prop_view_add_label (grid, row++, _("Size in memory:"));

  view->undo_label =
    gimp_image_prop_view_add_label (grid, row++, _("Undo steps:"));

  view->redo_label =
    gimp_image_prop_view_add_label (grid, row++, _("Redo steps:"));

  gtk_widget_set_margin_bottom (view->redo_label, 12);

  view->pixels_label =
    gimp_image_prop_view_add_label (grid, row++, _("Number of pixels:"));

  view->layers_label =
    gimp_image_prop_view_add_label (grid, row++, _("Number of layers:"));

  view->channels_label =
    gimp_image_prop_view_add_label (grid, row++, _("Number of channels:"));

  view->vectors_label =
    gimp_image_prop_view_add_label (grid, row++, _("Number of paths:"));

  g_signal_connect (view, "realize",
                    G_CALLBACK (gimp_image_prop_view_realize),
                    NULL);
}

static void
gimp_image_prop_view_constructed (GObject *object)
{
  GimpImagePropView *view = GIMP_IMAGE_PROP_VIEW (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (view->image != NULL);

  g_signal_connect_object (view->image, "name-changed",
                           G_CALLBACK (gimp_image_prop_view_file_update),
                           G_OBJECT (view),
                           G_CONNECT_SWAPPED);

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
  g_signal_connect_object (view->image, "undo-event",
                           G_CALLBACK (gimp_image_prop_view_undo_event),
                           G_OBJECT (view),
                           0);

  gimp_image_prop_view_update (view);
  gimp_image_prop_view_file_update (view);
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
      view->image = g_value_get_object (value);
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

static GtkWidget *
gimp_image_prop_view_add_label (GtkGrid     *grid,
                                gint         row,
                                const gchar *text)
{
  GtkWidget *label;
  GtkWidget *desc;

  desc = g_object_new (GTK_TYPE_LABEL,
                       "label",  text,
                       "xalign", 1.0,
                       "yalign", 0.0,
                       NULL);
  gimp_label_set_attributes (GTK_LABEL (desc),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_grid_attach (grid, desc, 0, row, 1, 1);
  gtk_widget_show (desc);

  label = g_object_new (GTK_TYPE_LABEL,
                        "xalign",     0.0,
                        "yalign",     0.5,
                        "selectable", TRUE,
                        NULL);

  gtk_grid_attach (grid, label, 1, row, 1, 1);

  gtk_widget_show (label);

  return label;
}

static void
gimp_image_prop_view_label_set_memsize (GtkWidget  *label,
                                        GimpObject *object)
{
  gchar *str = g_format_size (gimp_object_get_memsize (object, NULL));
  gtk_label_set_text (GTK_LABEL (label), str);
  g_free (str);
}

static void
gimp_image_prop_view_label_set_filename (GtkWidget *label,
                                         GimpImage *image)
{
  GFile *file = gimp_image_get_any_file (image);

  if (file)
    {
      gtk_label_set_text (GTK_LABEL (label),
                          gimp_file_get_utf8_name (file));
      /* In case the label is ellipsized. */
      gtk_widget_set_tooltip_text (GTK_WIDGET (label),
                                   gimp_file_get_utf8_name (file));
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (label), NULL);
      gimp_help_set_help_data (gtk_widget_get_parent (label), NULL, NULL);
    }
}

static void
gimp_image_prop_view_label_set_filesize (GtkWidget *label,
                                         GimpImage *image)
{
  GFile *file = gimp_image_get_any_file (image);

  if (file)
    {
      GFileInfo *info = g_file_query_info (file,
                                           G_FILE_ATTRIBUTE_STANDARD_SIZE,
                                           G_FILE_QUERY_INFO_NONE,
                                           NULL, NULL);

      if (info)
        {
          goffset  size = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_STANDARD_SIZE);
          gchar   *str  = g_format_size (size);

          gtk_label_set_text (GTK_LABEL (label), str);
          g_free (str);

          g_object_unref (info);
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (label), NULL);
        }
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (label), NULL);
    }
}

static void
gimp_image_prop_view_label_set_filetype (GtkWidget *label,
                                         GimpImage *image)
{
  GimpPlugInProcedure *proc = gimp_image_get_save_proc (image);

  if (! proc)
    proc = gimp_image_get_load_proc (image);

  if (! proc)
    {
      GimpPlugInManager *manager = image->gimp->plug_in_manager;
      GFile             *file    = gimp_image_get_file (image);

      if (file)
        proc = gimp_plug_in_manager_file_procedure_find (manager,
                                                         GIMP_FILE_PROCEDURE_GROUP_OPEN,
                                                         file, NULL);
    }

  gtk_label_set_text (GTK_LABEL (label),
                      proc ?
                      gimp_procedure_get_label (GIMP_PROCEDURE (proc)) : NULL);
}

static void
gimp_image_prop_view_label_set_undo (GtkWidget     *label,
                                     GimpUndoStack *stack)
{
  gint steps = gimp_undo_stack_get_depth (stack);

  if (steps > 0)
    {
      GimpObject *object = GIMP_OBJECT (stack);
      gchar      *str;
      gchar       buf[256];

      str = g_format_size (gimp_object_get_memsize (object, NULL));
      g_snprintf (buf, sizeof (buf), "%d (%s)", steps, str);
      g_free (str);

      gtk_label_set_text (GTK_LABEL (label), buf);
    }
  else
    {
      /*  no undo (or redo) steps available  */
      gtk_label_set_text (GTK_LABEL (label), _("None"));
    }
}

static void
gimp_image_prop_view_undo_event (GimpImage         *image,
                                 GimpUndoEvent      event,
                                 GimpUndo          *undo,
                                 GimpImagePropView *view)
{
  gimp_image_prop_view_update (view);
}

static void
gimp_image_prop_view_update (GimpImagePropView *view)
{
  GimpImage         *image = view->image;
  GimpColorProfile  *profile;
  GimpImageBaseType  type;
  GimpPrecision      precision;
  GimpUnit          *unit;
  gdouble            unit_factor;
  const gchar       *desc;
  gchar              format_buf[32];
  gchar              buf[256];
  gdouble            xres;
  gdouble            yres;

  gimp_image_get_resolution (image, &xres, &yres);

  /*  pixel size  */
  g_snprintf (buf, sizeof (buf), ngettext ("%d × %d pixel",
                                           "%d × %d pixels",
                                           gimp_image_get_height (image)),
              gimp_image_get_width  (image),
              gimp_image_get_height (image));
  gtk_label_set_text (GTK_LABEL (view->pixel_size_label), buf);

  /*  print size  */
  unit = gimp_get_default_unit ();

  g_snprintf (format_buf, sizeof (format_buf), "%%.%df × %%.%df %s",
              gimp_unit_get_scaled_digits (unit, xres),
              gimp_unit_get_scaled_digits (unit, yres),
              gimp_unit_get_name (unit));
  g_snprintf (buf, sizeof (buf), format_buf,
              gimp_pixels_to_units (gimp_image_get_width  (image), unit, xres),
              gimp_pixels_to_units (gimp_image_get_height (image), unit, yres));
  gtk_label_set_text (GTK_LABEL (view->print_size_label), buf);

  /*  resolution  */
  unit        = gimp_image_get_unit (image);
  unit_factor = gimp_unit_get_factor (unit);

  g_snprintf (format_buf, sizeof (format_buf), _("pixels/%s"),
              gimp_unit_get_abbreviation (unit));
  g_snprintf (buf, sizeof (buf), _("%g × %g %s"),
              xres / unit_factor,
              yres / unit_factor,
              unit == gimp_unit_inch () ? _("ppi") : format_buf);
  gtk_label_set_text (GTK_LABEL (view->resolution_label), buf);

  /*  color space  */
  type = gimp_image_get_base_type (image);
  gimp_enum_get_value (GIMP_TYPE_IMAGE_BASE_TYPE, type,
                       NULL, NULL, &desc, NULL);

  switch (type)
    {
    case GIMP_RGB:
    case GIMP_GRAY:
      profile = gimp_color_managed_get_color_profile (GIMP_COLOR_MANAGED (image));
      g_snprintf (buf, sizeof (buf), "%s: %s", desc,
                  gimp_color_profile_get_label (profile));
      break;
    case GIMP_INDEXED:
        {
          gint n_colors;

          n_colors = gimp_palette_get_n_colors (gimp_image_get_colormap_palette (image));
          g_snprintf (buf, sizeof (buf),
                      ngettext ("Indexed color (monochrome)",
                                "Indexed color (%d colors)",
                                n_colors), n_colors);
        }
      break;
    }

  gtk_label_set_text (GTK_LABEL (view->colorspace_label), buf);
  gtk_label_set_line_wrap (GTK_LABEL (view->colorspace_label), TRUE);

  /*  precision  */
  precision = gimp_image_get_precision (image);

  gimp_enum_get_value (GIMP_TYPE_PRECISION, precision,
                       NULL, NULL, &desc, NULL);

  gtk_label_set_text (GTK_LABEL (view->precision_label), desc);

  /*  size in memory  */
  gimp_image_prop_view_label_set_memsize (view->memsize_label,
                                          GIMP_OBJECT (image));

  /*  undo / redo  */
  gimp_image_prop_view_label_set_undo (view->undo_label,
                                       gimp_image_get_undo_stack (image));
  gimp_image_prop_view_label_set_undo (view->redo_label,
                                       gimp_image_get_redo_stack (image));

  /*  number of layers  */
  g_snprintf (buf, sizeof (buf), "%d",
              gimp_image_get_width  (image) *
              gimp_image_get_height (image));
  gtk_label_set_text (GTK_LABEL (view->pixels_label), buf);

  /*  number of layers  */
  g_snprintf (buf, sizeof (buf), "%d",
              gimp_image_get_n_layers (image));
  gtk_label_set_text (GTK_LABEL (view->layers_label), buf);

  /*  number of channels  */
  g_snprintf (buf, sizeof (buf), "%d",
              gimp_image_get_n_channels (image));
  gtk_label_set_text (GTK_LABEL (view->channels_label), buf);

  /*  number of vectors  */
  g_snprintf (buf, sizeof (buf), "%d",
              gimp_image_get_n_paths (image));
  gtk_label_set_text (GTK_LABEL (view->vectors_label), buf);
}

static void
gimp_image_prop_view_file_update (GimpImagePropView *view)
{
  GimpImage *image = view->image;

  /*  filename  */
  gimp_image_prop_view_label_set_filename (view->filename_label, image);

  /*  filesize  */
  gimp_image_prop_view_label_set_filesize (view->filesize_label, image);

  /*  filetype  */
  gimp_image_prop_view_label_set_filetype (view->filetype_label, image);
}

static void
gimp_image_prop_view_realize (GimpImagePropView *view,
                              gpointer           user_data)
{
  /* Ugly trick to avoid extra-wide dialog at construction because of
   * overlong file path. Basically I give a reasonable max size at
   * construction (if the path is longer, it is just ellipsized per set
   * rules), then once the widget is realized, I remove the max size,
   * allowing the widget to grow wider if ever the dialog were
   * manually resized (we don't want to keep the label short and
   * ellipsized if the dialog is explicitly made to have enough place).
   */
  gtk_label_set_max_width_chars (GTK_LABEL (view->filename_label), -1);
}
