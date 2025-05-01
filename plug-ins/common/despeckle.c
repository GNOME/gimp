/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpcolor/gimpcolor-private.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


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
  GimpPlugIn parent_instance;
};

struct _DespeckleClass
{
  GimpPlugInClass parent_class;
};


#define DESPECKLE_TYPE  (despeckle_get_type ())
#define DESPECKLE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), DESPECKLE_TYPE, Despeckle))

GType                   despeckle_get_type         (void) G_GNUC_CONST;

static GList          * despeckle_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * despeckle_create_procedure (GimpPlugIn           *plug_in,
                                                    const gchar          *name);

static GimpValueArray * despeckle_run              (GimpProcedure        *procedure,
                                                    GimpRunMode           run_mode,
                                                    GimpImage            *image,
                                                    GimpDrawable        **drawables,
                                                    GimpProcedureConfig  *config,
                                                    gpointer              run_data);

static gboolean         despeckle                  (GimpDrawable         *drawable,
                                                    GObject              *config,
                                                    GError              **error);
static void             despeckle_median           (GObject              *config,
                                                    guchar               *src,
                                                    guchar               *dst,
                                                    gint                  width,
                                                    gint                  height,
                                                    gint                  bpp,
                                                    gboolean              preview);

static gboolean         despeckle_dialog           (GimpProcedure        *procedure,
                                                    GObject              *config,
                                                    GimpDrawable         *drawable);

static void             preview_update             (GtkWidget            *preview,
                                                    GObject              *config);


G_DEFINE_TYPE (Despeckle, despeckle, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (DESPECKLE_TYPE)
DEFINE_STD_SET_I18N


/* Number of pixels in actual histogram falling into each category */
static gint                hist0;    /* Less than min threshold */
static gint                hist255;  /* More than max threshold */
static gint                histrest; /* From min to max        */

static DespeckleHistogram  histogram;


static void
despeckle_class_init (DespeckleClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = despeckle_query_procedures;
  plug_in_class->create_procedure = despeckle_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
despeckle_init (Despeckle *despeckle)
{
}

static GList *
despeckle_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
despeckle_create_procedure (GimpPlugIn  *plug_in,
                            const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            despeckle_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB*, GRAY*");
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

      gimp_procedure_set_menu_label (procedure, _("Des_peckle..."));
      gimp_procedure_add_menu_path (procedure, "<Image>/Filters/Enhance");

      gimp_procedure_set_documentation (procedure,
                                        _("Remove speckle noise from the "
                                          "image"),
                                        _("This plug-in selectively performs "
                                          "a median or adaptive box filter on "
                                          "an image."),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Michael Sweet <mike@easysw.com>",
                                      "Copyright 1997-1998 by Michael Sweet",
                                      PLUG_IN_VERSION);

      gimp_procedure_add_int_argument (procedure, "radius",
                                       _("R_adius"),
                                       _("Filter box radius"),
                                       1, MAX_RADIUS, 3,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "type",
                                          _("_Filter Type"),
                                          _("Filter type"),
                                          gimp_choice_new_with_values ("median",             0,                 _("Median"),             NULL,
                                                                       "adaptive",           FILTER_ADAPTIVE,   _("Adaptive"),           NULL,
                                                                       "recursive-median",   FILTER_RECURSIVE, _("Recursive-Median"),   NULL,
                                                                       "recursive-adaptive", 3,                 _("Recursive-Adaptive"), NULL,
                                                                       NULL),
                                          "adaptive",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "black",
                                       _("_Black level"),
                                       _("Black level"),
                                       -1, 255, 7,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "white",
                                       _("_White level"),
                                       _("White level"),
                                       0, 256, 248,
                                       G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
despeckle_run (GimpProcedure        *procedure,
               GimpRunMode           run_mode,
               GimpImage            *image,
               GimpDrawable        **drawables,
               GimpProcedureConfig  *config,
               gpointer              run_data)
{
  GimpDrawable *drawable;
  GError       *error = NULL;

  gegl_init (NULL, NULL);

  if (gimp_core_object_array_get_length ((GObject **) drawables) != 1)
    {
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

  if (! gimp_drawable_is_rgb (drawable) && ! gimp_drawable_is_gray (drawable))
    return gimp_procedure_new_return_values (procedure, GIMP_PDB_EXECUTION_ERROR, NULL);

  if (run_mode == GIMP_RUN_INTERACTIVE && ! despeckle_dialog (procedure, G_OBJECT (config), drawable))
    return gimp_procedure_new_return_values (procedure, GIMP_PDB_CANCEL, NULL);

  if (! despeckle (drawable, G_OBJECT (config), &error))
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               error);

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
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
      return GIMP_RGB_LUMINANCE (p[0], p[1], p[2]);

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
get_u8_format (GimpDrawable *drawable)
{
  if (gimp_drawable_is_rgb (drawable))
    {
      if (gimp_drawable_has_alpha (drawable))
        return babl_format ("R'G'B'A u8");
      else
        return babl_format ("R'G'B' u8");
    }
  else
    {
      if (gimp_drawable_has_alpha (drawable))
        return babl_format ("Y'A u8");
      else
        return babl_format ("Y' u8");
    }
}

static gboolean
despeckle (GimpDrawable *drawable,
           GObject      *config,
           GError      **error)
{
  GeglBuffer *src_buffer;
  GeglBuffer *dest_buffer;
  const Babl *format;
  guchar     *src;
  guchar     *dst;
  gint        img_bpp;
  gint        x, y;
  gint        width, height;
  gsize       bufsize = 0;

  if (! gimp_drawable_mask_intersect (drawable,
                                      &x, &y, &width, &height))
    return TRUE;

  format  = get_u8_format (drawable);
  img_bpp = babl_format_get_bytes_per_pixel (format);

  src_buffer  = gimp_drawable_get_buffer (drawable);
  dest_buffer = gimp_drawable_get_shadow_buffer (drawable);

  if (! g_size_checked_mul (&bufsize, width,   height) ||
      ! g_size_checked_mul (&bufsize, bufsize, img_bpp))
    {
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   _("Image dimensions too large: width %d x height %d"),
                   width, height);
      return FALSE;
    }

  src = g_try_malloc (bufsize);
  dst = g_try_malloc (bufsize);

  if (src == NULL || dst == NULL)
    {
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   _("There was not enough memory to complete the operation."));
      g_free (src);

      return FALSE;
    }

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

  gimp_drawable_merge_shadow (drawable, TRUE);
  gimp_drawable_update (drawable, x, y, width, height);

  g_free (dst);
  g_free (src);

  return TRUE;
}

static gboolean
despeckle_dialog (GimpProcedure *procedure,
                  GObject       *config,
                  GimpDrawable  *drawable)
{
  GtkWidget *dialog;
  GtkWidget *preview;
  GtkWidget *vbox;
  GtkWidget *scale;
  GtkWidget *combo;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Despeckle"));

  combo = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                            "type", G_TYPE_NONE);
  gtk_widget_set_margin_bottom (combo, 12);

  /*
   * Box size (diameter) control...
   */
  scale = gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                                "radius", 1.0);
  gtk_widget_set_margin_bottom (scale, 12);
  /*
   * Black level control...
   */
  scale = gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                                 "black", 1.0);
  gtk_widget_set_margin_bottom (scale, 12);
  /*
   * White level control...
   */
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "white", 1.0);

  vbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                         "despeckle-vbox",
                                         "type", "radius", "black", "white",
                                         NULL);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);

  preview = gimp_procedure_dialog_get_drawable_preview (GIMP_PROCEDURE_DIALOG (dialog),
                                                        "preview", drawable);
  gtk_widget_set_margin_bottom (preview, 12);

  g_object_set_data (config, "drawable", drawable);

  g_signal_connect (preview, "invalidated",
                    G_CALLBACK (preview_update),
                    config);

  g_signal_connect_swapped (config, "notify",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              "preview", "despeckle-vbox",
                              NULL);
  gtk_widget_show (dialog);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}

static void
preview_update (GtkWidget *widget,
                GObject   *config)
{
  GimpPreview  *preview  = GIMP_PREVIEW (widget);
  GimpDrawable *drawable = g_object_get_data (config, "drawable");
  gsize         bufsize  = 0;
  GeglBuffer   *src_buffer;
  const Babl   *format;
  guchar       *dst;
  guchar       *src;
  gint          img_bpp;
  gint          x1,y1;
  gint          width, height;

  format  = get_u8_format (drawable);
  img_bpp = babl_format_get_bytes_per_pixel (format);

  gimp_preview_get_size (preview, &width, &height);
  gimp_preview_get_position (preview, &x1, &y1);

  src_buffer = gimp_drawable_get_buffer (drawable);

  if (! g_size_checked_mul (&bufsize, width,   height) ||
      ! g_size_checked_mul (&bufsize, bufsize, img_bpp))
      return;

  src = g_try_malloc (bufsize);
  dst = g_try_malloc (bufsize);

  if (src == NULL || dst == NULL)
    {
      g_free (src);
      return;
    }

  gegl_buffer_get (src_buffer, GEGL_RECTANGLE (x1, y1, width, height), 1.0,
                   format, src,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  despeckle_median (config,
                    src, dst, width, height, img_bpp, TRUE);

  gimp_preview_draw_buffer (preview, dst, width * img_bpp);

  g_object_unref (src_buffer);

  g_free (src);
  g_free (dst);
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
                "black",  &black_level,
                "white",  &white_level,
                NULL);
  filter_type = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                                     "type");

  memset (&histogram, 0, sizeof(histogram));
  progress     = 0;
  max_progress = width * height;

  if (! preview)
    gimp_progress_init (_("Despeckle"));

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
        gimp_progress_update ((gdouble) progress / (gdouble) max_progress);
    }

  if (! preview)
    gimp_progress_update (1.0);
}
