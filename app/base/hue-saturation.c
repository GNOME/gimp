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
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "appenv.h"
#include "actionarea.h"
#include "canvas.h"
#include "colormaps.h"
#include "drawable.h"
#include "general.h"
#include "gimage_mask.h"
#include "gdisplay.h"
#include "hue_saturation.h"
#include "image_map.h"
#include "interface.h"
#include "pixelarea.h"
#include "pixelrow.h"

/* two temp routines */
static void calc_rgb_to_hls(gdouble *r, gdouble *g, gdouble *b);
static void calc_hls_to_rgb(gdouble *r, gdouble *g, gdouble *b);

#define HUE_PARTITION_MASK  GDK_EXPOSURE_MASK | GDK_ENTER_NOTIFY_MASK

#define TEXT_WIDTH  45
#define TEXT_HEIGHT 25
#define SLIDER_WIDTH  200
#define SLIDER_HEIGHT 35
#define DA_WIDTH  40
#define DA_HEIGHT 20

#define HUE_PARTITION     0x0
#define HUE_SLIDER        0x1
#define LIGHTNESS_SLIDER  0x2
#define SATURATION_SLIDER 0x4
#define HUE_TEXT          0x8
#define LIGHTNESS_TEXT    0x10
#define SATURATION_TEXT   0x20
#define DRAW              0x40
#define ALL               0xFF

typedef struct _HueSaturation HueSaturation;

struct _HueSaturation
{
  int x, y;    /*  coords for last mouse click  */
};

typedef struct _HueSaturationDialog HueSaturationDialog;

struct _HueSaturationDialog
{
  GtkWidget   *shell;
  GtkWidget   *gimage_name;
  GtkWidget   *hue_text;
  GtkWidget   *lightness_text;
  GtkWidget   *saturation_text;
  GtkWidget   *hue_partition_da[6];
  GtkAdjustment  *hue_data;
  GtkAdjustment  *lightness_data;
  GtkAdjustment  *saturation_data;

  GimpDrawable *drawable;
  ImageMap16   image_map;

  double       hue[7];
  double       lightness[7];
  double       saturation[7];

  int          hue_partition;
  gint         preview;
};

/*  hue saturation action functions  */

static void   hue_saturation_button_press   (Tool *, GdkEventButton *, gpointer);
static void   hue_saturation_button_release (Tool *, GdkEventButton *, gpointer);
static void   hue_saturation_motion         (Tool *, GdkEventMotion *, gpointer);
static void   hue_saturation_cursor_update  (Tool *, GdkEventMotion *, gpointer);
static void   hue_saturation_control        (Tool *, int, gpointer);

static HueSaturationDialog *  hue_saturation_new_dialog        (void);
static void   hue_saturation_update                  (HueSaturationDialog *, int);
static void   hue_saturation_preview                 (HueSaturationDialog *);
static void   hue_saturation_ok_callback             (GtkWidget *, gpointer);
static void   hue_saturation_cancel_callback         (GtkWidget *, gpointer);
static gint   hue_saturation_delete_callback         (GtkWidget *, GdkEvent *, gpointer);
static void   hue_saturation_master_callback         (GtkWidget *, gpointer);
static void   hue_saturation_R_callback              (GtkWidget *, gpointer);
static void   hue_saturation_Y_callback              (GtkWidget *, gpointer);
static void   hue_saturation_G_callback              (GtkWidget *, gpointer);
static void   hue_saturation_C_callback              (GtkWidget *, gpointer);
static void   hue_saturation_B_callback              (GtkWidget *, gpointer);
static void   hue_saturation_M_callback              (GtkWidget *, gpointer);
static void   hue_saturation_preview_update          (GtkWidget *, gpointer);
static void   hue_saturation_hue_scale_update        (GtkAdjustment *, gpointer);
static void   hue_saturation_lightness_scale_update  (GtkAdjustment *, gpointer);
static void   hue_saturation_saturation_scale_update (GtkAdjustment *, gpointer);
static void   hue_saturation_hue_text_update         (GtkWidget *, gpointer);
static void   hue_saturation_lightness_text_update   (GtkWidget *, gpointer);
static void   hue_saturation_saturation_text_update  (GtkWidget *, gpointer);
static gint   hue_saturation_hue_partition_events    (GtkWidget *, GdkEvent *, HueSaturationDialog *);

static void *hue_saturation_options = NULL;
static HueSaturationDialog *hue_saturation_dialog = NULL;

static Argument * hue_saturation_invoker  (Argument *);

static void hue_saturation_funcs (Tag);

typedef void (*HueSaturationFunc)(PixelArea *, PixelArea *, void *);
static HueSaturationFunc hue_saturation;
typedef void (*HueSaturationCalculateTransfersFunc)(gint, gint, gint);
static HueSaturationCalculateTransfersFunc hue_saturation_calculate_transfers;
typedef void (*HueSaturationAllocTransfersFunc)(void);
static HueSaturationAllocTransfersFunc hue_saturation_alloc_transfers;

static void hue_saturation_u8       (PixelArea *, PixelArea *, void *);
static void hue_saturation_calculate_transfers_u8 (gint, gint, gint);
static void hue_saturation_alloc_transfers_u8 (void);

static void hue_saturation_u16       (PixelArea *, PixelArea *, void *);
static void hue_saturation_calculate_transfers_u16 (gint, gint, gint);
static void hue_saturation_alloc_transfers_u16 (void);

static void hue_saturation_float       (PixelArea *, PixelArea *, void *);
static void hue_saturation_calculate_transfers_float (gint, gint, gint);
static void hue_saturation_alloc_transfers_float (void);

static void hue_saturation_init_transfers (void *);
static void hue_saturation_free_transfers (void);

/* formulas for calculating the transfers */
static gdouble hue_value (gdouble, gdouble, gdouble);
static gdouble saturation_value (gdouble, gdouble, gdouble);
static gdouble lightness_value (gdouble, gdouble, gdouble);

/*  Local variables  */
static PixelRow hue_trans[6];
static PixelRow lightness_trans[6];
static PixelRow saturation_trans[6];

static double hue[7]; 
static double lightness[7];
static double saturation[7];

static gdouble default_colors_double[6][3] =
{
  { 1.0, 0, 0 },
  { 1.0, 1.0, 0 },
  { 0, 1.0, 0 },
  { 0, 1.0, 1.0 },
  { 0, 0, 1.0 },
  { 1.0, 0, 1.0 }
};

/*  hue saturation machinery  */

static void
hue_saturation_funcs (Tag drawable_tag)
{
  switch (tag_precision (drawable_tag))
  {
  case PRECISION_U8:
    hue_saturation_alloc_transfers = hue_saturation_alloc_transfers_u8;
    hue_saturation_calculate_transfers = hue_saturation_calculate_transfers_u8;
    hue_saturation = hue_saturation_u8;
    break;
  case PRECISION_U16:
    hue_saturation_alloc_transfers = hue_saturation_alloc_transfers_u16;
    hue_saturation_calculate_transfers = hue_saturation_calculate_transfers_u16;
    hue_saturation = hue_saturation_u16;
    break;
  case PRECISION_FLOAT:
    hue_saturation_alloc_transfers = hue_saturation_alloc_transfers_float;
    hue_saturation_calculate_transfers = hue_saturation_calculate_transfers_float;
    hue_saturation = hue_saturation_float;
    break;
  default:
    hue_saturation_alloc_transfers = NULL;
    hue_saturation_calculate_transfers = NULL;
    hue_saturation = NULL;
    break; 
  }
}


static void
hue_saturation_alloc_transfers_u8 (void)
{
  gint i;
  Tag tag = tag_new (PRECISION_U8, FORMAT_GRAY, ALPHA_NO);
  
  for (i = 0; i < 6; i++)
  { 
       guint8* hue_data = (guint8*) g_malloc (sizeof(guint8) * 256 );
       guint8* lightness_data = (guint8*) g_malloc (sizeof(guint8) * 256 );
       guint8* saturation_data = (guint8*) g_malloc (sizeof(guint8) * 256 );
       pixelrow_init (&hue_trans[i], tag, (guchar*)hue_data, 256);
       pixelrow_init (&lightness_trans[i], tag, (guchar*)lightness_data, 256);
       pixelrow_init (&saturation_trans[i], tag, (guchar*)saturation_data, 256);
  }
  
  /* the values for the transfer arrays */ 
  for (i = 0; i < 7; i++)
  {
    hue[i] = 0.0;
    lightness[i] = 0.0;
    saturation[i] = 0.0;
  }
  hue_saturation_calculate_transfers_u8 ( TRUE, TRUE, TRUE);
}

static void
hue_saturation_alloc_transfers_u16 (void)
{
  gint i;
  Tag tag = tag_new (PRECISION_U16, FORMAT_GRAY, ALPHA_NO);
  
  for (i = 0; i < 6; i++)
  { 
       guint16* hue_data = (guint16*) g_malloc (sizeof(guint16) * 65536 );
       guint16* lightness_data = (guint16*) g_malloc (sizeof(guint16) * 65536 );
       guint16* saturation_data = (guint16*) g_malloc (sizeof(guint16) * 65536 );
       pixelrow_init (&hue_trans[i], tag, (guchar*)hue_data, 65536);
       pixelrow_init (&lightness_trans[i], tag, (guchar*)lightness_data, 65536);
       pixelrow_init (&saturation_trans[i], tag, (guchar*)saturation_data, 65536);
  }
  
  /* the values for the transfer arrays */ 
  for (i = 0; i < 7; i++)
  {
    hue[i] = 0.0;
    lightness[i] = 0.0;
    saturation[i] = 0.0;
  }
  hue_saturation_calculate_transfers_u16 ( TRUE, TRUE, TRUE);
}

static void
hue_saturation_alloc_transfers_float (void)
{
  g_warning("hue_saturation_alloc_transfers_float not implemented yet\n");
}

static void
hue_saturation_free_transfers (void)
{
  gint i;
  for (i = 0; i < 6; i++)
  { 
     guchar* hue_data =  pixelrow_data (&hue_trans[i]);
     guchar* lightness_data = pixelrow_data (&lightness_trans[i]);
     guchar* saturation_data = pixelrow_data (&saturation_trans[i]);
     g_free (hue_data);
     g_free (lightness_data);
     g_free (saturation_data);
     pixelrow_init (&hue_trans[i], tag_null(), NULL, 0); 
     pixelrow_init (&lightness_trans[i], tag_null(), NULL, 0); 
     pixelrow_init (&saturation_trans[i], tag_null(), NULL, 0); 
  }
}

static void 
hue_saturation_init_transfers( void * user_data )
{
  HueSaturationDialog *hsd = (HueSaturationDialog *) user_data;
  gint do_hue, do_lightness, do_saturation;
  gint i;

  do_hue = do_lightness = do_saturation = FALSE;
  for(i = 0; i < 7; i++)
  {
    if (hue[i] != hsd->hue[i])
    {
	do_hue = TRUE;
        hue[i] = hsd->hue[i];
    }
    if (lightness[i] != hsd->lightness[i])
    {
	do_lightness = TRUE;
        lightness[i] = hsd->lightness[i];
    }
    if (saturation[i] != hsd->saturation[i])
    {
	do_saturation = TRUE;
        saturation[i] = hsd->saturation[i];
    }
  }
  (*hue_saturation_calculate_transfers) (do_hue, do_lightness, do_saturation);
}

static gdouble 
hue_value (gdouble x, gdouble hue0, gdouble hue)
{
   gdouble value;
   value = (hue0 + hue)/ 360.0;
   if ((x + value) < 0)
     return 1.0 + (value + x);
   else if ((x + value) > 1.0)
     return (x + value) - 1.0;
   else 
     return x + value;
}

static gdouble
lightness_value (gdouble x, gdouble light0, gdouble light)
{
  gdouble value;
  value = (light0 + light) * .5 / 100.0;
  value = BOUNDS (value, -1.0, 1.0);
  if (value < 0)
    return x * ( 1.0 + value );
  else
    return x + ( (1.0 - x) * value );
}

static gdouble 
saturation_value (gdouble x, gdouble sat0, gdouble sat)
{
  gdouble value;
  value = (sat0 + sat) / 100.0;
  value = BOUNDS (value, -1.0, 1.0);
  if (value < 0)
    return x * ( 1.0 + value );
  else
    return x + ( (1.0 - x) * value );
}

static void
hue_saturation_calculate_transfers_u8 (
					gint do_hue,
					gint do_lightness,
					gint do_saturation
					)
{
  int i, j;
  
  /* hue transfer */
  if (do_hue)
    for (j = 0; j < 6; j++)
    {
      guint8* hue_trans_data = (guint8*)pixelrow_data (&hue_trans[j]);
      for (i = 0; i < 256; i++)
	{
            hue_trans_data[i] = (guint8)(255.0 * hue_value ((gdouble)i/255.0, 
					hue[0], hue[j + 1]) + .5);
	}
    }
 
  /* lightness transfer */ 
  if (do_lightness)
    for (j = 0; j < 6; j++)
    {
      guint8* lightness_trans_data = (guint8*)pixelrow_data (&lightness_trans[j]);
      for (i = 0; i < 256; i++)
	{
           lightness_trans_data[i] = (guint8)( 255.0 * lightness_value ((gdouble)i/255.0, 
					lightness[0], lightness[j + 1]) + .5);
	}
    }
  
  /*  saturation  transfer */
  if (do_saturation)
    for (j = 0; j < 6; j++)
    {
      guint8* saturation_trans_data = (guint8*)pixelrow_data (&saturation_trans[j]);
      for (i = 0; i < 256; i++)
	{
          saturation_trans_data[i] = (guint8)( 255.0 * saturation_value ((gdouble)i/255.0, 
						saturation[0], saturation[j + 1]) + .5);
	}
    }
}

static void
hue_saturation_calculate_transfers_u16 (
					gint do_hue,
					gint do_lightness,
					gint do_saturation
					)
{
  int i, j;
  
  /* hue transfer */
  if (do_hue)
    for (j = 0; j < 6; j++)
    {
      guint16* hue_trans_data = (guint16*)pixelrow_data (&hue_trans[j]);
      for (i = 0; i < 65536; i++)
	{
            hue_trans_data[i] = (guint16)(65535.0 * hue_value ((gdouble)i/65535.0, 
					hue[0], hue[j + 1]) + .5);
	}
    }
 
  /* lightness transfer */ 
  if (do_lightness)
    for (j = 0; j < 6; j++)
    {
      guint16* lightness_trans_data = (guint16*)pixelrow_data (&lightness_trans[j]);
      for (i = 0; i < 65536; i++)
	{
           lightness_trans_data[i] = (guint16)( 65535.0 * lightness_value ((gdouble)i/65535.0, 
					lightness[0], lightness[j + 1]) + .5);
	}
    }
  
  /*  saturation  transfer */
  if (do_saturation)
    for (j = 0; j < 6; j++)
    {
      guint16* saturation_trans_data = (guint16*)pixelrow_data (&saturation_trans[j]);
      for (i = 0; i < 65536; i++)
	{
          saturation_trans_data[i] = (guint16)( 65535.0 * saturation_value ((gdouble)i/65535.0, 
						saturation[0], saturation[j + 1]) + .5);
	}
    }
}


static void
hue_saturation_calculate_transfers_float (
					gint do_hue,
					gint do_lightness,
					gint do_saturation
					)
{
}


static void
hue_saturation_u8 (PixelArea *src_area,
		PixelArea *dest_area,
		void        *user_data)
{
  Tag src_tag = pixelarea_tag (src_area);
  Tag dest_tag = pixelarea_tag (dest_area);
  gint src_num_channels = tag_num_channels (src_tag);
  gint dest_num_channels = tag_num_channels (dest_tag);
  HueSaturationDialog *hsd;
  guchar *src, *dest;
  guint8 *s, *d;
  int has_alpha;
  int w, h;
  gdouble r, g, b;
  int i;
  guint8* hue_trans_data[6];
  guint8* lightness_trans_data[6];
  guint8* saturation_trans_data[6];

  hsd = (HueSaturationDialog *) user_data;

  /* get pointers to the transfer tables for speed */ 
  for (i = 0 ; i < 6; i++)
  { 
    hue_trans_data[i] = (guint8*)pixelrow_data (&hue_trans[i]);
    lightness_trans_data[i] = (guint8*)pixelrow_data (&lightness_trans[i]);
    saturation_trans_data[i] = (guint8*)pixelrow_data (&saturation_trans[i]);
  } 
  
  src = (guchar *)pixelarea_data (src_area);
  dest = (guchar *)pixelarea_data (dest_area);
  has_alpha = tag_alpha (src_tag) == ALPHA_YES ? TRUE : FALSE;
  h = pixelarea_height (src_area);
  while (h--)
    {
      s = (guint8*)src;
      d = (guint8*)dest;
      w = pixelarea_width (src_area);
      while (w--)
	{
	  r = s[RED_PIX]/255.0;
	  g = s[GREEN_PIX]/255.0;
	  b = s[BLUE_PIX]/255.0;

	  calc_rgb_to_hls (&r, &g, &b);

          r /= 360.0;
          i = (int)(r * 6.0);
	  i = BOUNDS (i, 0 , 5);

	  r = hue_trans_data[i][(guint8)(r * 255.0)] / 255.0;
	  g = lightness_trans_data[i][(guint8)(g * 255.0)] / 255.0;
	  b = saturation_trans_data[i][(guint8)(b * 255.0)] / 255.0;
           
          r *= 360;
	  calc_hls_to_rgb (&r, &g, &b);

	  d[RED_PIX] = r * 255.0;
	  d[GREEN_PIX] = g * 255.0;
	  d[BLUE_PIX] = b * 255.0;

	  if (has_alpha)
	    d[ALPHA_PIX] = s[ALPHA_PIX];

	  s += src_num_channels;
	  d += dest_num_channels;
	}

      src += pixelarea_rowstride (src_area);
      dest += pixelarea_rowstride (dest_area);
    }
}

static void
hue_saturation_u16 (PixelArea *src_area,
		PixelArea *dest_area,
		void        *user_data)
{
  Tag src_tag = pixelarea_tag (src_area);
  Tag dest_tag = pixelarea_tag (dest_area);
  gint src_num_channels = tag_num_channels (src_tag);
  gint dest_num_channels = tag_num_channels (dest_tag);
  HueSaturationDialog *hsd;
  guchar *src, *dest;
  guint16 *s, *d;
  int has_alpha;
  int w, h;
  gdouble r, g, b;
  gint i;
  guint16* hue_trans_data[6];
  guint16* lightness_trans_data[6];
  guint16* saturation_trans_data[6];
  hsd = (HueSaturationDialog *) user_data;

  /* get pointers to the transfer tables for speed */ 
  for (i = 0 ; i < 6; i++)
  { 
    hue_trans_data[i] = (guint16*)pixelrow_data (&hue_trans[i]);
    lightness_trans_data[i] = (guint16*)pixelrow_data (&lightness_trans[i]);
    saturation_trans_data[i] = (guint16*)pixelrow_data (&saturation_trans[i]);
  } 
  
  src = (guchar *)pixelarea_data (src_area);
  dest = (guchar *)pixelarea_data (dest_area);
  has_alpha = tag_alpha (src_tag) == ALPHA_YES ? TRUE : FALSE;
  h = pixelarea_height (src_area);
  while (h--)
    {
      s = (guint16*)src;
      d = (guint16*)dest;
      w = pixelarea_width (src_area);
      while (w--)
	{
	  r = s[RED_PIX]/65535.0;
	  g = s[GREEN_PIX]/65535.0;
	  b = s[BLUE_PIX]/65535.0;

	  calc_rgb_to_hls (&r, &g, &b);

          r /= 360.0;
          i = (int)(r * 6.0);
	  i = BOUNDS (i, 0 , 5);

	  r = hue_trans_data[i][(guint16)(r * 65535.0)] / 65535.0;
	  g = lightness_trans_data[i][(guint16)(g * 65535.0)] / 65535.0;
	  b = saturation_trans_data[i][(guint16)(b * 65535.0)] / 65535.0;
           
          r *= 360;
	  calc_hls_to_rgb (&r, &g, &b);

	  d[RED_PIX] = r * 65535.0;
	  d[GREEN_PIX] = g * 65535.0;
	  d[BLUE_PIX] = b * 65535.0;

	  if (has_alpha)
	    d[ALPHA_PIX] = s[ALPHA_PIX];

	  s += src_num_channels;
	  d += dest_num_channels;
	}

      src += pixelarea_rowstride (src_area);
      dest += pixelarea_rowstride (dest_area);
    }
}

static void
hue_saturation_float (PixelArea *src_area,
		PixelArea *dest_area,
		void        *user_data)
{
  g_warning("hue_saturation_float not implemented yet\n");
}


static void
hue_saturation_button_press (Tool           *tool,
			     GdkEventButton *bevent,
			     gpointer        gdisp_ptr)
{
}

static void
hue_saturation_button_release (Tool           *tool,
			       GdkEventButton *bevent,
			       gpointer        gdisp_ptr)
{
}

static void
hue_saturation_motion (Tool           *tool,
		       GdkEventMotion *mevent,
		       gpointer        gdisp_ptr)
{
}

static void
hue_saturation_cursor_update (Tool           *tool,
			      GdkEventMotion *mevent,
			      gpointer        gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = (GDisplay *) gdisp_ptr;
  gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);
}

static void
hue_saturation_control (Tool     *tool,
			int       action,
			gpointer  gdisp_ptr)
{
  HueSaturation * color_bal;

  color_bal = (HueSaturation *) tool->private;

  switch (action)
    {
    case PAUSE :
      break;
    case RESUME :
      break;
    case HALT :
      if (hue_saturation_dialog)
	{
	  image_map_abort_16 (hue_saturation_dialog->image_map);
          hue_saturation_free_transfers();
	  hue_saturation_dialog->image_map = NULL;
	  hue_saturation_cancel_callback (NULL, (gpointer) hue_saturation_dialog);
	}
      break;
    }
}

Tool *
tools_new_hue_saturation ()
{
  Tool * tool;
  HueSaturation * private;

  /*  The tool options  */
  if (!hue_saturation_options)
    hue_saturation_options = tools_register_no_options (HUE_SATURATION,	"Hue-Saturation Options");

  tool = (Tool *) g_malloc (sizeof (Tool));
  private = (HueSaturation *) g_malloc (sizeof (HueSaturation));

  tool->type = HUE_SATURATION;
  tool->state = INACTIVE;
  tool->scroll_lock = 1;  /*  Disallow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = (void *) private;
  tool->button_press_func = hue_saturation_button_press;
  tool->button_release_func = hue_saturation_button_release;
  tool->motion_func = hue_saturation_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = hue_saturation_cursor_update;
  tool->control_func = hue_saturation_control;
  tool->preserve = FALSE;
  tool->gdisp_ptr = NULL;
  tool->drawable = NULL;

  return tool;
}

void
tools_free_hue_saturation (Tool *tool)
{
  HueSaturation * color_bal;

  color_bal = (HueSaturation *) tool->private;

  /*  Close the color select dialog  */
  if (hue_saturation_dialog)
    hue_saturation_cancel_callback (NULL, (gpointer) hue_saturation_dialog);

  g_free (color_bal);
}

void
hue_saturation_initialize (void *gdisp_ptr)
{
  GDisplay *gdisp;
  GimpDrawable *drawable;
  int i;

  gdisp = (GDisplay *) gdisp_ptr;

  if (! drawable_color (gimage_active_drawable (gdisp->gimage)))
    {
      message_box ("Hue-Saturation operates only on RGB color drawables.", NULL, NULL);
      return;
    }

  /*  The "by color" dialog  */
  if (!hue_saturation_dialog)
    hue_saturation_dialog = hue_saturation_new_dialog ();
  else
    if (!GTK_WIDGET_VISIBLE (hue_saturation_dialog->shell))
      gtk_widget_show (hue_saturation_dialog->shell);

  for (i = 0; i < 7; i++)
    {
      hue_saturation_dialog->hue[i] = 0.0;
      hue_saturation_dialog->lightness[i] = 0.0;
      hue_saturation_dialog->saturation[i] = 0.0;
    }
  
  drawable = gimage_active_drawable (gdisp->gimage);
  
  /* assign the function pointers for our tag type */
  hue_saturation_funcs (drawable_tag (drawable));
  (*hue_saturation_alloc_transfers)();

  hue_saturation_dialog->drawable = drawable;
  hue_saturation_dialog->image_map = image_map_create_16 (gdisp_ptr, hue_saturation_dialog->drawable);
  hue_saturation_update (hue_saturation_dialog, ALL);
}

void
hue_saturation_free ()
{
  if (hue_saturation_dialog)
    {
      if (hue_saturation_dialog->image_map)
	{
	  image_map_abort_16 (hue_saturation_dialog->image_map);
	  hue_saturation_dialog->image_map = NULL;
	}
      gtk_widget_destroy (hue_saturation_dialog->shell);
    }
}


/*  the action area structure  */
static ActionAreaItem action_items[] =
{
  { "OK", hue_saturation_ok_callback, NULL, NULL },
  { "Cancel", hue_saturation_cancel_callback, NULL, NULL }
};

static HueSaturationDialog *
hue_saturation_new_dialog ()
{
  HueSaturationDialog *hsd;
  GtkWidget *main_vbox;
  GtkWidget *main_hbox;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *slider;
  GtkWidget *toggle;
  GtkWidget *radio_button;
  GtkWidget *frame;
  GtkObject *data;
  GSList *group = NULL;
  int i;
  char *hue_partition_names[7] =
  {
    "Master",
    "R",
    "Y",
    "G",
    "C",
    "B",
    "M"
  };
  ActionCallback hue_partition_callbacks[7] =
  {
    hue_saturation_master_callback,
    hue_saturation_R_callback,
    hue_saturation_Y_callback,
    hue_saturation_G_callback,
    hue_saturation_C_callback,
    hue_saturation_B_callback,
    hue_saturation_M_callback
  };

  hsd = g_malloc (sizeof (HueSaturationDialog));
  hsd->hue_partition = 0;
  hsd->preview = TRUE;

  /*  The shell and main vbox  */
  hsd->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (hsd->shell), "hue_saturation", "Gimp");
  gtk_window_set_title (GTK_WINDOW (hsd->shell), "Hue-Saturation");
  
  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (hsd->shell), "delete_event",
		      GTK_SIGNAL_FUNC (hue_saturation_delete_callback),
		      hsd);

  main_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_border_width (GTK_CONTAINER (main_vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (hsd->shell)->vbox), main_vbox, TRUE, TRUE, 0);

  /*  The main hbox containing hue partitions and sliders  */
  main_hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (main_vbox), main_hbox, FALSE, FALSE, 0);

  /*  The table containing hue partitions  */
  table = gtk_table_new (7, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (main_hbox), table, FALSE, FALSE, 0);

  /*  the radio buttons for hue partitions  */
  for (i = 0; i < 7; i++)
    {
      radio_button = gtk_radio_button_new_with_label (group, hue_partition_names[i]);
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio_button));

      if (!i)
	{
	  gtk_table_attach (GTK_TABLE (table), radio_button, 0, 2, 0, 1,
			    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);
	}
      else
	{
	  gtk_table_attach (GTK_TABLE (table), radio_button, 0, 1, i, i + 1,
			    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);

	  frame = gtk_frame_new (NULL);
	  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	  gtk_table_attach (GTK_TABLE (table), frame,  1, 2, i, i + 1,
			    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);
	  hsd->hue_partition_da[i - 1] = gtk_preview_new (GTK_PREVIEW_COLOR);
	  gtk_preview_size (GTK_PREVIEW (hsd->hue_partition_da[i - 1]), DA_WIDTH, DA_HEIGHT);
	  gtk_widget_set_events (hsd->hue_partition_da[i - 1], HUE_PARTITION_MASK);
	  gtk_signal_connect (GTK_OBJECT (hsd->hue_partition_da[i - 1]), "event",
			      (GtkSignalFunc) hue_saturation_hue_partition_events,
			      hsd);
	  gtk_container_add (GTK_CONTAINER (frame), hsd->hue_partition_da[i - 1]);

	  gtk_widget_show (hsd->hue_partition_da[i - 1]);
	  gtk_widget_show (frame);
	}

      gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
			  (GtkSignalFunc) hue_partition_callbacks[i],
			  hsd);
      gtk_widget_show (radio_button);
    }
  gtk_widget_show (table);

  /*  The vbox for the table and preview toggle  */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, FALSE, FALSE, 0);

  label = gtk_label_new ("Hue / Lightness / Saturation Adjustments");
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  The table containing sliders  */
  table = gtk_table_new (3, 3, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  Create the hue scale widget  */
  label = gtk_label_new ("Hue");
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);

  data = gtk_adjustment_new (0, -180, 180.0, 1.0, 1.0, 0.0);
  hsd->hue_data = GTK_ADJUSTMENT (data);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (data));
  gtk_widget_set_usize (slider, SLIDER_WIDTH, SLIDER_HEIGHT);
  gtk_scale_set_digits (GTK_SCALE (slider), 0);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_table_attach (GTK_TABLE (table), slider, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    2, 2);
  gtk_signal_connect (GTK_OBJECT (data), "value_changed",
		      (GtkSignalFunc) hue_saturation_hue_scale_update,
		      hsd);

  hsd->hue_text = gtk_entry_new ();
  gtk_widget_set_usize (hsd->hue_text, TEXT_WIDTH, TEXT_HEIGHT);
  gtk_table_attach (GTK_TABLE (table), hsd->hue_text, 2, 3, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);
  gtk_signal_connect (GTK_OBJECT (hsd->hue_text), "changed",
		      (GtkSignalFunc) hue_saturation_hue_text_update,
		      hsd);

  gtk_widget_show (label);
  gtk_widget_show (hsd->hue_text);
  gtk_widget_show (slider);


  /*  Create the lightness scale widget  */
  label = gtk_label_new ("Lightness");
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);

  data = gtk_adjustment_new (0, -100.0, 100.0, 1.0, 1.0, 0.0);
  hsd->lightness_data = GTK_ADJUSTMENT (data);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (data));
  gtk_widget_set_usize (slider, SLIDER_WIDTH, SLIDER_HEIGHT);
  gtk_scale_set_digits (GTK_SCALE (slider), 0);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_table_attach (GTK_TABLE (table), slider, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    2, 2);
  gtk_signal_connect (GTK_OBJECT (data), "value_changed",
		      (GtkSignalFunc) hue_saturation_lightness_scale_update,
		      hsd);

  hsd->lightness_text = gtk_entry_new ();
  gtk_widget_set_usize (hsd->lightness_text, TEXT_WIDTH, TEXT_HEIGHT);
  gtk_table_attach (GTK_TABLE (table), hsd->lightness_text, 2, 3, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);
  gtk_signal_connect (GTK_OBJECT (hsd->lightness_text), "changed",
		      (GtkSignalFunc) hue_saturation_lightness_text_update,
		      hsd);

  gtk_widget_show (label);
  gtk_widget_show (hsd->lightness_text);
  gtk_widget_show (slider);


  /*  Create the saturation scale widget  */
  label = gtk_label_new ("Saturation");
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);

  data = gtk_adjustment_new (0, -100.0, 100.0, 1.0, 1.0, 0.0);
  hsd->saturation_data = GTK_ADJUSTMENT (data);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (data));
  gtk_widget_set_usize (slider, SLIDER_WIDTH, SLIDER_HEIGHT);
  gtk_scale_set_digits (GTK_SCALE (slider), 0);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_table_attach (GTK_TABLE (table), slider, 1, 2, 2, 3,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    2, 2);
  gtk_signal_connect (GTK_OBJECT (data), "value_changed",
		      (GtkSignalFunc) hue_saturation_saturation_scale_update,
		      hsd);

  hsd->saturation_text = gtk_entry_new ();
  gtk_widget_set_usize (hsd->saturation_text, TEXT_WIDTH, TEXT_HEIGHT);
  gtk_table_attach (GTK_TABLE (table), hsd->saturation_text, 2, 3, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);
  gtk_signal_connect (GTK_OBJECT (hsd->saturation_text), "changed",
		      (GtkSignalFunc) hue_saturation_saturation_text_update,
		      hsd);

  gtk_widget_show (label);
  gtk_widget_show (hsd->saturation_text);
  gtk_widget_show (slider);


  /*  Horizontal box for preview and colorize toggle buttons  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /*  The preview toggle  */
  toggle = gtk_check_button_new_with_label ("Preview");
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), hsd->preview);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) hue_saturation_preview_update,
		      hsd);

  gtk_widget_show (toggle);
  gtk_widget_show (hbox);


  /*  The action area  */
  action_items[0].user_data = hsd;
  action_items[1].user_data = hsd;
  build_action_area (GTK_DIALOG (hsd->shell), action_items, 2, 0);

  gtk_widget_show (table);
  gtk_widget_show (vbox);
  gtk_widget_show (main_hbox);
  gtk_widget_show (main_vbox);
  gtk_widget_show (hsd->shell);

  return hsd;
}

static void
hue_saturation_update (HueSaturationDialog *hsd,
		       int                  update)
{
  int i, j;
  unsigned char buf[DA_WIDTH * 3];
  char text[12];
  
  if (update & HUE_SLIDER)
    {
      hsd->hue_data->value = hsd->hue[hsd->hue_partition];
      gtk_signal_emit_by_name (GTK_OBJECT (hsd->hue_data), "value_changed");
    }
  if (update & LIGHTNESS_SLIDER)
    {
      hsd->lightness_data->value = hsd->lightness[hsd->hue_partition];
      gtk_signal_emit_by_name (GTK_OBJECT (hsd->lightness_data), "value_changed");
    }
  if (update & SATURATION_SLIDER)
    {
      hsd->saturation_data->value = hsd->saturation[hsd->hue_partition];
      gtk_signal_emit_by_name (GTK_OBJECT (hsd->saturation_data), "value_changed");
    }
  if (update & HUE_TEXT)
    {
      sprintf (text, "%0.0f", hsd->hue[hsd->hue_partition]);
      gtk_entry_set_text (GTK_ENTRY (hsd->hue_text), text);
    }
  if (update & LIGHTNESS_TEXT)
    {
      sprintf (text, "%0.0f", hsd->lightness[hsd->hue_partition]);
      gtk_entry_set_text (GTK_ENTRY (hsd->lightness_text), text);
    }
  if (update & SATURATION_TEXT)
    {
      sprintf (text, "%0.0f", hsd->saturation[hsd->hue_partition]);
      gtk_entry_set_text (GTK_ENTRY (hsd->saturation_text), text);
    }
  
  hue_saturation_init_transfers ((void *)hsd);

  for (i = 0; i < 6; i++)
    {
      gdouble red, blue, green;

      red = default_colors_double[i][RED_PIX];
      green = default_colors_double[i][GREEN_PIX];
      blue = default_colors_double[i][BLUE_PIX];
      
      calc_rgb_to_hls (&red, &green, &blue);

      red = hue_value (red, hue[0], hue[i+1]);
      green = lightness_value (green, lightness[0], lightness[i+1]);   
      blue = saturation_value (blue, saturation[0], saturation[i+1]);


      calc_hls_to_rgb (&red, &green, &blue);
     
      for (j = 0; j < DA_WIDTH; j++)
      {
          buf[j * 3 + 0] = red * 255.0;
          buf[j * 3 + 1] = green * 255.0;
	  buf[j * 3 + 2] = blue * 255.0; 
       }
       
      for (j = 0; j < DA_HEIGHT; j++)
	gtk_preview_draw_row (GTK_PREVIEW (hsd->hue_partition_da[i]), buf, 0, j, DA_WIDTH);

      if (update & DRAW)
	gtk_widget_draw (hsd->hue_partition_da[i], NULL);
    }
}

static void
hue_saturation_preview (HueSaturationDialog *hsd)
{
  if (!hsd->image_map)
    g_warning ("No image map");
  active_tool->preserve = TRUE;
  image_map_apply_16 (hsd->image_map, hue_saturation, (void *) hsd);
  active_tool->preserve = FALSE;
  hue_saturation_init_transfers ((void *)hsd);
}

static void
hue_saturation_ok_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  HueSaturationDialog *hsd;

  hsd = (HueSaturationDialog *) client_data;

  if (GTK_WIDGET_VISIBLE (hsd->shell))
    gtk_widget_hide (hsd->shell);

  active_tool->preserve = TRUE;

  if (!hsd->preview)
  {
    image_map_apply_16 (hsd->image_map, hue_saturation, (void *) hsd);
    hue_saturation_init_transfers ((void *)hsd);
  }

  if (hsd->image_map)
  {
    image_map_commit_16 (hsd->image_map);
    hue_saturation_free_transfers();
  }

  active_tool->preserve = FALSE;
  hsd->image_map = NULL;
}

static gint
hue_saturation_delete_callback (GtkWidget *w,
				GdkEvent *e,
				gpointer client_data)
{
  hue_saturation_cancel_callback (w, client_data);

  return TRUE;
}

static void
hue_saturation_cancel_callback (GtkWidget *widget,
				gpointer   client_data)
{
  HueSaturationDialog *hsd;

  hsd = (HueSaturationDialog *) client_data;
  if (GTK_WIDGET_VISIBLE (hsd->shell))
    gtk_widget_hide (hsd->shell);

  if (hsd->image_map)
    {
      active_tool->preserve = TRUE;
      image_map_abort_16 (hsd->image_map);
      active_tool->preserve = FALSE;
      hue_saturation_free_transfers();
      gdisplays_flush ();
    }

  hsd->image_map = NULL;
}

static void
hue_saturation_master_callback (GtkWidget *widget,
				gpointer   client_data)
{
  HueSaturationDialog *hsd;

  hsd = (HueSaturationDialog *) client_data;
  hsd->hue_partition = 0;
  hue_saturation_update (hsd, ALL);
}

static void
hue_saturation_R_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  HueSaturationDialog *hsd;

  hsd = (HueSaturationDialog *) client_data;
  hsd->hue_partition = 1;
  hue_saturation_update (hsd, ALL);
}

static void
hue_saturation_Y_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  HueSaturationDialog *hsd;

  hsd = (HueSaturationDialog *) client_data;
  hsd->hue_partition = 2;
  hue_saturation_update (hsd, ALL);
}

static void
hue_saturation_G_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  HueSaturationDialog *hsd;

  hsd = (HueSaturationDialog *) client_data;
  hsd->hue_partition = 3;
  hue_saturation_update (hsd, ALL);
}

static void
hue_saturation_C_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  HueSaturationDialog *hsd;

  hsd = (HueSaturationDialog *) client_data;
  hsd->hue_partition = 4;
  hue_saturation_update (hsd, ALL);
}

static void
hue_saturation_B_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  HueSaturationDialog *hsd;

  hsd = (HueSaturationDialog *) client_data;
  hsd->hue_partition = 5;
  hue_saturation_update (hsd, ALL);
}

static void
hue_saturation_M_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  HueSaturationDialog *hsd;

  hsd = (HueSaturationDialog *) client_data;
  hsd->hue_partition = 6;
  hue_saturation_update (hsd, ALL);
}

static void
hue_saturation_preview_update (GtkWidget *w,
			       gpointer   data)
{
  HueSaturationDialog *hsd;

  hsd = (HueSaturationDialog *) data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    {
      hsd->preview = TRUE;
      hue_saturation_preview (hsd);
    }
  else
    hsd->preview = FALSE;
}

static void
hue_saturation_hue_scale_update (GtkAdjustment *adjustment,
				 gpointer       data)
{
  HueSaturationDialog *hsd;

  hsd = (HueSaturationDialog *) data;

  if (hsd->hue[hsd->hue_partition] != adjustment->value)
    {
      hsd->hue[hsd->hue_partition] = adjustment->value;
      hue_saturation_update (hsd, HUE_TEXT | DRAW);

      if (hsd->preview)
	hue_saturation_preview (hsd);
    }
}

static void
hue_saturation_lightness_scale_update (GtkAdjustment *adjustment,
				       gpointer       data)
{
  HueSaturationDialog *hsd;

  hsd = (HueSaturationDialog *) data;

  if (hsd->lightness[hsd->hue_partition] != adjustment->value)
    {
      hsd->lightness[hsd->hue_partition] = adjustment->value;
      hue_saturation_update (hsd, LIGHTNESS_TEXT | DRAW);

      if (hsd->preview)
	hue_saturation_preview (hsd);
    }
}

static void
hue_saturation_saturation_scale_update (GtkAdjustment *adjustment,
					gpointer       data)
{
  HueSaturationDialog *hsd;

  hsd = (HueSaturationDialog *) data;

  if (hsd->saturation[hsd->hue_partition] != adjustment->value)
    {
      hsd->saturation[hsd->hue_partition] = adjustment->value;
      hue_saturation_update (hsd, SATURATION_TEXT | DRAW);

      if (hsd->preview)
	hue_saturation_preview (hsd);
    }
}

static void
hue_saturation_hue_text_update (GtkWidget *w,
				gpointer   data)
{
  HueSaturationDialog *hsd;
  char *str;
  int value;

  str = gtk_entry_get_text (GTK_ENTRY (w));
  hsd = (HueSaturationDialog *) data;
  value = BOUNDS (((int) atof (str)), -180, 180);

  if ((int) hsd->hue[hsd->hue_partition] != value)
    {
      hsd->hue[hsd->hue_partition] = value;
      hue_saturation_update (hsd, HUE_SLIDER | DRAW);

      if (hsd->preview)
	hue_saturation_preview (hsd);
    }
}

static void
hue_saturation_lightness_text_update (GtkWidget *w,
				      gpointer   data)
{
  HueSaturationDialog *hsd;
  char *str;
  int value;

  str = gtk_entry_get_text (GTK_ENTRY (w));
  hsd = (HueSaturationDialog *) data;
  value = BOUNDS (((int) atof (str)), -100, 100);

  if ((int) hsd->lightness[hsd->hue_partition] != value)
    {
      hsd->lightness[hsd->hue_partition] = value;
      hue_saturation_update (hsd, LIGHTNESS_SLIDER | DRAW);

      if (hsd->preview)
	hue_saturation_preview (hsd);
    }
}

static void
hue_saturation_saturation_text_update (GtkWidget *w,
				       gpointer   data)
{
  HueSaturationDialog *hsd;
  char *str;
  int value;

  str = gtk_entry_get_text (GTK_ENTRY (w));
  hsd = (HueSaturationDialog *) data;
  value = BOUNDS (((int) atof (str)), -100, 100);

  if ((int) hsd->saturation[hsd->hue_partition] != value)
    {
      hsd->saturation[hsd->hue_partition] = value;
      hue_saturation_update (hsd, SATURATION_SLIDER | DRAW);

      if (hsd->preview)
	hue_saturation_preview (hsd);
    }
}

static gint
hue_saturation_hue_partition_events (GtkWidget           *widget,
				     GdkEvent            *event,
				     HueSaturationDialog *hsd)
{
  switch (event->type)
    {
    case GDK_EXPOSE:
      hue_saturation_update (hsd, HUE_PARTITION);
      break;

    default:
      break;
    }

  return FALSE;
}

/*  The hue_saturation procedure definition  */
ProcArg hue_saturation_args[] =
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
    "hue_range",
    "range of affected hues: { ALL_HUES (0), RED_HUES (1), YELLOW_HUES (2), GREEN_HUES (3), CYAN_HUES (4), BLUE_HUES (5), MAGENTA_HUES (6)"
  },
  { PDB_FLOAT,
    "hue_offset",
    "hue offset in degrees: (-180 <= hue_offset <= 180)"
  },
  { PDB_FLOAT,
    "lightness",
    "lightness modification: (-100 <= lightness <= 100)"
  },
  { PDB_FLOAT,
    "saturation",
    "saturation modification: (-100 <= saturation <= 100)"
  }
};

ProcRecord hue_saturation_proc =
{
  "gimp_hue_saturation",
  "Modify hue, lightness, and saturation in the specified drawable",
  "This procedures allows the hue, lightness, and saturation in the specified drawable to be modified.  The 'hue_range' parameter provides the capability to limit range of affected hues.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1997",
  PDB_INTERNAL,

  /*  Input arguments  */
  6,
  hue_saturation_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { hue_saturation_invoker } },
};


static Argument *
hue_saturation_invoker (Argument *args)
{
  PixelArea src_area, dest_area;
  int success = TRUE;
  HueSaturationDialog hsd;
  GImage *gimage;
  GimpDrawable *drawable;
  int hue_range;
  double hue_offset;
  double lightness;
  double saturation;
  int int_value;
  double fp_value;
  int x1, y1, x2, y2;
  int i;
  void *pr;

  drawable = NULL;
  hue_range   = 0;
  hue_offset  = 0.0;
  lightness   = 0.0;
  saturation  = 0.0;

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
    
  /*  hue_range  */
  if (success)
    {
      int_value = args[2].value.pdb_int;
      if (int_value < 0 || int_value > 6)
	success = FALSE;
      else
	hue_range = int_value;
    }
  /*  hue_offset  */
  if (success)
    {
      fp_value = args[3].value.pdb_float;
      if (fp_value < -180 || fp_value > 180)
	success = FALSE;
      else
	hue_offset = fp_value;
    }
  /*  lightness  */
  if (success)
    {
      fp_value = args[4].value.pdb_float;
      if (fp_value < -100 || fp_value > 100)
	success = FALSE;
      else
	lightness = fp_value;
    }
  /*  saturation  */
  if (success)
    {
      fp_value = args[5].value.pdb_float;
      if (fp_value < -100 || fp_value > 100)
	success = FALSE;
      else
	saturation = fp_value;
    }

  /*  arrange to modify the levels  */
  if (success)
    {
      for (i = 0; i < 7; i++)
	{
	  hsd.hue[i] = 0.0;
	  hsd.lightness[i] = 0.0;
	  hsd.saturation[i] = 0.0;
	}

      hsd.hue[hue_range] = hue_offset;
      hsd.lightness[hue_range] = lightness;
      hsd.saturation[hue_range] = saturation;

      /*  calculate the transfer arrays  */
      (*hue_saturation_alloc_transfers) ();
      hue_saturation_init_transfers (&hsd);

      /*  The application should occur only within selection bounds  */
      drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

      pixelarea_init (&src_area, drawable_data_canvas (drawable), NULL, 
		x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixelarea_init (&dest_area, drawable_shadow_canvas (drawable), NULL, 
		x1, y1, (x2 - x1), (y2 - y1), TRUE);

      for (pr = pixelarea_register (2, &src_area, &dest_area); 
		pr != NULL; 
		pr = pixelarea_process (pr))
	(*hue_saturation) (&src_area, &dest_area, (void *) &hsd);

      hue_saturation_free_transfers ();
      drawable_merge_shadow_canvas (drawable, TRUE);
      drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));
    }

  return procedural_db_return_args (&hue_saturation_proc, success);
}


static void
calc_rgb_to_hls (gdouble *r,
	    gdouble *g,
	    gdouble *b)
{
  gdouble min;
  gdouble max;
  gdouble red;
  gdouble green;
  gdouble blue;
  gdouble h, l, s;
  gdouble delta;

  red = *r;
  green = *g;
  blue = *b;

  if (red > green)
    {
      if (red > blue)
	max = red;
      else
	max = blue;

      if (green < blue)
	min = green;
      else
	min = blue;
    }
  else
    {
      if (green > blue)
	max = green;
      else
	max = blue;

      if (red < blue)
	min = red;
      else
	min = blue;
    }

  l = (max + min) / 2;
  s = 0;
  h = 0;

  if (max != min)
    {
      if (l <= 0.5)
	s = (max - min) / (max + min);
      else
	s = (max - min) / (2 - max - min);

      delta = max -min;
      if (red == max)
	h = (green - blue) / delta;
      else if (green == max)
	h = 2 + (blue - red) / delta;
      else if (blue == max)
	h = 4 + (red - green) / delta;

      h *= 60;
      if (h < 0.0)
	h += 360;
    }

  *r = h;
  *g = l;
  *b = s;
}

static void
calc_hls_to_rgb (gdouble *h,
	    gdouble *l,
	    gdouble *s)
{
  gdouble hue;
  gdouble lightness;
  gdouble saturation;
  gdouble m1, m2;
  gdouble r, g, b;

  lightness = *l;
  saturation = *s;

  if (lightness <= 0.5)
    m2 = lightness * (1 + saturation);
  else
    m2 = lightness + saturation - lightness * saturation;
  m1 = 2 * lightness - m2;

  if (saturation == 0)
    {
      *h = lightness;
      *l = lightness;
      *s = lightness;
    }
  else
    {
      hue = *h + 120;
      while (hue > 360)
	hue -= 360;
      while (hue < 0)
	hue += 360;

      if (hue < 60)
	r = m1 + (m2 - m1) * hue / 60;
      else if (hue < 180)
	r = m2;
      else if (hue < 240)
	r = m1 + (m2 - m1) * (240 - hue) / 60;
      else
	r = m1;

      hue = *h;
      while (hue > 360)
	hue -= 360;
      while (hue < 0)
	hue += 360;

      if (hue < 60)
	g = m1 + (m2 - m1) * hue / 60;
      else if (hue < 180)
	g = m2;
      else if (hue < 240)
	g = m1 + (m2 - m1) * (240 - hue) / 60;
      else
	g = m1;

      hue = *h - 120;
      while (hue > 360)
	hue -= 360;
      while (hue < 0)
	hue += 360;

      if (hue < 60)
	b = m1 + (m2 - m1) * hue / 60;
      else if (hue < 180)
	b = m2;
      else if (hue < 240)
	b = m1 + (m2 - m1) * (240 - hue) / 60;
      else
	b = m1;

      *h = r;
      *l = g;
      *s = b;
    }
}
