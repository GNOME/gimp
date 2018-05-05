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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "config.h"

#include <string.h>
#include <limits.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/** qbist renderer ***********************************************************/

#define MAX_TRANSFORMS  36
#define NUM_REGISTERS    6
#define PREVIEW_SIZE    64

#define PLUG_IN_PROC    "plug-in-qbist"
#define PLUG_IN_BINARY  "qbist"
#define PLUG_IN_ROLE    "gimp-qbist"
#define PLUG_IN_VERSION "January 2001, 1.12"

/** types *******************************************************************/

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
  gchar    path[PATH_MAX];
} QbistInfo;


/** prototypes **************************************************************/

static void      query                  (void);
static void      run                    (const gchar      *name,
                                         gint              nparams,
                                         const GimpParam  *param,
                                         gint             *nreturn_vals,
                                         GimpParam       **return_vals);

static gboolean  dialog_run             (void);
static void      dialog_new_variations  (GtkWidget        *widget,
                                         gpointer          data);
static void      dialog_update_previews (GtkWidget        *widget,
                                         gpointer          data);
static void      dialog_select_preview  (GtkWidget        *widget,
                                         ExpInfo          *n_info);

static QbistInfo  qbist_info;
static GRand     *gr = NULL;


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

/** Plugin interface *********************************************************/

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,                         /* init_proc  */
  NULL,                         /* quit_proc  */
  query,                        /* query_proc */
  run                           /* run_proc   */
};

MAIN ()

static void
query (void)
{
  GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",    "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable"       }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Generate a huge variety of abstract patterns"),
                          "This Plug-in is based on an article by "
                          "JÃ¶rn Loviscach (appeared in c't 10/95, page 326). "
                          "It generates modern art pictures from a random "
                          "genetic formula.",
                          "JÃ¶rn Loviscach, Jens Ch. Restemeier",
                          "JÃ¶rn Loviscach, Jens Ch. Restemeier",
                          PLUG_IN_VERSION,
                          N_("_Qbist..."),
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Render/Pattern");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  gint               sel_x1, sel_y1, sel_width, sel_height;
  gint               img_height, img_width;
  GimpRunMode        run_mode;
  gint32             drawable_id;
  GimpPDBStatusType  status;

  *nreturn_vals = 1;
  *return_vals  = values;

  status = GIMP_PDB_SUCCESS;

  if (param[0].type != GIMP_PDB_INT32)
    status = GIMP_PDB_CALLING_ERROR;
  run_mode = param[0].data.d_int32;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  if (param[2].type != GIMP_PDB_DRAWABLE)
    status = GIMP_PDB_CALLING_ERROR;

  drawable_id = param[2].data.d_drawable;

  img_width = gimp_drawable_width (drawable_id);
  img_height = gimp_drawable_height (drawable_id);

  if (! gimp_drawable_is_rgb (drawable_id))
    status = GIMP_PDB_CALLING_ERROR;

  if (! gimp_drawable_mask_intersect (drawable_id,
                                      &sel_x1, &sel_y1,
                                      &sel_width, &sel_height))
    {
      values[0].type          = GIMP_PDB_STATUS;
      values[0].data.d_status = status;

      return;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      gr = g_rand_new ();

      memset (&qbist_info, 0, sizeof (qbist_info));
      create_info (&qbist_info.info);
      qbist_info.oversampling = 4;

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          /* Possibly retrieve data */
          gimp_get_data (PLUG_IN_PROC, &qbist_info);

          /* Get information from the dialog */
          if (dialog_run ())
            {
              status = GIMP_PDB_SUCCESS;
              gimp_set_data (PLUG_IN_PROC, &qbist_info, sizeof (QbistInfo));
            }
          else
            status = GIMP_PDB_EXECUTION_ERROR;
          break;

        case GIMP_RUN_NONINTERACTIVE:
          status = GIMP_PDB_CALLING_ERROR;
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          /* Possibly retrieve data */
          gimp_get_data (PLUG_IN_PROC, &qbist_info);
          status = GIMP_PDB_SUCCESS;
          break;

        default:
          status = GIMP_PDB_CALLING_ERROR;
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          GeglBuffer         *buffer;
          GeglBufferIterator *iter;
          gint                total_pixels = img_width * img_height;
          gint                done_pixels  = 0;

          buffer = gimp_drawable_get_shadow_buffer (drawable_id);

          iter = gegl_buffer_iterator_new (buffer,
                                           GEGL_RECTANGLE (0, 0,
                                                           img_width, img_height),
                                           0, babl_format ("R'G'B'A float"),
                                           GEGL_ACCESS_READWRITE,
                                           GEGL_ABYSS_NONE);

          optimize (&qbist_info.info);

          gimp_progress_init (_("Qbist"));

          while (gegl_buffer_iterator_next (iter))
            {
              gfloat        *data = iter->data[0];
              GeglRectangle  roi  = iter->roi[0];
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

          gimp_drawable_merge_shadow (drawable_id, TRUE);
          gimp_drawable_update (drawable_id, sel_x1, sel_y1,
                                sel_width, sel_height);

          gimp_displays_flush ();
        }

      g_rand_free (gr);
    }

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
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
                        gpointer   data)
{
  static const Babl *fish = NULL;

  gfloat buf[PREVIEW_SIZE * PREVIEW_SIZE * 4];
  guchar u8_buf[PREVIEW_SIZE * PREVIEW_SIZE * 4];
  gint   i, j;

  if (! fish)
    fish = babl_fish (babl_format ("R'G'B'A float"),
                      babl_format ("R'G'B'A u8"));

  for (j = 0; j < 9; j++)
    {
      optimize (&info[(j + 5) % 9]);

      for (i = 0; i < PREVIEW_SIZE; i++)
        {
          qbist (&info[(j + 5) % 9], buf + i * PREVIEW_SIZE * 4,
                 0, i, PREVIEW_SIZE, PREVIEW_SIZE, PREVIEW_SIZE, 1);
        }

      babl_process (fish, buf, u8_buf, PREVIEW_SIZE * PREVIEW_SIZE);

      gimp_preview_area_draw (GIMP_PREVIEW_AREA (preview[j]),
                              0, 0, PREVIEW_SIZE, PREVIEW_SIZE,
                              GIMP_RGBA_IMAGE,
                              u8_buf,
                              PREVIEW_SIZE * 4);
    }
}

static void
dialog_select_preview (GtkWidget *widget,
                       ExpInfo   *n_info)
{
  memcpy (last_info, info, sizeof (info));
  info[0] = *n_info;
  dialog_new_variations (widget, NULL);
  dialog_update_previews (widget, NULL);
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
      get_be16 (buf + i * 2 + MAX_TRANSFORMS * 2 * 0);

  for (i = 0; i < MAX_TRANSFORMS; i++)
    info[0].source[i] = get_be16 (buf + i * 2 + MAX_TRANSFORMS * 2 * 1);

  for (i = 0; i < MAX_TRANSFORMS; i++)
    info[0].control[i] = get_be16 (buf + i * 2 + MAX_TRANSFORMS * 2 * 2);

  for (i = 0; i < MAX_TRANSFORMS; i++)
    info[0].dest[i] = get_be16 (buf + i * 2 + MAX_TRANSFORMS * 2 * 3);

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
  dialog_update_previews (NULL, NULL);
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

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

  gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), qbist_info.path);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      gchar *name = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      strncpy (qbist_info.path, name, PATH_MAX - 1);
      load_data (qbist_info.path);

      g_free (name);

      dialog_new_variations (NULL, NULL);
      dialog_update_previews (NULL, NULL);
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

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
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

      strncpy (qbist_info.path, name, PATH_MAX - 1);
      save_data (qbist_info.path);

      g_free (name);
    }

  gtk_widget_destroy (dialog);
}

static void
dialog_toggle_antialaising (GtkWidget *widget,
                            gpointer   data)
{
  qbist_info.oversampling =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)) ? 4 : 1;
}

static gboolean
dialog_run (void)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *bbox;
  GtkWidget *button;
  GtkWidget *grid;
  gint       i;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("G-Qbist"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  info[0] = qbist_info.info;
  dialog_new_variations (NULL, NULL);
  memcpy (last_info, info, sizeof (info));

  for (i = 0; i < 9; i++)
    {
      button = gtk_button_new ();
      gtk_grid_attach (GTK_GRID (grid), button, i % 3, i / 3, 1, 1);
                        // GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (button);

      g_signal_connect (button, "clicked",
                        G_CALLBACK (dialog_select_preview),
                        (gpointer) & (info[(i + 5) % 9]));

      preview[i] = gimp_preview_area_new ();
      gtk_widget_set_size_request (preview[i], PREVIEW_SIZE, PREVIEW_SIZE);
      gtk_container_add (GTK_CONTAINER (button), preview[i]);
      gtk_widget_show (preview[i]);
    }

  button = gtk_check_button_new_with_mnemonic (_("_Antialiasing"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                qbist_info.oversampling > 1);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (dialog_toggle_antialaising),
                    NULL);

  bbox = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_START);
  gtk_box_pack_start (GTK_BOX (vbox), bbox, FALSE, FALSE, 0);
  gtk_widget_show (bbox);

  button = gtk_button_new_with_mnemonic (_("_Undo"));
  gtk_container_add (GTK_CONTAINER (bbox), button);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (dialog_undo),
                    NULL);

  button = gtk_button_new_with_mnemonic (_("_Open"));
  gtk_container_add (GTK_CONTAINER (bbox), button);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (dialog_load),
                    NULL);

  button = gtk_button_new_with_mnemonic (_("_Save"));
  gtk_container_add (GTK_CONTAINER (bbox), button);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (dialog_save),
                    NULL);

  gtk_widget_show (dialog);
  dialog_update_previews (NULL, NULL);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  if (run)
    qbist_info.info = info[0];

  gtk_widget_destroy (dialog);

  return run;
}
