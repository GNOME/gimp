/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * PostScript file plugin
 * PostScript writing and GhostScript interfacing code
 * Copyright (C) 1997-98 Peter Kirchgessner
 * (email: peter@kirchgessner.net, WWW: http://www.kirchgessner.net)
 *
 * Added controls for TextAlphaBits and GraphicsAlphaBits
 *   George White <aa056@chebucto.ns.ca>
 *
 * Added Ascii85 encoding
 *   Austin Donnelly <austin@gimp.org>
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
 *
 */

/* Event history:
 * V 0.90, PK, 28-Mar-97: Creation.
 * V 0.91, PK, 03-Apr-97: Clip everything outside BoundingBox.
 *             24-Apr-97: Multi page read support.
 * V 1.00, PK, 30-Apr-97: PDF support.
 * V 1.01, PK, 05-Oct-97: Parse rc-file.
 * V 1.02, GW, 09-Oct-97: Antialiasing support.
 *         PK, 11-Oct-97: No progress bars when running non-interactive.
 *                        New procedure file_ps_load_setargs to set
 *                        load-arguments non-interactively.
 *                        If GS_OPTIONS are not set, use at least "-dSAFER"
 * V 1.03, nn, 20-Dec-97: Initialize some variables
 * V 1.04, PK, 20-Dec-97: Add Encapsulated PostScript output and preview
 * V 1.05, PK, 21-Sep-98: Write b/w-images (indexed) using image-operator
 * V 1.06, PK, 22-Dec-98: Fix problem with writing color PS files.
 *                        Ghostview may hang when displaying the files.
 * V 1.07, PK, 14-Sep-99: Add resolution to image
 * V 1.08, PK, 16-Jan-2000: Add PostScript-Level 2 by Austin Donnelly
 * V 1.09, PK, 15-Feb-2000: Force showpage on EPS-files
 *                          Add "RunLength" compression
 *                          Fix problem with "Level 2" toggle
 * V 1.10, PK, 15-Mar-2000: For load EPSF, allow negative Bounding Box Values
 *                          Save PS: don't start lines of image data with %%
 *                          to prevent problems with stupid PostScript
 *                          analyzer programs (Stanislav Brabec)
 *                          Add BeginData/EndData comments
 *                          Save PS: Set default rotation to 0
 * V 1.11, PK, 20-Aug-2000: Fix problem with BoundingBox recognition
 *                          for Mac files.
 *                          Fix problem with loop when reading not all
 *                          images of a multi page file.
 *         PK, 31-Aug-2000: Load PS: Add checks for space in filename.
 * V 1.12  PK, 19-Jun-2001: Fix problem with command line switch --
 *                          (reported by Ferenc Wagner)
 * V 1.13  PK, 07-Apr-2002: Fix problem with DOS binary EPS files
 * V 1.14  PK, 14-May-2002: Workaround EPS files of Adb. Ill. 8.0
 * V 1.15  PK, 04-Oct-2002: Be more accurate with using BoundingBox
 * V 1.16  PK, 22-Jan-2004: Don't use popen(), use g_spawn_async_with_pipes()
 *                          or g_spawn_sync().
 * V 1.17  PK, 19-Sep-2004: Fix problem with interpretation of bounding box
 */

#include "config.h"

#include <errno.h>
#include <string.h>
#include <time.h>

#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include <ghostscript/ierrors.h>
#include <ghostscript/iapi.h>
#include <ghostscript/gdevdsp.h>


#define VERSION 1.17
static const gchar dversion[] = "v1.17  19-Sep-2004";

#define LOAD_PS_PROC       "file-ps-load"
#define LOAD_EPS_PROC      "file-eps-load"
#define LOAD_PS_THUMB_PROC "file-ps-load-thumb"
#define EXPORT_PS_PROC     "file-ps-export"
#define EXPORT_EPS_PROC    "file-eps-export"
#define PLUG_IN_BINARY     "file-ps"
#define PLUG_IN_ROLE       "gimp-file-ps"

#define STR_LENGTH     64


typedef struct _PostScript      PostScript;
typedef struct _PostScriptClass PostScriptClass;

struct _PostScript
{
  GimpPlugIn      parent_instance;
};

struct _PostScriptClass
{
  GimpPlugInClass parent_class;
};


#define PS_TYPE  (ps_get_type ())
#define PS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PS_TYPE, PostScript))

GType                   ps_get_type         (void) G_GNUC_CONST;

static GList          * ps_query_procedures (GimpPlugIn            *plug_in);
static GimpProcedure  * ps_create_procedure (GimpPlugIn            *plug_in,
                                             const gchar           *name);

static gboolean         ps_extract          (GimpProcedure         *procedure,
                                             GimpRunMode            run_mode,
                                             GFile                 *file,
                                             GimpMetadata          *metadata,
                                             GimpProcedureConfig   *config,
                                             GimpVectorLoadData    *extracted_dimensions,
                                             gpointer              *data_for_run,
                                             GDestroyNotify        *data_for_run_destroy,
                                             gpointer               extract_data,
                                             GError               **error);
static GimpValueArray * ps_load             (GimpProcedure         *procedure,
                                             GimpRunMode            run_mode,
                                             GFile                 *file,
                                             gint                   width,
                                             gint                   height,
                                             GimpVectorLoadData     extracted_data,
                                             GimpMetadata          *metadata,
                                             GimpMetadataLoadFlags *flags,
                                             GimpProcedureConfig   *config,
                                             gpointer               data_from_extract,
                                             gpointer               run_data);
static GimpValueArray * ps_load_thumb       (GimpProcedure         *procedure,
                                             GFile                 *file,
                                             gint                   size,
                                             GimpProcedureConfig   *config,
                                             gpointer               run_data);
static GimpValueArray * ps_export           (GimpProcedure         *procedure,
                                             GimpRunMode            run_mode,
                                             GimpImage             *image,
                                             GFile                 *file,
                                             GimpExportOptions     *options,
                                             GimpMetadata          *metadata,
                                             GimpProcedureConfig   *config,
                                             gpointer               run_data);

static GimpImage      * load_image          (GFile                 *file,
                                             GimpProcedureConfig   *config,
                                             GError               **error);
static gboolean         export_image        (GFile                 *file,
                                             GObject               *config,
                                             GimpImage             *image,
                                             GimpDrawable          *drawable,
                                             GError               **error);

static void      ps_set_save_size (GObject           *config,
                                   GimpImage         *image);

static gboolean  save_ps_header   (GOutputStream     *output,
                                   GFile             *file,
                                   GObject           *config,
                                   GError           **error);
static gboolean  save_ps_setup    (GOutputStream     *output,
                                   GimpDrawable      *drawable,
                                   gint               width,
                                   gint               height,
                                   GObject           *config,
                                   gint               bpp,
                                   GError           **error);
static gboolean  save_ps_trailer  (GOutputStream     *output,
                                   GError           **error);

static gboolean  save_ps_preview  (GOutputStream     *output,
                                   GimpDrawable      *drawable,
                                   GObject           *config,
                                   GError           **error);

static gboolean  save_gray        (GOutputStream     *output,
                                   GimpImage         *image,
                                   GimpDrawable      *drawable,
                                   GObject           *config,
                                   GError           **error);
static gboolean  save_bw          (GOutputStream     *output,
                                   GimpImage         *image,
                                   GimpDrawable      *drawable,
                                   GObject           *config,
                                   GError           **error);
static gboolean  save_index       (GOutputStream     *output,
                                   GimpImage         *image,
                                   GimpDrawable      *drawable,
                                   GObject           *config,
                                   GError           **error);
static gboolean  save_rgb         (GOutputStream     *output,
                                   GimpImage         *image,
                                   GimpDrawable      *drawable,
                                   GObject           *config,
                                   GError           **error);

static gboolean  print            (GOutputStream     *output,
                                   GError           **error,
                                   const gchar       *format,
                                   ...) G_GNUC_PRINTF (3, 4);

static GimpImage * create_new_image (GFile             *file,
                                     guint              pagenum,
                                     guint              width,
                                     guint              height,
                                     GimpImageBaseType  type,
                                     GimpLayer        **layer);

static void      check_load_vals  (GimpProcedureConfig *config);
static void      check_save_vals  (GimpProcedureConfig *config);

static gint      page_in_list     (gchar               *list,
                                   guint                pagenum);

static gint      get_bbox         (GFile               *file,
                                   gint                *x0,
                                   gint                *y0,
                                   gint                *x1,
                                   gint                *y1);

static gboolean  ps_read_header   (GFile               *file,
                                   GimpProcedureConfig *config,
                                   gboolean            *is_pdf,
                                   gboolean            *is_epsf,
                                   gint                *bbox_x0,
                                   gint                *bbox_y0,
                                   gint                *bbox_x1,
                                   gint                *bbox_y1);
static FILE    * ps_open          (GFile               *file,
                                   GimpProcedureConfig *config,
                                   gint                *llx,
                                   gint                *lly,
                                   gint                *urx,
                                   gint                *ury,
                                   gboolean            *is_epsf,
                                   gchar              **tmp_filename);

static void      ps_close         (FILE              *ifp,
                                   gchar             *tmp_filename);
static gint      read_pnmraw_type (FILE              *ifp,
                                   gint              *width,
                                   gint              *height,
                                   gint              *maxval);

static gboolean  skip_ps          (FILE              *ifp);

static GimpImage * load_ps        (GFile             *file,
                                   guint              pagenum,
                                   FILE              *ifp,
                                   gint               llx,
                                   gint               lly,
                                   gint               urx,
                                   gint               ury);

static void      dither_grey      (const guchar      *grey,
                                   guchar            *bw,
                                   gint               npix,
                                   gint               linecount);


/* Dialog-handling */

static gint32    count_ps_pages   (GFile              *file);
static gboolean  load_dialog      (GFile              *file,
                                   GimpVectorLoadData  extracted_data,
                                   GimpProcedure      *procedure,
                                   GObject            *config);


typedef struct
{
  GtkAdjustment *adjustment[4];
  gint           level;
} SaveDialogVals;

static gboolean  save_dialog               (GimpProcedure *procedure,
                                            GObject       *config,
                                            GimpImage     *image);
static void      save_unit_changed_update  (GtkWidget     *widget,
                                            gpointer       data);


G_DEFINE_TYPE (PostScript, ps, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (PS_TYPE)
DEFINE_STD_SET_I18N


static const char hex[] = "0123456789abcdef";

/* The run mode */
static GimpRunMode l_run_mode;


static void
ps_class_init (PostScriptClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = ps_query_procedures;
  plug_in_class->create_procedure = ps_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
ps_init (PostScript *ps)
{
}

static GList *
ps_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PS_THUMB_PROC));
  list = g_list_append (list, g_strdup (LOAD_PS_PROC));
  list = g_list_append (list, g_strdup (LOAD_EPS_PROC));
  list = g_list_append (list, g_strdup (EXPORT_PS_PROC));
  list = g_list_append (list, g_strdup (EXPORT_EPS_PROC));

  return list;
}

static GimpProcedure *
ps_create_procedure (GimpPlugIn  *plug_in,
                     const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PS_PROC) ||
      ! strcmp (name, LOAD_EPS_PROC))
    {
      procedure = gimp_vector_load_procedure_new (plug_in, name,
                                                  GIMP_PDB_PROC_TYPE_PLUGIN,
                                                  ps_extract, NULL, NULL,
                                                  ps_load, NULL, NULL);

      if (! strcmp (name, LOAD_PS_PROC))
        {
          gimp_procedure_set_menu_label (procedure, _("PostScript document"));

          gimp_procedure_set_documentation (procedure,
                                            _("Load PostScript documents"),
                                            _("Load PostScript documents"),
                                            name);

          gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                               _("PostScript"));
          gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                              "application/postscript");
          gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                              "ps");
          gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                          "0,string,%!,0,long,0xc5d0d3c6");
        }
      else
        {
          gimp_procedure_set_menu_label (procedure,
                                         _("Encapsulated PostScript image"));

          gimp_procedure_set_documentation (procedure,
                                            _("Load Encapsulated PostScript images"),
                                            _("Load Encapsulated PostScript images"),
                                            name);

          gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                               _("Encapsulated PostScript"));
          gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                              "image/x-eps");
          gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                              "eps");
          gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                          "0,string,%!,0,long,0xc5d0d3c6");
        }

      gimp_procedure_set_attribution (procedure,
                                      "Peter Kirchgessner <peter@kirchgessner.net>",
                                      "Peter Kirchgessner",
                                      dversion);

      gimp_load_procedure_set_thumbnail_loader (GIMP_LOAD_PROCEDURE (procedure),
                                                LOAD_PS_THUMB_PROC);

      gimp_procedure_add_boolean_argument (procedure, "check-bbox",
                                           _("Try _Bounding Box"),
                                           _("FALSE: Use width/height, TRUE: Use BoundingBox"),
                                           TRUE,
                                           GIMP_PARAM_READWRITE);

      gimp_procedure_add_string_argument (procedure, "pages",
                                          _("_Pages"),
                                          _("Pages to load (e.g.: 1,3,5-7)"),
                                          "1",
                                          GIMP_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "coloring",
                                          _("Colorin_g"),
                                          _("Import color format"),
                                          gimp_choice_new_with_values ("bw",        4, _("B/W"),        NULL,
                                                                       "grayscale", 5, _("Gray"),       NULL,
                                                                       "rgb",       6, _("Color"),      NULL,
                                                                       "automatic", 7, _("Automatic"),  NULL,
                                                                       NULL),
                                          "rgb", G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "text-alpha-bits",
                                          _("Te_xt anti-aliasing"),
                                          _("Text anti-aliasing strength"),
                                          gimp_choice_new_with_values ("none",   1, _("None"),   NULL,
                                                                       "weak",   2, _("Weak"),   NULL,
                                                                       "strong", 4, _("Strong"), NULL,
                                                                       NULL),
                                          "none", G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "graphic-alpha-bits",
                                          _("Gra_phic anti-aliasing"),
                                          _("Graphic anti-aliasing strength"),
                                          gimp_choice_new_with_values ("none",   1, _("None"),   NULL,
                                                                       "weak",   2, _("Weak"),   NULL,
                                                                       "strong", 4, _("Strong"), NULL,
                                                                       NULL),
                                          "none", G_PARAM_READWRITE);
    }
  else if (! strcmp (name, LOAD_PS_THUMB_PROC))
    {
      procedure = gimp_thumbnail_procedure_new (plug_in, name,
                                                GIMP_PDB_PROC_TYPE_PLUGIN,
                                                ps_load_thumb, NULL, NULL);

      gimp_procedure_set_documentation (procedure,
                                        _("Loads a small preview from a "
                                          "PostScript or PDF document"),
                                        "",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Peter Kirchgessner <peter@kirchgessner.net>",
                                      "Peter Kirchgessner",
                                      dversion);
    }
  else if (! strcmp (name, EXPORT_PS_PROC) ||
           ! strcmp (name, EXPORT_EPS_PROC))
    {
      procedure = gimp_export_procedure_new (plug_in, name,
                                             GIMP_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, ps_export, NULL, NULL);

      if (! strcmp (name, EXPORT_PS_PROC))
        {
          gimp_procedure_set_menu_label (procedure, _("PostScript document"));

          gimp_procedure_set_documentation (procedure,
                                            _("Export image as PostScript document"),
                                            _("PostScript exporting handles all "
                                              "image types except those with alpha "
                                              "channels."),
                                            name);

          gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                               _("PS"));
          gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                              "application/postscript");
          gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                              "ps");
        }
      else
        {
          gimp_procedure_set_menu_label (procedure,
                                         _("Encapsulated PostScript"));

          gimp_procedure_set_documentation (procedure,
                                            _("Export image as Encapsulated "
                                              "PostScript image"),
                                            _("PostScript exporting handles all "
                                              "image types except those with alpha "
                                              "channels."),
                                            name);

          gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                               _("EPS"));
          gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                              "application/x-eps");
          gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                              "eps");
        }

      gimp_procedure_set_image_types (procedure, "RGB, GRAY, INDEXED");

      gimp_procedure_set_attribution (procedure,
                                      "Peter Kirchgessner <peter@kirchgessner.net>",
                                      "Peter Kirchgessner",
                                      dversion);

      gimp_file_procedure_set_handles_remote (GIMP_FILE_PROCEDURE (procedure),
                                              TRUE);

      gimp_export_procedure_set_capabilities (GIMP_EXPORT_PROCEDURE (procedure),
                                              GIMP_EXPORT_CAN_HANDLE_RGB  |
                                              GIMP_EXPORT_CAN_HANDLE_GRAY |
                                              GIMP_EXPORT_CAN_HANDLE_INDEXED,
                                              NULL, NULL, NULL);

      gimp_procedure_add_double_argument (procedure, "width",
                                          _("_Width"),
                                          _("Width of the image in PostScript file "
                                            "(0: use input image size)"),
                                          0, GIMP_MAX_IMAGE_SIZE, 287.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "height",
                                          _("_Height"),
                                          _("Height of the image in PostScript file "
                                            "(0: use input image size)"),
                                          0, GIMP_MAX_IMAGE_SIZE, 200.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "x-offset",
                                          _("_X offset"),
                                          _("X-offset to image from lower left corner"),
                                          -GIMP_MAX_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE, 5.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "y-offset",
                                          _("Y o_ffset"),
                                          _("Y-offset to image from lower left corner"),
                                          -GIMP_MAX_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE, 5.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "unit",
                                          _("_Unit"),
                                          _("Unit of measure for offset values"),
                                          gimp_choice_new_with_values ("inch",       0, _("Inch"),        NULL,
                                                                       "millimeter", 1, _("Millimeter"),   NULL,
                                                                       NULL),
                                          "inch", G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "keep-ratio",
                                           _("_Keep aspect ratio"),
                                           _("If enabled, aspect ratio will be maintained on export. "
                                             "Otherwise, the width and height values will be used."),
                                           TRUE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "rotation",
                                       _("Rotation"),
                                       "0, 90, 180, 270",
                                       0, 270, 0,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "level",
                                           _("PostScript Level _2"),
                                           _("If enabled, export in PostScript Level 2 format. "
                                             "Otherwise, export in PostScript Level 1 format."),
                                           TRUE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "eps-flag",
                                           _("Encapsula_ted PostScript"),
                                           _("If enabled, export as Encapsulated PostScript. "
                                             "Otherwise, export as PostScript."),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "show-preview",
                                           _("_Preview"),
                                           _("Show Preview"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "preview",
                                       _("Preview Si_ze"),
                                       _("Maximum size of preview. Set to 0 for no preview."),
                                       0, GIMP_MAX_IMAGE_SIZE, 256,
                                       G_PARAM_READWRITE);
    }

  return procedure;
}

static gboolean
ps_extract (GimpProcedure        *procedure,
            GimpRunMode           run_mode,
            GFile                *file,
            GimpMetadata         *metadata,
            GimpProcedureConfig  *config,
            GimpVectorLoadData   *extracted_dimensions,
            gpointer             *data_for_run,
            GDestroyNotify       *data_for_run_destroy,
            gpointer              extract_data,
            GError              **error)
{
  gboolean has_bbox;
  gboolean is_pdf;
  gboolean is_epsf;
  gint     bbox_x0 = 0;
  gint     bbox_y0 = 0;
  gint     bbox_x1 = 0;
  gint     bbox_y1 = 0;

  has_bbox = ps_read_header (file, NULL, &is_pdf, &is_epsf,
                             &bbox_x0, &bbox_y0, &bbox_x1, &bbox_y1);

  if (has_bbox)
    {
      extracted_dimensions->width         = (gdouble) bbox_x1 - bbox_x0;
      extracted_dimensions->height        = (gdouble) bbox_y1 - bbox_y0;
      extracted_dimensions->width_unit    = gimp_unit_point ();
      extracted_dimensions->height_unit   = gimp_unit_point ();
      extracted_dimensions->exact_width   = TRUE;
      extracted_dimensions->exact_height  = TRUE;
      extracted_dimensions->correct_ratio = TRUE;

      return TRUE;
    }
  else
    {
      return FALSE;
    }
}

static GimpValueArray *
ps_load (GimpProcedure         *procedure,
         GimpRunMode            run_mode,
         GFile                 *file,
         gint                   width,
         gint                   height,
         GimpVectorLoadData     extracted_data,
         GimpMetadata          *metadata,
         GimpMetadataLoadFlags *flags,
         GimpProcedureConfig   *config,
         gpointer               data_from_extract,
         gpointer               run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image  = NULL;
  GError         *error  = NULL;

  gegl_init (NULL, NULL);

  l_run_mode = run_mode;

  /* Why these values? No idea, they were the default until now, I just reuse
   * them. XXX
   */
  if (width == 0)
    g_object_set (config, "width", 826, NULL);
  if (height == 0)
    g_object_set (config, "height", 1170, NULL);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      if (! load_dialog (file, extracted_data, procedure, G_OBJECT (config)))
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CANCEL,
                                                 NULL);
      break;

    default:
      break;
    }

  check_load_vals (config);

  image = load_image (file, config, &error);

  if (! image)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static GimpValueArray *
ps_load_thumb (GimpProcedure       *procedure,
               GFile               *file,
               gint                 size,
               GimpProcedureConfig *config,
               gpointer             run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image = NULL;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  /*  We should look for an embedded preview but for now we
   *  just load the document at a small resolution and the
   *  first page only.
   */
  check_load_vals (NULL);

  image = load_image (file, NULL, &error);

  if (! image)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  gimp_value_array_truncate (return_vals, 2);

  return return_vals;
}

static GimpValueArray *
ps_export (GimpProcedure        *procedure,
           GimpRunMode           run_mode,
           GimpImage            *image,
           GFile                *file,
           GimpExportOptions    *options,
           GimpMetadata         *metadata,
           GimpProcedureConfig  *config,
           gpointer              run_data)
{
  GimpPDBStatusType  status   = GIMP_PDB_SUCCESS;
  GimpExportReturn   export   = GIMP_EXPORT_IGNORE;
  GList             *drawables;
  GimpImage         *orig_image;
  gboolean           eps_flag = FALSE;
  GError            *error    = NULL;

  gegl_init (NULL, NULL);

  eps_flag = strcmp (gimp_procedure_get_name (procedure), EXPORT_PS_PROC);
  g_object_set (config,
                "eps-flag", eps_flag ? 1 : 0,
                NULL);

  orig_image = image;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_ui_init (PLUG_IN_BINARY);
      ps_set_save_size (G_OBJECT (config), orig_image);

      if (! save_dialog (procedure, G_OBJECT (config), image))
        status = GIMP_PDB_CANCEL;
      break;

    case GIMP_RUN_NONINTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      break;

    default:
      break;
    }

  export    = gimp_export_options_get_image (options, &image);
  drawables = gimp_image_list_layers (image);

  if (status == GIMP_PDB_SUCCESS)
    {
      gdouble width  = 0.0;
      gdouble height = 0.0;

      if (config)
        g_object_get (config,
                      "width",  &width,
                      "height", &height,
                      NULL);

      if ((width == 0.0) || (height == 0.0))
        ps_set_save_size (G_OBJECT (config), orig_image);

      check_save_vals (config);

      if (! export_image (file, G_OBJECT (config), image, drawables->data,
                          &error))
        status = GIMP_PDB_EXECUTION_ERROR;
    }

  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image);

  g_list_free (drawables);
  return gimp_procedure_new_return_values (procedure, status, error);
}


static void compress_packbits (int            nin,
                               unsigned char *src,
                               int           *nout,
                               unsigned char *dst);

static guint32  ascii85_buf       = 0;
static gint     ascii85_len       = 0;
static gint     ascii85_linewidth = 0;

static GimpPageSelectorTarget ps_pagemode = GIMP_PAGE_SELECTOR_TARGET_LAYERS;

static void
ascii85_init (void)
{
  ascii85_len       = 0;
  ascii85_linewidth = 0;
}

static gboolean
ascii85_flush (GOutputStream  *output,
               GError        **error)
{
  gchar    c[5];
  gint     i;
  gboolean zero_case = (ascii85_buf == 0);
  GString  *string   = g_string_new (NULL);

  static gint max_linewidth = 75;

  for (i = 4; i >= 0; i--)
    {
      c[i] = (ascii85_buf % 85) + '!';
      ascii85_buf /= 85;
    }

  /* check for special case: "!!!!!" becomes "z", but only if not
   * at end of data. */
  if (zero_case && (ascii85_len == 4))
    {
      if (ascii85_linewidth >= max_linewidth)
        {
          g_string_append_c (string, '\n');

          ascii85_linewidth = 0;
        }

      g_string_append_c (string, 'z');

      ascii85_linewidth++;
    }
  else
    {
      for (i = 0; i < ascii85_len + 1; i++)
        {
          if ((ascii85_linewidth >= max_linewidth) && (c[i] != '%'))
            {
              g_string_append_c (string, '\n');

              ascii85_linewidth = 0;
            }

          g_string_append_c (string, c[i]);

          ascii85_linewidth++;
        }
    }

  ascii85_len = 0;
  ascii85_buf = 0;

  if (string->len > 0 &&
      ! g_output_stream_write_all (output,
                                   string->str, string->len, NULL,
                                   NULL, error))
    {
      g_string_free (string, TRUE);

      return FALSE;
    }

  g_string_free (string, TRUE);

  return TRUE;
}

static inline gboolean
ascii85_out (GOutputStream  *output,
             guchar          byte,
             GError        **error)
{
  if (ascii85_len == 4)
    if (! ascii85_flush (output, error))
      return FALSE;

  ascii85_buf <<= 8;
  ascii85_buf |= byte;
  ascii85_len++;

  return TRUE;
}

static gboolean
ascii85_nout (GOutputStream  *output,
              gint            n,
              guchar         *uptr,
              GError        **error)
{
  while (n-- > 0)
    {
      if (! ascii85_out (output, *uptr, error))
        return FALSE;

      uptr++;
    }

  return TRUE;
}

static gboolean
ascii85_done (GOutputStream  *output,
              GError        **error)
{
  if (ascii85_len)
    {
      /* zero any unfilled buffer portion, then flush */
      ascii85_buf <<= (8 * (4 - ascii85_len));

      if (! ascii85_flush (output, error))
        return FALSE;
    }

  if (! print (output, error, "~>\n"))
    return FALSE;

  return TRUE;
}


static void
compress_packbits (int nin,
                   unsigned char *src,
                   int *nout,
                   unsigned char *dst)

{
 unsigned char c;
 int nrepeat, nliteral;
 unsigned char *run_start;
 unsigned char *start_dst = dst;
 unsigned char *last_literal = NULL;

 for (;;)
 {
   if (nin <= 0) break;

   run_start = src;
   c = *run_start;

   /* Search repeat bytes */
   if ((nin > 1) && (c == src[1]))
   {
     nrepeat = 1;
     nin -= 2;
     src += 2;
     while ((nin > 0) && (c == *src))
     {
       nrepeat++;
       src++;
       nin--;
       if (nrepeat == 127) break; /* Maximum repeat */
     }

     /* Add two-byte repeat to last literal run ? */
     if (   (nrepeat == 1)
         && (last_literal != NULL) && (((*last_literal)+1)+2 <= 128))
     {
       *last_literal += 2;
       *(dst++) = c;
       *(dst++) = c;
       continue;
     }

     /* Add repeat run */
     *(dst++) = (unsigned char)((-nrepeat) & 0xff);
     *(dst++) = c;
     last_literal = NULL;
     continue;
   }
   /* Search literal bytes */
   nliteral = 1;
   nin--;
   src++;

   for (;;)
   {
     if (nin <= 0) break;

     if ((nin >= 2) && (src[0] == src[1])) /* A two byte repeat ? */
       break;

     nliteral++;
     nin--;
     src++;
     if (nliteral == 128) break; /* Maximum literal run */
   }

   /* Could be added to last literal run ? */
   if ((last_literal != NULL) && (((*last_literal)+1)+nliteral <= 128))
   {
     *last_literal += nliteral;
   }
   else
   {
     last_literal = dst;
     *(dst++) = (unsigned char)(nliteral-1);
   }
   while (nliteral-- > 0) *(dst++) = *(run_start++);
 }
 *nout = dst - start_dst;
}


typedef struct
{
  goffset eol;
  goffset begin_data;
} PS_DATA_POS;

static PS_DATA_POS ps_data_pos = { 0, 0 };

static gboolean
ps_begin_data (GOutputStream  *output,
               GError        **error)
{
  /*                                 %%BeginData: 123456789012 ASCII Bytes */
  if (! print (output, error, "%s", "%%BeginData:                         "))
    return FALSE;

  ps_data_pos.eol = g_seekable_tell (G_SEEKABLE (output));

  if (! print (output, error, "\n"))
    return FALSE;

  ps_data_pos.begin_data = g_seekable_tell (G_SEEKABLE (output));

  return TRUE;
}

static gboolean
ps_end_data (GOutputStream  *output,
             GError        **error)
{
  goffset end_data;
  gchar   s[64];

  if ((ps_data_pos.begin_data > 0) && (ps_data_pos.eol > 0))
    {
      end_data = g_seekable_tell (G_SEEKABLE (output));

      if (end_data > 0)
        {
          g_snprintf (s, sizeof (s),
                      "%"G_GOFFSET_FORMAT" ASCII Bytes", end_data - ps_data_pos.begin_data);

          if (! g_seekable_seek (G_SEEKABLE (output),
                                 ps_data_pos.eol - strlen (s), G_SEEK_SET,
                                 NULL, error))
            return FALSE;

          if (! print (output, error, "%s", s))
            return FALSE;

          if (! g_seekable_seek (G_SEEKABLE (output),
                                 end_data, G_SEEK_SET,
                                 NULL, error))
            return FALSE;
        }
    }

  if (! print (output, error, "%s\n", "%%EndData"))
    return FALSE;

  return TRUE;
}

static GimpImage *
load_image (GFile                *file,
            GimpProcedureConfig  *config,
            GError              **error)
{
  GimpImage    *image = NULL;
  GimpImage   **image_list, **nl;
  const gchar  *filename;
  gint          width;
  gint          height;
  gdouble       resolution;
  gchar        *pnm_type = NULL;
  gchar        *text_alpha = NULL;
  gchar        *graphics_alpha = NULL;
  gchar        *pages;
  guint         page_count;
  FILE         *ifp;
  gchar        *temp;
  gint          llx, lly, urx, ury;
  gint          k, n_images, max_images, max_pagenum;
  gboolean      is_epsf;
  GdkPixbuf    *pixbuf = NULL;
  gchar        *tmp_filename = NULL;

  if (config)
    {
      g_object_get (config,
                    "pixel-density",      &resolution,
                    "width",              &width,
                    "height",             &height,
                    "pages",              &pages,
                    "coloring",           &pnm_type,
                    "text-alpha-bits",    &text_alpha,
                    "graphic-alpha-bits", &graphics_alpha,
                    NULL);
    }
  else
    {
      pages = g_strdup_printf ("1");
    }

#ifdef PS_DEBUG
  g_print ("load_image:\n resolution = %f\n", resolution);
  g_print (" %dx%d pixels\n", width, height);
  g_print (" Coloring: %s\n", pnm_type);
  g_print (" TextAlphaBits: %s\n", text_alpha);
  g_print (" GraphicsAlphaBits: %s\n", graphics_alpha);
#endif

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_file_get_utf8_name (file));

  /* Try to see if PostScript file is available */
  filename = gimp_file_get_utf8_name (file);
  ifp = g_fopen (filename, "r");

  if (! ifp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }
  fclose (ifp);

  ifp = ps_open (file, config, &llx, &lly, &urx, &ury, &is_epsf,
                 &tmp_filename);
  if (!ifp)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR,
                   _("Could not interpret PostScript file '%s'"),
                   gimp_file_get_utf8_name (file));
      return NULL;
    }

  image_list = g_new (GimpImage *, 10);
  n_images = 0;
  max_images = 10;

  max_pagenum = 9999;  /* Try to get the maximum pagenumber to read */
  if (is_epsf)
    {
      max_pagenum = 1;
      /* Use pixbuf to load transparent EPS as PNGs */
      pixbuf = gdk_pixbuf_new_from_file (tmp_filename, error);
      if (! pixbuf)
        return NULL;
    }

  if (! page_in_list (pages, max_pagenum)) /* Is there a limit in list ? */
    {
      max_pagenum = -1;
      for (temp = pages; *temp != '\0'; temp++)
        {
          if ((*temp < '0') || (*temp > '9'))
            continue; /* Search next digit */
          sscanf (temp, "%d", &k);
          if (k > max_pagenum)
            max_pagenum = k;
          while ((*temp >= '0') && (*temp <= '9'))
            temp++;
          temp--;
        }

      if (max_pagenum < 1)
        max_pagenum = 9999;
    }

  /* Load all images */
  for (page_count = 1; page_count <= max_pagenum; page_count++)
    {
      if (page_in_list (pages, page_count))
        {
          image = load_ps (file, page_count, ifp, llx, lly, urx, ury);
          if (! image)
            break;

          gimp_image_set_resolution (image, resolution, resolution);

          if (n_images == max_images)
            {
              nl = (GimpImage **) g_realloc (image_list,
                                             (max_images+10)*sizeof (GimpImage *));
              if (nl == NULL) break;
              image_list = nl;
              max_images += 10;
            }
          image_list[n_images++] = image;
        }
      else  /* Skip an image */
        {
          image = NULL;
          if (! skip_ps (ifp))
            break;
        }
    }

  ps_close (ifp, tmp_filename);

  /* EPS are now imported using pngalpha, so they can be converted
   * to a layer with gimp_layer_new_from_pixbuf () and exported at
   * this part of the loading process
   */
  if (is_epsf)
    {
      GimpLayer *layer;

      image = gimp_image_new (urx, ury, GIMP_RGB);

      gimp_image_undo_disable (image);
      gimp_image_set_resolution (image, resolution, resolution);

      layer = gimp_layer_new_from_pixbuf (image, _("Rendered EPS"), pixbuf,
                                          100,
                                          gimp_image_get_default_new_layer_mode (image),
                                          0.0, 1.0);
      gimp_image_insert_layer (image, layer, NULL, 0);

      gimp_image_undo_enable (image);

      g_free (image_list);
      g_object_unref (pixbuf);

      return image;
    }

  if (ps_pagemode == GIMP_PAGE_SELECTOR_TARGET_LAYERS)
    {
      for (k = 0; k < n_images; k++)
        {
          if (k == 0)
            {
              GFile *new_file;
              gchar *uri;
              gchar *new_uri;

              image = image_list[0];

              uri = g_file_get_uri (file);

              new_uri = g_strdup_printf (_("%s-pages"), uri);
              g_free (uri);

              new_file = g_file_new_for_uri (new_uri);
              g_free (new_uri);

              g_object_unref (new_file);
            }
          else
            {
              GimpLayer     *current_layer;
              gchar         *name;
              GimpItem     **tmp_drawables;
              gint           n_drawables;

              tmp_drawables = gimp_image_get_selected_drawables (image_list[k], &n_drawables);

              name = gimp_item_get_name (GIMP_ITEM (tmp_drawables[0]));

              current_layer = gimp_layer_new_from_drawable (GIMP_DRAWABLE (tmp_drawables[0]),
                                                            image);
              gimp_item_set_name (GIMP_ITEM (current_layer), name);
              gimp_image_insert_layer (image, current_layer, NULL, -1);
              gimp_image_delete (image_list[k]);

              g_free (tmp_drawables);
              g_free (name);
            }
        }

      gimp_image_undo_enable (image);
    }
  else
    {
      /* Display images in reverse order.
       * The last will be displayed by GIMP itself
       */
      for (k = n_images - 1; k >= 0; k--)
        {
          gimp_image_undo_enable (image_list[k]);
          gimp_image_clean_all (image_list[k]);

          if (l_run_mode != GIMP_RUN_NONINTERACTIVE && k > 0)
            gimp_display_new (image_list[k]);
        }

      image = (n_images > 0) ? image_list[0] : NULL;
    }

  g_free (image_list);

  return image;
}


static gboolean
export_image (GFile         *file,
              GObject       *config,
              GimpImage     *image,
              GimpDrawable  *drawable,
              GError       **error)
{
  GOutputStream *output;
  GCancellable  *cancellable;
  GimpImageType  drawable_type;

  drawable_type = gimp_drawable_type (drawable);

  /*  Make sure we're not exporting an image with an alpha channel  */
  if (gimp_drawable_has_alpha (drawable))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("PostScript export cannot handle images with alpha channels"));
      return FALSE;
    }

  switch (drawable_type)
    {
    case GIMP_INDEXED_IMAGE:
    case GIMP_GRAY_IMAGE:
    case GIMP_RGB_IMAGE:
      break;

    default:
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Cannot operate on unknown image types."));
      return FALSE;
      break;
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

  if (! save_ps_header (output, file, config, error))
    goto fail;

  switch (drawable_type)
    {
    case GIMP_INDEXED_IMAGE:
      if (! save_index (output, image, drawable, config, error))
        goto fail;
      break;

    case GIMP_GRAY_IMAGE:
      if (! save_gray (output, image, drawable, config, error))
        goto fail;
      break;

    case GIMP_RGB_IMAGE:
      if (! save_rgb (output, image, drawable, config, error))
        goto fail;
      break;

    default:
      g_return_val_if_reached (FALSE);
    }

  if (! save_ps_trailer (output, error))
    goto fail;

  if (! g_output_stream_close (output, NULL, error))
    goto fail;

  g_object_unref (output);

  return TRUE;

 fail:

  cancellable = g_cancellable_new ();
  g_cancellable_cancel (cancellable);
  g_output_stream_close (output, cancellable, NULL);

  g_object_unref (output);
  g_object_unref (cancellable);

  return FALSE;
}


/* Check (and correct) the load values */
static void
check_load_vals (GimpProcedureConfig *config)
{
  gdouble  resolution     = GIMP_MIN_RESOLUTION;
  gint     width          = 2;
  gint     height         = 2;
  gboolean use_bbox       = FALSE;
  gint     pnm_type       = 6;
  gint     text_alpha     = 1;
  gint     graphics_alpha = 1;
  gchar   *pages          = NULL;

  if (config)
    {
      g_object_get (config,
                    "pixel-density",      &resolution,
                    "width",              &width,
                    "height",             &height,
                    "check-bbox",         &use_bbox,
                    "pages",              &pages,
                    NULL);
      pnm_type       = gimp_procedure_config_get_choice_id (config, "coloring");
      text_alpha     = gimp_procedure_config_get_choice_id (config, "text-alpha-bits");
      graphics_alpha = gimp_procedure_config_get_choice_id (config, "graphic-alpha-bits");
    }

  if (resolution < GIMP_MIN_RESOLUTION)
    resolution = GIMP_MIN_RESOLUTION;
  else if (resolution > GIMP_MAX_RESOLUTION)
    resolution = GIMP_MAX_RESOLUTION;

  if (width < 2)
    width = 2;
  if (height < 2)
    height = 2;
  use_bbox = (use_bbox != 0);
  if (! pages)
    pages = g_strdup_printf ("1-99");
  if ((pnm_type < 4) || (pnm_type > 7))
    pnm_type = 6;
  if ((text_alpha != 1) && (text_alpha != 2) &&
      (text_alpha != 4))
    text_alpha = 1;
  if ((graphics_alpha != 1) && (graphics_alpha != 2) &&
      (graphics_alpha != 4))
    graphics_alpha = 1;

  if (config)
    {
      g_object_set (config,
                    "pixel-density",      resolution,
                    "width",              width,
                    "height",             height,
                    "check_bbox",         use_bbox ? 1 : 0,
                    "pages",              pages,
                    NULL);

      switch (pnm_type)
        {
        case 4:
          g_object_set (config, "coloring", "bw", NULL);
          break;
        case 5:
          g_object_set (config, "coloring", "grayscale", NULL);
          break;
        case 6:
          g_object_set (config, "coloring", "rgb", NULL);
          break;
        case 7:
        default:
          g_object_set (config, "coloring", "automatic", NULL);
        }

      switch (text_alpha)
        {
          case 2:
            g_object_set (config, "text-alpha-bits", "weak", NULL);
            break;
          case 4:
            g_object_set (config, "text-alpha-bits", "strong", NULL);
            break;
          case 1:
          default:
            g_object_set (config, "text-alpha-bits", "none", NULL);
        }

      switch (graphics_alpha)
        {
          case 2:
            g_object_set (config, "graphic-alpha-bits", "weak", NULL);
            break;
          case 4:
            g_object_set (config, "graphic-alpha-bits", "strong", NULL);
            break;
          case 1:
          default:
            g_object_set (config, "graphic-alpha-bits", "none", NULL);
        }
    }

  g_free (pages);
}


/* Check (and correct) the save values */
static void
check_save_vals (GimpProcedureConfig *config)
{
  gint     i            = 90;
  gint     preview_size = 0;
  gboolean preview      = TRUE;

  g_object_get (config,
                "rotation",     &i,
                "show-preview", &preview,
                "preview",      &preview_size,
                NULL);

  if ((i != 0) && (i != 90) && (i != 180) && (i != 270))
    i = 90;
  if (preview_size <= 0)
    preview = FALSE;

  g_object_set (config,
                "rotation",     i,
                "show-preview", preview ? 1 : 0,
                "preview",      preview_size,
                NULL);
}

static void
ps_set_save_size (GObject   *config,
                  GimpImage *image)
{
  gdouble    xres, yres, factor, iw, ih;
  guint      width, height;
  GimpUnit  *unit;
  gboolean   unit_mm = FALSE;

  gimp_image_get_resolution (image, &xres, &yres);

  if ((xres < 1e-5) || (yres < 1e-5))
    xres = yres = 72.0;

  /* Calculate size of image in inches */
  width  = gimp_image_get_width (image);
  height = gimp_image_get_height (image);
  iw = width  / xres;
  ih = height / yres;

  unit = gimp_image_get_unit (image);
  factor = gimp_unit_get_factor (unit);

  if (factor == 0.0254 ||
      factor == 0.254 ||
      factor == 2.54 ||
      factor == 25.4)
    {
      unit_mm = TRUE;
    }

  if (unit_mm)
    {
      iw *= 25.4;
      ih *= 25.4;
    }

  g_object_set (config,
                "unit",   unit_mm ? "millimeter" : "inch",
                "width",  iw,
                "height", ih,
                NULL);
}

/* Check if a page is in a given list */
static gint
page_in_list (gchar *list,
              guint  page_num)
{
  char tmplist[STR_LENGTH], *c0, *c1;
  int state, start_num, end_num;
#define READ_STARTNUM  0
#define READ_ENDNUM    1
#define CHK_LIST(a,b,c) {int low=(a),high=(b),swp; \
  if ((low>0) && (high>0)) { \
  if (low>high) {swp=low; low=high; high=swp;} \
  if ((low<=(c))&&(high>=(c))) return (1); } }

  if ((list == NULL) || (*list == '\0'))
    return 1;

  g_strlcpy (tmplist, list, STR_LENGTH);

  c0 = c1 = tmplist;
  while (*c1)    /* Remove all whitespace and break on unsupported characters */
    {
      if ((*c1 >= '0') && (*c1 <= '9'))
        {
          *(c0++) = *c1;
        }
      else if ((*c1 == '-') || (*c1 == ','))
        { /* Try to remove double occurrences of these characters */
          if (c0 == tmplist)
            {
              *(c0++) = *c1;
            }
          else
            {
              if (*(c0-1) != *c1)
                *(c0++) = *c1;
            }
        }
      else
        break;

      c1++;
    }

  if (c0 == tmplist)
    return 1;

  *c0 = '\0';

  /* Now we have a comma separated list like 1-4-1,-3,1- */

  start_num = end_num = -1;
  state = READ_STARTNUM;
  for (c0 = tmplist; *c0 != '\0'; c0++)
    {
      switch (state)
        {
        case READ_STARTNUM:
          if (*c0 == ',')
            {
              if ((start_num > 0) && (start_num == (int)page_num))
                return -1;
              start_num = -1;
            }
          else if (*c0 == '-')
            {
              if (start_num < 0) start_num = 1;
              state = READ_ENDNUM;
            }
          else /* '0' - '9' */
            {
              if (start_num < 0) start_num = 0;
              start_num *= 10;
              start_num += *c0 - '0';
            }
          break;

        case READ_ENDNUM:
          if (*c0 == ',')
            {
              if (end_num < 0) end_num = 9999;
              CHK_LIST (start_num, end_num, (int)page_num);
              start_num = end_num = -1;
              state = READ_STARTNUM;
            }
          else if (*c0 == '-')
            {
              CHK_LIST (start_num, end_num, (int)page_num);
              start_num = end_num;
              end_num = -1;
            }
          else /* '0' - '9' */
            {
              if (end_num < 0) end_num = 0;
              end_num *= 10;
              end_num += *c0 - '0';
            }
          break;
        }
    }
  if (state == READ_STARTNUM)
    {
      if (start_num > 0)
        return (start_num == (int) page_num);
    }
  else
    {
      if (end_num < 0) end_num = 9999;
      CHK_LIST (start_num, end_num, (int)page_num);
    }

  return 0;
#undef CHK_LIST
}


/* A function like fgets, but treats single CR-character as line break. */
/* As a line break the newline-character is returned. */
static char *psfgets (char *s, int size, FILE *stream)

{
  int c;
  char *sptr = s;

  if (size <= 0)
    return NULL;

  if (size == 1)
    {
      *s = '\0';
      return NULL;
    }

  c = getc (stream);
  if (c == EOF)
    return NULL;

  for (;;)
    {
      /* At this point we have space in sptr for at least two characters */
      if (c == '\n')    /* Got end of line (UNIX line end) ? */
        {
          *(sptr++) = '\n';
          break;
        }
      else if (c == '\r')  /* Got a carriage return. Check next character */
        {
          c = getc (stream);
          if ((c == EOF) || (c == '\n')) /* EOF or DOS line end ? */
            {
              *(sptr++) = '\n';  /* Return UNIX line end */
              break;
            }
          else  /* Single carriage return. Return UNIX line end. */
            {
              ungetc (c, stream);  /* Save the extra character */
              *(sptr++) = '\n';
              break;
            }
        }
      else   /* no line end character */
        {
          *(sptr++) = (char)c;
          size--;
        }
      if (size == 1)
        break;  /* Only space for the nul-character ? */

      c = getc (stream);
      if (c == EOF)
        break;
    }

  *sptr = '\0';

  return s;
}


/* Get the BoundingBox of a PostScript file. On success, 0 is returned. */
/* On failure, -1 is returned. */
static gint
get_bbox (GFile *file,
          gint  *x0,
          gint  *y0,
          gint  *x1,
          gint  *y1)
{
  char   line[1024], *src;
  FILE  *ifp;
  int    retval = -1;

  ifp = g_fopen (g_file_peek_path (file), "rb");

  if (! ifp)
    return -1;

  for (;;)
    {
      if (psfgets (line, sizeof (line)-1, ifp) == NULL) break;
      if ((line[0] != '%') || (line[1] != '%')) continue;
      src = &(line[2]);
      while ((*src == ' ') || (*src == '\t')) src++;
      if (strncmp (src, "BoundingBox", 11) != 0) continue;
      src += 11;
      while ((*src == ' ') || (*src == '\t') || (*src == ':')) src++;
      if (strncmp (src, "(atend)", 7) == 0) continue;
      if (sscanf (src, "%d%d%d%d", x0, y0, x1, y1) == 4)
        retval = 0;
      break;
    }
  fclose (ifp);

  return retval;
}

static gboolean
ps_read_header (GFile               *file,
                GimpProcedureConfig *config,
                gboolean            *is_pdf,
                gboolean            *is_epsf,
                gint                *bbox_x0,
                gint                *bbox_y0,
                gint                *bbox_x1,
                gint                *bbox_y1)
{
  FILE        *eps_file;
  const gchar *filename;
  gboolean     use_bbox   = TRUE;
  gboolean     maybe_epsf = FALSE;
  gboolean     has_bbox   = FALSE;

  if (config)
    g_object_get (config, "check-bbox", &use_bbox, NULL);

  /* Check if the file is a PDF. For PDF, we can't set geometry */
  *is_pdf = FALSE;

  /* Check if it is a EPS-file */
  *is_epsf = FALSE;

  *bbox_x0 = 0.0;
  *bbox_y0 = 0.0;
  *bbox_x1 = 0.0;
  *bbox_y1 = 0.0;

  filename = gimp_file_get_utf8_name (file);
  eps_file = g_fopen (filename, "rb");

  if (eps_file)
    {
      gchar hdr[512];

      fread (hdr, 1, sizeof(hdr), eps_file);
      *is_pdf = (strncmp (hdr, "%PDF", 4) == 0);

      if (!*is_pdf)  /* Check for EPSF */
        {
          char                 *adobe, *epsf;
          int                   ds = 0;
          static unsigned char  doseps[5] = { 0xc5, 0xd0, 0xd3, 0xc6, 0 };

          hdr[sizeof(hdr)-1] = '\0';
          adobe = strstr (hdr, "PS-Adobe-");
          epsf = strstr (hdr, "EPSF-");

          if ((adobe != NULL) && (epsf != NULL))
            ds = epsf - adobe;

          *is_epsf = ((ds >= 11) && (ds <= 15));

          /* Illustrator uses negative values in BoundingBox without marking */
          /* files as EPSF. Try to handle that. */
          maybe_epsf =
            (strstr (hdr, "%%Creator: Adobe Illustrator(R) 8.0") != 0);

          /* Check DOS EPS binary file */
          if ((!*is_epsf) && (strncmp (hdr, (char *)doseps, 4) == 0))
            *is_epsf = 1;
        }

      fclose (eps_file);
    }

  if ((!*is_pdf) && (use_bbox))    /* Try the BoundingBox ? */
    {
      if (get_bbox (file, bbox_x0, bbox_y0, bbox_x1, bbox_y1) == 0)
        {
          has_bbox = TRUE;

          if (maybe_epsf && ((bbox_x0 < 0) || (bbox_y0 < 0)))
            *is_epsf = 1;
        }
    }

  return has_bbox;
}

/* Open the PostScript file. On failure, NULL is returned. */
/* The filepointer returned will give a PNM-file generated */
/* by the PostScript-interpreter. */
static FILE *
ps_open (GFile               *file,
         GimpProcedureConfig *config,
         gint                *llx,
         gint                *lly,
         gint                *urx,
         gint                *ury,
         gboolean            *is_epsf,
         gchar              **tmp_filename)
{
  const gchar  *driver;
  GPtrArray    *cmdA;
  gchar       **pcmdA;
  FILE         *fd_popen = NULL;
  gint          width, height;
  gdouble       resolution;
  gint          pnm_type       = 0;
  gint          text_alpha     = 0;
  gint          graphics_alpha = 0;
  gboolean      has_bbox;
  gint          bbox_x0 = 0;
  gint          bbox_y0 = 0;
  gint          bbox_x1 = 0;
  gint          bbox_y1 = 0;
  gint          offx    = 0;
  gint          offy    = 0;
  gboolean      is_pdf;
  int           code;
  void         *instance = NULL;

  if (config)
    {
      g_object_get (config,
                    "width",              &width,
                    "height",             &height,
                    "pixel-density",      &resolution,
                    NULL);
      pnm_type       = gimp_procedure_config_get_choice_id (config, "coloring");
      text_alpha     = gimp_procedure_config_get_choice_id (config, "text-alpha-bits");
      graphics_alpha = gimp_procedure_config_get_choice_id (config, "graphic-alpha-bits");
    }
  else
    {
      width = 256;
      height = 256;
      resolution = 256.0 / 4.0;
    }

  *llx = *lly = 0;
  *urx = width - 1;
  *ury = height - 1;

  has_bbox = ps_read_header (file, config, &is_pdf, is_epsf, &bbox_x0, &bbox_y0, &bbox_x1, &bbox_y1);

  if (has_bbox)
    {
      if (*is_epsf)  /* Handle negative BoundingBox for EPSF */
        {
          offx = -bbox_x0; bbox_x1 += offx; bbox_x0 += offx;
          offy = -bbox_y0; bbox_y1 += offy; bbox_y0 += offy;
        }

      if (bbox_x0 >= 0 && bbox_y0 >= 0 && bbox_x1 > bbox_x0 && bbox_y1 > bbox_y0)
        {
          *llx = (int) (bbox_x0 / 72.0 * resolution + 0.0001);
          *lly = (int) (bbox_y0 / 72.0 * resolution + 0.0001);
          /* Use upper bbox values as image size */
          width = (int) (bbox_x1 / 72.0 * resolution + 0.5);
          height = (int) (bbox_y1 / 72.0 * resolution + 0.5);
          /* Pixel coordinates must be one less */
          *urx = width - 1;
          *ury = height - 1;
          if (*urx < *llx) *urx = *llx;
          if (*ury < *lly) *ury = *lly;
        }
    }

  switch (pnm_type)
    {
    case 4:
      driver = "pbmraw";
      break;
    case 5:
      driver = "pgmraw";
      break;
    case 7:
      driver = "pnmraw";
      break;
    default:
      driver = "ppmraw";
      break;
    }

  /* For instance, the Win32 port of ghostscript doesn't work correctly when
   * using standard output as output file.
   * Thus, use a real output file.
   */
  if (*is_epsf)
    {
      driver = "pngalpha";
      *tmp_filename = g_file_get_path (gimp_temp_file ("png"));
    }
  else
    {
      *tmp_filename = g_file_get_path (gimp_temp_file ("pnm"));
    }

  /* Build command array */
  cmdA = g_ptr_array_new ();

  g_ptr_array_add (cmdA, g_strdup (g_get_prgname ()));
  g_ptr_array_add (cmdA, g_strdup_printf ("-sDEVICE=%s", driver));
  g_ptr_array_add (cmdA, g_strdup_printf ("-r%d", (gint) resolution));

  if (is_pdf)
    {
      /* Acrobat Reader honors CropBox over MediaBox, so let's match that
       * behavior.
       */
      g_ptr_array_add (cmdA, g_strdup ("-dUseCropBox"));
    }
  else
    {
      /* For PDF, we can't set geometry */
      g_ptr_array_add (cmdA, g_strdup_printf ("-g%dx%d", width, height));
    }

  /* Antialiasing not available for PBM-device */
  if ((pnm_type != 4) && (text_alpha != 1))
    g_ptr_array_add (cmdA, g_strdup_printf ("-dTextAlphaBits=%d",
                                            text_alpha));
  if ((pnm_type != 4) && (graphics_alpha != 1))
    g_ptr_array_add (cmdA, g_strdup_printf ("-dGraphicsAlphaBits=%d",
                                            graphics_alpha));
  g_ptr_array_add (cmdA, g_strdup ("-q"));
  g_ptr_array_add (cmdA, g_strdup ("-dBATCH"));
  g_ptr_array_add (cmdA, g_strdup ("-dNOPAUSE"));

  /* If no additional options specified, use at least -dSAFER */
  if (g_getenv ("GS_OPTIONS") == NULL)
    g_ptr_array_add (cmdA, g_strdup ("-dSAFER"));

  /* Output file name */
  g_ptr_array_add (cmdA, g_strdup_printf ("-sOutputFile=%s", *tmp_filename));

  /* Offset command for gs to get image part with negative x/y-coord. */
  if ((offx != 0) || (offy != 0))
    {
      g_ptr_array_add (cmdA, g_strdup ("-c"));
      g_ptr_array_add (cmdA, g_strdup_printf ("%d", offx));
      g_ptr_array_add (cmdA, g_strdup_printf ("%d", offy));
      g_ptr_array_add (cmdA, g_strdup ("translate"));
    }

  /* input file name */
  g_ptr_array_add (cmdA, g_strdup ("-f"));
  g_ptr_array_add (cmdA, g_strdup (gimp_file_get_utf8_name (file)));

  if (*is_epsf)
    {
      g_ptr_array_add (cmdA, g_strdup ("-c"));
      g_ptr_array_add (cmdA, g_strdup ("showpage"));
    }

  g_ptr_array_add (cmdA, g_strdup ("-c"));
  g_ptr_array_add (cmdA, g_strdup ("quit"));
  g_ptr_array_add (cmdA, NULL);

  pcmdA = (gchar **) cmdA->pdata;

#ifdef PS_DEBUG
  {
    gchar **p = pcmdA;
    g_print ("Passing args (argc=%d):\n", cmdA->len - 1);

    while (*p)
      {
        g_print ("%s\n", *p);
        p++;
      }
  }
#endif

  code = gsapi_new_instance (&instance, NULL);
  if (code == 0) {
    code = gsapi_set_arg_encoding(instance, GS_ARG_ENCODING_UTF8);
    code = gsapi_init_with_args (instance, cmdA->len - 1, pcmdA);
    code = gsapi_exit (instance);
    gsapi_delete_instance (instance);
  }

  /* Don't care about exit status of ghostscript. */
  /* Just try to read what it wrote. */

  fd_popen = g_fopen (*tmp_filename, "rb");

  g_ptr_array_free (cmdA, FALSE);
  g_strfreev (pcmdA);

  return fd_popen;
}


/* Close the PNM-File of the PostScript interpreter */
static void
ps_close (FILE *ifp, gchar *tmp_filename)
{
 /* If a real outputfile was used, close the file and remove it. */
  fclose (ifp);
  g_unlink (tmp_filename);
}


/* Read the header of a raw PNM-file and return type (4-6) or -1 on failure */
static gint
read_pnmraw_type (FILE *ifp,
                  gint *width,
                  gint *height,
                  gint *maxval)
{
  int frst, scnd, thrd;
  gint  pnmtype;
  gchar line[1024];

  /* GhostScript may write some informational messages infront of the header. */
  /* We are just looking at a Px\n in the input stream. */
  frst = getc (ifp);
  scnd = getc (ifp);
  thrd = getc (ifp);
  for (;;)
    {
      if (thrd == EOF) return -1;
#ifdef G_OS_WIN32
      if (thrd == '\r') thrd = getc (ifp);
#endif
      if ((thrd == '\n') && (frst == 'P') && (scnd >= '1') && (scnd <= '6'))
        break;
      frst = scnd;
      scnd = thrd;
      thrd = getc (ifp);
    }
  pnmtype = scnd - '0';
  /* We don't use the ASCII-versions */
  if ((pnmtype >= 1) && (pnmtype <= 3))
    return -1;

  /* Read width/height */
  for (;;)
    {
      if (fgets (line, sizeof (line)-1, ifp) == NULL)
        return -1;
      if (line[0] != '#')
        break;
    }
  if (sscanf (line, "%d%d", width, height) != 2)
    return -1;

  *maxval = 255;

  if (pnmtype != 4)  /* Read maxval */
    {
      for (;;)
        {
          if (fgets (line, sizeof (line)-1, ifp) == NULL)
            return -1;
          if (line[0] != '#')
            break;
        }
      if (sscanf (line, "%d", maxval) != 1)
        return -1;
    }

  return pnmtype;
}


/* Create an image. Sets layer, drawable and rgn. Returns image */
static GimpImage *
create_new_image (GFile              *file,
                  guint               pagenum,
                  guint               width,
                  guint               height,
                  GimpImageBaseType   type,
                  GimpLayer         **layer)
{
  GimpImage     *image;
  GimpImageType  gdtype;
  gchar         *tmp;

  switch (type)
    {
    case GIMP_GRAY:
      gdtype = GIMP_GRAY_IMAGE;
      break;
    case GIMP_INDEXED:
      gdtype = GIMP_INDEXED_IMAGE;
      break;
    case GIMP_RGB:
    default:
      gdtype = GIMP_RGB_IMAGE;
    }

  image = gimp_image_new_with_precision (width, height, type,
                                         GIMP_PRECISION_U8_NON_LINEAR);
  gimp_image_undo_disable (image);

  tmp = g_strdup_printf (_("Page %d"), pagenum);
  *layer = gimp_layer_new (image, tmp, width, height,
                           gdtype,
                           100,
                           gimp_image_get_default_new_layer_mode (image));
  g_free (tmp);

  gimp_image_insert_layer (image, *layer, NULL, 0);

  return image;
}


/* Skip PNM image generated from PostScript file. */
/* Return TRUE on success, FALSE otherwise.       */
static gboolean
skip_ps (FILE *ifp)
{
  guchar  buf[8192];
  gsize   len;
  gint    pnmtype, width, height, maxval, bpl;

  pnmtype = read_pnmraw_type (ifp, &width, &height, &maxval);

  if (pnmtype == 4)    /* Portable bitmap */
    bpl = (width + 7) / 8;
  else if (pnmtype == 5)
    bpl = width;
  else if (pnmtype == 6)
    bpl = width * 3;
  else
    return FALSE;

  len = bpl * height;
  while (len)
    {
      gsize  bytes = fread (buf, 1, MIN (len, sizeof (buf)), ifp);

      if (bytes < MIN (len, sizeof (buf)))
        return FALSE;

      len -= bytes;
    }

  return TRUE;
}


/* Load PNM image generated from PostScript file */
static GimpImage *
load_ps (GFile *file,
         guint  pagenum,
         FILE  *ifp,
         gint   llx,
         gint   lly,
         gint   urx,
         gint   ury)
{
  guchar *dest;
  guchar *data, *bitline = NULL, *byteline = NULL, *byteptr, *temp;
  guchar  bit2byte[256*8];
  int     width, height, tile_height, scan_lines, total_scan_lines;
  int     image_width, image_height;
  int     skip_left, skip_bottom;
  int     i, j, pnmtype, maxval, bpp, nread;
  GimpImageBaseType imagetype;
  GimpImage  *image;
  GimpLayer  *layer;
  GeglBuffer *buffer = NULL;
  int         err = 0;
  int         e;

  pnmtype = read_pnmraw_type (ifp, &width, &height, &maxval);

  if ((width == urx+1) && (height == ury+1))  /* gs respected BoundingBox ? */
    {
      skip_left = llx;    skip_bottom = lly;
      image_width = width - skip_left;
      image_height = height - skip_bottom;
    }
  else
    {
      skip_left = skip_bottom = 0;
      image_width = width;
      image_height = height;
    }
  if (pnmtype == 4)   /* Portable Bitmap */
    {
      imagetype = GIMP_INDEXED;
      nread = (width+7)/8;
      bpp = 1;
      bitline = g_new (guchar, nread);
      byteline = g_new (guchar, nread * 8);

      /* Get an array for mapping 8 bits in a byte to 8 bytes */
      temp = bit2byte;
      for (j = 0; j < 256; j++)
        for (i = 7; i >= 0; i--)
          *(temp++) = ((j & (1 << i)) != 0);
    }
  else if (pnmtype == 5)  /* Portable Greymap */
    {
      imagetype = GIMP_GRAY;
      nread = width;
      bpp = 1;
      byteline = g_new (guchar, nread);
    }
  else if (pnmtype == 6)  /* Portable Pixmap */
    {
      imagetype = GIMP_RGB;
      nread = width * 3;
      bpp = 3;
      byteline = g_new (guchar, nread);
    }
  else
    return NULL;

  image = create_new_image (file, pagenum,
                            image_width, image_height, imagetype,
                            &layer);
  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  tile_height = gimp_tile_height ();
  data = g_malloc (tile_height * image_width * bpp);

  dest = data;
  total_scan_lines = scan_lines = 0;

  if (pnmtype == 4)   /* Read bitimage ? Must be mapped to indexed */
    {
      const guchar BWColorMap[2*3] = { 255, 255, 255, 0, 0, 0 };

      gimp_image_set_colormap (image, BWColorMap, 2);

      for (i = 0; i < height; i++)
        {
          e = (fread (bitline, 1, nread, ifp) != nread);
          if (total_scan_lines >= image_height)
            continue;
          err |= e;
          if (err)
            break;

          j = width;   /* Map 1 byte of bitimage to 8 bytes of indexed image */
          temp = bitline;
          byteptr = byteline;
          while (j >= 8)
            {
              memcpy (byteptr, bit2byte + *(temp++)*8, 8);
              byteptr += 8;
              j -= 8;
            }
          if (j > 0)
            memcpy (byteptr, bit2byte + *temp*8, j);

          memcpy (dest, byteline+skip_left, image_width);
          dest += image_width;
          scan_lines++;
          total_scan_lines++;

          if ((scan_lines == tile_height) || ((i + 1) == image_height))
            {
              gegl_buffer_set (buffer,
                               GEGL_RECTANGLE (0, i-scan_lines+1,
                                               image_width, scan_lines),
                               0,
                               NULL,
                               data,
                               GEGL_AUTO_ROWSTRIDE);
              scan_lines = 0;
              dest = data;
            }

          if ((i % 20) == 0)
            gimp_progress_update ((gdouble) (i + 1) / (gdouble) image_height);

          if (err)
            break;
        }
    }
  else   /* Read gray/rgb-image */
    {
      for (i = 0; i < height; i++)
        {
          e = (fread (byteline, bpp, width, ifp) != width);
          if (total_scan_lines >= image_height)
            continue;
          err |= e;
          if (err)
            break;

          memcpy (dest, byteline+skip_left*bpp, image_width*bpp);
          dest += image_width*bpp;
          scan_lines++;
          total_scan_lines++;

          if ((scan_lines == tile_height) || ((i + 1) == image_height))
            {
              gegl_buffer_set (buffer,
                               GEGL_RECTANGLE (0, i-scan_lines+1,
                                               image_width, scan_lines),
                               0,
                               NULL,
                               data,
                               GEGL_AUTO_ROWSTRIDE);
              scan_lines = 0;
              dest = data;
            }

          if ((i % 20) == 0)
            gimp_progress_update ((gdouble) (i + 1) / (gdouble) image_height);

          if (err)
            break;
        }
    }
  gimp_progress_update (1.0);

  g_free (data);
  g_free (byteline);
  g_free (bitline);

  if (err)
    g_message ("EOF encountered on reading");

  g_object_unref (buffer);

  return (err ? NULL : image);
}


/* Write out the PostScript file header */
static gboolean
save_ps_header (GOutputStream  *output,
                GFile          *file,
                GObject        *config,
                GError        **error)
{
  gchar   *basename = g_path_get_basename (gimp_file_get_utf8_name (file));
  gboolean eps      = FALSE;
  gboolean level2   = FALSE;
  time_t   cutime   = time (NULL);

  g_object_get (config,
                "eps-flag", &eps,
                "level",    &level2,
                NULL);

  if (! print (output, error,
               "%%!PS-Adobe-3.0%s\n", eps ? " EPSF-3.0" : ""))
    goto fail;

  if (! print (output, error,
               "%%%%Creator: GIMP PostScript file plug-in V %4.2f "
               "by Peter Kirchgessner\n", VERSION))
    goto fail;

  if (! print (output, error,
               "%%%%Title: %s\n"
               "%%%%CreationDate: %s"
               "%%%%DocumentData: Clean7Bit\n",
               basename, ctime (&cutime)))
    goto fail;

  if (eps || level2)
    if (! print (output, error,"%%%%LanguageLevel: 2\n"))
      goto fail;

  if (! print (output, error, "%%%%Pages: 1\n"))
    goto fail;

  g_free (basename);

  return TRUE;

 fail:

  g_free (basename);

  return FALSE;
}


/* Write out transformation for image */
static gboolean
save_ps_setup (GOutputStream  *output,
               GimpDrawable   *drawable,
               gint            width,
               gint            height,
               GObject        *config,
               gint            bpp,
               GError        **error)
{
  gdouble  config_width;
  gdouble  config_height;
  gdouble  x_offset, y_offset, x_size, y_size;
  gdouble  urx, ury;
  gdouble  width_inch, height_inch;
  gdouble  f1, f2, dx, dy;
  gint     rotation;
  gint     xtrans, ytrans;
  gint     i_urx, i_ury;
  gint     preview_size;
  gboolean unit_mm;
  gboolean keep_ratio;
  gboolean show_preview;
  gboolean level2;
  gchar    tmpbuf1[G_ASCII_DTOSTR_BUF_SIZE];
  gchar    tmpbuf2[G_ASCII_DTOSTR_BUF_SIZE];

  /* initialize */
  g_object_get (config,
                "width",        &config_width,
                "height",       &config_height,
                "x-offset",     &x_offset,
                "y-offset",     &y_offset,
                "keep-ratio",   &keep_ratio,
                "level",        &level2,
                "rotation",     &rotation,
                "show-preview", &show_preview,
                "preview",      &preview_size,
                NULL);
  unit_mm = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                                 "unit");

  dx = 0.0;
  dy = 0.0;

  width_inch  = fabs (config_width);
  height_inch = fabs (config_height);

  if (unit_mm)
    {
      x_offset /= 25.4; y_offset /= 25.4;
      width_inch /= 25.4; height_inch /= 25.4;
    }

  if (keep_ratio)   /* Proportions to keep ? */
    {                        /* Fit the image into the allowed size */
      f1 = width_inch / (gint) width;
      f2 = height_inch / (gint) height;
      if (f1 < f2)
        height_inch = width_inch * (gdouble) height / (gdouble) width;
      else
        width_inch = fabs (height_inch) * (gdouble) width / (gdouble) height;
    }

  if ((rotation == 0) || (rotation == 180))
    {
      x_size = width_inch;
      y_size = height_inch;
    }
  else
    {
      y_size = width_inch;
      x_size = height_inch;
    }

  /* Round up upper right corner only for non-integer values */
  urx = (x_offset + x_size) * 72.0;
  ury = (y_offset + y_size) * 72.0;
  i_urx = (gint) urx;
  i_ury = (gint) ury;
  if (urx != (gdouble) i_urx) i_urx++;  /* Check for non-integer value */
  if (ury != (gdouble) i_ury) i_ury++;

  if (! print (output, error,
               "%%%%BoundingBox: %d %d %d %d\n"
               "%%%%EndComments\n",
               (gint) (x_offset * 72.0), (gint) (y_offset * 72.0), i_urx, i_ury))
    return FALSE;

  if (show_preview && (preview_size > 0))
    {
      if (! save_ps_preview (output, drawable, config, error))
        return FALSE;
    }

  if (! print (output, error,
               "%%%%BeginProlog\n"
               "%% Use own dictionary to avoid conflicts\n"
               "10 dict begin\n"
               "%%%%EndProlog\n"
               "%%%%Page: 1 1\n"
               "%% Translate for offset\n"
               "%s %s translate\n",
               g_ascii_dtostr (tmpbuf1, sizeof (tmpbuf1), x_offset * 72.0),
               g_ascii_dtostr (tmpbuf2, sizeof (tmpbuf2), y_offset * 72.0)))
    return FALSE;

  /* Calculate translation to startpoint of first scanline */
  switch (rotation)
    {
    case   0: dx = 0.0; dy = y_size * 72.0;
      break;
    case  90: dx = dy = 0.0;
      break;
    case 180: dx = x_size * 72.0; dy = 0.0;
      break;
    case 270: dx = x_size * 72.0; dy = y_size * 72.0;
      break;
    }

  if ((dx != 0.0) || (dy != 0.0))
    {
      if (! print (output, error,
                   "%% Translate to begin of first scanline\n"
                   "%s %s translate\n",
                   g_ascii_dtostr (tmpbuf1, sizeof (tmpbuf1), dx),
                   g_ascii_dtostr (tmpbuf2, sizeof (tmpbuf2), dy)))
        return FALSE;
    }

  if (rotation)
    if (! print (output, error, "%d rotate\n", (gint) rotation))
      return FALSE;

  if (! print (output, error,
               "%s %s scale\n",
               g_ascii_dtostr (tmpbuf1, sizeof (tmpbuf1),  72.0 * width_inch),
               g_ascii_dtostr (tmpbuf2, sizeof (tmpbuf2), -72.0 * height_inch)))
    return FALSE;

  /* Write the PostScript procedures to read the image */
  if (! level2)
    {
      if (! print (output, error,
                   "%% Variable to keep one line of raster data\n"))
        return FALSE;

      if (bpp == 1)
        {
          if (! print (output, error,
                       "/scanline %d string def\n", ((gint) width + 7) / 8))
            return FALSE;
        }
      else
        {
          if (! print (output, error,
                       "/scanline %d %d mul string def\n",
                       (gint) width, bpp / 8))
            return FALSE;
        }
    }

  if (! print (output, error,
               "%% Image geometry\n%d %d %d\n"
               "%% Transformation matrix\n",
               (gint) width, (gint) height, (bpp == 1) ? 1 : 8))
    return FALSE;

  xtrans = ytrans = 0;
  if (config_width  < 0.0)
    {
      width  = -width;
      xtrans = -width;
    }
  if (config_height < 0.0)
    {
      height = -height;
      ytrans = -height;
    }

  if (! print (output, error,
               "[ %d 0 0 %d %d %d ]\n", (gint) width, (gint) height, xtrans, ytrans))
    return FALSE;

  return TRUE;
}

static gboolean
save_ps_trailer (GOutputStream  *output,
                 GError        **error)
{
  return print (output, error,
                "%%%%Trailer\n"
                "end\n%%%%EOF\n");
}

/* Do a Floyd-Steinberg dithering on a grayscale scanline. */
/* linecount must keep the counter for the actual scanline (0, 1, 2, ...). */
/* If linecount is less than zero, all used memory is freed. */

static void
dither_grey (const guchar *grey,
             guchar       *bw,
             gint          npix,
             gint          linecount)
{
  static gboolean  do_init_arrays = TRUE;
  static gint     *fs_error = NULL;
  static gint      limit[1278];
  static gint      east_error[256];
  static gint      seast_error[256];
  static gint      south_error[256];
  static gint      swest_error[256];

  const guchar *greyptr;
  guchar       *bwptr, mask;
  gint         *fse;
  gint          x, greyval, fse_inline;

  if (linecount <= 0)
    {
      g_free (fs_error);

      if (linecount < 0)
        return;

      fs_error = g_new0 (gint, npix + 2);

      /* Initialize some arrays that speed up dithering */
      if (do_init_arrays)
        {
          gint i;

          do_init_arrays = FALSE;

          for (i = 0, x = -511; x <= 766; i++, x++)
            limit[i] = (x < 0) ? 0 : ((x > 255) ? 255 : x);

          for (greyval = 0; greyval < 256; greyval++)
            {
              east_error[greyval] = (greyval < 128) ?
                ((greyval * 79) >> 8) : (((greyval - 255) * 79) >> 8);
              seast_error[greyval] = (greyval < 128) ?
                ((greyval * 34) >> 8) : (((greyval - 255) * 34) >> 8);
              south_error[greyval] = (greyval < 128) ?
                ((greyval * 56) >> 8) : (((greyval - 255) * 56) >> 8);
              swest_error[greyval] = (greyval < 128) ?
                ((greyval * 12) >> 8) : (((greyval - 255) * 12) >> 8);
            }
        }
    }

  g_return_if_fail (fs_error != NULL);

  memset (bw, 0, (npix + 7) / 8); /* Initialize with white */

  greyptr = grey;
  bwptr = bw;
  mask = 0x80;

  fse_inline = fs_error[1];

  for (x = 0, fse = fs_error + 1; x < npix; x++, fse++)
    {
      greyval =
        limit[*(greyptr++) + fse_inline + 512];  /* 0 <= greyval <= 255 */

      if (greyval < 128)
        *bwptr |= mask;  /* Set a black pixel */

      /* Error distribution */
      fse_inline = east_error[greyval] + fse[1];
      fse[1] = seast_error[greyval];
      fse[0] += south_error[greyval];
      fse[-1] += swest_error[greyval];

      mask >>= 1;   /* Get mask for next b/w-pixel */

      if (!mask)
        {
          mask = 0x80;
          bwptr++;
        }
    }
}

/* Write a device independent screen preview */
static gboolean
save_ps_preview (GOutputStream  *output,
                 GimpDrawable   *drawable,
                 GObject        *config,
                 GError        **error)
{
  GimpImageType  drawable_type;
  GeglBuffer    *buffer = NULL;
  const Babl    *format;
  gint           bpp;
  guchar        *bwptr, *greyptr;
  gint           width, height, x, y, nbsl, out_count;
  gint           nchar_pl = 72, src_y;
  gdouble        f1, f2;
  guchar        *grey, *bw, *src_row, *src_ptr;
  guchar        *cmap;
  gint           ncols, cind;
  gint           preview_size;

  g_object_get (config,
                "preview", &preview_size,
                NULL);

  if (preview_size <= 0)
    return TRUE;

  buffer = gimp_drawable_get_buffer (drawable);
  cmap = NULL;

  drawable_type = gimp_drawable_type (drawable);
  switch (drawable_type)
    {
    case GIMP_GRAY_IMAGE:
      format = babl_format ("Y' u8");
      break;

    case GIMP_INDEXED_IMAGE:
      cmap = gimp_image_get_colormap (gimp_item_get_image (GIMP_ITEM (drawable)),
                                      NULL,
                                      &ncols);
      format = gimp_drawable_get_format (drawable);
      break;

    case GIMP_RGB_IMAGE:
    default:
      format = babl_format ("R'G'B' u8");
      break;
    }

  bpp = babl_format_get_bytes_per_pixel (format);

  width = gegl_buffer_get_width (buffer);
  height = gegl_buffer_get_height (buffer);

  /* Calculate size of preview */
  if ((width > preview_size) ||
      (height > preview_size))
    {
      f1 = (gdouble) preview_size / (gdouble) width;
      f2 = (gdouble) preview_size / (gdouble) height;

      if (f1 < f2)
        {
          width = preview_size;
          height *= f1;
          if (height <= 0)
            height = 1;
        }
      else
        {
          height = preview_size;
          width *= f1;
          if (width <= 0)
            width = 1;
        }
    }

  nbsl = (width + 7) / 8;  /* Number of bytes per scanline in bitmap */

  grey    = g_new (guchar, width);
  bw      = g_new (guchar, nbsl);
  src_row = g_new (guchar, gegl_buffer_get_width (buffer) * bpp);

  if (! print (output, error,
               "%%%%BeginPreview: %d %d 1 %d\n",
               width, height,
               ((nbsl * 2 + nchar_pl - 1) / nchar_pl) * height))
    goto fail;

  for (y = 0; y < height; y++)
    {
      /* Get a scanline from the input image and scale it to the desired
         width */
      src_y = (y * gegl_buffer_get_height (buffer)) / height;
      gegl_buffer_get (buffer,
                       GEGL_RECTANGLE (0, src_y,
                                       gegl_buffer_get_width (buffer), 1),
                       1.0, format, src_row,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      greyptr = grey;
      if (bpp == 3)   /* RGB-image */
        {
          for (x = 0; x < width; x++)
            {                       /* Convert to grey */
              src_ptr = src_row + ((x * gegl_buffer_get_width (buffer)) / width) * 3;
              *(greyptr++) = (3*src_ptr[0] + 6*src_ptr[1] + src_ptr[2]) / 10;
            }
        }
      else if (cmap)    /* Indexed image */
        {
          for (x = 0; x < width; x++)
            {
              src_ptr = src_row + ((x * gegl_buffer_get_width (buffer)) / width);
              cind = *src_ptr;   /* Get color index and convert to grey */
              src_ptr = (cind >= ncols) ? cmap : (cmap + 3*cind);
              *(greyptr++) = (3*src_ptr[0] + 6*src_ptr[1] + src_ptr[2]) / 10;
            }
        }
      else             /* Grey image */
        {
          for (x = 0; x < width; x++)
            *(greyptr++) = *(src_row + ((x * gegl_buffer_get_width (buffer)) / width));
        }

      /* Now we have a grayscale line for the desired width. */
      /* Dither it to b/w */
      dither_grey (grey, bw, width, y);

      /* Write out the b/w line */
      out_count = 0;
      bwptr = bw;
      for (x = 0; x < nbsl; x++)
        {
          if (out_count == 0)
            if (! print (output, error, "%% "))
              goto fail;

          if (! print (output, error, "%02x", *(bwptr++)))
            goto fail;

          out_count += 2;
          if (out_count >= nchar_pl)
            {
              if (! print (output, error, "\n"))
                goto fail;

              out_count = 0;
            }
        }

      if (! print (output, error, "\n"))
        goto fail;

      if ((y % 20) == 0)
        gimp_progress_update ((gdouble) y / (gdouble) height);
    }

  gimp_progress_update (1.0);

  if (! print (output, error, "%%%%EndPreview\n"))
    goto fail;

  dither_grey (grey, bw, width, -1);
  g_free (src_row);
  g_free (bw);
  g_free (grey);

  g_object_unref (buffer);

  return TRUE;

 fail:

  g_free (src_row);
  g_free (bw);
  g_free (grey);

  g_object_unref (buffer);

  return FALSE;
}

static gboolean
save_gray (GOutputStream  *output,
           GimpImage      *image,
           GimpDrawable   *drawable,
           GObject        *config,
           GError        **error)
{
  GeglBuffer *buffer = NULL;
  const Babl *format;
  gboolean    level2;
  gint        bpp;
  gint        height, width, i, j;
  gint        tile_height;
  guchar     *data;
  guchar     *src;
  guchar     *packb = NULL;

  g_object_get (config,
                "level", &level2,
                NULL);

  buffer = gimp_drawable_get_buffer (drawable);
  format = babl_format ("Y' u8");
  bpp    = babl_format_get_bytes_per_pixel (format);
  width  = gegl_buffer_get_width (buffer);
  height = gegl_buffer_get_height (buffer);

  tile_height = gimp_tile_height ();

  /* allocate a buffer for retrieving information from the pixel region  */
  src = data = (guchar *) g_malloc (tile_height * width * bpp);

  /* Set up transformation in PostScript */
  if (! save_ps_setup (output, drawable, width, height, config, 1 * 8, error))
    goto fail;

  /* Write read image procedure */
  if (! level2)
    {
      if (! print (output, error,
                   "{ currentfile scanline readhexstring pop }\n"))
        goto fail;
    }
  else
    {
      if (! print (output, error,
                   "currentfile /ASCII85Decode filter /RunLengthDecode filter\n"))
        goto fail;

      ascii85_init ();

      /* Allocate buffer for packbits data. Worst case: Less than 1% increase */
      packb = (guchar *) g_malloc ((width * 105) / 100 + 2);
    }

  if (! ps_begin_data (output, error))
    goto fail;

  if (! print (output, error, "image\n"))
    goto fail;

#define GET_GRAY_TILE(begin) \
  { gint scan_lines;                                                    \
    scan_lines = (i+tile_height-1 < height) ? tile_height : (height-i); \
    gegl_buffer_get (buffer, GEGL_RECTANGLE (0, i, width, scan_lines),  \
                     1.0, format, begin,                                \
                     GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);             \
    src = begin; }

  for (i = 0; i < height; i++)
    {
      if ((i % tile_height) == 0)
        GET_GRAY_TILE (data); /* Get more data */

      if (! level2)
        {
          for (j = 0; j < width; j++)
            {
              if (! print (output, error, "%c", hex[(*src) >> 4]))
                goto fail;

              if (! print (output, error, "%c", hex[(*(src++)) & 0x0f]))
                goto fail;

              if (((j + 1) % 39) == 0)
                if (! print (output, error, "\n"))
                  goto fail;
           }

          if (! print (output, error, "\n"))
            goto fail;
        }
      else
        {
          gint nout;

          compress_packbits (width, src, &nout, packb);

          if (! ascii85_nout (output, nout, packb, error))
            goto fail;

          src += width;
        }

      if ((i % 20) == 0)
        gimp_progress_update ((gdouble) i / (gdouble) height);
    }

  gimp_progress_update (1.0);

  if (level2)
    {
      /* Write EOD of RunLengthDecode filter */
      if (! ascii85_out (output, 128, error))
        goto fail;

      if (! ascii85_done (output, error))
        goto fail;
    }

  if (! ps_end_data (output, error))
    return FALSE;

  if (! print (output, error, "showpage\n"))
    goto fail;

  g_free (data);
  g_free (packb);

  g_object_unref (buffer);

  return TRUE;

 fail:

  g_free (data);
  g_free (packb);

  g_object_unref (buffer);

  return FALSE;

#undef GET_GRAY_TILE
}

static gboolean
save_bw (GOutputStream  *output,
         GimpImage      *image,
         GimpDrawable   *drawable,
         GObject        *config,
         GError        **error)
{
  GeglBuffer *buffer = NULL;
  const Babl *format;
  gboolean    level2;
  gint        bpp;
  gint        height, width, i, j;
  gint        ncols, nbsl, nwrite;
  gint        tile_height;
  guchar     *cmap, *ct;
  guchar     *data, *src;
  guchar     *packb = NULL;
  guchar     *scanline, *dst, mask;
  guchar     *hex_scanline;

  g_object_get (config,
                "level", &level2,
                NULL);

  cmap = gimp_image_get_colormap (image, NULL, &ncols);

  buffer = gimp_drawable_get_buffer (drawable);
  format = gimp_drawable_get_format (drawable);
  bpp    = babl_format_get_bytes_per_pixel (format);
  width  = gegl_buffer_get_width (buffer);
  height = gegl_buffer_get_height (buffer);

  tile_height = gimp_tile_height ();

  /* allocate a buffer for retrieving information from the pixel region  */
  src = data = g_new (guchar, tile_height * width * bpp);

  nbsl = (width + 7) / 8;

  scanline     = g_new (guchar, nbsl + 1);
  hex_scanline = g_new (guchar, (nbsl + 1) * 2);

  /* Set up transformation in PostScript */
  if (! save_ps_setup (output, drawable, width, height, config, 1, error))
    goto fail;

  /* Write read image procedure */
  if (! level2)
    {
      if (! print (output, error,
                   "{ currentfile scanline readhexstring pop }\n"))
        goto fail;
    }
  else
    {
      if (! print (output, error,
                   "currentfile /ASCII85Decode filter /RunLengthDecode filter\n"))
        goto fail;

      ascii85_init ();

      /* Allocate buffer for packbits data. Worst case: Less than 1% increase */
      packb = g_new (guchar, ((nbsl+1) * 105) / 100 + 2);
    }

  if (! ps_begin_data (output, error))
    goto fail;

  if (! print (output, error, "image\n"))
    goto fail;

#define GET_BW_TILE(begin) \
  { gint scan_lines;                                                    \
    scan_lines = (i+tile_height-1 < height) ? tile_height : (height-i); \
    gegl_buffer_get (buffer, GEGL_RECTANGLE (0, i, width, scan_lines),  \
                     1.0, format, begin,                                \
                     GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);             \
    src = begin; }

  for (i = 0; i < height; i++)
    {
      if ((i % tile_height) == 0)
        GET_BW_TILE (data); /* Get more data */

      dst = scanline;
      memset (dst, 0, nbsl);
      mask = 0x80;

      /* Build a bitmap for a scanline */
      for (j = 0; j < width; j++)
        {
          ct = cmap + *(src++)*3;
          if (ct[0] || ct[1] || ct[2])
            *dst |= mask;

          if (mask == 0x01)
            {
              mask = 0x80; dst++;
            }
          else
            {
              mask >>= 1;
            }
        }

      if (! level2)
        {
          /* Convert to hexstring */
          for (j = 0; j < nbsl; j++)
            {
              hex_scanline[j * 2]     = (guchar) hex[scanline[j] >> 4];
              hex_scanline[j * 2 + 1] = (guchar) hex[scanline[j] & 0x0f];
            }

          /* Write out hexstring */
          j = nbsl * 2;
          dst = hex_scanline;

          while (j > 0)
            {
              nwrite = (j > 78) ? 78 : j;

              if (! g_output_stream_write_all (output,
                                               dst, nwrite, NULL,
                                               NULL, error))
                goto fail;

              if (! print (output, error, "\n"))
                goto fail;

              j -= nwrite;
              dst += nwrite;
            }
        }
      else
        {
          gint nout;

          compress_packbits (nbsl, scanline, &nout, packb);

          if (! ascii85_nout (output, nout, packb, error))
            goto fail;
        }

      if ((i % 20) == 0)
        gimp_progress_update ((gdouble) i / (gdouble) height);
    }

  gimp_progress_update (1.0);

  if (level2)
    {
      /* Write EOD of RunLengthDecode filter */
      if (! ascii85_out (output, 128, error))
        goto fail;

      if (! ascii85_done (output, error))
        goto fail;
    }

  if (! ps_end_data (output, error))
    goto fail;

  if (! print (output, error, "showpage\n"))
    goto fail;

  g_free (hex_scanline);
  g_free (scanline);
  g_free (data);
  g_free (packb);

  g_object_unref (buffer);

  return TRUE;

 fail:

  g_free (hex_scanline);
  g_free (scanline);
  g_free (data);
  g_free (packb);

  g_object_unref (buffer);

  return FALSE;

#undef GET_BW_TILE
}

static gboolean
save_index (GOutputStream  *output,
            GimpImage      *image,
            GimpDrawable   *drawable,
            GObject        *config,
            GError        **error)
{
  GeglBuffer *buffer = NULL;
  const Babl *format;
  gboolean    level2;
  gint        bpp;
  gint        height, width, i, j;
  gint        ncols, bw;
  gint        tile_height;
  guchar     *cmap, *cmap_start;
  guchar     *data, *src;
  guchar     *packb = NULL;
  guchar     *plane = NULL;
  gchar       coltab[256 * 6], *ct;

  g_object_get (config,
                "level", &level2,
                NULL);

  cmap = cmap_start = gimp_image_get_colormap (image, NULL, &ncols);

  ct = coltab;
  bw = 1;
  for (j = 0; j < 256; j++)
    {
      if (j >= ncols)
        {
          memset (ct, 0, 6);
          ct += 6;
        }
      else
        {
          bw &= ((cmap[0] == 0)   && (cmap[1] == 0)   && (cmap[2] == 0)) ||
                ((cmap[0] == 255) && (cmap[1] == 255) && (cmap[2] == 255));

          *(ct++) = (guchar) hex[(*cmap) >> 4];
          *(ct++) = (guchar) hex[(*(cmap++)) & 0x0f];
          *(ct++) = (guchar) hex[(*cmap) >> 4];
          *(ct++) = (guchar) hex[(*(cmap++)) & 0x0f];
          *(ct++) = (guchar) hex[(*cmap) >> 4];
          *(ct++) = (guchar) hex[(*(cmap++)) & 0x0f];
        }
    }

  if (bw)
    return save_bw (output, image, drawable, config, error);

  buffer = gimp_drawable_get_buffer (drawable);
  format = gimp_drawable_get_format (drawable);
  bpp    = babl_format_get_bytes_per_pixel (format);
  width  = gegl_buffer_get_width (buffer);
  height = gegl_buffer_get_height (buffer);

  tile_height = gimp_tile_height ();

  /* allocate a buffer for retrieving information from the pixel region  */
  src = data = (guchar *) g_malloc (tile_height * width * bpp);

  /* Set up transformation in PostScript */
  if (! save_ps_setup (output, drawable, width, height, config, 3 * 8, error))
    goto fail;

  /* Write read image procedure */
  if (! level2)
    {
      if (! print (output, error,
                   "{ currentfile scanline readhexstring pop } false 3\n"))
        goto fail;
    }
  else
    {
      if (! print (output, error,
                   "%% Strings to hold RGB-samples per scanline\n"
                   "/rstr %d string def\n"
                   "/gstr %d string def\n"
                   "/bstr %d string def\n",
                   width, width, width))
        goto fail;

      if (! print (output, error,
                   "{currentfile /ASCII85Decode filter /RunLengthDecode filter\
 rstr readstring pop}\n"
                   "{currentfile /ASCII85Decode filter /RunLengthDecode filter\
 gstr readstring pop}\n"
                   "{currentfile /ASCII85Decode filter /RunLengthDecode filter\
 bstr readstring pop}\n"
                   "true 3\n"))
        goto fail;

      /* Allocate buffer for packbits data. Worst case: Less than 1% increase */
      packb = (guchar *) g_malloc ((width * 105) / 100 + 2);
      plane = (guchar *) g_malloc (width);
    }

  if (! ps_begin_data (output, error))
    goto fail;

  if (! print (output, error, "colorimage\n"))
    goto fail;

#define GET_INDEX_TILE(begin) \
  { gint scan_lines;                                                    \
    scan_lines = (i+tile_height-1 < height) ? tile_height : (height-i); \
    gegl_buffer_get (buffer, GEGL_RECTANGLE (0, i, width, scan_lines),  \
                     1.0, format, begin,                                \
                     GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);             \
    src = begin; }

  for (i = 0; i < height; i++)
    {
      if ((i % tile_height) == 0)
        GET_INDEX_TILE (data); /* Get more data */

      if (! level2)
        {
          for (j = 0; j < width; j++)
            {
              if (! g_output_stream_write_all (output,
                                               coltab + (*(src++)) * 6, 6, NULL,
                                               NULL, error))
                goto fail;

              if (((j + 1) % 13) == 0)
                if (! print (output, error, "\n"))
                  goto fail;
            }

          if (! print (output, error, "\n"))
            goto fail;
        }
      else
        {
          gint rgb;

          for (rgb = 0; rgb < 3; rgb++)
            {
              guchar *src_ptr   = src;
              guchar *plane_ptr = plane;
              gint    nout;

              for (j = 0; j < width; j++)
                *(plane_ptr++) = cmap_start[3 * *(src_ptr++) + rgb];

              compress_packbits (width, plane, &nout, packb);

              ascii85_init ();

              if (! ascii85_nout (output, nout, packb, error))
                goto fail;

              /* Write EOD of RunLengthDecode filter */
              if (! ascii85_out (output, 128, error))
                goto fail;

              if (! ascii85_done (output, error))
                goto fail;
            }

          src += width;
        }

      if ((i % 20) == 0)
        gimp_progress_update ((gdouble) i / (gdouble) height);
    }

  gimp_progress_update (1.0);

  if (! ps_end_data (output, error))
    goto fail;

  if (! print (output, error, "showpage\n"))
    goto fail;

  g_free (data);
  g_free (packb);
  g_free (plane);

  g_object_unref (buffer);

  return TRUE;

 fail:

  g_free (data);
  g_free (packb);
  g_free (plane);

  g_object_unref (buffer);

  return FALSE;

#undef GET_INDEX_TILE
}

static gboolean
save_rgb (GOutputStream  *output,
          GimpImage      *image,
          GimpDrawable   *drawable,
          GObject        *config,
          GError        **error)
{
  GeglBuffer *buffer = NULL;
  const Babl *format;
  gboolean    level2;
  gint        bpp;
  gint        height, width, tile_height;
  gint        i, j;
  guchar     *data, *src;
  guchar     *packb = NULL;
  guchar     *plane = NULL;

  g_object_get (config,
                "level", &level2,
                NULL);

  buffer = gimp_drawable_get_buffer (drawable);
  format = babl_format ("R'G'B' u8");
  bpp    = babl_format_get_bytes_per_pixel (format);
  width  = gegl_buffer_get_width (buffer);
  height = gegl_buffer_get_height (buffer);

  tile_height = gimp_tile_height ();

  /* allocate a buffer for retrieving information from the pixel region  */
  src = data = g_new (guchar, tile_height * width * bpp);

  /* Set up transformation in PostScript */
  if (! save_ps_setup (output, drawable, width, height, config, 3 * 8, error))
    goto fail;

  /* Write read image procedure */
  if (! level2)
    {
      if (! print (output, error,
                   "{ currentfile scanline readhexstring pop } false 3\n"))
        goto fail;
    }
  else
    {
      if (! print (output, error,
                   "%% Strings to hold RGB-samples per scanline\n"
                   "/rstr %d string def\n"
                   "/gstr %d string def\n"
                   "/bstr %d string def\n",
                   width, width, width))
        goto fail;

      if (! print (output, error,
                   "{currentfile /ASCII85Decode filter /RunLengthDecode filter\
 rstr readstring pop}\n"
                   "{currentfile /ASCII85Decode filter /RunLengthDecode filter\
 gstr readstring pop}\n"
                   "{currentfile /ASCII85Decode filter /RunLengthDecode filter\
 bstr readstring pop}\n"
                   "true 3\n"))
        goto fail;

      /* Allocate buffer for packbits data. Worst case: Less than 1% increase */
      packb = g_new (guchar, (width * 105) / 100 + 2);
      plane = g_new (guchar, width);
    }

  if (! ps_begin_data (output, error))
    goto fail;

  if (! print (output, error, "colorimage\n"))
    goto fail;

#define GET_RGB_TILE(begin) \
  { gint scan_lines;                                                    \
    scan_lines = (i+tile_height-1 < height) ? tile_height : (height-i); \
    gegl_buffer_get (buffer, GEGL_RECTANGLE (0, i, width, scan_lines),  \
                     1.0, format, begin,                                \
                     GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);             \
    src = begin; }

  for (i = 0; i < height; i++)
    {
      if ((i % tile_height) == 0)
        GET_RGB_TILE (data); /* Get more data */

      if (! level2)
        {
          for (j = 0; j < width; j++)
            {
              if (! print (output, error, "%c",
                           hex[(*src) >> 4]))        /* Red */
                goto fail;

              if (! print (output, error, "%c",
                           hex[(*(src++)) & 0x0f]))
                goto fail;

              if (! print (output, error, "%c",
                           hex[(*src) >> 4]))        /* Green */
                goto fail;

              if (! print (output, error, "%c",
                           hex[(*(src++)) & 0x0f]))
                goto fail;

              if (! print (output, error, "%c",
                           hex[(*src) >> 4]))        /* Blue */
                goto fail;

              if (! print (output, error, "%c",
                           hex[(*(src++)) & 0x0f]))
                goto fail;

              if (((j+1) % 13) == 0)
                if (! print (output, error, "\n"))
                  goto fail;
            }

          if (! print (output, error, "\n"))
            goto fail;
        }
      else
        {
          gint rgb;

          for (rgb = 0; rgb < 3; rgb++)
            {
              guchar *src_ptr   = src + rgb;
              guchar *plane_ptr = plane;
              gint    nout;

              for (j = 0; j < width; j++)
                {
                  *(plane_ptr++) = *src_ptr;
                  src_ptr += 3;
                }

              compress_packbits (width, plane, &nout, packb);

              ascii85_init ();

              if (! ascii85_nout (output, nout, packb, error))
                goto fail;

              /* Write EOD of RunLengthDecode filter */
              if (! ascii85_out (output, 128, error))
                goto fail;

              if (! ascii85_done (output, error))
                goto fail;
            }

          src += 3 * width;
        }

      if ((i % 20) == 0)
        gimp_progress_update ((gdouble) i / (gdouble) height);
    }

  gimp_progress_update (1.0);

  if (! ps_end_data (output, error))
    goto fail;

  if (! print (output, error, "showpage\n"))
    goto fail;

  g_free (data);
  g_free (packb);
  g_free (plane);

  g_object_unref (buffer);

  return TRUE;

 fail:

  g_free (data);
  g_free (packb);
  g_free (plane);

  g_object_unref (buffer);

  return FALSE;

#undef GET_RGB_TILE
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

/*  Load interface functions  */

static gint32
count_ps_pages (GFile *file)
{
  FILE   *psfile;
  gchar  *extension;
  gchar   buf[1024];
  gint32  num_pages      = 0;
  gint32  showpage_count = 0;

  extension = strrchr (g_file_peek_path (file), '.');
  if (extension)
    {
      extension = g_ascii_strdown (extension + 1, -1);

      if (strcmp (extension, "eps") == 0)
        {
          g_free (extension);
          return 1;
        }

      g_free (extension);
    }

  psfile = g_fopen (g_file_peek_path (file), "r");

  if (psfile == NULL)
    {
      g_message (_("Could not open '%s' for reading: %s"),
                 gimp_file_get_utf8_name (file), g_strerror (errno));
      return 0;
    }

  while (num_pages == 0 && !feof (psfile))
    {
      fgets (buf, sizeof (buf), psfile);

      if (strncmp (buf + 2, "Pages:", 6) == 0)
        sscanf (buf + strlen ("%%Pages:"), "%d", &num_pages);
      else if (strncmp (buf, "showpage", 8) == 0)
        showpage_count++;
    }

  if (feof (psfile) && num_pages < 1 && showpage_count > 0)
    num_pages = showpage_count;

  fclose (psfile);

  return num_pages;
}

static gboolean
load_dialog (GFile              *file,
             GimpVectorLoadData  extracted_data,
             GimpProcedure      *procedure,
             GObject            *config)
{
  GtkWidget     *dialog;
  GtkWidget     *hbox;
  GtkWidget     *vbox;
  GtkWidget     *target   = NULL;
  GtkWidget     *selector = NULL;
  gint32         page_count;
  gchar         *range    = NULL;
  gboolean       run;

  page_count = count_ps_pages (file);

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_vector_load_procedure_dialog_new (GIMP_VECTOR_LOAD_PROCEDURE (procedure),
                                                  GIMP_PROCEDURE_CONFIG (config),
                                                  &extracted_data, NULL);

  if (page_count > 1)
    {
      selector = gimp_page_selector_new ();
      gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                          selector, TRUE, TRUE, 0);
      gimp_page_selector_set_n_pages (GIMP_PAGE_SELECTOR (selector),
                                      page_count);
      gimp_page_selector_set_target (GIMP_PAGE_SELECTOR (selector),
                                     ps_pagemode);

      gtk_widget_set_visible (selector, TRUE);

      g_signal_connect_swapped (selector, "activate",
                                G_CALLBACK (gtk_window_activate_default),
                                dialog);
    }

  vbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                         "rendering-box",
                                         "check-bbox",
                                         NULL);

  if (page_count == 0)
    {
      GtkWidget *grid;
      GtkWidget *entry = NULL;

      grid = gtk_grid_new ();
      gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
      gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
      gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
      gtk_widget_set_visible (grid, TRUE);

      entry = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                                "pages", GTK_TYPE_ENTRY);
      gtk_widget_set_size_request (entry, 80, -1);
      gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                                _("Pages:"), 0.0, 0.5,
                                entry, 1);

      gimp_help_set_help_data (GTK_WIDGET (entry),
                               _("Pages to load (e.g.: 1-4 or 1,3,5-7)"),
                               NULL);

      target = gimp_enum_combo_box_new (GIMP_TYPE_PAGE_SELECTOR_TARGET);
      gtk_combo_box_set_active (GTK_COMBO_BOX (target), (int) ps_pagemode);
      gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                                _("Open as"), 0.0, 0.5,
                                target, 1);
    }

  /* Dialog formatting */
  hbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                         "ps-top-row",
                                         "rendering-box",
                                         NULL);
  gtk_box_set_spacing (GTK_BOX (hbox), 12);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
  gtk_widget_set_margin_bottom (hbox, 12);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (hbox),
                                  GTK_ORIENTATION_HORIZONTAL);

  vbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                         "ps-bottom-row",
                                         "coloring",
                                         "text-alpha-bits",
                                         "graphic-alpha-bits",
                                         NULL);
  gtk_box_set_spacing (GTK_BOX (vbox), 12);
  gtk_box_set_homogeneous (GTK_BOX (vbox), TRUE);
  gtk_widget_set_margin_bottom (vbox, 12);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              "ps-top-row",
                              "ps-bottom-row",
                              NULL);

  gtk_widget_set_visible (dialog, TRUE);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  if (selector)
    {
      range = gimp_page_selector_get_selected_range (GIMP_PAGE_SELECTOR (selector));

      if (strlen (range) < 1)
        {
          gimp_page_selector_select_all (GIMP_PAGE_SELECTOR (selector));
          range = gimp_page_selector_get_selected_range (GIMP_PAGE_SELECTOR (selector));
        }

      g_object_set (config,
                    "pages", range,
                    NULL);
      ps_pagemode = gimp_page_selector_get_target (GIMP_PAGE_SELECTOR (selector));
    }
  else if (page_count == 0)
    {
      ps_pagemode = gtk_combo_box_get_active (GTK_COMBO_BOX (target));
    }
  else
    {
      g_object_set (config,
                    "pages", "1",
                    NULL);
      ps_pagemode = GIMP_PAGE_SELECTOR_TARGET_IMAGES;
    }

  gtk_widget_destroy (dialog);

  return run;
}

/*  Save interface functions  */

static gboolean
save_dialog (GimpProcedure *procedure,
             GObject       *config,
             GimpImage     *image)
{
  GtkWidget      *dialog;
  GtkListStore   *store;
  GtkWidget      *toggle;
  GtkWidget      *combo;
  GtkWidget      *widget;
  GtkWidget      *hbox, *vbox;
  gboolean        run;

  dialog = gimp_export_procedure_dialog_new (GIMP_EXPORT_PROCEDURE (procedure),
                                             GIMP_PROCEDURE_CONFIG (config),
                                             image);

  /* Image Size */
  /* Width/Height/X-/Y-offset labels */
  toggle = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                             "keep-ratio", G_TYPE_NONE);

  gimp_help_set_help_data (toggle,
                           _("When toggled, the resulting image will be "
                             "scaled to fit into the given size without "
                             "changing the aspect ratio."),
                           "#keep_aspect_ratio"),

  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "image-title", _("Image Size"),
                                   FALSE, FALSE);

  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog), "image-box",
                                  "width", "height", "x-offset", "y-offset",
                                  "keep-ratio", NULL);

  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "image-frame", "image-title",
                                    FALSE, "image-box");

  /* Unit */
  widget = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                             "unit", G_TYPE_NONE);
  combo = gimp_label_string_widget_get_widget (GIMP_LABEL_STRING_WIDGET (widget));
  g_signal_connect (combo, "changed",
                    G_CALLBACK (save_unit_changed_update),
                    config);

  /* Rotation */
  store = gimp_int_store_new (_("_0"),   0,
                              _("_90"),  90,
                              _("_180"), 180,
                              _("_270"), 270,
                              NULL);
  gimp_procedure_dialog_get_int_radio (GIMP_PROCEDURE_DIALOG (dialog),
                                       "rotation", GIMP_INT_STORE (store));

  /* Output */
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "output-title", _("Output"),
                                   FALSE, FALSE);

  gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (dialog),
                                       "preview",
                                       TRUE, config, "show-preview", FALSE);


  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog), "output-box",
                                  "level", "eps-flag", "show-preview", "preview",
                                  NULL);

  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "output-frame", "output-title",
                                    FALSE, "output-box");

  /* Dialog Formatting */
  vbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog), "left-column",
                                         "image-frame", "unit",
                                         NULL);
  gtk_box_set_spacing (GTK_BOX (vbox), 12);
  vbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog), "right-column",
                                         "rotation", "output-frame",
                                         NULL);
  gtk_box_set_spacing (GTK_BOX (vbox), 12);
  hbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog), "ps-hbox",
                                         "left-column", "right-column",
                                         NULL);
  gtk_box_set_spacing (GTK_BOX (hbox), 12);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (hbox),
                                  GTK_ORIENTATION_HORIZONTAL);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              "ps-hbox",
                              NULL);

  gtk_widget_set_visible (dialog, TRUE);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}

static void
save_unit_changed_update (GtkWidget *widget,
                          gpointer   data)
{
  GimpProcedureConfig *config = GIMP_PROCEDURE_CONFIG (data);
  gdouble              factor;
  gboolean             unit_mm;
  gdouble              width;
  gdouble              height;
  gdouble              x_offset;
  gdouble              y_offset;

  g_object_get (config,
                "width",    &width,
                "height",   &height,
                "x-offset", &x_offset,
                "y-offset", &y_offset,
                NULL);
  unit_mm = gimp_procedure_config_get_choice_id (config, "unit");

  if (unit_mm)
    factor = 25.4;
  else
    factor = 1.0 / 25.4;

  g_object_set (config,
                "width",    width * factor,
                "height",   height * factor,
                "x-offset", x_offset * factor,
                "y-offset", y_offset * factor,
                NULL);
}
