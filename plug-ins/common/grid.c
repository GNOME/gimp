/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

/* Original plug-in coded by Tim Newsome.
 *
 * Changed to make use of real-life units by Sven Neumann <sven@ligma.org>.
 *
 * The interface code is heavily commented in the hope that it will
 * help other plug-in developers to adapt their plug-ins to make use
 * of the ligma_size_entry functionality.
 *
 * Note: There is a convenience constructor called ligma_coordinetes_new ()
 *       which simplifies the task of setting up a standard X,Y sizeentry.
 *
 * For more info and bugs see libligma/ligmasizeentry.h and libligma/ligmawidgets.h
 *
 * May 2000 tim copperfield [timecop@japan.co.jp]
 * http://www.ne.jp/asahi/linux/timecop
 * Added dynamic preview.  Due to weird implementation of signals from all
 * controls, preview will not auto-update.  But this plugin isn't really
 * crying for real-time updating either.
 *
 */

#include "config.h"

#include <string.h>

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "libligma/stdplugins-intl.h"


#define PLUG_IN_PROC        "plug-in-grid"
#define PLUG_IN_BINARY      "grid"
#define PLUG_IN_ROLE        "ligma-grid"
#define SPIN_BUTTON_WIDTH    8
#define COLOR_BUTTON_WIDTH  55


typedef struct
{
  gint    hwidth;
  gint    hspace;
  gint    hoffset;
  LigmaRGB hcolor;
  gint    vwidth;
  gint    vspace;
  gint    voffset;
  LigmaRGB vcolor;
  gint    iwidth;
  gint    ispace;
  gint    ioffset;
  LigmaRGB icolor;
} Config;


typedef struct _Grid      Grid;
typedef struct _GridClass GridClass;

struct _Grid
{
  LigmaPlugIn parent_instance;
};

struct _GridClass
{
  LigmaPlugInClass parent_class;
};


#define GRID_TYPE  (grid_get_type ())
#define GRID (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GRID_TYPE, Grid))

GType                   grid_get_type         (void) G_GNUC_CONST;

static GList          * grid_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * grid_create_procedure (LigmaPlugIn           *plug_in,
                                               const gchar          *name);

static LigmaValueArray * grid_run              (LigmaProcedure        *procedure,
                                               LigmaRunMode           run_mode,
                                               LigmaImage            *image,
                                               gint                  n_drawables,
                                               LigmaDrawable        **drawables,
                                               const LigmaValueArray *args,
                                               gpointer              run_data);

static guchar           best_cmap_match       (const guchar         *cmap,
                                               gint                  ncolors,
                                               const LigmaRGB        *color);
static void             grid                  (LigmaImage            *image,
                                               LigmaDrawable         *drawable,
                                               LigmaPreview          *preview);
static gint             dialog                (LigmaImage            *image,
                                               LigmaDrawable         *drawable);


G_DEFINE_TYPE (Grid, grid, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (GRID_TYPE)
DEFINE_STD_SET_I18N


static gint sx1, sy1, sx2, sy2;

static GtkWidget *main_dialog    = NULL;
static GtkWidget *hcolor_button  = NULL;
static GtkWidget *vcolor_button  = NULL;

static Config grid_cfg =
{
  1, 16, 8, { 0.0, 0.0, 0.0, 1.0 },    /* horizontal   */
  1, 16, 8, { 0.0, 0.0, 0.0, 1.0 },    /* vertical     */
  0,  2, 6, { 0.0, 0.0, 0.0, 1.0 },    /* intersection */
};


static void
grid_class_init (GridClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = grid_query_procedures;
  plug_in_class->create_procedure = grid_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
grid_init (Grid *grid)
{
}

static GList *
grid_query_procedures (LigmaPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static LigmaProcedure *
grid_create_procedure (LigmaPlugIn  *plug_in,
                               const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      const LigmaRGB black = { 0.0, 0.0, 0.0, 1.0 };

      procedure = ligma_image_procedure_new (plug_in, name,
                                            LIGMA_PDB_PROC_TYPE_PLUGIN,
                                            grid_run, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "*");
      ligma_procedure_set_sensitivity_mask (procedure,
                                           LIGMA_PROCEDURE_SENSITIVE_DRAWABLE);

      ligma_procedure_set_menu_label (procedure, _("_Grid (legacy)..."));
      ligma_procedure_add_menu_path (procedure,
                                    "<Image>/Filters/Render/Pattern");

      ligma_procedure_set_documentation (procedure,
                                        _("Draw a grid on the image"),
                                        "Draws a grid using the specified "
                                        "colors. The grid origin is the "
                                        "upper left corner.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Tim Newsome",
                                      "Tim Newsome, Sven Neumann, "
                                      "Tom Rathborne, TC",
                                      "1997 - 2000");

      LIGMA_PROC_ARG_INT (procedure, "hwidth",
                         "H width",
                         "Horizontal width",
                         0, LIGMA_MAX_IMAGE_SIZE, 1,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "hspace",
                         "H space",
                         "Horizontal spacing",
                         1, LIGMA_MAX_IMAGE_SIZE, 16,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "hoffset",
                         "H offset",
                         "Horizontal offset",
                         0, LIGMA_MAX_IMAGE_SIZE, 8,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_RGB (procedure, "hcolor",
                         "H color",
                         "Horizontal color",
                         TRUE, &black,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "vwidth",
                         "V width",
                         "Vertical width",
                         0, LIGMA_MAX_IMAGE_SIZE, 1,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "vspace",
                         "V space",
                         "Vertical spacing",
                         1, LIGMA_MAX_IMAGE_SIZE, 16,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "voffset",
                         "V offset",
                         "Vertical offset",
                         0, LIGMA_MAX_IMAGE_SIZE, 8,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_RGB (procedure, "vcolor",
                         "V color",
                         "Vertical color",
                         TRUE, &black,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "iwidth",
                         "I width",
                         "Intersection width",
                         0, LIGMA_MAX_IMAGE_SIZE, 0,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "ispace",
                         "I space",
                         "Intersection spacing",
                         1, LIGMA_MAX_IMAGE_SIZE, 2,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "ioffset",
                         "I offset",
                         "Intersection offset",
                         0, LIGMA_MAX_IMAGE_SIZE, 6,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_RGB (procedure, "icolor",
                         "I color",
                         "Intersection color",
                         TRUE, &black,
                         G_PARAM_READWRITE);
    }

  return procedure;
}

static LigmaValueArray *
grid_run (LigmaProcedure        *procedure,
          LigmaRunMode           run_mode,
          LigmaImage            *image,
          gint                  n_drawables,
          LigmaDrawable        **drawables,
          const LigmaValueArray *args,
          gpointer              run_data)
{
  LigmaDrawable *drawable;

  gegl_init (NULL, NULL);

  if (n_drawables != 1)
    {
      GError *error = NULL;

      g_set_error (&error, LIGMA_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   PLUG_IN_PROC);

      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  if (run_mode == LIGMA_RUN_NONINTERACTIVE)
    {
      grid_cfg.hwidth  = LIGMA_VALUES_GET_INT (args, 0);
      grid_cfg.hspace  = LIGMA_VALUES_GET_INT (args, 1);
      grid_cfg.hoffset = LIGMA_VALUES_GET_INT (args, 2);
      LIGMA_VALUES_GET_RGB (args, 3, &grid_cfg.hcolor);

      grid_cfg.vwidth  = LIGMA_VALUES_GET_INT (args, 4);
      grid_cfg.vspace  = LIGMA_VALUES_GET_INT (args, 5);
      grid_cfg.voffset = LIGMA_VALUES_GET_INT (args, 6);
      LIGMA_VALUES_GET_RGB (args, 7, &grid_cfg.vcolor);

      grid_cfg.iwidth  = LIGMA_VALUES_GET_INT (args, 8);
      grid_cfg.ispace  = LIGMA_VALUES_GET_INT (args, 9);
      grid_cfg.ioffset = LIGMA_VALUES_GET_INT (args, 10);
      LIGMA_VALUES_GET_RGB (args, 11, &grid_cfg.icolor);
    }
  else
    {
      ligma_context_get_foreground (&grid_cfg.hcolor);
      grid_cfg.vcolor = grid_cfg.icolor = grid_cfg.hcolor;

      ligma_get_data (PLUG_IN_PROC, &grid_cfg);
    }

  if (run_mode == LIGMA_RUN_INTERACTIVE)
    {
      if (! dialog (image, drawable))
        {
          return ligma_procedure_new_return_values (procedure,
                                                   LIGMA_PDB_CANCEL,
                                                   NULL);
        }
    }

  ligma_progress_init (_("Drawing grid"));

  grid (image, drawable, NULL);

  if (run_mode != LIGMA_RUN_NONINTERACTIVE)
    ligma_displays_flush ();

  if (run_mode == LIGMA_RUN_INTERACTIVE)
    ligma_set_data (PLUG_IN_PROC, &grid_cfg, sizeof (grid_cfg));

  return ligma_procedure_new_return_values (procedure, LIGMA_PDB_SUCCESS, NULL);
}


#define MAXDIFF 195076

static guchar
best_cmap_match (const guchar  *cmap,
                 gint           ncolors,
                 const LigmaRGB *color)
{
  guchar cmap_index = 0;
  gint   max = MAXDIFF;
  gint   i, diff, sum;
  guchar r, g, b;

  ligma_rgb_get_uchar (color, &r, &g, &b);

  for (i = 0; i < ncolors; i++)
    {
      diff = r - *cmap++;
      sum = SQR (diff);
      diff = g - *cmap++;
      sum += SQR (diff);
      diff = b - *cmap++;
      sum += SQR (diff);

      if (sum < max)
        {
          cmap_index = i;
          max = sum;
        }
    }

  return cmap_index;
}

static inline void
pix_composite (guchar   *p1,
               guchar    p2[4],
               gint      bytes,
               gboolean  blend,
               gboolean  alpha)
{
  gint b;

  if (blend)
    {
      if (alpha)
        bytes--;

      for (b = 0; b < bytes; b++)
        {
          *p1 = *p1 * (1.0 - p2[3]/255.0) + p2[b] * p2[3]/255.0;
          p1++;
        }
    }
  else
    {
      /* blend should only be TRUE for indexed (bytes == 1) */
      *p1++ = *p2;
    }

  if (alpha && *p1 < 255)
    {
      b = *p1 + 255.0 * ((gdouble) p2[3] / (255.0 - *p1));

      *p1 = b > 255 ? 255 : b;
    }
}

static void
grid (LigmaImage    *image,
      LigmaDrawable *drawable,
      LigmaPreview  *preview)
{
  GeglBuffer *src_buffer;
  GeglBuffer *dest_buffer;
  const Babl *format;
  gint        bytes;
  gint        x_offset;
  gint        y_offset;
  guchar     *dest;
  guchar     *buffer = NULL;
  gint        x, y;
  gboolean    alpha;
  gboolean    blend;
  guchar      hcolor[4];
  guchar      vcolor[4];
  guchar      icolor[4];
  guchar     *cmap;
  gint        ncolors;

  ligma_rgba_get_uchar (&grid_cfg.hcolor,
                       hcolor, hcolor + 1, hcolor + 2, hcolor + 3);
  ligma_rgba_get_uchar (&grid_cfg.vcolor,
                       vcolor, vcolor + 1, vcolor + 2, vcolor + 3);
  ligma_rgba_get_uchar (&grid_cfg.icolor,
                       icolor, icolor + 1, icolor + 2, icolor + 3);

  alpha = ligma_drawable_has_alpha (drawable);

  switch (ligma_image_get_base_type (image))
    {
    case LIGMA_RGB:
      blend = TRUE;

      if (alpha)
        format = babl_format ("R'G'B'A u8");
      else
        format = babl_format ("R'G'B' u8");
      break;

    case LIGMA_GRAY:
      hcolor[0] = ligma_rgb_luminance_uchar (&grid_cfg.hcolor);
      vcolor[0] = ligma_rgb_luminance_uchar (&grid_cfg.vcolor);
      icolor[0] = ligma_rgb_luminance_uchar (&grid_cfg.icolor);
      blend = TRUE;

      if (alpha)
        format = babl_format ("Y'A u8");
      else
        format = babl_format ("Y' u8");
      break;

    case LIGMA_INDEXED:
      cmap = ligma_image_get_colormap (image, &ncolors);

      hcolor[0] = best_cmap_match (cmap, ncolors, &grid_cfg.hcolor);
      vcolor[0] = best_cmap_match (cmap, ncolors, &grid_cfg.vcolor);
      icolor[0] = best_cmap_match (cmap, ncolors, &grid_cfg.icolor);

      g_free (cmap);
      blend = FALSE;

      format = ligma_drawable_get_format (drawable);
      break;

    default:
      g_assert_not_reached ();
      blend = FALSE;
    }

  bytes = babl_format_get_bytes_per_pixel (format);

  if (preview)
    {
      ligma_preview_get_position (preview, &sx1, &sy1);
      ligma_preview_get_size (preview, &sx2, &sy2);

      buffer = g_new (guchar, bytes * sx2 * sy2);

      sx2 += sx1;
      sy2 += sy1;
    }
  else
    {
      gint w, h;

      if (! ligma_drawable_mask_intersect (drawable,
                                          &sx1, &sy1, &w, &h))
        return;

      sx2 = sx1 + w;
      sy2 = sy1 + h;

      dest_buffer = ligma_drawable_get_shadow_buffer (drawable);
    }

  src_buffer = ligma_drawable_get_buffer (drawable);

  dest = g_new (guchar, (sx2 - sx1) * bytes);

  for (y = sy1; y < sy2; y++)
    {
      gegl_buffer_get (src_buffer, GEGL_RECTANGLE (sx1, y, sx2 - sx1, 1), 1.0,
                       format, dest,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      y_offset = y - grid_cfg.hoffset;
      while (y_offset < 0)
        y_offset += grid_cfg.hspace;

      if ((y_offset +
           (grid_cfg.hwidth / 2)) % grid_cfg.hspace < grid_cfg.hwidth)
        {
          for (x = sx1; x < sx2; x++)
            {
              pix_composite (&dest[(x-sx1) * bytes],
                             hcolor, bytes, blend, alpha);
            }
        }

      for (x = sx1; x < sx2; x++)
        {
          x_offset = grid_cfg.vspace + x - grid_cfg.voffset;
          while (x_offset < 0)
            x_offset += grid_cfg.vspace;

          if ((x_offset +
               (grid_cfg.vwidth / 2)) % grid_cfg.vspace < grid_cfg.vwidth)
            {
              pix_composite (&dest[(x-sx1) * bytes],
                             vcolor, bytes, blend, alpha);
            }

          if ((x_offset +
               (grid_cfg.iwidth / 2)) % grid_cfg.vspace < grid_cfg.iwidth
              &&
              ((y_offset % grid_cfg.hspace >= grid_cfg.ispace
                &&
                y_offset % grid_cfg.hspace < grid_cfg.ioffset)
               ||
               (grid_cfg.hspace -
                (y_offset % grid_cfg.hspace) >= grid_cfg.ispace
                &&
                grid_cfg.hspace -
                (y_offset % grid_cfg.hspace) < grid_cfg.ioffset)))
            {
              pix_composite (&dest[(x-sx1) * bytes],
                             icolor, bytes, blend, alpha);
            }
        }

      if ((y_offset +
           (grid_cfg.iwidth / 2)) % grid_cfg.hspace < grid_cfg.iwidth)
        {
          for (x = sx1; x < sx2; x++)
            {
              x_offset = grid_cfg.vspace + x - grid_cfg.voffset;
              while (x_offset < 0)
                x_offset += grid_cfg.vspace;

              if ((x_offset % grid_cfg.vspace >= grid_cfg.ispace
                   &&
                   x_offset % grid_cfg.vspace < grid_cfg.ioffset)
                  ||
                  (grid_cfg.vspace -
                   (x_offset % grid_cfg.vspace) >= grid_cfg.ispace
                   &&
                   grid_cfg.vspace -
                   (x_offset % grid_cfg.vspace) < grid_cfg.ioffset))
                {
                  pix_composite (&dest[(x-sx1) * bytes],
                                 icolor, bytes, blend, alpha);
                }
            }
        }

      if (preview)
        {
          memcpy (buffer + (y - sy1) * (sx2 - sx1) * bytes,
                  dest,
                  (sx2 - sx1) * bytes);
        }
      else
        {
          gegl_buffer_set (dest_buffer,
                           GEGL_RECTANGLE (sx1, y, sx2 - sx1, 1), 0,
                           format, dest,
                           GEGL_AUTO_ROWSTRIDE);

          if (y % 16 == 0)
            ligma_progress_update ((gdouble) y / (gdouble) (sy2 - sy1));
        }
    }

  g_free (dest);

  g_object_unref (src_buffer);

  if (preview)
    {
      ligma_preview_draw_buffer (preview, buffer, bytes * (sx2 - sx1));
      g_free (buffer);
    }
  else
    {
      ligma_progress_update (1.0);

      g_object_unref (dest_buffer);

      ligma_drawable_merge_shadow (drawable, TRUE);
      ligma_drawable_update (drawable,
                            sx1, sy1, sx2 - sx1, sy2 - sy1);
    }
}


/***************************************************
 * GUI stuff
 */


static void
update_values (void)
{
  GtkWidget *entry;

  entry = g_object_get_data (G_OBJECT (main_dialog), "width");

  grid_cfg.hwidth =
    RINT (ligma_size_entry_get_refval (LIGMA_SIZE_ENTRY (entry), 0));
  grid_cfg.vwidth =
    RINT (ligma_size_entry_get_refval (LIGMA_SIZE_ENTRY (entry), 1));
  grid_cfg.iwidth =
    RINT (ligma_size_entry_get_refval (LIGMA_SIZE_ENTRY (entry), 2));

  entry = g_object_get_data (G_OBJECT (main_dialog), "space");

  grid_cfg.hspace =
    RINT (ligma_size_entry_get_refval (LIGMA_SIZE_ENTRY (entry), 0));
  grid_cfg.vspace =
    RINT (ligma_size_entry_get_refval (LIGMA_SIZE_ENTRY (entry), 1));
  grid_cfg.ispace =
    RINT (ligma_size_entry_get_refval (LIGMA_SIZE_ENTRY (entry), 2));

  entry = g_object_get_data (G_OBJECT (main_dialog), "offset");

  grid_cfg.hoffset =
    RINT (ligma_size_entry_get_refval (LIGMA_SIZE_ENTRY (entry), 0));
  grid_cfg.voffset =
    RINT (ligma_size_entry_get_refval (LIGMA_SIZE_ENTRY (entry), 1));
  grid_cfg.ioffset =
    RINT (ligma_size_entry_get_refval (LIGMA_SIZE_ENTRY (entry), 2));
}

static void
update_preview (LigmaPreview  *preview,
                LigmaDrawable  *drawable)
{
  update_values ();

  grid (ligma_item_get_image (LIGMA_ITEM (drawable)),
        drawable, preview);
}

static void
entry_callback (GtkWidget *widget,
                gpointer   data)
{
  static gdouble x = -1.0;
  static gdouble y = -1.0;
  gdouble new_x;
  gdouble new_y;

  new_x = ligma_size_entry_get_refval (LIGMA_SIZE_ENTRY (widget), 0);
  new_y = ligma_size_entry_get_refval (LIGMA_SIZE_ENTRY (widget), 1);

  if (ligma_chain_button_get_active (LIGMA_CHAIN_BUTTON (data)))
    {
      if (new_x != x)
        {
          y = new_y = x = new_x;
          ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (widget), 1, y);
        }
      if (new_y != y)
        {
          x = new_x = y = new_y;
          ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (widget), 0, x);
        }
    }
  else
    {
      x = new_x;
      y = new_y;
    }
}

static void
color_callback (GtkWidget *widget,
                gpointer   data)
{
  if (ligma_chain_button_get_active (LIGMA_CHAIN_BUTTON (data)))
    {
      LigmaRGB  color;

      ligma_color_button_get_color (LIGMA_COLOR_BUTTON (widget), &color);

      if (widget == vcolor_button)
        ligma_color_button_set_color (LIGMA_COLOR_BUTTON (hcolor_button), &color);
      else if (widget == hcolor_button)
        ligma_color_button_set_color (LIGMA_COLOR_BUTTON (vcolor_button), &color);
    }
}


static gint
dialog (LigmaImage    *image,
        LigmaDrawable *drawable)
{
  LigmaColorConfig *config;
  GtkWidget       *dlg;
  GtkWidget       *main_vbox;
  GtkWidget       *vbox;
  GtkSizeGroup    *group;
  GtkWidget       *label;
  GtkWidget       *preview;
  GtkWidget       *button;
  GtkWidget       *width;
  GtkWidget       *space;
  GtkWidget       *offset;
  GtkWidget       *chain_button;
  LigmaUnit         unit;
  gint             d_width;
  gint             d_height;
  gdouble          xres;
  gdouble          yres;
  gboolean         run;

  g_return_val_if_fail (main_dialog == NULL, FALSE);

  ligma_ui_init (PLUG_IN_BINARY);

  d_width  = ligma_drawable_get_width  (drawable);
  d_height = ligma_drawable_get_height (drawable);

  main_dialog = dlg = ligma_dialog_new (_("Grid"), PLUG_IN_ROLE,
                                       NULL, 0,
                                       ligma_standard_help_func, PLUG_IN_PROC,

                                       _("_Cancel"), GTK_RESPONSE_CANCEL,
                                       _("_OK"),     GTK_RESPONSE_OK,

                                       NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dlg),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  ligma_window_set_transient (GTK_WINDOW (dlg));

  /*  Get the image resolution and unit  */
  ligma_image_get_resolution (image, &xres, &yres);
  unit = ligma_image_get_unit (image);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dlg))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  preview = ligma_drawable_preview_new_from_drawable (drawable);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);

  g_signal_connect (preview, "invalidated",
                    G_CALLBACK (update_preview),
                    drawable);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (main_vbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  /*  The width entries  */
  width = ligma_size_entry_new (3,                            /*  number_of_fields  */
                               unit,                         /*  unit              */
                               "%a",                         /*  unit_format       */
                               TRUE,                         /*  menu_show_pixels  */
                               TRUE,                         /*  menu_show_percent */
                               FALSE,                        /*  show_refval       */
                               SPIN_BUTTON_WIDTH,            /*  spinbutton_usize  */
                               LIGMA_SIZE_ENTRY_UPDATE_SIZE); /*  update_policy     */


  gtk_box_pack_start (GTK_BOX (vbox), width, FALSE, FALSE, 0);
  gtk_widget_show (width);

  /*  set the unit back to pixels, since most times we will want pixels */
  ligma_size_entry_set_unit (LIGMA_SIZE_ENTRY (width), LIGMA_UNIT_PIXEL);

  /*  set the resolution to the image resolution  */
  ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (width), 0, xres, TRUE);
  ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (width), 1, yres, TRUE);
  ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (width), 2, xres, TRUE);

  /*  set the size (in pixels) that will be treated as 0% and 100%  */
  ligma_size_entry_set_size (LIGMA_SIZE_ENTRY (width), 0, 0.0, d_height);
  ligma_size_entry_set_size (LIGMA_SIZE_ENTRY (width), 1, 0.0, d_width);
  ligma_size_entry_set_size (LIGMA_SIZE_ENTRY (width), 2, 0.0, d_width);

  /*  set upper and lower limits (in pixels)  */
  ligma_size_entry_set_refval_boundaries (LIGMA_SIZE_ENTRY (width), 0, 0.0,
                                         d_height);
  ligma_size_entry_set_refval_boundaries (LIGMA_SIZE_ENTRY (width), 1, 0.0,
                                         d_width);
  ligma_size_entry_set_refval_boundaries (LIGMA_SIZE_ENTRY (width), 2, 0.0,
                                         MAX (d_width, d_height));
  gtk_grid_set_column_spacing (GTK_GRID (width), 6);

  /*  initialize the values  */
  ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (width), 0, grid_cfg.hwidth);
  ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (width), 1, grid_cfg.vwidth);
  ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (width), 2, grid_cfg.iwidth);

  /*  attach labels  */
  ligma_size_entry_attach_label (LIGMA_SIZE_ENTRY (width), _("Horizontal\nLines"),
                                0, 1, 0.0);
  ligma_size_entry_attach_label (LIGMA_SIZE_ENTRY (width), _("Vertical\nLines"),
                                0, 2, 0.0);
  ligma_size_entry_attach_label (LIGMA_SIZE_ENTRY (width), _("Intersection"),
                                0, 3, 0.0);

  label = ligma_size_entry_attach_label (LIGMA_SIZE_ENTRY (width), _("Width:"),
                                        1, 0, 0.0);

  group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  gtk_size_group_add_widget (group, label);
  g_object_unref (group);

  /*  put a chain_button under the size_entries  */
  chain_button = ligma_chain_button_new (LIGMA_CHAIN_BOTTOM);
  if (grid_cfg.hwidth == grid_cfg.vwidth)
    ligma_chain_button_set_active (LIGMA_CHAIN_BUTTON (chain_button), TRUE);
  gtk_grid_attach (GTK_GRID (width), chain_button, 1, 2, 2, 1);
  gtk_widget_show (chain_button);

  /* connect to the 'value-changed' signal because we have to take care
   * of keeping the entries in sync when the chainbutton is active
   */
  g_signal_connect (width, "value-changed",
                    G_CALLBACK (entry_callback),
                    chain_button);
  g_signal_connect_swapped (width, "value-changed",
                            G_CALLBACK (ligma_preview_invalidate),
                            preview);

  /*  The spacing entries  */
  space = ligma_size_entry_new (3,                            /*  number_of_fields  */
                               unit,                         /*  unit              */
                               "%a",                         /*  unit_format       */
                               TRUE,                         /*  menu_show_pixels  */
                               TRUE,                         /*  menu_show_percent */
                               FALSE,                        /*  show_refval       */
                               SPIN_BUTTON_WIDTH,            /*  spinbutton_usize  */
                               LIGMA_SIZE_ENTRY_UPDATE_SIZE); /*  update_policy     */

  gtk_box_pack_start (GTK_BOX (vbox), space, FALSE, FALSE, 0);
  gtk_widget_show (space);

  ligma_size_entry_set_unit (LIGMA_SIZE_ENTRY (space), LIGMA_UNIT_PIXEL);

  /*  set the resolution to the image resolution  */
  ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (space), 0, xres, TRUE);
  ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (space), 1, yres, TRUE);
  ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (space), 2, xres, TRUE);

  /*  set the size (in pixels) that will be treated as 0% and 100%  */
  ligma_size_entry_set_size (LIGMA_SIZE_ENTRY (space), 0, 0.0, d_height);
  ligma_size_entry_set_size (LIGMA_SIZE_ENTRY (space), 1, 0.0, d_width);
  ligma_size_entry_set_size (LIGMA_SIZE_ENTRY (space), 2, 0.0, d_width);

  /*  set upper and lower limits (in pixels)  */
  ligma_size_entry_set_refval_boundaries (LIGMA_SIZE_ENTRY (space), 0, 1.0,
                                         d_height);
  ligma_size_entry_set_refval_boundaries (LIGMA_SIZE_ENTRY (space), 1, 1.0,
                                         d_width);
  ligma_size_entry_set_refval_boundaries (LIGMA_SIZE_ENTRY (space), 2, 0.0,
                                         MAX (d_width, d_height));
  gtk_grid_set_column_spacing (GTK_GRID (space), 6);

  /*  initialize the values  */
  ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (space), 0, grid_cfg.hspace);
  ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (space), 1, grid_cfg.vspace);
  ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (space), 2, grid_cfg.ispace);

  /*  attach labels  */
  label = ligma_size_entry_attach_label (LIGMA_SIZE_ENTRY (space), _("Spacing:"),
                                        1, 0, 0.0);
  gtk_size_group_add_widget (group, label);

  /*  put a chain_button under the spacing_entries  */
  chain_button = ligma_chain_button_new (LIGMA_CHAIN_BOTTOM);
  if (grid_cfg.hspace == grid_cfg.vspace)
    ligma_chain_button_set_active (LIGMA_CHAIN_BUTTON (chain_button), TRUE);
  gtk_grid_attach (GTK_GRID (space), chain_button, 1, 2, 2, 1);
  gtk_widget_show (chain_button);

  /* connect to the 'value-changed' and "unit-changed" signals because
   * we have to take care of keeping the entries in sync when the
   * chainbutton is active
   */
  g_signal_connect (space, "value-changed",
                    G_CALLBACK (entry_callback),
                    chain_button);
  g_signal_connect (space, "unit-changed",
                    G_CALLBACK (entry_callback),
                    chain_button);
  g_signal_connect_swapped (space, "value-changed",
                            G_CALLBACK (ligma_preview_invalidate),
                            preview);

  /*  The offset entries  */
  offset = ligma_size_entry_new (3,                            /*  number_of_fields  */
                                unit,                         /*  unit              */
                                "%a",                         /*  unit_format       */
                                TRUE,                         /*  menu_show_pixels  */
                                TRUE,                         /*  menu_show_percent */
                                FALSE,                        /*  show_refval       */
                                SPIN_BUTTON_WIDTH,            /*  spinbutton_usize  */
                                LIGMA_SIZE_ENTRY_UPDATE_SIZE); /*  update_policy     */

  gtk_box_pack_start (GTK_BOX (vbox), offset, FALSE, FALSE, 0);
  gtk_widget_show (offset);

  ligma_size_entry_set_unit (LIGMA_SIZE_ENTRY (offset), LIGMA_UNIT_PIXEL);

  /*  set the resolution to the image resolution  */
  ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (offset), 0, xres, TRUE);
  ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (offset), 1, yres, TRUE);
  ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (offset), 2, xres, TRUE);

  /*  set the size (in pixels) that will be treated as 0% and 100%  */
  ligma_size_entry_set_size (LIGMA_SIZE_ENTRY (offset), 0, 0.0, d_height);
  ligma_size_entry_set_size (LIGMA_SIZE_ENTRY (offset), 1, 0.0, d_width);
  ligma_size_entry_set_size (LIGMA_SIZE_ENTRY (offset), 2, 0.0, d_width);

  /*  set upper and lower limits (in pixels)  */
  ligma_size_entry_set_refval_boundaries (LIGMA_SIZE_ENTRY (offset), 0, 0.0,
                                         d_height);
  ligma_size_entry_set_refval_boundaries (LIGMA_SIZE_ENTRY (offset), 1, 0.0,
                                         d_width);
  ligma_size_entry_set_refval_boundaries (LIGMA_SIZE_ENTRY (offset), 2, 0.0,
                                         MAX (d_width, d_height));
  gtk_grid_set_column_spacing (GTK_GRID (offset), 6);

  /*  initialize the values  */
  ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (offset), 0, grid_cfg.hoffset);
  ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (offset), 1, grid_cfg.voffset);
  ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (offset), 2, grid_cfg.ioffset);

  /*  attach labels  */
  label = ligma_size_entry_attach_label (LIGMA_SIZE_ENTRY (offset), _("Offset:"),
                                        1, 0, 0.0);
  gtk_size_group_add_widget (group, label);

  /*  put a chain_button under the offset_entries  */
  chain_button = ligma_chain_button_new (LIGMA_CHAIN_BOTTOM);
  if (grid_cfg.hoffset == grid_cfg.voffset)
    ligma_chain_button_set_active (LIGMA_CHAIN_BUTTON (chain_button), TRUE);
  gtk_grid_attach (GTK_GRID (offset), chain_button, 1, 2, 2, 1);
  gtk_widget_show (chain_button);

  /* connect to the 'value-changed' and "unit-changed" signals because
   * we have to take care of keeping the entries in sync when the
   * chainbutton is active
   */
  g_signal_connect (offset, "value-changed",
                    G_CALLBACK (entry_callback),
                    chain_button);
  g_signal_connect (offset, "unit-changed",
                    G_CALLBACK (entry_callback),
                    chain_button);
  g_signal_connect_swapped (offset, "value-changed",
                            G_CALLBACK (ligma_preview_invalidate),
                            preview);

  /*  put a chain_button under the color_buttons  */
  chain_button = ligma_chain_button_new (LIGMA_CHAIN_BOTTOM);
  if (ligma_rgba_distance (&grid_cfg.hcolor, &grid_cfg.vcolor) < 0.0001)
    ligma_chain_button_set_active (LIGMA_CHAIN_BUTTON (chain_button), TRUE);
  gtk_grid_attach (GTK_GRID (offset), chain_button, 1, 4, 2, 1);
  gtk_widget_show (chain_button);

  /*  attach color selectors  */
  hcolor_button = ligma_color_button_new (_("Horizontal Color"),
                                         COLOR_BUTTON_WIDTH, 16,
                                         &grid_cfg.hcolor,
                                         LIGMA_COLOR_AREA_SMALL_CHECKS);
  ligma_color_button_set_update (LIGMA_COLOR_BUTTON (hcolor_button), TRUE);
  gtk_grid_attach (GTK_GRID (offset), hcolor_button, 1, 3, 1, 1);
  gtk_widget_show (hcolor_button);

  config = ligma_get_color_configuration ();
  ligma_color_button_set_color_config (LIGMA_COLOR_BUTTON (hcolor_button),
                                      config);

  g_signal_connect (hcolor_button, "color-changed",
                    G_CALLBACK (ligma_color_button_get_color),
                    &grid_cfg.hcolor);
  g_signal_connect (hcolor_button, "color-changed",
                    G_CALLBACK (color_callback),
                    chain_button);
  g_signal_connect_swapped (hcolor_button, "color-changed",
                            G_CALLBACK (ligma_preview_invalidate),
                            preview);

  vcolor_button = ligma_color_button_new (_("Vertical Color"),
                                         COLOR_BUTTON_WIDTH, 16,
                                         &grid_cfg.vcolor,
                                         LIGMA_COLOR_AREA_SMALL_CHECKS);
  ligma_color_button_set_update (LIGMA_COLOR_BUTTON (vcolor_button), TRUE);
  gtk_grid_attach (GTK_GRID (offset), vcolor_button, 2, 3, 1, 1);
  gtk_widget_show (vcolor_button);

  ligma_color_button_set_color_config (LIGMA_COLOR_BUTTON (vcolor_button),
                                      config);

  g_signal_connect (vcolor_button, "color-changed",
                    G_CALLBACK (ligma_color_button_get_color),
                    &grid_cfg.vcolor);
  g_signal_connect (vcolor_button, "color-changed",
                    G_CALLBACK (color_callback),
                    chain_button);
  g_signal_connect_swapped (vcolor_button, "color-changed",
                            G_CALLBACK (ligma_preview_invalidate),
                            preview);

  button = ligma_color_button_new (_("Intersection Color"),
                                  COLOR_BUTTON_WIDTH, 16,
                                  &grid_cfg.icolor,
                                  LIGMA_COLOR_AREA_SMALL_CHECKS);
  ligma_color_button_set_update (LIGMA_COLOR_BUTTON (button), TRUE);
  gtk_grid_attach (GTK_GRID (offset), button, 3, 3, 1, 1);
  gtk_widget_show (button);

  ligma_color_button_set_color_config (LIGMA_COLOR_BUTTON (button),
                                      config);
  g_object_unref (config);

  g_signal_connect (button, "color-changed",
                    G_CALLBACK (ligma_color_button_get_color),
                    &grid_cfg.icolor);
  g_signal_connect_swapped (button, "color-changed",
                            G_CALLBACK (ligma_preview_invalidate),
                            preview);

  gtk_widget_show (dlg);

  g_object_set_data (G_OBJECT (dlg), "width",  width);
  g_object_set_data (G_OBJECT (dlg), "space",  space);
  g_object_set_data (G_OBJECT (dlg), "offset", offset);

  run = (ligma_dialog_run (LIGMA_DIALOG (dlg)) == GTK_RESPONSE_OK);

  if (run)
    update_values ();

  gtk_widget_destroy (dlg);

  return run;
}

