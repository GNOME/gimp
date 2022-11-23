/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
 * They can be dropped for LIGMA 3.0
 */

#include "config.h"

#include <string.h>

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "libligma/stdplugins-intl.h"


#define COMPOSE_PROC          "plug-in-compose"
#define DRAWABLE_COMPOSE_PROC "plug-in-drawable-compose"
#define RECOMPOSE_PROC        "plug-in-recompose"
#define PLUG_IN_BINARY        "compose"
#define PLUG_IN_ROLE          "ligma-compose"

/* Maximum number of images to compose */
#define MAX_COMPOSE_IMAGES 4

typedef struct
{
  gboolean is_object;

  union
  {
    gpointer object;  /* Input images or drawable */
    guchar   val;     /* Mask value to compose with */
  } comp;
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

typedef struct
{
  ComposeInput inputs[MAX_COMPOSE_IMAGES];  /* Image or mask value of input */
  gchar        compose_type[32];            /* type of composition */
  gboolean     do_recompose;
  LigmaLayer   *source_layer;                /* for recomposing */
} ComposeVals;

/* Dialog structure */
typedef struct
{
  gint          width, height;                            /* Size of selected image */

  GtkWidget    *channel_label[MAX_COMPOSE_IMAGES];        /* The labels to change */
  GtkWidget    *channel_icon[MAX_COMPOSE_IMAGES];         /* The icons  */
  GtkWidget    *channel_menu[MAX_COMPOSE_IMAGES];         /* The menus */
  GtkWidget    *scales[MAX_COMPOSE_IMAGES];               /* The values color scales */

  ComposeInput  selected[MAX_COMPOSE_IMAGES];             /* Image Ids or mask values from menus */

  gint          compose_idx;                              /* Compose type */
} ComposeInterface;


typedef struct _Compose      Compose;
typedef struct _ComposeClass ComposeClass;

struct _Compose
{
  LigmaPlugIn parent_instance;
};

struct _ComposeClass
{
  LigmaPlugInClass parent_class;
};


#define COMPOSE_TYPE  (compose_get_type ())
#define COMPOSE (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), COMPOSE_TYPE, Compose))

GType                   compose_get_type         (void) G_GNUC_CONST;

static GList          * compose_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * compose_create_procedure (LigmaPlugIn           *plug_in,
                                                  const gchar          *name);

static LigmaValueArray * compose_run              (LigmaProcedure        *procedure,
                                                  LigmaRunMode           run_mode,
                                                  LigmaImage            *image,
                                                  gint                  n_drawables,
                                                  LigmaDrawable        **drawables,
                                                  const LigmaValueArray *args,
                                                  gpointer              run_data);

static void        cpn_affine_transform   (GeglBuffer      *buffer,
                                           gdouble          min,
                                           gdouble          max);

static void   fill_buffer_from_components (GeglBuffer      *temp[MAX_COMPOSE_IMAGES],
                                           GeglBuffer      *dst,
                                           gint             num_cpn,
                                           ComposeInput    *inputs,
                                           gdouble          mask_vals[MAX_COMPOSE_IMAGES]);

static void        perform_composition    (COMPOSE_DSC      curr_compose_dsc,
                                           GeglBuffer      *buffer_src[MAX_COMPOSE_IMAGES],
                                           GeglBuffer      *buffer_dst,
                                           ComposeInput    *inputs,
                                           gint             num_images);

static LigmaImage * compose                (const gchar     *compose_type,
                                           ComposeInput    *inputs,
                                           gboolean         compose_by_drawable);

static LigmaImage * create_new_image       (GFile           *file,
                                           guint            width,
                                           guint            height,
                                           LigmaImageType    gdtype,
                                           LigmaPrecision    precision,
                                           LigmaLayer      **layer,
                                           GeglBuffer     **buffer);

static gboolean    compose_dialog         (const gchar     *compose_type,
                                           LigmaDrawable    *drawable);

static gboolean    check_gray             (LigmaImage       *image,
                                           LigmaItem        *drawable,
                                           gpointer         data);

static void        combo_callback         (LigmaIntComboBox *cbox,
                                           gpointer         data);

static void        scale_callback         (LigmaLabelSpin   *scale,
                                           ComposeInput    *input);

static void        check_response         (GtkWidget       *dialog,
                                           gint             response,
                                           gpointer         data);

static void        type_combo_callback    (LigmaIntComboBox *combo,
                                           gpointer         data);


G_DEFINE_TYPE (Compose, compose, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (COMPOSE_TYPE)
DEFINE_STD_SET_I18N


/* Decompositions availables.
 * All the following values have to be kept in sync with those of decompose.c
 */

#define CPN_RGBA_R { "R", N_("_Red:"),   LIGMA_ICON_CHANNEL_RED, 0.0, 1.0, FALSE}
#define CPN_RGBA_G { "G", N_("_Green:"), LIGMA_ICON_CHANNEL_GREEN, 0.0, 1.0, FALSE}
#define CPN_RGBA_B { "B", N_("_Blue:"),  LIGMA_ICON_CHANNEL_BLUE, 0.0, 1.0, FALSE}
#define CPN_RGBA_A { "A", N_("_Alpha:"), LIGMA_ICON_CHANNEL_ALPHA, 0.0, 1.0, TRUE}

#define CPN_HSV_H  { "hue",        N_("_Hue:"),        NULL, 0.0, 1.0, TRUE}
#define CPN_HSV_S  { "saturation", N_("_Saturation:"), NULL, 0.0, 1.0, TRUE}
#define CPN_HSV_V  { "value",      N_("_Value:"),      NULL, 0.0, 1.0, TRUE}

#define CPN_HSL_H  { "hue",        N_("_Hue:"),        NULL, 0.0, 1.0, TRUE}
#define CPN_HSL_S  { "saturation", N_("_Saturation:"), NULL, 0.0, 1.0, TRUE}
#define CPN_HSL_L  { "lightness",  N_("_Lightness:"),  NULL, 0.0, 1.0, TRUE}

#define CPN_CMYK_C { "Cyan",    N_("_Cyan:"),    NULL, 0.0, 1.0, TRUE}
#define CPN_CMYK_M { "Magenta", N_("_Magenta:"), NULL, 0.0, 1.0, TRUE}
#define CPN_CMYK_Y { "Yellow",  N_("_Yellow:"),  NULL, 0.0, 1.0, TRUE}
#define CPN_CMYK_K { "Key",     N_("_Black:"),   NULL, 0.0, 1.0, TRUE}

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

static ComposeVals composevals =
{
  {{ 0, }}, /* Image IDs of images to compose or mask values */
  "RGB",    /* Type of composition */
  FALSE,    /* Do recompose */
  NULL      /* source layer */
};

static ComposeInterface composeint =
{
  0, 0,      /* width, height */
  { NULL },  /* Label Widgets */
  { NULL },  /* Icon Widgets */
  { NULL },  /* Menu Widgets */
  { NULL },  /* Color Scale Widgets */
  {{ 0, }},  /* Image Ids or mask values from menus */
  0          /* Compose type */
};


static void
compose_class_init (ComposeClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = compose_query_procedures;
  plug_in_class->create_procedure = compose_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
compose_init (Compose *compose)
{
}

static GList *
compose_query_procedures (LigmaPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (COMPOSE_PROC));
  list = g_list_append (list, g_strdup (DRAWABLE_COMPOSE_PROC));
  list = g_list_append (list, g_strdup (RECOMPOSE_PROC));

  return list;
}

static LigmaProcedure *
compose_create_procedure (LigmaPlugIn  *plug_in,
                           const gchar *name)
{
  LigmaProcedure *procedure = NULL;
  GString       *type_desc;
  gint           i;

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

  if (! strcmp (name, COMPOSE_PROC))
    {
      procedure = ligma_image_procedure_new (plug_in, name,
                                            LIGMA_PDB_PROC_TYPE_PLUGIN,
                                            compose_run, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "GRAY*");
      ligma_procedure_set_sensitivity_mask (procedure,
                                           LIGMA_PROCEDURE_SENSITIVE_DRAWABLE  |
                                           LIGMA_PROCEDURE_SENSITIVE_DRAWABLES |
                                           LIGMA_PROCEDURE_SENSITIVE_NO_DRAWABLES);

      ligma_procedure_set_menu_label (procedure, _("C_ompose..."));
      ligma_procedure_add_menu_path (procedure, "<Image>/Colors/Components");

      ligma_procedure_set_documentation (procedure,
                                        _("Create an image using multiple "
                                          "gray images as color channels"),
                                        "This function creates a new image from "
                                        "multiple gray images",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Peter Kirchgessner",
                                      "Peter Kirchgessner (peter@kirchgessner.net)",
                                      "1997");

      LIGMA_PROC_ARG_IMAGE (procedure, "image-2",
                           "Image 2",
                           "Second input image",
                           TRUE,
                           G_PARAM_READWRITE);

      LIGMA_PROC_ARG_IMAGE (procedure, "image-3",
                           "Image 3",
                           "Third input image",
                           TRUE,
                           G_PARAM_READWRITE);

      LIGMA_PROC_ARG_IMAGE (procedure, "image-4",
                           "Image 4",
                           "Fourth input image",
                           TRUE,
                           G_PARAM_READWRITE);

      LIGMA_PROC_ARG_STRING (procedure, "compose-type",
                            "Compose type",
                            type_desc->str,
                            "RGB",
                            G_PARAM_READWRITE);

      LIGMA_PROC_VAL_IMAGE (procedure, "new-image",
                           "New image",
                           "Output image",
                           FALSE,
                           G_PARAM_READWRITE);
    }
  else if (! strcmp (name, DRAWABLE_COMPOSE_PROC))
    {
      procedure = ligma_image_procedure_new (plug_in, name,
                                            LIGMA_PDB_PROC_TYPE_PLUGIN,
                                            compose_run, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "GRAY*");
      ligma_procedure_set_sensitivity_mask (procedure,
                                           LIGMA_PROCEDURE_SENSITIVE_DRAWABLE);

      ligma_procedure_set_documentation (procedure,
                                        "Compose an image from multiple "
                                        "drawables of gray images",
                                        "This function creates a new image from "
                                        "multiple drawables of gray images",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Peter Kirchgessner",
                                      "Peter Kirchgessner (peter@kirchgessner.net)",
                                      "1998");

      LIGMA_PROC_ARG_DRAWABLE (procedure, "drawable-2",
                              "Drawable 2",
                              "Second input drawable",
                              TRUE,
                              G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DRAWABLE (procedure, "drawable-3",
                              "Drawable 3",
                              "Third input drawable",
                              TRUE,
                              G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DRAWABLE (procedure, "drawable-4",
                              "Drawable 4",
                              "Fourth input drawable",
                              TRUE,
                              G_PARAM_READWRITE);

      LIGMA_PROC_ARG_STRING (procedure, "compose-type",
                            "Compose type",
                            type_desc->str,
                            "RGB",
                            G_PARAM_READWRITE);

      LIGMA_PROC_VAL_IMAGE (procedure, "new-image",
                           "New image",
                           "Output image",
                           FALSE,
                           G_PARAM_READWRITE);
    }
  else if (! strcmp (name, RECOMPOSE_PROC))
    {
      procedure = ligma_image_procedure_new (plug_in, name,
                                            LIGMA_PDB_PROC_TYPE_PLUGIN,
                                            compose_run, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "GRAY*");
      ligma_procedure_set_sensitivity_mask (procedure,
                                           LIGMA_PROCEDURE_SENSITIVE_DRAWABLE  |
                                           LIGMA_PROCEDURE_SENSITIVE_DRAWABLES |
                                           LIGMA_PROCEDURE_SENSITIVE_NO_DRAWABLES);

      ligma_procedure_set_menu_label (procedure, _("R_ecompose"));
      ligma_procedure_add_menu_path (procedure, "<Image>/Colors/Components");

      ligma_procedure_set_documentation (procedure,
                                        _("Recompose an image that was "
                                          "previously decomposed"),
                                        "This function recombines the grayscale "
                                        "layers produced by Decompose "
                                        "into a single RGB or RGBA layer, and "
                                        "replaces the originally decomposed "
                                        "layer with the result.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Bill Skaggs",
                                      "Bill Skaggs",
                                      "2004");
    }

  g_string_free (type_desc, TRUE);

  return procedure;
}

static LigmaValueArray *
compose_run (LigmaProcedure        *procedure,
             LigmaRunMode           run_mode,
             LigmaImage            *image,
             gint                  n_drawables,
             LigmaDrawable        **drawables,
             const LigmaValueArray *args,
             gpointer              run_data)
{
  LigmaValueArray *return_vals;
  LigmaDrawable   *drawable = NULL;
  const gchar    *name     = ligma_procedure_get_name (procedure);
  gint            compose_by_drawable;
  gint            i;

  gegl_init (NULL, NULL);

  compose_by_drawable = ! strcmp (name, DRAWABLE_COMPOSE_PROC);

  if (compose_by_drawable)
    {
      if (n_drawables != 1)
        {
          GError *error = NULL;

          g_set_error (&error, LIGMA_PLUG_IN_ERROR, 0,
                       _("Procedure '%s' only works with one drawable."),
                       name);

          return ligma_procedure_new_return_values (procedure,
                                                   LIGMA_PDB_CALLING_ERROR,
                                                   error);
        }
      else
        {
          drawable = drawables[0];
        }
    }

  if (! strcmp (name, RECOMPOSE_PROC))
    {
      LigmaParasite *parasite = ligma_image_get_parasite (image,
                                                        "decompose-data");

      if (! parasite)
        {
          g_message (_("You can only run 'Recompose' if the active image "
                       "was originally produced by 'Decompose'."));

          return ligma_procedure_new_return_values (procedure,
                                                   LIGMA_PDB_EXECUTION_ERROR,
                                                   NULL);
        }
      else
        {
          gchar   *parasite_data;
          guint32  parasite_size;
          gint     source;
          gint     input[4] = { 0, };
          gint     nret;

          parasite_data = (gchar *) ligma_parasite_get_data (parasite, &parasite_size);
          parasite_data = g_strndup (parasite_data, parasite_size);
          nret = sscanf (parasite_data,
                         "source=%d type=%31s %d %d %d %d",
                         &source,
                         composevals.compose_type,
                         input,
                         input + 1,
                         input + 2,
                         input + 3);

          ligma_parasite_free (parasite);
          g_free (parasite_data);

          if (nret < 5)
            {
              g_message (_("Error scanning 'decompose-data' parasite: "
                           "too few layers found"));

              return ligma_procedure_new_return_values (procedure,
                                                       LIGMA_PDB_EXECUTION_ERROR,
                                                       NULL);
            }

          compose_by_drawable = TRUE;

          composevals.do_recompose = TRUE;
          composevals.source_layer = ligma_layer_get_by_id (source);

          if (! composevals.source_layer)
            {
              g_message (_("Cannot recompose: Specified source layer ID %d "
                           "not found"),
                         source);

              return ligma_procedure_new_return_values (procedure,
                                                       LIGMA_PDB_EXECUTION_ERROR,
                                                       NULL);
            }

          for (i = 0; i < MAX_COMPOSE_IMAGES; i++)
            {
              composevals.inputs[i].is_object   = TRUE;
              composevals.inputs[i].comp.object = ligma_drawable_get_by_id (input[i]);

              /* fourth input is optional */
              if (i == 2 && nret == 5)
                break;

              if (! composevals.inputs[i].comp.object)
                {
                  g_message (_("Cannot recompose: Specified layer #%d ID %d "
                               "not found"),
                             i + 1, input[i]);

                  return ligma_procedure_new_return_values (procedure,
                                                           LIGMA_PDB_EXECUTION_ERROR,
                                                           NULL);
                }
            }
        }
    }
  else
    {
      composevals.do_recompose = FALSE;

      switch (run_mode)
        {
        case LIGMA_RUN_INTERACTIVE:
          ligma_get_data (name, &composevals);

          compose_by_drawable = TRUE;

          /* Get a drawable-ID of the image */
          if (! strcmp (name, COMPOSE_PROC))
            {
              LigmaLayer **layers;
              gint        nlayers;

              layers = ligma_image_get_layers (image, &nlayers);

              if (! layers)
                {
                  g_message (_("Could not get layers for image %d"),
                             (gint) ligma_image_get_id (image));

                  return ligma_procedure_new_return_values (procedure,
                                                           LIGMA_PDB_EXECUTION_ERROR,
                                                           NULL);
                }

              drawable = LIGMA_DRAWABLE (layers[0]);

              g_free (layers);
            }

          if (! compose_dialog (composevals.compose_type, drawable))
            return ligma_procedure_new_return_values (procedure,
                                                     LIGMA_PDB_CANCEL,
                                                     NULL);
          break;

        case LIGMA_RUN_NONINTERACTIVE:
          if (compose_by_drawable)
            {
              composevals.inputs[0].comp.object = drawable;
              composevals.inputs[1].comp.object = LIGMA_VALUES_GET_DRAWABLE (args, 0);
              composevals.inputs[2].comp.object = LIGMA_VALUES_GET_DRAWABLE (args, 1);
              composevals.inputs[3].comp.object = LIGMA_VALUES_GET_DRAWABLE (args, 2);
            }
          else
            {
              composevals.inputs[0].comp.object = image;
              composevals.inputs[1].comp.object = LIGMA_VALUES_GET_IMAGE (args, 0);
              composevals.inputs[2].comp.object = LIGMA_VALUES_GET_IMAGE (args, 1);
              composevals.inputs[3].comp.object = LIGMA_VALUES_GET_IMAGE (args, 2);
            }

          g_strlcpy (composevals.compose_type,
                     LIGMA_VALUES_GET_STRING (args, 3),
                     sizeof (composevals.compose_type));

          for (i = 0; i < MAX_COMPOSE_IMAGES; i++)
            {
              if (! composevals.inputs[i].comp.object)
                {
                  composevals.inputs[i].is_object = FALSE;
                  composevals.inputs[i].comp.val  = 0;
                }
              else
                {
                  composevals.inputs[i].is_object = TRUE;
                }
            }
          break;

        case LIGMA_RUN_WITH_LAST_VALS:
          ligma_get_data (name, &composevals);

          compose_by_drawable = TRUE;
          break;

        default:
          break;
        }
    }

  ligma_progress_init (_("Composing"));

  image = compose (composevals.compose_type,
                   composevals.inputs,
                   compose_by_drawable);

  if (! image)
    return ligma_procedure_new_return_values (procedure,
                                             LIGMA_PDB_EXECUTION_ERROR,
                                             NULL);

  if (composevals.do_recompose)
    {
      ligma_displays_flush ();
    }
  else
    {
      ligma_image_undo_enable (image);
      ligma_image_clean_all (image);

      if (run_mode != LIGMA_RUN_NONINTERACTIVE)
        ligma_display_new (image);
    }

  if (run_mode == LIGMA_RUN_INTERACTIVE)
    ligma_set_data (name, &composevals, sizeof (ComposeVals));

  return_vals = ligma_procedure_new_return_values (procedure,
                                                  LIGMA_PDB_SUCCESS,
                                                  NULL);

  if (strcmp (name, RECOMPOSE_PROC))
    LIGMA_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
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
                                 GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE, 1);

  while (gegl_buffer_iterator_next (gi))
    {
      gdouble *data = gi->items[0].data;
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
                                 GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 10);

  for (j = 0; j < num_cpn; j++)
    {
      if (inputs[j].is_object)
        gegl_buffer_iterator_add (gi, temp[j], NULL, 0, NULL,
                                  GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
    }

  while (gegl_buffer_iterator_next (gi))
    {
      gdouble *src_data[MAX_COMPOSE_IMAGES];
      gdouble *dst_data = (gdouble*) gi->items[0].data;
      gulong   k, count;

      count = 1;
      for (j = 0; j < num_cpn; j++)
        if (inputs[j].is_object)
          src_data[j] = (gdouble*) gi->items[count++].data;

      for (k = 0; k < gi->length; k++)
        {
          gulong pos = k * num_cpn;

          for (j = 0; j < num_cpn; j++)
            {
              if (inputs[j].is_object)
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

      if (! inputs[i].is_object)
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

      ligma_progress_update ((gdouble) i / (gdouble) (num_images + 1.0));
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

  ligma_progress_update ((gdouble) num_images / (gdouble) (num_images + 1.0));

  /* Copy back to the format LIGMA wants (and perform the conversion in itself) */
  gegl_buffer_copy (dst_temp, NULL, GEGL_ABYSS_NONE, buffer_dst, NULL);

  for (i = 0; i< num_images; i++)
    if (inputs[i].is_object)
      g_object_unref (temp[i]);

  g_object_unref (dst_temp);
}

/* Compose an image from several gray-images */
static LigmaImage *
compose (const gchar  *compose_type,
         ComposeInput *inputs,
         gboolean      compose_by_drawable)
{
  gint           width, height;
  gint           num_images, compose_idx;
  gint           i, j;
  gint           num_layers;
  LigmaLayer     *layer_dst;
  LigmaImage     *image_dst;
  gint           first_object;
  LigmaImageType  gdtype_dst;
  GeglBuffer    *buffer_src[MAX_COMPOSE_IMAGES];
  GeglBuffer    *buffer_dst;
  LigmaPrecision  precision;

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
    return NULL;

  num_images = compose_dsc[compose_idx].num_images;

  /* Check that at least one image or one drawable is provided */
  first_object = -1;
  for (i = 0; i < num_images; i++)
    {
      if (inputs[i].is_object)
        {
          first_object = i;
          break;
        }
    }

  if (first_object == -1)
    {
      g_message (_("At least one image is needed to compose"));
      return NULL;
    }

  /* Check image sizes */
  if (compose_by_drawable)
    {
      LigmaImage *first_image = ligma_item_get_image (inputs[first_object].comp.object);

      width  = ligma_drawable_get_width  (inputs[first_object].comp.object);
      height = ligma_drawable_get_height (inputs[first_object].comp.object);

      precision = ligma_image_get_precision (first_image);

      for (j = first_object + 1; j < num_images; j++)
        {
          if (inputs[j].is_object)
            {
              if ((width  != ligma_drawable_get_width  (inputs[j].comp.object)) ||
                  (height != ligma_drawable_get_height (inputs[j].comp.object)))
                {
                  g_message (_("Drawables have different size"));
                  return NULL;
                }
            }
        }

      for (j = 0; j < num_images; j++)
        {
          if (inputs[j].is_object)
            buffer_src[j] = ligma_drawable_get_buffer (inputs[j].comp.object);
          else
            buffer_src[j] = NULL;
        }
    }
  else    /* Compose by image */
    {
      width  = ligma_image_get_width  (inputs[first_object].comp.object);
      height = ligma_image_get_height (inputs[first_object].comp.object);

      precision = ligma_image_get_precision (inputs[first_object].comp.object);

      for (j = first_object + 1; j < num_images; j++)
        {
          if (inputs[j].is_object)
            {
              if ((width  != ligma_image_get_width  (inputs[j].comp.object)) ||
                  (height != ligma_image_get_height (inputs[j].comp.object)))
                {
                  g_message (_("Images have different size"));
                  return NULL;
                }
            }
        }

      /* Get first layer/drawable for all input images */
      for (j = 0; j < num_images; j++)
        {
          if (inputs[j].is_object)
            {
              LigmaLayer **layers;

              /* Get first layer of image */
              layers = ligma_image_get_layers (inputs[j].comp.object, &num_layers);

              if (! layers)
                {
                  g_message (_("Error in getting layer IDs"));
                  return NULL;
                }

              /* Get drawable for layer */
              buffer_src[j] = ligma_drawable_get_buffer (LIGMA_DRAWABLE (layers[0]));
              g_free (layers);
            }
        }
    }

  /* Unless recomposing, create new image */
  if (composevals.do_recompose)
    {
      layer_dst  = composevals.source_layer;
      image_dst  = ligma_item_get_image (LIGMA_ITEM (layer_dst));
      buffer_dst = ligma_drawable_get_shadow_buffer (LIGMA_DRAWABLE (layer_dst));
    }
  else
    {
      gdtype_dst = ((babl_model (compose_dsc[compose_idx].babl_model) == babl_model ("RGBA")) ?
                    LIGMA_RGBA_IMAGE : LIGMA_RGB_IMAGE);

      image_dst = create_new_image (g_file_new_for_path (compose_dsc[compose_idx].filename),
                                    width, height, gdtype_dst, precision,
                                    &layer_dst, &buffer_dst);
    }

  if (! compose_by_drawable)
    {
      gdouble  xres, yres;

      ligma_image_get_resolution (inputs[first_object].comp.object, &xres, &yres);
      ligma_image_set_resolution (image_dst, xres, yres);
    }

  perform_composition (compose_dsc[compose_idx],
                       buffer_src,
                       buffer_dst,
                       inputs,
                       num_images);

  ligma_progress_update (1.0);

  for (j = 0; j < num_images; j++)
    {
      if (inputs[j].is_object)
        g_object_unref (buffer_src[j]);
    }

  g_object_unref (buffer_dst);

  if (composevals.do_recompose)
    ligma_drawable_merge_shadow (LIGMA_DRAWABLE (layer_dst), TRUE);

  ligma_drawable_update (LIGMA_DRAWABLE (layer_dst), 0, 0,
                        ligma_drawable_get_width  (LIGMA_DRAWABLE (layer_dst)),
                        ligma_drawable_get_height (LIGMA_DRAWABLE (layer_dst)));

  return image_dst;
}


/* Create an image. Sets layer_ID, drawable and rgn. Returns image */
static LigmaImage *
create_new_image (GFile          *file,
                  guint           width,
                  guint           height,
                  LigmaImageType   gdtype,
                  LigmaPrecision   precision,
                  LigmaLayer     **layer,
                  GeglBuffer    **buffer)
{
  LigmaImage         *image;
  LigmaImageBaseType  gitype;

  if ((gdtype == LIGMA_GRAY_IMAGE) || (gdtype == LIGMA_GRAYA_IMAGE))
    gitype = LIGMA_GRAY;
  else if ((gdtype == LIGMA_INDEXED_IMAGE) || (gdtype == LIGMA_INDEXEDA_IMAGE))
    gitype = LIGMA_INDEXED;
  else
    gitype = LIGMA_RGB;

  image = ligma_image_new_with_precision (width, height, gitype, precision);

  ligma_image_undo_disable (image);
  ligma_image_set_file (image, file);

  *layer = ligma_layer_new (image, _("Background"), width, height,
                           gdtype,
                           100,
                           ligma_image_get_default_new_layer_mode (image));
  ligma_image_insert_layer (image, *layer, NULL, 0);

  *buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (*layer));

  return image;
}


static gboolean
compose_dialog (const gchar  *compose_type,
                LigmaDrawable *drawable)
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
  LigmaLayer   **layer_list;
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
  composeint.width  = ligma_drawable_get_width  (drawable);
  composeint.height = ligma_drawable_get_height (drawable);

  ligma_ui_init (PLUG_IN_BINARY);

  layer_list = ligma_image_get_layers (ligma_item_get_image (LIGMA_ITEM (drawable)),
                                      &nlayers);

  dialog = ligma_dialog_new (_("Compose"), PLUG_IN_ROLE,
                            NULL, 0,
                            ligma_standard_help_func, COMPOSE_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (check_response),
                    NULL);

  ligma_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  /* Compose type combo */

  frame = ligma_frame_new (_("Compose Channels"));
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

  combo = g_object_new (LIGMA_TYPE_INT_COMBO_BOX, NULL);
  for (j = 0; j < G_N_ELEMENTS (compose_dsc); j++)
    {
      gchar *label = g_strdup (gettext (compose_dsc[j].compose_type));
      gchar *l;

      for (l = label; *l; l++)
        if (*l == '-' || *l == '_')
          *l = ' ';

      ligma_int_combo_box_append (LIGMA_INT_COMBO_BOX (combo),
                                 LIGMA_INT_STORE_LABEL, label,
                                 LIGMA_INT_STORE_VALUE, j,
                                 -1);
      g_free (label);
    }

  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

  /* Channel representation grid */

  frame = ligma_frame_new (_("Channel Representations"));
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
      GtkWidget    *image;
      GtkWidget    *label;
      GtkWidget    *combo;
      GtkWidget    *scale;
      GtkTreeIter   iter;
      GtkTreeModel *model;

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
          composeint.selected[j].comp.object = layer_list[j];
        }
      else
        {
          composeint.selected[j].comp.object = drawable;
        }

      composeint.selected[j].is_object = TRUE;

      combo = ligma_drawable_combo_box_new (check_gray, NULL, NULL);
      composeint.channel_menu[j] = combo;

      model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
      gtk_list_store_append (GTK_LIST_STORE (model), &iter);
      gtk_list_store_set (GTK_LIST_STORE (model),   &iter,
                          LIGMA_INT_STORE_VALUE,     -1,
                          LIGMA_INT_STORE_LABEL,     _("Mask value"),
                          LIGMA_INT_STORE_ICON_NAME, LIGMA_ICON_CHANNEL_GRAY,
                          -1);
      gtk_grid_attach (GTK_GRID (grid), combo, 1, j, 1, 1);
                        // GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
      gtk_widget_show (combo);

      gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

      scale = ligma_color_scale_entry_new (NULL, 255.0, 0.0, 255.0, 0);
      gtk_grid_attach (GTK_GRID (grid), scale, 2, j, 3, 1);
      gtk_widget_show (scale);
      composeint.scales[j] = scale;

      gtk_widget_set_sensitive (scale, FALSE);

      g_signal_connect (scale, "value-changed",
                        G_CALLBACK (scale_callback),
                        &composeint.selected[j]);

      /* This has to be connected last otherwise it will emit before
       * combo_callback has any scale and spinbutton to work with
       */
      ligma_int_combo_box_connect (LIGMA_INT_COMBO_BOX (combo),
                                  ligma_item_get_id (composeint.selected[j].comp.object),
                                  G_CALLBACK (combo_callback),
                                  GINT_TO_POINTER (j), NULL);
    }

  g_free (layer_list);

  /* Calls the combo callback and sets icons, labels and sensitivity */
  ligma_int_combo_box_connect (LIGMA_INT_COMBO_BOX (combo),
                              composeint.compose_idx,
                              G_CALLBACK (type_combo_callback),
                              NULL, NULL);

  gtk_widget_show (dialog);

  run = (ligma_dialog_run (LIGMA_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  if (run)
    {
      gint j;

      for (j = 0; j < MAX_COMPOSE_IMAGES; j++)
        {
          composevals.inputs[j].is_object = composeint.selected[j].is_object;

          if (composevals.inputs[j].is_object)
            composevals.inputs[j].comp.object = composeint.selected[j].comp.object;
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
check_gray (LigmaImage *image,
            LigmaItem  *drawable,
            gpointer   data)

{
  return ((ligma_image_get_base_type (image) == LIGMA_GRAY) &&
          (ligma_image_get_width  (image) == composeint.width) &&
          (ligma_image_get_height (image) == composeint.height));
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
            if (composeint.selected[i].is_object)
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
combo_callback (LigmaIntComboBox *widget,
                gpointer         data)
{
  gint id;
  gint n;

  ligma_int_combo_box_get_active (LIGMA_INT_COMBO_BOX (widget), &id);
  n = GPOINTER_TO_INT (data);

  if (id == -1)
    {
      gtk_widget_set_sensitive (composeint.scales[n], TRUE);

      composeint.selected[n].is_object = FALSE;
      composeint.selected[n].comp.val  =
        ligma_label_spin_get_value (LIGMA_LABEL_SPIN (composeint.scales[n]));
    }
  else
    {
      gtk_widget_set_sensitive (composeint.scales[n], FALSE);

      composeint.selected[n].is_object   = TRUE;
      composeint.selected[n].comp.object = ligma_drawable_get_by_id (id);
    }
}

static void
scale_callback (LigmaLabelSpin *scale,
                ComposeInput  *input)
{
  input->comp.val = ligma_label_spin_get_value (scale);
}

static void
type_combo_callback (LigmaIntComboBox *combo,
                     gpointer         data)
{
  if (ligma_int_combo_box_get_active (combo, &composeint.compose_idx))
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

      scale4 = combo4 && !composeint.selected[3].is_object;
      gtk_widget_set_sensitive (composeint.scales[3], scale4);
    }
}
