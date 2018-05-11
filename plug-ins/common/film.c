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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
  gint32   image[MAX_FILM_PICTURES]; /* list of image IDs */
} FilmVals;

/* Data to use for the dialog */
typedef struct
{
  GtkAdjustment *advanced_adj[7];
  GtkTreeModel  *image_list_all;
  GtkTreeModel  *image_list_film;
} FilmInterface;


/* Declare local functions
 */
static void      query  (void);
static void      run    (const gchar      *name,
                         gint              nparams,
                         const GimpParam  *param,
                         gint             *nreturn_vals,
                         GimpParam       **return_vals);


static gint32    create_new_image   (const gchar    *filename,
                                     guint           width,
                                     guint           height,
                                     GimpImageType   gdtype,
                                     gint32         *layer_ID,
                                     GimpDrawable  **drawable,
                                     GimpPixelRgn   *pixel_rgn);

static gchar   * compose_image_name (gint32          image_ID);

static gint32    film               (void);

static gboolean  check_filmvals     (void);

static void      set_pixels         (gint            numpix,
                                     guchar         *dst,
                                     GimpRGB        *color);

static guchar  * create_hole_rgb    (gint            width,
                                     gint            height);

static void      draw_hole_rgb      (GimpDrawable   *drw,
                                     gint            x,
                                     gint            y,
                                     gint            width,
                                     gint            height,
                                     guchar         *hole);

static void      draw_number        (gint32          layer_ID,
                                     gint            num,
                                     gint            x,
                                     gint            y,
                                     gint            height);


static void        add_list_item_callback (GtkWidget        *widget,
                                           GtkTreeSelection *sel);
static void        del_list_item_callback (GtkWidget        *widget,
                                           GtkTreeSelection *sel);

static GtkTreeModel * add_image_list      (gboolean        add_box_flag,
                                           gint            n,
                                           gint32         *image_id,
                                           GtkWidget      *hbox);

static gboolean    film_dialog               (gint32                image_ID);
static void        film_reset_callback       (GtkWidget            *widget,
                                              gpointer              data);
static void        film_font_select_callback (GimpFontSelectButton *button,
                                              const gchar          *name,
                                              gboolean              closing,
                                              gpointer              data);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

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


static GimpRunMode run_mode;


MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,      "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,      "image",        "Input image (only used as default image in interactive mode)" },
    { GIMP_PDB_DRAWABLE,   "drawable",     "Input drawable (not used)" },
    { GIMP_PDB_INT32,      "film-height",  "Height of film (0: fit to images)" },
    { GIMP_PDB_COLOR,      "film-color",   "Color of the film" },
    { GIMP_PDB_INT32,      "number-start", "Start index for numbering" },
    { GIMP_PDB_STRING,     "number-font",  "Font for drawing numbers" },
    { GIMP_PDB_COLOR,      "number-color", "Color for numbers" },
    { GIMP_PDB_INT32,      "at-top",       "Flag for drawing numbers at top of film" },
    { GIMP_PDB_INT32,      "at-bottom",    "Flag for drawing numbers at bottom of film" },
    { GIMP_PDB_INT32,      "num-images",   "Number of images to be used for film" },
    { GIMP_PDB_INT32ARRAY, "image-ids",    "num-images image IDs to be used for film" }
  };

  static const GimpParamDef return_vals[] =
  {
    { GIMP_PDB_IMAGE, "new-image", "Output image" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Combine several images on a film strip"),
                          "Compose several images to a roll film",
                          "Peter Kirchgessner",
                          "Peter Kirchgessner (peter@kirchgessner.net)",
                          "1997",
                          N_("_Filmstrip..."),
                          "INDEXED*, GRAY*, RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args),
                          G_N_ELEMENTS (return_vals),
                          args, return_vals);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Combine");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[2];
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  gint32            image_ID;
  gint              k;

  INIT_I18N ();

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 2;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
  values[1].type          = GIMP_PDB_IMAGE;
  values[1].data.d_int32  = -1;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &filmvals);

      /*  First acquire information with a dialog  */
      if (! film_dialog (param[1].data.d_int32))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      /* Also we want to have some images to compose */
      if ((nparams != 12) || (param[10].data.d_int32 < 1))
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          filmvals.keep_height       = (param[3].data.d_int32 <= 0);
          filmvals.film_height       = (filmvals.keep_height ?
                                        128 : param[3].data.d_int32);
          filmvals.film_color        = param[4].data.d_color;
          filmvals.number_start      = param[5].data.d_int32;
          g_strlcpy (filmvals.number_font, param[6].data.d_string, FONT_LEN);
          filmvals.number_color      = param[7].data.d_color;
          filmvals.number_pos[0]     = param[8].data.d_int32;
          filmvals.number_pos[1]     = param[9].data.d_int32;
          filmvals.num_images        = param[10].data.d_int32;
          if (filmvals.num_images > MAX_FILM_PICTURES)
            filmvals.num_images = MAX_FILM_PICTURES;
          for (k = 0; k < filmvals.num_images; k++)
            filmvals.image[k] = param[11].data.d_int32array[k];
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &filmvals);
      break;

    default:
      break;
    }

  if (! check_filmvals ())
    status = GIMP_PDB_CALLING_ERROR;

  if (status == GIMP_PDB_SUCCESS)
    {
      gimp_progress_init (_("Composing images"));

      image_ID = film ();

      if (image_ID < 0)
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
      else
        {
          values[1].data.d_int32 = image_ID;
          gimp_image_undo_enable (image_ID);
          gimp_image_clean_all (image_ID);
          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_display_new (image_ID);
        }

      /*  Store data  */
      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data (PLUG_IN_PROC, &filmvals, sizeof (FilmVals));
    }

  values[0].data.d_status = status;
}

/* Compose a roll film image from several images */
static gint32
film (void)
{
  gint          width, height;
  guchar       *hole;
  gint          film_height, film_width;
  gint          picture_width, picture_height;
  gint          picture_space, picture_x0, picture_y0;
  gint          hole_offset, hole_width, hole_height, hole_space, hole_x;
  gint          number_height, num_images, num_pictures;
  gint          j, k, picture_count;
  gdouble       f;
  gint          num_layers;
  gint32       *image_ID_src, image_ID_dst, layer_ID_src, layer_ID_dst;
  gint          image_ID_tmp;
  gint32       *layers;
  GimpDrawable *drawable_dst;
  GimpPixelRgn  pixel_rgn_dst;
  gint          new_layer;
  gint          floating_sel;

  /* initialize */

  layers = NULL;

  num_images = filmvals.num_images;
  image_ID_src = filmvals.image;

  if (num_images <= 0)
    return (-1);

  gimp_context_push ();
  gimp_context_set_foreground (&filmvals.number_color);
  gimp_context_set_background (&filmvals.film_color);

  if (filmvals.keep_height) /* Search maximum picture height */
    {
      picture_height = 0;
      for (j = 0; j < num_images; j++)
        {
          height = gimp_image_height (image_ID_src[j]);
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
  for (j = 0; j < num_images; j++)
    {
      layers = gimp_image_get_layers (image_ID_src[j], &num_layers);
      /* Get scaled image size */
      width = gimp_image_width (image_ID_src[j]);
      height = gimp_image_height (image_ID_src[j]);
      f = ((double)picture_height) / (double)height;
      picture_width = width * f;

      for (k = 0; k < num_layers; k++)
        {
          if (gimp_layer_is_floating_sel (layers[k]))
            continue;

          film_width += (picture_space/2);  /* Leading space */
          film_width += picture_width;      /* Scaled image width */
          film_width += (picture_space/2);  /* Trailing space */
          num_pictures++;
        }

      g_free (layers);
    }

#ifdef FILM_DEBUG
  g_printerr ("film_height = %d, film_width = %d\n", film_height, film_width);
  g_printerr ("picture_height = %d, picture_space = %d, picture_y0 = %d\n",
              picture_height, picture_space, picture_y0);
  g_printerr ("Number of pictures = %d\n", num_pictures);
#endif

  image_ID_dst = create_new_image (_("Untitled"),
                                   (guint) film_width, (guint) film_height,
                                   GIMP_RGB_IMAGE, &layer_ID_dst,
                                   &drawable_dst, &pixel_rgn_dst);

  /* Fill film background */
  gimp_drawable_fill (layer_ID_dst, GIMP_FILL_BACKGROUND);

  /* Draw all the holes */
  hole_offset = film_height * filmvals.hole_offset;
  hole_width = film_height * filmvals.hole_width;
  hole_height = film_height * filmvals.hole_height;
  hole_space = film_height * filmvals.hole_space;
  hole_x = hole_space / 2;

#ifdef FILM_DEBUG
  g_printerr ("hole_x %d hole_offset %d hole_width %d hole_height %d hole_space %d\n",
              hole_x, hole_offset, hole_width, hole_height, hole_space );
#endif

  hole = create_hole_rgb (hole_width, hole_height);
  if (hole)
    {
      while (hole_x < film_width)
        {
          draw_hole_rgb (drawable_dst, hole_x,
                         hole_offset,
                         hole_width, hole_height, hole);
          draw_hole_rgb (drawable_dst, hole_x,
                         film_height-hole_offset-hole_height,
                         hole_width, hole_height, hole);

          hole_x += hole_width + hole_space;
        }
      g_free (hole);
    }
  gimp_drawable_detach (drawable_dst);


  /* Compose all images and layers */
  picture_x0 = 0;
  picture_count = 0;
  for (j = 0; j < num_images; j++)
    {
      image_ID_tmp = gimp_image_duplicate (image_ID_src[j]);
      width = gimp_image_width (image_ID_tmp);
      height = gimp_image_height (image_ID_tmp);
      f = ((gdouble) picture_height) / (gdouble) height;
      picture_width = width * f;
      if (gimp_image_base_type (image_ID_tmp) != GIMP_RGB)
        gimp_image_convert_rgb (image_ID_tmp);
      gimp_image_scale (image_ID_tmp, picture_width, picture_height);

      layers = gimp_image_get_layers (image_ID_tmp, &num_layers);
      for (k = 0; k < num_layers; k++)
        {
          if (gimp_layer_is_floating_sel (layers[k]))
            continue;

          picture_x0 += picture_space / 2;

          layer_ID_src = layers[k];
          gimp_layer_resize_to_image_size (layer_ID_src);
          new_layer = gimp_layer_new_from_drawable (layer_ID_src,
                                                    image_ID_dst);
          gimp_image_insert_layer (image_ID_dst, new_layer, -1, -1);
          gimp_layer_set_offsets (new_layer, picture_x0, picture_y0);

          /* Draw picture numbers */
          if ((number_height > 0) &&
              (filmvals.number_pos[0] || filmvals.number_pos[1]))
            {
              if (filmvals.number_pos[0])
                draw_number (layer_ID_dst,
                             filmvals.number_start + picture_count,
                             picture_x0 + picture_width/2,
                             (hole_offset-number_height)/2, number_height);
              if (filmvals.number_pos[1])
                draw_number (layer_ID_dst,
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

      g_free (layers);
      gimp_image_delete (image_ID_tmp);
    }
  gimp_progress_update (1.0);

  gimp_image_flatten (image_ID_dst);

  /* Drawing text/numbers leaves us with a floating selection. Stop it */
  floating_sel = gimp_image_get_floating_sel (image_ID_dst);
  if (floating_sel != -1)
    gimp_floating_sel_anchor (floating_sel);

  gimp_context_pop ();

  return image_ID_dst;
}

/* Check filmvals. Unreasonable values are reset to a default. */
/* If this is not possible, FALSE is returned. Otherwise TRUE is returned. */
static gboolean
check_filmvals (void)
{
  if (filmvals.film_height < 10)
    filmvals.film_height = 10;

  if (filmvals.number_start < 0)
    filmvals.number_start = 0;

  if (filmvals.number_font[0] == '\0')
    strcpy (filmvals.number_font, "Monospace");

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

/* Draw the hole at the specified position */
static void
draw_hole_rgb (GimpDrawable *drw,
               gint          x,
               gint          y,
               gint          width,
               gint          height,
               guchar       *hole)
{
  GimpPixelRgn    rgn;
  guchar         *data;
  gint            tile_height = gimp_tile_height ();
  gint            i, j, scan_lines;
  gint            d_width = gimp_drawable_width (drw->drawable_id);
  gint            length;

  if ((width <= 0) || (height <= 0))
    return;
  if ((x+width <= 0) || (x >= d_width))
    return;
  length = width;   /* Check that we don't draw past the image */
  if ((x+length) >= d_width)
    length = d_width-x;

  data = g_new (guchar, length * tile_height * drw->bpp);

  gimp_pixel_rgn_init (&rgn, drw, x, y, length, height, TRUE, FALSE);

  i = 0;
  while (i < height)
    {
      scan_lines = (i+tile_height-1 < height) ? tile_height : (height-i);
      if (length == width)
        {
          memcpy (data, hole + 3*width*i, width*scan_lines*3);
        }
      else  /* We have to do some clipping */
        {
          for (j = 0; j < scan_lines; j++)
            memcpy (data + j*length*3, hole + (i+j)*width*3, length*3);
        }
      gimp_pixel_rgn_set_rect (&rgn, data, x, y+i, length, scan_lines);

      i += scan_lines;
    }

  g_free (data);
}

/* Draw the number of the picture onto the film */
static void
draw_number (gint32 layer_ID,
             gint   num,
             gint   x,
             gint   y,
             gint   height)
{
  gchar         buf[32];
  GimpDrawable *drw;
  gint          k, delta, max_delta;
  gint32        image_ID;
  gint32        text_layer_ID;
  gint          text_width, text_height, text_ascent, descent;
  gchar        *fontname = filmvals.number_font;

  g_snprintf (buf, sizeof (buf), "%d", num);

  drw = gimp_drawable_get (layer_ID);
  image_ID = gimp_item_get_image (layer_ID);

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

  text_layer_ID = gimp_text_fontname (image_ID, layer_ID,
                                      x, y + descent / 2,
                                      buf, 1, FALSE,
                                      height, GIMP_PIXELS,
                                      fontname);

  if (text_layer_ID == -1)
    g_message ("draw_number: Error in drawing text\n");

  gimp_drawable_detach (drw);
}

/* Create an image. Sets layer_ID, drawable and rgn. Returns image_ID */
static gint32
create_new_image (const gchar    *filename,
                  guint           width,
                  guint           height,
                  GimpImageType   gdtype,
                  gint32         *layer_ID,
                  GimpDrawable  **drawable,
                  GimpPixelRgn   *pixel_rgn)
{
  gint32            image_ID;
  GimpImageBaseType gitype;

  if ((gdtype == GIMP_GRAY_IMAGE) || (gdtype == GIMP_GRAYA_IMAGE))
    gitype = GIMP_GRAY;
  else if ((gdtype == GIMP_INDEXED_IMAGE) || (gdtype == GIMP_INDEXEDA_IMAGE))
    gitype = GIMP_INDEXED;
  else
    gitype = GIMP_RGB;

  image_ID = gimp_image_new (width, height, gitype);
  gimp_image_set_filename (image_ID, filename);

  gimp_image_undo_disable (image_ID);
  *layer_ID = gimp_layer_new (image_ID, _("Background"), width, height,
                              gdtype,
                              100,
                              gimp_image_get_default_new_layer_mode (image_ID));
  gimp_image_insert_layer (image_ID, *layer_ID, -1, 0);

  if (drawable)
    {
      *drawable = gimp_drawable_get (*layer_ID);
      if (pixel_rgn != NULL)
        gimp_pixel_rgn_init (pixel_rgn, *drawable, 0, 0, (*drawable)->width,
                             (*drawable)->height, TRUE, FALSE);
    }

  return image_ID;
}

static gchar *
compose_image_name (gint32 image_ID)
{
  gchar *image_name;
  gchar *name;

  /* Compose a name of the basename and the image-ID */

  name = gimp_image_get_name (image_ID);

  image_name = g_strdup_printf ("%s-%d", name, image_ID);

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
          gint32  image_ID;
          gchar  *name;

          gtk_tree_model_get (model, &iter,
                              0, &image_ID,
                              1, &name,
                              -1);

          gtk_list_store_append (GTK_LIST_STORE (filmint.image_list_film),
                                 &iter);

          gtk_list_store_set (GTK_LIST_STORE (filmint.image_list_film),
                              &iter,
                              0, image_ID,
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
                gint       n,
                gint32    *image_id,
                GtkWidget *hbox)
{
  GtkWidget        *vbox;
  GtkWidget        *label;
  GtkWidget        *scrolled_win;
  GtkWidget        *tv;
  GtkWidget        *button;
  GtkListStore     *store;
  GtkTreeSelection *sel;
  gint              i;

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

  for (i = 0; i < n; i++)
    {
      GtkTreeIter  iter;
      gchar       *name;

      gtk_list_store_append (store, &iter);

      name = compose_image_name (image_id[i]);

      gtk_list_store_set (store, &iter,
                          0, image_id[i],
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
                      gint32     image_ID)
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
  gint32          *image_id_list;
  gint             nimages, j;

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
  adj = GTK_ADJUSTMENT (gtk_adjustment_new (filmvals.film_height, 10,
                                            GIMP_MAX_IMAGE_SIZE, 1, 10, 0));
  spinbutton = gtk_spin_button_new (adj, 1, 0);
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
  adj = GTK_ADJUSTMENT (gtk_adjustment_new (filmvals.number_start, 0,
                                            GIMP_MAX_IMAGE_SIZE, 1, 10, 0));
  spinbutton = gtk_spin_button_new (adj, 1, 0);
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
  image_id_list = gimp_image_list (&nimages);
  filmint.image_list_all = add_image_list (TRUE, nimages, image_id_list, hbox);

  /* Get a list of the images used for the film */
  filmint.image_list_film = add_image_list (FALSE, 1, &image_ID, hbox);

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
film_dialog (gint32 image_ID)
{
  GtkWidget *dlg;
  GtkWidget *main_vbox;
  GtkWidget *notebook;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

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

  create_selection_tab (notebook, image_ID);
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
          gint image_ID;

          gtk_tree_model_get (filmint.image_list_film, &iter,
                              0, &image_ID,
                              -1);

          if ((image_ID >= 0) && (num_images < MAX_FILM_PICTURES))
            filmvals.image[num_images++] = image_ID;
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
