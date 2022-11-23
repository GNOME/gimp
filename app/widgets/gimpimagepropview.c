/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaImagePropView
 * Copyright (C) 2005  Michael Natterer <mitch@ligma.org>
 * Copyright (C) 2006  Sven Neumann <sven@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-colormap.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmaundostack.h"
#include "core/ligma-utils.h"

#include "plug-in/ligmapluginmanager-file.h"
#include "plug-in/ligmapluginprocedure.h"

#include "ligmaimagepropview.h"
#include "ligmapropwidgets.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_IMAGE
};


static void        ligma_image_prop_view_constructed  (GObject           *object);
static void        ligma_image_prop_view_set_property (GObject           *object,
                                                      guint              property_id,
                                                      const GValue      *value,
                                                      GParamSpec        *pspec);
static void        ligma_image_prop_view_get_property (GObject           *object,
                                                      guint              property_id,
                                                      GValue            *value,
                                                      GParamSpec        *pspec);

static GtkWidget * ligma_image_prop_view_add_label    (GtkGrid           *grid,
                                                      gint               row,
                                                      const gchar       *text);
static void        ligma_image_prop_view_undo_event   (LigmaImage         *image,
                                                      LigmaUndoEvent      event,
                                                      LigmaUndo          *undo,
                                                      LigmaImagePropView *view);
static void        ligma_image_prop_view_update       (LigmaImagePropView *view);
static void        ligma_image_prop_view_file_update  (LigmaImagePropView *view);
static void        ligma_image_prop_view_realize      (LigmaImagePropView *view,
                                                      gpointer           user_data);


G_DEFINE_TYPE (LigmaImagePropView, ligma_image_prop_view, GTK_TYPE_GRID)

#define parent_class ligma_image_prop_view_parent_class


static void
ligma_image_prop_view_class_init (LigmaImagePropViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = ligma_image_prop_view_constructed;
  object_class->set_property = ligma_image_prop_view_set_property;
  object_class->get_property = ligma_image_prop_view_get_property;

  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_object ("image", NULL, NULL,
                                                        LIGMA_TYPE_IMAGE,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_image_prop_view_init (LigmaImagePropView *view)
{
  GtkGrid *grid = GTK_GRID (view);
  gint     row = 0;

  gtk_grid_set_column_spacing (grid, 6);
  gtk_grid_set_row_spacing (grid, 3);

  view->pixel_size_label =
    ligma_image_prop_view_add_label (grid, row++, _("Size in pixels:"));

  view->print_size_label =
    ligma_image_prop_view_add_label (grid, row++, _("Print size:"));

  view->resolution_label =
    ligma_image_prop_view_add_label (grid, row++, _("Resolution:"));

  view->colorspace_label =
    ligma_image_prop_view_add_label (grid, row++, _("Color space:"));

  view->precision_label =
    ligma_image_prop_view_add_label (grid, row++, _("Precision:"));

  gtk_widget_set_margin_bottom (view->precision_label, 12);

  view->filename_label =
    ligma_image_prop_view_add_label (grid, row++, _("File Name:"));

  gtk_label_set_ellipsize (GTK_LABEL (view->filename_label),
                           PANGO_ELLIPSIZE_MIDDLE);
  /* See ligma_image_prop_view_realize(). */
  gtk_label_set_max_width_chars (GTK_LABEL (view->filename_label), 25);

  view->filesize_label =
    ligma_image_prop_view_add_label (grid, row++, _("File Size:"));

  view->filetype_label =
    ligma_image_prop_view_add_label (grid, row++, _("File Type:"));

  gtk_widget_set_margin_bottom (view->filetype_label, 12);

  view->memsize_label =
    ligma_image_prop_view_add_label (grid, row++, _("Size in memory:"));

  view->undo_label =
    ligma_image_prop_view_add_label (grid, row++, _("Undo steps:"));

  view->redo_label =
    ligma_image_prop_view_add_label (grid, row++, _("Redo steps:"));

  gtk_widget_set_margin_bottom (view->redo_label, 12);

  view->pixels_label =
    ligma_image_prop_view_add_label (grid, row++, _("Number of pixels:"));

  view->layers_label =
    ligma_image_prop_view_add_label (grid, row++, _("Number of layers:"));

  view->channels_label =
    ligma_image_prop_view_add_label (grid, row++, _("Number of channels:"));

  view->vectors_label =
    ligma_image_prop_view_add_label (grid, row++, _("Number of paths:"));

  g_signal_connect (view, "realize",
                    G_CALLBACK (ligma_image_prop_view_realize),
                    NULL);
}

static void
ligma_image_prop_view_constructed (GObject *object)
{
  LigmaImagePropView *view = LIGMA_IMAGE_PROP_VIEW (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (view->image != NULL);

  g_signal_connect_object (view->image, "name-changed",
                           G_CALLBACK (ligma_image_prop_view_file_update),
                           G_OBJECT (view),
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (view->image, "size-changed",
                           G_CALLBACK (ligma_image_prop_view_update),
                           G_OBJECT (view),
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (view->image, "resolution-changed",
                           G_CALLBACK (ligma_image_prop_view_update),
                           G_OBJECT (view),
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (view->image, "unit-changed",
                           G_CALLBACK (ligma_image_prop_view_update),
                           G_OBJECT (view),
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (view->image, "mode-changed",
                           G_CALLBACK (ligma_image_prop_view_update),
                           G_OBJECT (view),
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (view->image, "undo-event",
                           G_CALLBACK (ligma_image_prop_view_undo_event),
                           G_OBJECT (view),
                           0);

  ligma_image_prop_view_update (view);
  ligma_image_prop_view_file_update (view);
}

static void
ligma_image_prop_view_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  LigmaImagePropView *view = LIGMA_IMAGE_PROP_VIEW (object);

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
ligma_image_prop_view_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  LigmaImagePropView *view = LIGMA_IMAGE_PROP_VIEW (object);

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
ligma_image_prop_view_new (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return g_object_new (LIGMA_TYPE_IMAGE_PROP_VIEW,
                       "image", image,
                       NULL);
}


/*  private functions  */

static GtkWidget *
ligma_image_prop_view_add_label (GtkGrid     *grid,
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
  ligma_label_set_attributes (GTK_LABEL (desc),
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
ligma_image_prop_view_label_set_memsize (GtkWidget  *label,
                                        LigmaObject *object)
{
  gchar *str = g_format_size (ligma_object_get_memsize (object, NULL));
  gtk_label_set_text (GTK_LABEL (label), str);
  g_free (str);
}

static void
ligma_image_prop_view_label_set_filename (GtkWidget *label,
                                         LigmaImage *image)
{
  GFile *file = ligma_image_get_any_file (image);

  if (file)
    {
      gtk_label_set_text (GTK_LABEL (label),
                          ligma_file_get_utf8_name (file));
      /* In case the label is ellipsized. */
      gtk_widget_set_tooltip_text (GTK_WIDGET (label),
                                   ligma_file_get_utf8_name (file));
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (label), NULL);
      ligma_help_set_help_data (gtk_widget_get_parent (label), NULL, NULL);
    }
}

static void
ligma_image_prop_view_label_set_filesize (GtkWidget *label,
                                         LigmaImage *image)
{
  GFile *file = ligma_image_get_any_file (image);

  if (file)
    {
      GFileInfo *info = g_file_query_info (file,
                                           G_FILE_ATTRIBUTE_STANDARD_SIZE,
                                           G_FILE_QUERY_INFO_NONE,
                                           NULL, NULL);

      if (info)
        {
          goffset  size = g_file_info_get_size (info);
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
ligma_image_prop_view_label_set_filetype (GtkWidget *label,
                                         LigmaImage *image)
{
  LigmaPlugInProcedure *proc = ligma_image_get_save_proc (image);

  if (! proc)
    proc = ligma_image_get_load_proc (image);

  if (! proc)
    {
      LigmaPlugInManager *manager = image->ligma->plug_in_manager;
      GFile             *file    = ligma_image_get_file (image);

      if (file)
        proc = ligma_plug_in_manager_file_procedure_find (manager,
                                                         LIGMA_FILE_PROCEDURE_GROUP_OPEN,
                                                         file, NULL);
    }

  gtk_label_set_text (GTK_LABEL (label),
                      proc ?
                      ligma_procedure_get_label (LIGMA_PROCEDURE (proc)) : NULL);
}

static void
ligma_image_prop_view_label_set_undo (GtkWidget     *label,
                                     LigmaUndoStack *stack)
{
  gint steps = ligma_undo_stack_get_depth (stack);

  if (steps > 0)
    {
      LigmaObject *object = LIGMA_OBJECT (stack);
      gchar      *str;
      gchar       buf[256];

      str = g_format_size (ligma_object_get_memsize (object, NULL));
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
ligma_image_prop_view_undo_event (LigmaImage         *image,
                                 LigmaUndoEvent      event,
                                 LigmaUndo          *undo,
                                 LigmaImagePropView *view)
{
  ligma_image_prop_view_update (view);
}

static void
ligma_image_prop_view_update (LigmaImagePropView *view)
{
  LigmaImage         *image = view->image;
  LigmaColorProfile  *profile;
  LigmaImageBaseType  type;
  LigmaPrecision      precision;
  LigmaUnit           unit;
  gdouble            unit_factor;
  const gchar       *desc;
  gchar              format_buf[32];
  gchar              buf[256];
  gdouble            xres;
  gdouble            yres;

  ligma_image_get_resolution (image, &xres, &yres);

  /*  pixel size  */
  g_snprintf (buf, sizeof (buf), ngettext ("%d × %d pixel",
                                           "%d × %d pixels",
                                           ligma_image_get_height (image)),
              ligma_image_get_width  (image),
              ligma_image_get_height (image));
  gtk_label_set_text (GTK_LABEL (view->pixel_size_label), buf);

  /*  print size  */
  unit = ligma_get_default_unit ();

  g_snprintf (format_buf, sizeof (format_buf), "%%.%df × %%.%df %s",
              ligma_unit_get_scaled_digits (unit, xres),
              ligma_unit_get_scaled_digits (unit, yres),
              ligma_unit_get_plural (unit));
  g_snprintf (buf, sizeof (buf), format_buf,
              ligma_pixels_to_units (ligma_image_get_width  (image), unit, xres),
              ligma_pixels_to_units (ligma_image_get_height (image), unit, yres));
  gtk_label_set_text (GTK_LABEL (view->print_size_label), buf);

  /*  resolution  */
  unit        = ligma_image_get_unit (image);
  unit_factor = ligma_unit_get_factor (unit);

  g_snprintf (format_buf, sizeof (format_buf), _("pixels/%s"),
              ligma_unit_get_abbreviation (unit));
  g_snprintf (buf, sizeof (buf), _("%g × %g %s"),
              xres / unit_factor,
              yres / unit_factor,
              unit == LIGMA_UNIT_INCH ? _("ppi") : format_buf);
  gtk_label_set_text (GTK_LABEL (view->resolution_label), buf);

  /*  color space  */
  type = ligma_image_get_base_type (image);
  ligma_enum_get_value (LIGMA_TYPE_IMAGE_BASE_TYPE, type,
                       NULL, NULL, &desc, NULL);

  switch (type)
    {
    case LIGMA_RGB:
    case LIGMA_GRAY:
      profile = ligma_color_managed_get_color_profile (LIGMA_COLOR_MANAGED (image));
      g_snprintf (buf, sizeof (buf), "%s: %s", desc,
                  ligma_color_profile_get_label (profile));
      break;
    case LIGMA_INDEXED:
      g_snprintf (buf, sizeof (buf),
                  ngettext ("Indexed color (monochrome)",
                            "Indexed color (%d colors)",
                            ligma_image_get_colormap_size (image)),
                  ligma_image_get_colormap_size (image));
      break;
    }

  gtk_label_set_text (GTK_LABEL (view->colorspace_label), buf);
  gtk_label_set_line_wrap (GTK_LABEL (view->colorspace_label), TRUE);

  /*  precision  */
  precision = ligma_image_get_precision (image);

  ligma_enum_get_value (LIGMA_TYPE_PRECISION, precision,
                       NULL, NULL, &desc, NULL);

  gtk_label_set_text (GTK_LABEL (view->precision_label), desc);

  /*  size in memory  */
  ligma_image_prop_view_label_set_memsize (view->memsize_label,
                                          LIGMA_OBJECT (image));

  /*  undo / redo  */
  ligma_image_prop_view_label_set_undo (view->undo_label,
                                       ligma_image_get_undo_stack (image));
  ligma_image_prop_view_label_set_undo (view->redo_label,
                                       ligma_image_get_redo_stack (image));

  /*  number of layers  */
  g_snprintf (buf, sizeof (buf), "%d",
              ligma_image_get_width  (image) *
              ligma_image_get_height (image));
  gtk_label_set_text (GTK_LABEL (view->pixels_label), buf);

  /*  number of layers  */
  g_snprintf (buf, sizeof (buf), "%d",
              ligma_image_get_n_layers (image));
  gtk_label_set_text (GTK_LABEL (view->layers_label), buf);

  /*  number of channels  */
  g_snprintf (buf, sizeof (buf), "%d",
              ligma_image_get_n_channels (image));
  gtk_label_set_text (GTK_LABEL (view->channels_label), buf);

  /*  number of vectors  */
  g_snprintf (buf, sizeof (buf), "%d",
              ligma_image_get_n_vectors (image));
  gtk_label_set_text (GTK_LABEL (view->vectors_label), buf);
}

static void
ligma_image_prop_view_file_update (LigmaImagePropView *view)
{
  LigmaImage *image = view->image;

  /*  filename  */
  ligma_image_prop_view_label_set_filename (view->filename_label, image);

  /*  filesize  */
  ligma_image_prop_view_label_set_filesize (view->filesize_label, image);

  /*  filetype  */
  ligma_image_prop_view_label_set_filetype (view->filetype_label, image);
}

static void
ligma_image_prop_view_realize (LigmaImagePropView *view,
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
