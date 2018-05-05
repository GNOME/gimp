/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Compose plug-in (C) 1997,1999 Peter Kirchgessner
 * e-mail: peter@kirchgessner.net, WWW: http://www.kirchgessner.net
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

/*
 * All redundant _256 versions of YCbCr* are here only for compatibility .
 * They can be dropped for GIMP 3.0
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
#define PLUG_IN_ROLE          "gimp-compose"

/* Maximum number of images to compose */
#define MAX_COMPOSE_IMAGES 4

typedef struct
{
  union
    {
      gint32 ID;  /* Image ID of input images or drawable */
      guchar val; /* Mask value to compose with */
    } comp;
  gboolean is_ID;
} ComposeInput;

/* Description of a component */
typedef struct
{
  const gchar    *babl_name;
  const gchar    *name;
  const gchar    *icon;
  const float     range_min;      /* val min of the component */
  const float     range_max;      /* val max of the component */
  const gboolean  is_perceptual;  /* Take the componenent from an Y' or Y buffer */
} COMPONENT_DSC;

/* Description of a composition */
typedef struct
{
  const gchar         *babl_model;
  const gchar         *compose_type;  /*  Type of composition ("RGB", "RGBA",...)  */
  gint                 num_images;    /*  Number of input images needed            */
                                      /*  Channel information                     */
  const COMPONENT_DSC  components[MAX_COMPOSE_IMAGES];
  const gchar         *filename;      /*  Name of new image                        */

} COMPOSE_DSC;


/* Declare local functions
 */
static void      query                  (void);
static void      run                    (const gchar     *name,
                                         gint            nparams,
                                         const GimpParam *param,
                                         gint            *nreturn_vals,
                                         GimpParam      **return_vals);

static void      cpn_affine_transform   (GeglBuffer      *buffer,
                                         gdouble          min,
                                         gdouble          max);

static void fill_buffer_from_components (GeglBuffer      *temp[MAX_COMPOSE_IMAGES],
                                         GeglBuffer      *dst,
                                         gint             num_cpn,
                                         ComposeInput    *inputs,
                                         gdouble          mask_vals[MAX_COMPOSE_IMAGES]);

static void      perform_composition    (COMPOSE_DSC      curr_compose_dsc,
                                         GeglBuffer      *buffer_src[MAX_COMPOSE_IMAGES],
                                         GeglBuffer      *buffer_dst,
                                         ComposeInput    *inputs,
                                         gint             num_images);

static gint32    compose                (const gchar     *compose_type,
                                         ComposeInput    *inputs,
                                         gboolean         compose_by_drawable);

static gint32    create_new_image       (const gchar     *filename,
                                         guint            width,
                                         guint            height,
                                         GimpImageType    gdtype,
                                         GimpPrecision    precision,
                                         gint32          *layer_ID,
                                         GeglBuffer     **drawable);

static gboolean  compose_dialog         (const gchar     *compose_type,
                                         gint32           drawable_ID);

static gboolean  check_gray             (gint32           image_id,
                                         gint32           drawable_id,
                                         gpointer         data);

static void      combo_callback         (GimpIntComboBox *cbox,
                                         gpointer         data);

static void      scale_callback         (GtkAdjustment   *adj,
                                         ComposeInput    *input);

static void      check_response         (GtkWidget       *dialog,
                                         gint             response,
                                         gpointer         data);

static void      type_combo_callback    (GimpIntComboBox *combo,
                                         gpointer         data);



/* Decompositions availables.
 * All the following values have to be kept in sync with those of decompose.c
 */

#define CPN_RGBA_R { "R", N_("_Red:"),   GIMP_ICON_CHANNEL_RED, 0.0, 1.0, FALSE}
#define CPN_RGBA_G { "G", N_("_Green:"), GIMP_ICON_CHANNEL_GREEN, 0.0, 1.0, FALSE}
#define CPN_RGBA_B { "B", N_("_Blue:"),  GIMP_ICON_CHANNEL_BLUE, 0.0, 1.0, FALSE}
#define CPN_RGBA_A { "A", N_("_Alpha:"), GIMP_ICON_CHANNEL_ALPHA, 0.0, 1.0, TRUE}

#define CPN_HSV_H  { "hue",        N_("_Hue:"),        NULL, 0.0, 1.0, TRUE}
#define CPN_HSV_S  { "saturation", N_("_Saturation:"), NULL, 0.0, 1.0, TRUE}
#define CPN_HSV_V  { "value",      N_("_Value:"),      NULL, 0.0, 1.0, TRUE}

#define CPN_HSL_H  { "hue",        N_("_Hue:"),        NULL, 0.0, 1.0, TRUE}
#define CPN_HSL_S  { "saturation", N_("_Saturation:"), NULL, 0.0, 1.0, TRUE}
#define CPN_HSL_L  { "lightness",  N_("_Lightness:"),  NULL, 0.0, 1.0, TRUE}

#define CPN_CMYK_C { "cyan",    N_("_Cyan:"),    NULL, 0.0, 1.0, TRUE}
#define CPN_CMYK_M { "magenta", N_("_Magenta:"), NULL, 0.0, 1.0, TRUE}
#define CPN_CMYK_Y { "yellow",  N_("_Yellow:"),  NULL, 0.0, 1.0, TRUE}
#define CPN_CMYK_K { "key",     N_("_Black:"),   NULL, 0.0, 1.0, TRUE}

#define CPN_CMY_C  { "cyan",    N_("_Cyan:"),    NULL, 0.0, 1.0, TRUE}
#define CPN_CMY_M  { "magenta", N_("_Magenta:"), NULL, 0.0, 1.0, TRUE}
#define CPN_CMY_Y  { "yellow",  N_("_Yellow:"),  NULL, 0.0, 1.0, TRUE}

#define CPN_LAB_L  { "CIE L", N_("_L:"), NULL, 0.0, 100.0, TRUE}
#define CPN_LAB_A  { "CIE a", N_("_A:"), NULL, -127.5, 127.5, TRUE}
#define CPN_LAB_B  { "CIE b", N_("_B:"), NULL, -127.5, 127.5, TRUE}

#define CPN_LCH_L  { "CIE L",     N_("_L"), NULL, 0.0, 100.0, TRUE}
#define CPN_LCH_C  { "CIE C(ab)", N_("_C"), NULL, 0.0, 200.0, TRUE}
#define CPN_LCH_H  { "CIE H(ab)", N_("_H"), NULL, 0.0, 360.0, TRUE}

#define CPN_YCBCR_Y  { "Y'", N_("_Luma y470:"),      NULL,  0.0, 1.0, TRUE }
#define CPN_YCBCR_CB { "Cb", N_("_Blueness cb470:"), NULL, -0.5, 0.5, TRUE }
#define CPN_YCBCR_CR { "Cr", N_("_Redness cr470:"),  NULL, -0.5, 0.5, TRUE }

#define CPN_YCBCR709_Y  { "Y'", N_("_Luma y709:"),      NULL,  0.0, 1.0, TRUE }
#define CPN_YCBCR709_CB { "Cb", N_("_Blueness cb709:"), NULL, -0.5, 0.5, TRUE }
#define CPN_YCBCR709_CR { "Cr", N_("_Redness cr709:"),  NULL, -0.5, 0.5, TRUE }


static COMPOSE_DSC compose_dsc[] =
{
  { "RGB",
    N_("RGB"), 3,
    { CPN_RGBA_R,
      CPN_RGBA_G,
      CPN_RGBA_B },
    "rgb-compose" },

  { "RGBA",
    N_("RGBA"), 4,
    { CPN_RGBA_R,
      CPN_RGBA_G,
      CPN_RGBA_B,
      CPN_RGBA_A },
    "rgba-compose" },

  { "HSV",
    N_("HSV"), 3,
    { CPN_HSV_H,
      CPN_HSV_S,
      CPN_HSV_V },
    "hsv-compose" },

  { "HSL",
    N_("HSL"), 3,
    { CPN_HSL_H,
      CPN_HSL_S,
      CPN_HSL_L },
    "hsl-compose" },

  { "CMY",
    N_("CMY"), 3,
    { CPN_CMY_C,
      CPN_CMY_M,
      CPN_CMY_Y },
    "cmy-compose" },

  { "CMYK",
    N_("CMYK"), 4,
    { CPN_CMYK_C,
      CPN_CMYK_M,
      CPN_CMYK_Y,
      CPN_CMYK_K },
    "cmyk-compose" },

  { "CIE Lab",
    N_("LAB"), 3,
    { CPN_LAB_L,
      CPN_LAB_A,
      CPN_LAB_B },
    "lab-compose" },

  { "CIE LCH(ab)",
    N_("LCH"), 3,
    { CPN_LCH_L,
      CPN_LCH_C,
      CPN_LCH_H },
    "lch-compose" },

  { "Y'CbCr",
    N_("YCbCr_ITU_R470"), 3,
    { CPN_YCBCR_Y,
      CPN_YCBCR_CB,
      CPN_YCBCR_CR },
    "ycbcr470-compose" },

  { "Y'CbCr709",
    N_("YCbCr_ITU_R709"), 3,
    { CPN_YCBCR709_Y,
      CPN_YCBCR709_CB,
      CPN_YCBCR709_CR },
    "ycbcr709-compose" },

  { "Y'CbCr",
    N_("YCbCr_ITU_R470_256"), 3,
    { CPN_YCBCR_Y,
      CPN_YCBCR_CB,
      CPN_YCBCR_CR },
    "ycbcr470F-compose" },

  { "Y'CbCr709",
    N_("YCbCr_ITU_R709_256"), 3,
    { CPN_YCBCR709_Y,
      CPN_YCBCR709_CB,
      CPN_YCBCR709_CR },
    "ycbcr709F-compose" }
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


MAIN ()


static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
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
    { GIMP_PDB_INT32,    "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
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
    { GIMP_PDB_INT32,    "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",    "Image to recompose from" },
    { GIMP_PDB_DRAWABLE, "drawable", "Not used" },
  };

  GString *type_desc;
  gint     i;

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
  GimpRunMode       run_mode;
  gint32            image_ID;
  gint32            drawable_ID = -1;
  gint              compose_by_drawable;
  gint              i;

  INIT_I18N ();
  gegl_init (NULL, NULL);

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
      GimpParasite *parasite = gimp_image_get_parasite (param[1].data.d_image,
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

static void
cpn_affine_transform (GeglBuffer *buffer,
                      gdouble     min,
                      gdouble     max)
{
  GeglBufferIterator *gi;
  const gdouble       scale  = max - min;
  const gdouble       offset = min;

  /* We want to scale values linearly, regardless of the format of the buffer */
  gegl_buffer_set_format (buffer, babl_format ("Y double"));

  gi = gegl_buffer_iterator_new (buffer, NULL, 0, NULL,
                                 GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (gi))
    {
      gdouble *data = gi->data[0];
      guint    k;

      for (k = 0; k < gi->length; k++)
        {
          data[k] = data[k] * scale + offset;
        }
    }
}

static void
fill_buffer_from_components (GeglBuffer   *temp[MAX_COMPOSE_IMAGES],
                             GeglBuffer   *dst,
                             gint          num_cpn,
                             ComposeInput *inputs,
                             gdouble       mask_vals[MAX_COMPOSE_IMAGES])
{
  GeglBufferIterator *gi;
  gint                j;

  gi = gegl_buffer_iterator_new (dst, NULL, 0, NULL,
                                 GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  for (j = 0; j < num_cpn; j++)
    {
      if (inputs[j].is_ID)
        gegl_buffer_iterator_add (gi, temp[j], NULL, 0, NULL,
                                  GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
    }

  while (gegl_buffer_iterator_next (gi))
    {
      gdouble *src_data[MAX_COMPOSE_IMAGES];
      gdouble *dst_data = (gdouble*) gi->data[0];
      gulong   k, count;

      count = 1;
      for (j = 0; j < num_cpn; j++)
        if (inputs[j].is_ID)
          src_data[j] = (gdouble*) gi->data[count++];

      for (k = 0; k < gi->length; k++)
        {
          gulong pos = k * num_cpn;

          for (j = 0; j < num_cpn; j++)
            {
              if (inputs[j].is_ID)
                dst_data[pos+j] = src_data[j][k];
              else
                dst_data[pos+j] = mask_vals[j];
            }
        }
    }
}

static void
perform_composition (COMPOSE_DSC   curr_compose_dsc,
                     GeglBuffer   *buffer_src[MAX_COMPOSE_IMAGES],
                     GeglBuffer   *buffer_dst,
                     ComposeInput *inputs,
                     gint          num_images)
{
  const Babl          *dst_format;
  GeglBuffer          *temp[MAX_COMPOSE_IMAGES];
  GeglBuffer          *dst_temp;
  const GeglRectangle *extent = NULL;

  const COMPONENT_DSC *components;
  gdouble              mask_vals[MAX_COMPOSE_IMAGES];
  gint                 i;

  components = curr_compose_dsc.components;

  /* Get all individual components in gray buffers */
  for (i = 0; i < num_images; i++)
    {
      COMPONENT_DSC  cpn_dsc = components[i];
      const Babl    *gray_format;

      if (cpn_dsc.is_perceptual)
        gray_format = babl_format ("Y' double");
      else
        gray_format = babl_format ("Y double");

      if (! inputs[i].is_ID)
        {
          const Babl *fish = babl_fish (babl_format ("Y' u8"), gray_format);

          babl_process (fish, &inputs[i].comp.val, &mask_vals[i], 1);

          mask_vals[i] = mask_vals[i] * (cpn_dsc.range_max - cpn_dsc.range_min) + cpn_dsc.range_min;
        }
      else
        {
          extent = gegl_buffer_get_extent (buffer_src[i]);

          temp[i] = gegl_buffer_new (extent, gray_format);

          gegl_buffer_copy (buffer_src[i], NULL, GEGL_ABYSS_NONE, temp[i], NULL);

          if (cpn_dsc.range_min != 0.0 || cpn_dsc.range_max != 1.0)
            cpn_affine_transform (temp[i], cpn_dsc.range_min, cpn_dsc.range_max);
        }

      gimp_progress_update ((gdouble) i / (gdouble) (num_images + 1.0));
    }

  dst_format = babl_format_new (babl_model (curr_compose_dsc.babl_model),
                                babl_type ("double"),
                                babl_component (components[0].babl_name),
                                num_images > 1 ? babl_component (components[1].babl_name) : NULL,
                                num_images > 2 ? babl_component (components[2].babl_name) : NULL,
                                num_images > 3 ? babl_component (components[3].babl_name) : NULL,
                                NULL);

  /* extent is not NULL because there is at least one drawable */
  dst_temp = gegl_buffer_new (extent, dst_format);

  /* Gather all individual components in the dst_format buffer */
  fill_buffer_from_components (temp, dst_temp, num_images, inputs, mask_vals);

  gimp_progress_update ((gdouble) num_images / (gdouble) (num_images + 1.0));

  /* Copy back to the format GIMP wants (and perform the conversion in itself) */
  gegl_buffer_copy (dst_temp, NULL, GEGL_ABYSS_NONE, buffer_dst, NULL);

  for (i = 0; i< num_images; i++)
    if( inputs[i].is_ID)
      g_object_unref (temp[i]);

  g_object_unref (dst_temp);
}

/* Compose an image from several gray-images */
static gint32
compose (const gchar  *compose_type,
         ComposeInput *inputs,
         gboolean      compose_by_drawable)
{
  gint           width, height;
  gint           num_images, compose_idx;
  gint           i, j;
  gint           num_layers;
  gint32         layer_ID_dst, image_ID_dst;
  gint           first_ID;
  GimpImageType  gdtype_dst;
  GeglBuffer    *buffer_src[MAX_COMPOSE_IMAGES];
  GeglBuffer    *buffer_dst;
  GimpPrecision  precision;

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
      g_message (_("At least one image is needed to compose"));
      return -1;
    }

  /* Check image sizes */
  if (compose_by_drawable)
    {
      gint32 first_image = gimp_item_get_image (inputs[first_ID].comp.ID);

      if (! gimp_item_is_valid (inputs[first_ID].comp.ID))
        {
          g_message (_("Specified layer %d not found"),
                     inputs[first_ID].comp.ID);
          return -1;
        }

      width  = gimp_drawable_width  (inputs[first_ID].comp.ID);
      height = gimp_drawable_height (inputs[first_ID].comp.ID);

      precision = gimp_image_get_precision (first_image);

      for (j = first_ID + 1; j < num_images; j++)
        {
          if (inputs[j].is_ID)
            {
              if (! gimp_item_is_valid (inputs[j].comp.ID))
                {
                  g_message (_("Specified layer %d not found"),
                             inputs[j].comp.ID);
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
            buffer_src[j] = gimp_drawable_get_buffer (inputs[j].comp.ID);
          else
            buffer_src[j] = NULL;
        }
    }
  else    /* Compose by image ID */
    {
      width  = gimp_image_width  (inputs[first_ID].comp.ID);
      height = gimp_image_height (inputs[first_ID].comp.ID);

      precision = gimp_image_get_precision (inputs[first_ID].comp.ID);

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
              gint32 *layers;

              /* Get first layer of image */
              layers = gimp_image_get_layers (inputs[j].comp.ID, &num_layers);

              if (! layers || (num_layers <= 0))
                {
                  g_message (_("Error in getting layer IDs"));
                  return -1;
                }

              /* Get drawable for layer */
              buffer_src[j] = gimp_drawable_get_buffer (layers[0]);
              g_free (layers);
            }
        }
    }

  /* Unless recomposing, create new image */
  if (composevals.do_recompose)
    {
      layer_ID_dst = composevals.source_layer_ID;

      if (! gimp_item_is_valid (layer_ID_dst))
        {
          g_message (_("Unable to recompose, source layer not found"));
          return -1;
        }

      image_ID_dst = gimp_item_get_image (layer_ID_dst);

      buffer_dst = gimp_drawable_get_shadow_buffer (layer_ID_dst);
    }
  else
    {
      gdtype_dst = ((babl_model (compose_dsc[compose_idx].babl_model) == babl_model ("RGBA")) ?
                    GIMP_RGBA_IMAGE : GIMP_RGB_IMAGE);

      image_ID_dst = create_new_image (compose_dsc[compose_idx].filename,
                                       width, height, gdtype_dst, precision,
                                       &layer_ID_dst, &buffer_dst);
    }

  if (! compose_by_drawable)
    {
      gdouble  xres, yres;

      gimp_image_get_resolution (inputs[first_ID].comp.ID, &xres, &yres);
      gimp_image_set_resolution (image_ID_dst, xres, yres);
    }

  perform_composition (compose_dsc[compose_idx],
                       buffer_src,
                       buffer_dst,
                       inputs,
                       num_images);

  gimp_progress_update (1.0);

  for (j = 0; j < num_images; j++)
    {
      if (inputs[j].is_ID)
        g_object_unref (buffer_src[j]);
    }

  g_object_unref (buffer_dst);

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
                  GimpPrecision   precision,
                  gint32         *layer_ID,
                  GeglBuffer    **buffer)
{
  gint32            image_ID;
  GimpImageBaseType gitype;

  if ((gdtype == GIMP_GRAY_IMAGE) || (gdtype == GIMP_GRAYA_IMAGE))
    gitype = GIMP_GRAY;
  else if ((gdtype == GIMP_INDEXED_IMAGE) || (gdtype == GIMP_INDEXEDA_IMAGE))
    gitype = GIMP_INDEXED;
  else
    gitype = GIMP_RGB;

  image_ID = gimp_image_new_with_precision (width, height, gitype, precision);

  gimp_image_undo_disable (image_ID);
  gimp_image_set_filename (image_ID, filename);

  *layer_ID = gimp_layer_new (image_ID, _("Background"), width, height,
                              gdtype,
                              100,
                              gimp_image_get_default_new_layer_mode (image_ID));
  gimp_image_insert_layer (image_ID, *layer_ID, -1, 0);

  *buffer = gimp_drawable_get_buffer (*layer_ID);

  return image_ID;
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
  GtkWidget    *grid;
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

  layer_list = gimp_image_get_layers (gimp_item_get_image (drawable_ID),
                                      &nlayers);

  dialog = gimp_dialog_new (_("Compose"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, COMPOSE_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (check_response),
                    NULL);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  /* Compose type combo */

  frame = gimp_frame_new (_("Compose Channels"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  label = gtk_label_new_with_mnemonic (_("Color _model:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
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

  /* Channel representation grid */

  frame = gimp_frame_new (_("Channel Representations"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  for (j = 0; j < MAX_COMPOSE_IMAGES; j++)
    {
      GtkWidget     *image;
      GtkWidget     *label;
      GtkWidget     *combo;
      GtkAdjustment *scale;
      GtkTreeIter    iter;
      GtkTreeModel  *model;
      GdkPixbuf     *ico;

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      gtk_grid_attach (GTK_GRID (grid), hbox, 0, j, 1, 1);
                        // GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (hbox);

      gtk_size_group_add_widget (size_group, hbox);

      composeint.channel_icon[j] = image = gtk_image_new ();
      gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
      gtk_widget_show (image);

      composeint.channel_label[j] = label = gtk_label_new_with_mnemonic ("");
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
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

      ico = gtk_widget_render_icon_pixbuf (dialog,
                                           GIMP_ICON_CHANNEL_GRAY,
                                           GTK_ICON_SIZE_BUTTON);
      model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
      gtk_list_store_append (GTK_LIST_STORE (model), &iter);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                          GIMP_INT_STORE_VALUE,  -1,
                          GIMP_INT_STORE_LABEL,  _("Mask value"),
                          GIMP_INT_STORE_PIXBUF, ico,
                          -1);
      g_object_unref (ico);
      gtk_grid_attach (GTK_GRID (grid), combo, 1, j, 1, 1);
                        // GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
      gtk_widget_show (combo);

      gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

      scale = gimp_color_scale_entry_new_grid (GTK_GRID (grid), 2, j, NULL,
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
          const gchar *text  = compose_dsc[compose_idx].components[j].name;
          const gchar *icon  = compose_dsc[compose_idx].components[j].icon;

          gtk_label_set_text_with_mnemonic (GTK_LABEL (label),
                                            text ? gettext (text) : "");

          if (icon)
            {
              gtk_image_set_from_icon_name (GTK_IMAGE (image),
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
      combo4 = (compose_dsc[compose_idx].num_images == 4);
      gtk_widget_set_sensitive (composeint.channel_menu[3], combo4);

      scale4 = combo4 && !composeint.selected[3].is_ID;
      gtk_widget_set_sensitive (composeint.color_scales[3], scale4);
      gtk_widget_set_sensitive (composeint.color_spins[3], scale4);
    }
}
