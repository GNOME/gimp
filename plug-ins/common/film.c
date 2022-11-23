/* LIGMA - The GNU Image Manipulation Program
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

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "libligma/stdplugins-intl.h"


#define PLUG_IN_PROC        "plug-in-film"
#define PLUG_IN_BINARY      "film"
#define PLUG_IN_ROLE        "ligma-film"

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
  LigmaRGB  film_color;            /* color of film */
  gdouble  picture_height;        /* height of picture (r) */
  gdouble  picture_space;         /* space between pictures (r) */
  gdouble  hole_offset;           /* distance from hole to edge of film (r) */
  gdouble  hole_width;            /* width of hole (r) */
  gdouble  hole_height;           /* height of holes (r) */
  gdouble  hole_space;            /* distance of holes (r) */
  gdouble  number_height;         /* height of picture numbering (r) */
  gint     number_start;          /* number for first picture */
  LigmaRGB  number_color;          /* color of number */
  gchar    number_font[FONT_LEN]; /* font family to use for numbering */
  gint     number_pos[2];         /* flags where to draw numbers (top/bottom) */
  gint     keep_height;           /* flag if to keep max. image height */
  gint     num_images;            /* number of images */
  gint32   images[MAX_FILM_PICTURES]; /* list of image IDs */
} FilmVals;

/* Data to use for the dialog */
typedef struct
{
  GtkWidget     *scales[7];
  GtkTreeModel  *image_list_all;
  GtkTreeModel  *image_list_film;
} FilmInterface;


typedef struct _Film      Film;
typedef struct _FilmClass FilmClass;

struct _Film
{
  LigmaPlugIn parent_instance;
};

struct _FilmClass
{
  LigmaPlugInClass parent_class;
};


#define FILM_TYPE  (film_get_type ())
#define FILM (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), FILM_TYPE, Film))

GType                   film_get_type         (void) G_GNUC_CONST;

static GList          * film_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * film_create_procedure (LigmaPlugIn           *plug_in,
                                               const gchar          *name);

static LigmaValueArray * film_run              (LigmaProcedure        *procedure,
                                               LigmaRunMode           run_mode,
                                               LigmaImage            *image,
                                               gint                  n_drawables,
                                               LigmaDrawable        **drawables,
                                               const LigmaValueArray *args,
                                               gpointer              run_data);

static LigmaImage      * create_new_image      (guint                 width,
                                               guint                 height,
                                               LigmaImageType         gdtype,
                                               LigmaLayer           **layer);

static gchar          * compose_image_name    (LigmaImage            *image);

static LigmaImage      * film                  (void);

static gboolean         check_filmvals        (void);

static void             set_pixels            (gint                  numpix,
                                               guchar               *dst,
                                               LigmaRGB              *color);

static guchar         * create_hole_rgb       (gint                  width,
                                               gint                  height);

static void             draw_number           (LigmaLayer            *layer,
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

static gboolean        film_dialog            (LigmaImage            *image);
static void            film_reset_callback    (GtkWidget            *widget,
                                               gpointer              data);
static void         film_font_select_callback (LigmaFontSelectButton *button,
                                               const gchar          *name,
                                               gboolean              closing,
                                               gpointer              data);

static void    film_scale_entry_update_double (LigmaLabelSpin        *entry,
                                               gdouble              *value);

G_DEFINE_TYPE (Film, film, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (FILM_TYPE)
DEFINE_STD_SET_I18N


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
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = film_query_procedures;
  plug_in_class->create_procedure = film_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
film_init (Film *film)
{
}

static GList *
film_query_procedures (LigmaPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static LigmaProcedure *
film_create_procedure (LigmaPlugIn  *plug_in,
                       const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = ligma_image_procedure_new (plug_in, name,
                                            LIGMA_PDB_PROC_TYPE_PLUGIN,
                                            film_run, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "*");
      ligma_procedure_set_sensitivity_mask (procedure,
                                           LIGMA_PROCEDURE_SENSITIVE_DRAWABLE  |
                                           LIGMA_PROCEDURE_SENSITIVE_DRAWABLES |
                                           LIGMA_PROCEDURE_SENSITIVE_NO_DRAWABLES);

      ligma_procedure_set_menu_label (procedure, _("_Filmstrip..."));
      ligma_procedure_add_menu_path (procedure, "<Image>/Filters/Combine");

      ligma_procedure_set_documentation (procedure,
                                        _("Combine several images on a "
                                          "film strip"),
                                        "Compose several images to a roll film",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Peter Kirchgessner",
                                      "Peter Kirchgessner (peter@kirchgessner.net)",
                                      "1997");

      LIGMA_PROC_ARG_INT (procedure, "film-height",
                         "Film height",
                         "Height of film (0: fit to images)",
                         0, LIGMA_MAX_IMAGE_SIZE, 0,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_RGB (procedure, "film-color",
                         "Film color",
                         "Color of the film",
                         TRUE, NULL,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "number-start",
                         "Number start",
                         "Start index for numbering",
                         G_MININT, G_MAXINT, 1,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_STRING (procedure, "number-font",
                            "Number font",
                            "Font for drawing numbers",
                            NULL,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_RGB (procedure, "number-color",
                         "Number color",
                         "Color for numbers",
                         TRUE, NULL,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "at-top",
                             "At top",
                             "Draw numbers at top",
                             TRUE,
                             G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "at-bottom",
                             "At bottom",
                             "Draw numbers at bottom",
                             TRUE,
                             G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "num-images",
                         "Num images",
                         "Number of images to be used for film",
                         1, MAX_FILM_PICTURES, 1,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_OBJECT_ARRAY (procedure, "images",
                                  "Images",
                                  "num-images images to be used for film",
                                  LIGMA_TYPE_IMAGE,
                                  G_PARAM_READWRITE);

      LIGMA_PROC_VAL_IMAGE (procedure, "new-image",
                           "New image",
                           "Output image",
                           FALSE,
                           G_PARAM_READWRITE);
    }

  return procedure;
}

static LigmaValueArray *
film_run (LigmaProcedure        *procedure,
          LigmaRunMode           run_mode,
          LigmaImage            *image,
          gint                  n_drawables,
          LigmaDrawable        **drawables,
          const LigmaValueArray *args,
          gpointer              run_data)
{
  LigmaValueArray     *return_vals = NULL;
  LigmaPDBStatusType   status      = LIGMA_PDB_SUCCESS;
  LigmaImage         **images;
  gint                i;

  gegl_init (NULL, NULL);

  switch (run_mode)
    {
    case LIGMA_RUN_INTERACTIVE:
      ligma_get_data (PLUG_IN_PROC, &filmvals);

      if (! film_dialog (image))
        {
          return ligma_procedure_new_return_values (procedure, LIGMA_PDB_CANCEL,
                                                   NULL);
        }
      break;

    case LIGMA_RUN_NONINTERACTIVE:
      filmvals.film_height = LIGMA_VALUES_GET_INT (args, 0);
      if (filmvals.film_height <= 0)
        {
          filmvals.keep_height = TRUE;
          filmvals.film_height = 128; /* arbitrary */
        }
      else
        {
          filmvals.keep_height = FALSE;
        }
      LIGMA_VALUES_GET_RGB (args, 1, &filmvals.film_color);
      filmvals.number_start = LIGMA_VALUES_GET_INT           (args, 2);
      g_strlcpy              (filmvals.number_font,
                              LIGMA_VALUES_GET_STRING        (args, 3),
                              FONT_LEN);
      LIGMA_VALUES_GET_RGB (args, 4, &filmvals.number_color);
      filmvals.number_pos[0] = LIGMA_VALUES_GET_INT          (args, 5);
      filmvals.number_pos[1] = LIGMA_VALUES_GET_INT          (args, 6);
      filmvals.num_images    = LIGMA_VALUES_GET_INT          (args, 7);
      images                 = LIGMA_VALUES_GET_OBJECT_ARRAY (args, 8);

      for (i = 0; i < filmvals.num_images; i++)
        filmvals.images[i] = ligma_image_get_id (images[i]);
      break;

    case LIGMA_RUN_WITH_LAST_VALS:
      ligma_get_data (PLUG_IN_PROC, &filmvals);
      break;

    default:
      break;
    }

  if (! check_filmvals ())
    status = LIGMA_PDB_CALLING_ERROR;

  if (status == LIGMA_PDB_SUCCESS)
    {
      LigmaImage *image;

      ligma_progress_init (_("Composing images"));

      image = film ();

      if (! image)
        {
          status = LIGMA_PDB_EXECUTION_ERROR;
        }
      else
        {
          return_vals = ligma_procedure_new_return_values (procedure, status,
                                                          NULL);

          LIGMA_VALUES_SET_IMAGE (return_vals, 1, image);

          ligma_image_undo_enable (image);
          ligma_image_clean_all (image);

          if (run_mode != LIGMA_RUN_NONINTERACTIVE)
            ligma_display_new (image);
        }

      /*  Store data  */
      if (run_mode == LIGMA_RUN_INTERACTIVE)
        ligma_set_data (PLUG_IN_PROC, &filmvals, sizeof (FilmVals));
    }

  if (! return_vals)
    return_vals = ligma_procedure_new_return_values (procedure, status, NULL);

  return return_vals;
}

/* Compose a roll film image from several images */
static LigmaImage *
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
  LigmaImage    *image_dst;
  LigmaImage    *image_tmp;
  LigmaLayer    *layer_src;
  LigmaLayer    *layer_dst;
  LigmaLayer    *new_layer;
  LigmaLayer    *floating_sel;

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
                                  ligma_image_get_by_id (filmvals.images[i]));
    }

  ligma_context_push ();
  ligma_context_set_foreground (&filmvals.number_color);
  ligma_context_set_background (&filmvals.film_color);

  if (filmvals.keep_height) /* Search maximum picture height */
    {
      picture_height = 0;
      for (iter = images_src; iter; iter = iter->next)
        {
          height = ligma_image_get_height (iter->data);
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
      layers = ligma_image_list_layers (iter->data);

      /* Get scaled image size */
      width = ligma_image_get_width (iter->data);
      height = ligma_image_get_height (iter->data);
      f = ((double)picture_height) / (double)height;
      picture_width = width * f;

      for (iter2 = layers; iter2; iter2 = g_list_next (iter2))
        {
          if (ligma_layer_is_floating_sel (iter2->data))
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
                                LIGMA_RGB_IMAGE, &layer_dst);

  /* Fill film background */
  ligma_drawable_fill (LIGMA_DRAWABLE (layer_dst), LIGMA_FILL_BACKGROUND);

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
      GeglBuffer *buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (layer_dst));

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
      image_tmp = ligma_image_duplicate (iter->data);
      width = ligma_image_get_width (image_tmp);
      height = ligma_image_get_height (image_tmp);
      f = ((gdouble) picture_height) / (gdouble) height;
      picture_width = width * f;
      if (ligma_image_get_base_type (image_tmp) != LIGMA_RGB)
        ligma_image_convert_rgb (image_tmp);
      ligma_image_scale (image_tmp, picture_width, picture_height);

      layers = ligma_image_list_layers (image_tmp);

      for (iter2 = layers; iter2; iter2 = g_list_next (iter2))
        {
          if (ligma_layer_is_floating_sel (iter2->data))
            continue;

          picture_x0 += picture_space / 2;

          layer_src = iter2->data;
          ligma_layer_resize_to_image_size (layer_src);
          new_layer = ligma_layer_new_from_drawable (LIGMA_DRAWABLE (layer_src),
                                                    image_dst);
          ligma_image_insert_layer (image_dst, new_layer, NULL, -1);
          ligma_layer_set_offsets (new_layer, picture_x0, picture_y0);

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

          ligma_progress_update (((gdouble) (picture_count + 1)) /
                                (gdouble) num_pictures);

          picture_count++;
        }

      g_list_free (layers);

      ligma_image_delete (image_tmp);
    }

  g_list_free (images_src);

  ligma_progress_update (1.0);

  ligma_image_flatten (image_dst);

  /* Drawing text/numbers leaves us with a floating selection. Stop it */
  floating_sel = ligma_image_get_floating_sel (image_dst);
  if (floating_sel)
    ligma_floating_sel_anchor (floating_sel);

  ligma_context_pop ();

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
      if (ligma_image_id_is_valid (filmvals.images[i]))
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
            LigmaRGB *color)
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
draw_number (LigmaLayer *layer,
             gint       num,
             gint       x,
             gint       y,
             gint       height)
{
  gchar         buf[32];
  gint          k, delta, max_delta;
  LigmaImage    *image;
  LigmaLayer    *text_layer;
  gint          text_width, text_height, text_ascent, descent;
  gchar        *fontname = filmvals.number_font;

  g_snprintf (buf, sizeof (buf), "%d", num);

  image = ligma_item_get_image (LIGMA_ITEM (layer));

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

      success = ligma_text_get_extents_fontname (buf,
                                                height + delta, LIGMA_PIXELS,
                                                fontname,
                                                &text_width, &text_height,
                                                &text_ascent, &descent);

      if (success)
        {
          height += delta;
          break;
        }
    }

  text_layer = ligma_text_fontname (image, LIGMA_DRAWABLE (layer),
                                   x, y + descent / 2,
                                   buf, 1, FALSE,
                                   height, LIGMA_PIXELS,
                                   fontname);

  if (! text_layer)
    g_message ("draw_number: Error in drawing text\n");
}

/* Create an image. Sets layer, drawable and rgn. Returns image */
static LigmaImage *
create_new_image (guint           width,
                  guint           height,
                  LigmaImageType   gdtype,
                  LigmaLayer     **layer)
{
  LigmaImage        *image;
  LigmaImageBaseType gitype;

  if ((gdtype == LIGMA_GRAY_IMAGE) || (gdtype == LIGMA_GRAYA_IMAGE))
    gitype = LIGMA_GRAY;
  else if ((gdtype == LIGMA_INDEXED_IMAGE) || (gdtype == LIGMA_INDEXEDA_IMAGE))
    gitype = LIGMA_INDEXED;
  else
    gitype = LIGMA_RGB;

  image = ligma_image_new (width, height, gitype);

  ligma_image_undo_disable (image);
  *layer = ligma_layer_new (image, _("Background"), width, height,
                           gdtype,
                           100,
                           ligma_image_get_default_new_layer_mode (image));
  ligma_image_insert_layer (image, *layer, NULL, 0);

  return image;
}

static gchar *
compose_image_name (LigmaImage *image)
{
  gchar *image_name;
  gchar *name;

  /* Compose a name of the basename and the image-ID */

  name = ligma_image_get_name (image);

  image_name = g_strdup_printf ("%s-%d", name, ligma_image_get_id (image));

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
          LigmaImage *image;
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
                          0, ligma_image_get_id (list->data),
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
                      LigmaImage *image)
{
  LigmaColorConfig *config;
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
  frame = ligma_frame_new (_("Filmstrip"));
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
                    G_CALLBACK (ligma_toggle_button_update),
                    &filmvals.keep_height);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  /* Film height */
  adj = gtk_adjustment_new (filmvals.film_height, 10,
                            LIGMA_MAX_IMAGE_SIZE, 1, 10, 0);
  spinbutton = ligma_spin_button_new (adj, 1, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);

  label = ligma_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                                    _("_Height:"), 0.0, 0.5,
                                    spinbutton, 1);
  gtk_size_group_add_widget (group, label);
  g_object_unref (group);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (ligma_int_adjustment_update),
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
  button = ligma_color_button_new (_("Select Film Color"),
                                  COLOR_BUTTON_WIDTH, COLOR_BUTTON_HEIGHT,
                                  &filmvals.film_color,
                                  LIGMA_COLOR_AREA_FLAT);
  label = ligma_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                                    _("Co_lor:"), 0.0, 0.5,
                                    button, 1);
  gtk_size_group_add_widget (group, label);

  g_signal_connect (button, "color-changed",
                    G_CALLBACK (ligma_color_button_get_color),
                    &filmvals.film_color);

  config = ligma_get_color_configuration ();
  ligma_color_button_set_color_config (LIGMA_COLOR_BUTTON (button), config);

  /* Film numbering: Startindex/Font/color */
  frame = ligma_frame_new (_("Numbering"));
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
                            LIGMA_MAX_IMAGE_SIZE, 1, 10, 0);
  spinbutton = ligma_spin_button_new (adj, 1, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);

  label = ligma_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                                    _("Start _index:"), 0.0, 0.5,
                                    spinbutton, 1);
  gtk_size_group_add_widget (group, label);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (ligma_int_adjustment_update),
                    &filmvals.number_start);

  /* Fontfamily for numbering */
  font_button = ligma_font_select_button_new (NULL, filmvals.number_font);
  g_signal_connect (font_button, "font-set",
                    G_CALLBACK (film_font_select_callback), &filmvals);
  label = ligma_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                                    _("_Font:"), 0.0, 0.5,
                                    font_button, 1);
  gtk_size_group_add_widget (group, label);

  /* Numbering color */
  button = ligma_color_button_new (_("Select Number Color"),
                                  COLOR_BUTTON_WIDTH, COLOR_BUTTON_HEIGHT,
                                  &filmvals.number_color,
                                  LIGMA_COLOR_AREA_FLAT);
  label = ligma_grid_attach_aligned (GTK_GRID (grid), 0, 2,
                                    _("Co_lor:"), 0.0, 0.5,
                                    button, 1);
  gtk_size_group_add_widget (group, label);

  g_signal_connect (button, "color-changed",
                    G_CALLBACK (ligma_color_button_get_color),
                    &filmvals.number_color);

  ligma_color_button_set_color_config (LIGMA_COLOR_BUTTON (button), config);
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
                        G_CALLBACK (ligma_toggle_button_update),
                        &filmvals.number_pos[j]);
    }


  /*** The right frame keeps the image selection ***/
  frame = ligma_frame_new (_("Image Selection"));
  gtk_widget_set_hexpand (frame, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), hbox);

  /* Get a list of all image names */
  image_list = ligma_list_images ();
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
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *grid;
  GtkWidget *frame;
  GtkWidget *scale;
  GtkWidget *button;
  gint       row;

  frame = ligma_frame_new (_("All Values are Fractions of the Strip Height"));
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

  filmint.scales[0] = scale =
    ligma_scale_entry_new (_("Image _height:"), filmvals.picture_height, 0.0, 1.0, 3);
  ligma_label_spin_set_increments (LIGMA_LABEL_SPIN (scale), 0.001, 0.01);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (film_scale_entry_update_double),
                    &filmvals.picture_height);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_show (scale);

  filmint.scales[1] = scale =
    ligma_scale_entry_new (_("Image spac_ing:"), filmvals.picture_space, 0.0, 1.0, 3);
  ligma_label_spin_set_increments (LIGMA_LABEL_SPIN (scale), 0.001, 0.01);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (film_scale_entry_update_double),
                    &filmvals.picture_space);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_set_margin_bottom (ligma_labeled_get_label (LIGMA_LABELED (scale)), 6);
  gtk_widget_set_margin_bottom (ligma_scale_entry_get_range (LIGMA_SCALE_ENTRY (scale)), 6);
  gtk_widget_set_margin_bottom (ligma_label_spin_get_spin_button (LIGMA_LABEL_SPIN (scale)), 6);
  gtk_widget_show (scale);

  filmint.scales[2] = scale =
    ligma_scale_entry_new (_("_Hole offset:"), filmvals.hole_offset, 0.0, 1.0, 3);
  ligma_label_spin_set_increments (LIGMA_LABEL_SPIN (scale), 0.001, 0.01);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (film_scale_entry_update_double),
                    &filmvals.hole_offset);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_show (scale);

  filmint.scales[3] = scale =
    ligma_scale_entry_new (_("Ho_le width:"), filmvals.hole_width, 0.0, 1.0, 3);
  ligma_label_spin_set_increments (LIGMA_LABEL_SPIN (scale), 0.001, 0.01);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (film_scale_entry_update_double),
                    &filmvals.hole_width);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_show (scale);

  filmint.scales[4] = scale =
    ligma_scale_entry_new (_("Hol_e height:"), filmvals.hole_height, 0.0, 1.0, 3);
  ligma_label_spin_set_increments (LIGMA_LABEL_SPIN (scale), 0.001, 0.01);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (film_scale_entry_update_double),
                    &filmvals.hole_height);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_show (scale);

  filmint.scales[5] = scale =
    ligma_scale_entry_new (_("Hole sp_acing:"), filmvals.hole_space, 0.0, 1.0, 3);
  ligma_label_spin_set_increments (LIGMA_LABEL_SPIN (scale), 0.001, 0.01);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (film_scale_entry_update_double),
                    &filmvals.hole_space);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_set_margin_bottom (ligma_labeled_get_label (LIGMA_LABELED (scale)), 6);
  gtk_widget_set_margin_bottom (ligma_scale_entry_get_range (LIGMA_SCALE_ENTRY (scale)), 6);
  gtk_widget_set_margin_bottom (ligma_label_spin_get_spin_button (LIGMA_LABEL_SPIN (scale)), 6);
  gtk_widget_show (scale);

  filmint.scales[6] = scale =
    ligma_scale_entry_new (_("_Number height:"), filmvals.number_height, 0.0, 1.0, 3);
  ligma_label_spin_set_increments (LIGMA_LABEL_SPIN (scale), 0.001, 0.01);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (film_scale_entry_update_double),
                    &filmvals.number_height);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_show (scale);

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
film_dialog (LigmaImage *image)
{
  GtkWidget *dlg;
  GtkWidget *main_vbox;
  GtkWidget *notebook;
  gboolean   run;

  ligma_ui_init (PLUG_IN_BINARY);

  dlg = ligma_dialog_new (_("Filmstrip"), PLUG_IN_ROLE,
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

  run = (ligma_dialog_run (LIGMA_DIALOG (dlg)) == GTK_RESPONSE_OK);

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
    ligma_label_spin_set_value (LIGMA_LABEL_SPIN (filmint.scales[i]),
                               advanced_defaults[i]);
}

static void
film_font_select_callback (LigmaFontSelectButton *button,
                           const gchar          *name,
                           gboolean              closing,
                           gpointer              data)
{
  FilmVals *vals = (FilmVals *) data;

  g_strlcpy (vals->number_font, name, FONT_LEN);
}

static void
film_scale_entry_update_double (LigmaLabelSpin *entry,
                                gdouble       *value)
{
  *value = ligma_label_spin_get_value (entry);
}
