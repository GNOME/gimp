/* xjt.c
 *
 * XJT (JPEG-TAR fileformat) loading and saving file filter for the GIMP
 *  -hof (Wolfgang Hofer)
 *
 * This filter requires UNIX tar and the "jpeglib" Library to run.
 * For optional further compression you also should install
 *  gzip and bzip2 compression Programs.
 *
 * IMPORTANT NOTE:
 *   This plugin needs GIMP 1.1.18 or newer versions of the GIMP-core to run.
 */

/* GIMP - The GNU Image Manipulation Program
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

/* TODO:
 *  - support user units
 *  - show continous progress while loading/saving
 */

/* revision history:
 * version 1.3.14a; 2003/06/03  hof: bugfix: using setlocale independent float conversion procedures
 *                                   p_my_ascii_strtod (g_ascii_strtod()) and g_ascii_formatd()
 * version 1.1.18a; 2000/03/07  hof: tattoo_state
 * version 1.1.16a; 2000/02/04  hof: load paths continued, load tattos, load/save unit
 * version 1.1.15b; 2000/01/28  hof: save/load paths  (load is not activated PDB-bug)
 *                                   continued save/load parasites,
 *                                   replaced static buffers by dynamic allocated memory (goodbye to sprintf)
 * version 1.1.15a; 2000/01/23  hof: NLS_macros, save/load parasites, \" and \n characters in names
 *                                   use G_DIR_SEPARATOR (but you still need UNIX tar to run this plugin)
 *                                   older gimp releases (prior to 1.1.15) are not supported any more.
 * version 1.02.00; 1999/03/16  hof: - save layer/channel Tattoos
 *                                   - load/save image resolution added
 *                                   - tolerate unknown properties with warnings
 * version 1.01.00; 1998/11/22  hof: added load/save of guides
 *                                   (you need gimp 1.1 to use this feature)
 * version 1.00.00; 1998/10/29  hof: 1.st (pre) release
 */

#include "config.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>

#ifdef __GNUC__
#warning GIMP_DISABLE_DEPRECATED
#endif
#undef GIMP_DISABLE_DEPRECATED

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#ifdef G_OS_WIN32
#include <process.h> /* getpid() */
#endif

/* XJT includes */
#include "xjpeg.h"

#define LOAD_PROC      "file-xjt-load"
#define SAVE_PROC      "file-xjt-save"

#ifdef _MSC_VER
typedef int pid_t;
#endif

#define GIMP_XJ_IMAGE  "GIMP_XJ_IMAGE"

#define SCALE_WIDTH 125

#define XJT_ORIENTATION_HORIZONTAL 0
#define XJT_ORIENTATION_VERTICAL   1

       gint     xjt_debug = FALSE;
static pid_t    g_pid;
static gchar   *global_parasite_prop_lines = NULL;
static gint     global_parasite_id = 0;


/* PROPERTY enums
 *  (0-21 are ident with PropType values as used in xcf.c
 *   the rest was added for xjt
 */
typedef enum
{
  PROP_END = 0,
  PROP_COLORMAP = 1,
  PROP_ACTIVE_LAYER = 2,
  PROP_ACTIVE_CHANNEL = 3,
  PROP_SELECTION = 4,
  PROP_FLOATING_SELECTION = 5,
  PROP_OPACITY = 6,
  PROP_MODE = 7,
  PROP_VISIBLE = 8,
  PROP_LINKED = 9,
  PROP_LOCK_ALPHA = 10,
  PROP_APPLY_MASK = 11,
  PROP_EDIT_MASK = 12,
  PROP_SHOW_MASK = 13,
  PROP_SHOW_MASKED = 14,
  PROP_OFFSETS = 15,
  PROP_COLOR = 16,
  PROP_COMPRESSION = 17,
  PROP_GUIDES = 18,
  PROP_RESOLUTION = 19,
  PROP_TATTOO = 20,
  PROP_PARASITES = 21,
  PROP_UNIT = 22,
  PROP_PATHS = 23,
  PROP_USER_UNIT = 24,

  PROP_TATTOO_STATE = 85,
  PROP_PATH_LOCKED = 86,
  PROP_PATH_CURRENT = 87,
  PROP_PATH_TYPE = 88,
  PROP_PATH_POINTS = 89,
  PROP_NAME = 90,
  PROP_DIMENSION = 91,
  PROP_TYPE = 92,
  PROP_VERSION = 93,
  PROP_FLOATING_ATTACHED = 94,
  PROP_PARASITE_FLAGS = 95,
  PROP_GIMP_VERSION = 96,

  PROP_SYNTAX_ERROR = 99
} t_proptype;

typedef enum
{
  PTYP_NOT_SUPPORTED = 0,
  PTYP_BOOLEAN = 1,
  PTYP_INT = 2,
  PTYP_FLT = 3,
  PTYP_STRING = 4,
  PTYP_2xINT = 5,
  PTYP_3xINT = 6,
  PTYP_2xFLT = 7,
  PTYP_3xFLT = 8,
  PTYP_FLIST = 9
} t_param_typ;

typedef enum
{
  XJT_IMAGE_PARASITE = 0,
  XJT_LAYER_PARASITE = 1,
  XJT_LAYER_MASK_PARASITE = 2,
  XJT_CHANNEL_PARASITE = 3
} t_parasitetype;

typedef enum
{
  XJT_RGB = 0,
  XJT_GRAY = 1
  /* XJT_INDEXED = 2 */  /* xjt does not support indexed images */
} XJTImageBaseType;

typedef enum
{
  XJT_PATHTYPE_UNDEF = 0,
  XJT_PATHTYPE_BEZIER = 1
} XJTPathType;

typedef enum
{
  XJT_UNIT_PIXEL = 0,
  XJT_UNIT_INCH  = 1,
  XJT_UNIT_MM    = 2,
  XJT_UNIT_POINT = 3,
  XJT_UNIT_PICA  = 4
} XJTUnitType;

typedef enum
{
  XJT_NORMAL_MODE = 0,
  XJT_DISSOLVE_MODE = 1,
  XJT_BEHIND_MODE = 2,
  XJT_MULTIPLY_MODE = 3,
  XJT_SCREEN_MODE = 4,
  XJT_OVERLAY_MODE = 5,
  XJT_DIFFERENCE_MODE = 6,
  XJT_ADDITION_MODE = 7,
  XJT_SUBTRACT_MODE = 8,
  XJT_DARKEN_ONLY_MODE = 9,
  XJT_LIGHTEN_ONLY_MODE = 10,
  XJT_HUE_MODE = 11,
  XJT_SATURATION_MODE = 12,
  XJT_COLOR_MODE = 13,
  XJT_VALUE_MODE = 14,
  XJT_DIVIDE_MODE = 15,
  XJT_DODGE_MODE = 16,
  XJT_BURN_MODE = 17,
  XJT_HARDLIGHT_MODE = 18,
  XJT_SOFTLIGHT_MODE = 19,
  XJT_GRAIN_EXTRACT_MODE = 20,
  XJT_GRAIN_MERGE_MODE = 21,
  XJT_COLOR_ERASE_MODE = 22
} XJTLayerModeEffects;

typedef struct
{
  t_proptype   prop_id;
  gchar       *prop_mnemonic;
  t_param_typ  param_typ;
  gdouble      default_val1;
  gdouble      default_val2;
  gdouble      default_val3;
} t_prop_table;


typedef struct
{
  gint32   int_val1;
  gint32   int_val2;
  gint32   int_val3;
  gdouble  flt_val1;
  gdouble  flt_val2;
  gdouble  flt_val3;
  gint32   num_fvals;
  gdouble *flt_val_list;
  gchar   *string_val;
}  t_param_prop;

typedef struct
{
  t_parasitetype  parasite_type;
  gint32          parasite_id;
  gint32          flags;
  gchar          *name;
  gint32          obj_pos;
  void           *next;
} t_parasite_props;

typedef struct
{
  gint32          path_type;
  gint32          path_locked;
  gint32          path_closed;
  gint32          current_flag;
  gint32          tattoo;
  gchar          *name;
  gint32          num_points;
  gdouble        *path_points;
  void           *next;
} t_path_props;

typedef struct
{
  gint     active_channel;
  gint     selection;
  gint     floating_attached;
  gdouble  opacity;
  gint     visible;
  gint     show_masked;
  guchar   color_r;
  guchar   color_g;
  guchar   color_b;
  gint32   offx;            /* do channels have offset ?? */
  gint32   offy;
  gchar   *name;
  gint32   tattoo;

  gint     channel_pos;
  void    *next;
}  t_channel_props;

typedef struct
{
  gint     active_layer;
  gint     floating_selection;
  gint     floating_attached;
  gdouble  opacity;
  gint32   mode;
  gint     visible;
  gint     linked;
  gint     lock_alpha;
  gint     apply_mask;
  gint     edit_mask;
  gint     show_mask;
  gint32   offx;
  gint32   offy;
  gchar   *name;
  gint32   tattoo;

  gint     layer_pos;
  gint     has_alpha;
  void    *next;
} t_layer_props;


typedef struct
{
  gint32   position;
  gint8    orientation;
  void    *next;
}  t_guide_props;


typedef struct
{
  gchar            *version;
  gint              gimp_major_version;
  gint              gimp_minor_version;
  gint              gimp_micro_version;
  GimpImageBaseType        image_type;
  gint              image_width;
  gint              image_height;
  gfloat            xresolution;
  gfloat            yresolution;
  GimpUnit          unit;
  gint32            tattoo;
  gint32            tattoo_state;
  gint              n_layers;
  gint              n_channels;
  t_layer_props    *layer_props;
  t_channel_props  *channel_props;
  t_channel_props  *mask_props;
  t_guide_props    *guide_props;
  t_parasite_props *parasite_props;
  t_path_props     *path_props;
} t_image_props;


#define PROP_TABLE_ENTRIES 35
t_prop_table g_prop_table[PROP_TABLE_ENTRIES] = {
  /* t_proptype              mnemonic   t_paramtyp             default values */
  { PROP_END,                   "*",      PTYP_NOT_SUPPORTED,       0.0,  0.0,  0.0 } ,
  { PROP_COLORMAP,              "*",      PTYP_NOT_SUPPORTED,       0.0,  0.0,  0.0 } ,
  { PROP_ACTIVE_LAYER,          "acl",    PTYP_BOOLEAN,             0.0,  0.0,  0.0 } ,
  { PROP_ACTIVE_CHANNEL,        "acc",    PTYP_BOOLEAN,             0.0,  0.0,  0.0 } ,
  { PROP_SELECTION,             "sel",    PTYP_BOOLEAN,             0.0,  0.0,  0.0 } ,
  { PROP_FLOATING_SELECTION,    "fsl",    PTYP_BOOLEAN,             0.0,  0.0,  0.0 } ,
  { PROP_OPACITY,               "op",     PTYP_FLT,               100.0,  0.0,  0.0 } ,
  { PROP_MODE,                  "md",     PTYP_INT,                 0.0,  0.0,  0.0 } ,
  { PROP_VISIBLE,               "iv",     PTYP_BOOLEAN,             0.0,  0.0,  0.0 } ,
  { PROP_LINKED,                "ln",     PTYP_BOOLEAN,             0.0,  0.0,  0.0 } ,
  { PROP_LOCK_ALPHA,            "pt",     PTYP_BOOLEAN,             0.0,  0.0,  0.0 } ,
  { PROP_APPLY_MASK,            "aml",    PTYP_BOOLEAN,             0.0,  0.0,  0.0 } ,
  { PROP_EDIT_MASK,             "eml",    PTYP_BOOLEAN,             0.0,  0.0,  0.0 } ,
  { PROP_SHOW_MASK,             "sml",    PTYP_BOOLEAN,             0.0,  0.0,  0.0 } ,
  { PROP_SHOW_MASKED,           "smc",    PTYP_BOOLEAN,             0.0,  0.0,  0.0 } ,
  { PROP_OFFSETS,               "o",      PTYP_2xINT,               0.0,  0.0,  0.0 } ,
  { PROP_COLOR,                 "c",      PTYP_3xINT,               0.0,  0.0,  0.0 } ,
  { PROP_COMPRESSION,           "*",      PTYP_NOT_SUPPORTED,       0.0,  0.0,  0.0 } ,
  { PROP_GUIDES,                "g",      PTYP_2xINT,               0.0,  0.0,  0.0 } ,
  { PROP_RESOLUTION,            "res",    PTYP_2xFLT,              72.0, 72.0,  0.0 } ,
  { PROP_UNIT,                  "unt",    PTYP_INT,                 0.0,  0.0,  0.0 } ,
  { PROP_TATTOO,                "tto",    PTYP_INT,                -1.0,  0.0,  0.0 } ,
  { PROP_TATTOO_STATE,          "tts",    PTYP_INT,                -1.0,  0.0,  0.0 } ,
  { PROP_PARASITES,             "pte",    PTYP_INT,                 0.0,  0.0,  0.0 } ,

  { PROP_PATH_POINTS,           "php",    PTYP_FLIST,               0.0,  0.0,  0.0 } ,
  { PROP_PATH_TYPE,             "pht",    PTYP_INT,                 1.0,  0.0,  0.0 } ,
  { PROP_PATH_CURRENT,          "pha",    PTYP_INT,                 0.0,  0.0,  0.0 } ,
  { PROP_PATH_LOCKED,           "phl",    PTYP_BOOLEAN,             0.0,  0.0,  0.0 } ,
  { PROP_PARASITE_FLAGS,        "ptf",    PTYP_INT,                 1.0,  0.0,  0.0 } ,
  { PROP_FLOATING_ATTACHED,     "fa",     PTYP_BOOLEAN,             0.0,  0.0,  0.0 } ,
  { PROP_NAME,                  "n",      PTYP_STRING,              0.0,  0.0,  0.0 } ,
  { PROP_DIMENSION,             "w/h",    PTYP_2xINT,               0.0,  0.0,  0.0 } ,
  { PROP_TYPE,                  "typ",    PTYP_INT,                 0.0,  0.0,  0.0 } ,
  { PROP_GIMP_VERSION,          "gimp",   PTYP_3xINT,               0.0,  0.0,  0.0 } ,
  { PROP_VERSION,               "ver",    PTYP_STRING,              0.0,  0.0,  0.0 }
};


/* Declare local functions.
 */
static void      query            (void);
static void      run              (const gchar      *name,
                                   gint              nparams,
                                   const GimpParam  *param,
                                   gint             *nreturn_vals,
                                   GimpParam       **return_vals);

static gint32    load_xjt_image   (const gchar      *filename);
static gint      save_xjt_image   (const gchar      *filename,
                                   gint32            image_ID,
                                   gint32            drawable_ID);

static gboolean  save_dialog      (void);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static t_JpegSaveVals jsvals =
{
  0.75,  /*  quality  */
  0.0,   /*  smoothing  */
  1,     /*  optimize  */
  0      /*  clr_transparent */
};

static gint
p_invert(gint value)
{
  return (value) ? FALSE : TRUE;
}

static
int p_system(const gchar *cmd)
{
  int l_rc;
  int l_rc2;

  if(xjt_debug) printf("CMD: %s\n", cmd);

  l_rc = system(cmd);
  if(l_rc != 0)
  {
     /* Shift 8 Bits gets Retcode of the executed Program */
     l_rc2 = l_rc >> 8;
     g_printerr ("ERROR system: %s\nreturncodes %d %d", cmd, l_rc, l_rc2);
     return -1;
  }
  return 0;
}

MAIN ()

static void
query (void)
{
  static const GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,  "run-mode",     "Interactive, non-interactive" },
    { GIMP_PDB_STRING, "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING, "raw-filename", "The name of the file to load" },
  };
  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" },
  };

  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",        "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",           "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",        "is ignored" },
    { GIMP_PDB_STRING,   "filename",        "The name of the file to save the image in" },
    { GIMP_PDB_STRING,   "raw-filename",    "The name of the file to save the image in" },
    { GIMP_PDB_FLOAT,    "quality",         "Quality of saved image (0 <= quality <= 1)" },
    { GIMP_PDB_FLOAT,    "smoothing",       "Smoothing factor for saved image (0 <= smoothing <= 1)" },
    { GIMP_PDB_INT32,    "optimize",        "Optimization of entropy encoding parameters" },
    { GIMP_PDB_INT32,    "clr-transparent", "set all full-transparent pixels to 0" },
  };

  gimp_install_procedure (LOAD_PROC,
                          "loads files of the jpeg-tar file format",
                          "loads files of the jpeg-tar file format",
                          "Wolfgang Hofer",
                          "Wolfgang Hofer",
                          "2000-Mar-07",
                          N_("GIMP compressed XJT image"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_magic_load_handler (LOAD_PROC,
                                    "xjt,xjtgz,xjtbz2",
                                    "",
                                    "");

  gimp_install_procedure (SAVE_PROC,
                          "saves files in the jpeg-tar file format",
                          "saves files in the jpeg-tar file format",
                          "Wolfgang Hofer",
                          "Wolfgang Hofer",
                          "2000-Mar-07",
                          N_("GIMP compressed XJT image"),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_register_save_handler (SAVE_PROC, "xjt,xjtgz,xjtbz2", "");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam values[2];
  GimpRunMode  run_mode;
  GimpPDBStatusType   status = GIMP_PDB_SUCCESS;
  gint32        image_ID;
  gchar        *l_env;

  g_pid = getpid ();
  xjt_debug = FALSE;

  INIT_I18N ();

  l_env = getenv("XJT_DEBUG");
  if(l_env != NULL)
    {
      if((*l_env != 'n') && (*l_env != 'N')) xjt_debug = TRUE;
    }

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, LOAD_PROC) == 0)
    {
      image_ID = load_xjt_image (param[1].data.d_string);

      if (image_ID != -1)
        {
          *nreturn_vals = 2;
          values[1].type         = GIMP_PDB_IMAGE;
          values[1].data.d_image = image_ID;
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }
  else if (strcmp (name, SAVE_PROC) == 0)
    {
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          /*  Possibly retrieve data  */
          gimp_get_data (SAVE_PROC, &jsvals);

          /*  First acquire information with a dialog  */
          if (! save_dialog ())
            {
              status = GIMP_PDB_CANCEL;
            }
          break;

        case GIMP_RUN_NONINTERACTIVE:
          /*  Make sure all the arguments are there!  */
          if (nparams != 8)
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
          else
            {
              jsvals.quality         = param[5].data.d_float;
              jsvals.smoothing       = param[6].data.d_float;
              jsvals.optimize        = param[7].data.d_int32;
              jsvals.clr_transparent = param[8].data.d_int32;

              if (jsvals.quality < 0.0 || jsvals.quality > 1.0)
                {
                  status = GIMP_PDB_CALLING_ERROR;
                }
              else if (jsvals.smoothing < 0.0 || jsvals.smoothing > 1.0)
                {
                  status = GIMP_PDB_CALLING_ERROR;
                }
            }
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          /*  Possibly retrieve data  */
          gimp_get_data (SAVE_PROC, &jsvals);
          break;

        default:
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          if (save_xjt_image (param[3].data.d_string,
                              param[1].data.d_int32,
                              param[2].data.d_int32) <0)
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
          else
            {
              /*  Store mvals data  */
              gimp_set_data (SAVE_PROC, &jsvals, sizeof (t_JpegSaveVals));
            }
        }
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}


/* ------------------------
 * p_my_ascii_strtod
 * ------------------------
 * call  g_ascii_strtod
 * with XJT private modification:
 *  g_ascii_strtod accepts both "." and "," as radix character (decimalpoint)
 *  for float numbers (at least for GERMAN LANG)
 *  because XJT PRP files have comma (,) seperated lists of float numbers this cant be accepted here.
 *  the private version substitutes a '\0' character for the next comma
 *  before calling g_ascii_strtod, and puts back the comma after the call
 */
static gdouble
p_my_ascii_strtod (gchar  *nptr,
                   gchar **endptr)
{
   gint ii;
   gint ic;
   gdouble l_rc;

   /* check for comma (is a terminating character for the NON locale float string)
    */
   ii=0;
   ic = -1;
   while (nptr)
   {
     if (nptr[ii] == ',')
     {
       ic = ii;
       nptr[ii] = '\0';  /* temporary use 0 as terminator for the call of g_ascii_strtod */
       break;
     }
     if ((nptr[ii] == '\0')
     ||  (nptr[ii] == ' ')
     ||  (nptr[ii] == '\n'))
     {
       break;
     }
     ii++;
   }

   l_rc = g_ascii_strtod(nptr, endptr);

   if (ic >= 0)
   {
       nptr[ii] = ',';   /* restore the comma */
   }

   return l_rc;
}  /* end p_my_ascii_strtod */

/* -- type transformer routines XJT -- GIMP internal enums ----------------- */

static gint32
p_to_GimpOrientation(gint32 orientation)
{
  return (orientation == XJT_ORIENTATION_VERTICAL)
    ? GIMP_ORIENTATION_VERTICAL : GIMP_ORIENTATION_HORIZONTAL;
}
static gint32
p_to_XJTOrientation(gint32 orientation)
{
  return (orientation == GIMP_ORIENTATION_VERTICAL)
    ? XJT_ORIENTATION_VERTICAL : XJT_ORIENTATION_HORIZONTAL;
}

static GimpLayerModeEffects
p_to_GimpLayerModeEffects(XJTLayerModeEffects intype)
{
  switch(intype)
  {
    case XJT_NORMAL_MODE:         return GIMP_NORMAL_MODE;
    case XJT_DISSOLVE_MODE:       return GIMP_DISSOLVE_MODE;
    case XJT_BEHIND_MODE:         return GIMP_BEHIND_MODE;
    case XJT_MULTIPLY_MODE:       return GIMP_MULTIPLY_MODE;
    case XJT_SCREEN_MODE:         return GIMP_SCREEN_MODE;
    case XJT_OVERLAY_MODE:        return GIMP_OVERLAY_MODE;
    case XJT_DIFFERENCE_MODE:     return GIMP_DIFFERENCE_MODE;
    case XJT_ADDITION_MODE:       return GIMP_ADDITION_MODE;
    case XJT_SUBTRACT_MODE:       return GIMP_SUBTRACT_MODE;
    case XJT_DARKEN_ONLY_MODE:    return GIMP_DARKEN_ONLY_MODE;
    case XJT_LIGHTEN_ONLY_MODE:   return GIMP_LIGHTEN_ONLY_MODE;
    case XJT_HUE_MODE:            return GIMP_HUE_MODE;
    case XJT_SATURATION_MODE:     return GIMP_SATURATION_MODE;
    case XJT_COLOR_MODE:          return GIMP_COLOR_MODE;
    case XJT_VALUE_MODE:          return GIMP_VALUE_MODE;
    case XJT_DIVIDE_MODE:         return GIMP_DIVIDE_MODE;
    case XJT_DODGE_MODE:          return GIMP_DODGE_MODE;
    case XJT_BURN_MODE:           return GIMP_BURN_MODE;
    case XJT_HARDLIGHT_MODE:      return GIMP_HARDLIGHT_MODE;
    case XJT_SOFTLIGHT_MODE:      return GIMP_SOFTLIGHT_MODE;
    case XJT_GRAIN_EXTRACT_MODE:  return GIMP_GRAIN_EXTRACT_MODE;
    case XJT_GRAIN_MERGE_MODE:    return GIMP_GRAIN_MERGE_MODE;
    case XJT_COLOR_ERASE_MODE:    return GIMP_COLOR_ERASE_MODE;
  }
  printf (_("XJT file contains unknown layermode %d"), (int)intype);
  if((gint32)intype > (gint32)XJT_DIVIDE_MODE)
  {
    return (GimpLayerModeEffects)intype;
  }
  return GIMP_NORMAL_MODE;
}

static XJTLayerModeEffects
p_to_XJTLayerModeEffects(GimpLayerModeEffects intype)
{
  switch(intype)
  {
    case GIMP_NORMAL_MODE:         return XJT_NORMAL_MODE;
    case GIMP_DISSOLVE_MODE:       return XJT_DISSOLVE_MODE;
    case GIMP_BEHIND_MODE:         return XJT_BEHIND_MODE;
    case GIMP_MULTIPLY_MODE:       return XJT_MULTIPLY_MODE;
    case GIMP_SCREEN_MODE:         return XJT_SCREEN_MODE;
    case GIMP_OVERLAY_MODE:        return XJT_OVERLAY_MODE;
    case GIMP_DIFFERENCE_MODE:     return XJT_DIFFERENCE_MODE;
    case GIMP_ADDITION_MODE:       return XJT_ADDITION_MODE;
    case GIMP_SUBTRACT_MODE:       return XJT_SUBTRACT_MODE;
    case GIMP_DARKEN_ONLY_MODE:    return XJT_DARKEN_ONLY_MODE;
    case GIMP_LIGHTEN_ONLY_MODE:   return XJT_LIGHTEN_ONLY_MODE;
    case GIMP_HUE_MODE:            return XJT_HUE_MODE;
    case GIMP_SATURATION_MODE:     return XJT_SATURATION_MODE;
    case GIMP_COLOR_MODE:          return XJT_COLOR_MODE;
    case GIMP_VALUE_MODE:          return XJT_VALUE_MODE;
    case GIMP_DIVIDE_MODE:         return XJT_DIVIDE_MODE;
    case GIMP_DODGE_MODE:          return XJT_DODGE_MODE;
    case GIMP_BURN_MODE:           return XJT_BURN_MODE;
    case GIMP_HARDLIGHT_MODE:      return XJT_HARDLIGHT_MODE;
    case GIMP_SOFTLIGHT_MODE:      return XJT_SOFTLIGHT_MODE;
    case GIMP_GRAIN_EXTRACT_MODE:  return XJT_GRAIN_EXTRACT_MODE;
    case GIMP_GRAIN_MERGE_MODE:    return XJT_GRAIN_MERGE_MODE;
    case GIMP_COLOR_ERASE_MODE:    return XJT_COLOR_ERASE_MODE;
  }
  printf (_("Warning: unsupported layermode %d saved to XJT"), (int)intype);
  if ((gint32)intype > (gint32)XJT_DIVIDE_MODE)
  {
    return (XJTLayerModeEffects)intype;
  }
  return XJT_NORMAL_MODE;
}

static gint32
p_to_GimpPathType(XJTPathType intype)
{
  switch(intype)
  {
    case XJT_PATHTYPE_UNDEF:      return 0;
    case XJT_PATHTYPE_BEZIER:     return 1;
  }
  printf (_("XJT file contains unknown pathtype %d"), (int)intype);
  if ((gint32)intype > (gint32)XJT_PATHTYPE_BEZIER)
  {
    return (gint32)intype;
  }
  return 0;
}

static XJTPathType
p_to_XJTPathType(gint32 intype)
{
  switch(intype)
  {
    case 0:      return XJT_PATHTYPE_UNDEF;
    case 1:      return XJT_PATHTYPE_BEZIER;
  }
  printf (_("Warning: unsupported pathtype %d saved to XJT"), (int)intype);
  if((gint32)intype > (gint32)XJT_PATHTYPE_BEZIER)
  {
    return((XJTPathType)intype);
  }
  return(XJT_PATHTYPE_UNDEF);
}

static GimpUnit
p_to_GimpUnit(XJTUnitType intype)
{
  switch(intype)
  {
    case XJT_UNIT_PIXEL:      return(GIMP_UNIT_PIXEL);
    case XJT_UNIT_INCH:       return(GIMP_UNIT_INCH);
    case XJT_UNIT_MM:         return(GIMP_UNIT_MM);
    case XJT_UNIT_POINT:      return(GIMP_UNIT_POINT);
    case XJT_UNIT_PICA:       return(GIMP_UNIT_PICA);
  }
  printf (_("XJT file contains unknown unittype %d"), (int)intype);
  if((gint32)intype > (gint32)XJT_UNIT_PICA)
  {
    return((GimpUnit)intype);
  }
  return(GIMP_UNIT_PIXEL);
}

static XJTUnitType
p_to_XJTUnitType(GimpUnit intype)
{
  switch(intype)
  {
    case GIMP_UNIT_PIXEL:      return(XJT_UNIT_PIXEL);
    case GIMP_UNIT_INCH:       return(XJT_UNIT_INCH);
    case GIMP_UNIT_MM:         return(XJT_UNIT_MM);
    case GIMP_UNIT_POINT:      return(XJT_UNIT_POINT);
    case GIMP_UNIT_PICA:       return(XJT_UNIT_PICA);
    case GIMP_UNIT_END:        break;
    case GIMP_UNIT_PERCENT:    break;
  }
  printf (_("Warning: unsupported unittype %d saved to XJT"), (int)intype);
  if((gint32)intype > (gint32)XJT_UNIT_PICA)
  {
    return((XJTUnitType)intype);
  }
  return(XJT_UNIT_PIXEL);
}

/* ---------------------- SAVE DIALOG procedures  -------------------------- */

static gboolean
save_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *table;
  GtkWidget *toggle;
  GtkObject *scale_data;
  gboolean   run;

  gimp_ui_init ("xjt", FALSE);

  dlg = gimp_dialog_new (_("Save as XJT"), "xjt",
                         NULL, 0,
                         gimp_standard_help_func, "file-xjt-save",

                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                         GTK_STOCK_SAVE,   GTK_RESPONSE_OK,

                         NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dlg),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dlg));

  table = gtk_table_new (4, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  toggle = gtk_check_button_new_with_label (_("Optimize"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), jsvals.optimize);
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 3, 0, 1,
                    GTK_FILL, 0, 0, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &jsvals.optimize);

  toggle = gtk_check_button_new_with_label (_("Clear transparent"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                               jsvals.clr_transparent);
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 3, 1, 2,
                    GTK_FILL, 0, 0, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &jsvals.clr_transparent);

  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
                                     _("Quality:"), SCALE_WIDTH, 0,
                                     jsvals.quality, 0.0, 1.0, 0.01, 0.11, 2,
                                     TRUE, 0, 0,
                                     NULL, NULL);
  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &jsvals.quality);

  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 3,
                                     _("Smoothing:"), SCALE_WIDTH, 0,
                                     jsvals.smoothing, 0.0, 1.0, 0.01, 0.1, 2,
                                     TRUE, 0, 0,
                                     NULL, NULL);
  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &jsvals.smoothing);

  gtk_widget_show (dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dlg);

  return run;
}

/* ---------------------- SAVE WORKER procedures  -------------------------- */

/* ============================================================================
 * p_get_property_index
 *   get index in g_prop_table by proptype (id)
 * ============================================================================
 */

static int
p_get_property_index(t_proptype proptype)
{
  int l_idx;

  for (l_idx = 0; l_idx < PROP_TABLE_ENTRIES; l_idx++)
    {
      if (g_prop_table[l_idx].prop_id == proptype)
        {
          return l_idx;
        }
    }
  return 0;  /* index of PROP_END -- not supported */
}       /* end p_get_property_index */

/* ============================================================================
 * p_float_to_str
 *   create string from float.
 * ============================================================================
 */
static gchar *
p_float_to_str(gdouble  flt_val)
{
   gchar l_dbl_str[G_ASCII_DTOSTR_BUF_SIZE];
   gchar *l_str;
   gint  l_idx;

   /* XJT float precision is limited to 5 digits */
   /* print setlocale independent float string */
   g_ascii_formatd(&l_dbl_str[0]
                  ,G_ASCII_DTOSTR_BUF_SIZE
                  ,"%.5f"
                  ,flt_val
                  );
   l_str = g_strdup(l_dbl_str);

   /* delete trailing '0' and '.' characters */
   l_idx = strlen(l_str) -1;
   while(l_idx > 0)
   {
     if(l_str[l_idx] != '0')
     {
       break;
     }
     l_str[l_idx] = '\0';
     l_idx--;
   }
   if(l_str[l_idx] == '.')
   {
     l_str[l_idx] = '\0';
   }
   return(l_str);
}

/* ============================================================================
 * p_namedup
 *   copy name and make sure that double quote backslash and newline
 *   characters are escaped by a backslash
 * ============================================================================
 */
static gchar *
p_namedup(const gchar *name)
{
  const gchar *l_str;
  gchar       *l_name;
  gchar       *l_ptr;
  gint         l_len;

  l_str = name;
  l_len = 0;
  while(*l_str != '\0')
  {
       if((*l_str == '\"')
       || (*l_str == '\n')
       || (*l_str == '\\'))
       {
         l_len++;
       }
       l_str++;
       l_len++;
  }

  l_name = g_malloc(l_len+2);
  l_str = name;
  l_ptr = l_name;
  while(*l_str != '\0')
  {
       if((*l_str == '\"')
       || (*l_str == '\n')
       || (*l_str == '\\'))
       {
         *l_ptr = '\\';
         l_ptr++;
         if(*l_str == '\n') { *l_ptr = 'n'; }
         else               { *l_ptr = *l_str; }
       }
       else
       {
         *l_ptr = *l_str;
       }
       l_ptr++;
       l_str++;
  }
  *l_ptr = '\0';
  return(l_name);
}

/* ============================================================================
 * p_write_prop_string
 *   allocate buffer and
 *   write out the property mnemonic to buffer
 *   and parameter(s) according to the property type
 *
 *   wr_all_prp is used for debug only and causes to write out all properties
 *   Normally (wr_all_prp == FALSE) properties are NOT written, if they are
 *   equal to their default value.
 * ============================================================================
 */

static gchar *
p_write_prop_string(t_proptype proptype, t_param_prop *param, gint wr_all_prp)
{
  int l_prop_idx;
  int l_idp;
  gchar *l_f1;
  gchar *l_f2;
  gchar *l_f3;
  gchar *l_str;
  gchar *buffer;

  buffer = NULL;
  l_prop_idx = p_get_property_index(proptype);

  switch (g_prop_table[l_prop_idx].param_typ)
  {
     case PTYP_BOOLEAN:
         /* boolean properties are written if they are not FALSE */
         if(param->int_val1)
         {
            buffer = g_strdup_printf(" %s", g_prop_table[l_prop_idx].prop_mnemonic);
         }
         else
         {
           if(wr_all_prp)
           {
              buffer = g_strdup_printf(" %s!", g_prop_table[l_prop_idx].prop_mnemonic);
           }
         }
         break;
     case PTYP_INT:
         /* int properties are written if they are not equal to default */
         if((param->int_val1 != g_prop_table[l_prop_idx].default_val1)
         || (wr_all_prp))
         {
           buffer = g_strdup_printf(" %s:%d", g_prop_table[l_prop_idx].prop_mnemonic,
                                 (int)param->int_val1);
         }
         break;
     case PTYP_FLT:
         if((param->flt_val1 != g_prop_table[l_prop_idx].default_val1)
         || (wr_all_prp))
         {
           l_f1 = p_float_to_str(param->flt_val1);
           buffer = g_strdup_printf(" %s:%s", g_prop_table[l_prop_idx].prop_mnemonic, l_f1);
           g_free(l_f1);
         }
         break;
     case PTYP_2xFLT:
         if((param->flt_val1 != g_prop_table[l_prop_idx].default_val1)
         || (param->flt_val2 != g_prop_table[l_prop_idx].default_val2)
         || (wr_all_prp))
         {
           l_f1 = p_float_to_str(param->flt_val1);
           l_f2 = p_float_to_str(param->flt_val2);
           buffer = g_strdup_printf(" %s:%s,%s", g_prop_table[l_prop_idx].prop_mnemonic, l_f1, l_f2);
           g_free(l_f1);
           g_free(l_f2);
         }
         break;
     case PTYP_3xFLT:
         if((param->flt_val1 != g_prop_table[l_prop_idx].default_val1)
         || (param->flt_val2 != g_prop_table[l_prop_idx].default_val2)
         || (param->flt_val3 != g_prop_table[l_prop_idx].default_val3)
         || (wr_all_prp))
         {
           l_f1 = p_float_to_str(param->flt_val1);
           l_f2 = p_float_to_str(param->flt_val2);
           l_f3 = p_float_to_str(param->flt_val3);
           buffer = g_strdup_printf(" %s:%s,%s,%s", g_prop_table[l_prop_idx].prop_mnemonic, l_f1, l_f2, l_f3);
           g_free(l_f1);
           g_free(l_f2);
           g_free(l_f3);
         }
         break;
     case PTYP_FLIST:
         if(param->num_fvals > 0)
         {
           l_f1 = p_float_to_str(param->flt_val_list[0]);
           buffer = g_strdup_printf(" %s:%s", g_prop_table[l_prop_idx].prop_mnemonic, l_f1);
           g_free(l_f1);
           for(l_idp = 1; l_idp < param->num_fvals; l_idp++)
           {
              l_f1 = p_float_to_str(param->flt_val_list[l_idp]);
              l_str = g_strdup_printf("%s,%s", buffer, l_f1);
              g_free(buffer);
              buffer = l_str;
              g_free(l_f1);
           }
         }
         break;
     case PTYP_STRING:
         if(param->string_val != NULL)
         {
           if((*param->string_val != '\0')
           || (wr_all_prp))
           {
              l_str = p_namedup(param->string_val);
              if(l_str)
              {
                buffer = g_strdup_printf(" %s:\"%s\"", g_prop_table[l_prop_idx].prop_mnemonic, l_str);
                g_free(l_str);
              }
           }
         }
         break;
     case PTYP_2xINT:
         if((param->int_val1 != g_prop_table[l_prop_idx].default_val1)
         || (param->int_val2 != g_prop_table[l_prop_idx].default_val2)
         || (wr_all_prp))
         {
           buffer = g_strdup_printf(" %s:%d,%d", g_prop_table[l_prop_idx].prop_mnemonic,
                                 (int)param->int_val1, (int)param->int_val2);
         }
         break;
      case PTYP_3xINT:
         if((param->int_val1 != g_prop_table[l_prop_idx].default_val1)
         || (param->int_val2 != g_prop_table[l_prop_idx].default_val2)
         || (param->int_val3 != g_prop_table[l_prop_idx].default_val3)
         || (wr_all_prp))
         {
           buffer = g_strdup_printf(" %s:%d,%d,%d", g_prop_table[l_prop_idx].prop_mnemonic,
                                 (int)param->int_val1,
                                 (int)param->int_val2,
                                 (int)param->int_val3);
         }
         break;
    default:  /*  PTYP_NOT_SUPPORTED */
         break;
  }

  if(buffer == NULL)
  {
    buffer = g_strdup("\0");
  }
  return (buffer);
}       /* end p_write_prop_string */

/* ============================================================================
 * p_write_prop
 *   write out the property mnemonic to file
 *   and parameter(s) according to the property type
 *
 *   wr_all_prp is used for debug only and causes to write out all properties
 *   Normally (wr_all_prp == FALSE) properties are NOT written, if they are
 *   equal to their default value.
 * ============================================================================
 */

static void
p_write_prop(FILE *fp, t_proptype proptype, t_param_prop *param, gint wr_all_prp)
{
  gchar    *l_buff;
  l_buff = p_write_prop_string(proptype, param, wr_all_prp);

  if (l_buff[0] != '\0')
    {
      fprintf(fp, "%s", l_buff);
    }
  g_free(l_buff);
}       /* end p_write_prop */


/* ============================================================================
 * p_write_parasite
 *   write out one parasite by
 *    - write property to PRP file
 *    - write parasite data to parasite_file
 *    - and prepare global_parasite_prop_lines
 * ============================================================================
 */

static gint
p_write_parasite(const gchar  *dirname,
                 FILE         *fp,
                 GimpParasite *parasite,
                 gint          wr_all_prp)
{
  gchar *l_new_parasite_prop_lines;
  gchar *l_parasite_buff;
  gchar *l_parasite_file;
  gchar *l_buff;
  gchar *l_buff2;
  FILE *l_fp_pte;
  t_param_prop   l_param;

  if(parasite->flags & GIMP_PARASITE_PERSISTENT)  /* check if GimpParasite should be saved */
  {
     global_parasite_id++;

     l_param.int_val1 = global_parasite_id;
     p_write_prop (fp, PROP_PARASITES, &l_param, TRUE);

     /* add parasite line like this:
      *     p2 n:"layer parasite A" ptf:1
      * to global_parasite_prop_lines buffer.
      * (the buffer is written as last block to the PRP file)
      */
     l_param.string_val = parasite->name;
     l_buff = p_write_prop_string(PROP_NAME, &l_param, wr_all_prp);

     l_param.int_val1 = parasite->flags;
     l_buff2 = p_write_prop_string(PROP_PARASITE_FLAGS, &l_param, wr_all_prp);

     l_parasite_buff = g_strdup_printf("p%d%s%s\n"
                                       , (int)global_parasite_id
                                       , l_buff
                                       , l_buff2);
     g_free(l_buff);
     g_free(l_buff2);

     if (global_parasite_prop_lines == NULL)
     {
       global_parasite_prop_lines = g_strdup(l_parasite_buff);
     }
     else
     {
       l_new_parasite_prop_lines = g_strdup_printf("%s%s", global_parasite_prop_lines, l_parasite_buff);
       g_free(global_parasite_prop_lines);
       global_parasite_prop_lines = l_new_parasite_prop_lines;
     }
     g_free(l_parasite_buff);

     /* write the parasite data to a file named p1.pte */
     l_parasite_file = g_strdup_printf("%s%cp%d.pte", dirname, G_DIR_SEPARATOR, (int)global_parasite_id);
     l_fp_pte = g_fopen(l_parasite_file, "wb");
     if(l_fp_pte == NULL)
     {
       g_message (_("Could not open '%s' for writing: %s"),
                   gimp_filename_to_utf8 (l_parasite_file), g_strerror (errno));
       g_free(l_parasite_file);
       return -1;
     }
     else
     {
       fwrite(parasite->data, parasite->size, 1, l_fp_pte);
       fclose(l_fp_pte);
       g_free(l_parasite_file);
     }
  }
  return 0;
}

static void
p_write_image_paths(FILE *fp, gint32 image_id, gint wr_all_prp)
{
  gint32     l_idx;
  gchar    **l_path_names = NULL;
  gchar     *l_current_pathname = NULL;
  gint32     l_num_paths = 0;
  gdouble   *l_path_points;
  gint32     l_path_type;
  gint32     l_path_closed;
  gint32     l_num_points = 0;
  t_param_prop   l_param;

  l_path_names = gimp_path_list(image_id, &l_num_paths);
  if(l_path_names == NULL) return;

  l_current_pathname = gimp_path_get_current(image_id);

  for(l_idx = 0; l_idx < l_num_paths; l_idx++)
  {
    if(xjt_debug) printf("p_write_image_paths NAME:%s:\n",  l_path_names[l_idx]);

    l_path_type = gimp_path_get_points(image_id, l_path_names[l_idx],
                                       &l_path_closed, &l_num_points, &l_path_points);
    if (l_path_points)
    {
       /* write PATH line identifier */
      fprintf(fp, "PATH");

      l_param.int_val1 = p_to_XJTPathType(l_path_type);
      p_write_prop (fp, PROP_PATH_TYPE, &l_param, wr_all_prp);

      l_param.int_val1 = gimp_path_get_tattoo(image_id, l_path_names[l_idx]);
      p_write_prop (fp, PROP_TATTOO, &l_param, wr_all_prp);

      l_param.int_val1 = gimp_path_get_locked(image_id, l_path_names[l_idx]);
      p_write_prop (fp, PROP_PATH_LOCKED, &l_param, wr_all_prp);

      l_param.string_val = l_path_names[l_idx];
      p_write_prop (fp, PROP_NAME, &l_param, wr_all_prp);

      /* current path flag */
      l_param.int_val1 = FALSE;
      if(l_current_pathname)
      {
        if(strcmp(l_current_pathname, l_path_names[l_idx]) == 0)
        {
           l_param.int_val1 = TRUE;
           p_write_prop (fp, PROP_PATH_CURRENT, &l_param, wr_all_prp);
        }
      }

      l_param.num_fvals = l_num_points;
      l_param.flt_val_list = l_path_points;
      p_write_prop (fp, PROP_PATH_POINTS, &l_param, wr_all_prp);

      fprintf(fp, "\n");
      g_free(l_path_points);
    }
  }

  g_free(l_current_pathname);
  g_free(l_path_names);
}

static void
p_write_image_parasites(const gchar *dirname,
                        FILE        *fp,
                        gint32       image_id,
                        gint         wr_all_prp)
{
  GimpParasite  *l_parasite;
  gint32         l_idx;
  gchar        **l_parasite_names = NULL;
  gint32         l_num_parasites = 0;

  if (!gimp_image_parasite_list (image_id, &l_num_parasites, &l_parasite_names))
    return;

  for(l_idx = 0; l_idx < l_num_parasites; l_idx++)
  {
    l_parasite = gimp_image_parasite_find(image_id, l_parasite_names[l_idx]);
    if(l_parasite)
    {
       if(xjt_debug) printf("p_write_image_parasites NAME:%s:\n",  l_parasite_names[l_idx]);
       p_write_parasite(dirname, fp, l_parasite, wr_all_prp);
    }
  }
  g_free(l_parasite_names);
}

static void
p_write_drawable_parasites (const gchar *dirname,
                            FILE        *fp,
                            gint32       drawable_id,
                            gint         wr_all_prp)
{
  GimpParasite  *l_parasite;
  gint32         l_idx;
  gchar        **l_parasite_names = NULL;
  gint32         l_num_parasites = 0;

  if (!gimp_drawable_parasite_list (drawable_id, &l_num_parasites, &l_parasite_names))
    return;

  for(l_idx = 0; l_idx < l_num_parasites; l_idx++)
  {
    l_parasite = gimp_drawable_parasite_find(drawable_id, l_parasite_names[l_idx]);
    if(l_parasite)
    {
       if(xjt_debug) printf("p_write_drawable_parasites NAME:%s:\n",  l_parasite_names[l_idx]);
       p_write_parasite(dirname, fp, l_parasite, wr_all_prp);
    }
  }
  g_free(l_parasite_names);
}

/* ============================================================================
 * p_write_layer_prp
 *   write out all properties of the given layer
 * ============================================================================
 */

static void
p_write_layer_prp(const gchar *dirname,
                  FILE        *fp,
                  const gchar *layer_shortname,
                  gint32       image_id,
                  gint32       layer_id,
                  gint         wr_all_prp)
{
  t_param_prop   l_param;
  gint           l_ofsx, l_ofsy;

  fprintf(fp, "%s", layer_shortname);

  l_param.int_val1 = (layer_id == gimp_image_get_active_layer(image_id));       /* TRUE/FALSE */
  p_write_prop (fp, PROP_ACTIVE_LAYER, &l_param, wr_all_prp);

  l_param.int_val1 = gimp_layer_is_floating_sel(layer_id);  /* TRUE/FALSE */
  p_write_prop (fp, PROP_FLOATING_SELECTION, &l_param, wr_all_prp);

  /* check if floating selection is attached to this layer */
  l_param.int_val1 = (layer_id == gimp_image_floating_sel_attached_to(image_id));
  p_write_prop (fp, PROP_FLOATING_ATTACHED, &l_param, wr_all_prp);

  l_param.flt_val1 = gimp_layer_get_opacity(layer_id);
  p_write_prop (fp, PROP_OPACITY, &l_param, wr_all_prp);

  l_param.int_val1 = (gint32)p_to_XJTLayerModeEffects(gimp_layer_get_mode(layer_id));
  p_write_prop (fp, PROP_MODE, &l_param, wr_all_prp);

  l_param.int_val1 = p_invert(gimp_drawable_get_visible(layer_id));
  p_write_prop (fp, PROP_VISIBLE, &l_param, wr_all_prp);

  l_param.int_val1 = gimp_drawable_get_linked (layer_id);
  p_write_prop (fp, PROP_LINKED, &l_param, wr_all_prp);

  l_param.int_val1 = gimp_layer_get_lock_alpha (layer_id);
  p_write_prop (fp, PROP_LOCK_ALPHA, &l_param, wr_all_prp);

  l_param.int_val1 = gimp_layer_get_apply_mask(layer_id);
  p_write_prop (fp, PROP_APPLY_MASK, &l_param, wr_all_prp);

  l_param.int_val1 = gimp_layer_get_edit_mask(layer_id);
  p_write_prop (fp, PROP_EDIT_MASK, &l_param, wr_all_prp);

  l_param.int_val1 = gimp_layer_get_show_mask(layer_id);
  p_write_prop (fp, PROP_SHOW_MASK, &l_param, wr_all_prp);

  gimp_drawable_offsets(layer_id, &l_ofsx, &l_ofsy);
  l_param.int_val1 = l_ofsx;
  l_param.int_val2 = l_ofsy;
  p_write_prop (fp, PROP_OFFSETS, &l_param, wr_all_prp);

  l_param.int_val1 = gimp_drawable_get_tattoo(layer_id);
  p_write_prop (fp, PROP_TATTOO, &l_param, wr_all_prp);

  l_param.string_val = gimp_drawable_get_name(layer_id);
  p_write_prop (fp, PROP_NAME, &l_param, wr_all_prp);

  p_write_drawable_parasites(dirname, fp, layer_id, wr_all_prp);


  fprintf(fp, "\n");
}       /* end p_write_layer_prp */


/* ============================================================================
 * p_write_channel_prp
 *   write out all properties of the given channel
 * ============================================================================
 */

static void
p_write_channel_prp(const gchar *dirname,
                    FILE        *fp,
                    const gchar *channel_shortname,
                    gint32       image_id,
                    gint32       channel_id,
                    gint         wr_all_prp)
{
  t_param_prop   l_param;
  gint           l_ofsx, l_ofsy;
  GimpRGB        color;
  guchar         l_r, l_g, l_b;

  fprintf(fp, "%s", channel_shortname);

  l_param.int_val1 = (channel_id == gimp_image_get_active_channel(image_id));       /* TRUE/FALSE */
  p_write_prop (fp, PROP_ACTIVE_CHANNEL, &l_param, wr_all_prp);

  l_param.int_val1 = (channel_id == gimp_image_get_selection (image_id));  /* TRUE/FALSE */
  p_write_prop (fp, PROP_SELECTION, &l_param, wr_all_prp);

  /* check if floating selection is attached to this channel */
  l_param.int_val1 = (channel_id == gimp_image_floating_sel_attached_to(image_id));
  p_write_prop (fp, PROP_FLOATING_ATTACHED, &l_param, wr_all_prp);

  l_param.flt_val1 = gimp_channel_get_opacity(channel_id);
  p_write_prop (fp, PROP_OPACITY, &l_param, wr_all_prp);

  l_param.int_val1 = p_invert(gimp_drawable_get_visible(channel_id));
  p_write_prop (fp, PROP_VISIBLE, &l_param, wr_all_prp);

  l_param.int_val1 = gimp_channel_get_show_masked(channel_id);
  p_write_prop (fp, PROP_SHOW_MASKED, &l_param, wr_all_prp);

  gimp_channel_get_color(channel_id, &color);
  gimp_rgb_get_uchar (&color, &l_r, &l_g, &l_b);
  l_param.int_val1 = l_r;
  l_param.int_val2 = l_g;
  l_param.int_val3 = l_b;
  p_write_prop (fp, PROP_COLOR, &l_param, wr_all_prp);

  gimp_drawable_offsets(channel_id, &l_ofsx, &l_ofsy);
  l_param.int_val1 = l_ofsx;
  l_param.int_val2 = l_ofsy;
  p_write_prop (fp, PROP_OFFSETS, &l_param, wr_all_prp);

  l_param.int_val1 = gimp_drawable_get_tattoo (channel_id);
  p_write_prop (fp, PROP_TATTOO, &l_param, wr_all_prp);

  l_param.string_val = gimp_drawable_get_name(channel_id);
  p_write_prop (fp, PROP_NAME, &l_param, wr_all_prp);

  p_write_drawable_parasites(dirname, fp, channel_id, wr_all_prp);


  fprintf(fp, "\n");
}       /* end p_write_channel_prp */

/* ============================================================================
 * p_write_image_prp
 *   write out the properties of the image
 * ============================================================================
 */

static void
p_write_image_prp (const gchar *dirname,
                   FILE        *fp,
                   gint32       image_id,
                   gint         wr_all_prp)
{
   GimpImageBaseType l_image_type;
   guint   l_width, l_height;
   gdouble l_xresolution, l_yresolution;
   t_param_prop   l_param;
   gint32 l_guide_id;

   /* get info about the image */
   l_width  = gimp_image_width(image_id);
   l_height = gimp_image_height(image_id);
   l_image_type = gimp_image_base_type(image_id);

   fprintf(fp, "%s", GIMP_XJ_IMAGE);

   l_param.string_val = "1.3.11";
   p_write_prop (fp, PROP_VERSION, &l_param, wr_all_prp);

   l_param.int_val1 = GIMP_MAJOR_VERSION;
   l_param.int_val2 = GIMP_MINOR_VERSION;
   l_param.int_val3 = GIMP_MICRO_VERSION;
   p_write_prop (fp, PROP_GIMP_VERSION, &l_param, wr_all_prp);

   l_param.int_val1 = l_width;
   l_param.int_val2 = l_height;
   p_write_prop (fp, PROP_DIMENSION, &l_param, wr_all_prp);

   gimp_image_get_resolution(image_id, &l_xresolution, &l_yresolution);
   l_param.flt_val1 = l_xresolution;
   l_param.flt_val2 = l_yresolution;
   p_write_prop (fp, PROP_RESOLUTION, &l_param, wr_all_prp);

   /* write unit */
   l_param.int_val1 = p_to_XJTUnitType(gimp_image_get_unit(image_id));
   p_write_prop (fp, PROP_UNIT, &l_param, wr_all_prp);

   /* write tattoo_state */
   l_param.int_val1 = gimp_image_get_tattoo_state(image_id);
   if (l_param.int_val1 > 0)
   {
     p_write_prop (fp, PROP_TATTOO_STATE, &l_param, wr_all_prp);
   }

   /* write guides */
   l_guide_id = gimp_image_find_next_guide(image_id, 0);  /* get 1.st guide */
   while (l_guide_id > 0)
   {
      /* get position and orientation for the current guide ID */

     l_param.int_val1 = gimp_image_get_guide_position(image_id, l_guide_id);
     l_param.int_val2 = p_to_XJTOrientation(gimp_image_get_guide_orientation(image_id, l_guide_id));
     p_write_prop (fp, PROP_GUIDES, &l_param, wr_all_prp);

     /* findnext returns 0 if no (more) guides there
      * (or -1 if no PDB interface is available)
      */
     l_guide_id = gimp_image_find_next_guide(image_id, l_guide_id);
   }

   if (l_image_type == GIMP_GRAY)
   {
     l_param.int_val1 = (gint32)XJT_GRAY;
   }
   else
   {
     l_param.int_val1 = (gint32)XJT_RGB;
   }
   p_write_prop (fp, PROP_TYPE, &l_param, wr_all_prp);

   p_write_image_parasites(dirname, fp, image_id, wr_all_prp);

   fprintf(fp, "\n");

   p_write_image_paths(fp, image_id, wr_all_prp);
}       /* end p_write_image_prp */


/* ---------------------- SAVE  -------------------------- */

static gint
save_xjt_image (const gchar *filename,
                gint32       image_id,
                gint32       drawable_id)
{
  int     l_rc;
  int     l_len;
  int     l_idx;
  gchar  *l_dirname;
  gchar  *l_prop_file;
  gchar  *l_jpg_file;
  gchar  *l_cmd;
  FILE   *l_fp_prp;

  GimpImageBaseType l_image_type;
  gint32 *l_layers_list;
  gint32 *l_channels_list;
  gint    l_nlayers;
  gint    l_nchannels;
  gint32  l_layer_id;
  gint32  l_channel_id;
  gint32  l_floating_layer_id;
  gint32  l_selection_channel_id;
  int     l_sel;
  gint32  l_x1, l_x2, l_y1, l_y2;
  gboolean non_empty;

  gint    l_wr_all_prp;

  l_rc = -1;  /* init retcode to Errorstate */
  l_floating_layer_id = -1;
  l_fp_prp = NULL;
  l_layers_list = NULL;
  l_channels_list = NULL;
  l_dirname = NULL;
  l_prop_file = NULL;
  l_jpg_file = NULL;
  l_wr_all_prp = FALSE;     /* FALSE write only non-default properties
                              * TRUE  write all properties (should be used for DEBUG only)
                              */
  global_parasite_id = 0;
  global_parasite_prop_lines = NULL;


  /* get info about the image */
  l_image_type = gimp_image_base_type(image_id);
  switch (l_image_type)
    {
    case GIMP_RGB:
    case GIMP_GRAY:
      break;
    case GIMP_INDEXED:
      g_message (_("Cannot operate on indexed color images."));
      return -1;
      break;
    default:
      g_message (_("Cannot operate on unknown image types."));
      return -1;
      break;
    }

  gimp_progress_init_printf (_("Saving '%s'"),
                             gimp_filename_to_utf8 (filename));

  /* create temporary directory */
  l_dirname = gimp_temp_name (".tmpdir");
  l_prop_file = g_strdup_printf ("%s%cPRP", l_dirname, G_DIR_SEPARATOR);
  if (g_mkdir (l_dirname, 0777) != 0)
    {
      g_message (_("Could not create working folder '%s': %s"),
                 gimp_filename_to_utf8 (l_dirname), g_strerror (errno));
      goto cleanup;
    }

  /* create property file PRP */
  l_fp_prp = g_fopen (l_prop_file, "wb");
  if (l_fp_prp == NULL)
    {
      g_message (_("Could not open '%s' for writing: %s"),
                 gimp_filename_to_utf8 (l_prop_file), g_strerror (errno));
      goto cleanup;
    }

  /* write image properties */
  p_write_image_prp (l_dirname, l_fp_prp, image_id, l_wr_all_prp);


  l_floating_layer_id = gimp_image_get_floating_sel (image_id);
  if (l_floating_layer_id >= 0)
    {
      if (xjt_debug) printf ("XJT-DEBUG: call floating_sel_relax fsel_id=%d\n",
                             (int) l_floating_layer_id);

      gimp_floating_sel_relax (l_floating_layer_id, FALSE);
    }

  l_layers_list = gimp_image_get_layers (image_id, &l_nlayers);

  /* foreach layer do */
  for (l_idx = 0; l_idx < l_nlayers; l_idx++)
    {
      l_layer_id = l_layers_list[l_idx];

      if (xjt_debug) printf ("Layer [%d] id=%d\n", (int)l_idx, (int)l_layer_id);

      /* save layer as jpeg file */
      l_jpg_file = g_strdup_printf ("%s%cl%d.jpg", l_dirname,
                                    G_DIR_SEPARATOR, l_idx);
      if (xjt_debug) printf ("XJT-DEBUG: saving layer to file %s\n", l_jpg_file);

      if (TRUE != xjpg_save_drawable (l_jpg_file,
                                      image_id,
                                      l_layer_id,
                                      JSVM_DRAWABLE,
                                      &jsvals))
        {
          goto cleanup;
        }
      g_free (l_jpg_file);

      /* write out the layer properties */
      if(gimp_drawable_has_alpha(l_layer_id)) { l_jpg_file = g_strdup_printf("L%d", l_idx); }
      else                                    { l_jpg_file = g_strdup_printf("l%d", l_idx); }
      p_write_layer_prp(l_dirname, l_fp_prp, l_jpg_file, image_id, l_layer_id, l_wr_all_prp);
      g_free(l_jpg_file);

      /* check, and save alpha channel */
      if(gimp_drawable_has_alpha(l_layer_id))
      {
         l_jpg_file = g_strdup_printf("%s%cla%d.jpg", l_dirname, G_DIR_SEPARATOR, l_idx);
         if(xjt_debug) printf("XJT-DEBUG: saving alpha-channel to file %s\n", l_jpg_file);

         if(TRUE != xjpg_save_drawable(l_jpg_file,
                                    image_id,
                                    l_layer_id,
                                    JSVM_ALPHA,
                                    &jsvals ))
         { goto cleanup;
         }
         g_free(l_jpg_file);
      }

      /* check and save layer_mask channel */
       l_channel_id = gimp_layer_get_mask (l_layer_id);
       if(l_channel_id >= 0)
       {
          l_jpg_file = g_strdup_printf("%s%clm%d.jpg", l_dirname, G_DIR_SEPARATOR, l_idx);
          if(xjt_debug) printf("XJT-DEBUG: saving layer-mask to file %s\n", l_jpg_file);

          if(TRUE != xjpg_save_drawable(l_jpg_file,
                                        image_id,
                                        l_channel_id,
                                        JSVM_DRAWABLE,
                                        &jsvals))
          {   goto cleanup;
          }
          g_free(l_jpg_file);

          /* write out the layer_mask (== channel) properties */
          l_jpg_file = g_strdup_printf("m%d", l_idx);
          p_write_channel_prp(l_dirname, l_fp_prp, l_jpg_file, image_id, l_channel_id, l_wr_all_prp);
          g_free(l_jpg_file);
       }
   }    /* end foreach layer */


   /* check and see if we have to save out the selection */
   l_sel = 0;
   l_selection_channel_id = gimp_image_get_selection(image_id);
   if ((gimp_selection_bounds(image_id, &non_empty, &l_x1, &l_y1, &l_x2, &l_y2))
       && (l_selection_channel_id >= 0))
   {
      l_sel = 1;
   }

   l_channels_list = gimp_image_get_channels(image_id, &l_nchannels);

   /* foreach channel do */
   for(l_idx = 0; l_idx < l_nchannels + l_sel; l_idx++)
   {
      if(l_idx < l_nchannels)  l_channel_id = l_channels_list[l_idx];
      else                     l_channel_id = l_selection_channel_id;

      if(xjt_debug) printf("channel [%d] id=%d\n", (int)l_idx, (int)l_channel_id);

      /* save channel as jpeg file */
      l_jpg_file = g_strdup_printf ("%s%cc%d.jpg", l_dirname, G_DIR_SEPARATOR, l_idx);
      if(xjt_debug) printf("XJT-DEBUG: saving channel to file %s\n", l_jpg_file);

      if(TRUE != xjpg_save_drawable(l_jpg_file,
                                    image_id,
                                    l_channel_id,
                                    JSVM_DRAWABLE,
                                    &jsvals))
      {   goto cleanup;
      }
      g_free(l_jpg_file);

      /* write out the channel properties */
      l_jpg_file = g_strdup_printf ("c%d", l_idx);
      p_write_channel_prp(l_dirname, l_fp_prp, l_jpg_file, image_id, l_channel_id, l_wr_all_prp);
      g_free(l_jpg_file);

   }            /* end foreach channel */


   if(global_parasite_prop_lines != NULL)
   {
     /* have to add parasite lines at end of PRP file */
     fprintf(l_fp_prp, "%s", global_parasite_prop_lines);
   }
   fclose( l_fp_prp );
   l_fp_prp = NULL;

   /* store properties and  all layers and cannels in a
    * tar archive with filename.tar
    */
   l_cmd = g_strdup_printf("cd %s; tar -cf \"%s\" *; cd ..", l_dirname, filename);
   l_rc = p_system(l_cmd);
   g_free(l_cmd);

   l_len = strlen(filename);
   if((l_len > 3) && (l_rc == 0))
   {
     /* call optional extracompression programs gzip or bzip2
      * (depends on filename's extension)
      *
      * used gzip options: (bzip2 uses the same options)
      *     -c --stdout --to-stdout
      *          Write  output  on  standard  output
      *     -f --force
      *           Force compression or decompression even if the file
      */
     if(strcmp(&filename[l_len - 3], "bz2") == 0)
     {
       l_cmd = g_strdup_printf("bzip2 -cf \"%s\" >\"%s.tmp_bz2\"",filename , filename);
       l_rc = p_system(l_cmd);
       g_free(l_cmd);
       l_cmd = g_strdup_printf("mv \"%s.tmp_bz2\" \"%s\"" ,filename , filename);
       l_rc = p_system(l_cmd);
       g_free(l_cmd);
     }
     else if(strcmp(&filename[l_len - 2], "gz") == 0)
     {
       l_cmd = g_strdup_printf("gzip -cf <\"%s\" >\"%s.tmp_gz\"",filename , filename);
       l_rc = p_system(l_cmd);
       g_free(l_cmd);
       l_cmd = g_strdup_printf("mv \"%s.tmp_gz\" \"%s\"" ,filename , filename);
       l_rc = p_system(l_cmd);
       g_free(l_cmd);
     }
   }

cleanup:
   if (l_fp_prp)
     {
       fclose (l_fp_prp);
     }

   if (l_floating_layer_id >= 0)
     {
       if (xjt_debug)
         printf("XJT-DEBUG: here we should call floating_sel_rigor sel_id=%d\n",
                (int)l_floating_layer_id);
       gimp_floating_sel_rigor (l_floating_layer_id, FALSE);
     }

   g_free (l_layers_list);
   g_free (l_channels_list);

   /* remove the temorary directory */
   l_cmd = g_strdup_printf("rm -rf \"%s\"", l_dirname);

   if(!xjt_debug) p_system(l_cmd);
   g_free(l_cmd);

   g_free (l_dirname);
   g_free (l_prop_file);
   g_free (l_jpg_file);

   return l_rc;
}

/* ---------------------- LOAD WORKER procedures  -------------------------- */


/* ============================================================================
 * p_new_layer_prop
 *   allocate new layer_properties element and init with default values
 * ============================================================================
 */
static t_layer_props *
p_new_layer_prop(void)
{
  t_layer_props  *l_new_prop;

  l_new_prop = g_malloc(sizeof(t_layer_props));
  l_new_prop->active_layer       = g_prop_table[p_get_property_index(PROP_ACTIVE_LAYER)].default_val1;
  l_new_prop->floating_selection = g_prop_table[p_get_property_index(PROP_FLOATING_SELECTION)].default_val1;
  l_new_prop->floating_attached = g_prop_table[p_get_property_index(PROP_FLOATING_ATTACHED)].default_val1;
  l_new_prop->opacity            = g_prop_table[p_get_property_index(PROP_OPACITY)].default_val1;
  l_new_prop->mode               = g_prop_table[p_get_property_index(PROP_MODE)].default_val1;
  l_new_prop->visible            = p_invert(g_prop_table[p_get_property_index(PROP_VISIBLE)].default_val1);
  l_new_prop->linked             = g_prop_table[p_get_property_index(PROP_LINKED)].default_val1;
  l_new_prop->lock_alpha         = g_prop_table[p_get_property_index(PROP_LOCK_ALPHA)].default_val1;
  l_new_prop->apply_mask         = g_prop_table[p_get_property_index(PROP_APPLY_MASK)].default_val1;
  l_new_prop->edit_mask          = g_prop_table[p_get_property_index(PROP_EDIT_MASK)].default_val1;
  l_new_prop->show_mask          = g_prop_table[p_get_property_index(PROP_SHOW_MASK)].default_val1;
  l_new_prop->offx               = g_prop_table[p_get_property_index(PROP_OFFSETS)].default_val1;
  l_new_prop->offy               = g_prop_table[p_get_property_index(PROP_OFFSETS)].default_val2;
  l_new_prop->tattoo             = g_prop_table[p_get_property_index(PROP_TATTOO)].default_val1;
  l_new_prop->name = NULL;

  l_new_prop->layer_pos = -1;
  l_new_prop->has_alpha = FALSE;
  l_new_prop->next = NULL;
  return l_new_prop;
}       /* end p_new_layer_prop */


/* ============================================================================
 * p_new_channel_prop
 *   allocate new channel_properties element and init with default values
 * ============================================================================
 */
static t_channel_props *
p_new_channel_prop(void)
{
  t_channel_props  *l_new_prop;

  l_new_prop = g_malloc(sizeof(t_channel_props));
  l_new_prop->active_channel    = g_prop_table[p_get_property_index(PROP_ACTIVE_CHANNEL)].default_val1;
  l_new_prop->selection         = g_prop_table[p_get_property_index(PROP_SELECTION)].default_val1;
  l_new_prop->floating_attached = g_prop_table[p_get_property_index(PROP_FLOATING_ATTACHED)].default_val1;
  l_new_prop->opacity           = g_prop_table[p_get_property_index(PROP_OPACITY)].default_val1;
  l_new_prop->visible           = p_invert(g_prop_table[p_get_property_index(PROP_VISIBLE)].default_val1);
  l_new_prop->show_masked       = g_prop_table[p_get_property_index(PROP_SHOW_MASKED)].default_val1;
  l_new_prop->offx              = g_prop_table[p_get_property_index(PROP_OFFSETS)].default_val1;
  l_new_prop->offy              = g_prop_table[p_get_property_index(PROP_OFFSETS)].default_val2;
  l_new_prop->color_r           = g_prop_table[p_get_property_index(PROP_COLOR)].default_val1;
  l_new_prop->color_g           = g_prop_table[p_get_property_index(PROP_COLOR)].default_val2;
  l_new_prop->color_b           = g_prop_table[p_get_property_index(PROP_COLOR)].default_val3;
  l_new_prop->tattoo            = g_prop_table[p_get_property_index(PROP_TATTOO)].default_val1;
  l_new_prop->name = NULL;

  l_new_prop->channel_pos = -1;
  l_new_prop->next = NULL;
  return l_new_prop;
}       /* end p_new_channel_prop */


/* ============================================================================
 * p_new_guide_prop
 *   allocate new guide_properties element and init with default values
 * ============================================================================
 */
static t_guide_props *
p_new_guide_prop(void)
{
  t_guide_props  *l_new_prop;

  l_new_prop = g_malloc(sizeof(t_guide_props));
  l_new_prop->position = 0;
  l_new_prop->orientation = 0;
  l_new_prop->next = NULL;
  return l_new_prop;
}       /* end p_new_guide_prop */

/* ============================================================================
 * p_new_parasite_prop
 *   allocate new parasite_properties element and init with default values
 * ============================================================================
 */
static t_parasite_props *
p_new_parasite_prop(void)
{
  t_parasite_props  *l_new_prop;

  l_new_prop = g_malloc(sizeof(t_parasite_props));
  l_new_prop->parasite_type = XJT_IMAGE_PARASITE;
  l_new_prop->parasite_id = -1;
  l_new_prop->name = NULL;
  l_new_prop->obj_pos = -1;
  l_new_prop->flags = GIMP_PARASITE_PERSISTENT;
  l_new_prop->next = NULL;
  return l_new_prop;
}       /* end p_new_parasite_prop */

/* ============================================================================
 * p_new_path_prop
 *   allocate new parasite_properties element and init with default values
 * ============================================================================
 */
static t_path_props *
p_new_path_prop(void)
{
  t_path_props  *l_new_prop;

  l_new_prop = g_malloc(sizeof(t_path_props));
  l_new_prop->path_type = p_to_GimpPathType(XJT_PATHTYPE_BEZIER);
  l_new_prop->tattoo    = g_prop_table[p_get_property_index(PROP_TATTOO)].default_val1;
  l_new_prop->path_locked = FALSE;
  l_new_prop->path_closed = FALSE;
  l_new_prop->current_flag = FALSE;
  l_new_prop->name = NULL;
  l_new_prop->num_points = 0;
  l_new_prop->path_points = NULL;
  l_new_prop->next = NULL;
  return l_new_prop;
}       /* end p_new_path_prop */

/* ============================================================================
 * p_new_image_prop
 *   allocate new layer_properties element and init with default values
 * ============================================================================
 */
static t_image_props *
p_new_image_prop(void)
{
  t_image_props  *l_new_prop;

  l_new_prop = g_malloc(sizeof(t_image_props));
  l_new_prop->image_type       = g_prop_table[p_get_property_index(PROP_TYPE)].default_val1;
  l_new_prop->image_width      = g_prop_table[p_get_property_index(PROP_DIMENSION)].default_val1;
  l_new_prop->image_height     = g_prop_table[p_get_property_index(PROP_DIMENSION)].default_val2;
  l_new_prop->xresolution      = g_prop_table[p_get_property_index(PROP_RESOLUTION)].default_val1;
  l_new_prop->yresolution      = g_prop_table[p_get_property_index(PROP_RESOLUTION)].default_val2;
  l_new_prop->unit             = p_to_GimpUnit(g_prop_table[p_get_property_index(PROP_UNIT)].default_val1);
  l_new_prop->tattoo           = g_prop_table[p_get_property_index(PROP_TATTOO)].default_val1;
  l_new_prop->tattoo_state     = g_prop_table[p_get_property_index(PROP_TATTOO_STATE)].default_val1;
  l_new_prop->n_layers = 0;
  l_new_prop->n_channels = 0;
  l_new_prop->layer_props = NULL;
  l_new_prop->channel_props = NULL;
  l_new_prop->mask_props = NULL;
  l_new_prop->guide_props = NULL;
  l_new_prop->parasite_props = NULL;
  l_new_prop->path_props = NULL;
  return l_new_prop;
}       /* end p_new_image_prop */


/* ============================================================================
 * p_skip_blanks
 *
 * ============================================================================
 */
static gchar *
p_skip_blanks(gchar* scan_ptr)
{
  while((*scan_ptr == ' ') || (*scan_ptr == '\t'))
  {
     scan_ptr++;
  }
  return(scan_ptr);
}

/* ============================================================================
 * p_scann_token
 *   scan one token with its parameter(s)
 *   return pointer to character behind the last scanned byte.
 *
 * Check prop_id for PROP_END             (no more tokens found)
 *               for PROP_SYNTAX_ERROR    (illegal token/params detected)
 * ============================================================================
 */

static gchar *
p_scann_token(gchar        *scan_ptr,
              t_param_prop *param,
              t_proptype   *prop_id)
{
  int    l_idx;
  int    l_ids;
  int    l_max_line_len;
  int    l_num_fvals;
  gchar *l_token;
  gchar *l_string_buff;
  gchar *l_ptr;
  gchar *l_ptr2;


  /* check for end of property line */
  if((*scan_ptr == '\0') || (*scan_ptr == '\n'))
  {
    *prop_id  = PROP_END;
    return(scan_ptr);
  }

  *prop_id  = PROP_SYNTAX_ERROR;

  l_max_line_len = strlen(scan_ptr);
  l_token = g_malloc(l_max_line_len + 1);

  /* extract current token at scann_ptr position */
  l_ptr = scan_ptr;
  for (l_idx = 0; l_idx < (l_max_line_len -1); l_idx++)
  {
     if ((*l_ptr == ' ')
     ||  (*l_ptr == '\n')
     ||  (*l_ptr == '\t')
     ||  (*l_ptr == '\0')
     ||  (*l_ptr == '!')
     ||  (*l_ptr == ':'))
     {
        break;
     }

     l_token[l_idx] = (*l_ptr);
     l_ptr++;
  }
  l_token[l_idx] = '\0';

  if(xjt_debug) printf("XJT:TOKEN:%s:", l_token);

  /* check if token is one of the supported mnemonics */
  for (l_idx = 0; l_idx < PROP_TABLE_ENTRIES; l_idx++)
  {
    if(g_prop_table[l_idx].param_typ == PTYP_NOT_SUPPORTED)
    {
      continue;
    }

    if(0 != strcmp(l_token, g_prop_table[l_idx].prop_mnemonic))
    {
      continue;
    }

    *prop_id = g_prop_table[l_idx].prop_id;

    /* scann parameters according to detected property type */
    switch(g_prop_table[l_idx].param_typ)
    {
       case PTYP_BOOLEAN:
           param->int_val1 = TRUE;
           if(*l_ptr == ':')
           {
              g_printerr ("XJT: PRP syntax error (bool property %s terminated with :)\n",
                          l_token);
              *prop_id  = PROP_SYNTAX_ERROR;
              return (l_ptr);
           }
           if(*l_ptr == '!')   /* inverter character */
           {
              param->int_val1 = FALSE;
              l_ptr++;
           }
           if(xjt_debug) printf("%d", (int)param->int_val1);
           break;
       case PTYP_3xINT:
       case PTYP_2xINT:
       case PTYP_INT:
           if(*l_ptr != ':')
           {
              g_printerr ("XJT: PRP syntax error (int property %s not terminated with :)\n",
                          l_token);
              *prop_id  = PROP_SYNTAX_ERROR;
           }
           else
           {
              l_ptr++;
              param->int_val1 = strtol(l_ptr, &l_ptr2, 10);   /* Scan 1.st integer (base = 10) */
              if (l_ptr == l_ptr2 )
              {
                 g_printerr ("XJT: PRP syntax error (int property %s :integer value missing)\n",
                             l_token);
                 *prop_id  = PROP_SYNTAX_ERROR;
                 return(l_ptr);
              }
              l_ptr = l_ptr2;
              if(xjt_debug) printf("%d", (int)param->int_val1);

              if((PTYP_2xINT == g_prop_table[l_idx].param_typ)
              || (PTYP_3xINT == g_prop_table[l_idx].param_typ))
              {
                 if(*l_ptr != ',')
                 {
                    g_printerr ("XJT: PRP syntax error (int property %s comma missing)\n",
                                l_token);
                    *prop_id  = PROP_SYNTAX_ERROR;
                    return(l_ptr);
                 }
                 l_ptr++;
                 param->int_val2 = strtol(l_ptr, &l_ptr2, 10);   /*  Scan 2.nd integer (base = 10)  */
                 if (l_ptr == l_ptr2 )
                 {
                    g_printerr ("XJT: PRP syntax error (int property %s : 2.nd integer value missing)\n",
                                l_token);
                    *prop_id  = PROP_SYNTAX_ERROR;
                    return(l_ptr);
                 }
                 l_ptr = l_ptr2;
                 if(xjt_debug) printf(",%d", (int)param->int_val2);

                 if(PTYP_3xINT == g_prop_table[l_idx].param_typ)
                 {
                    if(*l_ptr != ',')
                    {
                       g_printerr ("XJT: PRP syntax error (int property %s comma missing)\n",
                                   l_token);
                       *prop_id  = PROP_SYNTAX_ERROR;
                       return(l_ptr);
                    }
                    l_ptr++;
                    param->int_val3 = strtol(l_ptr, &l_ptr2, 10);   /*  Scan 3.rd integer (base = 10)  */
                    if (l_ptr == l_ptr2 )
                    {
                       g_printerr ("XJT: PRP syntax error (int property %s : 3.rd integer value missing)\n",
                                   l_token);
                       *prop_id  = PROP_SYNTAX_ERROR;
                       return(l_ptr);
                    }
                    l_ptr = l_ptr2;
                    if(xjt_debug) printf(",%d", (int)param->int_val3);
                 }
              }
           }
           break;
       case PTYP_3xFLT:
       case PTYP_2xFLT:
       case PTYP_FLT:
           if(*l_ptr != ':')
           {
              g_printerr ("XJT: PRP syntax error (float property %s not terminated with :)\n",
                          l_token);
              *prop_id  = PROP_SYNTAX_ERROR;
           }
           else
           {
              l_ptr++;
              param->flt_val1 = p_my_ascii_strtod(l_ptr, &l_ptr2);
              if (l_ptr == l_ptr2 )
              {
                 g_printerr ("XJT: PRP syntax error (float property %s :float value missing)\n",
                             l_token);
                 *prop_id  = PROP_SYNTAX_ERROR;
                 return(l_ptr);
              }
              l_ptr = l_ptr2;
              if(xjt_debug)    printf("%f", param->flt_val1);

              if((PTYP_2xFLT == g_prop_table[l_idx].param_typ)
              || (PTYP_3xFLT == g_prop_table[l_idx].param_typ))
              {
                 if(*l_ptr != ',')
                 {
                    g_printerr ("XJT: PRP syntax error (float property %s comma missing)\n",
                                l_token);
                    *prop_id  = PROP_SYNTAX_ERROR;
                    return(l_ptr);
                 }
                 l_ptr++;
                 param->flt_val2 = p_my_ascii_strtod(l_ptr, &l_ptr2);
                 if (l_ptr == l_ptr2 )
                 {
                    g_printerr ("XJT: PRP syntax error (float property %s : 2.nd float value missing)\n",
                                l_token);
                    *prop_id  = PROP_SYNTAX_ERROR;
                    return(l_ptr);
                 }
                 l_ptr = l_ptr2;
                 if(xjt_debug) printf(",%f", param->flt_val2);

                 if(PTYP_3xFLT == g_prop_table[l_idx].param_typ)
                 {
                    if(*l_ptr != ',')
                    {
                       g_printerr ("XJT: PRP syntax error (float property %s comma missing)\n",
                                   l_token);
                       *prop_id  = PROP_SYNTAX_ERROR;
                       return(l_ptr);
                    }
                    l_ptr++;
                    param->flt_val3 = p_my_ascii_strtod(l_ptr, &l_ptr2);
                    if (l_ptr == l_ptr2 )
                    {
                       g_printerr ("XJT: PRP syntax error (float property %s : 3.rd float value missing)\n",
                                   l_token);
                       *prop_id  = PROP_SYNTAX_ERROR;
                       return(l_ptr);
                    }
                    l_ptr = l_ptr2;
                    if(xjt_debug) printf(",%f", param->flt_val3);
                 }
              }
           }
           break;
       case PTYP_FLIST:
           if(*l_ptr != ':')
           {
              g_printerr ("XJT: PRP syntax error (floatlist property %s not terminated with :)\n",
                          l_token);
              *prop_id  = PROP_SYNTAX_ERROR;
           }
           else
           {
              l_ptr++;
              /* counting ',' to guess how much values are in the list */
              l_num_fvals = 1;
              l_ptr2 = l_ptr;
              while(1)
              {
                if((*l_ptr2 == '\0')
                || (*l_ptr2 == '\n'))
                {
                  break;
                }
                if(*l_ptr2 == ',')
                {
                  l_num_fvals++;
                }
                l_ptr2++;
              }

              param->num_fvals = 0;
              param->flt_val_list = g_malloc0(sizeof(gdouble) * l_num_fvals);
              while(1)
              {
                param->flt_val_list[param->num_fvals] = p_my_ascii_strtod(l_ptr, &l_ptr2);
                if (l_ptr == l_ptr2 )
                {
                   if(param->num_fvals == 0)
                   {
                     g_printerr ("XJT: PRP syntax error (floatlist property %s :no float value found)\n",
                                 l_token);
                     *prop_id  = PROP_SYNTAX_ERROR;
                     return(l_ptr);
                   }
                   break;
                }
                l_ptr = l_ptr2;
                if(xjt_debug)    printf("%f char:%c\n", param->flt_val_list[param->num_fvals], *l_ptr);
                param->num_fvals++;
                if(*l_ptr != ',')
                {
                  if((*l_ptr2 != '\0')
                  && (*l_ptr2 != '\n'))
                  {
                     g_printerr ("XJT: PRP syntax error (floatlist property %s :list contains illegal character: %c)\n",
                                 l_token, *l_ptr);
                     *prop_id  = PROP_SYNTAX_ERROR;
                     return(l_ptr);
                  }
                  break;
                }
                l_ptr++;
              }
           }
           break;
       case PTYP_STRING:
           if(*l_ptr != ':')
           {
              g_printerr ("XJT: PRP syntax error (string property %s not terminated with :)\n",
                          l_token);
              *prop_id  = PROP_SYNTAX_ERROR;
              return (l_ptr);
           }
           l_ptr++;
           if(*l_ptr != '\"')
           {
              g_printerr ("XJT: PRP syntax error (string property %s starting \" is missing)\n",
                          l_token);
              *prop_id  = PROP_SYNTAX_ERROR;
               return (l_ptr);
           }
           l_ptr++;
           l_string_buff = g_malloc(l_max_line_len + 2);
           for(l_ids= 0; l_ids < (l_max_line_len -2); l_ids++)
           {
              if(*l_ptr == '\0')
              {
                 break;
              }
              if(*l_ptr == '\\')
              {
                 if(l_ptr[1] == '\"')
                 {
                     l_string_buff[l_ids] = '\"';
                     l_ptr++;
                     l_ptr++;
                     continue;
                 }
                 if(l_ptr[1] == 'n')
                 {
                     l_string_buff[l_ids] = '\n';
                     l_ptr++;
                     l_ptr++;
                     continue;
                 }
                 if(l_ptr[1] == '\\')
                 {
                     l_string_buff[l_ids] = '\\';
                     l_ptr++;
                     l_ptr++;
                     continue;
                 }
                 l_ptr++;
              }
              if((*l_ptr == '\"') ||
                 (*l_ptr == '\n'))
              {
                 break;
              }
              l_string_buff[l_ids] = *l_ptr;
              l_ptr++;
           }
           l_string_buff[l_ids] = '\0';
           if (*l_ptr == '\"')
           {
             l_ptr++;
             param->string_val = g_strdup (l_string_buff);
             if(xjt_debug) printf("%s", param->string_val);
          }
           else
           {
             g_printerr ("XJT: PRP syntax error (string property %s terminating \" is missing)\n",
                         l_token);
             *prop_id  = PROP_SYNTAX_ERROR;
           }
           g_free(l_string_buff);
           break;
       default:
           g_printerr ("XJT: ** Warning ** PRP file with unsupported property %s\n",
                       l_token);
           *prop_id  = PROP_SYNTAX_ERROR;
           break;
    }

    break;
  }

  if(xjt_debug) printf("\n");

  /* advance l_ptr to next Blank
   * (this is needed to skip unknown tokens
   */
  while(1)
  {
     if ((*l_ptr == ' ')
     ||  (*l_ptr == '\n')
     ||  (*l_ptr == '\t')
     ||  (*l_ptr == '\0'))
     {
        break;
     }

     l_ptr++;
  }

  if(*prop_id  == PROP_SYNTAX_ERROR)
  {
    g_printerr ("XJT: ** Warning ** PRP file skipping property: %s\n", l_token);
  }

  g_free(l_token);
  return(l_ptr);
}       /* end p_scann_token */


/* ============================================================================
 * p_find_parasite
 *   if the parasite_props list has an item with parasite_id
 *   return the pointer to the matching item,
 *   else return NULL (no matching item found)
 * ============================================================================
 */
static t_parasite_props *
p_find_parasite(t_parasite_props *parasite_props, gint32 parasite_id)
{
  t_parasite_props  *l_prop;

  if(xjt_debug) printf("XJT: p_find_parasite search parasite_id (1): %d\n", (int)parasite_id );

  l_prop = parasite_props;
  while(l_prop)
  {
     if(xjt_debug) printf("XJT: p_find_parasite (2) listid=%d\n", (int)l_prop->parasite_id );
     if(l_prop->parasite_id == parasite_id)
     {
       if(xjt_debug) printf("XJT: p_find_parasite (3) listid=%d\n", (int)l_prop->parasite_id );
       return(l_prop);
     }
     l_prop = (t_parasite_props *)l_prop->next;
  }

  if(xjt_debug) printf("XJT: p_find_parasite (4) NULL\n");
  return(NULL);
}

/* ============================================================================
 * p_create_and_attach_parasite
 * ============================================================================
 */
static gint
p_create_and_attach_parasite (gint32            gimp_obj_id,
                              const gchar      *dirname,
                              t_parasite_props *parasite_props)
{
  gchar            *l_parasite_file;
  GimpParasite      l_parasite;
  struct stat       l_stat_buf;
  FILE *l_fp_pte;

  /* create filename dirname/p1.pte  1 == parasite_id */
  l_parasite_file = g_strdup_printf("%s%cp%d.pte", dirname, G_DIR_SEPARATOR, (int)parasite_props->parasite_id);

  if (0 != g_stat(l_parasite_file, &l_stat_buf))
  {
     /* stat error (file does not exist) */
     g_message (_("Could not open '%s' for reading: %s"),
                 gimp_filename_to_utf8 (l_parasite_file), g_strerror (errno));
     return(-1);
  }

  l_fp_pte = g_fopen(l_parasite_file, "rb");
  if(l_fp_pte == NULL)
  {
     g_message (_("Could not open '%s' for reading: %s"),
                 gimp_filename_to_utf8 (l_parasite_file), g_strerror (errno));
     return(-1);
  }

  g_free(l_parasite_file);

  l_parasite.size = l_stat_buf.st_size;
  l_parasite.data = g_malloc(l_parasite.size);
  l_parasite.flags = parasite_props->flags | GIMP_PARASITE_PERSISTENT;
  if(parasite_props->name)
  {
     l_parasite.name = g_strdup(parasite_props->name);
  }
  else
  {
     l_parasite.name = g_strdup("\0");
  }

  /* read the parasitedata from file */
  fread(l_parasite.data, l_parasite.size, 1, l_fp_pte);
  fclose(l_fp_pte);


  /* attach the parasite to gimp_obj_id
   * (is an Image or drawable id depending on parasite_type)
   */
  if(parasite_props->parasite_type == XJT_IMAGE_PARASITE)
  {
     if(xjt_debug) printf("XJT:   gimp_image_parasite_attach  name:%s\n", l_parasite.name);
     gimp_image_parasite_attach(gimp_obj_id, &l_parasite);

  }
  else
  {
     if(xjt_debug) printf("XJT:   gimp_drawable_parasite_attach  name:%s\n", l_parasite.name);
     gimp_drawable_parasite_attach(gimp_obj_id, &l_parasite);
  }

  if(l_parasite.data) g_free(l_parasite.data);
  if(l_parasite.name) g_free(l_parasite.name);

  return (0); /*OK */
}       /* end p_create_and_attach_parasite */

/* ============================================================================
 * p_check_and_add_parasite
 *   find parasite by type and pos (obj_pos)
 * ============================================================================
 */
static void
p_check_and_add_parasite (gint32            gimp_obj_id,
                          const gchar      *dirname,
                          t_parasite_props *parasite_props,
                          gint32            pos,
                          t_parasitetype    parasite_type)
{
  t_parasite_props  *l_prop;

  l_prop = parasite_props;
  while(l_prop)
  {
     if((l_prop->parasite_type == parasite_type)
     && (l_prop->obj_pos == pos))
     {
        p_create_and_attach_parasite(gimp_obj_id, dirname, l_prop);
     }
     l_prop = (t_parasite_props *)l_prop->next;
  }
}       /* end p_check_and_add_parasite */

/* ============================================================================
 * p_scann_layer_prop
 *   scann one inputline for layer properties (also used for layer_mask props)
 * ============================================================================
 */
static gint
p_scann_layer_prop (gchar         *scan_ptr,
                    t_image_props *image_prop)
{

  t_layer_props  *l_new_prop;
  t_parasite_props *l_parasite_prop;
  gchar          *l_ptr;
  t_param_prop    l_param;
  t_proptype      l_prop_id;

  l_new_prop = p_new_layer_prop();

  /* add the new element to the layer_props list */
  l_new_prop->next = image_prop->layer_props;
  image_prop->layer_props = l_new_prop;

  if(*scan_ptr == 'L')
  {
    l_new_prop->has_alpha = TRUE;
  }

  l_new_prop->layer_pos = strtol(&scan_ptr[1], &l_ptr, 10);   /*  Scan integer (base = 10)  */
  if(l_ptr == scan_ptr)
  {
     g_printerr ("XJT: PRP file layer# missing, scanned bad line:\n%s\n", scan_ptr);
     return -1;
  }

  while(1)
  {
     l_ptr = p_skip_blanks(l_ptr);
     l_ptr = p_scann_token(l_ptr, &l_param, &l_prop_id);

     switch(l_prop_id)
     {
       case PROP_END:
            return 0;
            break;
       case PROP_ACTIVE_LAYER:
            l_new_prop->active_layer = l_param.int_val1;
            break;
       case PROP_FLOATING_SELECTION:
            l_new_prop->floating_selection = l_param.int_val1;
            break;
       case PROP_FLOATING_ATTACHED:
            l_new_prop->floating_attached = l_param.int_val1;
            break;
       case PROP_OPACITY:
            l_new_prop->opacity = l_param.flt_val1;
            break;
       case PROP_MODE:
            l_new_prop->mode = p_to_GimpLayerModeEffects(l_param.int_val1);
            break;
       case PROP_VISIBLE:
            l_new_prop->visible = p_invert(l_param.int_val1);
            break;
       case PROP_LINKED:
            l_new_prop->linked = l_param.int_val1;
            break;
       case PROP_LOCK_ALPHA:
            l_new_prop->lock_alpha = l_param.int_val1;
            break;
       case PROP_APPLY_MASK:
            l_new_prop->apply_mask = l_param.int_val1;
            break;
       case PROP_EDIT_MASK:
            l_new_prop->edit_mask = l_param.int_val1;
            break;
       case PROP_SHOW_MASK:
            l_new_prop->show_mask = l_param.int_val1;
            break;
       case PROP_OFFSETS:
            l_new_prop->offx = l_param.int_val1;
            l_new_prop->offy = l_param.int_val2;
            break;
       case PROP_NAME:
            l_new_prop->name  = l_param.string_val;
            break;
       case PROP_TATTOO:
            l_new_prop->tattoo = l_param.int_val1;
            break;
       case PROP_PARASITES:
            l_parasite_prop = p_find_parasite(image_prop->parasite_props, l_param.int_val1);
            if(l_parasite_prop == NULL)
            {
              l_parasite_prop = p_new_parasite_prop();
              l_parasite_prop->next = image_prop->parasite_props;
              image_prop->parasite_props = l_parasite_prop;
            }

            if(l_parasite_prop)
            {
              l_parasite_prop->parasite_id = l_param.int_val1;
              l_parasite_prop->parasite_type = XJT_LAYER_PARASITE;
              l_parasite_prop->obj_pos = l_new_prop->layer_pos;
            }
            break;
       default :
            /* g_printerr ("XJT: PRP file scanned bad line:\n%s\n", scan_ptr); */
            /* return -1; */ /* skip unknow tokens */
            break;
     }
  }

}       /* end p_scann_layer_prop */


/* ============================================================================
 * p_scann_channel_prop
 *   scann one inputline for channel properties (also used for layer_mask props)
 * ============================================================================
 */
static gint
p_scann_channel_prop (const gchar   *scan_ptr,
                      t_image_props *image_prop)
{

  t_channel_props  *l_new_prop;
  t_parasite_props *l_parasite_prop;
  gchar          *l_ptr;
  t_param_prop    l_param;
  t_proptype      l_prop_id;
  t_parasitetype  l_parasite_type;

  l_new_prop = p_new_channel_prop();

  l_new_prop->channel_pos = strtol(&scan_ptr[1], &l_ptr, 10);   /*  Scan integer (base = 10)  */
  if(l_ptr == scan_ptr)
  {
     g_printerr ("XJT: PRP file channel# missing, scanned bad line:\n%s\n",
                 scan_ptr);
     return -1;
  }

  /* check if it is layer_mask or channel */
  if(*scan_ptr == 'm')
  {
    /* add the new element to the mask_props list of the image */
    l_new_prop->next = image_prop->mask_props;
    image_prop->mask_props = l_new_prop;
    l_parasite_type = XJT_LAYER_MASK_PARASITE;
  }
  else
  {
    /* add the new element to the channel_props list of the image */
    l_new_prop->next = image_prop->channel_props;
    image_prop->channel_props = l_new_prop;
    l_parasite_type = XJT_CHANNEL_PARASITE;
  }


  while(1)
  {
     l_ptr = p_skip_blanks(l_ptr);
     l_ptr = p_scann_token(l_ptr, &l_param, &l_prop_id);

     switch(l_prop_id)
     {
       case PROP_END:
            return 0;
            break;
       case PROP_ACTIVE_CHANNEL:
            l_new_prop->active_channel = l_param.int_val1;
            break;
       case PROP_SELECTION:
            l_new_prop->selection = l_param.int_val1;
            break;
       case PROP_FLOATING_ATTACHED:
            l_new_prop->floating_attached = l_param.int_val1;
            break;
       case PROP_OPACITY:
            l_new_prop->opacity = l_param.flt_val1;
            break;
       case PROP_VISIBLE:
            l_new_prop->visible = p_invert(l_param.int_val1);
            break;
       case PROP_SHOW_MASKED:
            l_new_prop->show_masked = l_param.int_val1;
            break;
       case PROP_OFFSETS:
            l_new_prop->offx = l_param.int_val1;
            l_new_prop->offy = l_param.int_val2;
            break;
       case PROP_COLOR:
            l_new_prop->color_r = l_param.int_val1;
            l_new_prop->color_g = l_param.int_val2;
            l_new_prop->color_b = l_param.int_val3;
            break;
       case PROP_NAME:
            l_new_prop->name  = l_param.string_val;
            break;
       case PROP_TATTOO:
            l_new_prop->tattoo = l_param.int_val1;
            break;
       case PROP_PARASITES:
            l_parasite_prop = p_find_parasite(image_prop->parasite_props, l_param.int_val1);
            if(l_parasite_prop == NULL)
            {
              l_parasite_prop = p_new_parasite_prop();
              l_parasite_prop->next = image_prop->parasite_props;
              image_prop->parasite_props = l_parasite_prop;
            }

            if(l_parasite_prop)
            {
              l_parasite_prop->parasite_id = l_param.int_val1;
              l_parasite_prop->parasite_type = l_parasite_type;
              l_parasite_prop->obj_pos = l_new_prop->channel_pos;
            }
            break;
       default :
            /* g_printerr ("XJT: PRP file scanned bad line:\n%s\n", scan_ptr); */
            /* return -1; */ /* skip unknow tokens */
            break;
     }
  }

}       /* end p_scann_channel_prop */

/* ============================================================================
 * p_scann_image_prop
 *
 * ============================================================================
 */
static gint
p_scann_image_prop (gchar         *scan_ptr,
                    t_image_props *image_prop)
{
  gchar            *l_ptr;
  t_param_prop      l_param;
  t_proptype        l_prop_id;
  t_guide_props    *l_guide_prop;
  t_parasite_props *l_parasite_prop;

  if(strncmp(scan_ptr, GIMP_XJ_IMAGE, strlen(GIMP_XJ_IMAGE)) != 0)
  {
    g_printerr ("XJT: PRP file %s identstring missing, scanned bad line:\n%s\n",
                GIMP_XJ_IMAGE, scan_ptr);
    return -1;
  }

  l_ptr = &scan_ptr[strlen(GIMP_XJ_IMAGE)];

  while(1)
  {
     l_ptr = p_skip_blanks(l_ptr);
     l_ptr = p_scann_token(l_ptr, &l_param, &l_prop_id);

     switch(l_prop_id)
     {
       case PROP_END:
            return 0;
            break;
       case PROP_VERSION:
            image_prop->version = l_param.string_val;
            break;
       case PROP_GIMP_VERSION:
            image_prop->gimp_major_version = l_param.int_val1;
            image_prop->gimp_minor_version = l_param.int_val2;
            image_prop->gimp_micro_version = l_param.int_val3;
            break;
       case PROP_TYPE:
            switch(l_param.int_val1)
            {
              case XJT_RGB:
                image_prop->image_type = GIMP_RGB;
                break;
              case XJT_GRAY:
                image_prop->image_type = GIMP_GRAY;
                break;
              default:
                g_printerr ("XJT: PRP unsupported image_type %d\n",
                            (int)l_param.int_val1);
                return -1;
                break;
            }
            break;
       case PROP_DIMENSION:
            image_prop->image_width = l_param.int_val1;
            image_prop->image_height = l_param.int_val2;
            break;
       case PROP_UNIT:
            image_prop->unit = p_to_GimpUnit(l_param.int_val1);
            break;
       case PROP_GUIDES:
            l_guide_prop = p_new_guide_prop();
            if(l_guide_prop)
            {
              l_guide_prop->next = image_prop->guide_props;
              image_prop->guide_props = l_guide_prop;

              l_guide_prop->position = l_param.int_val1;
              l_guide_prop->orientation = p_to_GimpOrientation(l_param.int_val2);
            }
            break;
       case PROP_PARASITES:
            l_parasite_prop = p_find_parasite(image_prop->parasite_props, l_param.int_val1);
            if(l_parasite_prop == NULL)
            {
              l_parasite_prop = p_new_parasite_prop();
              l_parasite_prop->next = image_prop->parasite_props;
              image_prop->parasite_props = l_parasite_prop;
            }

            if(l_parasite_prop)
            {
              l_parasite_prop->parasite_id = l_param.int_val1;
              l_parasite_prop->parasite_type = XJT_IMAGE_PARASITE;
              l_parasite_prop->obj_pos = 0;
            }
            break;
       case PROP_RESOLUTION:
            image_prop->xresolution = l_param.flt_val1;
            image_prop->yresolution = l_param.flt_val2;
            break;
       case PROP_TATTOO:
            image_prop->tattoo = l_param.int_val1;
            break;
       case PROP_TATTOO_STATE:
            image_prop->tattoo_state = l_param.int_val1;
            break;
       default :
            /* g_printerr ("XJT: Warning PRP unexpected token in line:\n%s\n", scan_ptr); */
            /* return -1; */ /* skip unknow tokens */
            break;
     }
  }
}       /* end p_scann_image_prop */


/* ============================================================================
 * p_scann_parasite_prop
 *   scann one inputline for parasite properties
 * ============================================================================
 */
static gint
p_scann_parasite_prop (const gchar   *scan_ptr,
                       t_image_props *image_prop)
{

  t_parasite_props  *l_new_prop;
  gchar          *l_ptr;
  t_param_prop    l_param;
  t_proptype      l_prop_id;
  gint32          l_parasite_id;

  l_parasite_id = strtol(&scan_ptr[1], &l_ptr, 10);   /*  Scan integer (base = 10)  */
  if(l_ptr == scan_ptr)
  {
     g_printerr ("XJT: PRP file parasite# missing, scanned bad line:\n%s\n",
                 scan_ptr);
     return -1;
  }

  l_new_prop = p_find_parasite(image_prop->parasite_props, l_parasite_id);
  if(l_new_prop == NULL)
  {
     l_new_prop = p_new_parasite_prop();
     /* add the new element to the parasite_props list of the image */
     l_new_prop->next = image_prop->parasite_props;
     image_prop->parasite_props = l_new_prop;
  }

  if(l_new_prop == NULL)
  {
     return -1;
  }

  l_new_prop->parasite_id = l_parasite_id;

  while(1)
  {
     l_ptr = p_skip_blanks(l_ptr);
     l_ptr = p_scann_token(l_ptr, &l_param, &l_prop_id);

     switch(l_prop_id)
     {
       case PROP_END:
            return 0;
            break;
       case PROP_NAME:
            l_new_prop->name  = l_param.string_val;
            break;
       case PROP_PARASITE_FLAGS:
            l_new_prop->flags = l_param.int_val1;
            break;
       default :
            /* g_printerr ("XJT: PRP file scanned bad line:\n%s\n", scan_ptr); */
            /* return -1; */ /* skip unknow tokens */
            break;
     }
  }
}       /* end p_scann_parasite_prop */


/* ============================================================================
 * p_scann_path_prop
 *   scann one inputline for path properties
 * ============================================================================
 */
static gint
p_scann_path_prop (gchar         *scan_ptr,
                   t_image_props *image_prop)
{
  t_path_props  *l_new_prop;
  gchar          *l_ptr;
  t_param_prop    l_param;
  t_proptype      l_prop_id;

  l_new_prop = p_new_path_prop ();

  /* add the new element to the path_props list */
  l_new_prop->next = image_prop->path_props;
  image_prop->path_props = l_new_prop;

  if (strncmp (scan_ptr, "PATH", strlen ("PATH")) != 0)
    {
      g_printerr ("XJT: PRP scanned bad line:\n%s\n", scan_ptr);
      return -1;
    }
  l_ptr = scan_ptr + strlen ("PATH");

  while (1)
    {
      l_ptr = p_skip_blanks (l_ptr);
      l_ptr = p_scann_token (l_ptr, &l_param, &l_prop_id);

      switch (l_prop_id)
        {
        case PROP_END:
          return 0;
          break;
        case PROP_PATH_TYPE:
          l_new_prop->path_type = p_to_GimpPathType(l_param.int_val1);
          break;
        case PROP_PATH_LOCKED:
          l_new_prop->path_locked = l_param.int_val1;
          break;
        case PROP_TATTOO:
          l_new_prop->tattoo = l_param.int_val1;
          break;
        case PROP_NAME:
          l_new_prop->name  = l_param.string_val;
          break;
        case PROP_PATH_CURRENT:
          l_new_prop->current_flag = l_param.int_val1;
          break;
        case PROP_PATH_POINTS:
          l_new_prop->num_points = l_param.num_fvals;
          l_new_prop->path_points = l_param.flt_val_list;
          break;
        default :
          /* g_printerr ("XJT: PRP file scanned bad line:\n%s\n", scan_ptr); */
          /* return -1; */ /* skip unknow tokens */
          break;
        }
    }

}       /* end p_scann_path_prop */


/* ============================================================================
 * p_add_paths
 * ============================================================================
 */
static void
p_add_paths (gint32        image_id,
             t_path_props *path_props)
{
  gchar     *l_current_pathname = NULL;
  t_path_props  *l_prop;

  l_prop = path_props;
  while (l_prop)
    {
      if(l_prop->num_points > 0)
        {
          if(xjt_debug)
            printf("XJT: p_add_path name:%s num_points:%d\n", l_prop->name,
                   (int)l_prop->num_points);
          if(xjt_debug) printf("XJT:                :path_type %d point[0]:%f\n",
                               (int)l_prop->path_type, (float)l_prop->path_points[0]);
          if(l_prop->current_flag)
            {
              l_current_pathname = l_prop->name;
            }
          gimp_path_set_points(image_id, l_prop->name,
                               l_prop->path_type, l_prop->num_points, l_prop->path_points);
          gimp_path_set_locked(image_id, l_prop->name, l_prop->path_locked);
          if(l_prop->tattoo >= 0)
            {
              gimp_path_set_tattoo(image_id, l_prop->name, l_prop->tattoo);
            }
        }
      l_prop = (t_path_props *)l_prop->next;
    }

  if(l_current_pathname)
    {
      if(xjt_debug) printf("XJT: p_add_path current:%s\n", l_current_pathname);
      gimp_path_set_current(image_id, l_current_pathname);
    }
}       /* end p_add_paths */


static gchar *
p_load_linefile (const gchar *filename,
                 gint32      *len)
{
  FILE *l_fp;
  struct stat  stat_buf;
  gchar  *l_file_buff;
  gint32  l_idx;

  *len = 0;
  /* get file length */
  if (0 != g_stat(filename, &stat_buf))
  {
    return(NULL);
  }
  *len = stat_buf.st_size;

  l_file_buff = g_malloc0(*len +1);

  /* read file into buffer */
  l_fp = g_fopen(filename, "rb");
  if(l_fp == NULL)
  {
    return(NULL);
  }
  fread(l_file_buff, *len, 1, l_fp);
  fclose(l_fp);

  /* replace all '\n' characters by '\0' */
  for(l_idx = 0; l_idx < *len; l_idx++)
  {
    if(l_file_buff[l_idx] == '\n')
    {
      l_file_buff[l_idx] = '\0';
    }
  }

  return(l_file_buff);
}


static gint32
p_next_lineindex (const gchar *file_buff,
                  gint32       max_len,
                  gint32       pos)
{
  if (pos < 0)
    return (-1);

  while (pos < max_len)
    {
      if (file_buff[pos] == '\0')
        {
          pos++;
          break;
        }
      pos++;
    }
  return (pos >= max_len) ? -1 : pos;
}

/* ============================================================================
 * p_load_prop_file
 *   read all properties from file "PRP"
 *   and return the information in a t_image_props stucture
 * ============================================================================
 */

static t_image_props *
p_load_prop_file (const gchar *prop_filename)
{
  gint32 l_filesize;
  gint32 l_line_idx;
  gchar *l_file_buff;
  gchar *l_line_ptr;
  gchar *l_ptr;
  t_image_props *l_image_prop;
  gint  l_rc;

  if(xjt_debug) printf("p_load_prop_file: %s\n", prop_filename);

  l_rc = -1;
  l_image_prop = p_new_image_prop();

  l_file_buff = p_load_linefile(prop_filename, &l_filesize);
  if(l_file_buff == NULL)
  {
    g_message(_("Error: Could not read XJT property file '%s'."),
               gimp_filename_to_utf8 (prop_filename));
    goto cleanup;
  }
  if(l_filesize == 0)
  {
    g_message(_("Error: XJT property file '%s' is empty."),
               gimp_filename_to_utf8 (prop_filename));
    goto cleanup;
  }

  /* parse 1.st line (image properties) */
  l_line_idx = 0;
  l_line_ptr = l_file_buff;

  if(xjt_debug) printf("\nXJT:PRP_LINE:%s:\n", l_line_ptr);
  l_rc = p_scann_image_prop(l_line_ptr, l_image_prop);
  if(l_rc != 0)
  {
    goto cleanup;
  }

  while(1)
  {
     l_line_idx = p_next_lineindex(l_file_buff, l_filesize, l_line_idx);

     if(l_line_idx < 0)
     {
        break;   /* end of file */
     }
     l_line_ptr = &l_file_buff[l_line_idx];

     if(xjt_debug) printf("\nXJT:PRP_LINE:%s:\n", l_line_ptr);

     l_ptr = p_skip_blanks(&l_line_ptr[0]);

     if(*l_ptr == '#')  continue;  /* skip commentlines */
     if(*l_ptr == '\n') continue;  /* skip empty lines */
     if(*l_ptr == '\0') continue;  /* skip empty lines */

     if((*l_ptr == 'l') || (*l_ptr == 'L'))
     {
         l_rc = p_scann_layer_prop(l_line_ptr, l_image_prop);
         l_image_prop->n_layers++;
     }
     else
     {
       if((*l_ptr == 'c') || (*l_ptr == 'm'))
       {
         l_rc = p_scann_channel_prop(l_line_ptr, l_image_prop);
         l_image_prop->n_channels++;
       }
       else
       {
         if(*l_ptr == 'p')
         {
           l_rc = p_scann_parasite_prop(l_line_ptr, l_image_prop);
         }
         else
         {
           if(*l_ptr == 'P')
           {
             l_rc = p_scann_path_prop(l_line_ptr, l_image_prop);
           }
           else
           {
             g_printerr ("XJT: Warning, undefined PRP line scanned:\n%s\n",
                         l_line_ptr);
             /* goto cleanup; */
           }
         }
       }
     }
  }


cleanup:
  g_free(l_file_buff);

  return (l_rc) ? NULL : l_image_prop;
}       /* end p_load_prop_file */


/* ---------------------- LOAD  -------------------------- */

static gint32
load_xjt_image (const gchar *filename)
{
  int     l_rc;
  int     l_len;
  gchar  *l_dirname;
  gchar  *l_prop_file;
  gchar  *l_jpg_file;
  gchar  *l_cmd;

  gint32 *l_layers_list;
  gint32 *l_channels_list;
  gint32  l_layer_id;
  gint32  l_channel_id;
  gint32  l_image_id;
  gint32  l_fsel_attached_to_id;         /* the drawable id where the floating selection is attached to */
  gint32  l_fsel_id;                     /* the drawable id of the floating selection itself */
  gint32  l_active_layer_id;
  gint32  l_active_channel_id;
  t_image_props   *l_image_prp_ptr;
  t_layer_props   *l_layer_prp_ptr;
  t_channel_props *l_channel_prp_ptr;
  t_guide_props   *l_guide_prp_ptr;

  l_rc = -1;  /* init retcode to Errorstate */
  l_image_id = -1;
  l_layers_list = NULL;
  l_channels_list = NULL;
  l_image_prp_ptr = NULL;
  l_dirname = NULL;
  l_prop_file = NULL;
  l_jpg_file = NULL;
  l_active_layer_id = -1;
  l_active_channel_id = -1;
  l_fsel_attached_to_id = -1;    /* -1  assume fsel is not available (and not attached to any drawable) */
  l_fsel_id = -1;                /* -1  assume there is no floating selection */

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (filename));

  /* create temporary directory */
  l_dirname = gimp_temp_name (".tmpdir");
  l_prop_file = g_strdup_printf("%s%cPRP", l_dirname, G_DIR_SEPARATOR);

  if(g_mkdir(l_dirname, 0777) != 0)
    {
      g_message (_("Could not create working folder '%s': %s"),
                  gimp_filename_to_utf8 (l_dirname), g_strerror (errno));
      goto cleanup;
    }


  /* prepare for extract tar files directly from filename into l_dirname */
  if((*filename == G_DIR_SEPARATOR)
     || (filename[1] == ':'))
    {
      /* filename with absolute path */
      l_cmd = g_strdup_printf("cd %s; tar -xf \"%s\"; cd ..", l_dirname, filename);
    }
  else
    {
      /* filename with relative path */
      l_cmd = g_strdup_printf("cd %s; tar -xf \"..%c%s\" *; cd ..", l_dirname, G_DIR_SEPARATOR, filename);
    }

  l_len = strlen(filename);
  if(l_len > 3)
    {
      /* call optional extracompression programs gzip or bzip2
       * (depends on filename's extension)
       *
       * used gzip options: (bzip2 uses the same options)
       *     -c --stdout --to-stdout
       *          Write  output  on  standard  output
       *     -d --decompress --uncompress
       *          Decompress.
       *     -f --force
       *           Force compression or decompression even if the file
       */
      if(strcmp(&filename[l_len - 3], "bz2") == 0)
        {
          g_free(l_cmd);
          l_cmd = g_strdup_printf("bunzip2 <\"%s\" >\"%s%carc.tar\"",filename , l_dirname, G_DIR_SEPARATOR);
          l_rc = p_system(l_cmd);
          g_free(l_cmd);
          l_cmd = g_strdup_printf("cd %s; tar -xf arc.tar; cd ..", l_dirname);
        }
      else if(strcmp(&filename[l_len - 2], "gz") == 0)
        {
          g_free(l_cmd);
          l_cmd = g_strdup_printf("gzip -cdf <\"%s\" >\"%s%carc.tar\"",filename , l_dirname, G_DIR_SEPARATOR);
          l_rc = p_system(l_cmd);
          g_free(l_cmd);
          l_cmd = g_strdup_printf("cd %s; tar -xf arc.tar; cd ..", l_dirname);
        }
    }


  /* now we can extract the unpacked tar archive */
  l_rc = p_system(l_cmd);
  g_free(l_cmd);

  if (l_rc != 0)
    goto cleanup;

  /* check and read Property file (PRP must exist in each xjt archive) */
  l_image_prp_ptr = p_load_prop_file(l_prop_file);
  if (l_image_prp_ptr == NULL)
    {  l_rc = -1;
    goto cleanup;
    }


  /* create new image (with type and size values from the Property file) */
  l_image_id = gimp_image_new (l_image_prp_ptr->image_width,
                               l_image_prp_ptr->image_height,
                               l_image_prp_ptr->image_type);
  if(l_image_id < 0)
    { l_rc = -1;
    goto cleanup;
    }

  gimp_image_set_filename (l_image_id, filename);
  gimp_image_set_resolution  (l_image_id,
                              l_image_prp_ptr->xresolution,
                              l_image_prp_ptr->yresolution);
  gimp_image_set_unit(l_image_id, l_image_prp_ptr->unit);

  p_check_and_add_parasite(l_image_id,
                           l_dirname,
                           l_image_prp_ptr->parasite_props,
                           0,
                           XJT_IMAGE_PARASITE);
  /* load all layers */

  for(l_layer_prp_ptr = l_image_prp_ptr->layer_props;
      l_layer_prp_ptr != NULL;
      l_layer_prp_ptr = (t_layer_props *)l_layer_prp_ptr->next)
    {
      l_jpg_file = g_strdup_printf("%s%cl%d.jpg", l_dirname, G_DIR_SEPARATOR, (int)l_layer_prp_ptr->layer_pos);
      if(xjt_debug) printf("XJT-DEBUG: loading layer from file %s\n", l_jpg_file);

      l_layer_id = xjpg_load_layer (l_jpg_file,
                                    l_image_id,
                                    l_image_prp_ptr->image_type,
                                    l_layer_prp_ptr->name,
                                    l_layer_prp_ptr->opacity,
                                    l_layer_prp_ptr->mode);

      g_free(l_jpg_file);
      if(l_layer_id < 0)
        {
          l_rc = -1;
          break;
        }

      if(l_layer_prp_ptr->floating_selection)
        {
          l_fsel_id = l_layer_id;    /* this layer is the floating selection */
        }
      else
        {
          /* add the layer on top of the images layerstak */
          gimp_image_add_layer (l_image_id, l_layer_id, 0);

          if(l_layer_prp_ptr->floating_attached)
            {
              l_fsel_attached_to_id = l_layer_id;    /* the floating selection is attached to this layer */
            }
        }

      /* check for alpha channel */
      if(l_layer_prp_ptr->has_alpha)
        {
          l_jpg_file = g_strdup_printf("%s%cla%d.jpg", l_dirname, G_DIR_SEPARATOR, (int)l_layer_prp_ptr->layer_pos);
          if(xjt_debug) printf("XJT-DEBUG: loading alpha-channel from file %s\n", l_jpg_file);

          if( xjpg_load_layer_alpha (l_jpg_file, l_image_id, l_layer_id) != 0)
            {
              l_rc = -1;
              break;
            }
          g_free(l_jpg_file);
        }

      /* adjust offsets and other layerproperties */
      gimp_layer_set_offsets(l_layer_id, l_layer_prp_ptr->offx, l_layer_prp_ptr->offy);
      gimp_drawable_set_visible (l_layer_id, l_layer_prp_ptr->visible);
      gimp_drawable_set_linked (l_layer_id, l_layer_prp_ptr->linked);
      gimp_layer_set_lock_alpha (l_layer_id, l_layer_prp_ptr->lock_alpha);
      if (l_layer_prp_ptr->tattoo >= 0)
        {
         gimp_drawable_set_tattoo (l_layer_id, l_layer_prp_ptr->tattoo);
        }

      if (l_layer_prp_ptr->active_layer)
        {
          l_active_layer_id = l_layer_id;
        }

      /* Handle layer parasites */
      p_check_and_add_parasite (l_layer_id,
                                l_dirname,
                                l_image_prp_ptr->parasite_props,
                                l_layer_prp_ptr->layer_pos,
                                XJT_LAYER_PARASITE);


      /* search for the properties of the layermask */
      for (l_channel_prp_ptr = l_image_prp_ptr->mask_props;
           l_channel_prp_ptr != NULL;
           l_channel_prp_ptr = (t_channel_props *) l_channel_prp_ptr->next)
        {
          if (l_channel_prp_ptr->channel_pos == l_layer_prp_ptr->layer_pos)
            {
              /* layermask properties found: load the layermask */
              l_jpg_file = g_strdup_printf ("%s%clm%d.jpg", l_dirname, G_DIR_SEPARATOR, (int)l_layer_prp_ptr->layer_pos);
              if(xjt_debug) printf("XJT-DEBUG: loading layer-mask from file %s\n", l_jpg_file);

              l_channel_id = gimp_layer_create_mask (l_layer_id, 0 /* mask_type 0 = WHITE_MASK */ );

              /* load should overwrite the layer_mask with data from jpeg file */

              l_channel_id = xjpg_load_channel (l_jpg_file,
                                                l_image_id,
                                                l_channel_id,
                                                l_channel_prp_ptr->name,
                                                l_channel_prp_ptr->opacity,
                                                l_channel_prp_ptr->color_r,
                                                l_channel_prp_ptr->color_g,
                                                l_channel_prp_ptr->color_b);
              g_free(l_jpg_file);
              if(l_channel_id >= 0)
                {

                  /* attach the layer_mask to the layer (with identical offsets) */
                  gimp_layer_add_mask (l_layer_id, l_channel_id);

                  if (l_channel_prp_ptr->floating_attached)
                    {
                      l_fsel_attached_to_id = l_channel_id;    /* the floating selection is attached to this layer_mask */
                    }

                  if (l_channel_prp_ptr->tattoo >= 0)
                    {
                      gimp_drawable_set_tattoo (l_channel_id,
                                                l_channel_prp_ptr->tattoo);
                    }

                  /* gimp_layer_set_offsets(l_channel_id, l_layer_prp_ptr->offx, l_layer_prp_ptr->offy); */

                  gimp_layer_set_apply_mask (l_layer_id, l_layer_prp_ptr->apply_mask);
                  gimp_layer_set_edit_mask  (l_layer_id, l_layer_prp_ptr->edit_mask);
                  gimp_layer_set_show_mask  (l_layer_id, l_layer_prp_ptr->show_mask);

                  /* Handle layermask parasites */
                  p_check_and_add_parasite (l_channel_id,
                                            l_dirname,
                                            l_image_prp_ptr->parasite_props,
                                            l_channel_prp_ptr->channel_pos,
                                            XJT_LAYER_MASK_PARASITE);
                }
              break;
            }
        }    /* end search for layermask */
    }

  /* load all channels */
  for (l_channel_prp_ptr = l_image_prp_ptr->channel_props;
       l_channel_prp_ptr != NULL;
       l_channel_prp_ptr = (t_channel_props *) l_channel_prp_ptr->next)
    {
      l_jpg_file = g_strdup_printf ("%s%cc%d.jpg", l_dirname, G_DIR_SEPARATOR,
                                    (int) l_channel_prp_ptr->channel_pos);
      if (xjt_debug) printf ("XJT-DEBUG: loading channel from file %s\n",
                             l_jpg_file);

      l_channel_id = xjpg_load_channel (l_jpg_file,
                                        l_image_id,
                                        -1,
                                        l_channel_prp_ptr->name,
                                        l_channel_prp_ptr->opacity,
                                        l_channel_prp_ptr->color_r,
                                        l_channel_prp_ptr->color_g,
                                        l_channel_prp_ptr->color_b);

      g_free (l_jpg_file);
      if (l_channel_id < 0)
        {
          l_rc = -1;
          break;
        }

      /* Handle channel parasites */
      p_check_and_add_parasite (l_channel_id,
                                l_dirname,
                                l_image_prp_ptr->parasite_props,
                                l_channel_prp_ptr->channel_pos,
                                XJT_CHANNEL_PARASITE);

      if (l_channel_prp_ptr->tattoo >= 0)
        {
          gimp_drawable_set_tattoo (l_channel_id, l_channel_prp_ptr->tattoo);
        }

      if (l_channel_prp_ptr->selection)
        {
          if (xjt_debug) printf ("XJT-DEBUG: SELECTION loaded channel id = %d\n",
                                 (int) l_channel_id);

          gimp_selection_load (l_channel_id);

          /* delete the channel after load into selection */
          gimp_drawable_delete (l_channel_id);
        }
      else
        {
          /* add channel on top of the channelstack */
          gimp_image_add_channel (l_image_id, l_channel_id, 0);

          /* adjust offsets and other channelproperties */
          gimp_drawable_set_visible (l_channel_id, l_channel_prp_ptr->visible);
          gimp_channel_set_show_masked (l_channel_id, l_channel_prp_ptr->show_masked);

          if (l_channel_prp_ptr->floating_attached)
            {
              l_fsel_attached_to_id = l_channel_id;    /* the floating_selection is attached to this channel */
            }

          if (l_channel_prp_ptr->active_channel)
            {
              l_active_channel_id = l_channel_id;
            }
        }
    }

  /* attach the floating selection... */
  if ((l_fsel_id >= 0) && (l_fsel_attached_to_id >= 0))
    {
      if (xjt_debug) printf("XJT-DEBUG: attaching floating_selection id=%d to id %d\n",
                           (int)l_fsel_id, (int)l_fsel_attached_to_id);
      if (gimp_floating_sel_attach (l_fsel_id, l_fsel_attached_to_id) < 0)
        {
          /* in case of error add floating_selection like an ordinary layer
           * (if patches are not installed you'll get the error for sure)
           */
          printf("XJT: floating_selection is added as top-layer (attach failed)\n");
          gimp_image_add_layer (l_image_id, l_fsel_id, 0);
        }
    }

  /* set active layer/channel */
  if (l_active_channel_id >= 0)
    {
      if (xjt_debug) printf("SET active channel\n");

      gimp_image_set_active_channel (l_image_id, l_active_channel_id);
    }

  if (l_active_layer_id >= 0)
    {
      if (xjt_debug) printf("SET active layer\n");

      gimp_image_set_active_layer (l_image_id, l_active_layer_id);
    }

  /* set guides */
  for (l_guide_prp_ptr = l_image_prp_ptr->guide_props;
       l_guide_prp_ptr != NULL;
       l_guide_prp_ptr = (t_guide_props *) l_guide_prp_ptr->next)
    {
      if (l_guide_prp_ptr->orientation == GIMP_ORIENTATION_HORIZONTAL)
        gimp_image_add_hguide (l_image_id, l_guide_prp_ptr->position);
      else
        gimp_image_add_vguide (l_image_id, l_guide_prp_ptr->position);
    }

  /* create paths */
  if (l_image_prp_ptr->path_props)
    {
      p_add_paths (l_image_id, l_image_prp_ptr->path_props);
    }

  /* set tattoo_state */
  if (l_image_prp_ptr->tattoo_state > 0)
    {
      gimp_image_set_tattoo_state (l_image_id, l_image_prp_ptr->tattoo_state);
    }

 cleanup:

  g_free (l_layers_list);
  g_free (l_channels_list);

  /* remove the temorary directory */
  l_cmd = g_strdup_printf ("rm -rf \"%s\"", l_dirname);
  p_system (l_cmd);
  g_free (l_cmd);

  g_free (l_dirname);
  g_free (l_prop_file);
  g_free (l_jpg_file);

  if (l_rc == 0)
    {
      return l_image_id;             /* all done OK */
    }

  /* destroy the tmp image */
  gimp_image_delete (l_image_id);
  return -1;
}
