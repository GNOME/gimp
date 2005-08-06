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

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>
#include <fcntl.h>

#include <glib-object.h>
#include <glib/gstdio.h>

#ifdef G_OS_WIN32
#include "libgimpbase/gimpwin32-io.h"
#endif

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "base/pixel-region.h"

#include "gimpcontainer.h"
#include "gimpgradient.h"
#include "gimpimage.h"
#include "gimppalette.h"
#include "gimpprojection.h"

#include "gimp-intl.h"


#define MAX_IMAGE_COLORS (10000 * 2)


/*  create a palette from a gradient  ****************************************/

GimpPalette *
gimp_palette_import_from_gradient (GimpGradient *gradient,
                                   gboolean      reverse,
				   const gchar  *palette_name,
				   gint          n_colors)
{
  GimpPalette         *palette;
  GimpGradientSegment *seg = NULL;
  gdouble              dx, cur_x;
  GimpRGB              color;
  gint                 i;

  g_return_val_if_fail (GIMP_IS_GRADIENT (gradient), NULL);
  g_return_val_if_fail (palette_name != NULL, NULL);
  g_return_val_if_fail (n_colors > 1, NULL);

  palette = GIMP_PALETTE (gimp_palette_new (palette_name));

  dx = 1.0 / (n_colors - 1);

  for (i = 0, cur_x = 0; i < n_colors; i++, cur_x += dx)
    {
      seg = gimp_gradient_get_color_at (gradient, seg, cur_x, reverse, &color);
      gimp_palette_add_entry (palette, -1, NULL, &color);
    }

  return palette;
}


/*  create a palette from a non-indexed image  *******************************/

typedef struct _ImgColors ImgColors;

struct _ImgColors
{
  guint  count;
  guint  r_adj;
  guint  g_adj;
  guint  b_adj;
  guchar r;
  guchar g;
  guchar b;
};

static gint count_color_entries = 0;

static GHashTable *
gimp_palette_import_store_colors (GHashTable *h_array,
				  guchar     *colors,
				  guchar     *colors_real,
				  gint        n_colors)
{
  gpointer   found_color = NULL;
  ImgColors *new_color;
  guint      key_colors = colors[0] * 256 * 256 + colors[1] * 256 + colors[2];

  if (h_array == NULL)
    {
      h_array = g_hash_table_new (g_direct_hash, g_direct_equal);
      count_color_entries = 0;
    }
  else
    {
      found_color = g_hash_table_lookup (h_array,
                                         GUINT_TO_POINTER (key_colors));
    }

  if (found_color == NULL)
    {
      if (count_color_entries > MAX_IMAGE_COLORS)
	{
	  /* Don't add any more new ones */
	  return h_array;
	}

      count_color_entries++;

      new_color = g_new (ImgColors, 1);

      new_color->count = 1;
      new_color->r_adj = 0;
      new_color->g_adj = 0;
      new_color->b_adj = 0;
      new_color->r     = colors[0];
      new_color->g     = colors[1];
      new_color->b     = colors[2];

      g_hash_table_insert (h_array, GUINT_TO_POINTER (key_colors), new_color);
    }
  else
    {
      new_color = (ImgColors *) found_color;

      if (new_color->count < (G_MAXINT - 1))
	new_color->count++;

      /* Now do the adjustments ...*/
      new_color->r_adj += (colors_real[0] - colors[0]);
      new_color->g_adj += (colors_real[1] - colors[1]);
      new_color->b_adj += (colors_real[2] - colors[2]);

      /* Boundary conditions */
      if(new_color->r_adj > (G_MAXINT - 255))
	new_color->r_adj /= new_color->count;

      if(new_color->g_adj > (G_MAXINT - 255))
	new_color->g_adj /= new_color->count;

      if(new_color->b_adj > (G_MAXINT - 255))
	new_color->b_adj /= new_color->count;
    }

  return h_array;
}

static void
gimp_palette_import_create_sorted_list (gpointer key,
					gpointer value,
					gpointer user_data)
{
  GSList    **sorted_list = (GSList**) user_data;
  ImgColors  *color_tab   = (ImgColors *) value;

  *sorted_list = g_slist_prepend (*sorted_list, color_tab);
}

static gint
gimp_palette_import_sort_colors (gconstpointer a,
				 gconstpointer b)
{
  ImgColors *s1 = (ImgColors *) a;
  ImgColors *s2 = (ImgColors *) b;

  if(s1->count > s2->count)
    return -1;
  if(s1->count < s2->count)
    return 1;

  return 0;
}

static void
gimp_palette_import_create_image_palette (gpointer data,
					  gpointer user_data)
{
  GimpPalette *palette;
  ImgColors   *color_tab;
  gint         n_colors;
  gchar       *lab;
  GimpRGB      color;

  palette   = (GimpPalette *) user_data;
  color_tab = (ImgColors *) data;

  n_colors = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (palette),
						 "import_n_colors"));

  if (palette->n_colors >= n_colors)
    return;

  lab = g_strdup_printf ("%s (occurs %u)", _("Untitled"), color_tab->count);

  /* Adjust the colors to the mean of the the sample */
  gimp_rgba_set_uchar
    (&color,
     (guchar) color_tab->r + (color_tab->r_adj / color_tab->count),
     (guchar) color_tab->g + (color_tab->g_adj / color_tab->count),
     (guchar) color_tab->b + (color_tab->b_adj / color_tab->count),
     255);

  gimp_palette_add_entry (palette, -1, lab, &color);

  g_free (lab);
}

static GimpPalette *
gimp_palette_import_image_make_palette (GHashTable  *h_array,
					const gchar *palette_name,
					gint         n_colors)
{
  GimpPalette *palette;
  GSList      *sorted_list = NULL;

  g_hash_table_foreach (h_array, gimp_palette_import_create_sorted_list,
			&sorted_list);
  sorted_list = g_slist_sort (sorted_list, gimp_palette_import_sort_colors);

  palette = GIMP_PALETTE (gimp_palette_new (palette_name));

  g_object_set_data (G_OBJECT (palette), "import_n_colors",
		     GINT_TO_POINTER (n_colors));

  g_slist_foreach (sorted_list, gimp_palette_import_create_image_palette,
		   palette);

  g_object_set_data (G_OBJECT (palette), "import_n_colors", NULL);

  /*  Free up used memory
   *  Note the same structure is on both the hash list and the sorted
   *  list. So only delete it once.
   */
  g_hash_table_destroy (h_array);

  g_slist_foreach (sorted_list, (GFunc) g_free, NULL);

  g_slist_free (sorted_list);

  return palette;
}

GimpPalette *
gimp_palette_import_from_image (GimpImage   *gimage,
				const gchar *palette_name,
				gint         n_colors,
				gint         threshold)
{
  PixelRegion    imagePR;
  guchar        *image_data;
  guchar        *idata;
  guchar         rgb[MAX_CHANNELS];
  guchar         rgb_real[MAX_CHANNELS];
  gboolean       has_alpha, indexed;
  gint           width, height;
  gint           bytes, alpha;
  gint           i, j;
  void          *pr;
  GimpImageType  d_type;
  GHashTable    *store_array = NULL;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (palette_name != NULL, NULL);
  g_return_val_if_fail (n_colors > 1, NULL);
  g_return_val_if_fail (threshold > 0, NULL);

  /*  Get the image information  */
  d_type    = gimp_projection_get_image_type (gimage->projection);
  bytes     = gimp_projection_get_bytes (gimage->projection);
  has_alpha = GIMP_IMAGE_TYPE_HAS_ALPHA (d_type);
  indexed   = GIMP_IMAGE_TYPE_IS_INDEXED (d_type);
  width     = gimage->width;
  height    = gimage->height;

  pixel_region_init (&imagePR, gimp_projection_get_tiles (gimage->projection),
                     0, 0, width, height, FALSE);

  alpha = bytes - 1;

  /*  iterate over the entire image  */
  for (pr = pixel_regions_register (1, &imagePR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      image_data = imagePR.data;

      for (i = 0; i < imagePR.h; i++)
	{
	  idata = image_data;

	  for (j = 0; j < imagePR.w; j++)
	    {
	      /*  Get the rgb values for the color  */
	      gimp_image_get_color (gimage, d_type, idata, rgb);
	      memcpy (rgb_real, rgb, MAX_CHANNELS); /* Structure copy */

	      rgb[0] = (rgb[0] / threshold) * threshold;
	      rgb[1] = (rgb[1] / threshold) * threshold;
	      rgb[2] = (rgb[2] / threshold) * threshold;

	      store_array =
		gimp_palette_import_store_colors (store_array, rgb, rgb_real,
						  n_colors);

	      idata += bytes;
	    }

	  image_data += imagePR.rowstride;
	}
    }

  return gimp_palette_import_image_make_palette (store_array,
						 palette_name,
						 n_colors);
}

/*  create a palette from an indexed image  **********************************/

GimpPalette *
gimp_palette_import_from_indexed_image (GimpImage   *gimage,
					const gchar *palette_name)
{
  GimpPalette *palette;
  gint         count;
  GimpRGB      color;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (gimp_image_base_type (gimage) == GIMP_INDEXED, NULL);
  g_return_val_if_fail (palette_name != NULL, NULL);

  palette = GIMP_PALETTE (gimp_palette_new (palette_name));

  for (count= 0; count < gimage->num_cols; ++count)
    {
      gimp_rgba_set_uchar (&color,
			   gimage->cmap[count * 3],
			   gimage->cmap[count * 3 + 1],
			   gimage->cmap[count * 3 + 2],
			   255);

      gimp_palette_add_entry (palette, -1, NULL, &color);
    }

  return palette;
}


/*  create a palette from a file  **********************************/

typedef enum
{
  GIMP_PALETTE_FILE_FORMAT_UNKNOWN,
  GIMP_PALETTE_FILE_FORMAT_GPL,      /* GIMP palette                        */
  GIMP_PALETTE_FILE_FORMAT_RIFF_PAL, /* RIFF palette                        */
  GIMP_PALETTE_FILE_FORMAT_ACT,      /* Photoshop binary color palette      */
  GIMP_PALETTE_FILE_FORMAT_PSP_PAL   /* JASC's Paint Shop Pro color palette */
} GimpPaletteFileFormat;

static GimpPaletteFileFormat
gimp_palette_detect_file_format (const gchar *filename)
{
  GimpPaletteFileFormat format = GIMP_PALETTE_FILE_FORMAT_UNKNOWN;
  gint                  fd;
  gchar                 header[16];
  struct stat           file_stat;

  fd = g_open (filename, O_RDONLY, 0);
  if (fd)
    {
      if (read (fd, header, sizeof (header)) == sizeof (header))
        {
	  if (strncmp (header + 0, "RIFF", 4) == 0 &&
              strncmp (header + 8, "PAL data", 8) == 0)
 	    {
              format = GIMP_PALETTE_FILE_FORMAT_RIFF_PAL;
	    }

	  if (strncmp (header, "GIMP Palette", 12) == 0)
	    {
              format = GIMP_PALETTE_FILE_FORMAT_GPL;
	    }

          if (strncmp (header, "JASC-PAL", 8) == 0)
            {
              format = GIMP_PALETTE_FILE_FORMAT_PSP_PAL;
            }
	}

      if (fstat (fd, &file_stat) >= 0)
        {
          if (file_stat.st_size == 768)
            format = GIMP_PALETTE_FILE_FORMAT_ACT;
        }

      close (fd);
    }

  return format;
}

GimpPalette *
gimp_palette_import_from_file (const gchar  *filename,
                               const gchar  *palette_name,
                               GError      **error)
{
  GimpPalette *palette = NULL;
  GList       *palette_list;
  GimpRGB      color;
  gint         fd;
  guchar       color_bytes[4];

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (palette_name != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  fd = g_open (filename, O_RDONLY, 0);
  if (! fd)
    {
      g_set_error (error,
                   G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Opening '%s' failed: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return NULL;
    }

  switch (gimp_palette_detect_file_format (filename))
    {
    case GIMP_PALETTE_FILE_FORMAT_GPL:
      palette_list = gimp_palette_load (filename, error);
      if (palette_list)
        {
          palette = palette_list->data;
          g_list_free (palette_list);
        }
      break;

    case GIMP_PALETTE_FILE_FORMAT_ACT:
      palette = GIMP_PALETTE (gimp_palette_new (palette_name));

      while (read (fd, color_bytes, 3) == 3)
        {
          gimp_rgba_set_uchar (&color,
                               color_bytes[0],
                               color_bytes[1],
                               color_bytes[2],
                               255);
          gimp_palette_add_entry (palette, -1, NULL, &color);
        }
      break;

    case GIMP_PALETTE_FILE_FORMAT_RIFF_PAL:
      palette = GIMP_PALETTE (gimp_palette_new (palette_name));

      lseek (fd, 28, SEEK_SET);
      while (read (fd,
                   color_bytes, sizeof (color_bytes)) == sizeof (color_bytes))
        {
          gimp_rgba_set_uchar (&color,
                               color_bytes[0],
                               color_bytes[1],
                               color_bytes[2],
                               255);
          gimp_palette_add_entry (palette, -1, NULL, &color);
        }
      break;

    case GIMP_PALETTE_FILE_FORMAT_PSP_PAL:
      {
        gint       number_of_colors;
        gint       data_size;
        gint       i, j;
        gboolean   color_ok;
        gchar      buffer[4096];
        /*Maximum valid file size: 256 * 4 * 3 + 256 * 2  ~= 3650 bytes */
        gchar    **lines;
        gchar    **ascii_colors;

        palette = GIMP_PALETTE (gimp_palette_new (palette_name));

        lseek (fd, 16, SEEK_SET);
        data_size = read (fd, buffer, sizeof (buffer) - 1);
        buffer [data_size] = '\0';

        lines = g_strsplit (buffer, "\x0d\x0a", -1);

        number_of_colors = atoi (lines[0]);

        for (i = 0; i < number_of_colors; i++)
          {
            if (lines[i + 1] == NULL)
              {
                g_printerr ("Premature end of file reading %s.",
                            gimp_filename_to_utf8 (filename));
                break;
              }

            ascii_colors = g_strsplit (lines [i + 1], " ", 3);
            color_ok = TRUE;

            for (j = 0 ; j < 3; j++)
              {
                if (ascii_colors [j] == NULL)
                  {
                    g_printerr ("Corrupted palette file %s.",
                                gimp_filename_to_utf8 (filename));
                    color_ok = FALSE;
                    break;
                  }

                color_bytes [j] = atoi (ascii_colors[j]);
              }

            if (color_ok)
              {
                gimp_rgba_set_uchar (&color,
                                     color_bytes[0],
                                     color_bytes[1],
                                     color_bytes[2],
                                     255);
                gimp_palette_add_entry (palette, -1, NULL, &color);
              }

            g_strfreev (ascii_colors);
          }

        g_strfreev (lines);
      }
      break;

    default:
      g_set_error (error,
                   GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Unknown type of palette file: %s"),
                   gimp_filename_to_utf8 (filename));
      break;
    }

  close (fd);

  return palette;
}

