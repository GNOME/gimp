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
#define MAX_FILM_PICTURES   1024
#define COLOR_BUTTON_WIDTH  50
#define COLOR_BUTTON_HEIGHT 20

#define FONT_LEN            256

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
                                               gint                  n_drawables,
                                               GimpDrawable        **drawables,
                                               GimpProcedureConfig  *config,
                                               gpointer              run_data);

static GimpImage      * create_new_image      (guint                 width,
                                               guint                 height,
                                               GimpImageType         gdtype,
                                               GimpLayer           **layer);

static gchar          * compose_image_name    (GimpImage            *image);

static GimpImage      * film                  (GimpProcedureConfig  *config);

static gboolean         check_filmvals        (GimpProcedureConfig  *config);

static void             set_pixels            (gint                  numpix,
                                               guchar               *dst,
                                               GimpRGB              *color);

static guchar         * create_hole_rgb       (gint                  width,
                                               gint                  height,
                                               GimpRGB              *film_color);

static void             draw_number           (GimpLayer            *layer,
                                               gint                  num,
                                               gint                  x,
                                               gint                  y,
                                               gint                  height,
                                               GimpFont             *font);


static void            add_list_item_callback (GtkWidget            *widget,
                                               GtkTreeSelection     *sel);
static void            del_list_item_callback (GtkWidget            *widget,
                                               GtkTreeSelection     *sel);

static GtkTreeModel  * add_image_list         (gboolean              add_box_flag,
                                               GList                *images,
                                               GtkWidget            *hbox);

static gboolean        film_dialog            (GimpImage            *image,
                                               GimpProcedureConfig  *config);
static void            film_reset_callback    (GtkWidget            *widget,
                                               gpointer              data);
static void         film_font_select_callback (GimpFontSelectButton *button,
                                               GimpResource         *resource,
                                               gboolean              closing,
                                               GimpFont            **font);

static void    film_scale_entry_update_double (GimpLabelSpin        *entry,
                                               gdouble              *value);

G_DEFINE_TYPE (Film, film, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (FILM_TYPE)
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
  plug_in_class->set_i18n         = STD_SET_I18N;
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
  const GimpRGB  default_film_color   = { 0.0, 0.0, 0.0, 1.0 };
  const GimpRGB  default_number_color = { 0.93, 0.61, 0.0, 1.0 };

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new2 (plug_in, name,
                                             GIMP_PDB_PROC_TYPE_PLUGIN,
                                             film_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE  |
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLES |
                                           GIMP_PROCEDURE_SENSITIVE_NO_DRAWABLES);

      gimp_procedure_set_menu_label (procedure, _("_Filmstrip..."));
      gimp_procedure_add_menu_path (procedure, "<Image>/Filters/Combine");

      gimp_procedure_set_documentation (procedure,
                                        _("Combine several images on a "
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
                         TRUE, &default_film_color,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "number-start",
                         "Number start",
                         "Start index for numbering",
                         0, G_MAXINT, 1,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_FONT (procedure, "number-font",
                          "Number font",
                          "Font for drawing numbers",
                          G_PARAM_READWRITE);

      GIMP_PROC_ARG_RGB (procedure, "number-color",
                         "Number color",
                         "Color for numbers",
                         TRUE, &default_number_color,
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

      /* Arguments ignored in interactive mode. */

      GIMP_PROC_ARG_INT (procedure, "num-images",
                         "Number of images",
                         "Number of images to be used for film",
                         1, MAX_FILM_PICTURES, 1,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_OBJECT_ARRAY (procedure, "images",
                                  "Images",
                                  "num-images images to be used for film",
                                  GIMP_TYPE_IMAGE,
                                  G_PARAM_READWRITE);

      /* The more specific settings. */

      GIMP_PROC_ARG_DOUBLE (procedure, "picture-height",
                            "Picture height",
                            "As fraction of the strip height",
                            0.0, 1.0, 0.695,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "picture-spacing",
                            "Picture spacing",
                            "The spacing between 2 images, as fraction of the strip height",
                            0.0, 1.0, 0.040,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "hole-offset",
                            "Hole offset",
                            "Hole offset to edge of film, as fraction of the strip height",
                            0.0, 1.0, 0.058,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "hole-width",
                            "Hole width",
                            "Hole width, as fraction of the strip height",
                            0.0, 1.0, 0.052,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "hole-height",
                            "Hole height",
                            "Hole height, as fraction of the strip height",
                            0.0, 1.0, 0.081,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "hole-spacing",
                            "Hole distance",
                            "Hole distance, as fraction of the strip height",
                            0.0, 1.0, 0.081,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "number-height",
                            "Number height",
                            "Image number height, as fraction of the strip height",
                            0.0, 1.0, 0.052,
                            G_PARAM_READWRITE);

      /* Auxiliary argument mostly for the GUI. */

      GIMP_PROC_AUX_ARG_BOOLEAN (procedure, "keep-height",
                                 _("_Fit height to images"),
                                 "Keep maximum image height",
                                 FALSE,
                                 G_PARAM_READWRITE);

      /* Returned image. */

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
          gint                  n_drawables,
          GimpDrawable        **drawables,
          GimpProcedureConfig  *config,
          gpointer              run_data)
{
  GimpValueArray     *return_vals = NULL;
  GimpPDBStatusType   status      = GIMP_PDB_SUCCESS;
  gint                film_height;

  gegl_init (NULL, NULL);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      if (! film_dialog (image, config))
        return gimp_procedure_new_return_values (procedure, GIMP_PDB_CANCEL,
                                                 NULL);
      break;

    case GIMP_RUN_NONINTERACTIVE:
      g_object_get (config, "film-height", &film_height, NULL);
      if (film_height <= 0)
        g_object_set (config,
                      "film-height", 128, /* arbitrary */
                      "keep-height", TRUE,
                      NULL);
      else
        g_object_set (config,
                      "keep-height", FALSE,
                      NULL);

      break;

    case GIMP_RUN_WITH_LAST_VALS:
      break;

    default:
      break;
    }

  if (! check_filmvals (config))
    status = GIMP_PDB_CALLING_ERROR;

  if (status == GIMP_PDB_SUCCESS)
    {
      GimpImage *image;

      gimp_progress_init (_("Composing images"));

      image = film (config);

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
    }

  if (! return_vals)
    return_vals = gimp_procedure_new_return_values (procedure, status, NULL);

  return return_vals;
}

/* Compose a roll film image from several images */
static GimpImage *
film (GimpProcedureConfig *config)
{
  GimpObjectArray *images;
  gint             width, height;
  guchar          *hole;
  gint             film_height;
  gint             film_width;
  gint             picture_width;
  gdouble          picture_height;
  gdouble          picture_space;
  gint             picture_x0;
  gint             picture_y0;
  gdouble          hole_offset;
  gdouble          hole_width;
  gdouble          hole_height;
  gdouble          hole_space;
  gint             hole_x;
  gdouble          number_height;
  gint             num_pictures;
  gint             number_start;
  gboolean         at_top;
  gboolean         at_bottom;
  gint             picture_count;
  GimpRGB         *number_color;
  GimpRGB         *film_color;
  gboolean         keep_height;
  gdouble          f;
  GimpImage       *image_dst;
  GimpImage       *image_tmp;
  GimpLayer       *layer_src;
  GimpLayer       *layer_dst;
  GimpLayer       *new_layer;
  GimpLayer       *floating_sel;

  GimpFont        *number_font;

  GList           *layers     = NULL;
  GList           *iter2;
  gint             i;

  g_object_get (config,
                "images",          &images,
                "film-height",     &film_height,
                "keep-height",     &keep_height,
                "number-color",    &number_color,
                "number-font",     &number_font,
                "film-color",      &film_color,
                "picture-height",  &picture_height,
                "picture-spacing", &picture_space,
                "number-height",   &number_height,
                "hole-offset",     &hole_offset,
                "hole-width",      &hole_width,
                "hole-height",     &hole_height,
                "hole-spacing",    &hole_space,
                "number-start",    &number_start,
                "at-top",          &at_top,
                "at-bottom",       &at_bottom,
                NULL);

  if (images->length <= 0)
    return NULL;

  gimp_context_push ();
  gimp_context_set_foreground (number_color);
  gimp_context_set_background (film_color);

  if (keep_height) /* Search maximum picture height */
    {
      gdouble max_height = 0;

      for (i = 0; i < images->length; i++)
        {
          GimpImage *image = GIMP_IMAGE (images->data[i]);

          height = gimp_image_get_height (image);
          if ((gdouble) height > max_height)
            max_height = (gdouble) height;
        }
      film_height = (int) (max_height / picture_height + 0.5);
    }

  picture_height = film_height * picture_height + 0.5;

  picture_space = (gdouble) film_height * picture_space + 0.5;
  picture_y0 = (film_height - picture_height) / 2;

  number_height = film_height * number_height;

  /* Calculate total film width */
  film_width = 0;
  num_pictures = 0;
  for (i = 0; i < images->length; i++)
    {
      GimpImage *image = GIMP_IMAGE (images->data[i]);

      layers = gimp_image_list_layers (image);

      /* Get scaled image size */
      width = gimp_image_get_width (image);
      height = gimp_image_get_height (image);
      f = picture_height / (gdouble) height;
      picture_width = (gint) width * f;

      for (iter2 = layers; iter2; iter2 = g_list_next (iter2))
        {
          if (gimp_layer_is_floating_sel (iter2->data))
            continue;

          film_width += (gint) (picture_space / 2.0);  /* Leading space */
          film_width += picture_width;      /* Scaled image width */
          film_width += (gint) (picture_space / 2.0);  /* Trailing space */
          num_pictures++;
        }

      g_list_free (layers);
    }

#ifdef FILM_DEBUG
  g_printerr ("film_height = %d, film_width = %d\n", film_height, film_width);
  g_printerr ("picture_height = %f, picture_space = %f, picture_y0 = %d\n",
              picture_height, picture_space, picture_y0);
  g_printerr ("Number of pictures = %d\n", num_pictures);
#endif

  image_dst = create_new_image ((guint) film_width, (guint) film_height,
                                GIMP_RGB_IMAGE, &layer_dst);

  /* Fill film background */
  gimp_drawable_fill (GIMP_DRAWABLE (layer_dst), GIMP_FILL_BACKGROUND);

  /* Draw all the holes */
  hole_offset = film_height * hole_offset;
  hole_width  = film_height * hole_width;
  hole_height = film_height * hole_height;
  hole_space  = film_height * hole_space;
  hole_x = hole_space / 2;

#ifdef FILM_DEBUG
  g_printerr ("hole_x %d hole_offset %d hole_width %d hole_height %d hole_space %d\n",
              hole_x, hole_offset, hole_width, hole_height, hole_space );
#endif

  hole = create_hole_rgb ((gint) hole_width, (gint) hole_height, film_color);
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
  for (i = 0; i < images->length; i++)
    {
      GimpImage *image = GIMP_IMAGE (images->data[i]);

      image_tmp = gimp_image_duplicate (image);
      width = gimp_image_get_width (image_tmp);
      height = gimp_image_get_height (image_tmp);
      f = ((gdouble) picture_height) / (gdouble) height;
      picture_width = width * f;
      if (gimp_image_get_base_type (image_tmp) != GIMP_RGB)
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
          if (number_height > 0.0 && (at_top || at_bottom))
            {
              if (at_top)
                draw_number (layer_dst,
                             number_start + picture_count,
                             picture_x0 + picture_width/2,
                             (hole_offset-number_height)/2, number_height,
                             number_font);
              if (at_bottom)
                draw_number (layer_dst,
                             number_start + picture_count,
                             picture_x0 + picture_width/2,
                             film_height - (hole_offset + number_height)/2,
                             number_height,
                             number_font);
            }

          picture_x0 += picture_width + (picture_space/2);

          gimp_progress_update (((gdouble) (picture_count + 1)) /
                                (gdouble) num_pictures);

          picture_count++;
        }

      g_list_free (layers);

      gimp_image_delete (image_tmp);
    }

  gimp_progress_update (1.0);

  gimp_image_flatten (image_dst);

  /* Drawing text/numbers leaves us with a floating selection. Stop it */
  floating_sel = gimp_image_get_floating_sel (image_dst);
  if (floating_sel)
    gimp_floating_sel_anchor (floating_sel);

  gimp_context_pop ();

  gimp_object_array_free (images);
  g_free (number_color);
  g_free (film_color);
  g_clear_object (&number_font);

  return image_dst;
}

/* Unreasonable values are reset to a default. */
/* If this is not possible, FALSE is returned. Otherwise TRUE is returned. */
static gboolean
check_filmvals (GimpProcedureConfig *config)
{
  GimpFont        *font;
  GimpObjectArray *images;
  gint             num_images;
  gint             film_height;
  gint             i, j;
  gboolean         success = FALSE;

  g_object_get (config,
                "num-images",  &num_images,
                "images",      &images,
                "number-font", &font,
                "film-height", &film_height,
                NULL);

  if (film_height < 10)
    g_object_set (config, "film-height", 10, NULL);

  if (font == NULL)
    {
      g_assert (GIMP_IS_FONT (gimp_context_get_font ()));
      g_object_set (config, "number-font", gimp_context_get_font (), NULL);
    }

  for (i = 0, j = 0; i < num_images; i++)
    {
      if (gimp_image_is_valid (GIMP_IMAGE (images->data[i])))
        {
          images->data[j] = images->data[i];
          j++;
        }
    }

  if (j > 0)
    {
      images->length = j;
      g_object_set (config,
                    "num-images", j,
                    "images",     images,
                    NULL);
      success = TRUE;
    }

  gimp_object_array_free (images);

  return success;
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
create_hole_rgb (gint     width,
                 gint     height,
                 GimpRGB *film_color)
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
          set_pixels (length, top, film_color);
          set_pixels (length, top + (width-length)*3, film_color);
          set_pixels (length, bottom, film_color);
          set_pixels (length, bottom + (width-length)*3, film_color);
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
             gint       height,
             GimpFont  *font)
{
  gchar         buf[32];
  gint          k, delta, max_delta;
  GimpImage    *image;
  GimpLayer    *text_layer;
  gint          text_width, text_height, text_ascent, descent;

  gchar        *fontname;

  fontname = gimp_resource_get_name (GIMP_RESOURCE (font));

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
create_selection_tab (GtkWidget            *notebook,
                      GimpImage            *image,
                      GimpProcedureConfig  *config,
                      gint                 *film_height,
                      gint                 *number_start,
                      GimpFont            **number_font,
                      GimpRGB              *number_color,
                      GimpRGB              *film_color,
                      gboolean             *keep_height,
                      gboolean             *at_top,
                      gboolean             *at_bottom)
{
  GimpColorConfig *color_config;
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
                    keep_height);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  /* Film height */
  adj = gtk_adjustment_new (*film_height, 10,
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
                    film_height);

  g_object_bind_property (toggle,     "active",
                          spinbutton, "sensitive",
                          G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);
  g_object_bind_property (toggle,     "active",
                          /* FIXME: eeeeeek */
                          g_list_nth_data (gtk_container_get_children (GTK_CONTAINER (grid)), 1), "sensitive",
                          G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), *keep_height);

  /* Film color */
  button = gimp_color_button_new (_("Select Film Color"),
                                  COLOR_BUTTON_WIDTH, COLOR_BUTTON_HEIGHT,
                                  film_color,
                                  GIMP_COLOR_AREA_FLAT);
  label = gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                                    _("Co_lor:"), 0.0, 0.5,
                                    button, 1);
  gtk_size_group_add_widget (group, label);

  g_signal_connect (button, "color-changed",
                    G_CALLBACK (gimp_color_button_get_color),
                    film_color);

  color_config = gimp_get_color_configuration ();
  gimp_color_button_set_color_config (GIMP_COLOR_BUTTON (button), color_config);

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
  adj = gtk_adjustment_new (*number_start, 0,
                            GIMP_MAX_IMAGE_SIZE, 1, 10, 0);
  spinbutton = gimp_spin_button_new (adj, 1, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);

  label = gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                                    _("Start _index:"), 0.0, 0.5,
                                    spinbutton, 1);
  gtk_size_group_add_widget (group, label);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    number_start);

  /* Fontfamily for numbering */
  /* Require *number_font is NULL or a valid GimpFont. */
  font_button = gimp_font_select_button_new (NULL, GIMP_RESOURCE (*number_font));
  g_signal_connect (font_button, "resource-set",
                    G_CALLBACK (film_font_select_callback), number_font);
  label = gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                                    _("_Font:"), 0.0, 0.5,
                                    font_button, 1);
  gtk_size_group_add_widget (group, label);

  /* Numbering color */
  button = gimp_color_button_new (_("Select Number Color"),
                                  COLOR_BUTTON_WIDTH, COLOR_BUTTON_HEIGHT,
                                  number_color,
                                  GIMP_COLOR_AREA_FLAT);
  label = gimp_grid_attach_aligned (GTK_GRID (grid), 0, 2,
                                    _("Co_lor:"), 0.0, 0.5,
                                    button, 1);
  gtk_size_group_add_widget (group, label);

  g_signal_connect (button, "color-changed",
                    G_CALLBACK (gimp_color_button_get_color),
                    number_color);

  gimp_color_button_set_color_config (GIMP_COLOR_BUTTON (button), color_config);
  g_object_unref (color_config);

  /* Numbering positions */

  toggle = gtk_check_button_new_with_mnemonic (_("At _top"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), *at_top);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    at_top);

  toggle = gtk_check_button_new_with_mnemonic (_("At _bottom"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), *at_bottom);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    at_bottom);

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
create_advanced_tab (GtkWidget           *notebook,
                     GimpProcedureConfig *config,
                     gdouble             *picture_height,
                     gdouble             *picture_space,
                     gdouble             *number_height,
                     gdouble             *hole_offset,
                     gdouble             *hole_width,
                     gdouble             *hole_height,
                     gdouble             *hole_space)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *grid;
  GtkWidget *frame;
  GtkWidget *scale;
  GtkWidget *button;
  gint       row;

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

  filmint.scales[0] = scale =
    gimp_scale_entry_new (_("Image _height:"), *picture_height, 0.0, 1.0, 3);
  gimp_label_spin_set_increments (GIMP_LABEL_SPIN (scale), 0.001, 0.01);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (film_scale_entry_update_double),
                    picture_height);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_show (scale);

  filmint.scales[1] = scale =
    gimp_scale_entry_new (_("Image spac_ing:"), *picture_space, 0.0, 1.0, 3);
  gimp_label_spin_set_increments (GIMP_LABEL_SPIN (scale), 0.001, 0.01);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (film_scale_entry_update_double),
                    picture_space);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_set_margin_bottom (gimp_labeled_get_label (GIMP_LABELED (scale)), 6);
  gtk_widget_set_margin_bottom (gimp_scale_entry_get_range (GIMP_SCALE_ENTRY (scale)), 6);
  gtk_widget_set_margin_bottom (gimp_label_spin_get_spin_button (GIMP_LABEL_SPIN (scale)), 6);
  gtk_widget_show (scale);

  filmint.scales[2] = scale =
    gimp_scale_entry_new (_("_Hole offset:"), *hole_offset, 0.0, 1.0, 3);
  gimp_label_spin_set_increments (GIMP_LABEL_SPIN (scale), 0.001, 0.01);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (film_scale_entry_update_double),
                    hole_offset);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_show (scale);

  filmint.scales[3] = scale =
    gimp_scale_entry_new (_("Ho_le width:"), *hole_width, 0.0, 1.0, 3);
  gimp_label_spin_set_increments (GIMP_LABEL_SPIN (scale), 0.001, 0.01);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (film_scale_entry_update_double),
                    hole_width);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_show (scale);

  filmint.scales[4] = scale =
    gimp_scale_entry_new (_("Hol_e height:"), *hole_height, 0.0, 1.0, 3);
  gimp_label_spin_set_increments (GIMP_LABEL_SPIN (scale), 0.001, 0.01);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (film_scale_entry_update_double),
                    hole_height);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_show (scale);

  filmint.scales[5] = scale =
    gimp_scale_entry_new (_("Hole sp_acing:"), *hole_space, 0.0, 1.0, 3);
  gimp_label_spin_set_increments (GIMP_LABEL_SPIN (scale), 0.001, 0.01);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (film_scale_entry_update_double),
                    hole_space);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_set_margin_bottom (gimp_labeled_get_label (GIMP_LABELED (scale)), 6);
  gtk_widget_set_margin_bottom (gimp_scale_entry_get_range (GIMP_SCALE_ENTRY (scale)), 6);
  gtk_widget_set_margin_bottom (gimp_label_spin_get_spin_button (GIMP_LABEL_SPIN (scale)), 6);
  gtk_widget_show (scale);

  filmint.scales[6] = scale =
    gimp_scale_entry_new (_("_Number height:"), *number_height, 0.0, 1.0, 3);
  gimp_label_spin_set_increments (GIMP_LABEL_SPIN (scale), 0.001, 0.01);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (film_scale_entry_update_double),
                    number_height);
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
film_dialog (GimpImage           *image,
             GimpProcedureConfig *config)
{
  GtkWidget *dlg;
  GtkWidget *main_vbox;
  GtkWidget *notebook;
  gboolean   run;

  gint       film_height;
  gint       number_start;
  GimpFont  *number_font;
  GimpRGB   *number_color;
  GimpRGB   *film_color;
  gboolean   keep_height;
  gboolean   at_top;
  gboolean   at_bottom;

  gdouble    picture_height;
  gdouble    picture_space;
  gdouble    number_height;
  gdouble    hole_offset;
  gdouble    hole_width;
  gdouble    hole_height;
  gdouble    hole_space;

  g_object_get (config,
                "film-height",     &film_height,
                "keep-height",     &keep_height,
                "number-color",    &number_color,
                "film-color",      &film_color,
                "number-start",    &number_start,
                "number-font",     &number_font,
                "at-top",          &at_top,
                "at-bottom",       &at_bottom,

                "picture-height",  &picture_height,
                "picture-spacing", &picture_space,
                "number-height",   &number_height,
                "hole-offset",     &hole_offset,
                "hole-width",      &hole_width,
                "hole-height",     &hole_height,
                "hole-spacing",    &hole_space,
                NULL);

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

  create_selection_tab (notebook, image, config,
                        &film_height,
                        &number_start,
                        &number_font,
                        number_color,
                        film_color,
                        &keep_height,
                        &at_top,
                        &at_bottom);
  create_advanced_tab (notebook, config,
                       &picture_height,
                       &picture_space,
                       &number_height,
                       &hole_offset,
                       &hole_width,
                       &hole_height,
                       &hole_space);

  gtk_widget_show (notebook);

  gtk_widget_show (dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  if (run)
    {
      GimpObjectArray *images_array;
      GimpImage       *images[MAX_FILM_PICTURES];
      gint             num_images = 0;
      gboolean         iter_valid;
      GtkTreeIter      iter;

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
              images[num_images] = gimp_image_get_by_id (image_id);
              num_images++;
            }
        }

      images_array = gimp_object_array_new (GIMP_TYPE_IMAGE, (GObject **) images, num_images, FALSE);

      g_object_set (config,
                    "images",          images_array,
                    "num-images",      num_images,
                    "film-height",     film_height,

                    "keep-height",     keep_height,
                    "number-color",    number_color,
                    "film-color",      film_color,
                    "number-start",    number_start,
                    "at-top",          at_top,
                    "at-bottom",       at_bottom,

                    "picture-height",  picture_height,
                    "picture-spacing", picture_space,
                    "number-height",   number_height,
                    "hole-offset",     hole_offset,
                    "hole-width",      hole_width,
                    "hole-height",     hole_height,
                    "hole-spacing",    hole_space,
                    NULL);

      gimp_object_array_free (images_array);
    }

  gtk_widget_destroy (dlg);
  g_free (number_color);
  g_free (film_color);
  g_clear_object (&number_font);

  return run;
}

static void
film_reset_callback (GtkWidget *widget,
                     gpointer   data)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS (advanced_defaults) ; i++)
    gimp_label_spin_set_value (GIMP_LABEL_SPIN (filmint.scales[i]),
                               advanced_defaults[i]);
}

static void
film_font_select_callback (GimpFontSelectButton  *button,
                           GimpResource          *resource,
                           gboolean               closing,
                           GimpFont             **font)
{
  g_clear_object (font);
  *font = GIMP_FONT (resource);
}

static void
film_scale_entry_update_double (GimpLabelSpin *entry,
                                gdouble       *value)
{
  *value = gimp_label_spin_get_value (entry);
}
