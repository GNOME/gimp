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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "appenv.h"
#include "actionarea.h"
#include "buildmenu.h"
#include "canvas.h"
#include "colormaps.h"
#include "drawable.h"
#include "general.h"
#include "gdisplay.h"
#include "histogram.h"
#include "image_map.h"
#include "interface.h"
#include "levels.h"
#include "pixelarea.h"
#include "pixelrow.h"

#define LOW_INPUT          0x1
#define GAMMA              0x2
#define HIGH_INPUT         0x4
#define LOW_OUTPUT         0x8
#define HIGH_OUTPUT        0x10
#define INPUT_LEVELS       0x20
#define OUTPUT_LEVELS      0x40
#define INPUT_SLIDERS      0x80
#define OUTPUT_SLIDERS     0x100
#define DRAW               0x200
#define ALL                0xFFF

#define TEXT_WIDTH       45
#define DA_WIDTH         256
#define DA_HEIGHT        25
#define GRADIENT_HEIGHT  15
#define CONTROL_HEIGHT   DA_HEIGHT - GRADIENT_HEIGHT
#define HISTOGRAM_WIDTH  256
#define HISTOGRAM_HEIGHT 150

#define LEVELS_DA_MASK  GDK_EXPOSURE_MASK | \
                        GDK_ENTER_NOTIFY_MASK | \
			GDK_BUTTON_PRESS_MASK | \
			GDK_BUTTON_RELEASE_MASK | \
			GDK_BUTTON1_MOTION_MASK | \
			GDK_POINTER_MOTION_HINT_MASK

typedef struct _Levels Levels;

struct _Levels
{
  int x, y;    /*  coords for last mouse click  */
};

typedef struct _LevelsDialog LevelsDialog;

struct _LevelsDialog
{
  GtkWidget *    shell;
  GtkWidget *    low_input_text;
  GtkWidget *    gamma_text;
  GtkWidget *    high_input_text;
  GtkWidget *    low_output_text;
  GtkWidget *    high_output_text;
  GtkWidget *    input_levels_da[2];
  GtkWidget *    output_levels_da[2];
  GtkWidget *    channel_menu;
  Histogram *    histogram;

  GimpDrawable * drawable;
  ImageMap16     image_map;
  int            color;
  int            channel;
  gint           preview;
 
  PixelRow       high_input_pr;  /* length 5 */
  double         gamma[5];
  PixelRow 	 low_input_pr;   /* length 5 */
  PixelRow       low_output_pr;  /* length 5 */
  PixelRow 	 high_output_pr; /* length 5 */

  int            active_slider;
  int            slider_pos[5];      /*  positions for the five sliders  */
  guchar         input_ui[5][256];   /*  input transfer lut for ui */
};

/* transfer luts for input and output */
PixelRow input[5];
PixelRow output[5];

/*  levels action functions  */

static void   levels_button_press   (Tool *, GdkEventButton *, gpointer);
static void   levels_button_release (Tool *, GdkEventButton *, gpointer);
static void   levels_motion         (Tool *, GdkEventMotion *, gpointer);
static void   levels_cursor_update  (Tool *, GdkEventMotion *, gpointer);
static void   levels_control        (Tool *, int, gpointer);

static LevelsDialog *  levels_new_dialog              (void);
static void            levels_update                  (LevelsDialog *, int);
static void            levels_preview                 (LevelsDialog *);
static void            levels_value_callback          (GtkWidget *, gpointer);
static void            levels_red_callback            (GtkWidget *, gpointer);
static void            levels_green_callback          (GtkWidget *, gpointer);
static void            levels_blue_callback           (GtkWidget *, gpointer);
static void            levels_alpha_callback          (GtkWidget *, gpointer);
static void            levels_auto_levels_callback    (GtkWidget *, gpointer);
static void            levels_ok_callback             (GtkWidget *, gpointer);
static void            levels_cancel_callback         (GtkWidget *, gpointer);
static gint            levels_delete_callback         (GtkWidget *, GdkEvent *, gpointer);
static void            levels_preview_update          (GtkWidget *, gpointer);
static void            levels_low_input_text_update   (GtkWidget *, gpointer);
static void            levels_gamma_text_update       (GtkWidget *, gpointer);
static void            levels_high_input_text_update  (GtkWidget *, gpointer);
static void            levels_low_output_text_update  (GtkWidget *, gpointer);
static void            levels_high_output_text_update (GtkWidget *, gpointer);
static gint            levels_output_da_events        (GtkWidget *, GdkEvent *, LevelsDialog *);
static gint            levels_input_da_events         (GtkWidget *, GdkEvent *, LevelsDialog *);
static void            levels_free_transfers          (LevelsDialog *);
static void            levels_calculate_transfers_ui  (LevelsDialog *);

static void *levels_options = NULL;
static LevelsDialog *levels_dialog = NULL;

static void       levels_histogram_info (PixelArea *, PixelArea *, HistogramValues, void *);
static void       levels_histogram_range (int, int, HistogramValues, void *);
static Argument * levels_invoker (Argument *);

static void  levels_funcs (Tag);

#define LOW_INPUT_STRING   0 
#define HIGH_INPUT_STRING  1
#define LOW_OUTPUT_STRING  2
#define HIGH_OUTPUT_STRING 3

static gchar *ui_strings[4];

typedef void (*LevelsFunc)(PixelArea *, PixelArea *, void *);
typedef void (*LevelsCalculateTransfersFunc)(LevelsDialog *);
typedef void (*LevelsInputDaSetuiValuesFunc)(LevelsDialog *, gint);
typedef void (*LevelsOutputDaSetuiValuesFunc)(LevelsDialog *, gint);
typedef gint (*LevelsHighOutputTextCheckFunc)(LevelsDialog *, char *);
typedef gint (*LevelsLowOutputTextCheckFunc)(LevelsDialog *, char *);
typedef gint (*LevelsHighInputTextCheckFunc)(LevelsDialog *, char *);
typedef gint (*LevelsLowInputTextCheckFunc)(LevelsDialog *, char *);
typedef void (*LevelsAllocTransfersFunc)(LevelsDialog *);
typedef void (*LevelsBuildInputDaTransferFunc)(LevelsDialog *, guchar *);
typedef void (*LevelsUpdateLowInputSetTextFunc)(LevelsDialog *, char *);
typedef void (*LevelsUpdateHighInputSetTextFunc)(LevelsDialog *, char *);
typedef void (*LevelsUpdateLowOutputSetTextFunc)(LevelsDialog *, char *);
typedef void (*LevelsUpdateHighOutputSetTextFunc)(LevelsDialog *, char *);
typedef gdouble (*LevelsGetLowInputFunc)(LevelsDialog *);
typedef gdouble (*LevelsGetHighInputFunc)(LevelsDialog *);
typedef gdouble (*LevelsGetLowOutputFunc)(LevelsDialog *);
typedef gdouble (*LevelsGetHighOutputFunc)(LevelsDialog *);
typedef void (*LevelsInitHighLowInputOutputFunc)(LevelsDialog *);

static LevelsFunc levels;
static LevelsCalculateTransfersFunc levels_calculate_transfers;
static LevelsInputDaSetuiValuesFunc levels_input_da_setui_values;
static LevelsOutputDaSetuiValuesFunc levels_output_da_setui_values;
static LevelsHighOutputTextCheckFunc levels_high_output_text_check;
static LevelsLowOutputTextCheckFunc levels_low_output_text_check;
static LevelsHighInputTextCheckFunc levels_high_input_text_check;
static LevelsLowInputTextCheckFunc levels_low_input_text_check;
static LevelsAllocTransfersFunc levels_alloc_transfers;
static LevelsBuildInputDaTransferFunc levels_build_input_da_transfer;
static LevelsUpdateLowInputSetTextFunc levels_update_low_input_set_text;
static LevelsUpdateHighInputSetTextFunc levels_update_high_input_set_text;
static LevelsUpdateLowOutputSetTextFunc levels_update_low_output_set_text;
static LevelsUpdateHighOutputSetTextFunc levels_update_high_output_set_text;
static LevelsGetLowInputFunc levels_get_low_input;
static LevelsGetHighInputFunc levels_get_high_input;
static LevelsGetLowOutputFunc levels_get_low_output;
static LevelsGetHighOutputFunc levels_get_high_output;
static LevelsInitHighLowInputOutputFunc levels_init_high_low_input_output;

static gchar ui_strings_u8[4][8] = {"0","255","0","255"};

static void       levels_u8 (PixelArea *, PixelArea *, void *);
static void       levels_calculate_transfers_u8 (LevelsDialog *ld);
static void       levels_input_da_setui_values_u8(LevelsDialog *, gint);
static void       levels_output_da_setui_values_u8(LevelsDialog *, gint);
static gint       levels_high_output_text_check_u8(LevelsDialog *, char *);
static gint       levels_low_output_text_check_u8(LevelsDialog *, char *);
static gint       levels_high_input_text_check_u8(LevelsDialog *, char *);
static gint       levels_low_input_text_check_u8 (LevelsDialog *, char *);
static void 	  levels_alloc_transfers_u8 (LevelsDialog *);
static void       levels_build_input_da_transfer_u8(LevelsDialog *, guchar *);
static void       levels_update_low_input_set_text_u8 ( LevelsDialog *, char *);
static void       levels_update_high_input_set_text_u8 ( LevelsDialog *, char *);
static void       levels_update_low_output_set_text_u8 ( LevelsDialog *, char *);
static void       levels_update_high_output_set_text_u8 ( LevelsDialog *, char *);
static gdouble    levels_get_low_input_u8 ( LevelsDialog *);
static gdouble    levels_get_high_input_u8 ( LevelsDialog *); 
static gdouble    levels_get_low_output_u8 ( LevelsDialog *);
static gdouble    levels_get_high_output_u8 ( LevelsDialog *);
static void       levels_init_high_low_input_output_u8 (LevelsDialog *ld);

static gchar ui_strings_u16[4][8] = {"0","65535","0","65535"};

static void       levels_u16 (PixelArea *, PixelArea *, void *);
static void       levels_calculate_transfers_u16 (LevelsDialog *ld);
static void       levels_input_da_setui_values_u16(LevelsDialog *, gint);
static void       levels_output_da_setui_values_u16(LevelsDialog *, gint);
static gint       levels_high_output_text_check_u16(LevelsDialog *, char *);
static gint       levels_low_output_text_check_u16(LevelsDialog *, char *);
static gint       levels_high_input_text_check_u16(LevelsDialog *, char *);
static gint       levels_low_input_text_check_u16 (LevelsDialog *, char *);
static void 	  levels_alloc_transfers_u16 (LevelsDialog *);
static void       levels_build_input_da_transfer_u16(LevelsDialog *, guchar *);
static void       levels_update_low_input_set_text_u16 ( LevelsDialog *, char *);
static void       levels_update_high_input_set_text_u16 ( LevelsDialog *, char *);
static void       levels_update_low_output_set_text_u16 ( LevelsDialog *, char *);
static void       levels_update_high_output_set_text_u16 ( LevelsDialog *, char *);
static gdouble    levels_get_low_input_u16 ( LevelsDialog *);
static gdouble    levels_get_high_input_u16 ( LevelsDialog *); 
static gdouble    levels_get_low_output_u16 ( LevelsDialog *);
static gdouble    levels_get_high_output_u16 ( LevelsDialog *);
static void       levels_init_high_low_input_output_u16 (LevelsDialog *ld);

static void
levels_u8 (PixelArea *src_area,
	PixelArea *dest_area,
	void        *user_data)
{
  Tag src_tag = pixelarea_tag (src_area);
  Tag dest_tag = pixelarea_tag (dest_area);
  gint src_num_channels = tag_num_channels (src_tag);
  gint dest_num_channels = tag_num_channels (dest_tag);
  LevelsDialog *ld;
  guchar *src, *dest;
  guint8 *s, *d;
  int has_alpha, alpha;
  int w, h;
  guint8 *output_r, *output_g, *output_b, *output_val, *output_a;
  guint8 *input_r, *input_g, *input_b, *input_val, *input_a;

  ld = (LevelsDialog *) user_data;

  src = pixelarea_data (src_area);
  dest = pixelarea_data (dest_area);
  has_alpha = tag_alpha (src_tag) == ALPHA_YES ? TRUE: FALSE;
  alpha = has_alpha ? src_num_channels - 1 : src_num_channels;

  output_r = (guint8*)pixelrow_data (&output[HISTOGRAM_RED]);
  output_g = (guint8*)pixelrow_data (&output[HISTOGRAM_GREEN]);
  output_b = (guint8*)pixelrow_data (&output[HISTOGRAM_BLUE]);
  output_val = (guint8*)pixelrow_data (&output[HISTOGRAM_VALUE]);
  if (has_alpha)
    output_a = (guint8*)pixelrow_data (&output[HISTOGRAM_ALPHA]);
  
  input_r = (guint8*)pixelrow_data (&input[HISTOGRAM_RED]);
  input_g = (guint8*)pixelrow_data (&input[HISTOGRAM_GREEN]);
  input_b = (guint8*)pixelrow_data (&input[HISTOGRAM_BLUE]);
  input_val = (guint8*)pixelrow_data (&input[HISTOGRAM_VALUE]);
  if (has_alpha)
    input_a = (guint8*)pixelrow_data (&input[HISTOGRAM_ALPHA]);
  
  h = pixelarea_height (src_area);
  while (h--)
    {
      s = (guint8*)src;
      d = (guint8*)dest;
      w = pixelarea_width (src_area);
      while (w--)
	{
	  if (ld->color)
	    {
	      /*  The contributions from the individual channel level settings  */
	      d[RED_PIX] = output_r[input_r[s[RED_PIX]]];
	      d[GREEN_PIX] = output_g[input_g[s[GREEN_PIX]]];
	      d[BLUE_PIX] = output_b[input_b[s[BLUE_PIX]]];

	      /*  The overall changes  */
	      d[RED_PIX] = output_val[input_val[d[RED_PIX]]];
	      d[GREEN_PIX] = output_val[input_val[d[GREEN_PIX]]];
	      d[BLUE_PIX] = output_val[input_val[d[BLUE_PIX]]];
	    }
	  else
	    d[GRAY_PIX] = output_val[input_val[s[GRAY_PIX]]];;

	  if (has_alpha)
	      d[alpha] = output_a[input_a[s[alpha]]];
	  s += src_num_channels;
	  d += dest_num_channels;
	}

      src += pixelarea_rowstride (src_area);
      dest += pixelarea_rowstride (dest_area);
    }
}

static void
levels_init_high_low_input_output_u8 (LevelsDialog *ld)
{
  gint i;
    guint8 *low_input = (guint8*)pixelrow_data (&levels_dialog->low_input_pr);
    guint8 *high_input = (guint8*)pixelrow_data (&levels_dialog->high_input_pr);
    guint8 *low_output = (guint8*)pixelrow_data (&levels_dialog->low_output_pr);
    guint8 *high_output = (guint8*)pixelrow_data (&levels_dialog->high_output_pr);
    for (i = 0; i < 5; i++)
      {
	low_input[i] = 0;
	levels_dialog->gamma[i] = 1.0;
	high_input[i] = 255;
	low_output[i] = 0;
	high_output[i] = 255;
      }
}

static void
levels_calculate_transfers_u8 (LevelsDialog *ld)
{
  double inten;
  int i, j;
  guint8* low_input = (guint8*)pixelrow_data (&ld->low_input_pr);
  guint8* high_input = (guint8*)pixelrow_data (&ld->high_input_pr);
  guint8* low_output = (guint8*)pixelrow_data (&ld->low_output_pr);
  guint8* high_output = (guint8*)pixelrow_data (&ld->high_output_pr);

  /*  Recalculate the levels arrays  */
  for (j = 0; j < 5; j++)
    {
      guint8 *out = (guint8*)pixelrow_data (&output[j]);
      guint8 *in = (guint8*)pixelrow_data (&input[j]); 
      for (i = 0; i < 256; i++)
	{
	  /*  determine input intensity  */
	  if (high_input[j] != low_input[j])
	    inten = (double) (i - low_input[j]) /
	      (double) (high_input[j] - low_input[j]);
	  else
	    inten = (double) (i - low_input[j]);

	  inten = BOUNDS (inten, 0.0, 1.0);
	  if (ld->gamma[j] != 0.0)
	    inten = pow (inten, (1.0 / ld->gamma[j]));
	  in[i] = (unsigned char) (inten * 255.0 + 0.5);

	  /*  determine the output intensity  */
	  inten = (double) i / 255.0;
	  if (high_output[j] >= low_output[j])
	    inten = (double) (inten * (high_output[j] - low_output[j]) + low_output[j]);
	  else if (high_output[j] < low_output[j])
	    inten = (double) (low_output[j] - inten * (low_output[j] - high_output[j]));

	  inten = BOUNDS (inten, 0.0, 255.0);
	  out[i] = (unsigned char) (inten + 0.5);
	}
    }
}


static void
levels_input_da_setui_values_u8 (
				LevelsDialog *ld,
				gint x	
				)
{
  char text[12];
  guint8* low_input = (guint8*)pixelrow_data (&ld->low_input_pr);
  guint8* high_input = (guint8*)pixelrow_data (&ld->high_input_pr);
  double width, mid, tmp;

  switch (ld->active_slider)
    {
    case 0:  /*  low input  */
      tmp =  ((double) x / (double) DA_WIDTH) * 255.0;
      low_input[ld->channel] = BOUNDS (tmp, 0, high_input[ld->channel]);
      break;

    case 1:  /*  gamma  */
      width = (double) (ld->slider_pos[2] - ld->slider_pos[0]) / 2.0;
      mid = ld->slider_pos[0] + width;

      x = BOUNDS (x, ld->slider_pos[0], ld->slider_pos[2]);
      tmp = (double) (x - mid) / width;
      ld->gamma[ld->channel] = 1.0 / pow (10, tmp);

      /*  round the gamma value to the nearest 1/100th  */
      sprintf (text, "%2.2f", ld->gamma[ld->channel]);
      ld->gamma[ld->channel] = atof (text);
      break;

    case 2:  /*  high input  */
      tmp = ((double) x / (double) DA_WIDTH) * 255.0;
      high_input[ld->channel] = BOUNDS (tmp, low_input[ld->channel], 255);
      break;
    }
}

static void
levels_output_da_setui_values_u8(
				LevelsDialog *ld,
				gint x	
				)
{
  gdouble tmp;
  guint8* low_output = (guint8*)pixelrow_data (&ld->low_output_pr);
  guint8* high_output = (guint8*)pixelrow_data (&ld->high_output_pr);

  switch (ld->active_slider)
    {
    case 3:  /*  low output  */
      tmp = ((double) x / (double) DA_WIDTH) * 255.0;
      low_output[ld->channel] = BOUNDS (tmp, 0, 255);
      break;

    case 4:  /*  high output  */
      tmp = ((double) x / (double) DA_WIDTH) * 255.0;
      high_output[ld->channel] = BOUNDS (tmp, 0, 255);
      break;
    }
}

static gint 
levels_high_output_text_check_u8 (
				LevelsDialog *ld,
				char *str 
				)
{
  int value;
  guint8* high_output = (guint8*)pixelrow_data (&ld->high_output_pr);

  value = BOUNDS (((int) atof (str)), 0, 255);
  if (value != high_output[ld->channel])
  {
      high_output[ld->channel] = value;
      return TRUE;
  }
  return FALSE;
}

static gint 
levels_low_output_text_check_u8 (
				LevelsDialog *ld,
				char *str 
				)
{
  int value;
  guint8* low_output = (guint8*)pixelrow_data (&ld->low_output_pr);

  value = BOUNDS (((int) atof (str)), 0, 255);
  if (value != low_output[ld->channel])
  {
      low_output[ld->channel] = value;
      return TRUE;
  }
  return FALSE;
}

static gint 
levels_high_input_text_check_u8 (
				LevelsDialog *ld,
				char *str 
				)
{
  int value;
  guint8* high_input = (guint8*)pixelrow_data (&ld->high_input_pr);
  guint8* low_input = (guint8*)pixelrow_data (&ld->low_input_pr);
  value = BOUNDS (((int) atof (str)), low_input[ld->channel], 255);
  if (value != high_input[ld->channel])
  {
      high_input[ld->channel] = value;
      return TRUE;
  }
  return FALSE;
}

static gint 
levels_low_input_text_check_u8 (
				LevelsDialog *ld,
				char *str 
				)
{
  int value;
  guint8* high_input = (guint8*)pixelrow_data (&ld->high_input_pr);
  guint8* low_input = (guint8*)pixelrow_data (&ld->low_input_pr);
  value = BOUNDS (((int) atof (str)), 0, high_input[ld->channel]);
  if (value != low_input[ld->channel])
  {
      low_input[ld->channel] = value;
      return TRUE;
  }
  return FALSE;
}

static void
levels_alloc_transfers_u8 (LevelsDialog *ld)
{
  gint i;
  guint8* data;
  Tag tag = tag_new (PRECISION_U8, FORMAT_GRAY, ALPHA_NO);
  
  
  /* alloc the high and low input and output arrays*/
  data = (guint8*) g_malloc (sizeof(guint8) * 5 );
  pixelrow_init (&ld->high_input_pr, tag, (guchar*)data, 5);
  data = (guint8*) g_malloc (sizeof(guint8) * 5 );
  pixelrow_init (&ld->low_input_pr, tag, (guchar*)data, 5);
  data = (guint8*) g_malloc (sizeof(guint8) * 5 );
  pixelrow_init (&ld->high_output_pr, tag, (guchar*)data, 5);
  data = (guint8*) g_malloc (sizeof(guint8) * 5 );
  pixelrow_init (&ld->low_output_pr, tag, (guchar*)data, 5);
	
  /* allocate the input and output transfer arrays */
  for (i = 0; i < 5; i++)
  { 
       data = (guint8*) g_malloc (sizeof(guint8) * 256 );
       pixelrow_init (&input[i], tag, (guchar*)data, 256);
  }
  
  for (i = 0; i < 5; i++)
  { 
       data = (guint8*) g_malloc (sizeof(guint8) * 256 );
       pixelrow_init (&output[i], tag, (guchar*)data, 256);
  }
}

static void
levels_build_input_da_transfer_u8 (
				LevelsDialog *ld,
				guchar *buf 
				)
{
  gint i;
  guint8 *input_data = (guint8 *)pixelrow_data (&input[ld->channel]); 
  for(i = 0; i < DA_WIDTH; i++)
	buf[i] = input_data[i]; 
}

static void
levels_update_low_input_set_text_u8 (
			 LevelsDialog *ld,
			 char * text
			)
{
 guint8 * low_input = (guint8*)pixelrow_data (&ld->low_input_pr);
 sprintf (text, "%d", low_input[ld->channel]);
}

static void
levels_update_high_input_set_text_u8 (
			 LevelsDialog *ld,
			 char * text
			)
{
 guint8 * high_input = (guint8*)pixelrow_data (&ld->high_input_pr);
 sprintf (text, "%d", high_input[ld->channel]);
}

static void
levels_update_low_output_set_text_u8 (
			 LevelsDialog *ld,
			 char * text
			)
{
 guint8 * low_output = (guint8*)pixelrow_data (&ld->low_output_pr);
 sprintf (text, "%d", low_output[ld->channel]);
}

static void
levels_update_high_output_set_text_u8 (
			 LevelsDialog *ld,
			 char * text
			)
{
 guint8 * high_output = (guint8*)pixelrow_data (&ld->high_output_pr);
 sprintf (text, "%d", high_output[ld->channel]);
}

static gdouble 
levels_get_low_input_u8 (
			 LevelsDialog *ld
			)
{
 guint8 * low_input = (guint8*)pixelrow_data (&ld->low_input_pr);
 return (gdouble)low_input[ld->channel]/255.0;
}

static gdouble 
levels_get_high_input_u8 (
			 LevelsDialog *ld
			)
{
 guint8 * high_input = (guint8*)pixelrow_data (&ld->high_input_pr);
 return (gdouble)high_input[ld->channel]/255.0;
}

static gdouble 
levels_get_low_output_u8 (
			 LevelsDialog *ld
			)
{
 guint8 * low_output = (guint8*)pixelrow_data (&ld->low_output_pr);
 return (gdouble)low_output[ld->channel]/255.0;
}

static gdouble 
levels_get_high_output_u8 (
			 LevelsDialog *ld
			)
{
 guint8 * high_output = (guint8*)pixelrow_data (&ld->high_output_pr);
 return (gdouble)high_output[ld->channel]/255.0;
}

static void
levels_u16 (PixelArea *src_area,
	PixelArea *dest_area,
	void        *user_data)
{
  Tag src_tag = pixelarea_tag (src_area);
  Tag dest_tag = pixelarea_tag (dest_area);
  gint src_num_channels = tag_num_channels (src_tag);
  gint dest_num_channels = tag_num_channels (dest_tag);
  LevelsDialog *ld;
  guchar *src, *dest;
  guint16 *s, *d;
  int has_alpha, alpha;
  int w, h;
  guint16 *output_r, *output_g, *output_b, *output_val, *output_a;
  guint16 *input_r, *input_g, *input_b, *input_val, *input_a;

  ld = (LevelsDialog *) user_data;

  src = pixelarea_data (src_area);
  dest = pixelarea_data (dest_area);
  has_alpha = tag_alpha (src_tag) == ALPHA_YES ? TRUE: FALSE;
  alpha = has_alpha ? src_num_channels - 1 : src_num_channels;

  output_r = (guint16*)pixelrow_data (&output[HISTOGRAM_RED]);
  output_g = (guint16*)pixelrow_data (&output[HISTOGRAM_GREEN]);
  output_b = (guint16*)pixelrow_data (&output[HISTOGRAM_BLUE]);
  output_val = (guint16*)pixelrow_data (&output[HISTOGRAM_VALUE]);
  if (has_alpha)
    output_a = (guint16*)pixelrow_data (&output[HISTOGRAM_ALPHA]);
  
  input_r = (guint16*)pixelrow_data (&input[HISTOGRAM_RED]);
  input_g = (guint16*)pixelrow_data (&input[HISTOGRAM_GREEN]);
  input_b = (guint16*)pixelrow_data (&input[HISTOGRAM_BLUE]);
  input_val = (guint16*)pixelrow_data (&input[HISTOGRAM_VALUE]);
  if (has_alpha)
    input_a = (guint16*)pixelrow_data (&input[HISTOGRAM_ALPHA]);
  
  h = pixelarea_height (src_area);
  while (h--)
    {
      s = (guint16*)src;
      d = (guint16*)dest;
      w = pixelarea_width (src_area);
      while (w--)
	{
	  if (ld->color)
	    {
	      /*  The contributions from the individual channel level settings  */
	      d[RED_PIX] = output_r[input_r[s[RED_PIX]]];
	      d[GREEN_PIX] = output_g[input_g[s[GREEN_PIX]]];
	      d[BLUE_PIX] = output_b[input_b[s[BLUE_PIX]]];

	      /*  The overall changes  */
	      d[RED_PIX] = output_val[input_val[d[RED_PIX]]];
	      d[GREEN_PIX] = output_val[input_val[d[GREEN_PIX]]];
	      d[BLUE_PIX] = output_val[input_val[d[BLUE_PIX]]];
	    }
	  else
	    d[GRAY_PIX] = output_val[input_val[s[GRAY_PIX]]];;

	  if (has_alpha)
	      d[alpha] = output_a[input_a[s[alpha]]];
	  s += src_num_channels;
	  d += dest_num_channels;
	}

      src += pixelarea_rowstride (src_area);
      dest += pixelarea_rowstride (dest_area);
    }
}

static void
levels_calculate_transfers_u16 (LevelsDialog *ld)
{
  gdouble inten;
  gint i, j;
  guint16* low_input = (guint16*)pixelrow_data (&ld->low_input_pr);
  guint16* high_input = (guint16*)pixelrow_data (&ld->high_input_pr);
  guint16* low_output = (guint16*)pixelrow_data (&ld->low_output_pr);
  guint16* high_output = (guint16*)pixelrow_data (&ld->high_output_pr);

  /*  Recalculate the levels arrays  */
  for (j = 0; j < 5; j++)
    {
      guint16 *out = (guint16*)pixelrow_data (&output[j]);
      guint16 *in = (guint16*)pixelrow_data (&input[j]); 
      for (i = 0; i < 65536; i++)
	{
	  /*  determine input intensity  */
	  if (high_input[j] != low_input[j])
	    inten = (double) (i - low_input[j]) /
	      (double) (high_input[j] - low_input[j]);
	  else
	    inten = (double) (i - low_input[j]);

	  inten = BOUNDS (inten, 0.0, 1.0);
	  if (ld->gamma[j] != 0.0)
	    inten = pow (inten, (1.0 / ld->gamma[j]));
	  in[i] = (guint16) (inten * 65535.0 + 0.5);

	  /*  determine the output intensity  */
	  inten = (double) i / 65535.0;
	  if (high_output[j] >= low_output[j])
	    inten = (double) (inten * (high_output[j] - low_output[j]) + low_output[j]);
	  else if (high_output[j] < low_output[j])
	    inten = (double) (low_output[j] - inten * (low_output[j] - high_output[j]));

	  inten = BOUNDS (inten, 0.0, 65535.0);
	  out[i] = (guint16) (inten + 0.5);
	}
    }
}

static void
levels_init_high_low_input_output_u16 (LevelsDialog *ld)
{
  gint i;
    guint16 *low_input = (guint16*)pixelrow_data (&levels_dialog->low_input_pr);
    guint16 *high_input = (guint16*)pixelrow_data (&levels_dialog->high_input_pr);
    guint16 *low_output = (guint16*)pixelrow_data (&levels_dialog->low_output_pr);
    guint16 *high_output = (guint16*)pixelrow_data (&levels_dialog->high_output_pr);
    for (i = 0; i < 5; i++)
      {
	low_input[i] = 0;
	levels_dialog->gamma[i] = 1.0;
	high_input[i] = 65535;
	low_output[i] = 0;
	high_output[i] = 65535;
      }
}

static void
levels_input_da_setui_values_u16 (
				LevelsDialog *ld,
				gint x	
				)
{
  char text[12];
  guint16* low_input = (guint16*)pixelrow_data (&ld->low_input_pr);
  guint16* high_input = (guint16*)pixelrow_data (&ld->high_input_pr);
  double width, mid, tmp;

  switch (ld->active_slider)
    {
    case 0:  /*  low input  */
      tmp =  ((double) x / (double) DA_WIDTH) * 65535.0;
      low_input[ld->channel] = BOUNDS (tmp, 0, high_input[ld->channel]);
      break;

    case 1:  /*  gamma  */
      width = (double) (ld->slider_pos[2] - ld->slider_pos[0]) / 2.0;
      mid = ld->slider_pos[0] + width;

      x = BOUNDS (x, ld->slider_pos[0], ld->slider_pos[2]);
      tmp = (double) (x - mid) / width;
      ld->gamma[ld->channel] = 1.0 / pow (10, tmp);

      /*  round the gamma value to the nearest 1/100th  */
      sprintf (text, "%2.2f", ld->gamma[ld->channel]);
      ld->gamma[ld->channel] = atof (text);
      break;

    case 2:  /*  high input  */
      tmp = ((double) x / (double) DA_WIDTH) * 65535.0;
      high_input[ld->channel] = BOUNDS (tmp, low_input[ld->channel], 65535);
      break;
    }
}

static void
levels_output_da_setui_values_u16(
				LevelsDialog *ld,
				gint x	
				)
{
  gdouble tmp;
  guint16* low_output = (guint16*)pixelrow_data (&ld->low_output_pr);
  guint16* high_output = (guint16*)pixelrow_data (&ld->high_output_pr);

  switch (ld->active_slider)
    {
    case 3:  /*  low output  */
      tmp = ((double) x / (double) DA_WIDTH) * 65535.0;
      low_output[ld->channel] = BOUNDS (tmp, 0, 65535);
      break;

    case 4:  /*  high output  */
      tmp = ((double) x / (double) DA_WIDTH) * 65535.0;
      high_output[ld->channel] = BOUNDS (tmp, 0, 65535);
      break;
    }
}

static gint 
levels_high_output_text_check_u16 (
				LevelsDialog *ld,
				char *str 
				)
{
  int value;
  guint16* high_output = (guint16*)pixelrow_data (&ld->high_output_pr);

  value = BOUNDS (((int) atof (str)), 0, 65535);
  if (value != high_output[ld->channel])
  {
      high_output[ld->channel] = value;
      return TRUE;
  }
  return FALSE;
}

static gint 
levels_low_output_text_check_u16 (
				LevelsDialog *ld,
				char *str 
				)
{
  int value;
  guint16* low_output = (guint16*)pixelrow_data (&ld->low_output_pr);

  value = BOUNDS (((int) atof (str)), 0, 65535);
  if (value != low_output[ld->channel])
  {
      low_output[ld->channel] = value;
      return TRUE;
  }
  return FALSE;
}

static gint 
levels_high_input_text_check_u16 (
				LevelsDialog *ld,
				char *str 
				)
{
  int value;
  guint16* high_input = (guint16*)pixelrow_data (&ld->high_input_pr);
  guint16* low_input = (guint16*)pixelrow_data (&ld->low_input_pr);
  value = BOUNDS (((int) atof (str)), low_input[ld->channel], 65535);
  if (value != high_input[ld->channel])
  {
      high_input[ld->channel] = value;
      return TRUE;
  }
  return FALSE;
}

static gint 
levels_low_input_text_check_u16 (
				LevelsDialog *ld,
				char *str 
				)
{
  int value;
  guint16* high_input = (guint16*)pixelrow_data (&ld->high_input_pr);
  guint16* low_input = (guint16*)pixelrow_data (&ld->low_input_pr);
  value = BOUNDS (((int) atof (str)), 0, high_input[ld->channel]);
  if (value != low_input[ld->channel])
  {
      low_input[ld->channel] = value;
      return TRUE;
  }
  return FALSE;
}

static void
levels_alloc_transfers_u16 (LevelsDialog *ld)
{
  gint i;
  guint16* data;
  Tag tag = tag_new (PRECISION_U16, FORMAT_GRAY, ALPHA_NO);
  
  
  /* alloc the high and low input and output arrays*/
  data = (guint16*) g_malloc (sizeof(guint16) * 5 );
  pixelrow_init (&ld->high_input_pr, tag, (guchar*)data, 5);
  data = (guint16*) g_malloc (sizeof(guint16) * 5 );
  pixelrow_init (&ld->low_input_pr, tag, (guchar*)data, 5);
  data = (guint16*) g_malloc (sizeof(guint16) * 5 );
  pixelrow_init (&ld->high_output_pr, tag, (guchar*)data, 5);
  data = (guint16*) g_malloc (sizeof(guint16) * 5 );
  pixelrow_init (&ld->low_output_pr, tag, (guchar*)data, 5);
	
  /* allocate the input and output transfer arrays */
  for (i = 0; i < 5; i++)
  { 
       data = (guint16*) g_malloc (sizeof(guint16) * 65536);
       pixelrow_init (&input[i], tag, (guchar*)data, 65536);
  }
  
  for (i = 0; i < 5; i++)
  { 
       data = (guint16*) g_malloc (sizeof(guint16) * 65536);
       pixelrow_init (&output[i], tag, (guchar*)data, 65536);
  }
}

static void
levels_build_input_da_transfer_u16 (
				LevelsDialog *ld,
				guchar *buf 
				)
{
  gint i;
  guint16 *input_data = (guint16 *)pixelrow_data (&input[ld->channel]); 
  for(i = 0; i < DA_WIDTH; i++)
	buf[i] = (guchar) (input_data[i * 257]/257.0); 
}

static void
levels_update_low_input_set_text_u16 (
			 LevelsDialog *ld,
			 char * text
			)
{
 guint16 * low_input = (guint16*)pixelrow_data (&ld->low_input_pr);
 sprintf (text, "%d", low_input[ld->channel]);
}

static void
levels_update_high_input_set_text_u16 (
			 LevelsDialog *ld,
			 char * text
			)
{
 guint16 * high_input = (guint16*)pixelrow_data (&ld->high_input_pr);
 sprintf (text, "%d", high_input[ld->channel]);
}

static void
levels_update_low_output_set_text_u16 (
			 LevelsDialog *ld,
			 char * text
			)
{
 guint16 * low_output = (guint16*)pixelrow_data (&ld->low_output_pr);
 sprintf (text, "%d", low_output[ld->channel]);
}

static void
levels_update_high_output_set_text_u16 (
			 LevelsDialog *ld,
			 char * text
			)
{
 guint16 * high_output = (guint16*)pixelrow_data (&ld->high_output_pr);
 sprintf (text, "%d", high_output[ld->channel]);
}

static gdouble 
levels_get_low_input_u16 (
			 LevelsDialog *ld
			)
{
 guint16 * low_input = (guint16*)pixelrow_data (&ld->low_input_pr);
 return (gdouble)low_input[ld->channel]/65535.0;
}

static gdouble 
levels_get_high_input_u16 (
			 LevelsDialog *ld
			)
{
 guint16 * high_input = (guint16*)pixelrow_data (&ld->high_input_pr);
 return (gdouble)high_input[ld->channel]/65535.0;
}

static gdouble 
levels_get_low_output_u16 (
			 LevelsDialog *ld
			)
{
 guint16 * low_output = (guint16*)pixelrow_data (&ld->low_output_pr);
 return (gdouble)low_output[ld->channel]/65535.0;
}

static gdouble 
levels_get_high_output_u16 (
			 LevelsDialog *ld
			)
{
 guint16 * high_output = (guint16*)pixelrow_data (&ld->high_output_pr);
 return (gdouble)high_output[ld->channel]/65535.0;
}


static void
levels_funcs (
		 Tag dest_tag
		)
{
  gint i;
  switch (tag_precision (dest_tag))
  {
  case PRECISION_U8:
	levels = levels_u8;
	levels_calculate_transfers = levels_calculate_transfers_u8;
	levels_input_da_setui_values = levels_input_da_setui_values_u8;
	levels_output_da_setui_values = levels_output_da_setui_values_u8;
	levels_high_output_text_check = levels_high_output_text_check_u8;
	levels_low_output_text_check = levels_low_output_text_check_u8;
	levels_high_input_text_check = levels_high_input_text_check_u8;
	levels_low_input_text_check = levels_low_input_text_check_u8;
	levels_alloc_transfers = levels_alloc_transfers_u8;
	levels_build_input_da_transfer = levels_build_input_da_transfer_u8; 
        levels_update_low_input_set_text = levels_update_low_input_set_text_u8;
	levels_update_high_input_set_text = levels_update_high_input_set_text_u8;
	levels_update_low_output_set_text = levels_update_low_output_set_text_u8;
	levels_update_high_output_set_text = levels_update_high_output_set_text_u8;
	levels_get_low_input = levels_get_low_input_u8;
	levels_get_high_input = levels_get_high_input_u8;
	levels_get_low_output = levels_get_low_output_u8;
	levels_get_high_output = levels_get_high_output_u8;
        levels_init_high_low_input_output = levels_init_high_low_input_output_u8;
	for (i = 0; i < 4; i++)
	  ui_strings[i] = ui_strings_u8[i];
	break;
  case PRECISION_U16:
	levels = levels_u16;
	levels_calculate_transfers = levels_calculate_transfers_u16;
	levels_input_da_setui_values = levels_input_da_setui_values_u16;
	levels_output_da_setui_values = levels_output_da_setui_values_u16;
	levels_high_output_text_check = levels_high_output_text_check_u16;
	levels_low_output_text_check = levels_low_output_text_check_u16;
	levels_high_input_text_check = levels_high_input_text_check_u16;
	levels_low_input_text_check = levels_low_input_text_check_u16;
	levels_alloc_transfers = levels_alloc_transfers_u16;
	levels_build_input_da_transfer = levels_build_input_da_transfer_u16; 
        levels_update_low_input_set_text = levels_update_low_input_set_text_u16;
	levels_update_high_input_set_text = levels_update_high_input_set_text_u16;
	levels_update_low_output_set_text = levels_update_low_output_set_text_u16;
	levels_update_high_output_set_text = levels_update_high_output_set_text_u16;
	levels_get_low_input = levels_get_low_input_u16;
	levels_get_high_input = levels_get_high_input_u16;
	levels_get_low_output = levels_get_low_output_u16;
	levels_get_high_output = levels_get_high_output_u16;
        levels_init_high_low_input_output = levels_init_high_low_input_output_u16;
	for (i = 0; i < 4; i++)
	  ui_strings[i] = ui_strings_u16[i];
	break;
  default:
	levels = NULL;
	levels_calculate_transfers = NULL;
	levels_input_da_setui_values = NULL;
	levels_output_da_setui_values = NULL;
	levels_high_output_text_check = NULL;
	levels_low_output_text_check = NULL;
	levels_high_input_text_check = NULL;
	levels_low_input_text_check = NULL;
	levels_alloc_transfers = NULL;
	levels_build_input_da_transfer = NULL; 
        levels_update_low_input_set_text = NULL;
	levels_update_high_input_set_text = NULL;
	levels_update_low_output_set_text = NULL;
	levels_update_high_output_set_text = NULL;
	levels_get_low_input = NULL;
	levels_get_high_input = NULL;
	levels_get_low_output = NULL;
	levels_get_high_output = NULL;
        levels_init_high_low_input_output = NULL;
	for (i = 0; i < 4; i++)
	  ui_strings[i] = NULL;
	break;
  }
}


static void
levels_calculate_transfers_ui (LevelsDialog *ld)
{
  double inten;
  int i, j;
  gint saved = ld->channel;

  for (j = 0; j < 5; j++)
    {
      ld->channel = j;
      {
	guint8 low_input = (*levels_get_low_input)(ld) * 255;
	guint8 high_input = (*levels_get_high_input)(ld) * 255;
	guint8 low_output = (*levels_get_low_output)(ld) * 255;
	guint8 high_output = (*levels_get_high_output)(ld) * 255;
	for (i = 0; i < 256; i++)
	  {
	    /*  determine input intensity  */
	    if (high_input != low_input)
	      inten = (double) (i - low_input) /
		(double) (high_input - low_input);
	    else
	      inten = (double) (i - low_input);

	    inten = BOUNDS (inten, 0.0, 1.0);
	    if (ld->gamma[j] != 0.0)
	      inten = pow (inten, (1.0 / ld->gamma[j]));
	    ld->input_ui[j][i] = (unsigned char) (inten * 255.0 + 0.5);
	  }
       }
    }
  ld->channel = saved;
}

static void
levels_free_transfers (LevelsDialog *ld)
{
  gint i;
  
  g_free (pixelrow_data (&ld->high_input_pr));
  g_free (pixelrow_data (&ld->low_input_pr));
  g_free (pixelrow_data (&ld->high_output_pr));
  g_free (pixelrow_data (&ld->low_output_pr));

  for (i = 0; i < 5; i++)
  {
    g_free (pixelrow_data (&input[i]));
    pixelrow_init (&input[i], tag_null(), NULL, 0);
    g_free (pixelrow_data (&output[i]));
    pixelrow_init (&output[i], tag_null(), NULL, 0);
  }
}


static void
levels_histogram_info (PixelArea     *src_area,
		       PixelArea     *mask_area,
		       HistogramValues  values,
		       void            *user_data)
{
  Tag src_tag = pixelarea_tag (src_area);
  gint src_num_channels = tag_num_channels (src_tag);
  Tag mask_tag;
  gint mask_num_channels;
  LevelsDialog *ld;
  guchar *src, *mask;
  guint8 *s, *m;
  int w, h;
  int value, red, green, blue;
  int has_alpha, alpha;
  
  mask = NULL;
  m    = NULL;
  
  if (mask_area)
    {
      mask_tag = pixelarea_tag (mask_area);
      mask_num_channels = tag_num_channels (mask_tag);
    }
  
  ld = (LevelsDialog *) user_data;

  src = pixelarea_data (src_area);
  has_alpha = tag_alpha (src_tag) == ALPHA_YES ? TRUE: FALSE;
  alpha = has_alpha ? src_num_channels - 1 : src_num_channels;

  if (mask_area)
    mask = pixelarea_data (mask_area);

  h = pixelarea_height (src_area);
  while (h--)
    {
      s = (guint8*)src;

      if (mask_area)
	m = (guint8*)mask;

      w = pixelarea_width (src_area);
      while (w--)
	{
	  if (ld->color)
	    {
	      value = MAX (s[RED_PIX], s[GREEN_PIX]);
	      value = MAX (value, s[BLUE_PIX]);
	      red = s[RED_PIX];
	      green = s[GREEN_PIX];
	      blue = s[BLUE_PIX];
	      alpha = s[ALPHA_PIX];
	      if (mask_area)
		{
		  values[HISTOGRAM_VALUE][value] += (double) *m / 255.0;
		  values[HISTOGRAM_RED][red]     += (double) *m / 255.0;
		  values[HISTOGRAM_GREEN][green] += (double) *m / 255.0;
		  values[HISTOGRAM_BLUE][blue]   += (double) *m / 255.0;
		}
	      else
		{
		  values[HISTOGRAM_VALUE][value] += 1.0;
		  values[HISTOGRAM_RED][red]     += 1.0;
		  values[HISTOGRAM_GREEN][green] += 1.0;
		  values[HISTOGRAM_BLUE][blue]   += 1.0;
		}
	    }
	  else
	    {
	      value = s[GRAY_PIX];
	      if (mask_area)
		values[HISTOGRAM_VALUE][value] += (double) *m / 255.0;
	      else
		values[HISTOGRAM_VALUE][value] += 1.0;
	    }

	  if (has_alpha)
	    {
	      if (mask_area)
		values[HISTOGRAM_ALPHA][s[alpha]] += (double) *m / 255.0;
	      else
		values[HISTOGRAM_ALPHA][s[alpha]] += 1.0;
	    }

	  s += src_num_channels;

	  if (mask_area)
	    m += mask_num_channels;
	}

      src += pixelarea_rowstride (src_area);

      if (mask_area)
	mask += pixelarea_rowstride (mask_area);
    }
}

static void
levels_histogram_range (int              start,
			int              end,
			HistogramValues  values,
			void            *user_data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) user_data;

  histogram_range (ld->histogram, -1, -1);
}

/*  levels action functions  */

static void
levels_button_press (Tool           *tool,
		     GdkEventButton *bevent,
		     gpointer        gdisp_ptr)
{
}

static void
levels_button_release (Tool           *tool,
		       GdkEventButton *bevent,
		       gpointer        gdisp_ptr)
{
}

static void
levels_motion (Tool           *tool,
	       GdkEventMotion *mevent,
	       gpointer        gdisp_ptr)
{
}

static void
levels_cursor_update (Tool           *tool,
		      GdkEventMotion *mevent,
		      gpointer        gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = (GDisplay *) gdisp_ptr;
  gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);
}

static void
levels_control (Tool     *tool,
		int       action,
		gpointer  gdisp_ptr)
{
  Levels * _levels;

  _levels = (Levels *) tool->private;

  switch (action)
    {
    case PAUSE :
      break;
    case RESUME :
      break;
    case HALT :
      if (levels_dialog)
	{
	  image_map_abort_16 (levels_dialog->image_map);
	  levels_free_transfers(levels_dialog);
	  levels_dialog->image_map = NULL;
	  levels_cancel_callback (NULL, (gpointer) levels_dialog);
	}
      break;
    }
}

Tool *
tools_new_levels ()
{
  Tool * tool;
  Levels * private;

  /*  The tool options  */
  if (!levels_options)
    levels_options = tools_register_no_options (LEVELS, "Levels Options");

  tool = (Tool *) g_malloc (sizeof (Tool));
  private = (Levels *) g_malloc (sizeof (Levels));

  tool->type = LEVELS;
  tool->state = INACTIVE;
  tool->scroll_lock = 1;  /*  Disallow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = (void *) private;
  tool->button_press_func = levels_button_press;
  tool->button_release_func = levels_button_release;
  tool->motion_func = levels_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = levels_cursor_update;
  tool->control_func = levels_control;

  return tool;
}

void
tools_free_levels (Tool *tool)
{
  Levels * _levels;

  _levels = (Levels *) tool->private;

  /*  Close the color select dialog  */
  if (levels_dialog)
    levels_ok_callback (NULL, (gpointer) levels_dialog);

  g_free (_levels);
}

static MenuItem color_option_items[] =
{
  { "Value", 0, 0, levels_value_callback, NULL, NULL, NULL },
  { "Red", 0, 0, levels_red_callback, NULL, NULL, NULL },
  { "Green", 0, 0, levels_green_callback, NULL, NULL, NULL },
  { "Blue", 0, 0, levels_blue_callback, NULL, NULL, NULL },
  { "Alpha", 0, 0, levels_alpha_callback, NULL, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL }
};

void
levels_initialize (void *gdisp_ptr)
{
  GDisplay *gdisp;
  int i;
  GimpDrawable * drawable;

  gdisp = (GDisplay *) gdisp_ptr;
  drawable = gimage_active_drawable (gdisp->gimage);

  if (drawable_indexed (drawable))
    {
      message_box ("Levels for indexed drawables cannot be adjusted.", NULL, NULL);
      return;
    }

  levels_funcs ( drawable_tag (drawable));
  /*  The levels dialog  */
  if (!levels_dialog)
    levels_dialog = levels_new_dialog ();
  else
    if (!GTK_WIDGET_VISIBLE (levels_dialog->shell))
      gtk_widget_show (levels_dialog->shell);

  /*  Initialize the values  */
  levels_dialog->channel = HISTOGRAM_VALUE;

   
   (*levels_alloc_transfers) (levels_dialog);
   (*levels_init_high_low_input_output) (levels_dialog);


  levels_dialog->drawable = drawable;
  levels_dialog->color = drawable_color (levels_dialog->drawable);
  levels_dialog->image_map = image_map_create_16 (gdisp_ptr, levels_dialog->drawable);

  /* check for alpha channel */
  if (drawable_has_alpha (levels_dialog->drawable))
    gtk_widget_set_sensitive( color_option_items[4].widget, TRUE);
  else 
    gtk_widget_set_sensitive( color_option_items[4].widget, FALSE);
  
  /*  hide or show the channel menu based on image type  */
  if (levels_dialog->color)
    for (i = 0; i < 4; i++) 
       gtk_widget_set_sensitive( color_option_items[i].widget, TRUE);
  else 
    for (i = 1; i < 4; i++) 
       gtk_widget_set_sensitive( color_option_items[i].widget, FALSE);

  /* set the current selection */
  gtk_option_menu_set_history ( GTK_OPTION_MENU (levels_dialog->channel_menu), 0);

  levels_update (levels_dialog, LOW_INPUT | GAMMA | HIGH_INPUT | LOW_OUTPUT | HIGH_OUTPUT | DRAW);
  levels_update (levels_dialog, INPUT_LEVELS | OUTPUT_LEVELS);
#if 0
  histogram_update (levels_dialog->histogram,
		    levels_dialog->drawable,
		    levels_histogram_info,
		    (void *) levels_dialog);
  histogram_range (levels_dialog->histogram, -1, -1);
#endif
}

void
levels_free ()
{
  if (levels_dialog)
    {
      if (levels_dialog->image_map)
	{
	  image_map_abort_16 (levels_dialog->image_map);
	  levels_free_transfers(levels_dialog);
	  levels_dialog->image_map = NULL;
	}
      gtk_widget_destroy (levels_dialog->shell);
    }
}

/****************************/
/*  Levels dialog  */
/****************************/


/*  the action area structure  */
static ActionAreaItem action_items[] =
{
  { "Auto Levels", levels_auto_levels_callback, NULL, NULL },
  { "OK", levels_ok_callback, NULL, NULL },
  { "Cancel", levels_cancel_callback, NULL, NULL }
};


LevelsDialog *
levels_new_dialog ()
{
  LevelsDialog *ld;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *vbox2;
  GtkWidget *label;
  GtkWidget *frame;
  GtkWidget *toggle;
  GtkWidget *channel_hbox;
  GtkWidget *menu;
  int i;

  ld = g_malloc (sizeof (LevelsDialog));
  ld->preview = TRUE;

  for (i = 0; i < 5; i++)
    color_option_items [i].user_data = (gpointer) ld;

  /*  The shell and main vbox  */
  ld->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (ld->shell), "levels", "Gimp");
  gtk_window_set_title (GTK_WINDOW (ld->shell), "Levels");

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (ld->shell), "delete_event",
		      GTK_SIGNAL_FUNC (levels_delete_callback),
		      ld);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (ld->shell)->vbox), vbox, TRUE, TRUE, 0);

  /*  The option menu for selecting channels  */
  channel_hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), channel_hbox, FALSE, FALSE, 0);

  label = gtk_label_new ("Modify Levels for Channel: ");
  gtk_box_pack_start (GTK_BOX (channel_hbox), label, FALSE, FALSE, 0);

  menu = build_menu (color_option_items, NULL);
  ld->channel_menu = gtk_option_menu_new ();
  gtk_box_pack_start (GTK_BOX (channel_hbox), ld->channel_menu, FALSE, FALSE, 2);

  gtk_widget_show (label);
  gtk_widget_show (ld->channel_menu);
  gtk_widget_show (channel_hbox);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (ld->channel_menu), menu);

  /*  Horizontal box for levels text widget  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new ("Input Levels: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  low input text  */
  
  ld->low_input_text = gtk_entry_new ();
/*gtk_entry_set_text (GTK_ENTRY (ld->low_input_text), "0");*/
  gtk_entry_set_text (GTK_ENTRY (ld->low_input_text), ui_strings[LOW_INPUT_STRING]);
  gtk_widget_set_usize (ld->low_input_text, TEXT_WIDTH, 25);
  gtk_box_pack_start (GTK_BOX (hbox), ld->low_input_text, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (ld->low_input_text), "changed",
		      (GtkSignalFunc) levels_low_input_text_update,
		      ld);
  gtk_widget_show (ld->low_input_text);

  /* input gamma text  */
  ld->gamma_text = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (ld->gamma_text), "1.0");
  gtk_widget_set_usize (ld->gamma_text, TEXT_WIDTH, 25);
  gtk_box_pack_start (GTK_BOX (hbox), ld->gamma_text, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (ld->gamma_text), "changed",
		      (GtkSignalFunc) levels_gamma_text_update,
		      ld);
  gtk_widget_show (ld->gamma_text);
  gtk_widget_show (hbox);

  /* high input text  */
  ld->high_input_text = gtk_entry_new ();
/*gtk_entry_set_text (GTK_ENTRY (ld->high_input_text), "255");*/
  gtk_entry_set_text (GTK_ENTRY (ld->high_input_text), ui_strings[HIGH_INPUT_STRING]);
  gtk_widget_set_usize (ld->high_input_text, TEXT_WIDTH, 25);
  gtk_box_pack_start (GTK_BOX (hbox), ld->high_input_text, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (ld->high_input_text), "changed",
		      (GtkSignalFunc) levels_high_input_text_update,
		      ld);
  gtk_widget_show (ld->high_input_text);
  gtk_widget_show (hbox);

  /*  The levels histogram  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, FALSE, 0);

  ld->histogram = histogram_create (HISTOGRAM_WIDTH, HISTOGRAM_HEIGHT,
				    levels_histogram_range, (void *) ld);
  gtk_container_add (GTK_CONTAINER (frame), ld->histogram->histogram_widget);
  gtk_widget_show (ld->histogram->histogram_widget);
  gtk_widget_show (frame);
  gtk_widget_show (hbox);

  /*  The input levels drawing area  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  vbox2 = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  ld->input_levels_da[0] = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size (GTK_PREVIEW (ld->input_levels_da[0]), DA_WIDTH, GRADIENT_HEIGHT);
  gtk_widget_set_events (ld->input_levels_da[0], LEVELS_DA_MASK);
  gtk_signal_connect (GTK_OBJECT (ld->input_levels_da[0]), "event",
		      (GtkSignalFunc) levels_input_da_events,
		      ld);
  gtk_box_pack_start (GTK_BOX (vbox2), ld->input_levels_da[0], FALSE, TRUE, 0);
  ld->input_levels_da[1] = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (ld->input_levels_da[1]), DA_WIDTH, CONTROL_HEIGHT);
  gtk_widget_set_events (ld->input_levels_da[1], LEVELS_DA_MASK);
  gtk_signal_connect (GTK_OBJECT (ld->input_levels_da[1]), "event",
		      (GtkSignalFunc) levels_input_da_events,
		      ld);
  gtk_box_pack_start (GTK_BOX (vbox2), ld->input_levels_da[1], FALSE, TRUE, 0);
  gtk_widget_show (ld->input_levels_da[0]);
  gtk_widget_show (ld->input_levels_da[1]);
  gtk_widget_show (vbox2);
  gtk_widget_show (frame);
  gtk_widget_show (hbox);

  /*  Horizontal box for levels text widget  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new ("Output Levels: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  low output text  */
  ld->low_output_text = gtk_entry_new ();
/*gtk_entry_set_text (GTK_ENTRY (ld->low_output_text), "0");*/
  gtk_entry_set_text (GTK_ENTRY (ld->low_output_text), ui_strings[LOW_OUTPUT_STRING]);
  gtk_widget_set_usize (ld->low_output_text, TEXT_WIDTH, 25);
  gtk_box_pack_start (GTK_BOX (hbox), ld->low_output_text, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (ld->low_output_text), "changed",
		      (GtkSignalFunc) levels_low_output_text_update,
		      ld);
  gtk_widget_show (ld->low_output_text);

  /*  high output text  */
  ld->high_output_text = gtk_entry_new ();
/*gtk_entry_set_text (GTK_ENTRY (ld->high_output_text), "255");*/
  gtk_entry_set_text (GTK_ENTRY (ld->high_output_text), ui_strings[HIGH_OUTPUT_STRING]);
  gtk_widget_set_usize (ld->high_output_text, TEXT_WIDTH, 25);
  gtk_box_pack_start (GTK_BOX (hbox), ld->high_output_text, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (ld->high_output_text), "changed",
		      (GtkSignalFunc) levels_high_output_text_update,
		      ld);
  gtk_widget_show (ld->high_output_text);
  gtk_widget_show (hbox);

  /*  The output levels drawing area  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  vbox2 = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  ld->output_levels_da[0] = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size (GTK_PREVIEW (ld->output_levels_da[0]), DA_WIDTH, GRADIENT_HEIGHT);
  gtk_widget_set_events (ld->output_levels_da[0], LEVELS_DA_MASK);
  gtk_signal_connect (GTK_OBJECT (ld->output_levels_da[0]), "event",
		      (GtkSignalFunc) levels_output_da_events,
		      ld);
  gtk_box_pack_start (GTK_BOX (vbox2), ld->output_levels_da[0], FALSE, TRUE, 0);
  ld->output_levels_da[1] = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size (GTK_PREVIEW (ld->output_levels_da[1]), DA_WIDTH, CONTROL_HEIGHT);
  gtk_widget_set_events (ld->output_levels_da[1], LEVELS_DA_MASK);
  gtk_signal_connect (GTK_OBJECT (ld->output_levels_da[1]), "event",
		      (GtkSignalFunc) levels_output_da_events,
		      ld);
  gtk_box_pack_start (GTK_BOX (vbox2), ld->output_levels_da[1], FALSE, TRUE, 0);
  gtk_widget_show (ld->output_levels_da[0]);
  gtk_widget_show (ld->output_levels_da[1]);
  gtk_widget_show (vbox2);
  gtk_widget_show (frame);
  gtk_widget_show (hbox);

  /*  Horizontal box for preview  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /*  The preview toggle  */
  toggle = gtk_check_button_new_with_label ("Preview");
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), ld->preview);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) levels_preview_update,
		      ld);

  gtk_widget_show (label);
  gtk_widget_show (toggle);
  gtk_widget_show (hbox);

  /*  The action area  */
  action_items[0].user_data = ld;
  action_items[1].user_data = ld;
  action_items[2].user_data = ld;
  build_action_area (GTK_DIALOG (ld->shell), action_items, 3, 0);

  gtk_widget_show (vbox);
  gtk_widget_show (ld->shell);

  return ld;
}

static void
levels_draw_slider (GdkWindow *window,
		    GdkGC     *border_gc,
		    GdkGC     *fill_gc,
		    int        xpos)
{
  int y;

  for (y = 0; y < CONTROL_HEIGHT; y++)
    gdk_draw_line(window, fill_gc, xpos - y / 2, y,
		  xpos + y / 2, y);

  gdk_draw_line(window, border_gc, xpos, 0,
		xpos - (CONTROL_HEIGHT - 1) / 2,  CONTROL_HEIGHT - 1);

  gdk_draw_line(window, border_gc, xpos, 0,
		xpos + (CONTROL_HEIGHT - 1) / 2, CONTROL_HEIGHT - 1);

  gdk_draw_line(window, border_gc, xpos - (CONTROL_HEIGHT - 1) / 2, CONTROL_HEIGHT - 1,
		xpos + (CONTROL_HEIGHT - 1) / 2, CONTROL_HEIGHT - 1);
}

static void
levels_erase_slider (GdkWindow *window,
		     int        xpos)
{
  gdk_window_clear_area (window, xpos - (CONTROL_HEIGHT - 1) / 2, 0,
			 CONTROL_HEIGHT - 1, CONTROL_HEIGHT);
}

static void
levels_update (LevelsDialog *ld,
	       int           update)
{
  char text[12];
  int i;
  
  /*  Calculate the ui transfer arrays  */
  levels_calculate_transfers_ui (ld);

  if (update & LOW_INPUT)
    {
      (*levels_update_low_input_set_text) (ld, text);
      gtk_entry_set_text (GTK_ENTRY (ld->low_input_text), text);
    }
  if (update & GAMMA)
    {
      sprintf (text, "%2.2f", ld->gamma[ld->channel]);
      gtk_entry_set_text (GTK_ENTRY (ld->gamma_text), text);
    }
  if (update & HIGH_INPUT)
    {
      (*levels_update_high_input_set_text) (ld, text);
      gtk_entry_set_text (GTK_ENTRY (ld->high_input_text), text);
    }
  if (update & LOW_OUTPUT)
    {
      (*levels_update_low_output_set_text) (ld, text);
      gtk_entry_set_text (GTK_ENTRY (ld->low_output_text), text);
    }
  if (update & HIGH_OUTPUT)
    {
      (*levels_update_high_output_set_text) (ld, text);
      gtk_entry_set_text (GTK_ENTRY (ld->high_output_text), text);
    }
  if (update & INPUT_LEVELS)
    {
      unsigned char buf[DA_WIDTH];
      /* use the ui input transfer for the da */
      for (i = 0; i < DA_WIDTH; i++)
	buf[i] = ld->input_ui[ld->channel][i];
      
      for (i = 0; i < GRADIENT_HEIGHT; i++)
	gtk_preview_draw_row (GTK_PREVIEW (ld->input_levels_da[0]),
			      buf, 0, i, DA_WIDTH);

      if (update & DRAW)
	gtk_widget_draw (ld->input_levels_da[0], NULL);
    }
  if (update & OUTPUT_LEVELS)
    {
      unsigned char buf[DA_WIDTH];

      for (i = 0; i < DA_WIDTH; i++)
	buf[i] = i;

      for (i = 0; i < GRADIENT_HEIGHT; i++)
	gtk_preview_draw_row (GTK_PREVIEW (ld->output_levels_da[0]),
			      buf, 0, i, DA_WIDTH);

      if (update & DRAW)
	gtk_widget_draw (ld->output_levels_da[0], NULL);
    }
  if (update & INPUT_SLIDERS)
    {
      double width, mid, tmp;

      levels_erase_slider (ld->input_levels_da[1]->window, ld->slider_pos[0]);
      levels_erase_slider (ld->input_levels_da[1]->window, ld->slider_pos[1]);
      levels_erase_slider (ld->input_levels_da[1]->window, ld->slider_pos[2]);

      ld->slider_pos[0] = DA_WIDTH * ((*levels_get_low_input) (ld));
      ld->slider_pos[2] = DA_WIDTH * ((*levels_get_high_input) (ld));
      
      width = (double) (ld->slider_pos[2] - ld->slider_pos[0]) / 2.0;
      mid = ld->slider_pos[0] + width;
      tmp = log10 (1.0 / ld->gamma[ld->channel]);
      ld->slider_pos[1] = (int) (mid + width * tmp + 0.5);

      levels_draw_slider (ld->input_levels_da[1]->window,
			  ld->input_levels_da[1]->style->black_gc,
			  ld->input_levels_da[1]->style->dark_gc[GTK_STATE_NORMAL],
			  ld->slider_pos[1]);
      levels_draw_slider (ld->input_levels_da[1]->window,
			  ld->input_levels_da[1]->style->black_gc,
			  ld->input_levels_da[1]->style->black_gc,
			  ld->slider_pos[0]);
      levels_draw_slider (ld->input_levels_da[1]->window,
			  ld->input_levels_da[1]->style->black_gc,
			  ld->input_levels_da[1]->style->white_gc,
			  ld->slider_pos[2]);
    }
  if (update & OUTPUT_SLIDERS)
    {
      
      levels_erase_slider (ld->output_levels_da[1]->window, ld->slider_pos[3]);
      levels_erase_slider (ld->output_levels_da[1]->window, ld->slider_pos[4]);

      ld->slider_pos[3] = DA_WIDTH * ((*levels_get_low_output) (ld));
      ld->slider_pos[4] = DA_WIDTH * ((*levels_get_high_output) (ld));

      levels_draw_slider (ld->output_levels_da[1]->window,
			  ld->output_levels_da[1]->style->black_gc,
			  ld->output_levels_da[1]->style->black_gc,
			  ld->slider_pos[3]);
      levels_draw_slider (ld->output_levels_da[1]->window,
			  ld->output_levels_da[1]->style->black_gc,
			  ld->output_levels_da[1]->style->white_gc,
			  ld->slider_pos[4]);
    }
}


static void
levels_preview (LevelsDialog *ld)
{
  if (!ld->image_map)
    g_warning ("No image map");
  (*levels_calculate_transfers) (ld);
  image_map_apply_16 (ld->image_map, levels, (void *) ld);
}

static void
levels_value_callback (GtkWidget *w,
		       gpointer   client_data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) client_data;

  if (ld->channel != HISTOGRAM_VALUE)
    {
      ld->channel = HISTOGRAM_VALUE;
      histogram_channel (ld->histogram, ld->channel);
      levels_update (ld, ALL);
    }
}

static void
levels_red_callback (GtkWidget *w,
		     gpointer   client_data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) client_data;

  if (ld->channel != HISTOGRAM_RED)
    {
      ld->channel = HISTOGRAM_RED;
      histogram_channel (ld->histogram, ld->channel);
      levels_update (ld, ALL);
    }
}

static void
levels_green_callback (GtkWidget *w,
		       gpointer   client_data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) client_data;

  if (ld->channel != HISTOGRAM_GREEN)
    {
      ld->channel = HISTOGRAM_GREEN;
      histogram_channel (ld->histogram, ld->channel);
      levels_update (ld, ALL);
    }
}

static void
levels_blue_callback (GtkWidget *w,
		      gpointer   client_data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) client_data;

  if (ld->channel != HISTOGRAM_BLUE)
    {
      ld->channel = HISTOGRAM_BLUE;
      histogram_channel (ld->histogram, ld->channel);
      levels_update (ld, ALL);
    }
}

static void
levels_alpha_callback (GtkWidget *w,
		      gpointer   client_data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) client_data;

  if (ld->channel != HISTOGRAM_ALPHA)
    {
      ld->channel = HISTOGRAM_ALPHA;
      histogram_channel (ld->histogram, ld->channel);
      levels_update (ld, ALL);
    }
}

static void
levels_adjust_channel (LevelsDialog    *ld,
		       HistogramValues *values,
		       int              channel)
{
  int i;
  double count, new_count, percentage, next_percentage;
  guint8 * low_input = (guint8*)pixelrow_data (&ld->low_input_pr);
  guint8 * high_input = (guint8*)pixelrow_data (&ld->high_input_pr);
  guint8 * low_output = (guint8*)pixelrow_data (&ld->low_output_pr);
  guint8 * high_output = (guint8*)pixelrow_data (&ld->high_output_pr);

  ld->gamma[channel]       = 1.0;
  low_output[channel]  = 0;
  high_output[channel] = 255;

  count = 0.0;
  for (i = 0; i < 256; i++)
    count += (*values)[channel][i];

  if (count == 0.0)
    {
      low_input[channel] = 0;
      high_input[channel] = 0;
    }
  else
    {
      /*  Set the low input  */
      new_count = 0.0;
      for (i = 0; i < 255; i++)
	{
	  new_count += (*values)[channel][i];
	  percentage = new_count / count;
	  next_percentage = (new_count + (*values)[channel][i + 1]) / count;

	  if (fabs (percentage - 0.006) < fabs (next_percentage - 0.006))
	    {
	      low_input[channel] = i + 1;
	      break;
	    }
	}
      /*  Set the high input  */
      new_count = 0.0;
      for (i = 255; i > 0; i--)
	{
	  new_count += (*values)[channel][i];
	  percentage = new_count / count;
	  next_percentage = (new_count + (*values)[channel][i - 1]) / count;

	  if (fabs (percentage - 0.006) < fabs (next_percentage - 0.006))
	    {
	      high_input[channel] = i - 1;
	      break;
	    }
	}
    }
}

static void
levels_auto_levels_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  LevelsDialog *ld;
  HistogramValues *values;
  int channel;

  ld = (LevelsDialog *) client_data;
  values = histogram_values (ld->histogram);

  if (ld->color)
    {
      /*  Set the overall value to defaults  */
      guint8 * low_input = (guint8*)pixelrow_data (&ld->low_input_pr);
      guint8 * high_input = (guint8*)pixelrow_data (&ld->high_input_pr);
      guint8 * low_output = (guint8*)pixelrow_data (&ld->low_output_pr);
      guint8 * high_output = (guint8*)pixelrow_data (&ld->high_output_pr);

      low_input[HISTOGRAM_VALUE]   = 0;
      high_input[HISTOGRAM_VALUE]  = 255;
      low_output[HISTOGRAM_VALUE]  = 0;
      high_output[HISTOGRAM_VALUE] = 255;
      
      ld->gamma[HISTOGRAM_VALUE]       = 1.0;

      for (channel = 0; channel < 3; channel ++)
	levels_adjust_channel (ld, values, channel + 1);
    }
  else
    levels_adjust_channel (ld, values, HISTOGRAM_VALUE);

  levels_update (ld, ALL);
  if (ld->preview)
    levels_preview (ld);
}

static void
levels_ok_callback (GtkWidget *widget,
		    gpointer   client_data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) client_data;

  if (GTK_WIDGET_VISIBLE (ld->shell))
    gtk_widget_hide (ld->shell);

  if (!ld->preview)
  {
    (*levels_calculate_transfers) (ld);
    image_map_apply_16 (ld->image_map, levels, (void *) ld);
  }

  if (ld->image_map)
  {
    image_map_commit_16 (ld->image_map);
    levels_free_transfers(ld);
  }

  ld->image_map = NULL;
}

static gint
levels_delete_callback (GtkWidget *w,
			GdkEvent *e,
			gpointer client_data)
{
  levels_cancel_callback (w, client_data);

  return FALSE;
}

static void
levels_cancel_callback (GtkWidget *widget,
			gpointer   client_data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) client_data;
  if (GTK_WIDGET_VISIBLE (ld->shell))
    gtk_widget_hide (ld->shell);

  if (ld->image_map)
    {
      image_map_abort_16 (ld->image_map);
      levels_free_transfers(ld);
      gdisplays_flush ();
    }

  ld->image_map = NULL;
}

static void
levels_preview_update (GtkWidget *w,
		       gpointer   data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    {
      ld->preview = TRUE;
      levels_preview (ld);
    }
  else
    ld->preview = FALSE;
}

static void
levels_low_input_text_update (GtkWidget *w,
			      gpointer   data)
{
  LevelsDialog *ld;
  char *str;

  str = gtk_entry_get_text (GTK_ENTRY (w));
  ld = (LevelsDialog *) data;

  if ( (*levels_low_input_text_check) (ld, str))
    {
      levels_update (ld, INPUT_LEVELS | INPUT_SLIDERS | DRAW);

      if (ld->preview)
	levels_preview (ld);
    }
}




static void
levels_gamma_text_update (GtkWidget *w,
			  gpointer   data)
{
  LevelsDialog *ld;
  char *str;
  double value;

  str = gtk_entry_get_text (GTK_ENTRY (w));
  ld = (LevelsDialog *) data;
  value = BOUNDS ((atof (str)), 0.1, 10.0);

  if (value != ld->gamma[ld->channel])
    {
      ld->gamma[ld->channel] = value;
      levels_update (ld, INPUT_LEVELS | INPUT_SLIDERS | DRAW);

      if (ld->preview)
	levels_preview (ld);
    }
}

static void
levels_high_input_text_update (GtkWidget *w,
			       gpointer   data)
{
  LevelsDialog *ld;
  char *str;
  
  str = gtk_entry_get_text (GTK_ENTRY (w));
  ld = (LevelsDialog *) data;

  if ( (*levels_high_input_text_check)(ld, str))
    {
      levels_update (ld, INPUT_LEVELS | INPUT_SLIDERS | DRAW);

      if (ld->preview)
	levels_preview (ld);
    }
}

static void
levels_low_output_text_update (GtkWidget *w,
			       gpointer   data)
{
  LevelsDialog *ld;
  char *str;

  str = gtk_entry_get_text (GTK_ENTRY (w));
  ld = (LevelsDialog *) data;

  if ( (*levels_low_output_text_check) (ld, str))
    {
      levels_update (ld, OUTPUT_LEVELS | OUTPUT_SLIDERS | DRAW);

      if (ld->preview)
	levels_preview (ld);
    }
}


static void
levels_high_output_text_update (GtkWidget *w,
				gpointer   data)
{
  LevelsDialog *ld;
  char *str;

  str = gtk_entry_get_text (GTK_ENTRY (w));
  ld = (LevelsDialog *) data;

  if ( (*levels_high_output_text_check)(ld, str))
    {
      levels_update (ld, OUTPUT_LEVELS | OUTPUT_SLIDERS | DRAW);

      if (ld->preview)
	levels_preview (ld);
    }
}

static gint
levels_input_da_events (GtkWidget    *widget,
			GdkEvent     *event,
			LevelsDialog *ld)
{
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  char text[12];
  double width, mid, tmp;
  int x, distance;
  int i;

  switch (event->type)
    {
    case GDK_EXPOSE:
      if (widget == ld->input_levels_da[1])
	levels_update (ld, INPUT_SLIDERS);
      break;

    case GDK_BUTTON_PRESS:
      gtk_grab_add (widget);
      bevent = (GdkEventButton *) event;

      distance = G_MAXINT;
      for (i = 0; i < 3; i++)
	if (fabs (bevent->x - ld->slider_pos[i]) < distance)
	  {
	    ld->active_slider = i;
	    distance = fabs (bevent->x - ld->slider_pos[i]);
	  }

      x = bevent->x;
      (*levels_input_da_setui_values) (ld, x);
      levels_update (ld, INPUT_SLIDERS | INPUT_LEVELS | DRAW);
      break;

    case GDK_BUTTON_RELEASE:
      gtk_grab_remove (widget);
      switch (ld->active_slider)
	{
	case 0:  /*  low input  */
	  levels_update (ld, LOW_INPUT | GAMMA | DRAW);
	  break;
	case 1:  /*  gamma  */
	  levels_update (ld, GAMMA);
	  break;
	case 2:  /*  high input  */
	  levels_update (ld, HIGH_INPUT | GAMMA | DRAW);
	  break;
	}

      if (ld->preview)
	levels_preview (ld);
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      gdk_window_get_pointer (widget->window, &x, NULL, NULL);
      (*levels_input_da_setui_values) (ld, x);
      switch (ld->active_slider)
	{
	case 0:  /*  low input  */
	  levels_update (ld, INPUT_SLIDERS | INPUT_LEVELS | LOW_INPUT | GAMMA | DRAW);
	  break;
	case 1:  /*  gamma  */
	  levels_update (ld, INPUT_SLIDERS | INPUT_LEVELS | GAMMA | DRAW);
	  break;
	case 2:  /*  high input  */
	  levels_update (ld, INPUT_SLIDERS | INPUT_LEVELS | HIGH_INPUT | GAMMA | DRAW);
	  break;
	}
      break;

    default:
      break;
    }

  return FALSE;
}


static gint
levels_output_da_events (GtkWidget    *widget,
			 GdkEvent     *event,
			 LevelsDialog *ld)
{
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  int x, distance;
  int i;

  switch (event->type)
    {
    case GDK_EXPOSE:
      if (widget == ld->output_levels_da[1])
	levels_update (ld, OUTPUT_SLIDERS);
      break;


    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      distance = G_MAXINT;
      for (i = 3; i < 5; i++)
	if (fabs (bevent->x - ld->slider_pos[i]) < distance)
	  {
	    ld->active_slider = i;
	    distance = fabs (bevent->x - ld->slider_pos[i]);
	  }

      x = bevent->x;
      (*levels_output_da_setui_values) (ld, x);	
      levels_update (ld, OUTPUT_SLIDERS | DRAW);
      break;

    case GDK_BUTTON_RELEASE:
      switch (ld->active_slider)
	{
	case 3:  /*  low output  */
	  levels_update (ld, LOW_OUTPUT | DRAW);
	  break;
	case 4:  /*  high output  */
	  levels_update (ld, HIGH_OUTPUT | DRAW);
	  break;
	}

      if (ld->preview)
	levels_preview (ld);
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      gdk_window_get_pointer (widget->window, &x, NULL, NULL);
      (*levels_output_da_setui_values) (ld, x);	
      switch (ld->active_slider)
	{
	case 3:  /*  low output  */
	  levels_update (ld, OUTPUT_SLIDERS | LOW_OUTPUT | DRAW);
	  break;
	case 4:  /*  high output  */
	  levels_update (ld, OUTPUT_SLIDERS | HIGH_OUTPUT | DRAW);
	  break;
	}
      break;

    default:
      break;
    }

  return FALSE;
}


/*  The levels procedure definition  */
ProcArg levels_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_INT32,
    "channel",
    "the channel to modify: { VALUE (0), RED (1), GREEN (2), BLUE (3), GRAY (0) }"
  },
  { PDB_INT32,
    "low_input",
    "intensity of lowest input: (0 <= low_input <= 255)"
  },
  { PDB_INT32,
    "high_input",
    "intensity of highest input: (0 <= high_input <= 255)"
  },
  { PDB_FLOAT,
    "gamma",
    "gamma correction factor: (0.1 <= gamma <= 10)"
  },
  { PDB_INT32,
    "low_output",
    "intensity of lowest output: (0 <= low_input <= 255)"
  },
  { PDB_INT32,
    "high_output",
    "intensity of highest output: (0 <= high_input <= 255)"
  }
};

ProcRecord levels_proc =
{
  "gimp_levels",
  "Modifies intensity levels in the specified drawable",
  "This tool allows intensity levels in the specified drawable to be remapped according to a set of parameters.  The low/high input levels specify an initial mapping from the source intensities.  The gamma value determines how intensities between the low and high input intensities are interpolated.  A gamma value of 1.0 results in a linear interpolation.  Higher gamma values result in more high-level intensities.  Lower gamma values result in more low-level intensities.  The low/high output levels constrain the final intensity mapping--that is, no final intensity will be lower than the low output level and no final intensity will be higher than the high output level.  This tool is only valid on RGB color and grayscale images.  It will not operate on indexed drawables.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  8,
  levels_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { levels_invoker } },
};


static Argument *
levels_invoker (Argument *args)
{
#if 0
  PixelArea src_area, dest_area;
  int success = TRUE;
  LevelsDialog ld;
  GImage *gimage;
  GimpDrawable *drawable;
  int channel;
  int low_input;
  int high_input;
  double gamma;
  int low_output;
  int high_output;
  int int_value;
  double fp_value;
  int x1, y1, x2, y2;
  int i;
  void *pr;

  drawable = NULL;
  low_input   = 0;
  high_input  = 0;
  gamma       = 1.0;
  low_output  = 0;
  high_output = 0;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
	success = FALSE;
    }
  /*  the drawable  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL || gimage != drawable_gimage (drawable))
	success = FALSE;
    }
  /*  make sure the drawable is not indexed color  */
  if (success)
    success = ! drawable_indexed (drawable);
   
  /*  channel  */
  if (success)
    {
      int_value = args[2].value.pdb_int;
      if (success)
	{
	  if (drawable_gray (drawable))
	    {
	      if (int_value != 0)
		success = FALSE;
	    }
	  else if (drawable_color (drawable))
	    {
	      if (int_value < 0 || int_value > 3)
		success = FALSE;
	    }
	  else
	    success = FALSE;
	}
      channel = int_value;
    }
  /*  low input  */
  if (success)
    {
      int_value = args[3].value.pdb_int;
      if (int_value >= 0 && int_value < 256)
	low_input = int_value;
      else
	success = FALSE;
    }
  /*  high input  */
  if (success)
    {
      int_value = args[4].value.pdb_int;
      if (int_value >= 0 && int_value < 256)
	high_input = int_value;
      else
	success = FALSE;
    }
  /*  gamma  */
  if (success)
    {
      fp_value = args[5].value.pdb_float;
      if (fp_value >= 0.1 && fp_value <= 10.0)
	gamma = fp_value;
      else
	success = FALSE;
    }
  /*  low output  */
  if (success)
    {
      int_value = args[6].value.pdb_int;
      if (int_value >= 0 && int_value < 256)
	low_output = int_value;
      else
	success = FALSE;
    }
  /*  high output  */
  if (success)
    {
      int_value = args[7].value.pdb_int;
      if (int_value >= 0 && int_value < 256)
	high_output = int_value;
      else
	success = FALSE;
    }

  /*  arrange to modify the levels  */
  if (success)
    {
      for (i = 0; i < 5; i++)
	{
	  ld.low_input[i] = 0;
	  ld.gamma[i] = 1.0;
	  ld.high_input[i] = 255;
	  ld.low_output[i] = 0;
	  ld.high_output[i] = 255;
	}

      ld.channel = channel;
      ld.color = drawable_color (drawable);
      ld.low_input[channel] = low_input;
      ld.high_input[channel] = high_input;
      ld.gamma[channel] = gamma;
      ld.low_output[channel] = low_output;
      ld.high_output[channel] = high_output;

      /*  calculate the transfer arrays  */
      levels_calculate_transfers (&ld);

      /*  The application should occur only within selection bounds  */
      drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

      pixelarea_init (&src_area, drawable_data_canvas (drawable), NULL, 
			x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixelarea_init (&dest_area, drawable_shadow_canvas (drawable), NULL, 
			x1, y1, (x2 - x1), (y2 - y1), TRUE);

      for (pr = pixelarea_register (2, &src_area, &dest_area); 
		pr != NULL; 
		pr = pixelarea_process (pr))
	levels (&src_area, &dest_area, (void *) &ld);

      drawable_merge_shadow_canvas (drawable, TRUE);
      drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));
    }

  return procedural_db_return_args (&levels_proc, success);
#endif
  return NULL;
}
