/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Film plug-in (C) 1997 Peter Kirchgessner
 * e-mail: pkirchg@aol.com, WWW: http://members.aol.com/pkirchg
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
 * This plug-in generates a film roll with several images
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC        "plug-in-film"
#define PLUG_IN_BINARY      "film"
#define PLUG_IN_ROLE        "gimp-film"

/* Maximum number of pictures per film */
#define MAX_FILM_PICTURES   64
#define COLOR_BUTTON_WIDTH  50
#define COLOR_BUTTON_HEIGHT 20

#define FONT_LEN            256

/* Define how the plug-in works. Values marked (r) are with regard */
/* to film_height (i.e. it should be a value from 0.0 to 1.0) */
typedef struct
{
  gint     film_height;           /* height of the film */
  GimpRGB  film_color;            /* color of film */
  gdouble  picture_height;        /* height of picture (r) */
  gdouble  picture_space;         /* space between pictures (r) */
  gdouble  hole_offset;           /* distance from hole to edge of film (r) */
  gdouble  hole_width;            /* width of hole (r) */
  gdouble  hole_height;           /* height of holes (r) */
  gdouble  hole_space;            /* distance of holes (r) */
  gdouble  number_height;         /* height of picture numbering (r) */
  gint     number_start;          /* number for first picture */
  GimpRGB  number_color;          /* color of number */
  gchar    number_font[FONT_LEN]; /* font family to use for numbering */
  gint     number_pos[2];         /* flags where to draw numbers (top/bottom) */
  gint     keep_height;           /* flag if to keep max. image height */
  gint     num_images;            /* number of images */
  gint32   images[MAX_FILM_PICTURES]; /* list of image IDs */
} FilmVals;

/* Data to use for the dialog */
typedef struct
{
  GtkAdjustment *advanced_adj[7];
  GtkTreeModel  *image_list_all;
  GtkTreeModel  *image_list_film;
} FilmInterface;


typedef struct _Film      Film;
typedef struct _FilmClass FilmClass;

struct _Film
{
  GimpPlugIn parent_instance;
};

struct _FilmClass
{
  GimpPlugInClass parent_class;
};


#define FILM_TYPE  (film_get_type ())
#define FILM (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), FILM_TYPE, Film))

GType                   film_get_type         (void) G_GNUC_CONST;

static GList          * film_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * film_create_procedure (GimpPlugIn           *plug_in,
                                               const gchar          *name);

static GimpValueArray * film_run              (GimpProcedure        *procedure,
                                               GimpRunMode           run_mode,
                                               GimpImage            *image,
                                               GimpDrawable         *drawable,
                                               const GimpValueArray *args,
                                               gpointer              run_data);

static GimpImage      * create_new_image      (guint                 width,
                                               guint                 height,
                                               GimpImageType         gdtype,
                                               GimpLayer           **layer);

static gchar          * compose_image_name    (GimpImage            *image);

static GimpImage      * film                  (void);

static gboolean         check_filmvals        (void);

static void             set_pixels            (gint                  numpix,
                                               guchar               *dst,
                                               GimpRGB              *color);

static guchar         * create_hole_rgb       (gint                  width,
                                               gint                  height);

static void             draw_number           (GimpLayer            *layer,
                                               gint                  num,
                                               gint                  x,
                                               gint                  y,
                                               gint                  height);


static void            add_list_item_callback (GtkWidget            *widget,
                                               GtkTreeSelection     *sel);
static void            del_list_item_callback (GtkWidget            *widget,
                                               GtkTreeSelection     *sel);

static GtkTreeModel  * add_image_list         (gboolean              add_box_flag,
                                               GList                *images,
                                               GtkWidget            *hbox);

static gboolean        film_dialog            (GimpImage            *image);
static void            film_reset_callback    (GtkWidget            *widget,
                                               gpointer              data);
static void         film_font_select_callback (GimpFontSelectButton *button,
                                               const gchar          *name,
                                               gboolean              closing,
                                               gpointer              data);


G_DEFINE_TYPE (Film, film, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (FILM_TYPE)


static gdouble advanced_defaults[] =
{
  0.695,           /* Picture height */
  0.040,           /* Picture spacing */
  0.058,           /* Hole offset to edge of film */
  0.052,           /* Hole width */
  0.081,           /* Hole height */
  0.081,           /* Hole distance */
  0.052            /* Image number height */
};

static FilmVals filmvals =
{
  256,             /* Height of film */
  { 0.0, 0.0, 0.0, 1.0 }, /* Color of film */
  0.695,           /* Picture height */
  0.040,           /* Picture spacing */
  0.058,           /* Hole offset to edge of film */
  0.052,           /* Hole width */
  0.081,           /* Hole height */
  0.081,           /* Hole distance */
  0.052,           /* Image number height */
  1,               /* Start index of numbering */
  { 0.93, 0.61, 0.0, 1.0 }, /* Color of number */
  "Monospace",     /* Font family for numbering */
  { TRUE, TRUE },  /* Numbering on top and bottom */
  0,               /* Don't keep max. image height */
  0,               /* Number of images */
  { 0 }            /* Input image list */
};

static FilmInterface filmint =
{
  { NULL },   /* advanced adjustments */
  NULL, NULL  /* image list widgets */
};


static void
film_class_init (FilmClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = film_query_procedures;
  plug_in_class->create_procedure = film_create_procedure;
}

static void
film_init (Film *film)
{
}

static GList *
film_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
film_create_procedure (GimpPlugIn  *plug_in,
                       const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            film_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, N_("_Filmstrip..."));
      gimp_procedure_add_menu_path (procedure, "<Image>/Filters/Combine");

      gimp_procedure_set_documentation (procedure,
                                        N_("Combine several images on a "
                                           "film strip"),
                                        "Compose several images to a roll film",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Peter Kirchgessner",
                                      "Peter Kirchgessner (peter@kirchgessner.net)",
                                      "1997");

      GIMP_PROC_ARG_INT (procedure, "film-height",
                         "Film height",
                         "Height of film (0: fit to images)",
                         0, GIMP_MAX_IMAGE_SIZE, 0,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_RGB (procedure, "film-color",
                         "Film color",
                         "Color of the film",
                         TRUE, NULL,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "number-start",
                         "Number start",
                         "Start index for numbering",
                         G_MININT, G_MAXINT, 1,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_STRING (procedure, "number-font",
                            "Number font",
                            "Font for drawing numbers",
                            NULL,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_RGB (procedure, "number-color",
                         "Number color",
                         "Color for numbers",
                         TRUE, NULL,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "at-top",
                             "At top",
                             "Draw numbers at top",
                             TRUE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "at-bottom",
                             "At bottom",
                             "Draw numbers at bottom",
                             TRUE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "num-images",
                         "Num images",
                         "Number of images to be used for film",
                         1, MAX_FILM_PICTURES, 1,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_OBJECT_ARRAY (procedure, "images",
                                  "Images",
                                  "num-images images to be used for film",
                                  GIMP_TYPE_IMAGE,
                                  G_PARAM_READWRITE);

      GIMP_PROC_VAL_IMAGE (procedure, "new-image",
                           "New image",
                           "Output image",
                           FALSE,
                           G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
film_run (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GimpImage            *image,
          GimpDrawable         *drawable,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpValueArray     *return_vals = NULL;
  GimpPDBStatusType   status      = GIMP_PDB_SUCCESS;
  GimpImage         **images;
  gint                i;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_PROC, &filmvals);

      if (! film_dialog (image))
        {
          return gimp_procedure_new_return_values (procedure, GIMP_PDB_CANCEL,
                                                   NULL);
        }
      break;

    case GIMP_RUN_NONINTERACTIVE:
      filmvals.film_height = GIMP_VALUES_GET_INT (args, 0);
      if (filmvals.film_height <= 0)
        {
          filmvals.keep_height = TRUE;
          filmvals.film_height = 128; /* arbitrary */
        }
      else
        {
          filmvals.keep_height = FALSE;
        }
      GIMP_VALUES_GET_RGB (args, 1, &filmvals.film_color);
      filmvals.number_start = GIMP_VALUES_GET_INT           (args, 2);
      g_strlcpy              (filmvals.number_font,
                              GIMP_VALUES_GET_STRING        (args, 3),
                              FONT_LEN);
      GIMP_VALUES_GET_RGB (args, 4, &filmvals.number_color);
      filmvals.number_pos[0] = GIMP_VALUES_GET_INT          (args, 5);
      filmvals.number_pos[1] = GIMP_VALUES_GET_INT          (args, 6);
      filmvals.num_images    = GIMP_VALUES_GET_INT          (args, 7);
      images                 = GIMP_VALUES_GET_OBJECT_ARRAY (args, 8);

      for (i = 0; i < filmvals.num_images; i++)
        filmvals.images[i] = gimp_image_get_id (images[i]);
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_PROC, &filmvals);
      break;

    default:
      break;
    }

  if (! check_filmvals ())
    status = GIMP_PDB_CALLING_ERROR;

  if (status == GIMP_PDB_SUCCESS)
    {
      GimpImage *image;

      gimp_progress_init (_("Composing images"));

      image = film ();

      if (! image)
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
      else
        {
          return_vals = gimp_procedure_new_return_values (procedure, status,
                                                          NULL);

          GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

          gimp_image_undo_enable (image);
          gimp_image_clean_all (image);

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_display_new (image);
        }

      /*  Store data  */
      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data (PLUG_IN_PROC, &filmvals, sizeof (FilmVals));
    }

  if (! return_vals)
    return_vals = gimp_procedure_new_return_values (procedure, status, NULL);

  return return_vals;
}

/* Compose a roll film image from several images */
static GimpImage *
film (void)
{
  gint          width, height;
  guchar       *hole;
  gint          film_height, film_width;
  gint          picture_width, picture_height;
  gint          picture_space, picture_x0, picture_y0;
  gint          hole_offset, hole_width, hole_height, hole_space, hole_x;
  gint          number_height, num_images, num_pictures;
  gint          picture_count;
  gdouble       f;
  GimpImage    *image_dst;
  GimpImage    *image_tmp;
  GimpLayer    *layer_src;
  GimpLayer    *layer_dst;
  GimpLayer    *new_layer;
  GimpLayer    *floating_sel;

  GList        *images_src = NULL;
  GList        *layers     = NULL;
  GList        *iter;
  GList        *iter2;
  gint          i;


  num_images = filmvals.num_images;

  if (num_images <= 0)
    return NULL;

  for (i = 0; i < filmvals.num_images; i++)
    {
      images_src = g_list_append (images_src,
                                  gimp_image_get_by_id (filmvals.images[i]));
    }

  gimp_context_push ();
  gimp_context_set_foreground (&filmvals.number_color);
  gimp_context_set_background (&filmvals.film_color);

  if (filmvals.keep_height) /* Search maximum picture height */
    {
      picture_height = 0;
      for (iter = images_src; iter; iter = iter->next)
        {
          height = gimp_image_height (iter->data);
          if (height > picture_height) picture_height = height;
        }
      film_height = (int)(picture_height / filmvals.picture_height + 0.5);
      filmvals.film_height = film_height;
    }
  else
    {
      film_height = filmvals.film_height;
      picture_height = (int)(film_height * filmvals.picture_height + 0.5);
    }

  picture_space = (int)(film_height * filmvals.picture_space + 0.5);
  picture_y0 = (film_height - picture_height)/2;

  number_height = film_height * filmvals.number_height;

  /* Calculate total film width */
  film_width = 0;
  num_pictures = 0;
  for (iter = images_src; iter; iter = g_list_next (iter))
    {
      layers = gimp_image_list_layers (iter->data);

      /* Get scaled image size */
      width = gimp_image_width (iter->data);
      height = gimp_image_height (iter->data);
      f = ((double)picture_height) / (double)height;
      picture_width = width * f;

      for (iter2 = layers; iter2; iter2 = g_list_next (iter2))
        {
          if (gimp_layer_is_floating_sel (iter2->data))
            continue;

          film_width += (picture_space/2);  /* Leading space */
          film_width += picture_width;      /* Scaled image width */
          film_width += (picture_space/2);  /* Trailing space */
          num_pictures++;
        }

      g_list_free (layers);
    }

#ifdef FILM_DEBUG
  g_printerr ("film_height = %d, film_width = %d\n", film_height, film_width);
  g_printerr ("picture_height = %d, picture_space = %d, picture_y0 = %d\n",
              picture_height, picture_space, picture_y0);
  g_printerr ("Number of pictures = %d\n", num_pictures);
#endif

  image_dst = create_new_image ((guint) film_width, (guint) film_height,
                                GIMP_RGB_IMAGE, &layer_dst);

  /* Fill film background */
  gimp_drawable_fill (GIMP_DRAWABLE (layer_dst), GIMP_FILL_BACKGROUND);

  /* Draw all the holes */
  hole_offset = film_height * filmvals.hole_offset;
  hole_width  = film_height * filmvals.hole_width;
  hole_height = film_height * filmvals.hole_height;
  hole_space  = film_height * filmvals.hole_space;
  hole_x = hole_space / 2;

#ifdef FILM_DEBUG
  g_printerr ("hole_x %d hole_offset %d hole_width %d hole_height %d hole_space %d\n",
              hole_x, hole_offset, hole_width, hole_height, hole_space );
#endif

  hole = create_hole_rgb (hole_width, hole_height);
  if (hole)
    {
      GeglBuffer *buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer_dst));

      while (hole_x < film_width)
        {
          gegl_buffer_set (buffer,
                           GEGL_RECTANGLE (hole_x,
                                           hole_offset,
                                           hole_width,
                                           hole_height), 0,
                           babl_format ("R'G'B' u8"), hole,
                           GEGL_AUTO_ROWSTRIDE);

          gegl_buffer_set (buffer,
                           GEGL_RECTANGLE (hole_x,
                                           film_height - hole_offset - hole_height,
                                           hole_width,
                                           hole_height), 0,
                           babl_format ("R'G'B' u8"), hole,
                           GEGL_AUTO_ROWSTRIDE);

          hole_x += hole_width + hole_space;
        }

      g_object_unref (buffer);
      g_free (hole);
    }

  /* Compose all images and layers */
  picture_x0 = 0;
  picture_count = 0;
  for (iter = images_src; iter; iter = iter->next)
    {
      image_tmp = gimp_image_duplicate (iter->data);
      width = gimp_image_width (image_tmp);
      height = gimp_image_height (image_tmp);
      f = ((gdouble) picture_height) / (gdouble) height;
      picture_width = width * f;
      if (gimp_image_base_type (image_tmp) != GIMP_RGB)
        gimp_image_convert_rgb (image_tmp);
      gimp_image_scale (image_tmp, picture_width, picture_height);

      layers = gimp_image_list_layers (image_tmp);

      for (iter2 = layers; iter2; iter2 = g_list_next (iter2))
        {
          if (gimp_layer_is_floating_sel (iter2->data))
            continue;

          picture_x0 += picture_space / 2;

          layer_src = iter2->data;
          gimp_layer_resize_to_image_size (layer_src);
          new_layer = gimp_layer_new_from_drawable (GIMP_DRAWABLE (layer_src),
                                                    image_dst);
          gimp_image_insert_layer (image_dst, new_layer, NULL, -1);
          gimp_layer_set_offsets (new_layer, picture_x0, picture_y0);

          /* Draw picture numbers */
          if ((number_height > 0) &&
              (filmvals.number_pos[0] || filmvals.number_pos[1]))
            {
              if (filmvals.number_pos[0])
                draw_number (layer_dst,
                             filmvals.number_start + picture_count,
                             picture_x0 + picture_width/2,
                             (hole_offset-number_height)/2, number_height);
              if (filmvals.number_pos[1])
                draw_number (layer_dst,
                             filmvals.number_start + picture_count,
                             picture_x0 + picture_width/2,
                             film_height - (hole_offset + number_height)/2,
                             number_height);
            }

          picture_x0 += picture_width + (picture_space/2);

          gimp_progress_update (((gdouble) (picture_count + 1)) /
                                (gdouble) num_pictures);

          picture_count++;
        }

      g_list_free (layers);

      gimp_image_delete (image_tmp);
    }

  g_list_free (images_src);

  gimp_progress_update (1.0);

  gimp_image_flatten (image_dst);

  /* Drawing text/numbers leaves us with a floating selection. Stop it */
  floating_sel = gimp_image_get_floating_sel (image_dst);
  if (floating_sel)
    gimp_floating_sel_anchor (floating_sel);

  gimp_context_pop ();

  return image_dst;
}

/* Check filmvals. Unreasonable values are reset to a default. */
/* If this is not possible, FALSE is returned. Otherwise TRUE is returned. */
static gboolean
check_filmvals (void)
{
  gint i, j;

  if (filmvals.film_height < 10)
    filmvals.film_height = 10;

  if (filmvals.number_start < 0)
    filmvals.number_start = 0;

  if (filmvals.number_font[0] == '\0')
    strcpy (filmvals.number_font, "Monospace");

  for (i = 0, j = 0; i < filmvals.num_images; i++)
    {
      if (gimp_image_id_is_valid (filmvals.images[i]))
        {
          filmvals.images[j] = filmvals.images[i];
          j++;
        }
    }

  filmvals.num_images = j;

  if (filmvals.num_images < 1)
    return FALSE;

  return TRUE;
}

/* Assigns numpix pixels starting at dst with color r,g,b */
static void
set_pixels (gint     numpix,
            guchar  *dst,
            GimpRGB *color)
{
  register gint   k;
  register guchar ur, ug, ub, *udest;

  ur = color->r * 255.999;
  ug = color->g * 255.999;
  ub = color->b * 255.999;
  k = numpix;
  udest = dst;

  while (k-- > 0)
    {
      *(udest++) = ur;
      *(udest++) = ug;
      *(udest++) = ub;
    }
}

/* Create the RGB-pixels that make up the hole */
static guchar *
create_hole_rgb (gint width,
                 gint height)
{
  guchar *hole, *top, *bottom;
  gint    radius, length, k;

  hole = g_new (guchar, width * height * 3);

  /* Fill a rectangle with white */
  memset (hole, 255, width * height * 3);
  radius = height / 4;
  if (radius > width / 2)
    radius = width / 2;
  top = hole;
  bottom = hole + (height-1)*width*3;
  for (k = radius-1; k > 0; k--)  /* Rounding corners */
    {
      length = (int)(radius - sqrt ((gdouble) (radius * radius - k * k))
                     - 0.5);
      if (length > 0)
        {
          set_pixels (length, top, &filmvals.film_color);
          set_pixels (length, top + (width-length)*3, &filmvals.film_color);
          set_pixels (length, bottom, &filmvals.film_color);
          set_pixels (length, bottom + (width-length)*3, &filmvals.film_color);
        }
      top += width*3;
      bottom -= width*3;
    }

  return hole;
}

/* Draw the number of the picture onto the film */
static void
draw_number (GimpLayer *layer,
             gint       num,
             gint       x,
             gint       y,
             gint       height)
{
  gchar         buf[32];
  gint          k, delta, max_delta;
  GimpImage    *image;
  GimpLayer    *text_layer;
  gint          text_width, text_height, text_ascent, descent;
  gchar        *fontname = filmvals.number_font;

  g_snprintf (buf, sizeof (buf), "%d", num);

  image = gimp_item_get_image (GIMP_ITEM (layer));

  max_delta = height / 10;
  if (max_delta < 1)
    max_delta = 1;

  /* Numbers don't need the descent. Inquire it and move the text down */
  for (k = 0; k < max_delta * 2 + 1; k++)
    {
      /* Try different font sizes if inquire of extent failed */
      gboolean success;

      delta = (k+1) / 2;

      if ((k & 1) == 0)
        delta = -delta;

      success = gimp_text_get_extents_fontname (buf,
                                                height + delta, GIMP_PIXELS,
                                                fontname,
                                                &text_width, &text_height,
                                                &text_ascent, &descent);

      if (success)
        {
          height += delta;
          break;
        }
    }

  text_layer = gimp_text_fontname (image, GIMP_DRAWABLE (layer),
                                   x, y + descent / 2,
                                   buf, 1, FALSE,
                                   height, GIMP_PIXELS,
                                   fontname);

  if (! text_layer)
    g_message ("draw_number: Error in drawing text\n");
}

/* Create an image. Sets layer, drawable and rgn. Returns image */
static GimpImage *
create_new_image (guint           width,
                  guint           height,
                  GimpImageType   gdtype,
                  GimpLayer     **layer)
{
  GimpImage        *image;
  GimpImageBaseType gitype;

  if ((gdtype == GIMP_GRAY_IMAGE) || (gdtype == GIMP_GRAYA_IMAGE))
    gitype = GIMP_GRAY;
  else if ((gdtype == GIMP_INDEXED_IMAGE) || (gdtype == GIMP_INDEXEDA_IMAGE))
    gitype = GIMP_INDEXED;
  else
    gitype = GIMP_RGB;

  image = gimp_image_new (width, height, gitype);

  gimp_image_undo_disable (image);
  *layer = gimp_layer_new (image, _("Background"), width, height,
                           gdtype,
                           100,
                           gimp_image_get_default_new_layer_mode (image));
  gimp_image_insert_layer (image, *layer, NULL, 0);

  return image;
}

static gchar *
compose_image_name (GimpImage *image)
{
  gchar *image_name;
  gchar *name;

  /* Compose a name of the basename and the image-ID */

  name = gimp_image_get_name (image);

  image_name = g_strdup_printf ("%s-%d", name, gimp_image_get_id (image));

  g_free (name);

  return image_name;
}

static void
add_list_item_callback (GtkWidget        *widget,
                        GtkTreeSelection *sel)
{
  GtkTreeModel *model;
  GList        *paths;
  GList        *list;

  paths = gtk_tree_selection_get_selected_rows (sel, &model);

  for (list = paths; list; list = g_list_next (list))
    {
      GtkTreeIter iter;

      if (gtk_tree_model_get_iter (model, &iter, list->data))
        {
          GimpImage *image;
          gchar  *name;

          gtk_tree_model_get (model, &iter,
                              0, &image,
                              1, &name,
                              -1);

          gtk_list_store_append (GTK_LIST_STORE (filmint.image_list_film),
                                 &iter);

          gtk_list_store_set (GTK_LIST_STORE (filmint.image_list_film),
                              &iter,
                              0, image,
                              1, name,
                              -1);

          g_free (name);
        }

      gtk_tree_path_free (list->data);
    }

  g_list_free (paths);
}

static void
del_list_item_callback (GtkWidget        *widget,
                        GtkTreeSelection *sel)
{
  GtkTreeModel *model;
  GList        *paths;
  GList        *references = NULL;
  GList        *list;

  paths = gtk_tree_selection_get_selected_rows (sel, &model);

  for (list = paths; list; list = g_list_next (list))
    {
      GtkTreeRowReference *ref;

      ref = gtk_tree_row_reference_new (model, list->data);
      references = g_list_prepend (references, ref);
      gtk_tree_path_free (list->data);
    }

  g_list_free (paths);

  for (list = references; list; list = g_list_next (list))
    {
      GtkTreePath *path;
      GtkTreeIter  iter;

      path = gtk_tree_row_reference_get_path (list->data);

      if (gtk_tree_model_get_iter (model, &iter, path))
        gtk_list_store_remove (GTK_LIST_STORE (model), &iter);

      gtk_tree_path_free (path);
      gtk_tree_row_reference_free (list->data);
    }

  g_list_free (references);
}

static GtkTreeModel *
add_image_list (gboolean   add_box_flag,
                GList     *images,
                GtkWidget *hbox)
{
  GtkWidget        *vbox;
  GtkWidget        *label;
  GtkWidget        *scrolled_win;
  GtkWidget        *tv;
  GtkWidget        *button;
  GtkListStore     *store;
  GtkTreeSelection *sel;
  GList            *list;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  label = gtk_label_new (add_box_flag ?
                         _("Available images:") :
                         _("On film:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_win);

  store = gtk_list_store_new (2, G_TYPE_INT, G_TYPE_STRING);
  tv = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref (store);

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tv), FALSE);

  if (! add_box_flag)
    gtk_tree_view_set_reorderable (GTK_TREE_VIEW (tv), TRUE);

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tv), 0, NULL,
                                               gtk_cell_renderer_text_new (),
                                               "text", 1,
                                               NULL);

  gtk_container_add (GTK_CONTAINER (scrolled_win), tv);
  gtk_widget_show (tv);

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
  gtk_tree_selection_set_mode (sel, GTK_SELECTION_MULTIPLE);

  for (list = images; list; list = list->next)
    {
      GtkTreeIter  iter;
      gchar       *name;

      gtk_list_store_append (store, &iter);

      name = compose_image_name (list->data);

      gtk_list_store_set (store, &iter,
                          0, gimp_image_get_id (list->data),
                          1, name,
                          -1);

      g_free (name);
    }

  button = gtk_button_new_with_mnemonic (add_box_flag ?
                                         _("_Add") : _("_Remove"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    add_box_flag ?
                    G_CALLBACK (add_list_item_callback) :
                    G_CALLBACK (del_list_item_callback),
                    sel);

  return GTK_TREE_MODEL (store);
}

static void
create_selection_tab (GtkWidget *notebook,
                      GimpImage *image)
{
  GimpColorConfig *config;
  GtkSizeGroup    *group;
  GtkWidget       *vbox;
  GtkWidget       *vbox2;
  GtkWidget       *hbox;
  GtkWidget       *grid;
  GtkWidget       *label;
  GtkWidget       *frame;
  GtkWidget       *toggle;
  GtkWidget       *spinbutton;
  GtkAdjustment   *adj;
  GtkWidget       *button;
  GtkWidget       *font_button;
  GList           *image_list;
  gint             j;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), hbox,
                            gtk_label_new_with_mnemonic (_("Selection")));
  gtk_widget_show (hbox);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, FALSE, 0);
  gtk_widget_set_hexpand (vbox2, FALSE);
  gtk_widget_show (vbox2);

  group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* Film height/color */
  frame = gimp_frame_new (_("Filmstrip"));
  gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /* Keep maximum image height */
  toggle = gtk_check_button_new_with_mnemonic (_("_Fit height to images"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &filmvals.keep_height);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  /* Film height */
  adj = gtk_adjustment_new (filmvals.film_height, 10,
                            GIMP_MAX_IMAGE_SIZE, 1, 10, 0);
  spinbutton = gimp_spin_button_new (adj, 1, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);

  label = gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                                    _("_Height:"), 0.0, 0.5,
                                    spinbutton, 1);
  gtk_size_group_add_widget (group, label);
  g_object_unref (group);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &filmvals.film_height);

  g_object_bind_property (toggle,     "active",
                          spinbutton, "sensitive",
                          G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);
  g_object_bind_property (toggle,     "active",
                          /* FIXME: eeeeeek */
                          g_list_nth_data (gtk_container_get_children (GTK_CONTAINER (grid)), 1), "sensitive",
                          G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                filmvals.keep_height);

  /* Film color */
  button = gimp_color_button_new (_("Select Film Color"),
                                  COLOR_BUTTON_WIDTH, COLOR_BUTTON_HEIGHT,
                                  &filmvals.film_color,
                                  GIMP_COLOR_AREA_FLAT);
  label = gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                                    _("Co_lor:"), 0.0, 0.5,
                                    button, 1);
  gtk_size_group_add_widget (group, label);

  g_signal_connect (button, "color-changed",
                    G_CALLBACK (gimp_color_button_get_color),
                    &filmvals.film_color);

  config = gimp_get_color_configuration ();
  gimp_color_button_set_color_config (GIMP_COLOR_BUTTON (button), config);

  /* Film numbering: Startindex/Font/color */
  frame = gimp_frame_new (_("Numbering"));
  gtk_box_pack_start (GTK_BOX (vbox2), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  /* Startindex */
  adj = gtk_adjustment_new (filmvals.number_start, 0,
                            GIMP_MAX_IMAGE_SIZE, 1, 10, 0);
  spinbutton = gimp_spin_button_new (adj, 1, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);

  label = gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                                    _("Start _index:"), 0.0, 0.5,
                                    spinbutton, 1);
  gtk_size_group_add_widget (group, label);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &filmvals.number_start);

  /* Fontfamily for numbering */
  font_button = gimp_font_select_button_new (NULL, filmvals.number_font);
  g_signal_connect (font_button, "font-set",
                    G_CALLBACK (film_font_select_callback), &filmvals);
  label = gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                                    _("_Font:"), 0.0, 0.5,
                                    font_button, 1);
  gtk_size_group_add_widget (group, label);

  /* Numbering color */
  button = gimp_color_button_new (_("Select Number Color"),
                                  COLOR_BUTTON_WIDTH, COLOR_BUTTON_HEIGHT,
                                  &filmvals.number_color,
                                  GIMP_COLOR_AREA_FLAT);
  label = gimp_grid_attach_aligned (GTK_GRID (grid), 0, 2,
                                    _("Co_lor:"), 0.0, 0.5,
                                    button, 1);
  gtk_size_group_add_widget (group, label);

  g_signal_connect (button, "color-changed",
                    G_CALLBACK (gimp_color_button_get_color),
                    &filmvals.number_color);

  gimp_color_button_set_color_config (GIMP_COLOR_BUTTON (button), config);
  g_object_unref (config);

  for (j = 0; j < 2; j++)
    {
      toggle = gtk_check_button_new_with_mnemonic (j ? _("At _bottom")
                                                   : _("At _top"));
      gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                    filmvals.number_pos[j]);
      gtk_widget_show (toggle);

      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &filmvals.number_pos[j]);
    }


  /*** The right frame keeps the image selection ***/
  frame = gimp_frame_new (_("Image Selection"));
  gtk_widget_set_hexpand (frame, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), hbox);

  /* Get a list of all image names */
  image_list = gimp_list_images ();
  filmint.image_list_all = add_image_list (TRUE, image_list, hbox);
  g_list_free (image_list);

  /* Get a list of the images used for the film */
  image_list = g_list_prepend (NULL, image);
  filmint.image_list_film = add_image_list (FALSE, image_list, hbox);
  g_list_free (image_list);

  gtk_widget_show (hbox);
}

static void
create_advanced_tab (GtkWidget *notebook)
{
  GtkWidget     *vbox;
  GtkWidget     *hbox;
  GtkWidget     *grid;
  GtkWidget     *frame;
  GtkAdjustment *adj;
  GtkWidget     *button;
  gint           row;

  frame = gimp_frame_new (_("All Values are Fractions of the Strip Height"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 12);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame,
                            gtk_label_new_with_mnemonic (_("Ad_vanced")));
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  row = 0;

  filmint.advanced_adj[0] = adj =
    gimp_scale_entry_new (GTK_GRID (grid), 0, row++,
                          _("Image _height:"), 0, 0,
                          filmvals.picture_height,
                          0.0, 1.0, 0.001, 0.01, 3,
                          TRUE, 0, 0,
                          NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &filmvals.picture_height);

  filmint.advanced_adj[1] = adj =
    gimp_scale_entry_new (GTK_GRID (grid), 0, row++,
                          _("Image spac_ing:"), 0, 0,
                          filmvals.picture_space,
                          0.0, 1.0, 0.001, 0.01, 3,
                          TRUE, 0, 0,
                          NULL, NULL);
  gtk_widget_set_margin_bottom (gtk_grid_get_child_at (GTK_GRID (grid), 0, 1), 6);
  gtk_widget_set_margin_bottom (gtk_grid_get_child_at (GTK_GRID (grid), 1, 1), 6);
  gtk_widget_set_margin_bottom (gtk_grid_get_child_at (GTK_GRID (grid), 2, 1), 6);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &filmvals.picture_space);

  filmint.advanced_adj[2] = adj =
    gimp_scale_entry_new (GTK_GRID (grid), 0, row++,
                          _("_Hole offset:"), 0, 0,
                          filmvals.hole_offset,
                          0.0, 1.0, 0.001, 0.01, 3,
                          TRUE, 0, 0,
                          NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &filmvals.hole_offset);

  filmint.advanced_adj[3] = adj =
    gimp_scale_entry_new (GTK_GRID (grid), 0, row++,
                          _("Ho_le width:"), 0, 0,
                          filmvals.hole_width,
                          0.0, 1.0, 0.001, 0.01, 3,
                          TRUE, 0, 0,
                          NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &filmvals.hole_width);

  filmint.advanced_adj[4] = adj =
    gimp_scale_entry_new (GTK_GRID (grid), 0, row++,
                          _("Hol_e height:"), 0, 0,
                          filmvals.hole_height,
                          0.0, 1.0, 0.001, 0.01, 3,
                          TRUE, 0, 0,
                          NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &filmvals.hole_height);

  filmint.advanced_adj[5] = adj =
    gimp_scale_entry_new (GTK_GRID (grid), 0, row++,
                          _("Hole sp_acing:"), 0, 0,
                          filmvals.hole_space,
                          0.0, 1.0, 0.001, 0.01, 3,
                          TRUE, 0, 0,
                          NULL, NULL);
  gtk_widget_set_margin_bottom (gtk_grid_get_child_at (GTK_GRID (grid), 0, 5), 6);
  gtk_widget_set_margin_bottom (gtk_grid_get_child_at (GTK_GRID (grid), 1, 5), 6);
  gtk_widget_set_margin_bottom (gtk_grid_get_child_at (GTK_GRID (grid), 2, 5), 6);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &filmvals.hole_space);

  filmint.advanced_adj[6] = adj =
    gimp_scale_entry_new (GTK_GRID (grid), 0, row++,
                          _("_Number height:"), 0, 0,
                          filmvals.number_height,
                          0.0, 1.0, 0.001, 0.01, 3,
                          TRUE, 0, 0,
                          NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &filmvals.number_height);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_mnemonic (_("Re_set"));
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (film_reset_callback),
                    NULL);
}

static gboolean
film_dialog (GimpImage *image)
{
  GtkWidget *dlg;
  GtkWidget *main_vbox;
  GtkWidget *notebook;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY);

  dlg = gimp_dialog_new (_("Filmstrip"), PLUG_IN_ROLE,
                         NULL, 0,
                         gimp_standard_help_func, PLUG_IN_PROC,

                         _("_Cancel"), GTK_RESPONSE_CANCEL,
                         _("_OK"),     GTK_RESPONSE_OK,

                         NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dlg),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dlg));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dlg))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (main_vbox), notebook, TRUE, TRUE, 0);

  create_selection_tab (notebook, image);
  create_advanced_tab (notebook);

  gtk_widget_show (notebook);

  gtk_widget_show (dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  if (run)
    {
      gint        num_images = 0;
      gboolean    iter_valid;
      GtkTreeIter iter;

      for (iter_valid = gtk_tree_model_get_iter_first (filmint.image_list_film,
                                                       &iter);
           iter_valid;
           iter_valid = gtk_tree_model_iter_next (filmint.image_list_film,
                                                  &iter))
        {
          gint image_id;

          gtk_tree_model_get (filmint.image_list_film, &iter,
                              0, &image_id,
                              -1);

          if ((image_id >= 0) && (num_images < MAX_FILM_PICTURES))
            {
              filmvals.images[num_images] = image_id;
              num_images++;
            }
        }

      filmvals.num_images = num_images;
    }

  gtk_widget_destroy (dlg);

  return run;
}

static void
film_reset_callback (GtkWidget *widget,
                     gpointer   data)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS (advanced_defaults) ; i++)
    gtk_adjustment_set_value (filmint.advanced_adj[i],
                              advanced_defaults[i]);
}

static void
film_font_select_callback (GimpFontSelectButton *button,
                           const gchar          *name,
                           gboolean              closing,
                           gpointer              data)
{
  FilmVals *vals = (FilmVals *) data;

  g_strlcpy (vals->number_font, name, FONT_LEN);
}
