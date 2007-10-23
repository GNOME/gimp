/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * mapcolor plugin
 * Copyright (C) 1998 Peter Kirchgessner
 * email: peter@kirchgessner.net, WWW: http://www.kirchgessner.net)
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
 *
 */

/* Event history:
 * V 1.00, PK, 26-Oct-98: Creation.
 * V 1.01, PK, 21-Nov-99: Fix problem with working on layered images
 *                        Internationalization
 * V 1.02, PK, 19-Mar-00: Better explaining text/dialogue
 *                        Preview added
 *                        Fix problem with divide by zero
 * V 1.03,neo, 22-May-00: Fixed divide by zero in preview code.
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


static const gchar dversio[] = "v1.03  22-May-00";


#define COLOR_MAP_PROC    "plug-in-color-map"
#define COLOR_ADJUST_PROC "plug-in-color-adjust"
#define PLUG_IN_BINARY    "mapcolor"
#define PRV_WIDTH         50
#define PRV_HEIGHT        20

typedef struct
{
  GimpRGB  colors[4];
  gint32   map_mode;
  gboolean preview;
} PluginValues;

PluginValues plvals =
{
  {
    { 0.0, 0.0, 0.0, 1.0 },
    { 1.0, 1.0, 1.0, 1.0 },
    { 0.0, 0.0, 0.0, 1.0 },
    { 1.0, 1.0, 1.0, 1.0 },
  },
  0
};

static guchar redmap[256], greenmap[256], bluemap[256];

/* Declare some local functions.
 */
static void   query (void);
static void   run   (const gchar      *name,
                     gint              nparams,
                     const GimpParam  *param,
                     gint             *nreturn_vals,
                     GimpParam       **return_vals);

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static void      update_img_preview (GimpDrawable *drawable,
                                     GimpPreview  *preview);

static gboolean  mapcolor_dialog    (GimpDrawable *drawable);
static void      get_mapping        (GimpRGB      *src_col1,
                                     GimpRGB      *src_col2,
                                     GimpRGB      *dst_col1,
                                     GimpRGB      *dst_col2,
                                     gint32        map_mode,
                                     guchar       *redmap,
                                     guchar       *greenmap,
                                     guchar       *bluemap);
static void      add_color_button   (gint          csel_index,
                                     gint          left,
                                     gint          top,
                                     GtkWidget    *table,
                                     GtkWidget    *preview);

static void      color_mapping      (GimpDrawable *drawable);

static gchar *csel_title[4] =
{
  N_("First Source Color"),
  N_("Second Source Color"),
  N_("First Destination Color"),
  N_("Second Destination Color")
};

MAIN ()

static void
query (void)

{
  static const GimpParamDef adjust_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",    "Input image (not used)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable to adjust" }
  };

  static const GimpParamDef map_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",   "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",      "Input image (not used)" },
    { GIMP_PDB_DRAWABLE, "drawable",   "Input drawable where colors are to map" },
    { GIMP_PDB_COLOR,    "srccolor-1", "First source color" },
    { GIMP_PDB_COLOR,    "srccolor-2", "Second source color" },
    { GIMP_PDB_COLOR,    "dstcolor-1", "First destination color" },
    { GIMP_PDB_COLOR,    "dstcolor-2", "Second destination color" },
    { GIMP_PDB_INT32,    "map-mode",   "Mapping mode (0: linear, others reserved)" }
  };

  gimp_install_procedure (COLOR_ADJUST_PROC,
                          N_("Map colors sending foreground to black, background to white"),
                          "The current foreground color is mapped to black "
                          "(black point), the current background color is "
                          "mapped to white (white point). Intermediate "
                          "colors are interpolated",
                          "Peter Kirchgessner",
                          "Peter Kirchgessner",
                          dversio,
                          N_("Adjust _Foreground & Background"),
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (adjust_args), 0,
                          adjust_args, NULL);

  gimp_plugin_menu_register (COLOR_ADJUST_PROC, "<Image>/Colors/Map");

  gimp_install_procedure (COLOR_MAP_PROC,
                          N_("Map color range specified by two colors "
                             "to another range"),
                          "Map color range specified by two colors"
                          "to color range specified by two other color. "
                          "Intermediate colors are interpolated.",
                          "Peter Kirchgessner",
                          "Peter Kirchgessner",
                          dversio,
                          N_("Color Range _Mapping..."),
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (map_args), 0,
                          map_args, NULL);

  gimp_plugin_menu_register (COLOR_MAP_PROC, "<Image>/Colors/Map");
}


static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)

{
  static GimpParam   values[1];
  GimpRunMode        run_mode;
  GimpDrawable      *drawable = NULL;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  gint               j;

  INIT_I18N ();

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  while (status == GIMP_PDB_SUCCESS)
    {
      if (nparams < 3)
        {
          status = GIMP_PDB_CALLING_ERROR;
          break;
        }

      /* Make sure the drawable is RGB color */
      drawable = gimp_drawable_get (param[2].data.d_drawable);
      if (!gimp_drawable_is_rgb (drawable->drawable_id))
        {
          g_message (_("Cannot operate on gray or indexed color images."));
          status = GIMP_PDB_EXECUTION_ERROR;
          break;
        }

      if (strcmp (name, COLOR_ADJUST_PROC) == 0)
        {
          if (nparams != 3)  /* Make sure all the arguments are there */
            {
              status = GIMP_PDB_CALLING_ERROR;
              break;
            }

          gimp_context_get_foreground (plvals.colors);
          gimp_context_get_background (plvals.colors + 1);

          gimp_rgb_set (plvals.colors + 2, 0.0, 0.0, 0.0);
          gimp_rgb_set (plvals.colors + 3, 1.0, 1.0, 1.0);

          plvals.map_mode = 0;

          gimp_progress_init (_("Adjusting foreground and background colors"));

          color_mapping (drawable);
          break;
        }

      if (strcmp (name, COLOR_MAP_PROC) == 0)
        {
          if (run_mode == GIMP_RUN_NONINTERACTIVE)
            {
              if (nparams != 8)  /* Make sure all the arguments are there */
                {
                  status = GIMP_PDB_CALLING_ERROR;
                  break;
                }

              for (j = 0; j < 4; j++)
                {
                  plvals.colors[j] = param[3+j].data.d_color;
                }
              plvals.map_mode = param[7].data.d_int32;
            }
          else if (run_mode == GIMP_RUN_INTERACTIVE)
            {
              gimp_get_data (name, &plvals);

              gimp_context_get_foreground (plvals.colors);
              gimp_context_get_background (plvals.colors + 1);

              if (!mapcolor_dialog (drawable))
                break;
            }
          else if (run_mode == GIMP_RUN_WITH_LAST_VALS)
            {
              gimp_get_data (name, &plvals);
            }
          else
            {
              status = GIMP_PDB_CALLING_ERROR;
              break;
            }

          gimp_progress_init (_("Mapping colors"));

          color_mapping (drawable);

          if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data (name, &plvals, sizeof (plvals));

          break;
        }

      status = GIMP_PDB_EXECUTION_ERROR;
    }

  if ((status == GIMP_PDB_SUCCESS) && (run_mode != GIMP_RUN_NONINTERACTIVE))
    gimp_displays_flush ();

  if (drawable != NULL)
    gimp_drawable_detach (drawable);

  values[0].data.d_status = status;
}

static void
update_img_preview (GimpDrawable *drawable,
                    GimpPreview  *preview)
{
  guchar    redmap[256], greenmap[256], bluemap[256];
  gint      width, height, bpp;
  gboolean  has_alpha;
  guchar   *src, *dest, *s, *d;
  gint      j;

  get_mapping (plvals.colors,
               plvals.colors + 1,
               plvals.colors + 2,
               plvals.colors + 3,
               plvals.map_mode,
               redmap, greenmap, bluemap);

  src = gimp_zoom_preview_get_source (GIMP_ZOOM_PREVIEW (preview),
                                      &width, &height, &bpp);
  has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);

  j = width * height;
  dest = g_new (guchar, j * bpp);

  s = src; d = dest;
  while (j-- > 0)
    {
      *(d++) = redmap[*(s++)];
      *(d++) = greenmap[*(s++)];
      *(d++) = bluemap[*(s++)];
      if (has_alpha)
        *(d++) = *(s++);
    }

  gimp_preview_draw_buffer (preview, dest, width * bpp);

  g_free (src);
  g_free (dest);
}

static gboolean
mapcolor_dialog (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *preview;
  GtkWidget *frame;
  GtkWidget *table;
  gint       j;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("Map Color Range"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, COLOR_MAP_PROC,

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

  preview = gimp_zoom_preview_new (drawable);
  gtk_box_pack_start_defaults (GTK_BOX (main_vbox), preview);
  gtk_widget_show (preview);
  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (update_img_preview),
                            drawable);

  for (j = 0; j < 2; j++)
    {
      frame = gimp_frame_new ((j == 0) ?
                              _("Source Color Range") :
                              _("Destination Color Range"));
      gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      /* The table keeps the color selections */
      table = gtk_table_new (1, 4, FALSE);
      gtk_table_set_col_spacings (GTK_TABLE (table), 6);
      gtk_container_add (GTK_CONTAINER (frame), table);
      gtk_widget_show (table);

      add_color_button (j * 2, 0, 0, table, preview);
      add_color_button (j * 2 + 1, 2, 0, table, preview);
    }

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  if (run)
    plvals.map_mode = 0;  /* Currently always linear mapping */

  gtk_widget_destroy (dialog);

  return run;
}

static void
add_color_button (gint       csel_index,
                  gint       left,
                  gint       top,
                  GtkWidget *table,
                  GtkWidget *preview)
{
  GtkWidget *button;

  button = gimp_color_button_new (gettext (csel_title[csel_index]),
                                  PRV_WIDTH, PRV_HEIGHT,
                                  &plvals.colors[csel_index],
                                  GIMP_COLOR_AREA_FLAT);
  gimp_table_attach_aligned (GTK_TABLE (table), left + 1, top,
                             (left == 0) ? _("From:") : _("To:"),
                             0.0, 0.5,
                             button, 1, TRUE);

  g_signal_connect (button, "color-changed",
                    G_CALLBACK (gimp_color_button_get_color),
                    &plvals.colors[csel_index]);
  g_signal_connect_swapped (button, "color-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
}

static void
get_mapping (GimpRGB *src_col1,
             GimpRGB *src_col2,
             GimpRGB *dst_col1,
             GimpRGB *dst_col2,
             gint32   map_mode,
             guchar  *redmap,
             guchar  *greenmap,
             guchar  *bluemap)
{
  guchar  src1[3];
  guchar  src2[3];
  guchar  dst1[3];
  guchar  dst2[3];
  gint    rgb, i, a, as, b, bs;
  guchar *colormap[3];

  /* Currently we always do a linear mapping */

  colormap[0] = redmap;
  colormap[1] = greenmap;
  colormap[2] = bluemap;

  gimp_rgb_get_uchar (src_col1, src1, src1 + 1, src1 + 2);
  gimp_rgb_get_uchar (src_col2, src2, src2 + 1, src2 + 2);
  gimp_rgb_get_uchar (dst_col1, dst1, dst1 + 1, dst1 + 2);
  gimp_rgb_get_uchar (dst_col2, dst2, dst2 + 1, dst2 + 2);

  switch (map_mode)
    {
    case 0:
    default:
      for (rgb = 0; rgb < 3; rgb++)
        {
          a = src1[rgb];  as = dst1[rgb];
          b = src2[rgb];  bs = dst2[rgb];

          if (b == a)
            {
              /*  For this channel, both source color values are the same.  */

              /*  Map everything below to the start of the destination range. */
              for (i = 0; i < a; i++)
                {
                  colormap[rgb][i] = as;
                }

              /*  Map source color to the middle of the destination range. */
              colormap[rgb][a] = (as + bs) / 2;

              /*  Map everything above to the end of the destination range. */
              for (i = a + 1; i < 256; i++)
                {
                  colormap[rgb][i] = bs;
                }
            }
          else
            {
              /*  Map color ranges.  */
              for (i = 0; i < 256; i++)
                {
                  gint j = ((i - a) * (bs - as)) / (b - a) + as;

                  colormap[rgb][i] = CLAMP0255(j);
                }
            }
        }
      break;
    }
}

static void
mapcolor_func (const guchar *src,
               guchar       *dest,
               gint          bpp,
               gpointer      data)
{
  dest[0] = redmap[src[0]];
  dest[1] = greenmap[src[1]];
  dest[2] = bluemap[src[2]];
  if (bpp > 3)
    dest[3] = src[3];
}

static void
color_mapping (GimpDrawable *drawable)

{
  if (!gimp_drawable_is_rgb (drawable->drawable_id))
    {
      g_message (_("Cannot operate on gray or indexed color images."));
      return;
    }

  gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));

  get_mapping (plvals.colors,
               plvals.colors + 1,
               plvals.colors + 2,
               plvals.colors + 3,
               plvals.map_mode,
               redmap, greenmap, bluemap);

  gimp_rgn_iterate2 (drawable, 0 /* unused */, mapcolor_func, NULL);
}
