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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "base/temp-buf.h"

#include "gimpimage.h"
#include "gimpgradient.h"

#include "errors.h"
#include "gimprc.h"

#include "libgimp/gimpintl.h"


#define EPSILON 1e-10


static void       gimp_gradient_class_init      (GimpGradientClass *klass);
static void       gimp_gradient_init            (GimpGradient      *gradient);
static void       gimp_gradient_destroy         (GtkObject         *object);
static TempBuf  * gimp_gradient_get_new_preview (GimpViewable      *viewable,
						 gint               width,
						 gint               height);
static void       gimp_gradient_dirty           (GimpData          *data);
static gboolean   gimp_gradient_save            (GimpData          *data);
static gchar    * gimp_gradient_get_extension   (GimpData          *data);
static GimpData * gimp_gradient_duplicate       (GimpData          *data);

static gdouble    gimp_gradient_calc_linear_factor            (gdouble  middle,
							       gdouble  pos);
static gdouble    gimp_gradient_calc_curved_factor            (gdouble  middle,
							       gdouble  pos);
static gdouble    gimp_gradient_calc_sine_factor              (gdouble  middle,
							       gdouble  pos);
static gdouble    gimp_gradient_calc_sphere_increasing_factor (gdouble  middle,
							       gdouble  pos);
static gdouble    gimp_gradient_calc_sphere_decreasing_factor (gdouble  middle,
							       gdouble  pos);


static GimpDataClass *parent_class = NULL;


GtkType
gimp_gradient_get_type (void)
{
  static GtkType gradient_type = 0;

  if (! gradient_type)
    {
      static const GtkTypeInfo gradient_info =
      {
        "GimpGradient",
        sizeof (GimpGradient),
        sizeof (GimpGradientClass),
        (GtkClassInitFunc) gimp_gradient_class_init,
        (GtkObjectInitFunc) gimp_gradient_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      gradient_type = gtk_type_unique (GIMP_TYPE_DATA, &gradient_info);
  }

  return gradient_type;
}

static void
gimp_gradient_class_init (GimpGradientClass *klass)
{
  GtkObjectClass    *object_class;
  GimpViewableClass *viewable_class;
  GimpDataClass     *data_class;

  object_class   = (GtkObjectClass *) klass;
  viewable_class = (GimpViewableClass *) klass;
  data_class     = (GimpDataClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_DATA);

  object_class->destroy = gimp_gradient_destroy;

  viewable_class->get_new_preview = gimp_gradient_get_new_preview;

  data_class->dirty         = gimp_gradient_dirty;
  data_class->save          = gimp_gradient_save;
  data_class->get_extension = gimp_gradient_get_extension;
  data_class->duplicate     = gimp_gradient_duplicate;
}

static void
gimp_gradient_init (GimpGradient *gradient)
{
  gradient->segments     = NULL;
  gradient->last_visited = NULL;
}

static void
gimp_gradient_destroy (GtkObject *object)
{
  GimpGradient *gradient;

  gradient = GIMP_GRADIENT (object);

  if (gradient->segments)
    gimp_gradient_segments_free (gradient->segments);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static TempBuf *
gimp_gradient_get_new_preview (GimpViewable *viewable,
			       gint          width,
			       gint          height)
{
  GimpGradient *gradient;
  TempBuf      *temp_buf;
  guchar       *buf;
  guchar       *p0, *p1;
  guchar       *even, *odd;
  gint          x, y;
  gdouble       dx, cur_x;
  GimpRGB       color;
  gdouble       c0, c1;

  gradient = GIMP_GRADIENT (viewable);

  dx    = 1.0 / (width - 1);
  cur_x = 0.0;
  p0    = even = g_malloc (width * 3);
  p1    = odd  = g_malloc (width * 3);

  /* Create lines to fill the image */

  for (x = 0; x < width; x++)
    {
      gimp_gradient_get_color_at (gradient, cur_x, &color);

      if ((x / GIMP_CHECK_SIZE_SM) & 1)
        {
          c0 = GIMP_CHECK_LIGHT;
          c1 = GIMP_CHECK_DARK;
        }
      else
        {
          c0 = GIMP_CHECK_DARK;
          c1 = GIMP_CHECK_LIGHT;
        }

      *p0++ = (c0 + (color.r - c0) * color.a) * 255.0;
      *p0++ = (c0 + (color.g - c0) * color.a) * 255.0;
      *p0++ = (c0 + (color.b - c0) * color.a) * 255.0;

      *p1++ = (c1 + (color.r - c1) * color.a) * 255.0;
      *p1++ = (c1 + (color.g - c1) * color.a) * 255.0;
      *p1++ = (c1 + (color.b - c1) * color.a) * 255.0;

      cur_x += dx;
    }

  temp_buf = temp_buf_new (width, height,
			   3,
			   0, 0,
			   NULL);

  buf = temp_buf_data (temp_buf);

  for (y = 0; y < height; y++)
    {
      if ((y / GIMP_CHECK_SIZE_SM) & 1)
        {
          memcpy (buf + (width * y * 3), odd, width * 3); 
        }
      else
        {
          memcpy (buf + (width * y * 3), even, width * 3); 
        }
    }

  g_free (even);
  g_free (odd);

  return temp_buf;
}

static GimpData *
gimp_gradient_duplicate (GimpData *data)
{
  GimpGradient        *gradient;
  GimpGradientSegment *head, *prev, *cur, *orig;

  gradient = GIMP_GRADIENT (gtk_type_new (GIMP_TYPE_GRADIENT));

  gimp_data_dirty (GIMP_DATA (gradient));

  prev = NULL;
  orig = GIMP_GRADIENT (data)->segments;
  head = NULL;

  while (orig)
    {
      cur = gimp_gradient_segment_new ();

      *cur = *orig;  /* Copy everything */

      cur->prev = prev;
      cur->next = NULL;

      if (prev)
	prev->next = cur;
      else
	head = cur;  /* Remember head */

      prev = cur;
      orig = orig->next;
    }

  gradient->segments = head;

  return GIMP_DATA (gradient);
}

GimpData *
gimp_gradient_new (const gchar *name)
{
  GimpGradient *gradient;

  g_return_val_if_fail (name != NULL, NULL);

  gradient = GIMP_GRADIENT (gtk_type_new (GIMP_TYPE_GRADIENT));

  gimp_object_set_name (GIMP_OBJECT (gradient), name);

  gradient->segments = gimp_gradient_segment_new ();

  return GIMP_DATA (gradient);
}

GimpData *
gimp_gradient_get_standard (void)
{
  static GimpGradient *standard_gradient = NULL;

  if (! standard_gradient)
    {
      standard_gradient = GIMP_GRADIENT (gimp_gradient_new ("Standard"));

      gtk_object_ref (GTK_OBJECT (standard_gradient));
      gtk_object_sink (GTK_OBJECT (standard_gradient));
    }

  return GIMP_DATA (standard_gradient);
}

GimpData *
gimp_gradient_load (const gchar *filename)
{
  GimpGradient        *gradient;
  GimpGradientSegment *seg;
  GimpGradientSegment *prev;
  gint                 num_segments;
  gint                 i;
  gint                 type, color;
  FILE                *file;
  gchar                line[1024];

  g_return_val_if_fail (filename != NULL, NULL);

  file = fopen (filename, "rb");
  if (!file)
    return NULL;

  fgets (line, 1024, file);
  if (strcmp (line, "GIMP Gradient\n") != 0)
    {
      fclose (file);
      return NULL;
    }

  gradient = GIMP_GRADIENT (gtk_type_new (GIMP_TYPE_GRADIENT));

  gimp_data_set_filename (GIMP_DATA (gradient), filename);

  fgets (line, 1024, file);
  if (! strncmp (line, "Name: ", strlen ("Name: ")))
    {
      gimp_object_set_name (GIMP_OBJECT (gradient),
			    g_strstrip (&line[strlen ("Name: ")]));

      fgets (line, 1024, file);
    }
  else /* old gradient format */
    {
      gimp_object_set_name (GIMP_OBJECT (gradient), g_basename (filename));
    }

  num_segments = atoi (line);

  if (num_segments < 1)
    {
      g_message ("%s(): invalid number of segments in \"%s\"",
		 G_GNUC_FUNCTION, filename);
      gtk_object_sink (GTK_OBJECT (gradient));
      fclose (file);
      return NULL;
    }

  prev = NULL;

  for (i = 0; i < num_segments; i++)
    {
      seg = gimp_gradient_segment_new ();

      seg->prev = prev;

      if (prev)
	prev->next = seg;
      else
	gradient->segments = seg;

      fgets (line, 1024, file);

      if (sscanf (line, "%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%d%d",
		  &(seg->left), &(seg->middle), &(seg->right),
		  &(seg->left_color.r),
		  &(seg->left_color.g),
		  &(seg->left_color.b),
		  &(seg->left_color.a),
		  &(seg->right_color.r),
		  &(seg->right_color.g),
		  &(seg->right_color.b),
		  &(seg->right_color.a),
		  &type, &color) != 13)
	{
	  g_message ("%s(): badly formatted gradient segment %d in \"%s\" --- "
		     "bad things may happen soon",
		     G_GNUC_FUNCTION, i, filename);
	}
      else
	{
	  seg->type  = (GimpGradientSegmentType) type;
	  seg->color = (GimpGradientSegmentColor) color;
	}

      prev = seg;
    }

  fclose (file);

  GIMP_DATA (gradient)->dirty = FALSE;

  return GIMP_DATA (gradient);
}

static void
gimp_gradient_dirty (GimpData *data)
{
  GimpGradient *gradient;

  gradient = GIMP_GRADIENT (data);

  gradient->last_visited = NULL;

  if (GIMP_DATA_CLASS (parent_class)->dirty)
    GIMP_DATA_CLASS (parent_class)->dirty (data);
}

static gboolean
gimp_gradient_save (GimpData *data)
{
  GimpGradient        *gradient;
  GimpGradientSegment *seg;
  gint                 num_segments;
  FILE                *file;

  gradient = GIMP_GRADIENT (data);

  file = fopen (data->filename, "wb");
  if (! file)
    {
      g_message ("%s(): can't open \"%s\"",
		 G_GNUC_FUNCTION, data->filename);
      return FALSE;
    }

  /* File format is:
   *
   *   GIMP Gradient
   *   Name: name
   *   number_of_segments
   *   left middle right r0 g0 b0 a0 r1 g1 b1 a1 type coloring
   *   left middle right r0 g0 b0 a0 r1 g1 b1 a1 type coloring
   *   ...
   */

  fprintf (file, "GIMP Gradient\n");

  fprintf (file, "Name: %s\n", GIMP_OBJECT (gradient)->name);

  /* Count number of segments */
  num_segments = 0;
  seg          = gradient->segments;

  while (seg)
    {
      num_segments++;
      seg = seg->next;
    }

  /* Write rest of file */
  fprintf (file, "%d\n", num_segments);

  for (seg = gradient->segments; seg; seg = seg->next)
    {
      fprintf (file, "%f %f %f %f %f %f %f %f %f %f %f %d %d\n",
	       seg->left, seg->middle, seg->right,
	       seg->left_color.r,
	       seg->left_color.g,
	       seg->left_color.b,
	       seg->left_color.a,
	       seg->right_color.r,
	       seg->right_color.g,
	       seg->right_color.b,
	       seg->right_color.a,
	       (gint) seg->type,
	       (gint) seg->color);
    }

  fclose (file);

  return TRUE;
}

static gchar *
gimp_gradient_get_extension (GimpData *data)
{
  return GIMP_GRADIENT_FILE_EXTENSION;
}

void
gimp_gradient_get_color_at (GimpGradient *gradient,
			    gdouble     pos,
			    GimpRGB    *color)
{
  gdouble              factor = 0.0;
  GimpGradientSegment *seg;
  gdouble              seg_len;
  gdouble              middle;
  GimpRGB              rgb;

  g_return_if_fail (gradient != NULL);
  g_return_if_fail (GIMP_IS_GRADIENT (gradient));
  g_return_if_fail (color != NULL);

  /* if there is no gradient return a totally transparent black */
  if (gradient == NULL) 
    {
      gimp_rgba_set (color, 0.0, 0.0, 0.0, 0.0);
      return;
    }

  if (pos < 0.0)
    pos = 0.0;
  else if (pos > 1.0)
    pos = 1.0;

  seg = gimp_gradient_get_segment_at (gradient, pos);

  seg_len = seg->right - seg->left;

  if (seg_len < EPSILON)
    {
      middle = 0.5;
      pos    = 0.5;
    }
  else
    {
      middle = (seg->middle - seg->left) / seg_len;
      pos    = (pos - seg->left) / seg_len;
    }

  switch (seg->type)
    {
    case GRAD_LINEAR:
      factor = gimp_gradient_calc_linear_factor (middle, pos);
      break;

    case GRAD_CURVED:
      factor = gimp_gradient_calc_curved_factor (middle, pos);
      break;

    case GRAD_SINE:
      factor = gimp_gradient_calc_sine_factor (middle, pos);
      break;

    case GRAD_SPHERE_INCREASING:
      factor = gimp_gradient_calc_sphere_increasing_factor (middle, pos);
      break;

    case GRAD_SPHERE_DECREASING:
      factor = gimp_gradient_calc_sphere_decreasing_factor (middle, pos);
      break;

    default:
      gimp_fatal_error ("%s(): Unknown gradient type %d",
			G_GNUC_FUNCTION, (gint) seg->type);
      break;
    }

  /* Calculate color components */

  rgb.a = seg->left_color.a + (seg->right_color.a - seg->left_color.a) * factor;

  if (seg->color == GRAD_RGB)
    {
      rgb.r 
	= seg->left_color.r + (seg->right_color.r - seg->left_color.r) * factor;

      rgb.g =
	seg->left_color.g + (seg->right_color.g - seg->left_color.g) * factor;

      rgb.b =
	seg->left_color.b + (seg->right_color.b - seg->left_color.b) * factor;
    }
  else
    {
      GimpHSV left_hsv;
      GimpHSV right_hsv;

      gimp_rgb_to_hsv (&seg->left_color,  &left_hsv);
      gimp_rgb_to_hsv (&seg->right_color, &right_hsv);

      left_hsv.s = left_hsv.s + (right_hsv.s - left_hsv.s) * factor;
      left_hsv.v = left_hsv.v + (right_hsv.v - left_hsv.v) * factor;

      switch (seg->color)
	{
	case GRAD_HSV_CCW:
	  if (left_hsv.h < right_hsv.h)
	    {
	      left_hsv.h += (right_hsv.h - left_hsv.h) * factor;
	    }
	  else
	    {
	      left_hsv.h += (1.0 - (left_hsv.h - right_hsv.h)) * factor;

	      if (left_hsv.h > 1.0)
		left_hsv.h -= 1.0;
	    }
	  break;

	case GRAD_HSV_CW:
	  if (right_hsv.h < left_hsv.h)
	    {
	      left_hsv.h -= (left_hsv.h - right_hsv.h) * factor;
	    }
	  else
	    {
	      left_hsv.h -= (1.0 - (right_hsv.h - left_hsv.h)) * factor;

	      if (left_hsv.h < 0.0)
		left_hsv.h += 1.0;
	    }
	  break;

	default:
	  gimp_fatal_error ("%s(): Unknown coloring mode %d",
			    G_GNUC_FUNCTION, (gint) seg->color);
	  break;
	}

      gimp_hsv_to_rgb (&left_hsv, &rgb);
    }

  *color = rgb;
}

GimpGradientSegment *
gimp_gradient_get_segment_at (GimpGradient *gradient,
			      gdouble       pos)
{
  GimpGradientSegment *seg;

  g_return_val_if_fail (gradient != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_GRADIENT (gradient), NULL);

  /* handle FP imprecision at the edges of the gradient */
  pos = CLAMP (pos, 0.0, 1.0);

  if (gradient->last_visited)
    seg = gradient->last_visited;
  else
    seg = gradient->segments;

  while (seg)
    {
      if (pos >= seg->left)
	{
	  if (pos <= seg->right)
	    {
	      gradient->last_visited = seg; /* for speed */
	      return seg;
	    }
	  else
	    {
	      seg = seg->next;
	    }
	}
      else
	{
	  seg = seg->prev;
	}
    }

  /* Oops: we should have found a segment, but we didn't */
  gimp_fatal_error ("%s(): no matching segment for position %0.15f",
		    G_GNUC_FUNCTION, pos);

  return NULL;
}

static gdouble
gimp_gradient_calc_linear_factor (gdouble middle,
				  gdouble pos)
{
  if (pos <= middle)
    {
      if (middle < EPSILON)
	return 0.0;
      else
	return 0.5 * pos / middle;
    }
  else
    {
      pos -= middle;
      middle = 1.0 - middle;

      if (middle < EPSILON)
	return 1.0;
      else
	return 0.5 + 0.5 * pos / middle;
    }
}

static gdouble
gimp_gradient_calc_curved_factor (gdouble middle,
				  gdouble pos)
{
  if (middle < EPSILON)
    middle = EPSILON;

  return pow(pos, log (0.5) / log (middle));
}

static gdouble
gimp_gradient_calc_sine_factor (gdouble middle,
				gdouble pos)
{
  pos = gimp_gradient_calc_linear_factor (middle, pos);

  return (sin ((-G_PI / 2.0) + G_PI * pos) + 1.0) / 2.0;
}

static gdouble
gimp_gradient_calc_sphere_increasing_factor (gdouble middle,
					     gdouble pos)
{
  pos = gimp_gradient_calc_linear_factor (middle, pos) - 1.0;

  return sqrt (1.0 - pos * pos); /* Works for convex increasing and concave decreasing */
}

static gdouble
gimp_gradient_calc_sphere_decreasing_factor (gdouble middle,
					     gdouble pos)
{
  pos = gimp_gradient_calc_linear_factor (middle, pos);

  return 1.0 - sqrt(1.0 - pos * pos); /* Works for convex decreasing and concave increasing */
}


/*  gradient segment functions  */

GimpGradientSegment *
gimp_gradient_segment_new (void)
{
  GimpGradientSegment *seg;

  seg = g_new (GimpGradientSegment, 1);

  seg->left   = 0.0;
  seg->middle = 0.5;
  seg->right  = 1.0;

  gimp_rgba_set (&seg->left_color,  0.0, 0.0, 0.0, 1.0);
  gimp_rgba_set (&seg->right_color, 1.0, 1.0, 1.0, 1.0);

  seg->type  = GRAD_LINEAR;
  seg->color = GRAD_RGB;

  seg->prev = seg->next = NULL;

  return seg;
}


void
gimp_gradient_segment_free (GimpGradientSegment *seg)
{
  g_return_if_fail (seg != NULL);

  g_free (seg);
}

void
gimp_gradient_segments_free (GimpGradientSegment *seg)
{
  GimpGradientSegment *tmp;

  g_return_if_fail (seg != NULL);

  while (seg)
    {
      tmp = seg->next;
      gimp_gradient_segment_free (seg);
      seg = tmp;
    }
}

GimpGradientSegment *
gimp_gradient_segment_get_last (GimpGradientSegment *seg)
{
  if (!seg)
    return NULL;

  while (seg->next)
    seg = seg->next;

  return seg;
}
