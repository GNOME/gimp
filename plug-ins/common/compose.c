/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Compose plug-in (C) 1997,1999 Peter Kirchgessner
 * e-mail: peter@kirchgessner.net, WWW: http://www.kirchgessner.net
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

/*  Lab colorspace support originally written by Alexey Dyachenko,
 *  merged into the official plug-in by Sven Neumann.
 *
 *  Support for channels empty or filled with a single mask value
 *  added by Sylvain FORET.
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define COMPOSE_PROC          "plug-in-compose"
#define DRAWABLE_COMPOSE_PROC "plug-in-drawable-compose"
#define RECOMPOSE_PROC        "plug-in-recompose"
#define PLUG_IN_BINARY        "compose"


typedef struct
{
  union
    {
      gint32 ID;  /* Image ID of input images or drawable */
      guchar val; /* Mask value to compose with */
    } comp;
  gboolean is_ID;
} ComposeInput;


/* Declare local functions
 */
static void      query              (void);
static void      run                (const gchar      *name,
                                     gint              nparams,
                                     const GimpParam  *param,
                                     gint             *nreturn_vals,
                                     GimpParam       **return_vals);

static gint32    compose            (const gchar      *compose_type,
                                     ComposeInput     *inputs,
                                     gboolean          compose_by_drawable);

static gint32    create_new_image   (const gchar      *filename,
                                     guint             width,
                                     guint             height,
                                     GimpImageType     gdtype,
                                     gint32           *layer_ID,
                                     GimpDrawable    **drawable,
                                     GimpPixelRgn     *pixel_rgn);

static void      compose_rgb         (guchar         **src,
                                      gint            *incr,
                                      gint             numpix,
                                      guchar          *dst,
                                      gboolean         dst_has_alpha);
static void      compose_rgba        (guchar         **src,
                                      gint            *incr,
                                      gint             numpix,
                                      guchar          *dst,
                                      gboolean         dst_has_alpha);
static void      compose_hsv         (guchar         **src,
                                      gint            *incr,
                                      gint             numpix,
                                      guchar          *dst,
                                      gboolean         dst_has_alpha);
static void      compose_hsl         (guchar         **src,
                                      gint            *incr,
                                      gint             numpix,
                                      guchar          *dst,
                                      gboolean         dst_has_alpha);
static void      compose_cmy         (guchar         **src,
                                      gint            *incr,
                                      gint             numpix,
                                      guchar          *dst,
                                      gboolean         dst_has_alpha);
static void      compose_cmyk        (guchar         **src,
                                      gint            *incr,
                                      gint             numpix,
                                      guchar          *dst,
                                      gboolean         dst_has_alpha);
static void      compose_lab         (guchar         **src,
                                      gint            *incr,
                                      gint             numpix,
                                      guchar          *dst,
                                      gboolean         dst_has_alpha);
static void      compose_ycbcr470    (guchar         **src,
                                      gint            *incr,
                                      gint             numpix,
                                      guchar          *dst,
                                      gboolean         dst_has_alpha);
static void      compose_ycbcr709    (guchar         **src,
                                      gint            *incr,
                                      gint             numpix,
                                      guchar          *dst,
                                      gboolean         dst_has_alpha);
static void      compose_ycbcr470f   (guchar         **src,
                                      gint            *incr,
                                      gint             numpix,
                                      guchar          *dst,
                                      gboolean         dst_has_alpha);
static void      compose_ycbcr709f   (guchar         **src,
                                      gint            *incr,
                                      gint             numpix,
                                      guchar          *dst,
                                      gboolean         dst_has_alpha);

static gboolean  compose_dialog      (const gchar     *compose_type,
                                      gint32           drawable_ID);

static gboolean  check_gray          (gint32           image_id,
                                      gint32           drawable_id,
                                      gpointer         data);

static void      combo_callback      (GimpIntComboBox *cbox,
                                      gpointer         data);

static void      scale_callback      (GtkAdjustment   *adj,
                                      ComposeInput    *input);

static void      check_response      (GtkWidget       *dialog,
                                      gint             response,
                                      gpointer         data);

static void      type_combo_callback (GimpIntComboBox *combo,
                                      gpointer         data);


/* LAB colorspace constants */
const double Xn	= 0.951;
const double Yn	= 1.0;
const double Zn	= 1.089;

/* Maximum number of images to compose */
#define MAX_COMPOSE_IMAGES 4


/* Description of a composition */
typedef struct
{
  const gchar  *compose_type;  /*  Type of composition ("RGB", "RGBA",...)  */
  gint          num_images;    /*  Number of input images needed            */
                               /*  Channel names and stock ids for dialog   */
  const gchar  *channel_name[MAX_COMPOSE_IMAGES];
  const gchar  *channel_icon[MAX_COMPOSE_IMAGES];
  const gchar  *filename;      /*  Name of new image                        */

  /*  Compose functon  */
  void  (* compose_fun) (guchar **src,
			 gint    *incr_src,
			 gint     numpix,
			 guchar  *dst,
                         gboolean dst_has_alpha);
} COMPOSE_DSC;

/* Array of available compositions. */

static COMPOSE_DSC compose_dsc[] =
{
  { N_("RGB"), 3,
    { N_("_Red:"),
      N_("_Green:"),
      N_("_Blue:"),
      NULL },
    { GIMP_STOCK_CHANNEL_RED,
      GIMP_STOCK_CHANNEL_GREEN,
      GIMP_STOCK_CHANNEL_BLUE,
      NULL },
    "rgb-compose",  compose_rgb },

  { N_("RGBA"), 4,
    { N_("_Red:"),
      N_("_Green:"),
      N_("_Blue:"),
      N_("_Alpha:") },
    { GIMP_STOCK_CHANNEL_RED,
      GIMP_STOCK_CHANNEL_GREEN,
      GIMP_STOCK_CHANNEL_BLUE,
      GIMP_STOCK_CHANNEL_ALPHA },
    "rgba-compose",  compose_rgba },

  { N_("HSV"), 3,
    { N_("_Hue:"),
      N_("_Saturation:"),
      N_("_Value:"),
      NULL },
    { NULL, NULL, NULL, NULL },
    "hsv-compose",  compose_hsv },

  { N_("HSL"), 3,
    { N_("_Hue:"),
      N_("_Saturation:"),
      N_("_Lightness:"),
      NULL },
    { NULL, NULL, NULL, NULL },
    "hsl-compose",  compose_hsl },

  { N_("CMY"), 3,
    { N_("_Cyan:"),
      N_("_Magenta:"),
      N_("_Yellow:"),
      NULL },
    { NULL, NULL, NULL, NULL },
    "cmy-compose",  compose_cmy },

  { N_("CMYK"), 4,
    { N_("_Cyan:"),
      N_("_Magenta:"),
      N_("_Yellow:"),
      N_("_Black:") },
    { NULL, NULL, NULL, NULL },
    "cmyk-compose", compose_cmyk },

  { N_("LAB"), 3,
    { "_L:",
      "_A:",
      "_B:",
      NULL },
    { NULL, NULL, NULL, NULL },
    "lab-compose", compose_lab },

  { "YCbCr_ITU_R470", 3,
    { N_("_Luma y470:"),
      N_("_Blueness cb470:"),
      N_("_Redness cr470:"),
      NULL },
    { NULL, NULL, NULL, NULL },
    "ycbcr470-compose",  compose_ycbcr470 },

  { "YCbCr_ITU_R709", 3,
    { N_("_Luma y709:"),
      N_("_Blueness cb709:"),
      N_("_Redness cr709:"),
      NULL },
    { NULL, NULL, NULL, NULL },
    "ycbcr709-compose", compose_ycbcr709 },

  { "YCbCr_ITU_R470_256", 3,
    { N_("_Luma y470f:"),
      N_("_Blueness cb470f:"),
      N_("_Redness cr470f:"),
      NULL },
    { NULL, NULL, NULL, NULL },
    "ycbcr470F-compose",  compose_ycbcr470f },

  { "YCbCr_ITU_R709_256", 3,
    { N_("_Luma y709f:"),
      N_("_Blueness cb709f:"),
      N_("_Redness cr709f:"),
      NULL },
    { NULL, NULL, NULL, NULL },
    "ycbcr709F-compose",  compose_ycbcr709f },
};


typedef struct
{
  ComposeInput inputs[MAX_COMPOSE_IMAGES];  /* Image IDs or mask value of input */
  gchar        compose_type[32];            /* type of composition */
  gboolean     do_recompose;
  gint32       source_layer_ID;             /* for recomposing */
} ComposeVals;

/* Dialog structure */
typedef struct
{
  gint          width, height;                            /* Size of selected image */

  GtkWidget    *channel_label[MAX_COMPOSE_IMAGES];        /* The labels to change */
  GtkWidget    *channel_icon[MAX_COMPOSE_IMAGES];         /* The icons  */
  GtkWidget    *channel_menu[MAX_COMPOSE_IMAGES];         /* The menus */
  GtkWidget    *color_scales[MAX_COMPOSE_IMAGES];         /* The values color scales */
  GtkWidget    *color_spins[MAX_COMPOSE_IMAGES];          /* The values spin buttons */

  ComposeInput  selected[MAX_COMPOSE_IMAGES];             /* Image Ids or mask values from menus */

  gint          compose_idx;                              /* Compose type */
} ComposeInterface;

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static ComposeVals composevals =
{
  {{{ 0 }}}, /* Image IDs of images to compose or mask values */
  "rgb",     /* Type of composition */
  FALSE,     /* Do recompose */
  -1         /* source layer ID */
};

static ComposeInterface composeint =
{
  0, 0,      /* width, height */
  { NULL },  /* Label Widgets */
  { NULL },  /* Icon Widgets */
  { NULL },  /* Menu Widgets */
  { NULL },  /* Color Scale Widgets */
  { NULL },  /* Color Spin Widgets */
  {{{ 0 }}}, /* Image Ids or mask values from menus */
  0          /* Compose type */
};

static GimpRunMode run_mode;


MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image1",       "First input image" },
    { GIMP_PDB_DRAWABLE, "drawable",     "Input drawable (not used)" },
    { GIMP_PDB_IMAGE,    "image2",       "Second input image" },
    { GIMP_PDB_IMAGE,    "image3",       "Third input image" },
    { GIMP_PDB_IMAGE,    "image4",       "Fourth input image" },
    { GIMP_PDB_STRING,   "compose-type", NULL }
  };

  static const GimpParamDef return_vals[] =
  {
    { GIMP_PDB_IMAGE, "new_image", "Output image" }
  };

  static GimpParamDef drw_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image1",       "First input image (not used)" },
    { GIMP_PDB_DRAWABLE, "drawable1",    "First input drawable" },
    { GIMP_PDB_DRAWABLE, "drawable2",    "Second input drawable" },
    { GIMP_PDB_DRAWABLE, "drawable3",    "Third input drawable" },
    { GIMP_PDB_DRAWABLE, "drawable4",    "Fourth input drawable" },
    { GIMP_PDB_STRING,   "compose-type", NULL }
  };

  static const GimpParamDef drw_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "new_image", "Output image" }
  };

  static const GimpParamDef recompose_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",    "Image to recompose from" },
    { GIMP_PDB_DRAWABLE, "drawable", "Not used" },
  };

  GString *type_desc;
  int i;

  type_desc = g_string_new ("What to compose: ");
  g_string_append_c (type_desc, '"');
  g_string_append (type_desc, compose_dsc[0].compose_type);
  g_string_append_c (type_desc, '"');

  for (i = 1; i < G_N_ELEMENTS (compose_dsc); i++)
    {
      g_string_append (type_desc, ", ");
      g_string_append_c (type_desc, '"');
      g_string_append (type_desc, compose_dsc[i].compose_type);
      g_string_append_c (type_desc, '"');
    }

  args[6].description = type_desc->str;
  drw_args[6].description = type_desc->str;

  gimp_install_procedure (COMPOSE_PROC,
			  N_("Create an image using multiple gray images as color channels"),
			  "This function creates a new image from "
			  "multiple gray images",
			  "Peter Kirchgessner",
			  "Peter Kirchgessner (peter@kirchgessner.net)",
			  "1997",
			  N_("C_ompose..."),
			  "GRAY*",
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (args),
                          G_N_ELEMENTS (return_vals),
			  args, return_vals);

  gimp_plugin_menu_register (COMPOSE_PROC, "<Image>/Colors/Components");

  gimp_install_procedure (DRAWABLE_COMPOSE_PROC,
			  "Compose an image from multiple drawables of gray images",
			  "This function creates a new image from "
			  "multiple drawables of gray images",
			  "Peter Kirchgessner",
			  "Peter Kirchgessner (peter@kirchgessner.net)",
			  "1998",
			  NULL,   /* It is not available in interactive mode */
			  "GRAY*",
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (drw_args),
                          G_N_ELEMENTS (drw_return_vals),
			  drw_args, drw_return_vals);

  gimp_install_procedure (RECOMPOSE_PROC,
			  N_("Recompose an image that was previously decomposed"),
			  "This function recombines the grayscale layers produced "
			  "by Decompose into a single RGB or RGBA layer, and "
                          "replaces the originally decomposed layer with the "
                          "result.",
			  "Bill Skaggs",
			  "Bill Skaggs",
			  "2004",
			  N_("R_ecompose"),
			  "GRAY*",
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (recompose_args), 0,
			  recompose_args, NULL);

  gimp_plugin_menu_register (RECOMPOSE_PROC, "<Image>/Colors/Components");

  g_string_free (type_desc, TRUE);
}


static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[2];
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  gint32            image_ID;
  gint32            drawable_ID = -1;
  gint              compose_by_drawable;
  gint              i;

  INIT_I18N ();

  run_mode = param[0].data.d_int32;
  compose_by_drawable = (strcmp (name, DRAWABLE_COMPOSE_PROC) == 0);

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
  values[1].type          = GIMP_PDB_IMAGE;
  values[1].data.d_int32  = -1;

  if (strcmp (name, RECOMPOSE_PROC) == 0)
    {
      GimpParasite *parasite = gimp_image_parasite_find (param[1].data.d_image,
                                                         "decompose-data");

      if (! parasite)
        {
          g_message (_("You can only run 'Recompose' if the active image "
                       "was originally produced by 'Decompose'."));
          status = GIMP_PDB_EXECUTION_ERROR;
        }
      else
        {
          gint nret;

          nret = sscanf (gimp_parasite_data (parasite),
                         "source=%d type=%31s %d %d %d %d",
                         &composevals.source_layer_ID,
                         composevals.compose_type,
                         &composevals.inputs[0].comp.ID,
                         &composevals.inputs[1].comp.ID,
                         &composevals.inputs[2].comp.ID,
                         &composevals.inputs[3].comp.ID);

          gimp_parasite_free (parasite);

          for (i = 0; i < MAX_COMPOSE_IMAGES; i++)
            composevals.inputs[i].is_ID = TRUE;

          if (nret < 5)
            {
              g_message (_("Error scanning 'decompose-data' parasite: "
                           "too few layers found"));
              status = GIMP_PDB_EXECUTION_ERROR;
            }
          else
            {
              composevals.do_recompose = TRUE;
              compose_by_drawable = TRUE;
            }
        }
    }
  else
    {
      composevals.do_recompose = FALSE;

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          /*  Possibly retrieve data  */
          gimp_get_data (name, &composevals);

          compose_by_drawable = TRUE;

          /* The dialog is now drawable based. Get a drawable-ID of the image */
          if (strcmp (name, COMPOSE_PROC) == 0)
            {
              gint32 *layer_list;
              gint    nlayers;

              layer_list = gimp_image_get_layers (param[1].data.d_int32,
                                                  &nlayers);
              if ((layer_list == NULL) || (nlayers <= 0))
                {
                  g_message (_("Could not get layers for image %d"),
                             (gint) param[1].data.d_int32);
                  return;
                }

              drawable_ID = layer_list[0];
              g_free (layer_list);
            }
          else
            {
              drawable_ID = param[2].data.d_int32;
            }

          /*  First acquire information with a dialog  */
          if (! compose_dialog (composevals.compose_type, drawable_ID))
            return;

          break;

        case GIMP_RUN_NONINTERACTIVE:
          /*  Make sure all the arguments are there!  */
          if (nparams < 7)
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
          else
            {
              composevals.inputs[0].comp.ID = (compose_by_drawable ?
                                               param[2].data.d_int32 :
                                               param[1].data.d_int32);
              composevals.inputs[1].comp.ID = param[3].data.d_int32;
              composevals.inputs[2].comp.ID = param[4].data.d_int32;
              composevals.inputs[3].comp.ID = param[5].data.d_int32;

              strncpy (composevals.compose_type, param[6].data.d_string,
                       sizeof (composevals.compose_type));
              composevals.compose_type[sizeof (composevals.compose_type)-1] = '\0';

              for (i = 0; i < MAX_COMPOSE_IMAGES; i++)
                {
                  if (composevals.inputs[i].comp.ID == -1)
                    {
                      composevals.inputs[i].is_ID = FALSE;
                      composevals.inputs[i].comp.val = 0;
                    }
                  else
                    {
                      composevals.inputs[i].is_ID = TRUE;
                    }
                }
            }
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          /*  Possibly retrieve data  */
          gimp_get_data (name, &composevals);

          compose_by_drawable = TRUE;
          break;

        default:
          break;
        }
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      gimp_progress_init (_("Composing"));

      image_ID = compose (composevals.compose_type,
                          composevals.inputs,
                          compose_by_drawable);

      if (image_ID < 0)
	{
	  status = GIMP_PDB_EXECUTION_ERROR;
	}
      else
	{
	  values[1].data.d_int32 = image_ID;

          if (composevals.do_recompose)
            {
              gimp_displays_flush ();
            }
          else
            {
              gimp_image_undo_enable (image_ID);
              gimp_image_clean_all (image_ID);

              if (run_mode != GIMP_RUN_NONINTERACTIVE)
                gimp_display_new (image_ID);
            }
	}

      /*  Store data  */
      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data (name, &composevals, sizeof (ComposeVals));
    }

  *nreturn_vals = composevals.do_recompose ? 1 : 2;

  values[0].data.d_status = status;
}


/* Compose an image from several gray-images */
static gint32
compose (const gchar  *compose_type,
         ComposeInput *inputs,
         gboolean      compose_by_drawable)
{
  gint           width, height, tile_height, scan_lines;
  gint           num_images, compose_idx, incr_src[MAX_COMPOSE_IMAGES];
  gint           i, j;
  gint           num_layers;
  gint32         layer_ID_dst, image_ID_dst;
  guchar        *src[MAX_COMPOSE_IMAGES];
  guchar        *dst;
  gint           first_ID;
  GimpImageType  gdtype_dst;
  GimpDrawable  *drawable_src[MAX_COMPOSE_IMAGES], *drawable_dst;
  GimpPixelRgn   pixel_rgn_src[MAX_COMPOSE_IMAGES], pixel_rgn_dst;
  GimpPixelRgn   pixel_rgn_dst_read;

  /* Search type of composing */
  compose_idx = -1;
  for (j = 0; j < G_N_ELEMENTS (compose_dsc); j++)
    {
      if (g_ascii_strcasecmp (compose_type, compose_dsc[j].compose_type) == 0)
        {
          compose_idx = j;
          break;
        }
    }
  if (compose_idx < 0)
    return -1;

  num_images = compose_dsc[compose_idx].num_images;

  /* Check that at least one image or one drawable is provided */
  first_ID = -1;
  for (i = 0; i < num_images; i++)
    {
      if (inputs[i].is_ID)
        {
          first_ID = i;
          break;
        }
    }
  if (-1 == first_ID)
    {
      g_message(_("At least one image is needed to compose"));
      return -1;
    }

  tile_height = gimp_tile_height ();

  /* Check image sizes */
  if (compose_by_drawable)
    {
      if (0 == gimp_drawable_bpp (inputs[first_ID].comp.ID))
        {
          g_message (_("Specified layer %d not found"), inputs[first_ID].comp.ID);
          return -1;
        }

      width = gimp_drawable_width (inputs[first_ID].comp.ID);
      height = gimp_drawable_height (inputs[first_ID].comp.ID);

      for (j = first_ID + 1; j < num_images; j++)
	{
          if (inputs[j].is_ID)
            {
              if (0 == gimp_drawable_bpp (inputs[j].comp.ID))
                {
                  g_message (_("Specified layer %d not found"), inputs[j].comp.ID);
                  return -1;
                }

              if ((width  != gimp_drawable_width  (inputs[j].comp.ID)) ||
                  (height != gimp_drawable_height (inputs[j].comp.ID)))
                {
                  g_message (_("Drawables have different size"));
                  return -1;
                }
            }
	}
      for (j = 0; j < num_images; j++)
        {
          if (inputs[j].is_ID)
            drawable_src[j] = gimp_drawable_get (inputs[j].comp.ID);
          else
            drawable_src[j] = NULL;
        }
    }
  else    /* Compose by image ID */
    {
      width  = gimp_image_width  (inputs[first_ID].comp.ID);
      height = gimp_image_height (inputs[first_ID].comp.ID);

      for (j = first_ID + 1; j < num_images; j++)
	{
          if (inputs[j].is_ID)
            {
              if ((width  != gimp_image_width (inputs[j].comp.ID)) ||
                  (height != gimp_image_height (inputs[j].comp.ID)))
                {
                  g_message (_("Images have different size"));
                  return -1;
                }
            }
	}

      /* Get first layer/drawable for all input images */
      for (j = 0; j < num_images; j++)
	{
          if (inputs[j].is_ID)
            {
              gint32 *g32;

              /* Get first layer of image */
              g32 = gimp_image_get_layers (inputs[j].comp.ID, &num_layers);
              if ((g32 == NULL) || (num_layers <= 0))
                {
                  g_message (_("Error in getting layer IDs"));
                  return -1;
                }

              /* Get drawable for layer */
              drawable_src[j] = gimp_drawable_get (g32[0]);
              g_free (g32);
            }
	}
    }

  /* Get pixel region for all input drawables */
  for (j = 0; j < num_images; j++)
    {
      gsize s;
      /* Check bytes per pixel */
      if (inputs[j].is_ID)
        {
          incr_src[j] = drawable_src[j]->bpp;
          if ((incr_src[j] != 1) && (incr_src[j] != 2))
            {
              g_message (_("Image is not a gray image (bpp=%d)"),
                         incr_src[j]);
              return -1;
            }

          /* Get pixel region */
          gimp_pixel_rgn_init (&(pixel_rgn_src[j]), drawable_src[j], 0, 0,
                               width, height, FALSE, FALSE);
        }
      else
        {
          incr_src[j] = 1;
        }
      /* Get memory for retrieving information */
      s = tile_height * width * incr_src[j];
      src[j] = g_new (guchar, s);
      if (! inputs[j].is_ID)
        memset (src[j], inputs[j].comp.val, s);
    }

  /* Unless recomposing, create new image */
  if (composevals.do_recompose)
    {
      layer_ID_dst = composevals.source_layer_ID;

      if (0 == gimp_drawable_bpp (layer_ID_dst))
        {
          g_message (_("Unable to recompose, source layer not found"));
          return -1;
        }

      drawable_dst = gimp_drawable_get (layer_ID_dst);
      gimp_pixel_rgn_init (&pixel_rgn_dst, drawable_dst, 0, 0,
                           drawable_dst->width,
                           drawable_dst->height,
                           TRUE, TRUE);
      gimp_pixel_rgn_init (&pixel_rgn_dst_read, drawable_dst, 0, 0,
                           drawable_dst->width,
                           drawable_dst->height,
                           FALSE, FALSE);
      image_ID_dst = gimp_drawable_get_image (layer_ID_dst);
    }
  else
    {
      gdtype_dst = ((compose_dsc[compose_idx].compose_fun == compose_rgba) ?
                    GIMP_RGBA_IMAGE : GIMP_RGB_IMAGE);

      image_ID_dst = create_new_image (compose_dsc[compose_idx].filename,
                                       width, height, gdtype_dst,
                                       &layer_ID_dst, &drawable_dst,
                                       &pixel_rgn_dst);
    }

  if (! compose_by_drawable)
    {
      gdouble  xres, yres;

      gimp_image_get_resolution (inputs[first_ID].comp.ID, &xres, &yres);
      gimp_image_set_resolution (image_ID_dst, xres, yres);
    }

  dst = g_new (guchar, tile_height * width * drawable_dst->bpp);

  /* Do the composition */
  i = 0;
  while (i < height)
    {
      scan_lines = (i+tile_height-1 < height) ? tile_height : (height-i);

      /* Get source pixel regions */
      for (j = 0; j < num_images; j++)
        if (inputs[j].is_ID)
          gimp_pixel_rgn_get_rect (&(pixel_rgn_src[j]), src[j], 0, i,
                                   width, scan_lines);

      if (composevals.do_recompose)
	gimp_pixel_rgn_get_rect (&pixel_rgn_dst_read, dst, 0, i,
				 width, scan_lines);

      /* Do the composition */
      compose_dsc[compose_idx].compose_fun (src,
					    incr_src,
					    width * tile_height,
					    dst,
                                            gimp_drawable_has_alpha (layer_ID_dst));

      /* Set destination pixel region */
      gimp_pixel_rgn_set_rect (&pixel_rgn_dst, dst, 0, i, width, scan_lines);

      i += scan_lines;

      gimp_progress_update ((gdouble) i / (gdouble) height);
    }

  for (j = 0; j < num_images; j++)
    {
      g_free (src[j]);
      if (inputs[j].is_ID)
        gimp_drawable_detach (drawable_src[j]);
    }
  g_free (dst);

  gimp_drawable_detach (drawable_dst);

  if (composevals.do_recompose)
    gimp_drawable_merge_shadow (layer_ID_dst, TRUE);

  gimp_drawable_update (layer_ID_dst, 0, 0,
                        gimp_drawable_width (layer_ID_dst),
                        gimp_drawable_height (layer_ID_dst));

  return image_ID_dst;
}


/* Create an image. Sets layer_ID, drawable and rgn. Returns image_ID */
static gint32
create_new_image (const gchar    *filename,
                  guint           width,
                  guint           height,
                  GimpImageType   gdtype,
                  gint32         *layer_ID,
                  GimpDrawable  **drawable,
                  GimpPixelRgn   *pixel_rgn)
{
  gint32            image_ID;
  GimpImageBaseType gitype;

  if ((gdtype == GIMP_GRAY_IMAGE) || (gdtype == GIMP_GRAYA_IMAGE))
    gitype = GIMP_GRAY;
  else if ((gdtype == GIMP_INDEXED_IMAGE) || (gdtype == GIMP_INDEXEDA_IMAGE))
    gitype = GIMP_INDEXED;
  else
    gitype = GIMP_RGB;

  image_ID = gimp_image_new (width, height, gitype);

  gimp_image_undo_disable (image_ID);
  gimp_image_set_filename (image_ID, filename);

  *layer_ID = gimp_layer_new (image_ID, _("Background"), width, height,
			      gdtype, 100, GIMP_NORMAL_MODE);
  gimp_image_add_layer (image_ID, *layer_ID, 0);

  *drawable = gimp_drawable_get (*layer_ID);
  gimp_pixel_rgn_init (pixel_rgn, *drawable, 0, 0, (*drawable)->width,
		       (*drawable)->height, TRUE, FALSE);

  return image_ID;
}

static void
compose_rgb (guchar **src,
             gint    *incr_src,
             gint     numpix,
             guchar  *dst,
             gboolean dst_has_alpha)
{
  register const guchar *red_src   = src[0];
  register const guchar *green_src = src[1];
  register const guchar *blue_src  = src[2];
  register       guchar *rgb_dst   = dst;
  register       gint    count     = numpix;
  gint red_incr   = incr_src[0];
  gint green_incr = incr_src[1];
  gint blue_incr  = incr_src[2];

  if ((red_incr == 1) && (green_incr == 1) && (blue_incr == 1))
    {
      while (count-- > 0)
	{
	  *(rgb_dst++) = *(red_src++);
	  *(rgb_dst++) = *(green_src++);
	  *(rgb_dst++) = *(blue_src++);
          if (dst_has_alpha)
            rgb_dst++;
	}
    }
  else
    {
      while (count-- > 0)
	{
	  *(rgb_dst++) = *red_src;     red_src += red_incr;
	  *(rgb_dst++) = *green_src;   green_src += green_incr;
	  *(rgb_dst++) = *blue_src;    blue_src += blue_incr;
          if (dst_has_alpha)
            rgb_dst++;
	}
    }
}


static void
compose_rgba (guchar **src,
              gint    *incr_src,
              gint     numpix,
              guchar  *dst,
              gboolean dst_has_alpha)
{
  register const guchar *red_src   = src[0];
  register const guchar *green_src = src[1];
  register const guchar *blue_src  = src[2];
  register const guchar *alpha_src = src[3];
  register       guchar *rgb_dst   = dst;
  register       gint    count     = numpix;
  gint red_incr   = incr_src[0];
  gint green_incr = incr_src[1];
  gint blue_incr  = incr_src[2];
  gint alpha_incr = incr_src[3];

  if ((red_incr == 1) && (green_incr == 1) && (blue_incr == 1) &&
      (alpha_incr == 1))
    {
      while (count-- > 0)
	{
	  *(rgb_dst++) = *(red_src++);
	  *(rgb_dst++) = *(green_src++);
	  *(rgb_dst++) = *(blue_src++);
	  *(rgb_dst++) = *(alpha_src++);
	}
    }
  else
    {
      while (count-- > 0)
	{
	  *(rgb_dst++) = *red_src;    red_src += red_incr;
	  *(rgb_dst++) = *green_src;  green_src += green_incr;
	  *(rgb_dst++) = *blue_src;   blue_src += blue_incr;
	  *(rgb_dst++) = *alpha_src;  alpha_src += alpha_incr;
	}
    }
}


static void
compose_hsv (guchar **src,
             gint    *incr_src,
             gint     numpix,
             guchar  *dst,
             gboolean dst_has_alpha)
{
  register const guchar *hue_src = src[0];
  register const guchar *sat_src = src[1];
  register const guchar *val_src = src[2];
  register       guchar *rgb_dst = dst;
  register       gint    count   = numpix;
  gint hue_incr = incr_src[0];
  gint sat_incr = incr_src[1];
  gint val_incr = incr_src[2];

  while (count-- > 0)
    {
      gimp_hsv_to_rgb4 (rgb_dst, (gdouble) *hue_src / 255.0,
			         (gdouble) *sat_src / 255.0,
			         (gdouble) *val_src / 255.0);
      rgb_dst += 3;
      hue_src += hue_incr;
      sat_src += sat_incr;
      val_src += val_incr;

      if (dst_has_alpha)
        rgb_dst++;
    }
}


static void
compose_hsl (guchar **src,
             gint    *incr_src,
             gint     numpix,
             guchar  *dst,
             gboolean dst_has_alpha)
{
  register const guchar *hue_src = src[0];
  register const guchar *sat_src = src[1];
  register const guchar *lum_src = src[2];
  register       guchar *rgb_dst = dst;
  register       gint    count   = numpix;
  gint    hue_incr = incr_src[0];
  gint    sat_incr = incr_src[1];
  gint    lum_incr = incr_src[2];
  GimpHSL hsl;
  GimpRGB rgb;

  while (count-- > 0)
    {
      hsl.h = (gdouble) *hue_src / 255.0;
      hsl.s = (gdouble) *sat_src / 255.0;
      hsl.l = (gdouble) *lum_src / 255.0;

      gimp_hsl_to_rgb (&hsl, &rgb);
      gimp_rgb_get_uchar (&rgb, &(rgb_dst[0]), &(rgb_dst[1]), &(rgb_dst[2]));

      rgb_dst += 3;
      hue_src += hue_incr;
      sat_src += sat_incr;
      lum_src += lum_incr;

      if (dst_has_alpha)
        rgb_dst++;
    }
}


static void
compose_cmy (guchar **src,
             gint    *incr_src,
             gint     numpix,
             guchar  *dst,
             gboolean dst_has_alpha)
{
  register const guchar *cyan_src    = src[0];
  register const guchar *magenta_src = src[1];
  register const guchar *yellow_src  = src[2];
  register       guchar *rgb_dst     = dst;
  register       gint    count       = numpix;
  gint cyan_incr    = incr_src[0];
  gint magenta_incr = incr_src[1];
  gint yellow_incr  = incr_src[2];

  if ((cyan_incr == 1) && (magenta_incr == 1) && (yellow_incr == 1))
    {
      while (count-- > 0)
	{
	  *(rgb_dst++) = 255 - *(cyan_src++);
	  *(rgb_dst++) = 255 - *(magenta_src++);
	  *(rgb_dst++) = 255 - *(yellow_src++);
          if (dst_has_alpha)
            rgb_dst++;
	}
    }
  else
    {
      while (count-- > 0)
	{
	  *(rgb_dst++) = 255 - *cyan_src;
	  *(rgb_dst++) = 255 - *magenta_src;
	  *(rgb_dst++) = 255 - *yellow_src;
	  cyan_src += cyan_incr;
	  magenta_src += magenta_incr;
	  yellow_src += yellow_incr;
          if (dst_has_alpha)
            rgb_dst++;
	}
    }
}


static void
compose_cmyk (guchar **src,
              gint    *incr_src,
              gint     numpix,
              guchar  *dst,
              gboolean dst_has_alpha)
{
  register const guchar *cyan_src    = src[0];
  register const guchar *magenta_src = src[1];
  register const guchar *yellow_src  = src[2];
  register const guchar *black_src   = src[3];
  register       guchar *rgb_dst     = dst;
  register       gint    count       = numpix;
  gint cyan, magenta, yellow, black;
  gint cyan_incr    = incr_src[0];
  gint magenta_incr = incr_src[1];
  gint yellow_incr  = incr_src[2];
  gint black_incr   = incr_src[3];

  while (count-- > 0)
    {
      black = (gint)*black_src;
      if (black)
	{
	  cyan    = (gint) *cyan_src;
	  magenta = (gint) *magenta_src;
	  yellow  = (gint) *yellow_src;

	  cyan    += black; if (cyan > 255) cyan = 255;
	  magenta += black; if (magenta > 255) magenta = 255;
	  yellow  += black; if (yellow > 255) yellow = 255;

	  *(rgb_dst++) = 255 - cyan;
	  *(rgb_dst++) = 255 - magenta;
	  *(rgb_dst++) = 255 - yellow;
	}
      else
	{
	  *(rgb_dst++) = 255 - *cyan_src;
	  *(rgb_dst++) = 255 - *magenta_src;
	  *(rgb_dst++) = 255 - *yellow_src;
	}
      cyan_src += cyan_incr;
      magenta_src += magenta_incr;
      yellow_src += yellow_incr;
      black_src += black_incr;

      if (dst_has_alpha)
        rgb_dst++;
    }
}

static void
compose_lab (guchar **src,
             gint    *incr_src,
             gint     numpix,
             guchar  *dst,
             gboolean dst_has_alpha)
{
  register guchar *l_src = src[0];
  register guchar *a_src = src[1];
  register guchar *b_src = src[2];
  register guchar *rgb_dst = dst;

  register gint count = numpix;
  gint l_incr = incr_src[0], a_incr = incr_src[1], b_incr = incr_src[2];

  gdouble red, green, blue;
  gdouble x, y, z;
  gdouble l, a, b;

  gdouble p, yyn;
  gdouble ha, hb, sqyyn;

  while (count-- > 0)
    {
      l = *l_src / 2.550;
      a = ( *a_src - 128.0 ) / 1.27;
      b = ( *b_src - 128.0 ) / 1.27;

      p = (l + 16.) / 116.;
      yyn = p*p*p;

      if (yyn > 0.008856)
        {
          y = Yn * yyn;
          ha = (p + a/500.);
          x = Xn * ha*ha*ha;
          hb = (p - b/200.);
          z = Zn * hb*hb*hb;
        }
      else
        {
          y = Yn * l/903.3;
          sqyyn = pow(l/903.3,1./3.);
          ha = a/500./7.787 + sqyyn;
          x = Xn * ha*ha*ha;
          hb = sqyyn - b/200./7.787;
          z = Zn * hb*hb*hb;
        };

      red   =  3.063 * x - 1.393 * y - 0.476 * z;
      green = -0.969 * x + 1.876 * y + 0.042 * z;
      blue  =  0.068 * x - 0.229 * y + 1.069 * z;

      red   = ( red   > 0 ) ? red   : 0;
      green = ( green > 0 ) ? green : 0;
      blue  = ( blue  > 0 ) ? blue  : 0;

      red   = ( red   < 1.0 ) ? red   : 1.0;
      green = ( green < 1.0 ) ? green : 1.0;
      blue  = ( blue  < 1.0 ) ? blue  : 1.0;

      rgb_dst[0] = (guchar) ( red   * 255.999 );
      rgb_dst[1] = (guchar) ( green * 255.999 );
      rgb_dst[2] = (guchar) ( blue  * 255.999 );

      rgb_dst += 3;
      l_src += l_incr;
      a_src += a_incr;
      b_src += b_incr;

      if (dst_has_alpha)
        rgb_dst++;
    }
}


/* these are here so the code is more readable and we can use
   the standart values instead of some scaled and rounded fixpoint values */
#define FIX(a) ((int)((a)*256.0*256.0 + 0.5))
#define FIXY(a) ((int)((a)*256.0*256.0*255.0/219.0 + 0.5))
#define FIXC(a) ((int)((a)*256.0*256.0*255.0/224.0 + 0.5))


static void
compose_ycbcr470 (guchar **src,
                  gint    *incr_src,
                  gint     numpix,
                  guchar  *dst,
                  gboolean dst_has_alpha)
{
  register const guchar *y_src   = src[0];
  register const guchar *cb_src  = src[1];
  register const guchar *cr_src  = src[2];
  register       guchar *rgb_dst = dst;
  register       gint    count   = numpix;
  gint y_incr  = incr_src[0];
  gint cb_incr = incr_src[1];
  gint cr_incr = incr_src[2];

  while (count-- > 0)
    {
      int r,g,b,y,cb,cr;
      y = *y_src  - 16;
      cb= *cb_src - 128;
      cr= *cr_src - 128;
      y_src  += y_incr;
      cb_src += cb_incr;
      cr_src += cr_incr;

      r = (FIXY(1.0)*y                   + FIXC(1.4022)*cr + FIX(0.5))>>16;
      g = (FIXY(1.0)*y - FIXC(0.3456)*cb - FIXC(0.7145)*cr + FIX(0.5))>>16;
      b = (FIXY(1.0)*y + FIXC(1.7710)*cb                   + FIX(0.5))>>16;

      if(((unsigned)r) > 255) r = ((r>>10)&255)^255;
      if(((unsigned)g) > 255) g = ((g>>10)&255)^255;
      if(((unsigned)b) > 255) b = ((b>>10)&255)^255;

      *(rgb_dst++) = r;
      *(rgb_dst++) = g;
      *(rgb_dst++) = b;
      if (dst_has_alpha)
        rgb_dst++;
    }
}


static void
compose_ycbcr709 (guchar **src,
                  gint    *incr_src,
                  gint     numpix,
                  guchar  *dst,
                  gboolean dst_has_alpha)
{
  register const guchar *y_src   = src[0];
  register const guchar *cb_src  = src[1];
  register const guchar *cr_src  = src[2];
  register       guchar *rgb_dst = dst;
  register       gint    count   = numpix;
  gint y_incr  = incr_src[0];
  gint cb_incr = incr_src[1];
  gint cr_incr = incr_src[2];

  while (count-- > 0)
    {
      int r,g,b,y,cb,cr;
      y = *y_src  - 16;
      cb= *cb_src - 128;
      cr= *cr_src - 128;
      y_src  += y_incr;
      cb_src += cb_incr;
      cr_src += cr_incr;

      r = (FIXY(1.0)*y                   + FIXC(1.5748)*cr + FIX(0.5))>>16;
      g = (FIXY(1.0)*y - FIXC(0.1873)*cb - FIXC(0.4681)*cr + FIX(0.5))>>16;
      b = (FIXY(1.0)*y + FIXC(1.8556)*cb                   + FIX(0.5))>>16;

      if(((unsigned)r) > 255) r = ((r>>10)&255)^255;
      if(((unsigned)g) > 255) g = ((g>>10)&255)^255;
      if(((unsigned)b) > 255) b = ((b>>10)&255)^255;

      *(rgb_dst++) = r;
      *(rgb_dst++) = g;
      *(rgb_dst++) = b;
      if (dst_has_alpha)
        rgb_dst++;
    }
}


static void
compose_ycbcr470f (guchar **src,
                   gint    *incr_src,
                   gint     numpix,
                   guchar  *dst,
                   gboolean dst_has_alpha)
{
  register const guchar *y_src   = src[0];
  register const guchar *cb_src  = src[1];
  register const guchar *cr_src  = src[2];
  register       guchar *rgb_dst = dst;
  register       gint    count   = numpix;
  gint y_incr  = incr_src[0];
  gint cb_incr = incr_src[1];
  gint cr_incr = incr_src[2];

  while (count-- > 0)
    {
      int r,g,b,y,cb,cr;
      y = *y_src;
      cb= *cb_src - 128;
      cr= *cr_src - 128;
      y_src  += y_incr;
      cb_src += cb_incr;
      cr_src += cr_incr;

      r = (FIX(1.0)*y                  + FIX(1.4022)*cr + FIX(0.5))>>16;
      g = (FIX(1.0)*y - FIX(0.3456)*cb - FIX(0.7145)*cr + FIX(0.5))>>16;
      b = (FIX(1.0)*y + FIX(1.7710)*cb                  + FIX(0.5))>>16;

      if(((unsigned)r) > 255) r = ((r>>10)&255)^255;
      if(((unsigned)g) > 255) g = ((g>>10)&255)^255;
      if(((unsigned)b) > 255) b = ((b>>10)&255)^255;

      *(rgb_dst++) = r;
      *(rgb_dst++) = g;
      *(rgb_dst++) = b;
      if (dst_has_alpha)
        rgb_dst++;
    }
}


static void
compose_ycbcr709f (guchar **src,
                   gint    *incr_src,
                   gint     numpix,
                   guchar  *dst,
                   gboolean dst_has_alpha)
{
  register const guchar *y_src   = src[0];
  register const guchar *cb_src  = src[1];
  register const guchar *cr_src  = src[2];
  register       guchar *rgb_dst = dst;
  register       gint    count   = numpix;
  gint y_incr  = incr_src[0];
  gint cb_incr = incr_src[1];
  gint cr_incr = incr_src[2];

  while (count-- > 0)
    {
      int r,g,b,y,cb,cr;
      y = *y_src;
      cb= *cb_src - 128;
      cr= *cr_src - 128;
      y_src  += y_incr;
      cb_src += cb_incr;
      cr_src += cr_incr;

      r = (FIX(1.0)*y                   + FIX(1.5748)*cr + FIX(0.5))>>16;
      g = (FIX(1.0)*y - FIX(0.1873)*cb  - FIX(0.4681)*cr + FIX(0.5))>>16;
      b = (FIX(1.0)*y + FIX(1.8556)*cb                   + FIX(0.5))>>16;

      if(((unsigned)r) > 255) r = ((r>>10)&255)^255;
      if(((unsigned)g) > 255) g = ((g>>10)&255)^255;
      if(((unsigned)b) > 255) b = ((b>>10)&255)^255;

      *(rgb_dst++) = r;
      *(rgb_dst++) = g;
      *(rgb_dst++) = b;
      if (dst_has_alpha)
        rgb_dst++;
    }
}


static gboolean
compose_dialog (const gchar *compose_type,
                gint32       drawable_ID)
{
  GtkWidget    *dialog;
  GtkWidget    *main_vbox;
  GtkWidget    *frame;
  GtkWidget    *vbox;
  GtkWidget    *hbox;
  GtkWidget    *label;
  GtkWidget    *combo;
  GtkWidget    *table;
  GtkSizeGroup *size_group;
  gint32       *layer_list;
  gint          nlayers;
  gint          j;
  gboolean      run;

  /* Check default compose type */
  composeint.compose_idx = 0;
  for (j = 0; j < G_N_ELEMENTS (compose_dsc); j++)
    {
      if (g_ascii_strcasecmp (compose_type, compose_dsc[j].compose_type) == 0)
        {
          composeint.compose_idx = j;
          break;
        }
    }

  /* Save original image width/height */
  composeint.width  = gimp_drawable_width (drawable_ID);
  composeint.height = gimp_drawable_height (drawable_ID);

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  layer_list = gimp_image_get_layers (gimp_drawable_get_image (drawable_ID),
                                      &nlayers);

  dialog = gimp_dialog_new (_("Compose"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, COMPOSE_PROC,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (check_response),
                    NULL);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  /* Compose type combo */

  frame = gimp_frame_new (_("Compose Channels"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  label = gtk_label_new_with_mnemonic (_("Color _model:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_size_group_add_widget (size_group, label);
  g_object_unref (size_group);

  combo = g_object_new (GIMP_TYPE_INT_COMBO_BOX, NULL);
  for (j = 0; j < G_N_ELEMENTS (compose_dsc); j++)
    {
      gchar *label = g_strdup (gettext (compose_dsc[j].compose_type));
      gchar *l;

      for (l = label; *l; l++)
        if (*l == '-' || *l == '_')
          *l = ' ';

      gimp_int_combo_box_append (GIMP_INT_COMBO_BOX (combo),
                                 GIMP_INT_STORE_LABEL, label,
                                 GIMP_INT_STORE_VALUE, j,
                                 -1);
      g_free (label);
    }

  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

  /* Channel representation table */

  frame = gimp_frame_new (_("Channel Representations"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  table = gtk_table_new (MAX_COMPOSE_IMAGES, 4, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  for (j = 0; j < MAX_COMPOSE_IMAGES; j++)
    {
      GtkWidget    *image;
      GtkWidget    *label;
      GtkWidget    *combo;
      GtkObject    *scale;
      GtkTreeIter   iter;
      GtkTreeModel *model;
      GdkPixbuf    *ico;

      hbox = gtk_hbox_new (FALSE, 6);
      gtk_table_attach (GTK_TABLE (table), hbox, 0, 1, j, j + 1,
                        GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (hbox);

      gtk_size_group_add_widget (size_group, hbox);

      composeint.channel_icon[j] = image = gtk_image_new ();
      gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
      gtk_widget_show (image);

      composeint.channel_label[j] = label = gtk_label_new_with_mnemonic ("");
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      if (composeint.compose_idx >= 0 &&
          nlayers >= compose_dsc[composeint.compose_idx].num_images &&
          j < nlayers)
        {
          composeint.selected[j].comp.ID = layer_list[j];
        }
      else
        {
          composeint.selected[j].comp.ID = drawable_ID;
        }

      composeint.selected[j].is_ID = TRUE;

      combo = gimp_drawable_combo_box_new (check_gray, NULL);
      composeint.channel_menu[j] = combo;

      ico = gtk_widget_render_icon (dialog,
                                    GIMP_STOCK_CHANNEL_GRAY,
                                    GTK_ICON_SIZE_BUTTON, NULL);
      model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
      gtk_list_store_append (GTK_LIST_STORE (model), &iter);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                          GIMP_INT_STORE_VALUE,  -1,
                          GIMP_INT_STORE_LABEL,  _("Mask value"),
                          GIMP_INT_STORE_PIXBUF, ico,
                          -1);
      g_object_unref (ico);
      gtk_table_attach (GTK_TABLE (table), combo, 1, 2, j, j + 1,
			GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
      gtk_widget_show (combo);

      gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

      scale = gimp_color_scale_entry_new (GTK_TABLE (table), 2, j, NULL,
                                          100, 4,
                                          255.0, 0.0, 255.0, 1.0, 10.0, 0,
                                          NULL, NULL);
      composeint.color_scales[j] = GIMP_SCALE_ENTRY_SCALE (scale);
      composeint.color_spins[j]  = GIMP_SCALE_ENTRY_SPINBUTTON (scale);

      gtk_widget_set_sensitive (composeint.color_scales[j], FALSE);
      gtk_widget_set_sensitive (composeint.color_spins[j],  FALSE);

      g_signal_connect (scale, "value-changed",
                        G_CALLBACK (scale_callback),
                        &composeint.selected[j]);

      /* This has to be connected last otherwise it will emit before
       * combo_callback has any scale and spinbutton to work with
       */
      gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                                  composeint.selected[j].comp.ID,
                                  G_CALLBACK (combo_callback),
                                  GINT_TO_POINTER (j));
    }

  g_free (layer_list);

  /* Calls the combo callback and sets icons, labels and sensitivity */
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                              composeint.compose_idx,
                              G_CALLBACK (type_combo_callback),
                              NULL);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  if (run)
    {
      gint j;

      for (j = 0; j < MAX_COMPOSE_IMAGES; j++)
        {
          composevals.inputs[j].is_ID = composeint.selected[j].is_ID;

          if (composevals.inputs[j].is_ID)
            composevals.inputs[j].comp.ID = composeint.selected[j].comp.ID;
          else
            composevals.inputs[j].comp.val = composeint.selected[j].comp.val;
        }

      strcpy (composevals.compose_type,
              compose_dsc[composeint.compose_idx].compose_type);
    }

  return run;
}

/*  Compose interface functions  */

static gboolean
check_gray (gint32   image_id,
            gint32   drawable_id,
            gpointer data)

{
  return ((gimp_image_base_type (image_id) == GIMP_GRAY) &&
	  (gimp_image_width  (image_id) == composeint.width) &&
	  (gimp_image_height (image_id) == composeint.height));
}

static void
check_response (GtkWidget *dialog,
                gint       response,
                gpointer   data)
{
  switch (response)
    {
    case GTK_RESPONSE_OK:
      {
        gint     i;
        gint     nb = 0;
        gboolean has_image = FALSE;

        nb = compose_dsc[composeint.compose_idx].num_images;

        for (i = 0; i < nb; i++)
          {
            if (composeint.selected[i].is_ID)
              {
                has_image = TRUE;
                break;
              }
          }

        if (! has_image)
          {
            GtkWidget *d;

            g_signal_stop_emission_by_name (dialog, "response");
            d = gtk_message_dialog_new (GTK_WINDOW (dialog),
                                        GTK_DIALOG_DESTROY_WITH_PARENT |
                                        GTK_DIALOG_MODAL,
                                        GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_CLOSE,
                                        _("At least one image is needed to compose"));

            gtk_dialog_run (GTK_DIALOG (d));
            gtk_widget_destroy (d);
          }
      }
      break;

    default:
      break;
    }
}

static void
combo_callback (GimpIntComboBox *widget,
                gpointer         data)
{
  gint id;
  gint n;

  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), &id);
  n = GPOINTER_TO_INT (data);

  if (id == -1)
    {
      gtk_widget_set_sensitive (composeint.color_scales[n], TRUE);
      gtk_widget_set_sensitive (composeint.color_spins[n],  TRUE);

      composeint.selected[n].is_ID    = FALSE;
      composeint.selected[n].comp.val =
          gtk_range_get_value (GTK_RANGE (composeint.color_scales[n]));
    }
  else
    {
      gtk_widget_set_sensitive (composeint.color_scales[n], FALSE);
      gtk_widget_set_sensitive (composeint.color_spins[n],  FALSE);

      composeint.selected[n].is_ID   = TRUE;
      composeint.selected[n].comp.ID = id;
    }
}

static void
scale_callback (GtkAdjustment *adj,
                ComposeInput  *input)
{
  input->comp.val = gtk_adjustment_get_value (adj);
}

static void
type_combo_callback (GimpIntComboBox *combo,
                     gpointer         data)
{
  if (gimp_int_combo_box_get_active (combo, &composeint.compose_idx))
    {
      gboolean combo4;
      gboolean scale4;
      gint     compose_idx;
      gint     j;

      compose_idx = composeint.compose_idx;

      for (j = 0; j < MAX_COMPOSE_IMAGES; j++)
        {
          GtkWidget   *label = composeint.channel_label[j];
          GtkWidget   *image = composeint.channel_icon[j];
          const gchar *text  = compose_dsc[compose_idx].channel_name[j];
          const gchar *icon  = compose_dsc[compose_idx].channel_icon[j];

          gtk_label_set_text_with_mnemonic (GTK_LABEL (label),
                                            text ? gettext (text) : "");

          if (icon)
            {
              gtk_image_set_from_stock (GTK_IMAGE (image),
                                        icon, GTK_ICON_SIZE_BUTTON);
              gtk_widget_show (image);
            }
          else
            {
              gtk_image_clear (GTK_IMAGE (image));
              gtk_widget_hide (image);
            }
        }

      /* Set sensitivity of last menu */
      combo4 = (compose_dsc[compose_idx].channel_name[3] != NULL);
      gtk_widget_set_sensitive (composeint.channel_menu[3], combo4);

      scale4 = combo4 && !composeint.selected[3].is_ID;
      gtk_widget_set_sensitive (composeint.color_scales[3], scale4);
      gtk_widget_set_sensitive (composeint.color_spins[3], scale4);
    }
}
