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

#include <math.h>
#include <stdio.h>
#include <glib.h>

#include "appenv.h"
#include "gimplut.h"
#include "gimphistogram.h"

/* ---------- Brightness/Contrast -----------*/

typedef struct B_C_struct
{
  double brightness;
  double contrast;
} B_C_struct;

static float
brightness_contrast_lut_func(B_C_struct *data,
			     int nchannels, int channel, float value)
{
  float nvalue;
  double power;

  /* return the original value for the alpha channel */
  if ((nchannels == 2 || nchannels == 4) && channel == nchannels -1)
    return value;

  /* apply brightness */
  if (data->brightness < 0.0)
    value = value * (1.0 + data->brightness);
  else
    value = value + ((1.0 - value) * data->brightness);

  /* apply contrast */
  if (data->contrast < 0.0)
  {
    if (value > 0.5)
      nvalue = 1.0 - value;
    else
      nvalue = value;
    if (nvalue < 0.0)
      nvalue = 0.0;
    nvalue = 0.5 * pow (nvalue * 2.0 , (double) (1.0 + data->contrast));
    if (value > 0.5)
      value = 1.0 - nvalue;
    else
      value = nvalue;
  }
  else
  {
    if (value > 0.5)
      nvalue = 1.0 - value;
    else
      nvalue = value;
    if (nvalue < 0.0)
      nvalue = 0.0;
    power = (data->contrast == 1.0) ? 127 : 1.0 / (1.0 - data->contrast);
    nvalue = 0.5 * pow (2.0 * nvalue, power);
    if (value > 0.5)
      value = 1.0 - nvalue;
    else
      value = nvalue;
  }
  return value;
}

void
brightness_contrast_lut_setup(GimpLut *lut, double brightness, double contrast,
			      int nchannels)
{
  B_C_struct data;
  data.brightness = brightness;
  data.contrast = contrast;
  gimp_lut_setup(lut,  (GimpLutFunc) brightness_contrast_lut_func,
		 (void *) &data, nchannels);
}

GimpLut *
brightness_contrast_lut_new(double brightness, double contrast,
			    int nchannels)
{
  GimpLut *lut;
  lut = gimp_lut_new();
  brightness_contrast_lut_setup(lut, brightness, contrast, nchannels);
  return lut;
}

/* ---------------- invert ------------------ */

static float
invert_lut_func(void *unused,
		int nchannels, int channel, float value)
{
  /* don't invert the alpha channel */
  if ((nchannels == 2 || nchannels == 4) && channel == nchannels -1)
    return value;

  return 1.0 - value;
}

void
invert_lut_setup(GimpLut *lut, int nchannels)
{
  gimp_lut_setup_exact(lut,  (GimpLutFunc) invert_lut_func,
		       NULL , nchannels);
}

GimpLut *
invert_lut_new(int nchannels)
{
  GimpLut *lut;
  lut = gimp_lut_new();
  invert_lut_setup(lut, nchannels);
  return lut;
}

/* ---------------- add (or subract)------------------ */

static float
add_lut_func(double *ammount,
	     int nchannels, int channel, float value)
{
  /* don't change the alpha channel */
  if ((nchannels == 2 || nchannels == 4) && channel == nchannels -1)
    return value;

  return (value + *ammount);
}

void
add_lut_setup(GimpLut *lut, double ammount, int nchannels)
{
  gimp_lut_setup(lut,  (GimpLutFunc) add_lut_func,
		 (void *) &ammount , nchannels);
}

GimpLut *
add_lut_new(double ammount, int nchannels)
{
  GimpLut *lut;
  lut = gimp_lut_new();
  add_lut_setup(lut, ammount, nchannels);
  return lut;
}

/* ---------------- intersect (MINIMUM(pixel, value)) ------------------ */

static float
intersect_lut_func(double *min,
	     int nchannels, int channel, float value)
{
  /* don't change the alpha channel */
  if ((nchannels == 2 || nchannels == 4) && channel == nchannels -1)
    return value;

  return MIN(value, *min);
}

void
intersect_lut_setup(GimpLut *lut, double value, int nchannels)
{
  gimp_lut_setup_exact(lut,  (GimpLutFunc) intersect_lut_func,
		       (void *) &value , nchannels);
}

GimpLut *
intersect_lut_new(double value, int nchannels)
{
  GimpLut *lut;
  lut = gimp_lut_new();
  intersect_lut_setup(lut, value, nchannels);
  return lut;
}

/* ---------------- Threshold ------------------ */

static float
threshold_lut_func(double *min,
	     int nchannels, int channel, float value)
{
  /* don't change the alpha channel */
  if ((nchannels == 2 || nchannels == 4) && channel == nchannels -1)
    return value;
  if (value < *min)
    return 0.0;
  return 1.0;
}

void
threshold_lut_setup(GimpLut *lut, double value, int nchannels)
{
  gimp_lut_setup_exact(lut,  (GimpLutFunc) threshold_lut_func,
		       (void *) &value , nchannels);
}

GimpLut *
threshold_lut_new(double value, int nchannels)
{
  GimpLut *lut;
  lut = gimp_lut_new();
  threshold_lut_setup(lut, value, nchannels);
  return lut;
}

/* ------------- levels ------------ */

typedef struct 
{
  double *gamma;

  int    *low_input;
  int    *high_input;

  int    *low_output;
  int    *high_output;
} levels_struct;

static float
levels_lut_func(levels_struct *data,
		int nchannels, int channel, float value)
{
  double inten;
  int j;

  if (nchannels == 1)
    j = 0;
  else
    j = channel + 1;
  inten = value;
  /* For color  images this runs through the loop with j = channel +1
     the first time and j = 0 the second time */
  /* For bw images this runs through the loop with j = 0 the first and
     only time  */
  for (; j >= 0; j -= (channel + 1))
  {
    /* don't apply the overall curve to the alpha channel */
    if (j == 0 && (nchannels == 2 || nchannels == 4)
	&& channel == nchannels -1)
      return inten;

    /*  determine input intensity  */
    if (data->high_input[j] != data->low_input[j])
      inten = (double) (255.0*inten - data->low_input[j]) /
	(double) (data->high_input[j] - data->low_input[j]);
    else
      inten = (double) (255.0*inten - data->low_input[j]);

    if (data->gamma[j] != 0.0)
      {
	if (inten >= 0.0)
	  inten =  pow ( inten, (1.0 / data->gamma[j]));
	else
	  inten = -pow (-inten, (1.0 / data->gamma[j]));
      }

  /*  determine the output intensity  */
    if (data->high_output[j] >= data->low_output[j])
      inten = (double) (inten * (data->high_output[j] - data->low_output[j]) +
			data->low_output[j]);
    else if (data->high_output[j] < data->low_output[j])
      inten = (double) (data->low_output[j] - inten *
			(data->low_output[j] - data->high_output[j]));

    inten /= 255.0;
  }
  return inten;
}

void
levels_lut_setup(GimpLut *lut, double *gamma, int *low_input, int *high_input,
		 int *low_output, int *high_output, int nchannels)
{
  levels_struct data;
  data.gamma = gamma;
  data.low_input  = low_input;
  data.high_input = high_input;
  data.low_output  = low_output;
  data.high_output = high_output;
  gimp_lut_setup(lut,  (GimpLutFunc) levels_lut_func,
		 (void *) &data, nchannels);
}

GimpLut *
levels_lut_new(double *gamma, int *low_input, int *high_input,
	       int *low_output, int *high_output, int nchannels)
{
  GimpLut *lut;
  lut = gimp_lut_new();
  levels_lut_setup(lut, gamma, low_input, high_input,
		   low_output, high_output, nchannels);
  return lut;
}

/* --------------- posterize ---------------- */

static float
posterize_lut_func(int *ilevels,
		   int nchannels, int channel, float value)
{
  int levels;
  /* don't posterize the alpha channel */
  if ((nchannels == 2 || nchannels == 4) && channel == nchannels -1)
    return value;

  if (*ilevels < 2)
    levels = 2;
  else
    levels = *ilevels;

  value = RINT(value * (levels - 1.0)) / (levels - 1.0);

  return value;
}

void
posterize_lut_setup(GimpLut *lut, int levels, int nchannels)
{
  gimp_lut_setup_exact(lut,  (GimpLutFunc) posterize_lut_func,
		       (void *) &levels , nchannels);
}

GimpLut *
posterize_lut_new(int levels, int nchannels)
{
  GimpLut *lut;
  lut = gimp_lut_new();
  posterize_lut_setup(lut, levels, nchannels);
  return lut;
}

/* --------------- equalize ------------- */

struct hist_lut_struct
{
  GimpHistogram *histogram;
  int part[5][257];
};

static float
equalize_lut_func(struct hist_lut_struct *hlut, 
		  int nchannels, int channel, float value)
{
  int i = 0, j;
  j = (int)(value * 255.0 + 0.5);
  while (hlut->part[channel][i + 1] <= j)
    i++;
  return i / 255.0;
}

void
eq_histogram_lut_setup (GimpLut *lut, GimpHistogram *hist, int bytes)
{
  int    i, k, j;
  struct hist_lut_struct hlut;
  double pixels_per_value;
  double desired;
  double sum, dif;

  /* Find partition points */
  pixels_per_value = gimp_histogram_get_count(hist, 0, 255) / 256.0;

  for (k = 0; k < bytes; k++)
    {
      /* First and last points in partition */
      hlut.part[k][0]   = 0;
      hlut.part[k][256] = 256;
      
      /* Find intermediate points */
      j = 0;
      sum = gimp_histogram_get_channel(hist, k, 0) + 
	    gimp_histogram_get_channel(hist, k, 1);
      for (i = 1; i < 256; i++)
	{
	  desired = i * pixels_per_value;
	  while (sum <= desired)
	  {
	    j++;
	    sum += gimp_histogram_get_channel(hist, k, j + 1);
	  }

	  /* Nearest sum */
	  dif = sum - gimp_histogram_get_channel(hist, k, j);
	  if ((sum - desired) > (dif / 2.0))
	    hlut.part[k][i] = j;
	  else
	    hlut.part[k][i] = j + 1;
	}
    }

  gimp_lut_setup(lut, (GimpLutFunc) equalize_lut_func,
		 (void *) &hlut, bytes);
}

GimpLut *
eq_histogram_lut_new(GimpHistogram *h, int nchannels)
{
  GimpLut *lut;
  lut = gimp_lut_new();
  eq_histogram_lut_setup(lut, h, nchannels);
  return lut;
}
