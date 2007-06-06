/* threshold_alpha.c -- This is a plug-in for GIMP (1.0's API)
 * Author: Shuji Narazaki <narazaki@InetQ.or.jp>
 * Time-stamp: <2000-01-09 13:25:30 yasuhiro>
 * Version: 0.13A (the 'A' is for Adam who hacked in greyscale
 *                 support - don't know if there's a more recent official
 *                 version)
 *
 * Copyright (C) 1997 Shuji Narazaki <narazaki@InetQ.or.jp>
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

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC    "plug-in-threshold-alpha"
#define PLUG_IN_BINARY  "threshold_alpha"

#define SCALE_WIDTH     120


static void              query (void);
static void              run   (const gchar      *name,
                                gint              nparams,
                                const GimpParam  *param,
                                gint             *nreturn_vals,
                                GimpParam       **return_vals);

static void      threshold_alpha        (GimpDrawable *drawable,
                                         GimpPreview  *preview);

static gboolean  threshold_alpha_dialog (GimpDrawable *drawable);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

typedef struct
{
  gint  threshold;
} ValueType;

static ValueType VALS =
{
  127 /* threshold */
};


MAIN ()

static void
query (void)
{
  static const GimpParamDef args [] =
  {
    { GIMP_PDB_INT32,    "run-mode",  "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",     "Input image (not used)"       },
    { GIMP_PDB_DRAWABLE, "drawable",  "Input drawable"               },
    { GIMP_PDB_INT32,    "threshold", "Threshold"                    }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Make transparency all-or-nothing"),
                          "",
                          "Shuji Narazaki (narazaki@InetQ.or.jp)",
                          "Shuji Narazaki",
                          "1997",
                          N_("_Threshold Alpha..."),
                          "RGBA,GRAYA",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC,
                             "<Image>/Layer/Transparency/Modify");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpRunMode        run_mode;
  GimpDrawable      *drawable;

  run_mode = param[0].data.d_int32;
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /* Since a channel might be selected, we must check wheter RGB or not. */
      if (gimp_layer_get_lock_alpha (drawable->drawable_id))
        {
          g_message (_("The layer has its alpha channel locked."));
          return;
        }
      if (!gimp_drawable_is_rgb (drawable->drawable_id) &&
          !gimp_drawable_is_gray (drawable->drawable_id))
        {
          g_message (_("RGBA/GRAYA drawable is not selected."));
          return;
        }
      gimp_get_data (PLUG_IN_PROC, &VALS);
      if (! threshold_alpha_dialog (drawable))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (nparams != 4)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          VALS.threshold = param[3].data.d_int32;
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_PROC, &VALS);
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (gimp_drawable_has_alpha (drawable->drawable_id))
        {
          gimp_progress_init (_("Coloring transparency"));

          threshold_alpha (drawable, NULL);

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();
          if (run_mode == GIMP_RUN_INTERACTIVE && status == GIMP_PDB_SUCCESS)
            gimp_set_data (PLUG_IN_PROC, &VALS, sizeof (ValueType));
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
      gimp_drawable_detach (drawable);
    }

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}

static void
threshold_alpha_func (const guchar *src,
                      guchar       *dest,
                      gint          bpp)
{
  for (bpp--; bpp; bpp--)
    *dest++ = *src++;
  *dest = (VALS.threshold < *src) ? 255 : 0;
}

static void
threshold_alpha (GimpDrawable *drawable,
                 GimpPreview  *preview)
{
  if (preview)
    {
      GimpPixelRgn  src_rgn;
      guchar       *src, *dst;
      gint          i;
      gint          x1, y1;
      gint          width, height;
      gint          bpp;

      gimp_preview_get_position (preview, &x1, &y1);
      gimp_preview_get_size (preview, &width, &height);

      bpp = drawable->bpp;

      src = g_new (guchar, width * height * bpp);
      dst = g_new (guchar, width * height * bpp);

      gimp_pixel_rgn_init (&src_rgn, drawable,
                           x1, y1, width, height,
                           FALSE, FALSE);
      gimp_pixel_rgn_get_rect (&src_rgn, src, x1, y1, width, height);

      for (i = 0; i < width * height; i++)
        threshold_alpha_func (src + i * bpp, dst + i * bpp, bpp);

      gimp_preview_draw_buffer (preview, dst, width * bpp);

      g_free (src);
      g_free (dst);
    }
  else
    {
      gimp_rgn_iterate2 (drawable, 0 /* unused */,
                         (GimpRgnFunc2)threshold_alpha_func, NULL);
    }
}

static gboolean
threshold_alpha_dialog (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *preview;
  GtkWidget *table;
  GtkObject *adj;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Threshold Alpha"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  preview = gimp_drawable_preview_new (drawable, NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);
  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (threshold_alpha),
                            drawable);

  table = gtk_table_new (1 ,3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              _("Threshold:"), SCALE_WIDTH, 0,
                              VALS.threshold, 0, 255, 1, 8, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &VALS.threshold);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}
