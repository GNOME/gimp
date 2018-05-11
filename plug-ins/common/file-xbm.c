/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * X10 and X11 bitmap (XBM) loading and exporting file filter for GIMP.
 * XBM code Copyright (C) 1998 Gordon Matzigkeit
 *
 * The XBM reading and writing code was written from scratch by Gordon
 * Matzigkeit <gord@gnu.org> based on the XReadBitmapFile(3X11) manual
 * page distributed with X11R6 and by staring at valid XBM files.  It
 * does not contain any code written for other XBM file loaders.
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

/* Release 1.0, 1998-02-04, Gordon Matzigkeit <gord@gnu.org>:
 *   - Load and save X10 and X11 bitmaps.
 *   - Allow the user to specify the C identifier prefix.
 *
 * TODO:
 *   - Parsing is very tolerant, and the algorithms are quite hairy, so
 *     load_image should be carefully tested to make sure there are no XBM's
 *     that fail.
 */

/* Set this for debugging. */
/* #define VERBOSE 2 */

#include "config.h"

#include <errno.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC      "file-xbm-load"
#define SAVE_PROC      "file-xbm-save"
#define PLUG_IN_BINARY "file-xbm"
#define PLUG_IN_ROLE   "gimp-file-xbm"


/* Wear your GIMP with pride! */
#define DEFAULT_USE_COMMENT TRUE
#define MAX_COMMENT         72
#define MAX_MASK_EXT        32

/* C identifier prefix. */
#define DEFAULT_PREFIX "bitmap"
#define MAX_PREFIX     64

/* Whether or not to export as X10 bitmap. */
#define DEFAULT_X10_FORMAT FALSE

typedef struct _XBMSaveVals
{
  gchar    comment[MAX_COMMENT + 1];
  gint     x10_format;
  gint     use_hot;
  gint     x_hot;
  gint     y_hot;
  gchar    prefix[MAX_PREFIX + 1];
  gboolean write_mask;
  gchar    mask_ext[MAX_MASK_EXT + 1];
} XBMSaveVals;

static XBMSaveVals xsvals =
{
  "###",                /* comment */
  DEFAULT_X10_FORMAT,   /* x10_format */
  FALSE,
  0,                    /* x_hot */
  0,                    /* y_hot */
  DEFAULT_PREFIX,       /* prefix */
  FALSE,                /* write_mask */
  "-mask"
};


/* Declare some local functions.
 */
static void      query                   (void);
static void      run                     (const gchar      *name,
                                          gint              nparams,
                                          const GimpParam  *param,
                                          gint             *nreturn_vals,
                                          GimpParam       **return_vals);

static gint32    load_image              (const gchar      *filename,
                                          GError          **error);
static gint      save_image              (GFile            *file,
                                          const gchar      *prefix,
                                          const gchar      *comment,
                                          gboolean          save_mask,
                                          gint32            image_ID,
                                          gint32            drawable_ID,
                                          GError          **error);
static gboolean  save_dialog             (gint32            drawable_ID);

static gboolean  print                   (GOutputStream    *output,
                                          GError          **error,
                                          const gchar      *format,
                                          ...) G_GNUC_PRINTF (3, 4);

#if 0
/* DISABLED - see http://bugzilla.gnome.org/show_bug.cgi?id=82763 */
static void      comment_entry_callback  (GtkWidget        *widget,
                                          gpointer          data);
#endif
static void      prefix_entry_callback   (GtkWidget        *widget,
                                          gpointer          data);
static void      mask_ext_entry_callback (GtkWidget        *widget,
                                          gpointer          data);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

MAIN ()

#ifdef VERBOSE
static int verbose = VERBOSE;
#endif


static void
query (void)
{
  static const GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,  "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_STRING, "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING, "raw-filename", "The name entered"             }
  };

  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE,  "image",        "Output image" }
  };

  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",       "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",          "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",       "Drawable to export" },
    { GIMP_PDB_STRING,   "filename",       "The name of the file to export" },
    { GIMP_PDB_STRING,   "raw-filename",   "The name entered" },
    { GIMP_PDB_STRING,   "comment",        "Image description (maximum 72 bytes)" },
    { GIMP_PDB_INT32,    "x10",            "Export in X10 format" },
    { GIMP_PDB_INT32,    "x-hot",          "X coordinate of hotspot" },
    { GIMP_PDB_INT32,    "y-hot",          "Y coordinate of hotspot" },
    { GIMP_PDB_STRING,   "prefix",         "Identifier prefix [determined from filename]"},
    { GIMP_PDB_INT32,    "write-mask",     "(0 = ignore, 1 = save as extra file)" },
    { GIMP_PDB_STRING,   "mask-extension", "Extension of the mask file" }
  } ;

  gimp_install_procedure (LOAD_PROC,
                          "Load a file in X10 or X11 bitmap (XBM) file format",
                          "Load a file in X10 or X11 bitmap (XBM) file format.  XBM is a lossless format for flat black-and-white (two color indexed) images.",
                          "Gordon Matzigkeit",
                          "Gordon Matzigkeit",
                          "1998",
                          N_("X BitMap image"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_file_handler_mime (LOAD_PROC, "image/x-xbitmap");
  gimp_register_load_handler (LOAD_PROC,
                              "xbm,icon,bitmap",
                              "");

  gimp_install_procedure (SAVE_PROC,
                          "Export a file in X10 or X11 bitmap (XBM) file format",
                          "Export a file in X10 or X11 bitmap (XBM) file format.  XBM is a lossless format for flat black-and-white (two color indexed) images.",
                          "Gordon Matzigkeit",
                          "Gordon Matzigkeit",
                          "1998",
                          N_("X BitMap image"),
                          "INDEXED",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_register_file_handler_mime (SAVE_PROC, "image/x-xbitmap");
  gimp_register_file_handler_uri (SAVE_PROC);
  gimp_register_save_handler (SAVE_PROC, "xbm,icon,bitmap", "");
}

static gchar *
init_prefix (const gchar *filename)
{
  gchar *p, *prefix;
  gint len;

  prefix = g_path_get_basename (filename);

  memset (xsvals.prefix, 0, sizeof (xsvals.prefix));

  if (prefix)
    {
      /* Strip any extension. */
      p = strrchr (prefix, '.');
      if (p && p != prefix)
        len = MIN (MAX_PREFIX, p - prefix);
      else
        len = MAX_PREFIX;

      strncpy (xsvals.prefix, prefix, len);
      g_free (prefix);
    }

  return xsvals.prefix;
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[2];
  GimpRunMode        run_mode;
  GimpPDBStatusType  status        = GIMP_PDB_SUCCESS;
  gint32             image_ID;
  gint32             drawable_ID;
  GimpParasite      *parasite      = NULL;
  gchar             *mask_basename = NULL;
  GError            *error         = NULL;
  GimpExportReturn   export        = GIMP_EXPORT_CANCEL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  strncpy (xsvals.comment, "Created with GIMP", MAX_COMMENT);

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

#ifdef VERBOSE
  if (verbose)
    printf ("XBM: RUN %s\n", name);
#endif

  if (strcmp (name, LOAD_PROC) == 0)
    {
      image_ID = load_image (param[1].data.d_string, &error);

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
      image_ID    = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          gimp_ui_init (PLUG_IN_BINARY, FALSE);

          export = gimp_export_image (&image_ID, &drawable_ID, "XBM",
                                      GIMP_EXPORT_CAN_HANDLE_BITMAP |
                                      GIMP_EXPORT_CAN_HANDLE_ALPHA);

          if (export == GIMP_EXPORT_CANCEL)
            {
              values[0].data.d_status = GIMP_PDB_CANCEL;
              return;
            }
          break;

        default:
          break;
        }

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          /*  Possibly retrieve data  */
          gimp_get_data (SAVE_PROC, &xsvals);

          /* Always override the prefix with the filename. */
          mask_basename = g_strdup (init_prefix (param[3].data.d_string));
          break;

        case GIMP_RUN_NONINTERACTIVE:
          /*  Make sure all the required arguments are there!  */
          if (nparams < 5)
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
          else
            {
              gint i = 5;

              if (nparams > i)
                {
                  memset (xsvals.comment, 0, sizeof (xsvals.comment));
                  strncpy (xsvals.comment, param[i].data.d_string,
                           MAX_COMMENT);
                }

              i ++;
              if (nparams > i)
                xsvals.x10_format = (param[i].data.d_int32) ? TRUE : FALSE;

              i += 2;
              if (nparams > i)
                {
                  /* They've asked for a hotspot. */
                  xsvals.use_hot = TRUE;
                  xsvals.x_hot = param[i - 1].data.d_int32;
                  xsvals.y_hot = param[i].data.d_int32;
                }

              mask_basename = g_strdup (init_prefix (param[3].data.d_string));

              i ++;
              if (nparams > i)
                {
                  memset (xsvals.prefix, 0, sizeof (xsvals.prefix));
                  strncpy (xsvals.prefix, param[i].data.d_string,
                           MAX_PREFIX);
                }

              i += 2;
              if (nparams > i)
                {
                  xsvals.write_mask = param[i - 1].data.d_int32;
                  memset (xsvals.mask_ext, 0, sizeof (xsvals.mask_ext));
                  strncpy (xsvals.mask_ext, param[i].data.d_string,
                           MAX_MASK_EXT);
                }

              i ++;
              /* Too many arguments. */
              if (nparams > i)
                status = GIMP_PDB_CALLING_ERROR;
            }
          break;

        default:
          break;
        }

      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          /* Get the parasites */
          parasite = gimp_image_get_parasite (image_ID, "gimp-comment");

          if (parasite)
            {
              gint size = gimp_parasite_data_size (parasite);

              strncpy (xsvals.comment,
                       gimp_parasite_data (parasite), MIN (size, MAX_COMMENT));
              xsvals.comment[MIN (size, MAX_COMMENT) + 1] = 0;

              gimp_parasite_free (parasite);
            }

          parasite = gimp_image_get_parasite (image_ID, "hot-spot");

          if (parasite)
            {
              gint x, y;

              if (sscanf (gimp_parasite_data (parasite), "%i %i", &x, &y) == 2)
               {
                 xsvals.use_hot = TRUE;
                 xsvals.x_hot = x;
                 xsvals.y_hot = y;
               }
             gimp_parasite_free (parasite);
           }

          /*  Acquire information with a dialog  */
          if (! save_dialog (drawable_ID))
            status = GIMP_PDB_CANCEL;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          GFile *file = g_file_new_for_uri (param[3].data.d_string);
          GFile *mask_file;
          GFile *dir;
          gchar *mask_prefix;
          gchar *temp;

          dir = g_file_get_parent (file);
          temp = g_strdup_printf ("%s%s.xbm", mask_basename, xsvals.mask_ext);

          mask_file = g_file_get_child (dir, temp);

          g_free (temp);
          g_object_unref (dir);

          /* Change any non-alphanumeric prefix characters to underscores. */
          for (temp = xsvals.prefix; *temp; temp++)
            if (! g_ascii_isalnum (*temp))
              *temp = '_';

          mask_prefix = g_strdup_printf ("%s%s",
                                         xsvals.prefix, xsvals.mask_ext);

          for (temp = mask_prefix; *temp; temp++)
            if (! g_ascii_isalnum (*temp))
              *temp = '_';

          if (save_image (file,
                          xsvals.prefix,
                          xsvals.comment,
                          FALSE,
                          image_ID, drawable_ID,
                          &error)

              && (! xsvals.write_mask ||
                  save_image (mask_file,
                              mask_prefix,
                              xsvals.comment,
                              TRUE,
                              image_ID, drawable_ID,
                              &error)))
            {
              /*  Store xsvals data  */
              gimp_set_data (SAVE_PROC, &xsvals, sizeof (xsvals));
            }
          else
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }

          g_free (mask_prefix);
          g_free (mask_basename);

          g_object_unref (file);
          g_object_unref (mask_file);
        }

      if (export == GIMP_EXPORT_EXPORT)
        gimp_image_delete (image_ID);
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  if (status != GIMP_PDB_SUCCESS && error)
    {
      *nreturn_vals = 2;
      values[1].type          = GIMP_PDB_STRING;
      values[1].data.d_string = error->message;
    }

  values[0].data.d_status = status;
}


/* Return the value of a digit. */
static gint
getval (gint c,
        gint base)
{
  const gchar *digits = "0123456789abcdefABCDEF";
  gint         val;

  /* Include uppercase hex digits. */
  if (base == 16)
    base = 22;

  /* Find a match. */
  for (val = 0; val < base; val ++)
    if (c == digits[val])
      return (val < 16) ? val : (val - 6);
  return -1;
}


/* Get a comment */
static gchar *
fgetcomment (FILE *fp)
{
  GString *str = NULL;
  gint comment, c;

  comment = 0;
  do
    {
      c = fgetc (fp);
      if (comment)
        {
          if (c == '*')
            {
              /* In a comment, with potential to leave. */
              comment = 1;
            }
          else if (comment == 1 && c == '/')
            {
              gchar *retval;

              /* Leaving a comment. */
              comment = 0;

              retval = g_strstrip (g_strdup (str->str));
              g_string_free (str, TRUE);
              return retval;
            }
          else
            {
              /* In a comment, with no potential to leave. */
              comment = 2;
              g_string_append_c (str, c);
            }
        }
      else
        {
          /* Not in a comment. */
          if (c == '/')
            {
              /* Potential to enter a comment. */
              c = fgetc (fp);
              if (c == '*')
                {
                  /* Entered a comment, with no potential to leave. */
                  comment = 2;
                  str = g_string_new (NULL);
                }
              else
                {
                  /* put everything back and return */
                  ungetc (c, fp);
                  c = '/';
                  ungetc (c, fp);
                  return NULL;
                }
            }
          else if (c != EOF && g_ascii_isspace (c))
            {
              /* Skip leading whitespace */
              continue;
            }
        }
    }
  while (comment && c != EOF);

  if (str)
    g_string_free (str, TRUE);

  return NULL;
}


/* Same as fgetc, but skip C-style comments and insert whitespace. */
static gint
cpp_fgetc (FILE *fp)
{
  gint comment, c;

  /* FIXME: insert whitespace as advertised. */
  comment = 0;
  do
    {
      c = fgetc (fp);
      if (comment)
        {
          if (c == '*')
            /* In a comment, with potential to leave. */
            comment = 1;
          else if (comment == 1 && c == '/')
            /* Leaving a comment. */
            comment = 0;
          else
            /* In a comment, with no potential to leave. */
            comment = 2;
        }
      else
        {
          /* Not in a comment. */
          if (c == '/')
            {
              /* Potential to enter a comment. */
              c = fgetc (fp);
              if (c == '*')
                /* Entered a comment, with no potential to leave. */
                comment = 2;
              else
                {
                  /* Just a slash in the open. */
                  ungetc (c, fp);
                  c = '/';
                }
            }
        }
    }
  while (comment && c != EOF);
  return c;
}


/* Match a string with a file. */
static gint
match (FILE        *fp,
       const gchar *s)
{
  gint c;

  do
    {
      c = fgetc (fp);
      if (c == *s)
        s ++;
      else
        break;
    }
  while (c != EOF && *s);

  if (!*s)
    return TRUE;

  if (c != EOF)
    ungetc (c, fp);
  return FALSE;
}


/* Read the next integer from the file, skipping all non-integers. */
static gint
get_int (FILE *fp)
{
  int digval, base, val, c;

  do
    c = cpp_fgetc (fp);
  while (c != EOF && ! g_ascii_isdigit (c));

  if (c == EOF)
    return 0;

  /* Check for the base. */
  if (c == '0')
    {
      c = fgetc (fp);
      if (c == 'x' || c == 'X')
        {
          c = fgetc (fp);
          base = 16;
        }
      else if (g_ascii_isdigit (c))
        base = 8;
      else
        {
          ungetc (c, fp);
          return 0;
        }
    }
  else
    base = 10;

  val = 0;
  for (;;)
    {
      digval = getval (c, base);
      if (digval == -1)
        {
          ungetc (c, fp);
          break;
        }
      val *= base;
      val += digval;
      c = fgetc (fp);
    }

  return val;
}


static gint32
load_image (const gchar  *filename,
            GError      **error)
{
  FILE         *fp;
  GeglBuffer   *buffer;
  gint32        image_ID;
  gint32        layer_ID;
  guchar       *data;
  gint          intbits;
  gint          width  = 0;
  gint          height = 0;
  gint          x_hot  = 0;
  gint          y_hot  = 0;
  gint          c, i, j, k;
  gint          tileheight, rowoffset;
  gchar        *comment;

  const guchar cmap[] =
  {
    0x00, 0x00, 0x00,           /* black */
    0xff, 0xff, 0xff            /* white */
  };

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (filename));

  fp = g_fopen (filename, "rb");
  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return -1;
    }

  comment = fgetcomment (fp);

  /* Loosely parse the header */
  intbits = height = width = 0;
  c = ' ';
  do
    {
      if (g_ascii_isspace (c))
        {
          if (match (fp, "char"))
            {
              c = fgetc (fp);
              if (g_ascii_isspace (c))
                {
                  intbits = 8;
                  continue;
                }
            }
          else if (match (fp, "short"))
            {
              c = fgetc (fp);
              if (g_ascii_isspace (c))
                {
                  intbits = 16;
                  continue;
                }
            }
        }

      if (c == '_')
        {
          if (match (fp, "width"))
            {
              c = fgetc (fp);
              if (g_ascii_isspace (c))
                {
                  width = get_int (fp);
                  continue;
                }
            }
          else if (match (fp, "height"))
            {
              c = fgetc (fp);
              if (g_ascii_isspace (c))
                {
                  height = get_int (fp);
                  continue;
                }
            }
          else if (match (fp, "x_hot"))
            {
              c = fgetc (fp);
              if (g_ascii_isspace (c))
                {
                  x_hot = get_int (fp);
                  continue;
                }
            }
          else if (match (fp, "y_hot"))
            {
              c = fgetc (fp);
              if (g_ascii_isspace (c))
                {
                  y_hot = get_int (fp);
                  continue;
                }
            }
        }

      c = cpp_fgetc (fp);
    }
  while (c != '{' && c != EOF);

  if (c == EOF)
    {
      g_message (_("'%s':\nCould not read header (ftell == %ld)"),
                 gimp_filename_to_utf8 (filename), ftell (fp));
      return -1;
    }

  if (width <= 0)
    {
      g_message (_("'%s':\nNo image width specified"),
                 gimp_filename_to_utf8 (filename));
      return -1;
    }

  if (width > GIMP_MAX_IMAGE_SIZE)
    {
      g_message (_("'%s':\nImage width is larger than GIMP can handle"),
                 gimp_filename_to_utf8 (filename));
      return -1;
    }

  if (height <= 0)
    {
      g_message (_("'%s':\nNo image height specified"),
                 gimp_filename_to_utf8 (filename));
      return -1;
    }

  if (height > GIMP_MAX_IMAGE_SIZE)
    {
      g_message (_("'%s':\nImage height is larger than GIMP can handle"),
                 gimp_filename_to_utf8 (filename));
      return -1;
    }

  if (intbits == 0)
    {
      g_message (_("'%s':\nNo image data type specified"),
                 gimp_filename_to_utf8 (filename));
      return -1;
    }

  image_ID = gimp_image_new (width, height, GIMP_INDEXED);
  gimp_image_set_filename (image_ID, filename);

  if (comment)
    {
      GimpParasite *parasite;

      parasite = gimp_parasite_new ("gimp-comment",
                                    GIMP_PARASITE_PERSISTENT,
                                    strlen (comment) + 1, (gpointer) comment);
      gimp_image_attach_parasite (image_ID, parasite);
      gimp_parasite_free (parasite);

      g_free (comment);
    }

  x_hot = CLAMP (x_hot, 0, width);
  y_hot = CLAMP (y_hot, 0, height);

  if (x_hot > 0 || y_hot > 0)
    {
      GimpParasite *parasite;
      gchar        *str;

      str = g_strdup_printf ("%d %d", x_hot, y_hot);
      parasite = gimp_parasite_new ("hot-spot",
                                    GIMP_PARASITE_PERSISTENT,
                                    strlen (str) + 1, (gpointer) str);
      g_free (str);
      gimp_image_attach_parasite (image_ID, parasite);
      gimp_parasite_free (parasite);
    }

  /* Set a black-and-white colormap. */
  gimp_image_set_colormap (image_ID, cmap, 2);

  layer_ID = gimp_layer_new (image_ID,
                             _("Background"),
                             width, height,
                             GIMP_INDEXED_IMAGE,
                             100,
                             gimp_image_get_default_new_layer_mode (image_ID));
  gimp_image_insert_layer (image_ID, layer_ID, -1, 0);

  buffer = gimp_drawable_get_buffer (layer_ID);

  /* Allocate the data. */
  tileheight = gimp_tile_height ();
  data = (guchar *) g_malloc (width * tileheight);

  for (i = 0; i < height; i += tileheight)
    {
      tileheight = MIN (tileheight, height - i);

#ifdef VERBOSE
      if (verbose > 1)
        printf ("XBM: reading %dx(%d+%d) pixel region\n", width, i,
                tileheight);
#endif

      /* Parse the data from the file */
      for (j = 0; j < tileheight; j ++)
        {
          /* Read each row. */
          rowoffset = j * width;
          for (k = 0; k < width; k ++)
            {
              /* Expand each integer into INTBITS pixels. */
              if (k % intbits == 0)
                {
                  c = get_int (fp);

                  /* Flip all the bits so that 1's become black and
                     0's become white. */
                  c ^= 0xffff;
                }

              data[rowoffset + k] = c & 1;
              c >>= 1;
            }
        }

      /* Put the data into the image. */
      gegl_buffer_set (buffer, GEGL_RECTANGLE (0, i, width, tileheight), 0,
                       NULL, data, GEGL_AUTO_ROWSTRIDE);

      gimp_progress_update ((double) (i + tileheight) / (double) height);
    }

  g_free (data);
  g_object_unref (buffer);
  fclose (fp);

  gimp_progress_update (1.0);

  return image_ID;
}

static gboolean
save_image (GFile        *file,
            const gchar  *prefix,
            const gchar  *comment,
            gboolean      save_mask,
            gint32        image_ID,
            gint32        drawable_ID,
            GError      **error)
{
  GOutputStream *output;
  GeglBuffer    *buffer;
  gint           width, height, colors, dark;
  gint           intbits, lineints, need_comma, nints, rowoffset, tileheight;
  gint           c, i, j, k, thisbit;
  gboolean       has_alpha;
  gint           bpp;
  guchar        *data = NULL;
  guchar        *cmap;
  const gchar   *intfmt;

#if 0
  if (save_mask)
    g_printerr ("%s: save_mask '%s'\n", G_STRFUNC, prefix);
  else
    g_printerr ("%s: save_image '%s'\n", G_STRFUNC, prefix);
#endif

  cmap = gimp_image_get_colormap (image_ID, &colors);

  if (! gimp_drawable_is_indexed (drawable_ID) || colors > 2)
    {
      /* The image is not black-and-white. */
      g_message (_("The image which you are trying to export as "
                   "an XBM contains more than two colors.\n\n"
                   "Please convert it to a black and white "
                   "(1-bit) indexed image and try again."));
      g_free (cmap);
      return FALSE;
    }

  has_alpha = gimp_drawable_has_alpha (drawable_ID);

  if (! has_alpha && save_mask)
    {
      g_message (_("You cannot save a cursor mask for an image\n"
                   "which has no alpha channel."));
      return FALSE;
    }

  buffer = gimp_drawable_get_buffer (drawable_ID);
  width  = gegl_buffer_get_width  (buffer);
  height = gegl_buffer_get_height (buffer);
  bpp    = gimp_drawable_bpp (drawable_ID);

  /* Figure out which color is black, and which is white. */
  dark = 0;
  if (colors > 1)
    {
      gint first, second;

      /* Maybe the second color is darker than the first. */
      first  = (cmap[0] * cmap[0]) + (cmap[1] * cmap[1]) + (cmap[2] * cmap[2]);
      second = (cmap[3] * cmap[3]) + (cmap[4] * cmap[4]) + (cmap[5] * cmap[5]);

      if (second < first)
        dark = 1;
    }

  gimp_progress_init_printf (_("Exporting '%s'"),
                             gimp_file_get_utf8_name (file));

  output = G_OUTPUT_STREAM (g_file_replace (file,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, error));
  if (output)
    {
      GOutputStream *buffered;

      buffered = g_buffered_output_stream_new (output);
      g_object_unref (output);

      output = buffered;
    }
  else
    {
      return FALSE;
    }

  /* Maybe write the image comment. */
#if 0
  /* DISABLED - see http://bugzilla.gnome.org/show_bug.cgi?id=82763 */
  /* a future version should write the comment at the end of the file */
  if (*comment)
    {
      if (! print (output, error, "/* %s */\n", comment))
        goto fail;
    }
#endif

  /* Write out the image height and width. */
  if (! print (output, error, "#define %s_width %d\n",  prefix, width) ||
      ! print (output, error, "#define %s_height %d\n", prefix, height))
    goto fail;

  /* Write out the hotspot, if any. */
  if (xsvals.use_hot)
    {
      if (! print (output, error,
                   "#define %s_x_hot %d\n", prefix, xsvals.x_hot) ||
          ! print (output, error,
                   "#define %s_y_hot %d\n", prefix, xsvals.y_hot))
        goto fail;
    }

  /* Now write the actual data. */
  if (xsvals.x10_format)
    {
      /* We can fit 9 hex shorts on a single line. */
      lineints = 9;
      intbits = 16;
      intfmt = " 0x%04x";
    }
  else
    {
      /* We can fit 12 hex chars on a single line. */
      lineints = 12;
      intbits = 8;
      intfmt = " 0x%02x";
    }

  if (! print (output, error,
               "static %s %s_bits[] = {\n  ",
               xsvals.x10_format ? "unsigned short" : "unsigned char", prefix))
    goto fail;

  /* Allocate a new set of pixels. */
  tileheight = gimp_tile_height ();
  data = (guchar *) g_malloc (width * tileheight * bpp);

  /* Write out the integers. */
  need_comma = 0;
  nints = 0;
  for (i = 0; i < height; i += tileheight)
    {
      /* Get a horizontal slice of the image. */
      tileheight = MIN (tileheight, height - i);

      gegl_buffer_get (buffer, GEGL_RECTANGLE (0, i, width, tileheight), 1.0,
                       NULL, data,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

#ifdef VERBOSE
      if (verbose > 1)
        printf ("XBM: writing %dx(%d+%d) pixel region\n",
                width, i, tileheight);
#endif

      for (j = 0; j < tileheight; j ++)
        {
          /* Write out a row at a time. */
          rowoffset = j * width * bpp;
          c = 0;
          thisbit = 0;

          for (k = 0; k < width * bpp; k += bpp)
            {
              if (k != 0 && thisbit == intbits)
                {
                  /* Output a completed integer. */
                  if (need_comma)
                    {
                      if (! print (output, error, ","))
                        goto fail;
                    }

                  need_comma = 1;

                  /* Maybe start a new line. */
                  if (nints ++ >= lineints)
                    {
                      nints = 1;

                      if (! print (output, error, "\n  "))
                        goto fail;
                    }

                  if (! print (output, error, intfmt, c))
                    goto fail;

                  /* Start a new integer. */
                  c = 0;
                  thisbit = 0;
                }

              /* Pack INTBITS pixels into an integer. */
              if (save_mask)
                {
                  c |= ((data[rowoffset + k + 1] < 128) ? 0 : 1) << (thisbit ++);
                }
              else
                {
                  if (has_alpha && (data[rowoffset + k + 1] < 128))
                    c |= 0 << (thisbit ++);
                  else
                    c |= ((data[rowoffset + k] == dark) ? 1 : 0) << (thisbit ++);
                }
            }

          if (thisbit != 0)
            {
              /* Write out the last oddball int. */
              if (need_comma)
                {
                  if (! print (output, error, ","))
                    goto fail;
                }

              need_comma = 1;

              /* Maybe start a new line. */
              if (nints ++ == lineints)
                {
                  nints = 1;

                  if (! print (output, error, "\n  "))
                    goto fail;
                }

              if (! print (output, error, intfmt, c))
                goto fail;
            }
        }

      gimp_progress_update ((double) (i + tileheight) / (double) height);
    }

  /* Write the trailer. */
  if (! print (output, error, " };\n"))
    goto fail;

  if (! g_output_stream_close (output, NULL, error))
    goto fail;

  g_free (data);
  g_object_unref (buffer);
  g_object_unref (output);

  gimp_progress_update (1.0);

  return TRUE;

 fail:

  g_free (data);
  g_object_unref (buffer);
  g_object_unref (output);

  return FALSE;
}

static gboolean
save_dialog (gint32 drawable_ID)
{
  GtkWidget     *dialog;
  GtkWidget     *frame;
  GtkWidget     *vbox;
  GtkWidget     *toggle;
  GtkWidget     *grid;
  GtkWidget     *entry;
  GtkWidget     *spinbutton;
  GtkAdjustment *adj;
  gboolean       run;

  dialog = gimp_export_dialog_new (_("XBM"), PLUG_IN_BINARY, SAVE_PROC);

  /* parameter settings */
  frame = gimp_frame_new (_("XBM Options"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 12);
  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
                      frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  /*  X10 format  */
  toggle = gtk_check_button_new_with_mnemonic (_("_X10 format bitmap"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), xsvals.x10_format);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &xsvals.x10_format);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  /* prefix */
  entry = gtk_entry_new ();
  gtk_entry_set_max_length (GTK_ENTRY (entry), MAX_PREFIX);
  gtk_entry_set_text (GTK_ENTRY (entry), xsvals.prefix);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("_Identifier prefix:"), 0.0, 0.5,
                            entry, 1);
  g_signal_connect (entry, "changed",
                    G_CALLBACK (prefix_entry_callback),
                    NULL);

  /* comment string. */
#if 0
  /* DISABLED - see http://bugzilla.gnome.org/show_bug.cgi?id=82763 */
  entry = gtk_entry_new ();
  gtk_entry_set_max_length (GTK_ENTRY (entry), MAX_COMMENT);
  gtk_widget_set_size_request (entry, 240, -1);
  gtk_entry_set_text (GTK_ENTRY (entry), xsvals.comment);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("Comment:"), 0.0, 0.5,
                            entry, 1);
  g_signal_connect (entry, "changed",
                    G_CALLBACK (comment_entry_callback),
                    NULL);
#endif

  /* hotspot toggle */
  toggle = gtk_check_button_new_with_mnemonic (_("_Write hot spot values"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), xsvals.use_hot);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &xsvals.use_hot);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  g_object_bind_property (toggle, "active",
                          grid,   "sensitive",
                          G_BINDING_SYNC_CREATE);

  adj = (GtkAdjustment *)
    gtk_adjustment_new (xsvals.x_hot, 0,
                        gimp_drawable_width (drawable_ID) - 1,
                        1, 10, 0);
  spinbutton = gtk_spin_button_new (adj, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("Hot spot _X:"), 0.0, 0.5,
                            spinbutton, 1);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &xsvals.x_hot);

  adj = (GtkAdjustment *)
    gtk_adjustment_new (xsvals.y_hot, 0,
                        gimp_drawable_height (drawable_ID) - 1,
                        1, 10, 0);
  spinbutton = gtk_spin_button_new (adj, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("Hot spot _Y:"), 0.0, 0.5,
                            spinbutton, 1);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &xsvals.y_hot);

  /* mask file */
  frame = gimp_frame_new (_("Mask File"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  toggle = gtk_check_button_new_with_mnemonic (_("W_rite extra mask file"));
  gtk_grid_attach (GTK_GRID (grid), toggle, 0, 0, 2, 1);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), xsvals.write_mask);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &xsvals.write_mask);

  entry = gtk_entry_new ();
  gtk_entry_set_max_length (GTK_ENTRY (entry), MAX_MASK_EXT);
  gtk_entry_set_text (GTK_ENTRY (entry), xsvals.mask_ext);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("_Mask file extension:"), 0.0, 0.5,
                            entry, 1);
  g_signal_connect (entry, "changed",
                    G_CALLBACK (mask_ext_entry_callback),
                    NULL);

  g_object_bind_property (toggle, "active",
                          entry,  "sensitive",
                          G_BINDING_SYNC_CREATE);

  gtk_widget_set_sensitive (frame, gimp_drawable_has_alpha (drawable_ID));

  /* Done. */
  gtk_widget_show (vbox);
  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static gboolean
print (GOutputStream  *output,
       GError        **error,
       const gchar    *format,
       ...)
{
  va_list  args;
  gboolean success;

  va_start (args, format);
  success = g_output_stream_vprintf (output, NULL, NULL,
                                     error, format, args);
  va_end (args);

  return success;
}

/* Update the comment string. */
#if 0
/* DISABLED - see http://bugzilla.gnome.org/show_bug.cgi?id=82763 */
static void
comment_entry_callback (GtkWidget *widget,
                        gpointer   data)
{
  memset (xsvals.comment, 0, sizeof (xsvals.comment));
  strncpy (xsvals.comment,
           gtk_entry_get_text (GTK_ENTRY (widget)), MAX_COMMENT);
}
#endif

static void
prefix_entry_callback (GtkWidget *widget,
                       gpointer   data)
{
  memset (xsvals.prefix, 0, sizeof (xsvals.prefix));
  strncpy (xsvals.prefix,
           gtk_entry_get_text (GTK_ENTRY (widget)), MAX_PREFIX);
}

static void
mask_ext_entry_callback (GtkWidget *widget,
                       gpointer   data)
{
  memset (xsvals.mask_ext, 0, sizeof (xsvals.mask_ext));
  strncpy (xsvals.mask_ext,
           gtk_entry_get_text (GTK_ENTRY (widget)), MAX_MASK_EXT);
}
