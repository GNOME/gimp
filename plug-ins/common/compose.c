/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Compose plug-in (C) 1997 Peter Kirchgessner
 * e-mail: pkirchg@aol.com, WWW: http://members.aol.com/pkirchg
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

/*
 * This plug-in composes RGB-images from several types of channels
 */

/* Event history:
 * V 1.00, PK, 29-Jul-97, Creation
 * V 1.01, nn, 20-Dec-97, Add default case in switch for hsv_to_rgb ()
 */
static char ident[] = "@(#) GIMP Compose plug-in v1.01 20-Dec-97";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "libgimp/gimpmenu.h"

/* Declare local functions
 */
static void      query  (void);
static void      run    (char      *name,
			 int        nparams,
			 GParam    *param,
			 int       *nreturn_vals,
			 GParam   **return_vals);

static gint32    compose (char *compose_type,
                          gint32 *compose_ID);

static gint32    create_new_image (char *filename, guint width, guint height,
                   GDrawableType gdtype, gint32 *layer_ID, GDrawable **drawable,
                   GPixelRgn *pixel_rgn);

static int       cmp_icase (char *s1, char *s2);

static void      compose_rgb  (unsigned char **src, int numpix,
                               unsigned char *dst);
static void      compose_rgba (unsigned char **src, int numpix,
                               unsigned char *dst);
static void      compose_hsv  (unsigned char **src, int numpix,
                               unsigned char *dst);
static void      compose_cmy  (unsigned char **src, int numpix,
                               unsigned char *dst);
static void      compose_cmyk (unsigned char **src, int numpix,
                               unsigned char *dst);

static void      hsv_to_rgb (unsigned char *h, unsigned char *s,
                             unsigned char *v, unsigned char *rgb);

static gint      compose_dialog (char *compose_type,
                                 gint32 image_ID);

static gint      check_gray (gint32 image_id,
                             gint32 drawable_id,
                             gpointer data);

static void      image_menu_callback (gint32     id,
                                      gpointer   data);

static void      compose_close_callback      (GtkWidget *widget,
                                              gpointer   data);
static void      compose_ok_callback         (GtkWidget *widget,
                                              gpointer   data);
static void      compose_type_toggle_update  (GtkWidget *widget,
                                              gpointer   data);

/* Maximum number of images to compose */
#define MAX_COMPOSE_IMAGES 4


/* Description of a composition */
typedef struct {
  char *compose_type;             /* Type of composition ("RGB", "RGBA",...) */
  int num_images;                 /* Number of input images needed */
  char *channel_name[MAX_COMPOSE_IMAGES];  /* channel names for dialog */
  char *filename;                 /* Name of new image */
                                  /* Compose functon */
  void (*compose_fun)(unsigned char **src, int numpix, unsigned char *dst);
} COMPOSE_DSC;

/* Array of available compositions. */
static COMPOSE_DSC compose_dsc[] = {
 { "RGB",  3, { "Red:",   "Green:     ", "Blue:",   "N/A" },
              "rgb-compose",  compose_rgb },
 { "RGBA", 4, { "Red:",   "Green:     ", "Blue:",   "Alpha:" },
              "rgba-compose",  compose_rgba },
 { "HSV",  3, { "Hue:",   "Saturation:", "Value:",  "N/A" },
              "hsv-compose",  compose_hsv },
 { "CMY",  3, { "Cyan:",  "Magenta:   ", "Yellow:", "N/A" },
              "cmy-compose",  compose_cmy },
 { "CMYK", 4, { "Cyan:",  "Magenta:   ", "Yellow:", "Black:" },
              "cmyk-compose", compose_cmyk }
};

#define MAX_COMPOSE_TYPES (sizeof (compose_dsc) / sizeof (compose_dsc[0]))


typedef struct {
  gint32 compose_ID[MAX_COMPOSE_IMAGES];  /* Image IDs of input images */
  char compose_type[32];                  /* type of composition */
} ComposeVals;

/* Dialog structure */
typedef struct {
  GtkWidget *channel_label[MAX_COMPOSE_IMAGES]; /* The labels to change */

  gint32 select_ID[MAX_COMPOSE_IMAGES];  /* Image Ids selected by menu */
  gint compose_flag[MAX_COMPOSE_TYPES];  /* toggle data of compose type */
  gint run;
} ComposeInterface;

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static ComposeVals composevals =
{
  { 0 },  /* Image IDs of images to compose */
  "rgb"   /* Type of composition */
};

static ComposeInterface composeint =
{
  { NULL }, /* Label Widgets */
  { 0 },    /* Image IDs from menues */
  { 0 },    /* Compose type toggle flags */
  FALSE     /* run */
};

static GRunModeType run_mode;


MAIN ()

static void
query ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "First input image" },
    { PARAM_DRAWABLE, "drawable", "Input drawable (not used)" },
    { PARAM_IMAGE, "image", "Second input image" },
    { PARAM_IMAGE, "image", "Third input image" },
    { PARAM_IMAGE, "image", "Fourth input image" },
    { PARAM_STRING, "compose_type", "What to compose: RGB, RGBA, HSV,\
 CMY, CMYK" }
  };
  static GParamDef return_vals[] =
  {
    { PARAM_IMAGE, "new_image", "Output image" }
  };
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = sizeof (return_vals) / sizeof (return_vals[0]);

  gimp_install_procedure ("plug_in_compose",
			  "Compose an image from different types of channels",
			  "This function creates a new image from\
 different channel informations kept in gray images",
			  "Peter Kirchgessner",
			  "Peter Kirchgessner (pkirchg@aol.com)",
			  "1997",
			  "<Image>/Image/Channel Ops/Compose",
			  "GRAY",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[2];
  GStatusType status = STATUS_SUCCESS;
  gint32 image_ID;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 2;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
  values[1].type = PARAM_IMAGE;
  values[1].data.d_int32 = -1;

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_compose", &composevals);

      /*  First acquire information with a dialog  */
      if (! compose_dialog (composevals.compose_type, param[1].data.d_int32))
	return;
      break;

    case RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 7)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
          composevals.compose_ID[0] = param[1].data.d_int32;
          composevals.compose_ID[1] = param[3].data.d_int32;
          composevals.compose_ID[2] = param[4].data.d_int32;
          composevals.compose_ID[3] = param[5].data.d_int32;
          strncpy (composevals.compose_type, param[6].data.d_string,
                   sizeof (composevals.compose_type));
          composevals.compose_type[sizeof (composevals.compose_type)-1] = '\0';
	}
      break;

    case RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_compose", &composevals);
      break;

    default:
      break;
    }

  if (status == STATUS_SUCCESS)
    {
      if (run_mode != RUN_NONINTERACTIVE)
        gimp_progress_init ("Composing...");

      image_ID = compose (composevals.compose_type, composevals.compose_ID);

      if (image_ID <= 0)
      {
        status = STATUS_EXECUTION_ERROR;
      }
      else
      {
        values[1].data.d_int32 = image_ID;
        gimp_image_enable_undo (image_ID);
        gimp_image_clean_all (image_ID);
        if (run_mode != RUN_NONINTERACTIVE)
          gimp_display_new (image_ID);
      }

      /*  Store data  */
      if (run_mode == RUN_INTERACTIVE)
        gimp_set_data ("plug_in_compose", &composevals, sizeof (ComposeVals));
    }

  values[0].data.d_status = status;
}


/* Compose an image from several gray-images */
static gint32
compose (char *compose_type,
         gint32 *compose_ID)

{int width, height, tile_height, scan_lines;
 int num_images, compose_idx;
 int i, j;
 gint num_layers;
 gint32 *g32, layer_ID_src[MAX_COMPOSE_IMAGES], layer_ID_dst, image_ID_dst;
 unsigned char *src[MAX_COMPOSE_IMAGES], *dst = (unsigned char *)ident;
 GDrawableType gdtype_dst;
 GDrawable *drawable_src[MAX_COMPOSE_IMAGES], *drawable_dst;
 GPixelRgn pixel_rgn_src[MAX_COMPOSE_IMAGES], pixel_rgn_dst;

  /* Search type of composing */
  compose_idx = -1;
  for (j = 0; j < MAX_COMPOSE_TYPES; j++)
  {
    if (cmp_icase (compose_type, compose_dsc[j].compose_type) == 0)
      compose_idx = j;
  }
  if (compose_idx < 0)
    return (-1);

  num_images = compose_dsc[compose_idx].num_images;

  /* Check image sizes */
  width = gimp_image_width (compose_ID[0]);
  height = gimp_image_height (compose_ID[0]);
  tile_height = gimp_tile_height ();

  for (j = 1; j < num_images; j++)
  {
    if (   (width != (int)gimp_image_width (compose_ID[j]))
        || (height != (int)gimp_image_height (compose_ID[j])))
    {
      printf ("compose: images have different size\n");
      return -1;
    }
  }

  /* Get first layer/drawable/pixel region for all input images */
  for (j = 0; j < num_images; j++)
  {
    /* Get first layer of image */
    g32 = gimp_image_get_layers (compose_ID[j], &num_layers);
    if ((g32 == NULL) || (num_layers <= 0))
    {
      printf ("compose: error in getting layer IDs\n");
      return (-1);
    }
    layer_ID_src[j] = g32[0];

    /* Get drawable for layer */
    drawable_src[j] = gimp_drawable_get (layer_ID_src[j]);
    if (drawable_src[j]->bpp != 1)
    {
      printf ("compose: image is not a gray image\n");
      return (-1);
    }

    /* Get pixel region */
    gimp_pixel_rgn_init (&(pixel_rgn_src[j]), drawable_src[j], 0, 0,
                         width, height, FALSE, FALSE);

    /* Get memory for retrieving information */
    src[j] = (unsigned char *)g_malloc (tile_height * width
                                        * drawable_src[j]->bpp);
    if (src[j] == NULL)
    {
      printf ("compose: not enough memory\n");
      return (-1);
    }
  }

  /* Create new image */
  gdtype_dst = (compose_dsc[compose_idx].compose_fun == compose_rgba)
              ? RGBA_IMAGE : RGB_IMAGE;
  image_ID_dst = create_new_image (compose_dsc[compose_idx].filename,
                      width, height, gdtype_dst,
                      &layer_ID_dst, &drawable_dst, &pixel_rgn_dst);
  dst = (unsigned char *)g_malloc (tile_height * width * drawable_dst->bpp);
  if (dst == NULL)
  {
    for (j = 0; j < num_images; j++) g_free (src[j]);
    printf ("compose: not enough memory\n");
    return (-1);
  }

  /* Do the composition */
  i = 0;
  while (i < height)
  {
    scan_lines = (i+tile_height-1 < height) ? tile_height : (height-i);

    /* Get source pixel regions */
    for (j = 0; j < num_images; j++)
      gimp_pixel_rgn_get_rect (&(pixel_rgn_src[j]), src[j], 0, i,
                               width, scan_lines);

    /* Do the composition */
    compose_dsc[compose_idx].compose_fun (src, width*tile_height, dst);

    /* Set destination pixel region */
    gimp_pixel_rgn_set_rect (&pixel_rgn_dst, dst, 0, i, width, scan_lines);

    i += scan_lines;

    if (run_mode != RUN_NONINTERACTIVE)
      gimp_progress_update (((double)i) / (double)height);
  }

  for (j = 0; j < num_images; j++)
  {
    g_free (src[j]);
    gimp_drawable_flush (drawable_src[j]);
    gimp_drawable_detach (drawable_src[j]);
  }
  g_free (dst);
  gimp_drawable_flush (drawable_dst);
  gimp_drawable_detach (drawable_dst);

  return image_ID_dst;
}


/* Create an image. Sets layer_ID, drawable and rgn. Returns image_ID */
static gint32
create_new_image (char *filename,
                  guint width,
                  guint height,
                  GDrawableType gdtype,
                  gint32 *layer_ID,
                  GDrawable **drawable,
                  GPixelRgn *pixel_rgn)

{gint32 image_ID;
 GImageType gitype;

 if ((gdtype == GRAY_IMAGE) || (gdtype == GRAYA_IMAGE))
   gitype = GRAY;
 else if ((gdtype == INDEXED_IMAGE) || (gdtype == INDEXEDA_IMAGE))
   gitype = INDEXED;
 else
   gitype = RGB;

 image_ID = gimp_image_new (width, height, gitype);
 gimp_image_set_filename (image_ID, filename);

 *layer_ID = gimp_layer_new (image_ID, "Background", width, height,
                             gdtype, 100, NORMAL_MODE);
 gimp_image_add_layer (image_ID, *layer_ID, 0);

 *drawable = gimp_drawable_get (*layer_ID);
 gimp_pixel_rgn_init (pixel_rgn, *drawable, 0, 0, (*drawable)->width,
                      (*drawable)->height, TRUE, FALSE);

 return (image_ID);
}


/* Compare two strings ignoring case (could also be done by strcasecmp() */
/* but is it available everywhere ?) */
static int cmp_icase (char *s1, char *s2)

{int c1, c2;

  c1 = toupper (*s1);  c2 = toupper (*s2);
  while (*s1 && *s2)
  {
    if (c1 != c2) return (c2 - c1);
    c1 = toupper (*(++s1));  c2 = toupper (*(++s2));
  }
  return (c2 - c1);
}


static void
compose_rgb (unsigned char **src,
             int numpix,
             unsigned char *dst)

{register unsigned char *red_src = src[0];
 register unsigned char *green_src = src[1];
 register unsigned char *blue_src = src[2];
 register unsigned char *rgb_dst = dst;
 register int count = numpix;

 while (count-- > 0)
 {
   *(rgb_dst++) = *(red_src++);
   *(rgb_dst++) = *(green_src++);
   *(rgb_dst++) = *(blue_src++);
 }
}


static void
compose_rgba (unsigned char **src,
              int numpix,
              unsigned char *dst)

{register unsigned char *red_src = src[0];
 register unsigned char *green_src = src[1];
 register unsigned char *blue_src = src[2];
 register unsigned char *alpha_src = src[3];
 register unsigned char *rgb_dst = dst;
 register int count = numpix;

 while (count-- > 0)
 {
   *(rgb_dst++) = *(red_src++);
   *(rgb_dst++) = *(green_src++);
   *(rgb_dst++) = *(blue_src++);
   *(rgb_dst++) = *(alpha_src++);
 }
}


static void
compose_hsv (unsigned char **src,
             int numpix,
             unsigned char *dst)

{register unsigned char *hue_src = src[0];
 register unsigned char *sat_src = src[1];
 register unsigned char *val_src = src[2];
 register unsigned char *rgb_dst = dst;
 register int count = numpix;

 while (count-- > 0)
 {
   hsv_to_rgb (hue_src++, sat_src++, val_src++, rgb_dst);
   rgb_dst += 3;
 }
}


static void
compose_cmy (unsigned char **src,
             int numpix,
             unsigned char *dst)

{register unsigned char *cyan_src = src[0];
 register unsigned char *magenta_src = src[1];
 register unsigned char *yellow_src = src[2];
 register unsigned char *rgb_dst = dst;
 register int count = numpix;

 while (count-- > 0)
 {
   *(rgb_dst++) = 255 - *(cyan_src++);
   *(rgb_dst++) = 255 - *(magenta_src++);
   *(rgb_dst++) = 255 - *(yellow_src++);
 }
}


static void
compose_cmyk (unsigned char **src,
              int numpix,
              unsigned char *dst)

{register unsigned char *cyan_src = src[0];
 register unsigned char *magenta_src = src[1];
 register unsigned char *yellow_src = src[2];
 register unsigned char *black_src = src[3];
 register unsigned char *rgb_dst = dst;
 register int count = numpix;
 int cyan, magenta, yellow, black;

 while (count-- > 0)
 {
   black = (int)*(black_src++);
   if (black)
   {
     cyan = (int)*(cyan_src++);
     magenta = (int)*(magenta_src++);
     yellow = (int)*(yellow_src++);
     cyan += black; if (cyan > 255) cyan = 255;
     magenta += black; if (magenta > 255) magenta = 255;
     yellow += black; if (yellow > 255) yellow = 255;
     *(rgb_dst++) = 255 - cyan;
     *(rgb_dst++) = 255 - magenta;
     *(rgb_dst++) = 255 - yellow;
   }
   else
   {
     *(rgb_dst++) = 255 - *(cyan_src++);
     *(rgb_dst++) = 255 - *(magenta_src++);
     *(rgb_dst++) = 255 - *(yellow_src++);
   }
 }
}


static gint
compose_dialog (char *compose_type,
                gint32 image_ID)
{
  GtkWidget *dlg;
  GtkWidget *button;
  GtkWidget *toggle;
  GtkWidget *left_frame, *right_frame;
  GtkWidget *left_vbox, *right_vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *table;
  GtkWidget *image_option_menu, *image_menu;
  GSList *group;
  gchar **argv;
  gint argc;
  int j, compose_idx;

  /* Check default compose type */
  compose_idx = -1;
  for (j = 0; j < MAX_COMPOSE_TYPES; j++)
  {
    if (cmp_icase (compose_type, compose_dsc[j].compose_type) == 0)
      compose_idx = j;
  }
  if (compose_idx < 0) compose_idx = 0;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("Compose");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "Compose");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) compose_close_callback,
		      NULL);

  /*  Action area  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) compose_ok_callback, dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button,
                      TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button,
                      TRUE, TRUE, 0);
  gtk_widget_show (button);

  /*  parameter settings  */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (hbox), 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  /* The left frame keeps the compose type toggles */
  left_frame = gtk_frame_new ("Compose channels:");
  gtk_frame_set_shadow_type (GTK_FRAME (left_frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (left_frame), 10);
  gtk_box_pack_start (GTK_BOX (hbox), left_frame, TRUE, TRUE, 0);

  left_vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (left_vbox), 10);
  gtk_container_add (GTK_CONTAINER (left_frame), left_vbox);

  /* The right frame keeps the selection menues for images. */
  /* Because the labels within this frame will change when a toggle */
  /* in the left frame is changed, fill in the right part first. */
  /* Otherwise it can occur, that a non-existing label might be changed. */

  right_frame = gtk_frame_new ("Channel representations:");
  gtk_frame_set_shadow_type (GTK_FRAME (right_frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (right_frame), 10);
  gtk_box_pack_start (GTK_BOX (hbox), right_frame, TRUE, TRUE, 0);

  right_vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (right_vbox), 10);
  gtk_container_add (GTK_CONTAINER (right_frame), right_vbox);

  table = gtk_table_new (MAX_COMPOSE_IMAGES, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table), 5);
  gtk_box_pack_start (GTK_BOX (right_vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  /* Channel names */
  for (j = 0; j < MAX_COMPOSE_IMAGES; j++)
  {
    composeint.channel_label[j] = label =
         gtk_label_new (compose_dsc[compose_idx].channel_name[j]);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 1, 2, j, j+1,
                      GTK_FILL, GTK_FILL, 0, 0);
    gtk_widget_show (label);
  }

  /* Menues to select images */
  for (j = 0; j <  MAX_COMPOSE_IMAGES; j++)
  {
    composeint.select_ID[j] = image_ID;
    image_option_menu = gtk_option_menu_new ();
    image_menu = gimp_image_menu_new (check_gray, image_menu_callback,
                        &(composeint.select_ID[j]), composeint.select_ID[j]);
    gtk_table_attach (GTK_TABLE (table), image_option_menu, 2, 3, j, j+1,
                      GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

    gtk_widget_show (image_option_menu);
    gtk_option_menu_set_menu (GTK_OPTION_MENU (image_option_menu), image_menu);
  }

  /* Compose types */
  group = NULL;
  for (j = 0; j < MAX_COMPOSE_TYPES; j++)
  {
    toggle = gtk_radio_button_new_with_label (group,
                                              compose_dsc[j].compose_type);
    group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (left_vbox), toggle, TRUE, TRUE, 0);
    composeint.compose_flag[j] = (j == compose_idx);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                        (GtkSignalFunc) compose_type_toggle_update,
                        &(composeint.compose_flag[j]));
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle),
                                 composeint.compose_flag[j]);
    gtk_widget_show (toggle);
  }

  gtk_widget_show (left_vbox);
  gtk_widget_show (right_vbox);
  gtk_widget_show (left_frame);
  gtk_widget_show (right_frame);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return composeint.run;
}


/* hsv_to_rgb has been taken from the compose-plug-in of GIMP V 0.54
 * and hass been modified a little bit
 */
static void
hsv_to_rgb (unsigned char *h,
            unsigned char *s,
            unsigned char *v,
            unsigned char *rgb)

{double hue, sat, val;
 double f, p, q, t;
 int red, green, blue;

  if (*s == 0)
  {
    rgb[0] = rgb[1] = rgb[2] = *v;
  }
  else
  {
    hue = *h * 6.0 / 255.0;
    sat = *s / 255.0;
    val = *v / 255.0;

    f = hue - (int) hue;
    p = val * (1.0 - sat);
    q = val * (1.0 - (sat * f));
    t = val * (1.0 - (sat * (1.0 - f)));

    switch ((int) hue)
      {
      case 0:
        red = (int)(val * 255.0);
        green = (int)(t * 255.0);
        blue = (int)(p * 255.0);
        break;
      case 1:
        red = (int)(q * 255.0);
        green = (int)(val * 255.0);
        blue = (int)(p * 255.0);
        break;
      case 2:
        red = (int)(p * 255.0);
        green = (int)(val * 255.0);
        blue = (int)(t * 255.0);
        break;
      case 3:
        red = (int)(p * 255.0);
        green = (int)(q * 255.0);
        blue = (int)(val * 255.0);
        break;
      case 4:
        red = (int)(t * 255.0);
        green = (int)(p * 255.0);
        blue = (int)(val * 255.0);
        break;
      case 5:
        red = (int)(val * 255.0);
        green = (int)(p * 255.0);
        blue = (int)(q * 255.0);
        break;
      default:
        red = 0;
        green = 0;
        blue = 0;
        break;
      }
    if (red < 0) red = 0; else if (red > 255) red = 255;
    if (green < 0) green = 0; else if (green > 255) green = 255;
    if (blue < 0) blue = 0; else if (blue > 255) blue = 255;
    rgb[0] = (unsigned char)red;
    rgb[1] = (unsigned char)green;
    rgb[2] = (unsigned char)blue;
  }
}

/*  Compose interface functions  */

static gint
check_gray (gint32 image_id,
            gint32 drawable_id,
            gpointer data)

{
  return (gimp_image_base_type (image_id) == GRAY);
}


static void
image_menu_callback (gint32   id,
                     gpointer data)
{
 *(gint32 *)data = id;
}


static void
compose_close_callback (GtkWidget *widget,
                        gpointer   data)
{
  gtk_main_quit ();
}


static void
compose_ok_callback (GtkWidget *widget,
                     gpointer   data)
{int j;

  composeint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));

  for (j = 0; j < MAX_COMPOSE_IMAGES; j++)
    composevals.compose_ID[j] = composeint.select_ID[j];

  for (j = 0; j < MAX_COMPOSE_TYPES; j++)
  {
    if (composeint.compose_flag[j])
    {
      strcpy (composevals.compose_type, compose_dsc[j].compose_type);
      break;
    }
  }
}


static void
compose_type_toggle_update (GtkWidget *widget,
                            gpointer   data)
{
  gint *toggle_val;
  gint compose_idx, j;

  toggle_val = (gint *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
  {
    *toggle_val = TRUE;
    compose_idx = toggle_val - &(composeint.compose_flag[0]);
    for (j = 0; j < MAX_COMPOSE_IMAGES; j++)
      gtk_label_set_text (GTK_LABEL (composeint.channel_label[j]),
                          compose_dsc[compose_idx].channel_name[j]);
  }
  else
    *toggle_val = FALSE;
}
