/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp_brush_generated module Copyright 1998 Jay Cox <jaycox@earthlink.net>
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

#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "base/temp-buf.h"

#include "gimpbrushgenerated.h"

#include "gimp-intl.h"


#define OVERSAMPLING 5


/*  local function prototypes  */
static void   gimp_brush_generated_class_init (GimpBrushGeneratedClass *klass);
static void   gimp_brush_generated_init       (GimpBrushGenerated      *brush);

static gboolean   gimp_brush_generated_save          (GimpData  *data,
                                                      GError   **error);
static void       gimp_brush_generated_dirty         (GimpData  *data);
static gchar    * gimp_brush_generated_get_extension (GimpData  *data);
static GimpData * gimp_brush_generated_duplicate     (GimpData  *data,
                                                      gboolean   stingy_memory_use);


static GimpBrushClass *parent_class = NULL;


GType
gimp_brush_generated_get_type (void)
{
  static GType brush_type = 0;

  if (! brush_type)
    {
      static const GTypeInfo brush_info =
      {
        sizeof (GimpBrushGeneratedClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_brush_generated_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data     */
	sizeof (GimpBrushGenerated),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_brush_generated_init,
      };

      brush_type = g_type_register_static (GIMP_TYPE_BRUSH,
					   "GimpBrushGenerated",
					   &brush_info, 0);
    }

  return brush_type;
}

static void
gimp_brush_generated_class_init (GimpBrushGeneratedClass *klass)
{
  GimpDataClass *data_class;

  data_class = GIMP_DATA_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  data_class->save          = gimp_brush_generated_save;
  data_class->dirty         = gimp_brush_generated_dirty;
  data_class->get_extension = gimp_brush_generated_get_extension;
  data_class->duplicate     = gimp_brush_generated_duplicate;
}

static void
gimp_brush_generated_init (GimpBrushGenerated *brush)
{
  brush->radius       = 5.0;
  brush->hardness     = 0.0;
  brush->angle        = 0.0;
  brush->aspect_ratio = 1.0;
  brush->freeze       = 0;
}

static gboolean
gimp_brush_generated_save (GimpData  *data,
                           GError   **error)
{
  GimpBrushGenerated *brush;
  FILE               *fp;
  gchar               buf[G_ASCII_DTOSTR_BUF_SIZE];

  brush = GIMP_BRUSH_GENERATED (data);

  /*  we are (finaly) ready to try to save the generated brush file  */
  if ((fp = fopen (data->filename, "wb")) == NULL)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_OPEN,
                   _("Could not open '%s' for writing: %s"),
                   gimp_filename_to_utf8 (data->filename),
		   g_strerror (errno));
      return FALSE;
    }

  /* write magic header */
  fprintf (fp, "GIMP-VBR\n");

  /* write version */
  fprintf (fp, "1.0\n");

  /* write name */
  fprintf (fp, "%.255s\n", GIMP_OBJECT (brush)->name);

  /* write brush spacing */
  fprintf (fp, "%s\n",
           g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f",
                            GIMP_BRUSH (brush)->spacing));

  /* write brush radius */
  fprintf (fp, "%s\n",
           g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f",
                            brush->radius));

  /* write brush hardness */
  fprintf (fp, "%s\n",
           g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f",
                            brush->hardness));

  /* write brush aspect_ratio */
  fprintf (fp, "%s\n",
           g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f",
                            brush->aspect_ratio));

  /* write brush angle */
  fprintf (fp, "%s\n",
           g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f",
                            brush->angle));

  fclose (fp);

  return TRUE;
}

static gchar *
gimp_brush_generated_get_extension (GimpData *data)
{
  return GIMP_BRUSH_GENERATED_FILE_EXTENSION;
}

static GimpData *
gimp_brush_generated_duplicate (GimpData *data,
                                gboolean  stingy_memory_use)
{
  GimpBrushGenerated *brush;

  brush = GIMP_BRUSH_GENERATED (data);

  return gimp_brush_generated_new (brush->radius,
				   brush->hardness,
				   brush->angle,
				   brush->aspect_ratio,
                                   stingy_memory_use);
}

static gdouble
gauss (gdouble f)
{
  /* this aint' a real gauss function */
  if (f < -0.5)
    {
      f = -1.0 - f;
      return (2.0 * f*f);
    }

  if (f < 0.5)
    return (1.0 - 2.0 * f*f);

  f = 1.0 - f;
  return (2.0 * f*f);
}

static void
gimp_brush_generated_dirty (GimpData *data)
{
  GimpBrushGenerated *brush;
  GimpBrush          *gbrush = NULL;
  gint     x, y;
  guchar  *centerp;
  gdouble  d;
  gdouble  exponent;
  guchar   a;
  gint     length;
  gint     width, height;
  guchar  *lookup;
  gdouble  sum;
  gdouble  c, s;
  gdouble  short_radius;
  gdouble  buffer[OVERSAMPLING];

  brush = GIMP_BRUSH_GENERATED (data);

  if (brush->freeze) /* if we are frozen defer rerendering till later */
    return;

  gbrush = GIMP_BRUSH (brush);

  if (gbrush->mask)
    temp_buf_free (gbrush->mask);

  s = sin (gimp_deg_to_rad (brush->angle));
  c = cos (gimp_deg_to_rad (brush->angle));

  short_radius = brush->radius / brush->aspect_ratio;

  gbrush->x_axis.x =        c * brush->radius;
  gbrush->x_axis.y = -1.0 * s * brush->radius;
  gbrush->y_axis.x =        s * short_radius;
  gbrush->y_axis.y =        c * short_radius;

  width  = ceil (sqrt (gbrush->x_axis.x * gbrush->x_axis.x +
                       gbrush->y_axis.x * gbrush->y_axis.x));
  height = ceil (sqrt (gbrush->x_axis.y * gbrush->x_axis.y +
                       gbrush->y_axis.y * gbrush->y_axis.y));

  gbrush->mask = temp_buf_new (width  * 2 + 1,
			       height * 2 + 1,
			       1, width, height, 0);

  centerp = temp_buf_data (gbrush->mask) + height * gbrush->mask->width + width;

  /* set up lookup table */
  length = OVERSAMPLING * ceil (1 + sqrt (2 *
                                          ceil (brush->radius + 1.0) *
                                          ceil (brush->radius + 1.0)));

  if ((1.0 - brush->hardness) < 0.000001)
    exponent = 1000000.0;
  else
    exponent = 1.0 / (1.0 - brush->hardness);

  lookup = g_malloc (length);
  sum = 0.0;

  for (x = 0; x < OVERSAMPLING; x++)
    {
      d = fabs ((x + 0.5) / OVERSAMPLING - 0.5);

      if (d > brush->radius)
	buffer[x] = 0.0;
      else
	buffer[x] = gauss (pow (d / brush->radius, exponent));

      sum += buffer[x];
    }

  for (x = 0; d < brush->radius || sum > 0.00001; d += 1.0 / OVERSAMPLING)
    {
      sum -= buffer[x % OVERSAMPLING];

      if (d > brush->radius)
	buffer[x % OVERSAMPLING] = 0.0;
      else
	buffer[x % OVERSAMPLING] = gauss (pow (d / brush->radius, exponent));

      sum += buffer[x % OVERSAMPLING];
      lookup[x++] = RINT (sum * (255.0 / OVERSAMPLING));
    }

  while (x < length)
    {
      lookup[x++] = 0;
    }

  /* compute one half and mirror it */
  for (y = 0; y <= height; y++)
    {
      for (x = -width; x <= width; x++)
	{
          gdouble tx, ty;

	  tx = c*x - s*y;
	  ty = c*y + s*x;
	  ty *= brush->aspect_ratio;
	  d = sqrt (tx*tx + ty*ty);

	  if (d < brush->radius + 1)
	    a = lookup[(gint) RINT (d * OVERSAMPLING)];
	  else
	    a = 0;

	  centerp[     y * gbrush->mask->width + x] = a;
	  centerp[-1 * y * gbrush->mask->width - x] = a;
	}
    }

  g_free (lookup);

  if (GIMP_DATA_CLASS (parent_class)->dirty)
    GIMP_DATA_CLASS (parent_class)->dirty (data);
}

GimpData *
gimp_brush_generated_new (gfloat   radius,
			  gfloat   hardness,
			  gfloat   angle,
			  gfloat   aspect_ratio,
                          gboolean stingy_memory_use)
{
  GimpBrushGenerated *brush;

  /* set up normal brush data */
  brush = g_object_new (GIMP_TYPE_BRUSH_GENERATED, NULL);

  gimp_object_set_name (GIMP_OBJECT (brush), "Untitled");

  GIMP_BRUSH (brush)->spacing = 20;

  /* set up gimp_brush_generated data */
  brush->radius       = radius;
  brush->hardness     = hardness;
  brush->angle        = angle;
  brush->aspect_ratio = aspect_ratio;

  /* render brush mask */
  gimp_data_dirty (GIMP_DATA (brush));

  if (stingy_memory_use)
    temp_buf_swap (GIMP_BRUSH (brush)->mask);

  return GIMP_DATA (brush);
}

GimpData *
gimp_brush_generated_load (const gchar  *filename,
                           gboolean      stingy_memory_use,
                           GError      **error)
{
  GimpBrushGenerated *brush;
  FILE               *fp;
  gchar               string[256];

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if ((fp = fopen (filename, "rb")) == NULL)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_OPEN,
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return NULL;
    }

  /* make sure the file we are reading is the right type */
  fgets (string, 255, fp);

  if (strncmp (string, "GIMP-VBR", 8) != 0)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file '%s': "
                     "Not a GIMP brush file."),
                   gimp_filename_to_utf8 (filename));
      return NULL;
    }

  /* make sure we are reading a compatible version */
  fgets (string, 255, fp);
  if (strncmp (string, "1.0", 3))
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file '%s': "
                     "Unknown GIMP brush version."),
                   gimp_filename_to_utf8 (filename));
      return NULL;
    }

  /* create new brush */
  brush = g_object_new (GIMP_TYPE_BRUSH_GENERATED, NULL);

  gimp_data_set_filename (GIMP_DATA (brush), filename);

  gimp_brush_generated_freeze (brush);

  /* read name */
  fgets (string, 255, fp);
  if (string[strlen (string) - 1] == '\n')
    string[strlen (string) - 1] = 0;
  gimp_object_set_name (GIMP_OBJECT (brush), string);

  /* read brush spacing */
  fgets (string, 255, fp);
  GIMP_BRUSH (brush)->spacing = g_ascii_strtod (string, NULL);

  /* read brush radius */
  fgets (string, 255, fp);
  gimp_brush_generated_set_radius (brush, g_ascii_strtod (string, NULL));

  /* read brush hardness */
  fgets (string, 255, fp);
  gimp_brush_generated_set_hardness (brush, g_ascii_strtod (string, NULL));

  /* read brush aspect_ratio */
  fgets (string, 255, fp);
  gimp_brush_generated_set_aspect_ratio (brush, g_ascii_strtod (string, NULL));

  /* read brush angle */
  fgets (string, 255, fp);
  gimp_brush_generated_set_angle (brush, g_ascii_strtod (string, NULL));

  fclose (fp);

  gimp_brush_generated_thaw (brush);

  if (stingy_memory_use)
    temp_buf_swap (GIMP_BRUSH (brush)->mask);

  GIMP_DATA (brush)->dirty = FALSE;

  return GIMP_DATA (brush);
}

void
gimp_brush_generated_freeze (GimpBrushGenerated *brush)
{
  g_return_if_fail (GIMP_IS_BRUSH_GENERATED (brush));

  brush->freeze++;
}

void
gimp_brush_generated_thaw (GimpBrushGenerated *brush)
{
  g_return_if_fail (GIMP_IS_BRUSH_GENERATED (brush));

  if (brush->freeze > 0)
    brush->freeze--;

  if (!brush->freeze)
    gimp_data_dirty (GIMP_DATA (brush));
}

gfloat
gimp_brush_generated_set_radius (GimpBrushGenerated *brush,
				 gfloat              radius)
{
  g_return_val_if_fail (GIMP_IS_BRUSH_GENERATED (brush), -1.0);

  if (radius < 0.0)
    radius = 0.0;
  else if (radius > 32767.0)
    radius = 32767.0;

  if (radius == brush->radius)
    return radius;

  brush->radius = radius;

  if (!brush->freeze)
    gimp_data_dirty (GIMP_DATA (brush));

  return brush->radius;
}

gfloat
gimp_brush_generated_set_hardness (GimpBrushGenerated *brush,
				   gfloat              hardness)
{
  g_return_val_if_fail (GIMP_IS_BRUSH_GENERATED (brush), -1.0);

  if (hardness < 0.0)
    hardness = 0.0;
  else if (hardness > 1.0)
    hardness = 1.0;

  if (brush->hardness == hardness)
    return hardness;

  brush->hardness = hardness;

  if (!brush->freeze)
    gimp_data_dirty (GIMP_DATA (brush));

  return brush->hardness;
}

gfloat
gimp_brush_generated_set_angle (GimpBrushGenerated *brush,
				gfloat              angle)
{
  g_return_val_if_fail (GIMP_IS_BRUSH_GENERATED (brush), -1.0);

  if (angle < 0.0)
    angle = -1.0 * fmod (angle, 180.0);
  else if (angle > 180.0)
    angle = fmod (angle, 180.0);

  if (brush->angle == angle)
    return angle;

  brush->angle = angle;

  if (!brush->freeze)
    gimp_data_dirty (GIMP_DATA (brush));

  return brush->angle;
}

gfloat
gimp_brush_generated_set_aspect_ratio (GimpBrushGenerated *brush,
				       gfloat              ratio)
{
  g_return_val_if_fail (GIMP_IS_BRUSH_GENERATED (brush), -1.0);

  if (ratio < 1.0)
    ratio = 1.0;
  else if (ratio > 1000)
    ratio = 1000;

  if (brush->aspect_ratio == ratio)
    return ratio;

  brush->aspect_ratio = ratio;

  if (!brush->freeze)
    gimp_data_dirty (GIMP_DATA (brush));

  return brush->aspect_ratio;
}

gfloat
gimp_brush_generated_get_radius (const GimpBrushGenerated *brush)
{
  g_return_val_if_fail (GIMP_IS_BRUSH_GENERATED (brush), -1.0);

  return brush->radius;
}

gfloat
gimp_brush_generated_get_hardness (const GimpBrushGenerated *brush)
{
  g_return_val_if_fail (GIMP_IS_BRUSH_GENERATED (brush), -1.0);

  return brush->hardness;
}

gfloat
gimp_brush_generated_get_angle (const GimpBrushGenerated *brush)
{
  g_return_val_if_fail (GIMP_IS_BRUSH_GENERATED (brush), -1.0);

  return brush->angle;
}

gfloat
gimp_brush_generated_get_aspect_ratio (const GimpBrushGenerated *brush)
{
  g_return_val_if_fail (GIMP_IS_BRUSH_GENERATED (brush), -1.0);

  return brush->aspect_ratio;
}
