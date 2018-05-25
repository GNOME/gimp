/* max_rgb.c -- This is a plug-in for GIMP
 * Author: Shuji Narazaki <narazaki@InetQ.or.jp>
 * Time-stamp: <2000-02-08 16:26:24 yasuhiro>
 * Version: 0.35
 *
 * Copyright (C) 1997 Shuji Narazaki <narazaki@InetQ.or.jp>
 *
 * May 2000 - tim copperfield [timecop@japan.co.jp]
 *
 * Added a preview mode.  After adding preview mode realised just exactly
 * how useless this plugin is :)
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

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-max-rgb"
#define PLUG_IN_BINARY "max-rgb"
#define PLUG_IN_ROLE   "gimp-max-rgb"


static void     query   (void);
static void     run     (const gchar      *name,
                         gint              nparams,
                         const GimpParam  *param,
                         gint             *nreturn_vals,
                         GimpParam       **return_vals);

static GimpPDBStatusType main_function  (GimpDrawable *drawable,
                                         GimpPreview  *preview);

static gint              max_rgb_dialog (GimpDrawable *drawable);

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

enum
{
  MIN_CHANNELS = 0,
  MAX_CHANNELS = 1
};

typedef struct
{
  gint max_p;
} ValueType;

static ValueType pvals =
{
  MAX_CHANNELS
};

MAIN ()

static void
query (void)
{
  static const GimpParamDef args [] =
  {
    { GIMP_PDB_INT32,    "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"       },
    { GIMP_PDB_IMAGE,    "image",    "Input image (not used)"         },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable"                 },
    { GIMP_PDB_INT32,    "max-p",    "{ MINIMIZE (0), MAXIMIZE (1) }" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Reduce image to pure red, green, and blue"),
                          "There's no help yet.",
                          "Shuji Narazaki (narazaki@InetQ.or.jp)",
                          "Shuji Narazaki",
                          "May 2000",
                          N_("Maxim_um RGB..."),
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  GimpDrawable      *drawable;
  static GimpParam   values[1];
  GimpPDBStatusType  status = GIMP_PDB_EXECUTION_ERROR;
  GimpRunMode        run_mode;

  run_mode = param[0].data.d_int32;
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_PROC, &pvals);
      /* Since a channel might be selected, we must check whether RGB or not. */
      if (!gimp_drawable_is_rgb (drawable->drawable_id))
        {
          g_message (_("Can only operate on RGB drawables."));
          return;
        }
      if (! max_rgb_dialog (drawable))
        return;
      break;
    case GIMP_RUN_NONINTERACTIVE:
      /* You must copy the values of parameters to pvals or dialog variables. */
      pvals.max_p = param[3].data.d_int32;
      break;
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_PROC, &pvals);
      break;
    }

  status = main_function (drawable, NULL);

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush ();
  if (run_mode == GIMP_RUN_INTERACTIVE && status == GIMP_PDB_SUCCESS)
    gimp_set_data (PLUG_IN_PROC, &pvals, sizeof (ValueType));

  values[0].data.d_status = status;
}

typedef struct
{
  gint     init_value;
  gint     flag;
  gboolean has_alpha;
} MaxRgbParam_t;

static void
max_rgb_func (const guchar *src,
              guchar       *dest,
              gint          bpp,
              gpointer      data)
{
  MaxRgbParam_t *param = (MaxRgbParam_t*) data;
  gint   ch, max_ch = 0;
  guchar max, tmp_value;

  max = param->init_value;
  for (ch = 0; ch < 3; ch++)
    if (param->flag * max <= param->flag * (tmp_value = (*src++)))
      {
        if (max == tmp_value)
          {
            max_ch += 1 << ch;
          }
        else
          {
            max_ch = 1 << ch; /* clear memories of old channels */
            max = tmp_value;
          }
      }

  dest[0] = (max_ch & (1 << 0)) ? max : 0;
  dest[1] = (max_ch & (1 << 1)) ? max : 0;
  dest[2] = (max_ch & (1 << 2)) ? max : 0;
  if (param->has_alpha)
    dest[3] = *src;
}

static GimpPDBStatusType
main_function (GimpDrawable *drawable,
               GimpPreview  *preview)
{
  MaxRgbParam_t param;

  param.init_value = (pvals.max_p > 0) ? 0 : 255;
  param.flag = (0 < pvals.max_p) ? 1 : -1;
  param.has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);

  if (preview)
    {
      gint    i;
      guchar *buffer;
      guchar *src;
      gint    width, height, bpp;

      src = gimp_zoom_preview_get_source (GIMP_ZOOM_PREVIEW (preview),
                                          &width, &height, &bpp);

      buffer = g_new (guchar, width * height * bpp);

      for (i = 0; i < width * height; i++)
        {
          max_rgb_func (src    + i * bpp,
                        buffer + i * bpp,
                        bpp,
                        &param);
        }

      gimp_preview_draw_buffer (preview, buffer, width * bpp);
      g_free (buffer);
      g_free (src);
    }
  else
    {
      gimp_progress_init (_("Max RGB"));

      gimp_rgn_iterate2 (drawable, 0 /* unused */, max_rgb_func, &param);

      gimp_drawable_detach (drawable);
    }

  return GIMP_PDB_SUCCESS;
}


/* dialog stuff */
static gint
max_rgb_dialog (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *preview;
  GtkWidget *frame;
  GtkWidget *max;
  GtkWidget *min;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("Maximum RGB Value"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  preview = gimp_zoom_preview_new_from_drawable_id (drawable->drawable_id);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);

  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (main_function),
                            drawable);

  frame = gimp_int_radio_group_new (FALSE, NULL,
                                    G_CALLBACK (gimp_radio_button_update),
                                    &pvals.max_p, pvals.max_p,

                                    _("_Hold the maximal channels"),
                                    MAX_CHANNELS, &max,

                                    _("Ho_ld the minimal channels"),
                                    MIN_CHANNELS, &min,

                                    NULL);

  g_signal_connect_swapped (max, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  g_signal_connect_swapped (min, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}
