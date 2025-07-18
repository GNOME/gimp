/* GIMP - The GNU Image Manipulation Program
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
 * Changed to make use of real-life units by Sven Neumann <sven@gimp.org>.
 *
 * The interface code is heavily commented in the hope that it will
 * help other plug-in developers to adapt their plug-ins to make use
 * of the gimp_size_entry functionality.
 *
 * Note: There is a convenience constructor called gimp_coordinetes_new ()
 *       which simplifies the task of setting up a standard X,Y sizeentry.
 *
 * For more info and bugs see libgimp/gimpsizeentry.h and libgimp/gimpwidgets.h
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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC         "plug-in-grid"
#define PLUG_IN_BINARY       "grid"
#define PLUG_IN_ROLE         "gimp-grid"
#define SPIN_BUTTON_WIDTH     8
#define COLOR_BUTTON_WIDTH  110


typedef struct _Grid      Grid;
typedef struct _GridClass GridClass;

struct _Grid
{
  GimpPlugIn           parent_instance;

  GimpDrawable        *drawable;
  GimpProcedureConfig *config;
  GtkWidget           *hcolor_button;
  GtkWidget           *vcolor_button;
  GtkWidget           *color_chain;
};

struct _GridClass
{
  GimpPlugInClass parent_class;
};


#define GRID_TYPE  (grid_get_type ())
#define GRID(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GRID_TYPE, Grid))

GType                   grid_get_type         (void) G_GNUC_CONST;

static GList          * grid_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * grid_create_procedure (GimpPlugIn           *plug_in,
                                               const gchar          *name);

static GimpValueArray * grid_run              (GimpProcedure        *procedure,
                                               GimpRunMode           run_mode,
                                               GimpImage            *image,
                                               GimpDrawable        **drawables,
                                               GimpProcedureConfig  *config,
                                               gpointer              run_data);

static guchar           best_cmap_match       (const guchar         *cmap,
                                               gint                  ncolors,
                                               GeglColor            *color);
static void             render_grid           (GimpImage            *image,
                                               GimpDrawable         *drawable,
                                               GimpProcedureConfig  *config,
                                               GimpPreview          *preview);
static gint             dialog                (GimpImage            *image,
                                               GimpDrawable         *drawable,
                                               GimpProcedure        *procedure,
                                               GimpProcedureConfig  *config);


G_DEFINE_TYPE (Grid, grid, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (GRID_TYPE)
DEFINE_STD_SET_I18N


static gint sx1, sy1, sx2, sy2;

static GtkWidget *main_dialog    = NULL;


static void
grid_class_init (GridClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = grid_query_procedures;
  plug_in_class->create_procedure = grid_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
grid_init (Grid *grid)
{
}

static GList *
grid_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
grid_create_procedure (GimpPlugIn  *plug_in,
                       const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      GeglColor *default_hcolor;
      GeglColor *default_vcolor;
      GeglColor *default_icolor;

      gegl_init (NULL, NULL);

      default_hcolor = gegl_color_new ("black");
      default_vcolor = gegl_color_new ("black");
      default_icolor = gegl_color_new ("black");

      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            grid_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

      gimp_procedure_set_menu_label (procedure, _("_Grid (legacy)..."));
      gimp_procedure_add_menu_path (procedure,
                                    "<Image>/Filters/Render/Pattern");

      gimp_procedure_set_documentation (procedure,
                                        _("Draw a grid on the image"),
                                        _("Draws a grid using the specified "
                                          "colors. The grid origin is the "
                                          "upper left corner."),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Tim Newsome",
                                      "Tim Newsome, Sven Neumann, "
                                      "Tom Rathborne, TC",
                                      "1997 - 2000");

      gimp_procedure_add_int_argument (procedure, "hwidth",
                                       _("Horizontal width"),
                                       _("Horizontal width"),
                                       0, GIMP_MAX_IMAGE_SIZE, 1,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "hspace",
                                       _("Horizontal spacing"),
                                       _("Horizontal spacing"),
                                       1, GIMP_MAX_IMAGE_SIZE, 16,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "hoffset",
                                       _("Horizontal offset"),
                                       _("Horizontal offset"),
                                       0, GIMP_MAX_IMAGE_SIZE, 8,
                                       G_PARAM_READWRITE);

      /* TODO: for "hcolor", "icolor" and "vcolor", the original code would use
       * the foreground color as default. Future work would be to get the
       * foreground/background color from context.
       */
      gimp_procedure_add_color_argument (procedure, "hcolor",
                                         _("Horizontal color"),
                                         _("Horizontal color"),
                                         TRUE, default_hcolor,
                                         G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "vwidth",
                                       _("Vertical width"),
                                       _("Vertical width"),
                                       0, GIMP_MAX_IMAGE_SIZE, 1,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "vspace",
                                       _("Vertical spacing"),
                                       _("Vertical spacing"),
                                       1, GIMP_MAX_IMAGE_SIZE, 16,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "voffset",
                                       _("Vertical offset"),
                                       _("Vertical offset"),
                                       0, GIMP_MAX_IMAGE_SIZE, 8,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_color_argument (procedure, "vcolor",
                                         _("Vertical color"),
                                         _("Vertical color"),
                                         TRUE, default_vcolor,
                                         G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "iwidth",
                                       _("Intersection width"),
                                       _("Intersection width"),
                                       0, GIMP_MAX_IMAGE_SIZE, 0,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "ispace",
                                       _("Intersection spacing"),
                                       _("Intersection spacing"),
                                       1, GIMP_MAX_IMAGE_SIZE, 2,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "ioffset",
                                       _("Intersection offset"),
                                       _("Intersection offset"),
                                       0, GIMP_MAX_IMAGE_SIZE, 6,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_color_argument (procedure, "icolor",
                                         _("Intersection color"),
                                         _("Intersection color"),
                                         TRUE, default_icolor,
                                         G_PARAM_READWRITE);

      gimp_procedure_add_unit_aux_argument (procedure, "width-unit",
                                            NULL, NULL,
                                            TRUE, TRUE, gimp_unit_pixel (),
                                            GIMP_PARAM_READWRITE);
      gimp_procedure_add_unit_aux_argument (procedure,
                                            "intersection-width-unit",
                                            NULL, NULL,
                                            TRUE, TRUE, gimp_unit_pixel (),
                                            GIMP_PARAM_READWRITE);

      gimp_procedure_add_unit_aux_argument (procedure, "space-unit",
                                            NULL, NULL,
                                            TRUE, TRUE, gimp_unit_pixel (),
                                            GIMP_PARAM_READWRITE);
      gimp_procedure_add_unit_aux_argument (procedure,
                                            "intersection-space-unit",
                                            NULL, NULL,
                                            TRUE, TRUE, gimp_unit_pixel (),
                                            GIMP_PARAM_READWRITE);

      gimp_procedure_add_unit_aux_argument (procedure, "offset-unit",
                                            NULL, NULL,
                                            TRUE, TRUE, gimp_unit_pixel (),
                                            GIMP_PARAM_READWRITE);
      gimp_procedure_add_unit_aux_argument (procedure,
                                            "intersection-offset-unit",
                                            NULL, NULL,
                                            TRUE, TRUE, gimp_unit_pixel (),
                                            GIMP_PARAM_READWRITE);

      g_object_unref (default_hcolor);
      g_object_unref (default_vcolor);
      g_object_unref (default_icolor);
    }

  return procedure;
}

static GimpValueArray *
grid_run (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GimpImage            *image,
          GimpDrawable        **drawables,
          GimpProcedureConfig  *config,
          gpointer              run_data)
{
  GimpDrawable *drawable;

  gegl_init (NULL, NULL);

  if (gimp_core_object_array_get_length ((GObject **) drawables) != 1)
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

  if (run_mode == GIMP_RUN_INTERACTIVE && ! dialog (image, drawable, procedure, config))
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_CANCEL,
                                             NULL);

  gimp_progress_init (_("Drawing grid"));

  render_grid (image, drawable, config, NULL);

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush ();

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}


#define MAXDIFF 195076

static guchar
best_cmap_match (const guchar  *cmap,
                 gint           ncolors,
                 GeglColor     *color)
{
  guchar cmap_index = 0;
  gint   max        = MAXDIFF;
  gint   i, diff, sum;
  guchar rgb[3];

  gegl_color_get_pixel (color, babl_format ("R'G'B' u8"), rgb);

  for (i = 0; i < ncolors; i++)
    {
      diff = rgb[0] - *cmap++;
      sum = SQR (diff);
      diff = rgb[1] - *cmap++;
      sum += SQR (diff);
      diff = rgb[2] - *cmap++;
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
  gint alpha_index;

  alpha_index = alpha ? (bytes - 1) : bytes;

  if (blend)
    {
      if (alpha)
        bytes--;

      for (b = 0; b < bytes; b++)
        {
          *p1 = *p1 * (1.0 - p2[alpha_index] / 255.0) +
                p2[b] * p2[alpha_index] / 255.0;
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
      b = *p1 + 255.0 * ((gdouble) p2[alpha_index] / (255.0 - *p1));

      *p1 = b > 255 ? 255 : b;
    }
}

static void
render_grid (GimpImage           *image,
             GimpDrawable        *drawable,
             GimpProcedureConfig *config,
             GimpPreview         *preview)
{
  GeglBuffer *src_buffer  = NULL;
  GeglBuffer *dest_buffer = NULL;
  const Babl *format;
  gint        bytes;
  gint        x_offset;
  gint        y_offset;
  guchar     *dest        = NULL;
  guchar     *buffer      = NULL;
  gint        x, y;
  gboolean    alpha;
  gboolean    blend;
  guchar      hcolor[4];
  guchar      vcolor[4];
  guchar      icolor[4];
  guchar     *cmap;
  gint        ncolors;

  gint        hwidth;
  gint        hspace;
  gint        hoffset;
  GeglColor  *hcolor_gegl;
  gint        vwidth;
  gint        vspace;
  gint        voffset;
  GeglColor  *vcolor_gegl;
  gint        iwidth;
  gint        ispace;
  gint        ioffset;
  GeglColor  *icolor_gegl;

  g_object_get (config,
                "hwidth",  &hwidth,
                "hspace",  &hspace,
                "hoffset", &hoffset,
                "hcolor",  &hcolor_gegl,
                "vwidth",  &vwidth,
                "vspace",  &vspace,
                "voffset", &voffset,
                "vcolor",  &vcolor_gegl,
                "iwidth",  &iwidth,
                "ispace",  &ispace,
                "ioffset", &ioffset,
                "icolor",  &icolor_gegl,
                NULL);

  if (hcolor_gegl == NULL)
    hcolor_gegl = gegl_color_new ("black");
  if (vcolor_gegl == NULL)
    vcolor_gegl = gegl_color_new ("black");
  if (icolor_gegl == NULL)
    icolor_gegl = gegl_color_new ("black");

  alpha = gimp_drawable_has_alpha (drawable);

  switch (gimp_image_get_base_type (image))
    {
    case GIMP_RGB:
      blend = TRUE;

      if (alpha)
        format = babl_format ("R'G'B'A u8");
      else
        format = babl_format ("R'G'B' u8");

      gegl_color_get_pixel (hcolor_gegl, babl_format ("R'G'B'A u8"), hcolor);
      gegl_color_get_pixel (vcolor_gegl, babl_format ("R'G'B'A u8"), vcolor);
      gegl_color_get_pixel (icolor_gegl, babl_format ("R'G'B'A u8"), icolor);
      break;

    case GIMP_GRAY:
      blend = TRUE;

      if (alpha)
        format = babl_format ("Y'A u8");
      else
        format = babl_format ("Y' u8");

      gegl_color_get_pixel (hcolor_gegl, babl_format ("Y'A u8"), hcolor);
      gegl_color_get_pixel (vcolor_gegl, babl_format ("Y'A u8"), vcolor);
      gegl_color_get_pixel (icolor_gegl, babl_format ("Y'A u8"), icolor);
      break;

    case GIMP_INDEXED:
      cmap = gimp_palette_get_colormap (gimp_image_get_palette (image), babl_format ("R'G'B' u8"), &ncolors, NULL);

      hcolor[0] = best_cmap_match (cmap, ncolors, hcolor_gegl);
      vcolor[0] = best_cmap_match (cmap, ncolors, vcolor_gegl);
      icolor[0] = best_cmap_match (cmap, ncolors, icolor_gegl);

      g_free (cmap);
      blend = FALSE;

      format = gimp_drawable_get_format (drawable);
      break;

    default:
      g_assert_not_reached ();
      blend = FALSE;
    }

  g_clear_object (&hcolor_gegl);
  g_clear_object (&vcolor_gegl);
  g_clear_object (&icolor_gegl);

  bytes = babl_format_get_bytes_per_pixel (format);

  if (preview)
    {
      gimp_preview_get_position (preview, &sx1, &sy1);
      gimp_preview_get_size (preview, &sx2, &sy2);

      buffer = g_new (guchar, bytes * sx2 * sy2);

      sx2 += sx1;
      sy2 += sy1;
    }
  else
    {
      gint w, h;

      if (! gimp_drawable_mask_intersect (drawable,
                                          &sx1, &sy1, &w, &h))
        return;

      sx2 = sx1 + w;
      sy2 = sy1 + h;

      dest_buffer = gimp_drawable_get_shadow_buffer (drawable);
    }

  src_buffer = gimp_drawable_get_buffer (drawable);

  dest = g_new (guchar, (sx2 - sx1) * bytes);

  for (y = sy1; y < sy2; y++)
    {
      gegl_buffer_get (src_buffer, GEGL_RECTANGLE (sx1, y, sx2 - sx1, 1), 1.0,
                       format, dest,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      y_offset = y - hoffset;
      while (y_offset < 0)
        y_offset += hspace;

      if ((y_offset + (hwidth / 2)) % hspace < hwidth)
        {
          for (x = sx1; x < sx2; x++)
            {
              pix_composite (&dest[(x-sx1) * bytes],
                             hcolor, bytes, blend, alpha);
            }
        }

      for (x = sx1; x < sx2; x++)
        {
          x_offset = vspace + x - voffset;
          while (x_offset < 0)
            x_offset += vspace;

          if ((x_offset + (vwidth / 2)) % vspace < vwidth)
            {
              pix_composite (&dest[(x-sx1) * bytes],
                             vcolor, bytes, blend, alpha);
            }

          if ((x_offset + (iwidth / 2)) % vspace < iwidth
              &&
              ((y_offset % hspace >= ispace
                &&
                y_offset % hspace < ioffset)
               ||
               (hspace -
                (y_offset % hspace) >= ispace
                &&
                hspace -
                (y_offset % hspace) < ioffset)))
            {
              pix_composite (&dest[(x-sx1) * bytes],
                             icolor, bytes, blend, alpha);
            }
        }

      if ((y_offset + (iwidth / 2)) % hspace < iwidth)
        {
          for (x = sx1; x < sx2; x++)
            {
              x_offset = vspace + x - voffset;
              while (x_offset < 0)
                x_offset += vspace;

              if ((x_offset % vspace >= ispace
                   &&
                   x_offset % vspace < ioffset)
                  ||
                  (vspace -
                   (x_offset % vspace) >= ispace
                   &&
                   vspace -
                   (x_offset % vspace) < ioffset))
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
            gimp_progress_update ((gdouble) y / (gdouble) (sy2 - sy1));
        }
    }

  g_free (dest);

  g_object_unref (src_buffer);

  if (preview)
    {
      gimp_preview_draw_buffer (preview, buffer, bytes * (sx2 - sx1));
      g_free (buffer);
    }
  else
    {
      gimp_progress_update (1.0);

      g_object_unref (dest_buffer);

      gimp_drawable_merge_shadow (drawable, TRUE);
      gimp_drawable_update (drawable,
                            sx1, sy1, sx2 - sx1, sy2 - sy1);
    }
}


/***************************************************
 * GUI stuff
 */

static void
update_preview (GimpPreview *preview,
                GObject     *config)
{
  GimpDrawable *drawable = g_object_get_data (config, "drawable");

  render_grid (gimp_item_get_image (GIMP_ITEM (drawable)), drawable,
               GIMP_PROCEDURE_CONFIG (config), preview);
}

static void
color_callback (GObject          *config,
                const GParamSpec *pspec,
                gpointer          data)
{
  Grid      *grid         = GRID (data);
  GtkWidget *chain_button = grid->color_chain;
  GeglColor *color        = NULL;

  g_signal_handlers_block_by_func (config, color_callback, grid);

  if (gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (chain_button)))
    {
      if (! strcmp (pspec->name, "vcolor"))
        {
          g_object_get (config, "vcolor", &color, NULL);
          g_object_set (config, "hcolor", color, NULL);
        }
      else if (! strcmp (pspec->name, "hcolor"))
        {
          g_object_get (config, "hcolor", &color, NULL);
          g_object_set (config, "vcolor", color, NULL);
        }

      g_clear_object (&color);
    }

  g_signal_handlers_unblock_by_func (config, color_callback, grid);
}


static gint
dialog (GimpImage           *image,
        GimpDrawable        *drawable,
        GimpProcedure       *procedure,
        GimpProcedureConfig *config)
{
  Grid            *grid;
  GtkWidget       *dlg;
  GtkWidget       *hbox;
  GtkWidget       *label;
  GtkWidget       *preview;
  GtkWidget       *size_entry;
  GtkWidget       *color_button;
  GtkWidget       *color_grid;
  GeglColor       *hcolor;
  GeglColor       *vcolor;
  gdouble          xres;
  gdouble          yres;
  gboolean         run;

  g_return_val_if_fail (main_dialog == NULL, FALSE);

  gimp_ui_init (PLUG_IN_BINARY);

  grid           = GRID (gimp_procedure_get_plug_in (procedure));
  grid->drawable = drawable;
  grid->config   = config;

  g_object_get (config,
                "hcolor", &hcolor,
                "vcolor", &vcolor,
                NULL);

  main_dialog = dlg = gimp_procedure_dialog_new (procedure,
                                                 GIMP_PROCEDURE_CONFIG (config),
                                                 _("Grid"));

  /*  Get the image resolution and unit  */
  gimp_image_get_resolution (image, &xres, &yres);

  preview = gimp_procedure_dialog_get_drawable_preview (GIMP_PROCEDURE_DIALOG (dlg),
                                                        "preview", drawable);
  g_object_set_data (G_OBJECT (config), "drawable", drawable);

  g_signal_connect (preview, "invalidated",
                    G_CALLBACK (update_preview),
                    config);

  g_signal_connect_swapped (config, "notify",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /* Create entries for line width, spacing, and offset */
  gimp_procedure_dialog_get_coordinates (GIMP_PROCEDURE_DIALOG (dlg),
                                         "width-coordinates", "hwidth",
                                         "vwidth", "width-unit",
                                         "%a", GIMP_SIZE_ENTRY_UPDATE_SIZE,
                                         xres, yres);
  size_entry = gimp_procedure_dialog_get_size_entry (GIMP_PROCEDURE_DIALOG (dlg),
                                                     "iwidth", TRUE,
                                                     "intersection-width-unit",
                                                     "%a",
                                                     GIMP_SIZE_ENTRY_UPDATE_SIZE,
                                                     xres);
  label = gtk_grid_get_child_at (GTK_GRID (size_entry), 0, 1);
  gtk_widget_set_visible (label, FALSE);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (size_entry),
                                _("Intersection width"), 0, 1, 0.0);


  gimp_procedure_dialog_get_coordinates (GIMP_PROCEDURE_DIALOG (dlg),
                                         "space-coordinates", "hspace",
                                         "vspace", "space-unit",
                                         "%a", GIMP_SIZE_ENTRY_UPDATE_SIZE,
                                         xres, yres);
  size_entry = gimp_procedure_dialog_get_size_entry (GIMP_PROCEDURE_DIALOG (dlg),
                                                     "ispace", TRUE,
                                                     "intersection-space-unit",
                                                     "%a",
                                                     GIMP_SIZE_ENTRY_UPDATE_SIZE,
                                                     xres);
  label = gtk_grid_get_child_at (GTK_GRID (size_entry), 0, 1);
  gtk_widget_set_visible (label, FALSE);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (size_entry),
                                _("Intersection spacing"), 0, 1, 0.0);

  gimp_procedure_dialog_get_coordinates (GIMP_PROCEDURE_DIALOG (dlg),
                                         "offset-coordinates", "hoffset",
                                         "voffset", "offset-unit",
                                         "%a", GIMP_SIZE_ENTRY_UPDATE_SIZE,
                                         xres, yres);
  size_entry = gimp_procedure_dialog_get_size_entry (GIMP_PROCEDURE_DIALOG (dlg),
                                                     "ioffset", TRUE,
                                                     "intersection-offset-unit",
                                                     "%a",
                                                     GIMP_SIZE_ENTRY_UPDATE_SIZE,
                                                     xres);
  label = gtk_grid_get_child_at (GTK_GRID (size_entry), 0, 1);
  gtk_widget_set_visible (label, FALSE);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (size_entry),
                                _("Intersection offset"), 0, 1, 0.0);

  /* Create color widgets */
  grid->hcolor_button =
    gimp_procedure_dialog_get_color_widget (GIMP_PROCEDURE_DIALOG (dlg),
                                            "hcolor", TRUE,
                                            GIMP_COLOR_AREA_SMALL_CHECKS);
  gtk_widget_set_visible (gimp_labeled_get_label (GIMP_LABELED (grid->hcolor_button)),
                          FALSE);
  color_button = gimp_label_color_get_color_widget (GIMP_LABEL_COLOR (grid->hcolor_button));
  g_object_set (color_button, "area-width", COLOR_BUTTON_WIDTH, NULL);

  grid->vcolor_button =
    gimp_procedure_dialog_get_color_widget (GIMP_PROCEDURE_DIALOG (dlg),
                                            "vcolor", TRUE,
                                            GIMP_COLOR_AREA_SMALL_CHECKS);
  gtk_widget_set_visible (gimp_labeled_get_label (GIMP_LABELED (grid->vcolor_button)),
                          FALSE);
  color_button = gimp_label_color_get_color_widget (GIMP_LABEL_COLOR (grid->vcolor_button));
  g_object_set (color_button, "area-width", COLOR_BUTTON_WIDTH, NULL);

  color_button =
    gimp_procedure_dialog_get_color_widget (GIMP_PROCEDURE_DIALOG (dlg),
                                            "icolor", TRUE,
                                            GIMP_COLOR_AREA_SMALL_CHECKS);
  color_button = gimp_label_color_get_color_widget (GIMP_LABEL_COLOR (color_button));
  g_object_set (color_button, "area-width", COLOR_BUTTON_WIDTH, NULL);

  /* Create layout */
  hbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dlg),
                                         "width-hbox", "width-coordinates",
                                         "iwidth", NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (hbox),
                                  GTK_ORIENTATION_HORIZONTAL);

  hbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dlg),
                                         "space-hbox", "space-coordinates",
                                         "ispace", NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (hbox),
                                  GTK_ORIENTATION_HORIZONTAL);

  hbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dlg),
                                         "offset-hbox", "offset-coordinates",
                                         "ioffset", NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (hbox),
                                  GTK_ORIENTATION_HORIZONTAL);

  hbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dlg),
                                         "color-hbox", "icolor", NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (hbox),
                                  GTK_ORIENTATION_HORIZONTAL);

  /* Create chain for color button */
  color_grid        = gtk_grid_new ();
  grid->color_chain = gimp_chain_button_new (GIMP_CHAIN_BOTTOM);
  if (gimp_color_is_perceptually_identical (hcolor, vcolor))
    gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (grid->color_chain), TRUE);
  g_clear_object (&hcolor);
  g_clear_object (&vcolor);

  gtk_grid_attach (GTK_GRID (color_grid), grid->color_chain, 0, 1, 2, 1);
  gtk_widget_set_visible (grid->color_chain, TRUE);

  gtk_grid_attach (GTK_GRID (color_grid), grid->hcolor_button, 0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (color_grid), grid->vcolor_button, 1, 0, 1, 1);
  gtk_widget_set_visible (color_grid, TRUE);

  gtk_box_pack_start (GTK_BOX (hbox), color_grid, FALSE, FALSE, 12);
  gtk_box_reorder_child (GTK_BOX (hbox), color_grid, 0);

  g_signal_connect (config, "notify::hcolor",
                    G_CALLBACK (color_callback),
                    grid);
  g_signal_connect (config, "notify::vcolor",
                    G_CALLBACK (color_callback),
                    grid);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dlg),
                              "preview", "width-hbox",
                              "space-hbox", "offset-hbox",
                              "color-hbox", NULL);

  gtk_widget_show (dlg);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dlg));

  gtk_widget_destroy (dlg);

  return run;
}
