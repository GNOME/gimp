/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Despeckle (adaptive median) filter
 *
 * Copyright 1997-1998 Michael Sweet (mike@easysw.com)
 * optimized in 2010 by Przemyslaw Zych (kermidt.zed@gmail.com)
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

#include "config.h"

#include <stdlib.h>

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "libligma/stdplugins-intl.h"


/*
 * Constants...
 */

#define PLUG_IN_PROC     "plug-in-despeckle"
#define PLUG_IN_BINARY   "despeckle"
#define PLUG_IN_VERSION  "May 2010"
#define SCALE_WIDTH      100
#define ENTRY_WIDTH        3
#define MAX_RADIUS        30

#define FILTER_ADAPTIVE  0x01
#define FILTER_RECURSIVE 0x02

/* List that stores pixels falling in to the same luma bucket */
#define MAX_LIST_ELEMS   SQR(2 * MAX_RADIUS + 1)


typedef struct
{
  const guchar *elems[MAX_LIST_ELEMS];
  gint          start;
  gint          count;
} PixelsList;

typedef struct
{
  gint       elems[256]; /* Number of pixels that fall into each luma bucket */
  PixelsList origs[256]; /* Original pixels */
  gint       xmin;
  gint       ymin;
  gint       xmax;
  gint       ymax; /* Source rect */
} DespeckleHistogram;


typedef struct _Despeckle      Despeckle;
typedef struct _DespeckleClass DespeckleClass;

struct _Despeckle
{
  LigmaPlugIn parent_instance;
};

struct _DespeckleClass
{
  LigmaPlugInClass parent_class;
};


#define DESPECKLE_TYPE  (despeckle_get_type ())
#define DESPECKLE (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), DESPECKLE_TYPE, Despeckle))

GType                   despeckle_get_type         (void) G_GNUC_CONST;

static GList          * despeckle_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * despeckle_create_procedure (LigmaPlugIn           *plug_in,
                                                    const gchar          *name);

static LigmaValueArray * despeckle_run              (LigmaProcedure        *procedure,
                                                    LigmaRunMode           run_mode,
                                                    LigmaImage            *image,
                                                    gint                  n_drawables,
                                                    LigmaDrawable        **drawables,
                                                    const LigmaValueArray *args,
                                                    gpointer              run_data);

static void             despeckle                  (LigmaDrawable         *drawable,
                                                    GObject              *config);
static void             despeckle_median           (GObject              *config,
                                                    guchar               *src,
                                                    guchar               *dst,
                                                    gint                  width,
                                                    gint                  height,
                                                    gint                  bpp,
                                                    gboolean              preview);

static gboolean         despeckle_dialog           (LigmaProcedure        *procedure,
                                                    GObject              *config,
                                                    LigmaDrawable         *drawable);

static void             dialog_adaptive_callback   (GtkWidget            *button,
                                                    GObject              *config);
static void             dialog_adaptive_notify     (GObject              *config,
                                                    const GParamSpec     *pspec,
                                                    GtkWidget            *button);
static void             dialog_recursive_callback  (GtkWidget            *button,
                                                    GObject              *config);
static void             dialog_recursive_notify    (GObject              *config,
                                                    const GParamSpec     *pspec,
                                                    GtkWidget            *button);

static void             preview_update             (GtkWidget            *preview,
                                                    GObject              *config);


G_DEFINE_TYPE (Despeckle, despeckle, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (DESPECKLE_TYPE)
DEFINE_STD_SET_I18N


/* Number of pixels in actual histogram falling into each category */
static gint                hist0;    /* Less than min threshold */
static gint                hist255;  /* More than max threshold */
static gint                histrest; /* From min to max        */

static DespeckleHistogram  histogram;


static void
despeckle_class_init (DespeckleClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = despeckle_query_procedures;
  plug_in_class->create_procedure = despeckle_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
despeckle_init (Despeckle *despeckle)
{
}

static GList *
despeckle_query_procedures (LigmaPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static LigmaProcedure *
despeckle_create_procedure (LigmaPlugIn  *plug_in,
                            const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = ligma_image_procedure_new (plug_in, name,
                                            LIGMA_PDB_PROC_TYPE_PLUGIN,
                                            despeckle_run, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "RGB*, GRAY*");
      ligma_procedure_set_sensitivity_mask (procedure,
                                           LIGMA_PROCEDURE_SENSITIVE_DRAWABLE);

      ligma_procedure_set_menu_label (procedure, _("Des_peckle..."));
      ligma_procedure_add_menu_path (procedure, "<Image>/Filters/Enhance");

      ligma_procedure_set_documentation (procedure,
                                        _("Remove speckle noise from the "
                                          "image"),
                                        "This plug-in selectively performs "
                                        "a median or adaptive box filter on "
                                        "an image.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Michael Sweet <mike@easysw.com>",
                                      "Copyright 1997-1998 by Michael Sweet",
                                      PLUG_IN_VERSION);

      LIGMA_PROC_ARG_INT (procedure, "radius",
                         "Radius",
                         "Filter box radius",
                         1, MAX_RADIUS, 3,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "type",
                         "Type",
                         "Filter type { MEDIAN (0), ADAPTIVE (1), "
                         "RECURSIVE-MEDIAN (2), RECURSIVE-ADAPTIVE (3) }",
                         0, 3, FILTER_ADAPTIVE,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "black",
                         "Black",
                         "Black level",
                         -1, 255, 7,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "white",
                         "White",
                         "White level",
                         0, 256, 248,
                         G_PARAM_READWRITE);
    }

  return procedure;
}

static LigmaValueArray *
despeckle_run (LigmaProcedure        *procedure,
               LigmaRunMode           run_mode,
               LigmaImage            *image,
               gint                  n_drawables,
               LigmaDrawable        **drawables,
               const LigmaValueArray *args,
               gpointer              run_data)
{
  LigmaProcedureConfig *config;
  LigmaDrawable        *drawable;

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

  if (! ligma_drawable_is_rgb  (drawable) &&
      ! ligma_drawable_is_gray (drawable))
    {
      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_EXECUTION_ERROR,
                                               NULL);
    }

  config = ligma_procedure_create_config (procedure);
  ligma_procedure_config_begin_run (config, NULL, run_mode, args);

  if (run_mode == LIGMA_RUN_INTERACTIVE)
    {
      if (! despeckle_dialog (procedure, G_OBJECT (config), drawable))
        {
          return ligma_procedure_new_return_values (procedure,
                                                   LIGMA_PDB_CANCEL,
                                                   NULL);
        }
    }

  despeckle (drawable, G_OBJECT (config));

  ligma_procedure_config_end_run (config, LIGMA_PDB_SUCCESS);
  g_object_unref (config);

  return ligma_procedure_new_return_values (procedure, LIGMA_PDB_SUCCESS, NULL);
}

static inline guchar
pixel_luminance (const guchar *p,
                 gint          bpp)
{
  switch (bpp)
    {
    case 1:
    case 2:
      return p[0];

    case 3:
    case 4:
      return LIGMA_RGB_LUMINANCE (p[0], p[1], p[2]);

    default:
      return 0; /* should not be reached */
    }
}

static inline void
pixel_copy (guchar       *dest,
            const guchar *src,
            gint          bpp)
{
  switch (bpp)
    {
    case 4:
      *dest++ = *src++;
    case 3:
      *dest++ = *src++;
    case 2:
      *dest++ = *src++;
    case 1:
      *dest++ = *src++;
    }
}

/*
 * 'despeckle()' - Despeckle an image using a median filter.
 *
 * A median filter basically collects pixel values in a region around the
 * target pixel, sorts them, and uses the median value. This code uses a
 * circular row buffer to improve performance.
 *
 * The adaptive filter is based on the median filter but analyzes the histogram
 * of the region around the target pixel and adjusts the despeckle diameter
 * accordingly.
 */

static const Babl *
get_u8_format (LigmaDrawable *drawable)
{
  if (ligma_drawable_is_rgb (drawable))
    {
      if (ligma_drawable_has_alpha (drawable))
        return babl_format ("R'G'B'A u8");
      else
        return babl_format ("R'G'B' u8");
    }
  else
    {
      if (ligma_drawable_has_alpha (drawable))
        return babl_format ("Y'A u8");
      else
        return babl_format ("Y' u8");
    }
}

static void
despeckle (LigmaDrawable *drawable,
           GObject      *config)
{
  GeglBuffer *src_buffer;
  GeglBuffer *dest_buffer;
  const Babl *format;
  guchar     *src;
  guchar     *dst;
  gint        img_bpp;
  gint        x, y;
  gint        width, height;

  if (! ligma_drawable_mask_intersect (drawable,
                                      &x, &y, &width, &height))
    return;

  format  = get_u8_format (drawable);
  img_bpp = babl_format_get_bytes_per_pixel (format);

  src_buffer  = ligma_drawable_get_buffer (drawable);
  dest_buffer = ligma_drawable_get_shadow_buffer (drawable);

  src = g_new (guchar, width * height * img_bpp);
  dst = g_new (guchar, width * height * img_bpp);

  gegl_buffer_get (src_buffer, GEGL_RECTANGLE (x, y, width, height), 1.0,
                   format, src,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  despeckle_median (config,
                    src, dst, width, height, img_bpp, FALSE);

  gegl_buffer_set (dest_buffer, GEGL_RECTANGLE (x, y, width, height), 0,
                   format, dst,
                   GEGL_AUTO_ROWSTRIDE);

  g_object_unref (src_buffer);
  g_object_unref (dest_buffer);

  ligma_drawable_merge_shadow (drawable, TRUE);
  ligma_drawable_update (drawable, x, y, width, height);

  g_free (dst);
  g_free (src);
}

static gboolean
despeckle_dialog (LigmaProcedure *procedure,
                  GObject       *config,
                  LigmaDrawable  *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *preview;
  GtkWidget *vbox;
  GtkWidget *grid;
  GtkWidget *frame;
  GtkWidget *button;
  GtkWidget *scale;
  gint       filter_type;
  gboolean   run;

  ligma_ui_init (PLUG_IN_BINARY);

  dialog = ligma_procedure_dialog_new (procedure,
                                      LIGMA_PROCEDURE_CONFIG (config),
                                      _("Despeckle"));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  preview = ligma_drawable_preview_new_from_drawable (drawable);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);

  g_object_set_data (config, "drawable", drawable);

  g_signal_connect (preview, "invalidated",
                    G_CALLBACK (preview_update),
                    config);

  frame = ligma_frame_new (_("Median"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  g_object_get (config, "type", &filter_type, NULL);

  button = gtk_check_button_new_with_mnemonic (_("_Adaptive"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                filter_type & FILTER_ADAPTIVE);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (dialog_adaptive_callback),
                    config);
  g_signal_connect (config, "notify::type",
                    G_CALLBACK (dialog_adaptive_notify),
                    button);

  button = gtk_check_button_new_with_mnemonic (_("R_ecursive"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                filter_type & FILTER_RECURSIVE);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (dialog_recursive_callback),
                    config);
  g_signal_connect (config, "notify::type",
                    G_CALLBACK (dialog_recursive_notify),
                    button);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  /*
   * Box size (diameter) control...
   */

  scale = ligma_prop_scale_entry_new (config, "radius",
                                     _("_Radius:"),
                                     1.0, FALSE, 0, 0);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, 0, 1, 1);
  gtk_widget_show (scale);

  /*
   * Black level control...
   */

  scale = ligma_prop_scale_entry_new (config, "black",
                                     _("_Black level:"),
                                     1.0, FALSE, 0, 0);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, 1, 1, 1);
  gtk_widget_show (scale);

  /*
   * White level control...
   */

  scale = ligma_prop_scale_entry_new (config, "white",
                                     _("_White level:"),
                                     1.0, FALSE, 0, 0);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, 2, 1, 1);
  gtk_widget_show (scale);

  g_signal_connect_swapped (config, "notify",
                            G_CALLBACK (ligma_preview_invalidate),
                            preview);

  gtk_widget_show (dialog);

  run = ligma_procedure_dialog_run (LIGMA_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}

static void
preview_update (GtkWidget *widget,
                GObject   *config)
{
  LigmaPreview  *preview = LIGMA_PREVIEW (widget);
  LigmaDrawable *drawable = g_object_get_data (config, "drawable");
  GeglBuffer   *src_buffer;
  const Babl   *format;
  guchar       *dst;
  guchar       *src;
  gint          img_bpp;
  gint          x1,y1;
  gint          width, height;

  format  = get_u8_format (drawable);
  img_bpp = babl_format_get_bytes_per_pixel (format);

  ligma_preview_get_size (preview, &width, &height);
  ligma_preview_get_position (preview, &x1, &y1);

  src_buffer = ligma_drawable_get_buffer (drawable);

  dst = g_new (guchar, width * height * img_bpp);
  src = g_new (guchar, width * height * img_bpp);

  gegl_buffer_get (src_buffer, GEGL_RECTANGLE (x1, y1, width, height), 1.0,
                   format, src,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  despeckle_median (config,
                    src, dst, width, height, img_bpp, TRUE);

  ligma_preview_draw_buffer (preview, dst, width * img_bpp);

  g_object_unref (src_buffer);

  g_free (src);
  g_free (dst);
}

static void
dialog_adaptive_callback (GtkWidget *button,
                          GObject   *config)
{
  gint filter_type;

  g_object_get (config, "type", &filter_type, NULL);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    filter_type |= FILTER_ADAPTIVE;
  else
    filter_type &= ~FILTER_ADAPTIVE;

  g_signal_handlers_block_by_func (config,
                                   dialog_adaptive_notify,
                                   button);

  g_object_set (config, "type", filter_type, NULL);

  g_signal_handlers_unblock_by_func (config,
                                     dialog_adaptive_notify,
                                     button);
}

static void
dialog_adaptive_notify (GObject          *config,
                        const GParamSpec *pspec,
                        GtkWidget        *button)
{
  gint filter_type;

  g_object_get (config, "type", &filter_type, NULL);

  g_signal_handlers_block_by_func (button,
                                   dialog_adaptive_callback,
                                   config);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                (filter_type & FILTER_ADAPTIVE) != 0);

  g_signal_handlers_unblock_by_func (button,
                                     dialog_adaptive_callback,
                                     config);
}

static void
dialog_recursive_callback (GtkWidget *button,
                           GObject   *config)
{
  gint filter_type;

  g_object_get (config, "type", &filter_type, NULL);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    filter_type |= FILTER_RECURSIVE;
  else
    filter_type &= ~FILTER_RECURSIVE;

  g_signal_handlers_block_by_func (config,
                                   dialog_recursive_notify,
                                   button);

  g_object_set (config, "type", filter_type, NULL);

  g_signal_handlers_unblock_by_func (config,
                                     dialog_recursive_notify,
                                     button);
}

static void
dialog_recursive_notify (GObject          *config,
                         const GParamSpec *pspec,
                         GtkWidget        *button)
{
  gint filter_type;

  g_object_get (config, "type", &filter_type, NULL);

  g_signal_handlers_block_by_func (button,
                                   dialog_recursive_callback,
                                   config);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                (filter_type & FILTER_RECURSIVE) != 0);

  g_signal_handlers_unblock_by_func (button,
                                     dialog_recursive_callback,
                                     config);
}

static inline void
list_add_elem (PixelsList   *list,
               const guchar *elem)
{
  const gint pos = list->start + list->count++;

  list->elems[pos >= MAX_LIST_ELEMS ? pos - MAX_LIST_ELEMS : pos] = elem;
}

static inline void
list_del_elem (PixelsList* list)
{
  list->count--;
  list->start++;

  if (list->start >= MAX_LIST_ELEMS)
    list->start = 0;
}

static inline const guchar *
list_get_random_elem (PixelsList *list)
{
  const gint pos = list->start + rand () % list->count;

  if (pos >= MAX_LIST_ELEMS)
    return list->elems[pos - MAX_LIST_ELEMS];

  return list->elems[pos];
}

static inline void
histogram_add (DespeckleHistogram *hist,
               guchar              val,
               const guchar       *orig)
{
  hist->elems[val]++;
  list_add_elem (&hist->origs[val], orig);
}

static inline void
histogram_remove (DespeckleHistogram *hist,
                  guchar              val)
{
  hist->elems[val]--;
  list_del_elem (&hist->origs[val]);
}

static inline void
histogram_clean (DespeckleHistogram *hist)
{
  gint i;

  for (i = 0; i < 256; i++)
    {
      hist->elems[i] = 0;
      hist->origs[i].count = 0;
    }
}

static inline const guchar *
histogram_get_median (DespeckleHistogram *hist,
                      const guchar       *_default)
{
  gint count = histrest;
  gint i;
  gint sum = 0;

  if (! count)
    return _default;

  count = (count + 1) / 2;

  i = 0;
  while ((sum += hist->elems[i]) < count)
    i++;

  return list_get_random_elem (&hist->origs[i]);
}

static inline void
add_val (DespeckleHistogram *hist,
         gint                black_level,
         gint                white_level,
         const guchar       *src,
         gint                width,
         gint                bpp,
         gint                x,
         gint                y)
{
  const gint pos   = (x + (y * width)) * bpp;
  const gint value = pixel_luminance (src + pos, bpp);

  if (value > black_level && value < white_level)
    {
      histogram_add (hist, value, src + pos);
      histrest++;
    }
  else
    {
      if (value <= black_level)
        hist0++;

      if (value >= white_level)
        hist255++;
    }
}

static inline void
del_val (DespeckleHistogram *hist,
         gint                black_level,
         gint                white_level,
         const guchar       *src,
         gint                width,
         gint                bpp,
         gint                x,
         gint                y)
{
  const gint pos   = (x + (y * width)) * bpp;
  const gint value = pixel_luminance (src + pos, bpp);

  if (value > black_level && value < white_level)
    {
      histogram_remove (hist, value);
      histrest--;
    }
  else
    {
      if (value <= black_level)
        hist0--;

      if (value >= white_level)
        hist255--;
    }
}

static inline void
add_vals (DespeckleHistogram *hist,
          gint                black_level,
          gint                white_level,
          const guchar       *src,
          gint                width,
          gint                bpp,
          gint                xmin,
          gint                ymin,
          gint                xmax,
          gint                ymax)
{
  gint x;
  gint y;

  if (xmin > xmax)
    return;

  for (y = ymin; y <= ymax; y++)
    {
      for (x = xmin; x <= xmax; x++)
        {
          add_val (hist,
                   black_level, white_level,
                   src, width, bpp, x, y);
        }
    }
}

static inline void
del_vals (DespeckleHistogram *hist,
          gint                black_level,
          gint                white_level,
          const guchar       *src,
          gint                width,
          gint                bpp,
          gint                xmin,
          gint                ymin,
          gint                xmax,
          gint                ymax)
{
  gint x;
  gint y;

  if (xmin > xmax)
    return;

  for (y = ymin; y <= ymax; y++)
    {
      for (x = xmin; x <= xmax; x++)
        {
          del_val (hist,
                   black_level, white_level,
                   src, width, bpp, x, y);
        }
    }
}

static inline void
update_histogram (DespeckleHistogram *hist,
                  gint                black_level,
                  gint                white_level,
                  const guchar       *src,
                  gint                width,
                  gint                bpp,
                  gint                xmin,
                  gint                ymin,
                  gint                xmax,
                  gint                ymax)
{
  /* assuming that radious of the box can change no more than one
     pixel in each call */
  /* assuming that box is moving either right or down */

  del_vals (hist,
            black_level, white_level,
            src, width, bpp, hist->xmin, hist->ymin, xmin - 1, hist->ymax);
  del_vals (hist,
            black_level, white_level,
            src, width, bpp, xmin, hist->ymin, xmax, ymin - 1);
  del_vals (hist,
            black_level, white_level,
            src, width, bpp, xmin, ymax + 1, xmax, hist->ymax);

  add_vals (hist,
            black_level, white_level,
            src, width, bpp, hist->xmax + 1, ymin, xmax, ymax);
  add_vals (hist,
            black_level, white_level,
            src, width, bpp, xmin, ymin, hist->xmax, hist->ymin - 1);
  add_vals (hist,
            black_level, white_level,
            src, width, bpp, hist->xmin, hist->ymax + 1, hist->xmax, ymax);

  hist->xmin = xmin;
  hist->ymin = ymin;
  hist->xmax = xmax;
  hist->ymax = ymax;
}

static void
despeckle_median (GObject  *config,
                  guchar   *src,
                  guchar   *dst,
                  gint      width,
                  gint      height,
                  gint      bpp,
                  gboolean  preview)
{
  gint   radius;
  gint   filter_type;
  gint   black_level;
  gint   white_level;
  guint  progress;
  guint  max_progress;
  gint   x, y;
  gint   adapt_radius;
  gint   pos;
  gint   ymin;
  gint   ymax;
  gint   xmin;
  gint   xmax;

  g_object_get (config,
                "radius", &radius,
                "type",   &filter_type,
                "black",  &black_level,
                "white",  &white_level,
                NULL);

  memset (&histogram, 0, sizeof(histogram));
  progress     = 0;
  max_progress = width * height;

  if (! preview)
    ligma_progress_init (_("Despeckle"));

  adapt_radius = radius;
  for (y = 0; y < height; y++)
    {
      x = 0;
      ymin = MAX (0, y - adapt_radius);
      ymax = MIN (height - 1, y + adapt_radius);
      xmin = MAX (0, x - adapt_radius);
      xmax = MIN (width - 1, x + adapt_radius);
      hist0   = 0;
      histrest = 0;
      hist255 = 0;
      histogram_clean (&histogram);
      histogram.xmin = xmin;
      histogram.ymin = ymin;
      histogram.xmax = xmax;
      histogram.ymax = ymax;
      add_vals (&histogram,
                black_level, white_level,
                src, width, bpp,
                histogram.xmin, histogram.ymin,
                histogram.xmax, histogram.ymax);

      for (x = 0; x < width; x++)
        {
          const guchar *pixel;

          ymin = MAX (0, y - adapt_radius); /* update ymin, ymax when adapt_radius changed (FILTER_ADAPTIVE) */
          ymax = MIN (height - 1, y + adapt_radius);
          xmin = MAX (0, x - adapt_radius);
          xmax = MIN (width - 1, x + adapt_radius);

          update_histogram (&histogram,
                            black_level, white_level,
                            src, width, bpp, xmin, ymin, xmax, ymax);

          pos = (x + (y * width)) * bpp;
          pixel = histogram_get_median (&histogram, src + pos);

          if (filter_type & FILTER_RECURSIVE)
            {
              del_val (&histogram,
                       black_level, white_level,
                       src, width, bpp, x, y);

              pixel_copy (src + pos, pixel, bpp);

              add_val (&histogram,
                       black_level, white_level,
                       src, width, bpp, x, y);
            }

          pixel_copy (dst + pos, pixel, bpp);

          /*
           * Check the histogram and adjust the diameter accordingly...
           */
          if (filter_type & FILTER_ADAPTIVE)
            {
              if (hist0 >= adapt_radius || hist255 >= adapt_radius)
                {
                  if (adapt_radius < radius)
                    adapt_radius++;
                }
              else if (adapt_radius > 1)
                {
                  adapt_radius--;
                }
            }
        }

      progress += width;

      if (! preview && y % 32 == 0)
        ligma_progress_update ((gdouble) progress / (gdouble) max_progress);
    }

  if (! preview)
    ligma_progress_update (1.0);
}
