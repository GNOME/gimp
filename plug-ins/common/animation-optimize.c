/*
 * Animation Optimizer plug-in version 1.1.2
 *
 * (c) Adam D. Moss, 1997-2003
 *     adam@gimp.org
 *     adam@foxbox.org
 *
 * GIMP - The GNU Image Manipulation Program
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

/*
#define EXPERIMENTAL_BACKDROP_CODE
*/


#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"


#define OPTIMIZE_PROC        "plug-in-animationoptimize"
#define OPTIMIZE_DIFF_PROC   "plug-in-animationoptimize-diff"
#define UNOPTIMIZE_PROC      "plug-in-animationunoptimize"
#define REMOVE_BACKDROP_PROC "plug-in-animation-remove-backdrop"
#define FIND_BACKDROP_PROC   "plug-in-animation-find-backdrop"


typedef enum
{
  DISPOSE_UNDEFINED = 0x00,
  DISPOSE_COMBINE   = 0x01,
  DISPOSE_REPLACE   = 0x02
} DisposeType;


typedef enum
{
  OPOPTIMIZE   = 0L,
  OPUNOPTIMIZE = 1L,
  OPFOREGROUND = 2L,
  OPBACKGROUND = 3L
} operatingMode;


typedef struct _Optimize      Optimize;
typedef struct _OptimizeClass OptimizeClass;

struct _Optimize
{
  GimpPlugIn parent_instance;
};

struct _OptimizeClass
{
  GimpPlugInClass parent_class;
};


#define OPTIMIZE_TYPE  (optimize_get_type ())
#define OPTIMIZE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), OPTIMIZE_TYPE, Optimize))

GType                   optimize_get_type         (void) G_GNUC_CONST;

static GList          * optimize_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * optimize_create_procedure (GimpPlugIn           *plug_in,
                                                   const gchar          *name);

static GimpValueArray * optimize_run              (GimpProcedure        *procedure,
                                                   GimpRunMode           run_mode,
                                                   GimpImage            *image,
                                                   GimpDrawable        **drawables,
                                                   GimpProcedureConfig  *config,
                                                   gpointer              run_data);

static  GimpImage     * do_optimizations          (GimpRunMode  run_mode,
                                                   GimpImage   *image,
                                                   gboolean     diff_only);

/* tag util functions*/
static  gint            parse_ms_tag              (const gchar *str);
static  DisposeType     parse_disposal_tag        (const gchar *str);
static  DisposeType     get_frame_disposal        (guint        whichframe);
static  guint32         get_frame_duration        (guint        whichframe);
static  void            remove_disposal_tag       (gchar       *dest,
                                                   gchar       *src);
static  void            remove_ms_tag             (gchar       *dest,
                                                   gchar       *src);
static  gboolean        is_disposal_tag           (const gchar *str,
                                                   DisposeType *disposal,
                                                   gint        *taglength);
static  gboolean        is_ms_tag                 (const gchar *str,
                                                   gint        *duration,
                                                   gint        *taglength);


G_DEFINE_TYPE (Optimize, optimize, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (OPTIMIZE_TYPE)
DEFINE_STD_SET_I18N


/* Global widgets'n'stuff */
static  guint             width, height;
static  gint32            total_frames;
static  GimpLayer       **layers;
static  guchar            pixelstep;
static  operatingMode     opmode;


static void
optimize_class_init (OptimizeClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = optimize_query_procedures;
  plug_in_class->create_procedure = optimize_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
optimize_init (Optimize *optimize)
{
}

static GList *
optimize_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (OPTIMIZE_PROC));
  list = g_list_append (list, g_strdup (OPTIMIZE_DIFF_PROC));
  list = g_list_append (list, g_strdup (UNOPTIMIZE_PROC));
#ifdef EXPERIMENTAL_BACKDROP_CODE
  list = g_list_append (list, g_strdup (REMOVE_BACKDROP_PROC));
  list = g_list_append (list, g_strdup (FIND_BACKDROP_PROC));
#endif

  return list;
}

static GimpProcedure *
optimize_create_procedure (GimpPlugIn  *plug_in,
                           const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, OPTIMIZE_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            optimize_run, NULL, NULL);

      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE  |
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLES |
                                           GIMP_PROCEDURE_SENSITIVE_NO_DRAWABLES);

      gimp_procedure_set_menu_label (procedure, _("Optimize (for _GIF)"));

      gimp_procedure_set_documentation (procedure,
                                        _("Modify image to reduce size when "
                                          "saved as GIF animation"),
                                        "This procedure applies various "
                                        "optimizations to a GIMP layer-based "
                                        "animation in an attempt to reduce the "
                                        "final file size.  If a frame of the"
                                        "animation can use the 'combine' "
                                        "mode, this procedure attempts to "
                                        "maximize the number of adjacent "
                                        "pixels having the same color, which"
                                        "improves the compression for some "
                                        "image formats such as GIF or MNG.",
                                        name);
    }
  else if (! strcmp (name, OPTIMIZE_DIFF_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            optimize_run, NULL, NULL);

      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE  |
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLES |
                                           GIMP_PROCEDURE_SENSITIVE_NO_DRAWABLES);

      gimp_procedure_set_menu_label (procedure, _("_Optimize (Difference)"));

      gimp_procedure_set_documentation (procedure,
                                        _("Reduce file size where "
                                          "combining layers is possible"),
                                        "This procedure applies various "
                                        "optimizations to a GIMP layer-based "
                                        "animation in an attempt to reduce "
                                        "the final file size.  If a frame of "
                                        "the animation can use the 'combine' "
                                        "mode, this procedure uses a simple "
                                        "difference between the frames.",
                                        name);
    }
  else if (! strcmp (name, UNOPTIMIZE_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            optimize_run, NULL, NULL);

      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE  |
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLES |
                                           GIMP_PROCEDURE_SENSITIVE_NO_DRAWABLES);

      gimp_procedure_set_menu_label (procedure, _("_Unoptimize"));

      gimp_procedure_set_documentation (procedure,
                                        _("Remove optimization to make "
                                          "editing easier"),
                                        "This procedure 'simplifies' a GIMP "
                                        "layer-based animation that has been "
                                        "optimized for animation. This makes "
                                        "editing the animation much easier.",
                                        name);
    }
  else if (! strcmp (name, REMOVE_BACKDROP_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            optimize_run, NULL, NULL);

      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE  |
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLES |
                                           GIMP_PROCEDURE_SENSITIVE_NO_DRAWABLES);

      gimp_procedure_set_menu_label (procedure, _("_Remove Backdrop"));

      gimp_procedure_set_documentation (procedure,
                                        "This procedure attempts to remove "
                                        "the backdrop from a GIMP layer-based "
                                        "animation, leaving the foreground "
                                        "animation over transparency.",
                                        NULL,
                                        name);
    }
  else if (! strcmp (name, FIND_BACKDROP_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            optimize_run, NULL, NULL);

      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE  |
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLES |
                                           GIMP_PROCEDURE_SENSITIVE_NO_DRAWABLES);

      gimp_procedure_set_menu_label (procedure, _("_Find Backdrop"));

      gimp_procedure_set_documentation (procedure,
                                        "This procedure attempts to remove "
                                        "the foreground from a GIMP "
                                        "layer-based animation, leaving"
                                        " a one-layered image containing only "
                                        "the constant backdrop image.",
                                        NULL,
                                        name);
    }

  if (procedure)
    {
      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_add_menu_path (procedure, "<Image>/Filters/Animation");

      gimp_procedure_set_attribution (procedure,
                                      "Adam D. Moss <adam@gimp.org>",
                                      "Adam D. Moss <adam@gimp.org>",
                                      "1997-2003");

      gimp_procedure_add_image_return_value (procedure, "result",
                                             "Result",
                                             "Resulting image",
                                             FALSE,
                                             G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
optimize_run (GimpProcedure        *procedure,
              GimpRunMode           run_mode,
              GimpImage            *image,
              GimpDrawable        **drawables,
              GimpProcedureConfig  *config,
              gpointer              run_data)
{
  GimpValueArray *return_vals;
  const gchar    *name      = gimp_procedure_get_name (procedure);
  gboolean        diff_only = FALSE;

  gegl_init (NULL, NULL);

  if (! strcmp (name, OPTIMIZE_PROC))
    {
      opmode = OPOPTIMIZE;
    }
  else if (! strcmp (name, OPTIMIZE_DIFF_PROC))
    {
      opmode = OPOPTIMIZE;
      diff_only = TRUE;
    }
  else if (! strcmp (name, UNOPTIMIZE_PROC))
    {
      opmode = OPUNOPTIMIZE;
    }
  else if (strcmp (name, FIND_BACKDROP_PROC))
    {
      opmode = OPBACKGROUND;
    }
  else if (strcmp (name, REMOVE_BACKDROP_PROC))
    {
      opmode = OPFOREGROUND;
    }

  image = do_optimizations (run_mode, image, diff_only);

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush ();

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}



/* Rendering Functions */

static void
total_alpha (guchar  *imdata,
             guint32  numpix,
             guchar   bytespp)
{
  /* Set image to total-transparency w/black
   */

  memset (imdata, 0, numpix * bytespp);
}

static const Babl *
get_format (GimpDrawable *drawable)
{
  if (gimp_drawable_is_rgb (drawable))
    {
      if (gimp_drawable_has_alpha (drawable))
        return babl_format ("R'G'B'A u8");
      else
        return babl_format ("R'G'B' u8");
    }
  else if (gimp_drawable_is_gray (drawable))
    {
      if (gimp_drawable_has_alpha (drawable))
        return babl_format ("Y'A u8");
      else
        return babl_format ("Y' u8");
    }

  return gimp_drawable_get_format (drawable);
}

static void
compose_row (gint          frame_num,
             DisposeType   dispose,
             gint          row_num,
             guchar       *dest,
             gint          dest_width,
             GimpDrawable *drawable,
             gboolean      cleanup)
{
  static guchar *line_buf = NULL;
  GeglBuffer    *src_buffer;
  const Babl    *format;
  guchar        *srcptr;
  gint           rawx, rawy, rawbpp, rawwidth, rawheight;
  gint           i;
  gboolean       has_alpha;

  if (cleanup)
    {
      if (line_buf)
        {
          g_free (line_buf);
          line_buf = NULL;
        }

      return;
    }

  if (dispose == DISPOSE_REPLACE)
    {
      total_alpha (dest, dest_width, pixelstep);
    }

  gimp_drawable_get_offsets (drawable, &rawx, &rawy);

  rawwidth  = gimp_drawable_get_width (drawable);
  rawheight = gimp_drawable_get_height (drawable);

  /* this frame has nothing to give us for this row; return */
  if (row_num >= rawheight + rawy ||
      row_num < rawy)
    return;

  format = get_format (drawable);

  has_alpha = gimp_drawable_has_alpha (drawable);
  rawbpp    = babl_format_get_bytes_per_pixel (format);

  if (line_buf)
    {
      g_free (line_buf);
      line_buf = NULL;
    }
  line_buf = g_malloc (rawwidth * rawbpp);

  /* Initialise and fetch the raw new frame row */

  src_buffer = gimp_drawable_get_buffer (drawable);

  gegl_buffer_get (src_buffer, GEGL_RECTANGLE (0, row_num - rawy,
                                               rawwidth, 1), 1.0,
                   format, line_buf,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  g_object_unref (src_buffer);

  /* render... */

  srcptr = line_buf;

  for (i=rawx; i<rawwidth+rawx; i++)
    {
      if (i>=0 && i<dest_width)
        {
          if ((!has_alpha) || ((*(srcptr+rawbpp-1))&128))
            {
              gint pi;

              for (pi = 0; pi < pixelstep-1; pi++)
                {
                  dest[i*pixelstep +pi] = *(srcptr + pi);
                }

              dest[i*pixelstep + pixelstep - 1] = 255;
            }
        }

      srcptr += rawbpp;
    }
}


static GimpImage *
do_optimizations (GimpRunMode  run_mode,
                  GimpImage   *image,
                  gboolean     diff_only)
{
  GimpImage         *new_image;
  GimpImageBaseType  imagetype;
  GimpImageType      drawabletype_alpha;
  static guchar     *rawframe = NULL;
  guchar            *srcptr;
  guchar            *destptr;
  gint               row, this_frame_num;
  guint32            frame_sizebytes;
  GimpLayer         *new_layer;
  DisposeType        dispose;
  guchar            *this_frame = NULL;
  guchar            *last_frame = NULL;
  guchar            *opti_frame = NULL;
  guchar            *back_frame = NULL;

  gint               this_delay;
  gint               cumulated_delay = 0;
  GimpLayer         *last_true_frame = NULL;
  gint               buflen;

  gchar             *oldlayer_name;
  gchar             *newlayer_name;

  gboolean           can_combine;

  gint32             bbox_top, bbox_bottom, bbox_left, bbox_right;
  gint32             rbox_top, rbox_bottom, rbox_left, rbox_right;

  switch (opmode)
    {
    case OPUNOPTIMIZE:
      gimp_progress_init (_("Unoptimizing animation"));
      break;
    case OPFOREGROUND:
      gimp_progress_init (_("Removing animation background"));
      break;
    case OPBACKGROUND:
      gimp_progress_init (_("Finding animation background"));
      break;
    case OPOPTIMIZE:
    default:
      gimp_progress_init (_("Optimizing animation"));
      break;
    }

  width     = gimp_image_get_width (image);
  height    = gimp_image_get_height (image);
  layers    = gimp_image_get_layers (image);
  total_frames = gimp_core_object_array_get_length ((GObject **) layers);
  imagetype = gimp_image_get_base_type (image);
  pixelstep = (imagetype == GIMP_RGB) ? 4 : 2;

  drawabletype_alpha = (imagetype == GIMP_RGB) ? GIMP_RGBA_IMAGE :
    ((imagetype == GIMP_INDEXED) ? GIMP_INDEXEDA_IMAGE : GIMP_GRAYA_IMAGE);

  frame_sizebytes = width * height * pixelstep;

  this_frame = g_malloc (frame_sizebytes);
  last_frame = g_malloc (frame_sizebytes);
  opti_frame = g_malloc (frame_sizebytes);

  if (opmode == OPBACKGROUND ||
      opmode == OPFOREGROUND)
    back_frame = g_malloc (frame_sizebytes);

  total_alpha (this_frame, width*height, pixelstep);
  total_alpha (last_frame, width*height, pixelstep);

  new_image = gimp_image_new (width, height, imagetype);
  gimp_image_undo_disable (new_image);

  if (imagetype == GIMP_INDEXED)
    gimp_image_set_palette (new_image, gimp_image_get_palette (image));

#if 1
  if (opmode == OPBACKGROUND ||
      opmode == OPFOREGROUND)
    {
      /* iterate through all rows of all frames, find statistical
         mode for each pixel position. */
      gint     i;
      guint    j;
      guchar **these_rows;
      guchar **red;
      guchar **green;
      guchar **blue;
      guint  **count;
      guint   *num_colors;

      these_rows = g_new (guchar *, total_frames);
      red =        g_new (guchar *, total_frames);
      green =      g_new (guchar *, total_frames);
      blue =       g_new (guchar *, total_frames);
      count =      g_new (guint *, total_frames);

      num_colors = g_new (guint, width);

      for (this_frame_num=0; this_frame_num<total_frames; this_frame_num++)
        {
          these_rows[this_frame_num] = g_malloc (width * pixelstep);

          red[this_frame_num]   = g_new (guchar, width);
          green[this_frame_num] = g_new (guchar, width);
          blue[this_frame_num]  = g_new (guchar, width);

          count[this_frame_num] = g_new0(guint, width);
        }

      for (row = 0; row < height; row++)
        {
          memset (num_colors, 0, width * sizeof (guint));

          for (this_frame_num=0; this_frame_num<total_frames; this_frame_num++)
            {
              GimpDrawable *drawable =
                GIMP_DRAWABLE (layers[total_frames-(this_frame_num+1)]);

              dispose = get_frame_disposal (this_frame_num);

              compose_row (this_frame_num,
                           dispose,
                           row,
                           these_rows[this_frame_num],
                           width,
                           drawable,
                           FALSE);
            }

          for (this_frame_num=0; this_frame_num<total_frames; this_frame_num++)
            {
              for (i=0; i<width; i++)
                {
                  if (these_rows[this_frame_num][i * pixelstep + pixelstep -1]
                      >= 128)
                    {
                      for (j=0; j<num_colors[i]; j++)
                        {

                          switch (pixelstep)
                            {
                            case 4:
                              if (these_rows[this_frame_num][i * 4 +0] ==
                                  red[j][i] &&
                                  these_rows[this_frame_num][i * 4 +1] ==
                                  green[j][i] &&
                                  these_rows[this_frame_num][i * 4 +2] ==
                                  blue[j][i])
                                {
                                  (count[j][i])++;
                                  goto same;
                                }
                              break;
                            case 2:
                              if (these_rows[this_frame_num][i * 2 +0] ==
                                  red[j][i])
                                {
                                  (count[j][i])++;
                                  goto same;
                                }
                              break;
                            default:
                              g_error ("Eeep!");
                              break;
                            }
                        }

                      count[num_colors[i]][i] = 1;
                      red[num_colors[i]][i] =
                        these_rows[this_frame_num][i * pixelstep];
                      if (pixelstep == 4)
                        {
                          green[num_colors[i]][i] =
                            these_rows[this_frame_num][i * 4 +1];
                          blue[num_colors[i]][i] =
                            these_rows[this_frame_num][i * 4 +2];
                        }
                      num_colors[i]++;
                    }
                same:
                  /* nop */;
                }
            }

          for (i=0; i<width; i++)
            {
              guint  best_count = 0;
              guchar best_r = 255, best_g = 0, best_b = 255;

              for (j=0; j<num_colors[i]; j++)
                {
                  if (count[j][i] > best_count)
                    {
                      best_count = count[j][i];
                      best_r = red[j][i];
                      best_g = green[j][i];
                      best_b = blue[j][i];
                    }
                }

              back_frame[width * pixelstep * row +i*pixelstep + 0] = best_r;
              if (pixelstep == 4)
                {
                  back_frame[width * pixelstep * row +i*pixelstep + 1] =
                    best_g;
                  back_frame[width * pixelstep * row +i*pixelstep + 2] =
                    best_b;
                }
              back_frame[width * pixelstep * row +i*pixelstep +pixelstep-1] =
                (best_count == 0) ? 0 : 255;

              if (best_count == 0)
                g_warning ("yayyyy!");
            }
          /*      memcpy (&back_frame[width * pixelstep * row],
                  these_rows[0],
                  width * pixelstep);*/
        }

      for (this_frame_num=0; this_frame_num<total_frames; this_frame_num++)
        {
          g_free (these_rows[this_frame_num]);
          g_free (red[this_frame_num]);
          g_free (green[this_frame_num]);
          g_free (blue[this_frame_num]);
          g_free (count[this_frame_num]);
        }

      g_free (these_rows);
      g_free (red);
      g_free (green);
      g_free (blue);
      g_free (count);
      g_free (num_colors);
    }
#endif

  if (opmode == OPBACKGROUND)
    {
      GeglBuffer *buffer;
      const Babl *format;

      new_layer = gimp_layer_new (new_image,
                                  "Backgroundx",
                                  width, height,
                                  drawabletype_alpha,
                                  100.0,
                                  gimp_image_get_default_new_layer_mode (new_image));

      gimp_image_insert_layer (new_image, new_layer, NULL, 0);

      buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (new_layer));

      format = get_format (GIMP_DRAWABLE (new_layer));

      gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                       format, back_frame,
                       GEGL_ABYSS_NONE);

      g_object_unref (buffer);
    }
  else
    {
      for (this_frame_num=0; this_frame_num<total_frames; this_frame_num++)
        {
          /*
           * BUILD THIS FRAME into our 'this_frame' buffer.
           */

          GimpDrawable *drawable =
            GIMP_DRAWABLE (layers[total_frames-(this_frame_num+1)]);

          /* Image has been closed/etc since we got the layer list? */
          /* FIXME - How do we tell if a gimp_drawable_get () fails? */
          if (gimp_drawable_get_width (drawable) == 0)
            {
              gimp_quit ();
            }

          this_delay = get_frame_duration (this_frame_num);
          dispose    = get_frame_disposal (this_frame_num);

          for (row = 0; row < height; row++)
            {
              compose_row (this_frame_num,
                           dispose,
                           row,
                           &this_frame[pixelstep*width * row],
                           width,
                           drawable,
                           FALSE
                           );
            }

          if (opmode == OPFOREGROUND)
            {
              gint xit, yit, byteit;

              for (yit=0; yit<height; yit++)
                {
                  for (xit=0; xit<width; xit++)
                    {
                      for (byteit=0; byteit<pixelstep-1; byteit++)
                        {
                          if (back_frame[yit*width*pixelstep + xit*pixelstep
                                        + byteit]
                              !=
                              this_frame[yit*width*pixelstep + xit*pixelstep
                                        + byteit])
                            {
                              goto enough;
                            }
                        }
                      this_frame[yit*width*pixelstep + xit*pixelstep
                                + pixelstep - 1] = 0;
                    enough:
                      /* nop */;
                    }
                }
            }

          can_combine = FALSE;
          bbox_left   = 0;
          bbox_top    = 0;
          bbox_right  = width;
          bbox_bottom = height;
          rbox_left   = 0;
          rbox_top    = 0;
          rbox_right  = width;
          rbox_bottom = height;

          /* copy 'this' frame into a buffer which we can safely molest */
          memcpy (opti_frame, this_frame, frame_sizebytes);
          /*
           *
           * OPTIMIZE HERE!
           *
           */
          if (
              (this_frame_num != 0) /* Can't delta bottom frame! */
              && (opmode == OPOPTIMIZE)
              )
            {
              gint xit, yit, byteit;

              can_combine = TRUE;

              /*
               * SEARCH FOR BOUNDING BOX
               */
              bbox_left   = width;
              bbox_top    = height;
              bbox_right  = 0;
              bbox_bottom = 0;
              rbox_left   = width;
              rbox_top    = height;
              rbox_right  = 0;
              rbox_bottom = 0;

              for (yit=0; yit<height; yit++)
                {
                  for (xit=0; xit<width; xit++)
                    {
                      gboolean keep_pix;
                      gboolean opaq_pix;

                      /* Check if 'this' and 'last' are transparent */
                      if (!(this_frame[yit*width*pixelstep + xit*pixelstep
                                      + pixelstep-1]&128)
                          &&
                          !(last_frame[yit*width*pixelstep + xit*pixelstep
                                      + pixelstep-1]&128))
                        {
                          keep_pix = FALSE;
                          opaq_pix = FALSE;
                          goto decided;
                        }
                      /* Check if just 'this' is transparent */
                      if ((last_frame[yit*width*pixelstep + xit*pixelstep
                                     + pixelstep-1]&128)
                          &&
                          !(this_frame[yit*width*pixelstep + xit*pixelstep
                                      + pixelstep-1]&128))
                        {
                          keep_pix = TRUE;
                          opaq_pix = FALSE;
                          can_combine = FALSE;
                          goto decided;
                        }
                      /* Check if just 'last' is transparent */
                      if (!(last_frame[yit*width*pixelstep + xit*pixelstep
                                      + pixelstep-1]&128)
                          &&
                          (this_frame[yit*width*pixelstep + xit*pixelstep
                                     + pixelstep-1]&128))
                        {
                          keep_pix = TRUE;
                          opaq_pix = TRUE;
                          goto decided;
                        }
                      /* If 'last' and 'this' are opaque, we have
                       *  to check if they're the same color - we
                       *  only have to keep the pixel if 'last' or
                       *  'this' are opaque and different.
                       */
                      keep_pix = FALSE;
                      opaq_pix = TRUE;
                      for (byteit=0; byteit<pixelstep-1; byteit++)
                        {
                          if ((last_frame[yit*width*pixelstep + xit*pixelstep
                                         + byteit]
                               !=
                               this_frame[yit*width*pixelstep + xit*pixelstep
                                         + byteit])
                              )
                            {
                              keep_pix = TRUE;
                              goto decided;
                            }
                        }
                    decided:
                      if (opaq_pix)
                        {
                          if (xit<rbox_left) rbox_left=xit;
                          if (xit>rbox_right) rbox_right=xit;
                          if (yit<rbox_top) rbox_top=yit;
                          if (yit>rbox_bottom) rbox_bottom=yit;
                        }
                      if (keep_pix)
                        {
                          if (xit<bbox_left) bbox_left=xit;
                          if (xit>bbox_right) bbox_right=xit;
                          if (yit<bbox_top) bbox_top=yit;
                          if (yit>bbox_bottom) bbox_bottom=yit;
                        }
                      else
                        {
                          /* pixel didn't change this frame - make
                           *  it transparent in our optimized buffer!
                           */
                          opti_frame[yit*width*pixelstep + xit*pixelstep
                                    + pixelstep-1] = 0;
                        }
                    } /* xit */
                } /* yit */

              if (!can_combine)
                {
                  bbox_left = rbox_left;
                  bbox_top = rbox_top;
                  bbox_right = rbox_right;
                  bbox_bottom = rbox_bottom;
                }

              bbox_right++;
              bbox_bottom++;

              if (can_combine && !diff_only)
                {
                  /* Try to optimize the pixel data for RLE or LZW compression
                   * by making some transparent pixels non-transparent if they
                   * would have the same color as the adjacent pixels.  This
                   * gives a better compression if the algorithm compresses
                   * the image line by line.
                   * See: http://bugzilla.gnome.org/show_bug.cgi?id=66367
                   * It may not be very efficient to add two additional passes
                   * over the pixels, but this hopefully makes the code easier
                   * to maintain and less error-prone.
                   */
                  for (yit = bbox_top; yit < bbox_bottom; yit++)
                    {
                      /* Compare with previous pixels from left to right */
                      for (xit = bbox_left + 1; xit < bbox_right; xit++)
                        {
                          if (!(opti_frame[yit*width*pixelstep
                                           + xit*pixelstep
                                           + pixelstep-1]&128)
                              && (opti_frame[yit*width*pixelstep
                                             + (xit-1)*pixelstep
                                             + pixelstep-1]&128)
                              && (last_frame[yit*width*pixelstep
                                             + xit*pixelstep
                                             + pixelstep-1]&128))
                            {
                              for (byteit=0; byteit<pixelstep-1; byteit++)
                                {
                                  if (opti_frame[yit*width*pixelstep
                                                 + (xit-1)*pixelstep
                                                 + byteit]
                                      !=
                                      last_frame[yit*width*pixelstep
                                                 + xit*pixelstep
                                                 + byteit])
                                    {
                                      goto skip_right;
                                    }
                                }
                              /* copy the color and alpha */
                              for (byteit=0; byteit<pixelstep; byteit++)
                                {
                                  opti_frame[yit*width*pixelstep
                                             + xit*pixelstep
                                             + byteit]
                                    = last_frame[yit*width*pixelstep
                                                 + xit*pixelstep
                                                 + byteit];
                                }
                            }
                        skip_right:
                          /* nop */;
                        } /* xit */

                      /* Compare with next pixels from right to left */
                      for (xit = bbox_right - 2; xit >= bbox_left; xit--)
                        {
                          if (!(opti_frame[yit*width*pixelstep
                                           + xit*pixelstep
                                           + pixelstep-1]&128)
                              && (opti_frame[yit*width*pixelstep
                                             + (xit+1)*pixelstep
                                             + pixelstep-1]&128)
                              && (last_frame[yit*width*pixelstep
                                             + xit*pixelstep
                                             + pixelstep-1]&128))
                            {
                              for (byteit=0; byteit<pixelstep-1; byteit++)
                                {
                                  if (opti_frame[yit*width*pixelstep
                                                 + (xit+1)*pixelstep
                                                 + byteit]
                                      !=
                                      last_frame[yit*width*pixelstep
                                                 + xit*pixelstep
                                                 + byteit])
                                    {
                                      goto skip_left;
                                    }
                                }
                              /* copy the color and alpha */
                              for (byteit=0; byteit<pixelstep; byteit++)
                                {
                                  opti_frame[yit*width*pixelstep
                                             + xit*pixelstep
                                             + byteit]
                                    = last_frame[yit*width*pixelstep
                                                 + xit*pixelstep
                                                 + byteit];
                                }
                            }
                        skip_left:
                          /* nop */;
                        } /* xit */
                    } /* yit */
                }

              /*
               * Collapse opti_frame data down such that the data
               *  which occupies the bounding box sits at the start
               *  of the data (for convenience with ..set_rect ()).
               */
              destptr = opti_frame;
              /*
               * If can_combine, then it's safe to use our optimized
               *  alpha information.  Otherwise, an opaque pixel became
               *  transparent this frame, and we'll have to use the
               *  actual true frame's alpha.
               */
              if (can_combine)
                srcptr = opti_frame;
              else
                srcptr = this_frame;
              for (yit=bbox_top; yit<bbox_bottom; yit++)
                {
                  for (xit=bbox_left; xit<bbox_right; xit++)
                    {
                      for (byteit=0; byteit<pixelstep; byteit++)
                        {
                          *(destptr++) = srcptr[yit*pixelstep*width +
                                               pixelstep*xit + byteit];
                        }
                    }
                }
            } /* !bot frame? */
          else
            {
              memcpy (opti_frame, this_frame, frame_sizebytes);
            }

          /*
           *
           * REMEMBER THE ANIMATION STATUS TO DELTA AGAINST NEXT TIME
           *
           */
          memcpy (last_frame, this_frame, frame_sizebytes);


          /*
           *
           * PUT THIS FRAME INTO A NEW LAYER IN THE NEW IMAGE
           *
           */

          oldlayer_name =
            gimp_item_get_name (GIMP_ITEM (layers[total_frames-(this_frame_num+1)]));

          buflen = strlen (oldlayer_name) + 40;

          newlayer_name = g_malloc (buflen);

          remove_disposal_tag (newlayer_name, oldlayer_name);
          g_free (oldlayer_name);

          oldlayer_name = g_malloc (buflen);

          remove_ms_tag (oldlayer_name, newlayer_name);

          g_snprintf (newlayer_name, buflen, "%s(%dms)%s",
                      oldlayer_name, this_delay,
                      (this_frame_num ==  0) ? "" :
                      can_combine ? "(combine)" : "(replace)");

          g_free (oldlayer_name);

          /* Empty frame! */
          if (bbox_right <= bbox_left ||
              bbox_bottom <= bbox_top)
            {
              cumulated_delay += this_delay;

              g_free (newlayer_name);

              oldlayer_name = gimp_item_get_name (GIMP_ITEM (last_true_frame));

              buflen = strlen (oldlayer_name) + 40;

              newlayer_name = g_malloc (buflen);

              remove_disposal_tag (newlayer_name, oldlayer_name);
              g_free (oldlayer_name);

              oldlayer_name = g_malloc (buflen);

              remove_ms_tag (oldlayer_name, newlayer_name);

              g_snprintf (newlayer_name, buflen, "%s(%dms)%s",
                          oldlayer_name, cumulated_delay,
                          (this_frame_num ==  0) ? "" :
                          can_combine ? "(combine)" : "(replace)");

              gimp_item_set_name (GIMP_ITEM (last_true_frame), newlayer_name);

              g_free (newlayer_name);
              g_free (oldlayer_name);
            }
          else
            {
              GeglBuffer *buffer;
              const Babl *format;

              cumulated_delay = this_delay;

              last_true_frame =
                new_layer = gimp_layer_new (new_image,
                                            newlayer_name,
                                            bbox_right-bbox_left,
                                            bbox_bottom-bbox_top,
                                            drawabletype_alpha,
                                            100.0,
                                            gimp_image_get_default_new_layer_mode (new_image));
              g_free (newlayer_name);

              gimp_image_insert_layer (new_image, new_layer, NULL, 0);

              buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (new_layer));

              format = get_format (GIMP_DRAWABLE (new_layer));

              gegl_buffer_set (buffer,
                               GEGL_RECTANGLE (0, 0,
                                               bbox_right-bbox_left,
                                               bbox_bottom-bbox_top), 0,
                               format, opti_frame,
                               GEGL_AUTO_ROWSTRIDE);

              g_object_unref (buffer);
              gimp_item_transform_translate (GIMP_ITEM (new_layer), bbox_left, bbox_top);
            }

          gimp_progress_update (((gdouble) this_frame_num + 1.0) /
                                ((gdouble) total_frames));
        }

      gimp_progress_update (1.0);
    }

  gimp_image_undo_enable (new_image);

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_display_new (new_image);

  g_free (rawframe);
  rawframe = NULL;

  g_free (last_frame);
  last_frame = NULL;

  g_free (this_frame);
  this_frame = NULL;

  g_free (opti_frame);
  opti_frame = NULL;

  g_free (back_frame);
  back_frame = NULL;

  return new_image;
}

/* Util. */

static DisposeType
get_frame_disposal (guint whichframe)
{
  gchar       *layer_name;
  DisposeType  disposal;

  layer_name = gimp_item_get_name (GIMP_ITEM (layers[total_frames-(whichframe+1)]));
  disposal = parse_disposal_tag (layer_name);
  g_free (layer_name);

  return disposal;
}

static guint32
get_frame_duration (guint whichframe)
{
  gchar* layer_name;
  gint   duration = 0;

  layer_name = gimp_item_get_name (GIMP_ITEM (layers[total_frames-(whichframe+1)]));
  if (layer_name)
    {
      duration = parse_ms_tag (layer_name);
      g_free (layer_name);
    }

  if (duration < 0) duration = 100;  /* FIXME for default-if-not-said  */
  if (duration == 0) duration = 100; /* FIXME - 0-wait is nasty */

  return (guint32) duration;
}

static gboolean
is_ms_tag (const gchar *str,
           gint        *duration,
           gint        *taglength)
{
  gint sum = 0;
  gint offset;
  gint length;

  length = strlen (str);

  if (str[0] != '(')
    return FALSE;

  offset = 1;

  /* eat any spaces between open-parenthesis and number */
  while ((offset<length) && (str[offset] == ' '))
    offset++;

  if ((offset>=length) || (!g_ascii_isdigit (str[offset])))
    return 0;

  do
    {
      sum *= 10;
      sum += str[offset] - '0';
      offset++;
    }
  while ((offset<length) && (g_ascii_isdigit (str[offset])));

  if (length-offset <= 2)
    return FALSE;

  /* eat any spaces between number and 'ms' */
  while ((offset<length) && (str[offset] == ' '))
    offset++;

  if ((length-offset <= 2) ||
      (g_ascii_toupper (str[offset]) != 'M') ||
      (g_ascii_toupper (str[offset+1]) != 'S'))
    return FALSE;

  offset += 2;

  /* eat any spaces between 'ms' and close-parenthesis */
  while ((offset<length) && (str[offset] == ' '))
    offset++;

  if ((length-offset < 1) || (str[offset] != ')'))
    return FALSE;

  offset++;

  *duration  = sum;
  *taglength = offset;

  return TRUE;
}

static int
parse_ms_tag (const char *str)
{
  gint i;
  gint rtn;
  gint dummy;
  gint length;

  length = strlen (str);

  for (i = 0; i < length; i++)
    {
      if (is_ms_tag (&str[i], &rtn, &dummy))
        return rtn;
    }

  return -1;
}

static gboolean
is_disposal_tag (const gchar *str,
                 DisposeType *disposal,
                 gint        *taglength)
{
  if (strlen (str) != 9)
    return FALSE;

  if (strncmp (str, "(combine)", 9) == 0)
    {
      *taglength = 9;
      *disposal = DISPOSE_COMBINE;
      return TRUE;
    }
  else if (strncmp (str, "(replace)", 9) == 0)
    {
      *taglength = 9;
      *disposal = DISPOSE_REPLACE;
      return TRUE;
    }

  return FALSE;
}


static DisposeType
parse_disposal_tag (const gchar *str)
{
  DisposeType rtn;
  gint        i, dummy;
  gint        length;

  length = strlen (str);

  for (i=0; i<length; i++)
    {
      if (is_disposal_tag (&str[i], &rtn, &dummy))
        {
          return rtn;
        }
    }

  return DISPOSE_UNDEFINED; /* FIXME */
}

static void
remove_disposal_tag (gchar *dest,
                     gchar *src)
{
  gint        offset = 0;
  gint        destoffset = 0;
  gint        length;
  int         taglength;
  DisposeType dummy;

  length = strlen (src);

  strcpy (dest, src);

  while (offset<=length)
    {
      if (is_disposal_tag (&src[offset], &dummy, &taglength))
        {
          offset += taglength;
        }
      dest[destoffset] = src[offset];
      destoffset++;
      offset++;
    }

  dest[offset] = '\0';
}

static void
remove_ms_tag (gchar *dest,
               gchar *src)
{
  gint offset = 0;
  gint destoffset = 0;
  gint length;
  gint taglength;
  gint dummy;

  length = strlen (src);

  strcpy (dest, src);

  while (offset<=length)
    {
      if (is_ms_tag (&src[offset], &dummy, &taglength))
        {
          offset += taglength;
        }
      dest[destoffset] = src[offset];
      destoffset++;
      offset++;
    }

  dest[offset] = '\0';
}
