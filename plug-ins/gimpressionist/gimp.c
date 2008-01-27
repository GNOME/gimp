#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>

#include "ppmtool.h"
#include "infile.h"
#include "gimpressionist.h"
#include "preview.h"
#include "brush.h"
#include "presets.h"
#include "random.h"
#include "orientmap.h"
#include "size.h"


#include "libgimp/stdplugins-intl.h"

static void query               (void);
static void run                 (const gchar      *name,
                                 gint              nparams,
                                 const GimpParam  *param,
                                 gint             *nreturn_vals,
                                 GimpParam       **return_vals);
static void gimpressionist_main (void);


const GimpPlugInInfo PLUG_IN_INFO = {
        NULL,   /* init_proc */
        NULL,   /* quit_proc */
        query,  /* query_proc */
        run     /* run_proc */
}; /* PLUG_IN_INFO */

static GimpDrawable *drawable;
static ppm_t         infile =  {0, 0, NULL};
static ppm_t         inalpha = {0, 0, NULL};


void
infile_copy_to_ppm (ppm_t * p)
{
  if (!PPM_IS_INITED (&infile))
    grabarea ();
#if 0
    updatepreview (NULL, (void *)2); /* Force grabarea () */
#endif
  ppm_copy (&infile, p);
}

void
infile_copy_alpha_to_ppm (ppm_t * p)
{
  ppm_copy (&inalpha, p);
}

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",  "Interactive"    },
    { GIMP_PDB_IMAGE,    "image",     "Input image"    },
    { GIMP_PDB_DRAWABLE, "drawable",  "Input drawable" },
    { GIMP_PDB_STRING,   "preset",    "Preset Name"    },
  };

  gimp_install_procedure (PLUG_IN_NAME,
                          N_("Performs various artistic operations"),
                          "Performs various artistic operations on an image",
                          "Vidar Madsen <vidar@prosalg.no>",
                          "Vidar Madsen",
                          PLUG_IN_VERSION,
                          N_("_GIMPressionist..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_NAME, "<Image>/Filters/Artistic");
}

static void
gimpressionist_get_data (void)
{
  restore_default_values ();
  gimp_get_data (PLUG_IN_NAME, &pcvals);
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
  GimpPDBStatusType  status;
  gboolean           with_specified_preset;
  gchar             *preset_name = NULL;

  status   = GIMP_PDB_SUCCESS;
  run_mode = param[0].data.d_int32;
  with_specified_preset = FALSE;

  if (nparams > 3)
    {
      preset_name = param[3].data.d_string;
      if (strcmp (preset_name, ""))
        {
          with_specified_preset = TRUE;
        }
    }

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals  = values;

  INIT_I18N ();

  /* Get the active drawable info */

  drawable = gimp_drawable_get (param[2].data.d_drawable);
  img_has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);

  random_generator = g_rand_new ();

  switch (run_mode)
    {
        /*
         * Note: there's a limitation here. Running this plug-in before the
         * interactive plug-in was run will cause it to crash, because the
         * data is uninitialized.
         * */
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_NONINTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      gimpressionist_get_data ();
      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          if (!create_gimpressionist ())
            return;
        }
      break;
    default:
      status = GIMP_PDB_EXECUTION_ERROR;
      break;
    }
  if ((status == GIMP_PDB_SUCCESS) &&
      (gimp_drawable_is_rgb (drawable->drawable_id) ||
       gimp_drawable_is_gray (drawable->drawable_id)))
    {

      if (with_specified_preset)
        {
          /* If select_preset fails - set to an error */
          if (select_preset (preset_name))
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
        }
      /* It seems that the value of the run variable is stored in
       * the preset. I don't know if it's a bug or a feature, but
       * I just work here and am anxious to get a working version.
       * So I'm setting it to the correct value here.
       *
       * It also seems that defaultpcvals have this erroneous
       * value as well, so it gets set to FALSE as well. Thus it
       * is always set to TRUE upon a non-interactive run.
       *        -- Shlomi Fish
       * */
      if (run_mode == GIMP_RUN_NONINTERACTIVE)
        {
          pcvals.run = TRUE;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          gimpressionist_main ();
          gimp_displays_flush ();

          if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data (PLUG_IN_NAME,
                           &pcvals,
                           sizeof (gimpressionist_vals_t));
        }
    }
  else if (status == GIMP_PDB_SUCCESS)
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  /* Resources Cleanup */
  g_rand_free (random_generator);
  free_parsepath_cache ();
  brush_reload (NULL, NULL);
  preview_free_resources ();
  brush_free ();
  preset_free ();
  orientation_map_free_resources ();
  size_map_free_resources ();

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

void
grabarea (void)
{
  GimpPixelRgn  src_rgn;
  guchar       *src_row;
  guchar       *src;
  gint          alpha, bpp;
  gboolean      has_alpha;
  gint          x, y;
  ppm_t        *p;
  gint          x1, y1, x2, y2;
  gint          row, col;
  int           rowstride;

  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);

  gimp_tile_cache_ntiles ((x2 - x1) / gimp_tile_width () + 1);

  bpp = gimp_drawable_bpp (drawable->drawable_id);
  has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);
  alpha = (has_alpha) ? bpp - 1 : bpp;

  ppm_new (&infile, x2-x1, y2-y1);
  p = &infile;
  if (has_alpha)
    {
      ppm_new (&inalpha, x2-x1, y2-y1);
    }

  rowstride = p->width * 3;

  src_row = g_new (guchar, (x2 - x1) * bpp);

  gimp_pixel_rgn_init (&src_rgn, drawable,
                       0, 0, x2 - x1, y2 - y1,
                       FALSE, FALSE);

  if (bpp == 3)
    { /* RGB */
      int bpr = (x2 - x1) * 3;

      for (row = 0, y = y1; y < y2; row++, y++)
        {
          gimp_pixel_rgn_get_row (&src_rgn, src_row, x1, y, (x2 - x1));
          memcpy (p->col + row * rowstride, src_row, bpr);
        }
    }
  else if (bpp > 3)
    { /* RGBA */
      for (row = 0, y = y1; y < y2; row++, y++)
        {
          guchar *tmprow  = p->col + row * rowstride;
          guchar *tmparow = inalpha.col + row * rowstride;

          gimp_pixel_rgn_get_row (&src_rgn, src_row, x1, y, (x2 - x1));
          src = src_row;

          for (col = 0, x = x1; x < x2; col++, x++)
            {
              int k = col * 3;

              tmprow[k+0] = src[0];
              tmprow[k+1] = src[1];
              tmprow[k+2] = src[2];
              tmparow[k] = 255 - src[3];
              src += src_rgn.bpp;
            }
        }
    }
  else if (bpp == 2)
    {
      /* GrayA */
      for (row = 0, y = y1; y < y2; row++, y++)
        {
          guchar *tmprow  = p->col + row * rowstride;
          guchar *tmparow = inalpha.col + row * rowstride;

          gimp_pixel_rgn_get_row (&src_rgn, src_row, x1, y, (x2 - x1));
          src = src_row;

          for (col = 0, x = x1; x < x2; col++, x++)
            {
              int k = col * 3;

              tmprow[k+0] = src[0];
              tmprow[k+1] = src[0];
              tmprow[k+2] = src[0];
              tmparow[k] = 255 - src[1];
              src += src_rgn.bpp;
            }
        }
    }
  else
    { /* Gray */
      for (row = 0, y = y1; y < y2; row++, y++)
        {
          guchar *tmprow = p->col + row * rowstride;

          gimp_pixel_rgn_get_row (&src_rgn, src_row, x1, y, (x2 - x1));
          src = src_row;
          for (col = 0, x = x1; x < x2; col++, x++)
            {
              int k = col * 3;

              tmprow[k+0] = src[0];
              tmprow[k+1] = src[0];
              tmprow[k+2] = src[0];
              src += src_rgn.bpp;
            }
        }
    }
  g_free (src_row);
}

static void
gimpressionist_main (void)
{
  GimpPixelRgn  dest_rgn;
  guchar       *dest_row;
  guchar       *dest;
  gint          alpha, bpp;
  gboolean      has_alpha;
  gint          x, y;
  ppm_t        *p;
  gint          x1, y1, x2, y2;
  gint          row, col;
  int           rowstride;

  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);

  gimp_tile_cache_ntiles ((x2 - x1) / gimp_tile_width () + 1);

  bpp = gimp_drawable_bpp (drawable->drawable_id);
  has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);
  alpha = (has_alpha) ? bpp - 1 : bpp;

  dest_row = g_new (guchar, (x2 - x1) * bpp);

  gimp_progress_init (_("Painting"));

  if (!PPM_IS_INITED (&infile))
    {
      grabarea ();
    }

  repaint (&infile, (img_has_alpha) ? &inalpha : NULL);

  gimp_pixel_rgn_init (&dest_rgn, drawable,
                       0, 0, x2 - x1, y2 - y1,
                       TRUE, TRUE);

  p = &infile;

  rowstride = p->width * 3;

  if (bpp == 3)
    {
      int bpr = (x2 - x1) * 3;

      for (row = 0, y = y1; y < y2; row++, y++)
        {
          if (row % 10 == 0)
            gimp_progress_update (0.8 + 0.2 * ((double)row / (y2 - y1)));
          memcpy (dest_row, p->col + row * rowstride, bpr);
          gimp_pixel_rgn_set_row (&dest_rgn, dest_row, x1, y, (x2 - x1));
        }
    }
  else if (bpp == 4)
    {

      for (row = 0, y = y1; y < y2; row++, y++)
        {
          guchar *tmprow = p->col + row * rowstride;

          if (row % 10 == 0)
            gimp_progress_update (0.8 + 0.2 * ((double)row / (y2-y1)));
          dest = dest_row;

          for (col = 0, x = x1; x < x2; col++, x++)
            {
              int k = col * 3;

              dest[0] = tmprow[k+0];
              dest[1] = tmprow[k+1];
              dest[2] = tmprow[k+2];
              dest[3] = 255 - inalpha.col[row * rowstride + k];
              dest += dest_rgn.bpp;
            }
          gimp_pixel_rgn_set_row (&dest_rgn, dest_row, x1, y, (x2 - x1));
        }
    }
  else if (bpp == 2)
    {
      for (row = 0, y = y1; y < y2; row++, y++)
        {
          guchar *tmprow = p->col + row * rowstride;

          if (row % 10 == 0)
            gimp_progress_update (0.8 + 0.2 * ((double)row / (y2-y1)));
          dest = dest_row;

          for (col = 0, x = x1; x < x2; col++, x++)
            {
              int k = col * 3;

              dest[0] = (tmprow[k+0] + tmprow[k+1] + tmprow[k+2]) / 3;
              dest[1] = 255 - inalpha.col[row * rowstride + k];
              dest += dest_rgn.bpp;
            }
          gimp_pixel_rgn_set_row (&dest_rgn, dest_row, x1, y, (x2 - x1));
        }
    }
  else
    {
      for (row = 0, y = y1; y < y2; row++, y++)
        {
          guchar *tmprow = p->col + row * rowstride;

          if (row % 10 == 0)
            gimp_progress_update (0.8 + 0.2 * ((double)row / (y2-y1)));
          dest = dest_row;

          for (col = 0, x = x1; x < x2; col++, x++)
            {
              int k = col * 3;
              dest[0] = (tmprow[k+0] + tmprow[k+1] + tmprow[k+2]) / 3;
              dest += dest_rgn.bpp;
            }
          gimp_pixel_rgn_set_row (&dest_rgn, dest_row, x1, y, (x2 - x1));
        }
    }

  g_free (dest_row);

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id, x1, y1, (x2 - x1), (y2 - y1));
}
