/*============================================================================*
 
  Paper Tile 1.0  -- A GIMP PLUG-IN

  Copyright (C) 1997-1999 Hirotsuna Mizuno <s1041150@u-aizu.ac.jp>

  This program  is  free software;  you can redistribute it  and/or  modify it
  under the terms of the GNU Public License as published  by the Free Software
  Foundation;  either version 2 of the License,  or (at your option) any later
  version.

  This program is distributed in the hope that it will be useful,  but WITHOUT
  ANY WARRANTY;  without  even  the  implied  warranty  of MERCHANTABILITY  or
  FITNESS  FOR  A PARTICULAR PURPOSE.  See the GNU General Public License  for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 *============================================================================*/

#include "config.h"

#include <stdlib.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

/*============================================================================*/
/* DEFINES                                                                    */
/*============================================================================*/

#define PLUGIN_PROCEDURE_NAME     "plug_in_papertile"

/*============================================================================*/
/* TYPES                                                                      */
/*============================================================================*/

typedef enum
{
  BACKGROUND_TYPE_TRANSPARENT,
  BACKGROUND_TYPE_INVERTED,
  BACKGROUND_TYPE_IMAGE,
  BACKGROUND_TYPE_FOREGROUND,
  BACKGROUND_TYPE_BACKGROUND,
  BACKGROUND_TYPE_COLOR
} BackgroundType;

typedef enum
{
  FRACTIONAL_TYPE_BACKGROUND,           /* AS BACKGROUND */
  FRACTIONAL_TYPE_IGNORE,               /* NOT OPERATED  */
  FRACTIONAL_TYPE_FORCE                 /* FORCE DIVISION */
} FractionalType;

typedef struct _PluginParams PluginParams;
struct _PluginParams
{
  gint32          tile_size;
  gint32          division_x;
  gint32          division_y;
  gdouble         move_max_rate;
  FractionalType  fractional_type;
  gboolean        centering;
  gboolean        wrap_around;
  BackgroundType  background_type;
  guchar          background_color[4];
};

/*============================================================================*/
/* VARIABLES                                                                  */
/*============================================================================*/

static struct
{
  PluginParams  params;

  gint32        image;
  GDrawable    *drawable;
  gboolean      drawable_has_alpha;
  
  struct
  {
    gint        x0;
    gint        y0;
    gint        x1;
    gint        y1;
    gint        width;
    gint        height;
  } selection;
  
  GRunModeType  run_mode;
  gboolean      run;
} p =
{
  {
    1,                          /* tile_size             */
    16,                         /* division_x            */
    16,                         /* division_y            */
    25.0,                       /* move_max_rate         */
    FRACTIONAL_TYPE_BACKGROUND, /* fractional_type       */
    TRUE,                       /* centering             */
    FALSE,                      /* wrap_around           */
    BACKGROUND_TYPE_INVERTED,   /* background_type       */
    { 0, 0, 255, 255 }          /* background_color[4]   */
  },

  0,                            /* image                 */
  NULL,                         /* drawable              */
  FALSE,                        /* drawable_has_alpha    */

  { 0, 0, 0, 0, 0, 0 },         /* selection             */
  
  RUN_INTERACTIVE,              /* run_mode              */
  FALSE                         /* run                   */
};

/*----------------------------------------------------------------------------*/

static void
params_save_to_gimp (void)
{
  gimp_set_data (PLUGIN_PROCEDURE_NAME, &p.params, sizeof p.params);
}

static void
params_load_from_gimp (void)
{
  gimp_get_data (PLUGIN_PROCEDURE_NAME, &p.params);

  if (0 < p.params.division_x)
    {
      p.params.tile_size  = p.drawable->width / p.params.division_x;
      if (0 < p.params.tile_size)
	{
	  p.params.division_y = p.drawable->height / p.params.tile_size;
	}
    }
  if (p.params.tile_size <= 0 ||
      p.params.division_x <= 0 ||
      p.params.division_y <= 0)
    {
      p.params.tile_size  = MIN (p.drawable->width, p.drawable->height);
      p.params.division_x = p.drawable->width / p.params.tile_size;
      p.params.division_y = p.drawable->height / p.params.tile_size;
    }
  if (!p.drawable_has_alpha)
    {
      if (p.params.background_type == BACKGROUND_TYPE_TRANSPARENT)
	{
	  p.params.background_type = BACKGROUND_TYPE_INVERTED;
	}
      p.params.background_color[3] = 255;
    }
}

/*============================================================================*/
/* GUI                                                                        */
/*============================================================================*/

static struct
{
  GtkObject *tile_size_adj;
  GtkObject *division_x_adj;
  GtkObject *division_y_adj;
} w;

static void
tile_size_adj_changed (GtkAdjustment *adj)
{
  if (p.params.tile_size != (gint)adj->value)
    {
      p.params.tile_size  = adj->value;
      p.params.division_x = p.drawable->width  / p.params.tile_size;
      p.params.division_y = p.drawable->height / p.params.tile_size;
      gtk_adjustment_set_value (GTK_ADJUSTMENT (w.division_x_adj),
				p.params.division_x);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (w.division_y_adj),
				p.params.division_y);
  }
}

static void
division_x_adj_changed (GtkAdjustment *adj)
{
  if (p.params.division_x != (gint)adj->value)
    {
      p.params.division_x = adj->value;
      p.params.tile_size  = p.drawable->width  / p.params.division_x;
      p.params.division_y =
	p.drawable->height * p.params.division_x / p.drawable->width;
      gtk_adjustment_set_value (GTK_ADJUSTMENT (w.tile_size_adj),
				p.params.tile_size);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (w.division_y_adj),
				p.params.division_y);
  }
}

static void
division_y_adj_changed (GtkAdjustment *adj)
{
  if (p.params.division_y != (gint)adj->value)
    {
      p.params.division_y = adj->value;
      p.params.tile_size  = p.drawable->height / p.params.division_y;
      p.params.division_x =
	p.drawable->width * p.params.division_y / p.drawable->height;
      gtk_adjustment_set_value (GTK_ADJUSTMENT (w.tile_size_adj),
				p.params.tile_size);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (w.division_x_adj),
				p.params.division_x);
  }
}

static void
dialog_ok_clicked (GtkWidget *widget,
		   gpointer   data)
{
  p.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
open_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *main_hbox;
  GtkWidget *button;
  GtkObject *adjustment;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *frame;
  GtkWidget *box;
  GtkWidget *color_button;
  GtkWidget *sep;

  gimp_ui_init ("papertile", TRUE);

  dialog = gimp_dialog_new (_("Paper Tile"), "papertile",
			    gimp_standard_help_func, "filters/papertile.html",
			    GTK_WIN_POS_MOUSE,
			    FALSE, FALSE, FALSE,

			    _("OK"), dialog_ok_clicked,
			    NULL, NULL, NULL, TRUE, FALSE,
			    _("Cancel"), gtk_widget_destroy,
			    NULL, 1, NULL, FALSE, TRUE,

			    NULL);

  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  main_hbox = gtk_hbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (main_hbox), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_hbox);
  gtk_widget_show (main_hbox);

  /* Left */
  vbox = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  frame = gtk_frame_new (_("Division"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  button = gimp_spin_button_new (&w.division_x_adj, p.params.division_x,
				 1.0, p.drawable->width, 1.0, 5.0, 0, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("X:"), 1.0, 0.5,
			     button, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (w.division_x_adj), "value_changed",
		      GTK_SIGNAL_FUNC (division_x_adj_changed),
		      NULL);

  button = gimp_spin_button_new (&w.division_y_adj, p.params.division_y,
				 1.0, p.drawable->width, 1.0, 5.0, 0, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Y:"), 1.0, 0.5,
			     button, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (w.division_y_adj), "value_changed",
		      GTK_SIGNAL_FUNC (division_y_adj_changed),
		      NULL);

  button = gimp_spin_button_new (&w.tile_size_adj, p.params.tile_size,
				 1.0, MAX (p.drawable->width,
					   p.drawable->height),
				 1.0, 5.0, 0, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("Size:"), 1.0, 0.5,
			     button, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (w.tile_size_adj), "value_changed",
		      GTK_SIGNAL_FUNC (tile_size_adj_changed),
		      NULL);

  frame = gimp_radio_group_new2 (TRUE, _("Fractional Pixels"),
				 gimp_radio_button_update,
				 &p.params.fractional_type,
				 (gpointer) p.params.fractional_type,

				 _("Background"),
				 (gpointer) FRACTIONAL_TYPE_BACKGROUND, NULL,
				 _("Ignore"),
				 (gpointer) FRACTIONAL_TYPE_IGNORE, NULL,
				 _("Force"),
				 (gpointer) FRACTIONAL_TYPE_FORCE, NULL,

				 NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  box = GTK_BIN (frame)->child;

  sep = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (box), sep, FALSE, FALSE, 1);
  gtk_widget_show (sep);

  button = gtk_check_button_new_with_label(_("Centering"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), p.params.centering);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &p.params.centering);
  gtk_widget_show (button);

  /* Right */
  vbox = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  frame = gtk_frame_new (_("Movement"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  button = gimp_spin_button_new (&adjustment, p.params.move_max_rate,
				 0.0, 100.0, 1.0, 10.0, 0, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Max (%):"), 1.0, 0.5,
			     button, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &p.params.move_max_rate);

  button = gtk_check_button_new_with_label (_("Wrap Around"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				p.params.wrap_around);
  gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 2, 1, 2);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &p.params.wrap_around);
  gtk_widget_show (button);

  frame = gimp_radio_group_new2 (TRUE, _("Background Type"),
				 gimp_radio_button_update,
				 &p.params.background_type,
				 (gpointer) p.params.background_type,

				 _("Transparent"),
				 (gpointer) BACKGROUND_TYPE_TRANSPARENT, NULL,
				 _("Inverted Image"),
				 (gpointer) BACKGROUND_TYPE_INVERTED, NULL,
				 _("Image"),
				 (gpointer) BACKGROUND_TYPE_IMAGE, NULL,
				 _("Foreground Color"),
				 (gpointer) BACKGROUND_TYPE_FOREGROUND, NULL,
				 _("Background Color"),
				 (gpointer) BACKGROUND_TYPE_BACKGROUND, NULL,
				 (gpointer) 1, /* button without label */
				 (gpointer) BACKGROUND_TYPE_COLOR, &button,

				 NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  color_button = gimp_color_button_new (_("Background Color"), 100, 16,
					p.params.background_color,
					p.drawable_has_alpha ? 4 : 3);
  gtk_container_add (GTK_CONTAINER (button), color_button);
  gtk_widget_show (color_button);

  gtk_widget_set_sensitive (color_button,
			    p.params.background_type == BACKGROUND_TYPE_COLOR);
  gtk_object_set_data (GTK_OBJECT (button), "set_sensitive", color_button);

  gtk_widget_show (dialog);

  gtk_main ();
  gdk_flush ();
}

/*============================================================================*/
/* PLUGIN CORE                                                                */
/*============================================================================*/

typedef struct _Tile Tile;
struct _Tile
{
  guint x;
  guint y;
  gint  z;
  guint width;
  guint height;
  gint  move_x;
  gint  move_y;
};

static gint
tile_compare (const void *x,
	      const void *y)
{
  return ((Tile *) x)->z - ((Tile *) y)->z;
}

/*----------------------------------------------------------------------------*/

static inline gdouble
drand (void)
{
  static gdouble R = 2.0 / (gdouble) G_MAXRAND;
  return (gdouble) rand () * R - 1.0;
}

static inline void
random_move (gint *x,
	     gint *y,
	     gint  max)
{
  gdouble angle  = drand () * G_PI;
  gdouble radius = drand () * (gdouble) max;
  *x = (gint) (radius * cos (angle));
  *y = (gint) (radius * sin (angle));
}

/*----------------------------------------------------------------------------*/

static void
overlap_RGB (guchar       *base,
	     const guchar *top)
{
  base[0] = top[0];
  base[1] = top[1];
  base[2] = top[2];
}

static void
overlap_RGBA (guchar       *base,
	      const guchar *top)
{
  gdouble R1 = (gdouble) base[0] / 255.0;
  gdouble G1 = (gdouble) base[1] / 255.0;
  gdouble B1 = (gdouble) base[2] / 255.0;
  gdouble A1 = (gdouble) base[3] / 255.0;
  gdouble R2 = (gdouble)  top[0] / 255.0;
  gdouble G2 = (gdouble)  top[1] / 255.0;
  gdouble B2 = (gdouble)  top[2] / 255.0;
  gdouble A2 = (gdouble)  top[3] / 255.0;
  gdouble A3 = A2 + A1 * (1.0 - A2);
  if(0.0 < A3)
    {
      gdouble R3 = (R1 * A1 * (1.0 - A2) + R2 * A2) / A3;
      gdouble G3 = (G1 * A1 * (1.0 - A2) + G2 * A2) / A3;
      gdouble B3 = (B1 * A1 * (1.0 - A2) + B2 * A2) / A3;
      R3 = CLAMP (R3, 0.0, 1.0);
      G3 = CLAMP (G3, 0.0, 1.0);
      B3 = CLAMP (B3, 0.0, 1.0);
      base[0] = (guchar) (R3 * 255.0);
      base[1] = (guchar) (G3 * 255.0);
      base[2] = (guchar) (B3 * 255.0);
      base[3] = (guchar) (A3 * 255.0);
    }
  else
    {
      base[0] = 0;
      base[1] = 0;
      base[2] = 0;
      base[3] = 0;
    }
}

/*----------------------------------------------------------------------------*/

static inline void
filter (void)
{
  static void (* overlap)(guchar *, const guchar *);
  GPixelRgn  src;
  GPixelRgn  dst;
  guchar     pixel[4];
  gint       division_x;
  gint       division_y;
  gint       offset_x;
  gint       offset_y;
  Tile      *tiles;
  gint       numof_tiles;
  Tile      *t;
  gint       i;
  gint       x;
  gint       y;
  gint       move_max_pixels;
  guchar    *row_buffer;
  gint       clear_x0;
  gint       clear_y0;
  gint       clear_x1;
  gint       clear_y1;
  gint       clear_width;
  gint       clear_height;
  guchar    *pixels;
  guchar    *buffer;
  gint       dindex;
  gint       sindex;
  gint       px, py;

  /* INITIALIZE */
  gimp_pixel_rgn_init (&src, p.drawable, 0, 0,
		       p.drawable->width, p.drawable->height, FALSE, FALSE);
  gimp_pixel_rgn_init (&dst, p.drawable, 0, 0,
		       p.drawable->width, p.drawable->height, TRUE, TRUE);
  row_buffer = g_new (guchar, p.drawable->bpp * p.drawable->width);
  pixels = g_new (guchar,
		  p.drawable->bpp * p.drawable->width * p.drawable->height);
  buffer = g_new (guchar,
		  p.drawable->bpp * p.params.tile_size * p.params.tile_size);

  overlap = p.drawable_has_alpha ? overlap_RGBA : overlap_RGB;

  gimp_progress_init ("Paper Tile...");

  /* TILES */
  srand (0);
  division_x = p.params.division_x;
  division_y = p.params.division_y;
  if (p.params.fractional_type == FRACTIONAL_TYPE_FORCE)
    {
      if (0 < p.drawable->width  % p.params.tile_size) division_x++;
      if (0 < p.drawable->height % p.params.tile_size) division_y++;
      if (p.params.centering)
	{
	  if (1 < p.drawable->width % p.params.tile_size)
	    {
	      division_x++;
	      offset_x =
		(p.drawable->width % p.params.tile_size) / 2 -
		p.params.tile_size;
	    }
	  else
	    {
	      offset_x = 0;
	    }
	  if (1 < p.drawable->height % p.params.tile_size)
	    {
	      division_y++;
	      offset_y =
		(p.drawable->height % p.params.tile_size) / 2 -
		p.params.tile_size;
	    }
	  else
	    {
	      offset_y = 0;
	    }
	}
      else
	{
	  offset_x = 0;
	  offset_y = 0;
	}
    }
  else
    {
      if (p.params.centering)
	{
	  offset_x = (p.drawable->width  % p.params.tile_size) / 2;
	  offset_y = (p.drawable->height % p.params.tile_size) / 2;
	}
      else
	{
	  offset_x = 0;
	  offset_y = 0;
	}
    }
  move_max_pixels = p.params.move_max_rate * p.params.tile_size / 100.0;
  numof_tiles = division_x * division_y;
  t = tiles = g_new(Tile, numof_tiles);
  for (y = 0; y < division_y; y++)
    {
      gint srcy = offset_y + p.params.tile_size * y;
      for (x = 0; x < division_x; x++, t++)
	{
	  gint srcx = offset_x + p.params.tile_size * x;
	  if (srcx < 0)
	    {
	      t->x     = 0;
	      t->width = srcx + p.params.tile_size;
	    }
	  else if (srcx + p.params.tile_size < p.drawable->width)
	    {
	      t->x     = srcx;
	      t->width = p.params.tile_size;
	    }
	  else
	    {
	      t->x     = srcx;
	      t->width = p.drawable->width - srcx;
	    }
	  if (srcy < 0)
	    {
	      t->y      = 0;
	      t->height = srcy + p.params.tile_size;
	    }
	  else if (srcy + p.params.tile_size < p.drawable->height)
	    {
	      t->y      = srcy;
	      t->height = p.params.tile_size;
	    }
	  else
	    {
	      t->y      = srcy;
	      t->height = p.drawable->height - srcy;
	    }
	  t->z = rand ();
	  random_move (&t->move_x, &t->move_y, move_max_pixels);
	}
    }
  qsort (tiles, numof_tiles, sizeof *tiles, tile_compare);

  /* CLEAR PIXELS */
  for (y = 0; y < p.drawable->height; y++)
    {
      gimp_pixel_rgn_get_row (&src,
			      &pixels[p.drawable->bpp*p.drawable->width*y],
			      0, y, p.drawable->width);
    }
  if (p.params.fractional_type == FRACTIONAL_TYPE_IGNORE)
    {
      clear_x0     = offset_x;
      clear_y0     = offset_y;
      clear_width  = p.params.tile_size * division_x;
      clear_height = p.params.tile_size * division_y;
    }
  else
    {
      clear_x0     = 0;
      clear_y0     = 0;
      clear_width  = p.drawable->width;
      clear_height = p.drawable->height;
    }
  clear_x1 = clear_x0 + clear_width;
  clear_y1 = clear_y0 + clear_height;

  switch (p.params.background_type)
    {
    case BACKGROUND_TYPE_TRANSPARENT:
      for (y = clear_y0; y < clear_y1; y++)
	{
	  for (x = clear_x0; x < clear_x1; x++)
	    {
	      dindex = p.drawable->bpp * (p.drawable->width * y + x);
	      for (i = 0; i < p.drawable->bpp; i++)
		{
		  pixels[dindex+i] = 0;
		}
	    }
	}
      break;

    case BACKGROUND_TYPE_INVERTED:
      for (y = clear_y0; y < clear_y1; y++)
	{
	  for (x = clear_x0; x < clear_x1; x++)
	    {
	      dindex = p.drawable->bpp * (p.drawable->width * y + x);
	      pixels[dindex+0] = 255 - pixels[dindex+0];
	      pixels[dindex+1] = 255 - pixels[dindex+1];
	      pixels[dindex+2] = 255 - pixels[dindex+2];
	    }
	}
      break;

    case BACKGROUND_TYPE_IMAGE:
      break;

    case BACKGROUND_TYPE_FOREGROUND:
      gimp_palette_get_foreground (&pixel[0], &pixel[1], &pixel[2]);
      pixel[3] = 255;
      for (y = clear_y0; y < clear_y1; y++)
	{
	  for (x = clear_x0; x < clear_x1; x++)
	    {
	      dindex = p.drawable->bpp * (p.drawable->width * y + x);
	      for (i = 0; i < p.drawable->bpp; i++)
		{
		  pixels[dindex+i] = pixel[i];
		}
	    }
	}
      break;

    case BACKGROUND_TYPE_BACKGROUND:
      gimp_palette_get_background (&pixel[0], &pixel[1], &pixel[2]);
      pixel[3] = 255;
      for (y = clear_y0; y < clear_y1; y++)
	{
	  for (x = clear_x0; x < clear_x1; x++)
	    {
	      dindex = p.drawable->bpp * (p.drawable->width * y + x);
	      for(i = 0; i < p.drawable->bpp; i++)
		{
		  pixels[dindex+i] = pixel[i];
		}
	    }
	}
      break;

    case BACKGROUND_TYPE_COLOR:
      pixel[0] = p.params.background_color[0];
      pixel[1] = p.params.background_color[1];
      pixel[2] = p.params.background_color[2];
      pixel[3] = p.params.background_color[3];
      for (y = clear_y0; y < clear_y1; y++)
	{
	  for (x = clear_x0; x < clear_x1; x++)
	    {
	      dindex = p.drawable->bpp * (p.drawable->width * y + x);
	      for(i = 0; i < p.drawable->bpp; i++)
		{
		  pixels[dindex+i] = pixel[i];
		}
	    }
	}
      break;
    }

  /* DRAW */
  for (t = tiles, i = 0; i < numof_tiles; i++, t++)
    {
      gint x0 = t->x + t->move_x;
      gint y0 = t->y + t->move_y;

      gimp_pixel_rgn_get_rect (&src, buffer, t->x, t->y, t->width, t->height);

      for (y = 0; y < t->height; y++)
	{
	  py = y0 + y;
	  for (x = 0; x < t->width; x++)
	    {
	      px = x0 + x;
	      sindex = p.drawable->bpp * (t->width * y + x);
	      if (0 <= px && px < p.drawable->width &&
		  0 <= py && py < p.drawable->height)
		{
		  dindex = p.drawable->bpp * (p.drawable->width * py + px);
		  overlap(&pixels[dindex], &buffer[sindex]);
		}
	      else if (p.params.wrap_around)
		{
		  px = (px + p.drawable->width)  % p.drawable->width;
		  py = (py + p.drawable->height) % p.drawable->height;
		  dindex = p.drawable->bpp * (p.drawable->width * py + px);
		  overlap(&pixels[dindex], &buffer[sindex]);
		}
	    }
	}

      gimp_progress_update ((gdouble) i / (gdouble) numof_tiles / 2.0);
    }

  /* SEND IT */
  for (y = 0; y < p.drawable->height; y++)
    {
      gimp_pixel_rgn_set_row (&dst, &pixels[p.drawable->bpp*p.drawable->width*y],
			      0, y, p.drawable->width);
      gimp_progress_update ((gdouble) y /
			    (gdouble) p.drawable->height / 2.0 + 0.5);
    }

  gimp_drawable_flush (p.drawable);
  gimp_drawable_merge_shadow (p.drawable->id, TRUE);
  gimp_drawable_update (p.drawable->id, p.selection.x0, p.selection.y0,
			p.selection.width, p.selection.height);

  /* FREE */
  g_free (buffer);
  g_free (pixels);
  g_free (tiles);
  g_free (row_buffer);
}

/*============================================================================*/
/* PLUGIN INTERFACES                                                          */
/*============================================================================*/

static void
plugin_query (void)
{
  static GParamDef     args[]            =
  {
    { PARAM_INT32,    "run_mode",         "run mode"                         },
    { PARAM_IMAGE,    "image",            "input image"                      },
    { PARAM_DRAWABLE, "drawable",         "input drawable"                   },
    { PARAM_INT32,    "tile_size",        "tile size (pixels)"               },
    { PARAM_FLOAT,    "move_max",         "max move rate (%)"                },
    { PARAM_INT32,    "fractional_type",  "0:Background 1:Ignore 2:Force"    },
    { PARAM_INT32,    "wrap_around",      "wrap around (bool)"               },
    { PARAM_INT32,    "centering",        "centering (bool)"                 },
    { PARAM_INT32,    "background_type",
      "0:Transparent 1:Inverted 2:Image? 3:FG 4:BG 5:Color"                  },
    { PARAM_INT32,    "background_color", "background color (for bg-type 5)" },
    { PARAM_INT32,    "background_alpha", "opacity (for bg-type 5)"          }
  };
  static gint numof_args        = sizeof args / sizeof args[0];

  gimp_install_procedure (PLUGIN_PROCEDURE_NAME,
			  "Cuts an image into paper tiles, and slides each paper tile.",
			  "This plug-in cuts an image into paper tiles and slides each paper tile.",
			  "Hirotsuna Mizuno <s1041150@u-aizu.ac.jp>",
			  "Copyright (c)1997-1999 Hirotsuna Mizuno",
			  _("September 31, 1999"),
			  N_("<Image>/Filters/Map/Paper Tile..."),
			  "RGB*",
			  PROC_PLUG_IN,
			  numof_args, 0,
			  args, NULL);
}

static void
plugin_run (gchar   *name,
	    gint     numof_params,
	    GParam  *params,
	    gint    *numof_return_vals,
	    GParam **return_vals)
{
  GStatusType status;

  INIT_I18N_UI();

  status = STATUS_SUCCESS;
  p.run  = FALSE;
  p.run_mode = params[0].data.d_int32;
  p.image    = params[1].data.d_image;
  p.drawable = gimp_drawable_get(params[2].data.d_drawable);
  p.drawable_has_alpha = gimp_drawable_has_alpha(p.drawable->id);

  gimp_drawable_mask_bounds (p.drawable->id,
			     &p.selection.x0, &p.selection.y0,
			     &p.selection.x1, &p.selection.y1);
  p.selection.width  = p.selection.x1 - p.selection.x0;
  p.selection.height = p.selection.y1 - p.selection.y0;

  if (gimp_drawable_is_rgb (p.drawable->id))
    {
      switch (p.run_mode)
	{
	case RUN_INTERACTIVE:
	  params_load_from_gimp ();
	  open_dialog ();
	  break;

	case RUN_NONINTERACTIVE:
	  if (numof_params == 11)
	    {
	      p.params.tile_size  = params[3].data.d_int32;
	      p.params.division_x = p.drawable->width  / p.params.tile_size;
	      p.params.division_y = p.drawable->height / p.params.tile_size;
	      p.params.move_max_rate = params[4].data.d_float;
	      p.params.fractional_type = (FractionalType)params[5].data.d_int32;
	      p.params.wrap_around = params[6].data.d_int32;
	      p.params.centering = params[7].data.d_int32;
	      p.params.background_type = (BackgroundType)params[8].data.d_int32;
	      p.params.background_color[0] = params[9].data.d_color.red;
	      p.params.background_color[1] = params[9].data.d_color.green;
	      p.params.background_color[2] = params[9].data.d_color.blue;
	      p.params.background_color[3] = params[10].data.d_int32;
	      p.run = TRUE;
	    }
	  else
	    {
	      status = STATUS_CALLING_ERROR;
	    }
	  break;

	case RUN_WITH_LAST_VALS:
	  params_load_from_gimp ();
	  p.run = TRUE;
	  break;
	}
    }
  else
    {
      status = STATUS_EXECUTION_ERROR;
    }

  if (status == STATUS_SUCCESS && p.run)
    {
      if(p.run_mode == RUN_INTERACTIVE)
	{
	  params_save_to_gimp ();
	  gimp_undo_push_group_start (p.image);
	}

      filter ();

      if (p.run_mode == RUN_INTERACTIVE)
	{
	  gimp_undo_push_group_end (p.image);
	  gimp_displays_flush ();
	}
    }
  
  {
    static GParam return_value[1];
    return_value[0].type          = PARAM_STATUS;
    return_value[0].data.d_status = status;
    *numof_return_vals            = 1;
    *return_vals                  = return_value;
  }
}

GPlugInInfo PLUG_IN_INFO =
{
  NULL,
  NULL,
  plugin_query,
  plugin_run
};

MAIN ()
