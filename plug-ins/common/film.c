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
#define FILM(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), FILM_TYPE, Film))

GType                   film_get_type         (void) G_GNUC_CONST;

static GList          * film_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * film_create_procedure (GimpPlugIn           *plug_in,
                                               const gchar          *name);

static GimpValueArray * film_run              (GimpProcedure        *procedure,
                                               GimpRunMode           run_mode,
                                               GimpImage            *image,
                                               GimpDrawable        **drawables,
                                               GimpProcedureConfig  *config,
                                               gpointer              run_data);

static GimpImage      * create_new_image      (guint                 width,
                                               guint                 height,
                                               GimpImageType         gdtype,
                                               GimpLayer           **layer);

static GimpImage      * film                  (GimpProcedureConfig  *config);

static gboolean         check_filmvals        (GimpProcedureConfig  *config,
                                               GError              **error);

static void             set_pixels            (gint                  numpix,
                                               guchar               *dst,
                                               GeglColor            *color);

static guchar         * create_hole_color     (gint                  width,
                                               gint                  height,
                                               GeglColor            *film_color);

static void             draw_number           (GimpLayer            *layer,
                                               gint                  num,
                                               gint                  x,
                                               gint                  y,
                                               gint                  height,
                                               GimpFont             *font);

static gchar         * compose_image_name     (GimpImage            *image);
static void            add_list_item_callback (GtkWidget            *widget,
                                               GtkTreeSelection     *sel);
static void            del_list_item_callback (GtkWidget            *widget,
                                               GtkTreeSelection     *sel);

static GtkTreeModel  * add_image_list         (gboolean              add_box_flag,
                                               GList                *images,
                                               GtkWidget            *hbox);
static void            create_selection_tab   (GimpProcedureDialog  *dialog,
                                               GimpImage            *image);
static gboolean        film_dialog            (GimpImage            *image,
                                               GimpProcedure        *procedure,
                                               GimpProcedureConfig  *config);


G_DEFINE_TYPE (Film, film, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (FILM_TYPE)
DEFINE_STD_SET_I18N


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
  GeglColor     *default_film_color;
  GeglColor     *default_number_color;
  gdouble        default_number_rgb[4] = { 0.93, 0.61, 0.0, 1.0 };

  gegl_init (NULL, NULL);
  default_film_color   = gegl_color_new ("black");
  default_number_color = gegl_color_new (NULL);
  gegl_color_set_pixel (default_number_color, babl_format ("R'G'B'A double"), default_number_rgb);

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
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

      gimp_procedure_add_int_argument (procedure, "film-height",
                                       _("Film _height"),
                                       _("Height of film (0: fit to images)"),
                                       0, GIMP_MAX_IMAGE_SIZE, 0,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_color_argument (procedure, "film-color",
                                         _("_Film color"),
                                         _("Color of the film"),
                                         TRUE, default_film_color,
                                         G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "number-start",
                                       _("Start _index"),
                                       _("Start index for numbering"),
                                       0, G_MAXINT, 1,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_font_argument (procedure, "number-font",
                                        _("Number _font"),
                                        _("Font for drawing numbers"),
                                        FALSE, NULL, TRUE,
                                        G_PARAM_READWRITE);

      gimp_procedure_add_color_argument (procedure, "number-color",
                                         _("_Number color"),
                                         _("Color for numbers"),
                                         TRUE, default_number_color,
                                         G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "at-top",
                                           _("At _top"),
                                           _("Draw numbers at top"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "at-bottom",
                                           _("At _bottom"),
                                           _("Draw numbers at bottom"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      /* Arguments ignored in interactive mode. */

      gimp_procedure_add_core_object_array_argument (procedure, "images",
                                                     "Images",
                                                     "Images to be used for film",
                                                     GIMP_TYPE_IMAGE,
                                                     G_PARAM_READWRITE);

      /* The more specific settings. */

      gimp_procedure_add_double_argument (procedure, "picture-height",
                                          _("Image _height"),
                                          _("As fraction of the strip height"),
                                          0.0, 1.0, 0.695,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "picture-spacing",
                                          _("Image s_pacing"),
                                          _("The spacing between 2 images, as fraction of the strip height"),
                                          0.0, 1.0, 0.040,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "hole-offset",
                                          _("Hole offse_t"),
                                          _("The offset from the edge of film, as fraction of the strip height"),
                                          0.0, 1.0, 0.058,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "hole-width",
                                          _("Hole _width"),
                                          _("The width of the holes, as fraction of the strip height"),
                                          0.0, 1.0, 0.052,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "hole-height",
                                          _("Hole hei_ght"),
                                          _("The height of the holes, as fraction of the strip height"),
                                          0.0, 1.0, 0.081,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "hole-spacing",
                                          _("Hole _distance"),
                                          _("The distance between holes, as fraction of the strip height"),
                                          0.0, 1.0, 0.081,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "number-height",
                                          _("_Number height"),
                                          _("The height of drawn numbers, as fraction of the strip height"),
                                          0.0, 1.0, 0.052,
                                          G_PARAM_READWRITE);

      /* Auxiliary argument mostly for the GUI. */

      gimp_procedure_add_boolean_aux_argument (procedure, "keep-height",
                                               _("F_it height to images"),
                                               _("Keep maximum image height"),
                                               TRUE,
                                               G_PARAM_READWRITE);

      /* Returned image. */

      gimp_procedure_add_image_return_value (procedure, "new-image",
                                             "New image",
                                             "Output image",
                                             FALSE,
                                             G_PARAM_READWRITE);
    }

  g_object_unref (default_film_color);
  g_object_unref (default_number_color);

  return procedure;
}

static GimpValueArray *
film_run (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GimpImage            *image,
          GimpDrawable        **drawables,
          GimpProcedureConfig  *config,
          gpointer              run_data)
{
  GimpValueArray     *return_vals = NULL;
  GimpPDBStatusType   status      = GIMP_PDB_SUCCESS;
  GError             *error       = NULL;
  gint                film_height;

  gegl_init (NULL, NULL);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
        {
          GParamSpec *spec;
          gint        default_value;

          /* In interactive mode, not only do we always use the current image's
           * height as initial value, but also as factory default. Any other
           * value makes no sense as a "default".
           */
          default_value = gimp_image_get_height (image);
          spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config), "film-height");
          G_PARAM_SPEC_INT (spec)->default_value = default_value;
          g_object_set (config, "film-height", default_value, NULL);

          if (! film_dialog (image, procedure, config))
            return gimp_procedure_new_return_values (procedure, GIMP_PDB_CANCEL,
                                                     NULL);
        }
      break;

    case GIMP_RUN_NONINTERACTIVE:
      g_object_get (config, "film-height", &film_height, NULL);
      g_object_set (config,
                    "keep-height", (film_height == 0),
                    NULL);
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      break;

    default:
      break;
    }

  if (! check_filmvals (config, &error))
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
    return_vals = gimp_procedure_new_return_values (procedure, status, error);

  return return_vals;
}

/* Compose a roll film image from several images */
static GimpImage *
film (GimpProcedureConfig *config)
{
  GimpImage **images;
  gint        width, height;
  guchar     *hole;
  gint        film_height;
  gint        film_width;
  gint        picture_width;
  gdouble     picture_height;
  gdouble     picture_space;
  gint        picture_x0;
  gint        picture_y0;
  gdouble     hole_offset;
  gdouble     hole_width;
  gdouble     hole_height;
  gdouble     hole_space;
  gint        hole_x;
  gdouble     number_height;
  gint        num_pictures;
  gint        number_start;
  gboolean    at_top;
  gboolean    at_bottom;
  gint        picture_count;
  GeglColor  *number_color = NULL;
  GeglColor  *film_color   = NULL;
  gboolean    keep_height;
  gdouble     f;
  GimpImage  *image_dst;
  GimpImage  *image_tmp;
  GimpLayer  *layer_src;
  GimpLayer  *layer_dst;
  GimpLayer  *new_layer;
  GimpLayer  *floating_sel;

  GimpFont   *number_font;

  GList      *layers     = NULL;
  GList      *iter2;
  gint        i;

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

  if (images[0] == NULL)
    return NULL;

  if (film_color == NULL)
    film_color = gegl_color_new ("black");

  if (number_color == NULL)
    {
      gdouble default_number_rgb[4] = { 0.93, 0.61, 0.0, 1.0 };

      number_color = gegl_color_new (NULL);
      gegl_color_set_pixel (number_color, babl_format ("R'G'B'A double"), default_number_rgb);
    }

  gimp_context_push ();
  gimp_context_set_foreground (number_color);
  gimp_context_set_background (film_color);

  if (keep_height) /* Search maximum picture height */
    {
      gdouble max_height = 0;

      for (i = 0; images[i] != NULL; i++)
        {
          GimpImage *image = images[i];

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
  for (i = 0; images[i] != NULL; i++)
    {
      GimpImage *image = images[i];

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

  /* TODO: create the hole in the drawable format instead of creating a raw
   * buffer in sRGB u8.
   */
  hole = create_hole_color ((gint) hole_width, (gint) hole_height, film_color);
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
  for (i = 0; images[i] != NULL; i++)
    {
      GimpImage *image = images[i];

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

  g_free (images);
  g_clear_object (&number_color);
  g_clear_object (&film_color);
  g_clear_object (&number_font);

  return image_dst;
}

/* Unreasonable values are reset to a default. */
/* If this is not possible, FALSE is returned. Otherwise TRUE is returned. */
static gboolean
check_filmvals (GimpProcedureConfig  *config,
                GError              **error)
{
  GimpFont   *font = NULL;
  GimpImage **images;
  gint        film_height;
  gint        i, j;
  gboolean    success = FALSE;

  g_object_get (config,
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

  if (images != NULL && images[0] != NULL)
    {
      for (i = 0, j = 0; images[i] != NULL; i++)
        {
          if (gimp_image_is_valid (images[i]))
            {
              images[j] = images[i];
              j++;
            }
        }

      if (j > 0)
        {
          images[j] = NULL;
          g_object_set (config,
                        "images", images,
                        NULL);
          success = TRUE;
        }
    }

  if (images == NULL || images[0] == NULL)
    g_set_error_literal (error, GIMP_PLUG_IN_ERROR, 0,
                         _("\"Filmstrip\" cannot be run without any input images"));

  g_free (images);
  g_clear_object (&font);

  return success;
}

/* Assigns numpix pixels starting at dst with color r,g,b */
static void
set_pixels (gint       numpix,
            guchar    *dst,
            GeglColor *color)
{
  register gint   k;
  register guchar *udest;
  guchar urgb[3];

  gegl_color_get_pixel (color, babl_format ("R'G'B' u8"), urgb);
  k = numpix;
  udest = dst;

  while (k-- > 0)
    {
      *(udest++) = urgb[0];
      *(udest++) = urgb[1];
      *(udest++) = urgb[2];
    }
}

/* Create the RGB-pixels that make up the hole */
static guchar *
create_hole_color (gint       width,
                   gint       height,
                   GeglColor *film_color)
{
  guchar *hole, *top, *bottom;
  gint    radius, length, k;

  if (width <= 0 || height <= 0)
    return NULL;

  hole = g_new (guchar, width * height * 3);

  /* Fill a rectangle with white */
  memset (hole, 255, width * height * 3);
  radius = height / 4;
  if (radius > width / 2)
    radius = width / 2;
  top = hole;
  bottom = hole + (height-1) * width * 3;
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
  gchar      buf[32];
  gint       k, delta, max_delta;
  GimpImage *image;
  GimpLayer *text_layer;
  gint       text_width, text_height, text_ascent, descent;

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

      success = gimp_text_get_extents_font (buf, height + delta, font,
                                            &text_width, &text_height,
                                            &text_ascent, &descent);

      if (success)
        {
          height += delta;
          break;
        }
    }

  text_layer = gimp_text_font (image, GIMP_DRAWABLE (layer),
                               x, y - descent / 2,
                               buf, 1, FALSE,
                               height, font);

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
create_selection_tab (GimpProcedureDialog *dialog,
                      GimpImage           *image)
{
  GtkWidget *paned;
  GtkWidget *frame;
  GtkWidget *hbox;
  GList     *image_list;

  gimp_procedure_dialog_fill_frame (dialog,
                                    "fit-height-frame",
                                    "keep-height", TRUE,
                                    "film-height");
  gimp_procedure_dialog_fill_box (dialog,
                                  "filmstrip-box",
                                  "fit-height-frame",
                                  "film-color",
                                  NULL);
  gimp_procedure_dialog_get_label (dialog, "filmstrip-title", _("Filmstrip"), FALSE, FALSE);
  gimp_procedure_dialog_fill_frame (dialog,
                                    "filmstrip-frame",
                                    "filmstrip-title", FALSE,
                                    "filmstrip-box");

  gimp_procedure_dialog_fill_box (dialog,
                                  "numbering-box",
                                  "number-start",
                                  "number-font",
                                  "number-color",
                                  "at-top",
                                  "at-bottom",
                                  NULL);
  gimp_procedure_dialog_get_label (dialog, "numbering-title", _("Numbering"), FALSE, FALSE);
  gimp_procedure_dialog_fill_frame (dialog,
                                    "numbering-frame",
                                    "numbering-title", FALSE,
                                    "numbering-box");

  gimp_procedure_dialog_fill_box (dialog,
                                  "selection-box",
                                  "filmstrip-frame",
                                  "numbering-frame",
                                  NULL);

  paned = gimp_procedure_dialog_fill_paned (dialog,
                                            "selection-paned",
                                            GTK_ORIENTATION_HORIZONTAL,
                                            "selection-box", NULL);

  /*** The right frame keeps the image selection ***/
  frame = gimp_frame_new (_("Image Selection"));
  gtk_widget_set_hexpand (frame, TRUE);
  gtk_widget_show (frame);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  /* Get a list of all image names */
  image_list = gimp_list_images ();
  filmint.image_list_all = add_image_list (TRUE, image_list, hbox);
  g_list_free (image_list);

  /* Get a list of the images used for the film */
  image_list = g_list_prepend (NULL, image);
  filmint.image_list_film = add_image_list (FALSE, image_list, hbox);
  g_list_free (image_list);

  gtk_paned_pack2 (GTK_PANED (paned), frame, TRUE, FALSE);
}

static gboolean
film_dialog (GimpImage           *image,
             GimpProcedure       *procedure,
             GimpProcedureConfig *config)
{
  GtkWidget *dialog;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Filmstrip"));

  create_selection_tab (GIMP_PROCEDURE_DIALOG (dialog), image);

  /* Create Advanced tab. */
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                  "advanced-box",
                                  "picture-height",
                                  "picture-spacing",
                                  "hole-offset",
                                  "hole-width",
                                  "hole-height",
                                  "hole-spacing",
                                  "number-height",
                                  NULL);
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog), "advanced-frame-label",
                                   _("All Values are Fractions of the Strip Height"), FALSE, FALSE);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "advanced-frame",
                                    "advanced-frame-label", FALSE,
                                    "advanced-box");

  /* Fill the notebook. */
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog), "advanced-tab", _("Ad_vanced"), FALSE, TRUE);
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog), "selection-tab", _("_Selection"), FALSE, TRUE);
  gimp_procedure_dialog_fill_notebook (GIMP_PROCEDURE_DIALOG (dialog),
                                       "main-notebook",
                                       "selection-tab",
                                       "selection-paned",
                                       "advanced-tab",
                                       "advanced-frame",
                                       NULL);
  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog), "main-notebook", NULL);
  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  if (run)
    {
      GimpImage    *images[MAX_FILM_PICTURES + 1];
      gint          num_images = 0;
      gboolean      iter_valid;
      GtkTreeIter   iter;

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
      images[num_images] = NULL;

      g_object_set (config,
                    "images", images,
                    NULL);
    }

  gtk_widget_destroy (dialog);

  return run;
}
