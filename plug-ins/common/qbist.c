/*
 * Written 1997 Jens Ch. Restemeier <jrestemeier@currantbun.com>
 * This program is based on an algorithm / article by
 * Jörn Loviscach.
 *
 * It appeared in c't 10/95, page 326 and is called
 * "Ausgewürfelt - Moderne Kunst algorithmisch erzeugen"
 * (~modern art created with algorithms).
 *
 * It generates one main formula (the middle button) and 8 variations of it.
 * If you select a variation it becomes the new main formula. If you
 * press "OK" the main formula will be applied to the image.
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

#include "config.h"

#include <string.h>
#include <limits.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define MAX_TRANSFORMS  36
#define NUM_REGISTERS    6
#define PREVIEW_SIZE    64

#define PLUG_IN_PROC    "plug-in-qbist"
#define PLUG_IN_BINARY  "qbist"
#define PLUG_IN_ROLE    "gimp-qbist"
#define PLUG_IN_VERSION "January 2001, 1.12"


/* experiment with this */
typedef gfloat vreg[3];

typedef enum
{
  PROJECTION,
  SHIFT,
  SHIFTBACK,
  ROTATE,
  ROTATE2,
  MULTIPLY,
  SINE,
  CONDITIONAL,
  COMPLEMENT
} TransformType;

#define NUM_TRANSFORMS  (COMPLEMENT + 1)


typedef struct
{
  TransformType transformSequence[MAX_TRANSFORMS];
  gint          source[MAX_TRANSFORMS];
  gint          control[MAX_TRANSFORMS];
  gint          dest[MAX_TRANSFORMS];
} ExpInfo;

typedef struct
{
  ExpInfo  info;
  gint     oversampling;
  gchar   *path;
} QbistInfo;


typedef struct _Qbist      Qbist;
typedef struct _QbistClass QbistClass;

struct _Qbist
{
  GimpPlugIn parent_instance;
};

struct _QbistClass
{
  GimpPlugInClass parent_class;
};


#define QBIST_TYPE  (qbist_get_type ())
#define QBIST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), QBIST_TYPE, Qbist))

GType                   qbist_get_type         (void) G_GNUC_CONST;

static GList          * qbist_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * qbist_create_procedure (GimpPlugIn           *plug_in,
                                                const gchar          *name);

static GimpValueArray * qbist_run              (GimpProcedure        *procedure,
                                                GimpRunMode           run_mode,
                                                GimpImage            *image,
                                                gint                  n_drawables,
                                                GimpDrawable        **drawables,
                                                GimpProcedureConfig  *config,
                                                gpointer              run_data);

static gboolean         dialog_run             (GimpProcedure        *procedure,
                                                GimpProcedureConfig  *config);
static void             dialog_new_variations  (GtkWidget            *widget,
                                                gpointer              data);
static void             dialog_update_previews (GtkWidget            *widget,
                                                gint                  preview_size);
static void             dialog_select_preview  (GtkWidget            *widget,
                                                ExpInfo              *n_info);

static void             create_info            (ExpInfo              *info);
static void             optimize               (ExpInfo              *info);
static void             qbist                  (ExpInfo              *info,
                                                gfloat               *buffer,
                                                gint                  xp,
                                                gint                  yp,
                                                gint                  num,
                                                gint                  width,
                                                gint                  height,
                                                gint                  oversampling);


G_DEFINE_TYPE (Qbist, qbist, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (QBIST_TYPE)
DEFINE_STD_SET_I18N


static QbistInfo  qbist_info;
static GRand     *gr = NULL;


static void
qbist_class_init (QbistClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = qbist_query_procedures;
  plug_in_class->create_procedure = qbist_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
qbist_init (Qbist *qbist)
{
}

static GList *
qbist_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
qbist_create_procedure (GimpPlugIn  *plug_in,
                        const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            qbist_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB*");
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

      gimp_procedure_set_menu_label (procedure, _("_Qbist..."));
      gimp_procedure_add_menu_path (procedure,
                                    "<Image>/Filters/Render/Pattern");

      gimp_procedure_set_documentation (procedure,
                                        _("Generate a huge variety of "
                                          "abstract patterns"),
                                        _("This Plug-in is based on an article by "
                                          "Jörn Loviscach (appeared in c't 10/95, "
                                          "page 326). It generates modern art "
                                          "pictures from a random genetic "
                                          "formula."),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Jörn Loviscach, Jens Ch. Restemeier",
                                      "Jörn Loviscach, Jens Ch. Restemeier",
                                      PLUG_IN_VERSION);

      gimp_procedure_add_boolean_argument (procedure, "anti-aliasing",
                                           _("_Anti-aliasing"),
                                           _("Enable anti-aliasing using an oversampling "
                                             "algorithm"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      /* Saving the pattern as a parasite is a trick allowing to store
       * random binary data.
       */
      gimp_procedure_add_parasite_aux_argument (procedure, "pattern",
                                                "Qbist pattern", NULL,
                                                G_PARAM_READWRITE);

     gimp_procedure_add_string_aux_argument (procedure, "data-path",
                                              "Path of data file",
                                              _("Any file which will be used as source for pattern generation"),
                                              NULL,
                                              G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
qbist_run (GimpProcedure        *procedure,
           GimpRunMode           run_mode,
           GimpImage            *image,
           gint                  n_drawables,
           GimpDrawable        **drawables,
           GimpProcedureConfig  *config,
           gpointer              run_data)
{
  gint                sel_x1, sel_y1, sel_width, sel_height;
  gint                img_height, img_width;
  GeglBuffer         *buffer;
  GeglBufferIterator *iter;
  GimpDrawable       *drawable;
  GimpParasite       *pattern_parasite;
  gconstpointer       pattern_data;
  guint32             pattern_data_length;
  gint                total_pixels;
  gint                done_pixels;
  gboolean            anti_aliasing = TRUE;

  gegl_init (NULL, NULL);

  if (n_drawables != 1)
    {
      GError *error = NULL;

      g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   PLUG_IN_PROC);

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  img_width  = gimp_drawable_get_width (drawable);
  img_height = gimp_drawable_get_height (drawable);

  if (! gimp_drawable_is_rgb (drawable))
    {
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               NULL);
    }

  if (! gimp_drawable_mask_intersect (drawable,
                                      &sel_x1, &sel_y1,
                                      &sel_width, &sel_height))
    {
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_SUCCESS,
                                               NULL);
    }

  gr = g_rand_new ();

  memset (&qbist_info, 0, sizeof (qbist_info));
  create_info (&qbist_info.info);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      g_object_get (config,
                    "data-path",    &qbist_info.path,
                    "pattern",      &pattern_parasite,
                    NULL);
      if (pattern_parasite)
        {
          pattern_data = gimp_parasite_get_data (pattern_parasite,
                                                 &pattern_data_length);
          memcpy (&qbist_info.info, pattern_data, pattern_data_length);
          gimp_parasite_free (pattern_parasite);
          pattern_parasite = NULL;
        }

      if (! dialog_run (procedure, config))
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CANCEL,
                                                 NULL);

      g_object_get (config, "anti-aliasing", &anti_aliasing, NULL);

      pattern_parasite = gimp_parasite_new ("pattern", 0,
                                            sizeof (qbist_info.info),
                                            &qbist_info.info);
      g_object_set (config,
                    "data-path",    qbist_info.path,
                    "pattern",      pattern_parasite,
                    NULL);
      gimp_parasite_free (pattern_parasite);
      break;

    case GIMP_RUN_NONINTERACTIVE:
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               NULL);
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      g_object_get (config,
                    "data-path",     &qbist_info.path,
                    "anti-aliasing", &anti_aliasing,
                    "pattern",       &pattern_parasite,
                    NULL);

      if (pattern_parasite)
        {
          pattern_data = gimp_parasite_get_data (pattern_parasite,
                                                 &pattern_data_length);
          memcpy (&qbist_info.info, pattern_data, pattern_data_length);
          gimp_parasite_free (pattern_parasite);
          pattern_parasite = NULL;
        }
      break;
    }

  total_pixels = img_width * img_height;
  done_pixels  = 0;

  qbist_info.oversampling = anti_aliasing ? 4 : 1;

  buffer = gimp_drawable_get_shadow_buffer (drawable);

  iter = gegl_buffer_iterator_new (buffer,
                                   GEGL_RECTANGLE (0, 0,
                                                   img_width, img_height),
                                   0, babl_format ("R'G'B'A float"),
                                   GEGL_ACCESS_READWRITE,
                                   GEGL_ABYSS_NONE, 1);

  optimize (&qbist_info.info);

  gimp_progress_init (_("Qbist"));

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat        *data = iter->items[0].data;
      GeglRectangle  roi  = iter->items[0].roi;
      gint           row;

      for (row = 0; row < roi.height; row++)
        {
          qbist (&qbist_info.info,
                 data + row * roi.width * 4,
                 roi.x,
                 roi.y + row,
                 roi.width,
                 sel_width,
                 sel_height,
                 qbist_info.oversampling);
        }

      done_pixels += roi.width * roi.height;

      gimp_progress_update ((gdouble) done_pixels /
                            (gdouble) total_pixels);
    }

  g_object_unref (buffer);

  gimp_progress_update (1.0);

  gimp_drawable_merge_shadow (drawable, TRUE);
  gimp_drawable_update (drawable, sel_x1, sel_y1,
                        sel_width, sel_height);

  gimp_displays_flush ();

  g_rand_free (gr);

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}


/** qbist functions *********************************************************/

static void
create_info (ExpInfo *info)
{
  gint k;

  for (k = 0; k < MAX_TRANSFORMS; k++)
    {
      info->transformSequence[k] = g_rand_int_range (gr, 0, NUM_TRANSFORMS);
      info->source[k] = g_rand_int_range (gr, 0, NUM_REGISTERS);
      info->control[k] = g_rand_int_range (gr, 0, NUM_REGISTERS);
      info->dest[k] = g_rand_int_range (gr, 0, NUM_REGISTERS);
    }
}

static void
modify_info (ExpInfo *o_info,
             ExpInfo *n_info)
{
  gint k, n;

  *n_info = *o_info;
  n = g_rand_int_range (gr, 0, MAX_TRANSFORMS);
  for (k = 0; k < n; k++)
    {
      switch (g_rand_int_range (gr, 0, 4))
        {
        case 0:
          n_info->transformSequence[g_rand_int_range (gr, 0, MAX_TRANSFORMS)] =
            g_rand_int_range (gr, 0, NUM_TRANSFORMS);
          break;

        case 1:
          n_info->source[g_rand_int_range (gr, 0, MAX_TRANSFORMS)] =
            g_rand_int_range (gr, 0, NUM_REGISTERS);
          break;

        case 2:
          n_info->control[g_rand_int_range (gr, 0, MAX_TRANSFORMS)] =
            g_rand_int_range (gr, 0, NUM_REGISTERS);
          break;

        case 3:
          n_info->dest[g_rand_int_range (gr, 0, MAX_TRANSFORMS)] =
            g_rand_int_range (gr, 0, NUM_REGISTERS);
          break;
        }
    }
}

/*
 * Optimizer
 */
static gint used_trans_flag[MAX_TRANSFORMS];
static gint used_reg_flag[NUM_REGISTERS];

static void
check_last_modified (ExpInfo *info,
                     gint    p,
                     gint    n)
{
  p--;
  while ((p >= 0) && (info->dest[p] != n))
    p--;
  if (p < 0)
    used_reg_flag[n] = 1;
  else
    {
      used_trans_flag[p] = 1;
      check_last_modified (info, p, info->source[p]);
      check_last_modified (info, p, info->control[p]);
    }
}

static void
optimize (ExpInfo *info)
{
  gint i;

  /* double-arg fix: */
  for (i = 0; i < MAX_TRANSFORMS; i++)
    {
      used_trans_flag[i] = 0;
      if (i < NUM_REGISTERS)
        used_reg_flag[i] = 0;
      /* double-arg fix: */
      switch (info->transformSequence[i])
        {
        case ROTATE:
        case ROTATE2:
        case COMPLEMENT:
          info->control[i] = info->dest[i];
          break;

        default:
          break;
        }
    }
  /* check for last modified item */
  check_last_modified (info, MAX_TRANSFORMS, 0);
}

static void
qbist (ExpInfo *info,
       gfloat  *buffer,
       gint     xp,
       gint     yp,
       gint     num,
       gint     width,
       gint     height,
       gint     oversampling)
{
  gint gx;

  vreg reg[NUM_REGISTERS];

  for (gx = 0; gx < num; gx++)
    {
      gfloat accum[3];
      gint   yy, i;

      for (i = 0; i < 3; i++)
        {
          accum[i] = 0.0;
        }

      for (yy = 0; yy < oversampling; yy++)
        {
          gint xx;

          for (xx = 0; xx < oversampling; xx++)
            {
              for (i = 0; i < NUM_REGISTERS; i++)
                {
                  if (used_reg_flag[i])
                    {
                      reg[i][0] = ((gfloat) ((gx + xp) * oversampling + xx)) / ((gfloat) (width * oversampling));
                      reg[i][1] = ((gfloat) (yp * oversampling + yy)) / ((gfloat) (height * oversampling));
                      reg[i][2] = ((gfloat) i) / ((gfloat) NUM_REGISTERS);
                    }
                }

              for (i = 0; i < MAX_TRANSFORMS; i++)
                {
                  gushort sr, cr, dr;

                  sr = info->source[i];
                  cr = info->control[i];
                  dr = info->dest[i];

                  if (used_trans_flag[i])
                    switch (info->transformSequence[i])
                      {
                      case PROJECTION:
                        {
                          gfloat scalarProd;

                          scalarProd = (reg[sr][0] * reg[cr][0]) +
                            (reg[sr][1] * reg[cr][1]) + (reg[sr][2] * reg[cr][2]);

                          reg[dr][0] = scalarProd * reg[sr][0];
                          reg[dr][1] = scalarProd * reg[sr][1];
                          reg[dr][2] = scalarProd * reg[sr][2];
                          break;
                        }

                      case SHIFT:
                        reg[dr][0] = reg[sr][0] + reg[cr][0];
                        if (reg[dr][0] >= 1.0)
                          reg[dr][0] -= 1.0;
                        reg[dr][1] = reg[sr][1] + reg[cr][1];
                        if (reg[dr][1] >= 1.0)
                          reg[dr][1] -= 1.0;
                        reg[dr][2] = reg[sr][2] + reg[cr][2];
                        if (reg[dr][2] >= 1.0)
                          reg[dr][2] -= 1.0;
                        break;

                      case SHIFTBACK:
                        reg[dr][0] = reg[sr][0] - reg[cr][0];
                        if (reg[dr][0] <= 0.0)
                          reg[dr][0] += 1.0;
                        reg[dr][1] = reg[sr][1] - reg[cr][1];
                        if (reg[dr][1] <= 0.0)
                          reg[dr][1] += 1.0;
                        reg[dr][2] = reg[sr][2] - reg[cr][2];
                        if (reg[dr][2] <= 0.0)
                          reg[dr][2] += 1.0;
                        break;

                      case ROTATE:
                        reg[dr][0] = reg[sr][1];
                        reg[dr][1] = reg[sr][2];
                        reg[dr][2] = reg[sr][0];
                        break;

                      case ROTATE2:
                        reg[dr][0] = reg[sr][2];
                        reg[dr][1] = reg[sr][0];
                        reg[dr][2] = reg[sr][1];
                        break;

                      case MULTIPLY:
                        reg[dr][0] = reg[sr][0] * reg[cr][0];
                        reg[dr][1] = reg[sr][1] * reg[cr][1];
                        reg[dr][2] = reg[sr][2] * reg[cr][2];
                        break;

                      case SINE:
                        reg[dr][0] = 0.5 + (0.5 * sin (20.0 * reg[sr][0] * reg[cr][0]));
                        reg[dr][1] = 0.5 + (0.5 * sin (20.0 * reg[sr][1] * reg[cr][1]));
                        reg[dr][2] = 0.5 + (0.5 * sin (20.0 * reg[sr][2] * reg[cr][2]));
                        break;

                      case CONDITIONAL:
                        if ((reg[cr][0] + reg[cr][1] + reg[cr][2]) > 0.5)
                          {
                            reg[dr][0] = reg[sr][0];
                            reg[dr][1] = reg[sr][1];
                            reg[dr][2] = reg[sr][2];
                          }
                        else
                          {
                            reg[dr][0] = reg[cr][0];
                            reg[dr][1] = reg[cr][1];
                            reg[dr][2] = reg[cr][2];
                          }
                        break;

                      case COMPLEMENT:
                        reg[dr][0] = 1.0 - reg[sr][0];
                        reg[dr][1] = 1.0 - reg[sr][1];
                        reg[dr][2] = 1.0 - reg[sr][2];
                        break;
                      }
                }

              accum[0] += reg[0][0];
              accum[1] += reg[0][1];
              accum[2] += reg[0][2];
            }
        }

      buffer[0] = accum[0] / (gfloat) (oversampling * oversampling);
      buffer[1] = accum[1] / (gfloat) (oversampling * oversampling);
      buffer[2] = accum[2] / (gfloat) (oversampling * oversampling);
      buffer[3] = 1.0;

      buffer += 4;
    }
}


/** User interface ***********************************************************/

static GtkWidget *preview[9];
static ExpInfo    info[9];
static ExpInfo    last_info[9];

static void
dialog_new_variations (GtkWidget *widget,
                       gpointer   data)
{
  gint i;

  for (i = 1; i < 9; i++)
    modify_info (&(info[0]), &(info[i]));
}

static void
dialog_update_previews (GtkWidget *widget,
                        gint       preview_size)
{
  static const Babl *fish = NULL;
  static gint        size = -1;

  gfloat            *buf;
  guchar            *u8_buf;
  gint               i, j;

  if (preview_size > 0)
    size = preview_size;

  g_return_if_fail (size > 0);

  buf    = g_new (gfloat, size * size * 4);
  u8_buf = g_new (guchar, size * size * 4);

  if (! fish)
    fish = babl_fish (babl_format ("R'G'B'A float"),
                      babl_format ("R'G'B'A u8"));

  for (j = 0; j < 9; j++)
    {
      optimize (&info[(j + 5) % 9]);

      for (i = 0; i < size; i++)
        {
          qbist (&info[(j + 5) % 9], buf + i * size * 4,
                 0, i, size, size, size, 1);
        }

      babl_process (fish, buf, u8_buf, size * size);

      gimp_preview_area_draw (GIMP_PREVIEW_AREA (preview[j]),
                              0, 0, size, size,
                              GIMP_RGBA_IMAGE,
                              u8_buf,
                              size * 4);
    }

  g_free (buf);
  g_free (u8_buf);
}

static void
dialog_select_preview (GtkWidget *widget,
                       ExpInfo   *n_info)
{
  memcpy (last_info, info, sizeof (info));
  info[0] = *n_info;
  dialog_new_variations (widget, NULL);
  dialog_update_previews (widget, -1);
}

/* File I/O stuff */

static guint16
get_be16 (guint8 *buf)
{
  return (guint16) buf[0] << 8 | buf[1];
}

static void
set_be16 (guint8  *buf,
          guint16  val)
{
  buf[0] = val >> 8;
  buf[1] = val & 0xFF;
}

static gboolean
load_data (gchar *name)
{
  gint    i;
  FILE   *f;
  guint8  buf[288];

  f = g_fopen (name, "rb");
  if (f == NULL)
    {
      return FALSE;
    }
  if (fread (buf, 1, sizeof (buf), f) != sizeof (buf))
    {
      fclose (f);
      return FALSE;
    }
  fclose (f);

  for (i = 0; i < MAX_TRANSFORMS; i++)
    info[0].transformSequence[i] =
      get_be16 (buf + i * 2 + MAX_TRANSFORMS * 2 * 0) % NUM_TRANSFORMS;

  for (i = 0; i < MAX_TRANSFORMS; i++)
    info[0].source[i] = get_be16 (buf + i * 2 + MAX_TRANSFORMS * 2 * 1) % NUM_REGISTERS;

  for (i = 0; i < MAX_TRANSFORMS; i++)
    info[0].control[i] = get_be16 (buf + i * 2 + MAX_TRANSFORMS * 2 * 2) % NUM_REGISTERS;

  for (i = 0; i < MAX_TRANSFORMS; i++)
    info[0].dest[i] = get_be16 (buf + i * 2 + MAX_TRANSFORMS * 2 * 3) % NUM_REGISTERS;

  return TRUE;
}

static gboolean
save_data (gchar *name)
{
  gint    i = 0;
  FILE   *f;
  guint8  buf[288];

  f = g_fopen (name, "wb");
  if (f == NULL)
    {
      return FALSE;
    }

  for (i = 0; i < MAX_TRANSFORMS; i++)
    set_be16 (buf + i * 2 + MAX_TRANSFORMS * 2 * 0,
              info[0].transformSequence[i]);

  for (i = 0; i < MAX_TRANSFORMS; i++)
    set_be16 (buf + i * 2 + MAX_TRANSFORMS * 2 * 1, info[0].source[i]);

  for (i = 0; i < MAX_TRANSFORMS; i++)
    set_be16 (buf + i * 2 + MAX_TRANSFORMS * 2 * 2, info[0].control[i]);

  for (i = 0; i < MAX_TRANSFORMS; i++)
    set_be16 (buf + i * 2 + MAX_TRANSFORMS * 2 * 3, info[0].dest[i]);

  fwrite (buf, 1, sizeof (buf), f);
  fclose (f);

  return TRUE;
}

static void
dialog_undo (GtkWidget *widget,
             gpointer   data)
{
  ExpInfo temp_info[9];

  memcpy (temp_info, info, sizeof (info));
  memcpy (info, last_info, sizeof (info));
  dialog_update_previews (NULL, -1);
  memcpy (last_info, temp_info, sizeof (info));
}

static void
dialog_load (GtkWidget *widget,
             gpointer   data)
{
  GtkWidget *parent;
  GtkWidget *dialog;

  parent = gtk_widget_get_toplevel (widget);

  dialog = gtk_file_chooser_dialog_new (_("Load QBE File"),
                                        GTK_WINDOW (parent),
                                        GTK_FILE_CHOOSER_ACTION_OPEN,

                                        _("_Cancel"), GTK_RESPONSE_CANCEL,
                                        _("_Open"),   GTK_RESPONSE_OK,

                                        NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                            GTK_RESPONSE_OK,
                                            GTK_RESPONSE_CANCEL,
                                            -1);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

  if (qbist_info.path)
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), qbist_info.path);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      gchar *name = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      g_free (qbist_info.path);
      qbist_info.path = name;
      load_data (qbist_info.path);

      dialog_new_variations (NULL, NULL);
      dialog_update_previews (NULL, -1);
    }

  gtk_widget_destroy (dialog);
}

static void
dialog_save (GtkWidget *widget,
             gpointer   data)
{
  GtkWidget *parent;
  GtkWidget *dialog;

  parent = gtk_widget_get_toplevel (widget);

  dialog = gtk_file_chooser_dialog_new (_("Save as QBE File"),
                                        GTK_WINDOW (parent),
                                        GTK_FILE_CHOOSER_ACTION_SAVE,

                                        _("_Cancel"), GTK_RESPONSE_CANCEL,
                                        _("_Save"),   GTK_RESPONSE_OK,

                                        NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                            GTK_RESPONSE_OK,
                                            GTK_RESPONSE_CANCEL,
                                            -1);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog),
                                                  TRUE);

  gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), qbist_info.path);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      gchar *name = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      g_free (qbist_info.path);
      qbist_info.path = name;
      save_data (qbist_info.path);
    }

  gtk_widget_destroy (dialog);
}

static gboolean
dialog_run (GimpProcedure       *procedure,
            GimpProcedureConfig *config)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *bbox;
  GtkWidget *button;
  GtkWidget *grid;
  gint       preview_size;
  gint       i;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("G-Qbist"));

  vbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                         "qbist-vbox", "anti-aliasing",
                                         NULL);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_widget_set_margin_bottom (grid, 6);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_set_visible (grid, TRUE);
  gtk_box_reorder_child (GTK_BOX (vbox), grid, 0);

  info[0] = qbist_info.info;
  dialog_new_variations (NULL, NULL);
  memcpy (last_info, info, sizeof (info));

  preview_size = gtk_widget_get_scale_factor (grid) * PREVIEW_SIZE;
  for (i = 0; i < 9; i++)
    {
      button = gtk_button_new ();
      if (i == 4)
        {
          GtkWidget *frame;

          frame = gtk_frame_new (_("Pattern"));
          gtk_frame_set_label_align (GTK_FRAME (frame), 0.5, 0.5);
          gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_OUT);
          gtk_grid_attach (GTK_GRID (grid), frame, i % 3, i / 3, 1, 2);

          gtk_container_add (GTK_CONTAINER (frame), button);
          gtk_widget_set_visible (frame, TRUE);
        }
      else if (i > 2)
        {
          gtk_grid_attach (GTK_GRID (grid), button, i % 3, i / 3 + 1, 1, 1);
        }
      else
        {
          gtk_grid_attach (GTK_GRID (grid), button, i % 3, i / 3, 1, 1);
        }
      gtk_widget_set_valign (button, GTK_ALIGN_END);
      gtk_widget_set_visible (button, TRUE);

      g_signal_connect (button, "clicked",
                        G_CALLBACK (dialog_select_preview),
                        (gpointer) & (info[(i + 5) % 9]));

      preview[i] = gimp_preview_area_new ();
      gtk_widget_set_size_request (preview[i], preview_size, preview_size);
      gtk_container_add (GTK_CONTAINER (button), preview[i]);
      gtk_widget_set_visible (preview[i], TRUE);
    }

  bbox = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_START);
  gtk_widget_set_margin_top (bbox, 6);
  gtk_box_pack_start (GTK_BOX (vbox), bbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (bbox, TRUE);

  button = gtk_button_new_with_mnemonic (_("_Undo"));
  gtk_container_add (GTK_CONTAINER (bbox), button);
  gtk_widget_set_visible (button, TRUE);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (dialog_undo),
                    NULL);

  button = gtk_button_new_with_mnemonic (_("_Open"));
  gtk_container_add (GTK_CONTAINER (bbox), button);
  gtk_widget_set_visible (button, TRUE);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (dialog_load),
                    NULL);

  button = gtk_button_new_with_mnemonic (_("_Save"));
  gtk_container_add (GTK_CONTAINER (bbox), button);
  gtk_widget_set_visible (button, TRUE);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (dialog_save),
                    NULL);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              "qbist-vbox", NULL);
  gtk_widget_set_visible (dialog, TRUE);
  dialog_update_previews (NULL, preview_size);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  if (run)
    qbist_info.info = info[0];

  gtk_widget_destroy (dialog);

  return run;
}
