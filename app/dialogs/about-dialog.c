/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimptoolinfo.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpdnd.h"

#include "about-dialog.h"
#include "authors.h"

#include "gimp-intl.h"


#define ANIMATION_STEPS 16
#define ANIMATION_SIZE   2


static char * wilber2_xpm[] = {
"95 95 9 1",
"       c None",
".      c #FFFFFF",
"+      c #B6B6B6",
"@      c #494949",
"#      c #DBDBDB",
"$      c #6D6D6D",
"%      c #929292",
"&      c #242424",
"*      c #000000",
"                                              .+.                                              ",
"                                     .+.     .+@#     ..+.                                     ",
"                                     #@+.    .$@#     .%&.                                     ",
"                                     #&+.    .&@#.   .+&$.     ..#.                            ",
"                            .++.     +*$..   .$*%.   .%*%.    ..%%.                            ",
"                            .$@.     #@*%.   .#*&.   .#&@.    .%*%.                            ",
"                            .$&+.    .#&&+   .%*&#   .#&*#    #&@..                            ",
"                            .+*&+.   .#*&+  .#&*$.  .#@*&#    #&$.                             ",
"                    .+#.    ..%&&+.  .%*&#  .%**+.  .$**%.   .+&@.    ..++.                    ",
"                    .%@#.    ..@*$.  #@*&#. .$**+.  #**@.. ..+@*@.   .#$&%.                    ",
"                    .+&$..    .@*%.  .@**%. .#&*@.. #&*@.  .%**&+.   .%*%..                    ",
"                     #&*@+.   #&*@.. .+&*&+. .%**%. .$*&#. #&*&+.    .$*#.                     ",
"                  .#+.+$*&+.  #&*&%.. .+&*&+..#&*&#..+**%. #&*@.   ..+&*#                      ",
"              ... .@&...@*%.  .%***$#. .%**&# .@**%. #&*@. .@*@.  .+@&*&.     ....             ",
"              +%..#&$+..@*@.. ..+@**&+..#&**+ .@**$..#**&# .$*&#  #&**@+.   .#+@%.             ",
"              +&%.+*@+#.@**%#.. .#@**@. #&**+.#&**+..%**&. .$*&+  +**$#.    .$*@#.             ",
"              #@&$@**%+#%***@%#...+**&#.+&*@##@**$..%&**$..#&**+ .+**+.    .#&&#.              ",
"              .+@&***@##.%&***&%. #&*&#.@*&+#@**$.#@**&%..#$**&+ .%*&#  ..#+$*%.               ",
"               ..+&**&#...#+&**&#.#&*$.%**@.#&**+.%**&+.#$&***$. .$*&# .#$&**&+.               ",
"                 .@***%.....+&**+.%**+.%**@.#&**+.$**@..@***&%#..+&*&# .%***@%.                ",
"        ...      .$***@......$**+.@**+.+***+.@**$.%**@.+&**$#..#%&**&#..$**%#..   .....        ",
"        #$#.     .$***&%.....#@*#.@**@#.$**&+#&**+#&*&#+**&#.%&*****%.#$@*@..    ..%@&+        ",
"        #@@++#.. .$****&$+....#$#.%***@+#$**&+%**@.$**+#&*@.#&***&@%#%$+$*%.     .%*&%.        ",
"        .+&*&&$. .@******&%.....++#%&**&+#@**$#***#%**$#&*&#+**&%##+@%##&*+. .#+++@&+..        ",
"         .+%$**+..$********$.....#+++@**&+%**&#&**#%**@#@*&#%**$#$&*%..%*&# .+&****$.          ",
"          ...@*&%#%*****@$@*@#....#&$+&**$+**&+***#@**$#&*&+%**&***@..#&*$. .@***&@#.          ",
"            .+***&&*****&##+@@+....%&&&**@+**&@**@%***+$**&+$*****@#..#@%#..+&*@+#..           ",
"             .@**********%..#@&+...#&$%&*&@**&**&@***@%&***&*****&+.........$*&#.              ",
"    ...      ..+++%&*****@...#&@...#&@##@*********************@%&%....++###$**$.      ..##..   ",
"    +$+..#..     ..+*****&#...+&+..+@&$.#&@%+%$$@&&**&***&&@$+#%&#...$&*&&&**&+. .....#@&&%.   ",
"    #@&@@&$#.      #@*****@#...%$...#@&.+$#......##++%%%%+#.+$@&%...+&*******@. .#%$++@*$%#.   ",
"    .#&*&**$#..#++#.#******&+..#%....+%....................#&@%+....@**&$$@&$#..#@*****$...    ",
"     ..#.%**&@&***&+.$*******&%%++%$@&$.............##++#..#+......+**&#........$***&&@#.      ",
"        ..@********@##&*************&%..........+##....+$%#.......+@**%.#++#...%&*@+##..       ",
"         .#@@@%%$&**+.+&************%..........%&&@$.....$$#.....+&**&##@**&$%@**&#.           ",
"          .......+&*&##@***********@...........%$%+@+.....$%.#%$$@***%#@*********%.            ",
"                 .%***&&***********+...............+@#....#$+#+##+@&%#$***@@&***$.. ...  ..... ",
" .##..#+#.  ..###.#&**************&#........#$#.....%#.....#@.....#$%$&*&%#.#%$+....+%#..+@@%# ",
" #@@$$&*@#..+@&&@+.%&*&&@&*********@+#......+*@$$+..........@#.....#&***%..##... ..$&*&$$&&&@+ ",
" .#@**@**&@&*****&#.####.#%*******@&*&$+....+%+++$$#.....##.%+......&**@#+@&@%...#$******&+... ",
"  ..+#.%&****&&***$#.#+$&@&******&+##%@&@#........$+.....%&@%%......@&$#+&****@%$@**@%%$$#.    ",
"      ..%&&$+##+@**&@&***********&#....%&#..#......#.....@**&%...#+.@+#+&**********$#.....     ",
"       ..##..  .#&***************&#....+&#.#$........+.#%****%...$*%&@&***@%+$****@#.          ",
"            .....+&**&@%%$*******&#....+@#.#&&@$@$..#*&&*****#.+#$*******$#...+@@%#.           ",
"           .#%@$#.+%%##++%&******&#...#@+...+%+$$$%.#&******&.#&&&**&**&$##%%#.......#..       ",
"    .###...+&***&#...+@&*********&+...$%.....#.......%&*****%.#@****%%%+#%&**&+.  .#%&@+..#+#..",
".##.+@*&%%$******&++$&************%..+&#.....+#.......%@&**&+..#+@**@%%%@*****&+##+@***&$%&*&%#",
"#$&&*&&*****&$$@*******&&&********@..%$.......%%#......#$@++@$%$@&&*************&&***&&***&$$$#",
".#%@@##$**&$#...%****&$##+&*******&#.%$........%@%%%%%$$%#..#+++%$$@@&***&*********&+##%@$#....",
" .......+$+..  ..%&&@+#+@&*********$.+@.........#+++++#.............+&**$+$****&**@#. .....    ",
"        ... ..#+#.##..$*************$#&$..........%$@$$%...........#&**@+$&****$+#..           ",
"           .#$&*&%#.#$***************$@*$.......+@&@@@$+..........#@***********@#  ....        ",
"          .#$*****@@&**&@%%***********&&*%.....$@%#...............%************&#..#%%#.       ",
"     ..+#.#$***&&*****&%#+@**************&#....#.................#&*************$%$&**&#...    ",
"   ..#@*&&&**@+##%&**&+#@*****************@+....................#@&********************&%%@.   ",
" .#+%@*&****$#. ..+$%##$*******************&&$%+###............#$&@***********&&**&$+%&***&.   ",
" #$&*&%#%@&$#...##....+&**@%@***************@@@&&&@@$+#......#%@**@&*********&@@$%#...+@@%%.   ",
" ..#++....#....%&&@%+%&**%#+&***********&%@*@##&&@&***&@%%%%$&****&$********@$@#...   .....    ",
"             .%*********@#$************&@+&&@$.&*+%****************&%@&**&@$@$#.               ",
"            .#&**&&****&#+&***&*******&+$&*$#$%&*$$*$&***&**********&@@@@$@&*@+###++#.         ",
"        ....#@**$##+$&$+.@**&+%&********&&*%.#@&&@@*@$*&&***************$+#@***&&&**$..        ",
"       .#%%$&**%.. .....+**&#+&***********@+..#&%+*&$&&+%****************$.+*********%....     ",
"      .#@*****@#..#+%+#+&**%#&***&*********&$+.+#.@$#+++&***@***$%@@&&&**&#.%@@@%+%&*&%%%$.    ",
"    ..#%*&@&&@#..#@**&&&**&#%***%%&**********&@+#.####+&****%***@+####+@*&+...#....#@*****.    ",
"    #$&*@#.###. .$********%.@**$#@**************&@@@&****&**%$****@$%#.#**$..      .#%%+%..    ",
"    #%$%#.      #&**@%%$@+.#&*&#$***$%&*****************&$**@#$******@#.&**$#.....  ....       ",
"    .....      .+**$#......%**@#&**$+&**&@&**********&**@%***%#+$@&***%.@***&@@@%#.            ",
"            ...#@*@#...+%%$&**$#&*&#$**&%@**&$**&$&*&%**&#@***$+..+@**$.+&*******%.            ",
"           .#%$&**%..#$******&+#&*@.@**$+&**%@**$%**@+***++@****@#.#&*$..#%$$$@**@..           ",
"           #@****@#..%****&&&+.#**@#&**+%**@+&**+%**&#@**@+#%&***@#.&*@........%&*%##.         ",
"          .+*&@@$+. .@**@+###..%**@.@**#%**@#&**++**&++&**&%##$***+.&**%..    ..$**&@+.        ",
"        ..+@&#....  .&*&#...#+$&**$.$**++**@#@**%.&**@#+&***$.#@*&+.@***$+#..  .#%$@&@#        ",
"        .$*&$.     .#**%..#$*****&#.$**%#@*&+%**&#+&**@##@***#.$*&#.+&*****@+.  ....+@%        ",
"        #++#..    ..$*&+..%***&@%+.#@**$.$**$.@**$.+&**$.#&**%.@*&#..+$@&***@.      .##        ",
"                ..+$**&# .$**@#...+@***+.%**@.+***#.%**&#.@**##@**+. ...#+&*&#                 ",
"                .$****$. .@**+..#@****$..$**@.#&**+.#**&##@*@..@**&+..   .#**+..               ",
"                #&*@$+.. .$*@. .@***$#.#%&**%.#&**+.+**$.+**%..+&**&$+..  .@*&%+.              ",
"               .+*$#..   .@*@. .&**%..#@**&%##@**$.#@*&#.$**$. .+@&**&$.  .+@&*&#.             ",
"              .#@*+.    .#&*@. .&*&#..%**&+..$**$#.%**$..%**&#. ..+$**&#   ..#%*%.             ",
"              #&&%.    .#@**$. .@*&. .@**$. +**&#..@**%..#&**@#.. ..$**+     ..$@.             ",
"              #+#..   .+&**&+. .%*&. .$**+. +**@# .$**@#..+@**&%.. .#&*+.     ..#.             ",
"                      .$*@%#.  .%*&# .+**%. #&*&#..+&*&+. .#%&**%.  #&*$#.                     ",
"                      .@&#..  ..$**+  .&*$. .$**+. .+&*&+. ..%**$.  .%&*@+.                    ",
"                     .#@&.   ..%**&#  .$*&#...@*&#. .+&*&#. .#&*%.  ..#%*$.                    ",
"                     .$&$.   .%**@+.  .%**%. .+**$.  .$**%.  #&*+.    ..@@.                    ",
"                     #@%#.   .$*@#.  ..@**+.  #**&.  .%**+.  #&*%..    .+$#                    ",
"                     ##..    .%*+.   .%**$.  .+**$.  .%*&#   .%&&%.     ...                    ",
"                             .+*%.   #@*$..  .@*@#.  .@*$.   ..#&&#                            ",
"                             #@&+.   .$*+.   .@*%.   .%&@#.    .%&#                            ",
"                             +&%.    .+*%.   .$*%.   ..%*$.    .+@#                            ",
"                             ##..    .+*$.   .#&&#    .#@@.     .#.                            ",
"                                     .$&#.    .$&#     .@$.                                    ",
"                                     .$%.     .@@.     .%+.                                    ",
"                                     .#..     .%+.     ....                                    ",
"                                              ...                                              "};


static gboolean  about_dialog_load_logo   (GtkWidget      *window);
static void      about_dialog_destroy     (GtkObject      *object,
					   gpointer        data);
static void      about_dialog_unmap       (GtkWidget      *widget,
                                           gpointer        data);
static gint      about_dialog_logo_expose (GtkWidget      *widget,
					   GdkEventExpose *event,
					   gpointer        data);
static gint      about_dialog_button      (GtkWidget      *widget,
					   GdkEventButton *event,
					   gpointer        data);
static gint      about_dialog_key         (GtkWidget      *widget,
					   GdkEventKey    *event,
					   gpointer        data);
static void      about_dialog_tool_drop   (GtkWidget      *widget,
					   GimpViewable   *viewable,
					   gpointer        data);
static gboolean  about_dialog_timer       (gpointer        data);


static gboolean     double_speed     = FALSE;

static GtkWidget   *about_dialog     = NULL;
static GtkWidget   *logo_area        = NULL;
static GtkWidget   *scroll_area      = NULL;
static GdkPixmap   *logo_pixmap      = NULL;
static GdkPixmap   *scroll_pixmap    = NULL;
static PangoLayout *scroll_layout    = NULL;
static guchar      *dissolve_map     = NULL;
static gint         dissolve_width;
static gint         dissolve_height;
static gint         logo_width       = 0;
static gint         logo_height      = 0;
static gboolean     do_animation     = FALSE;
static gboolean     do_scrolling     = FALSE;
static gint         scroll_state     = 0;
static gint         frame            = 0;
static gint         offset           = 0;
static gint         timer            = 0;
static gint         hadja_state      = 0;
static gchar      **scroll_text      = authors;
static gint         nscroll_texts    = G_N_ELEMENTS (authors);
static gint         scroll_text_widths[G_N_ELEMENTS (authors)];
static gint         cur_scroll_text  = 0;
static gint         cur_scroll_index = 0;
static gint         shuffle_array[G_N_ELEMENTS (authors)];

static gchar *drop_text[] =
{
  "We are The GIMP." ,
  "Prepare to be manipulated.",
  "Resistance is futile."
};

static gchar *hadja_text[] =
{
  "Hadjaha!",
  "Nej!",
  "Tvärtom!"
};


GtkWidget *
about_dialog_create (void)
{
  GtkWidget *vbox;
  GtkWidget *aboutframe;
  GtkWidget *label;
  GtkWidget *alignment;
  gint       max_width;
  gint       width;
  gint       height;
  gint       i;
  gchar     *label_text;

  if (! about_dialog)
    {
      about_dialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      gtk_window_set_type_hint (GTK_WINDOW (about_dialog),
				GDK_WINDOW_TYPE_HINT_DIALOG);
      gtk_window_set_role (GTK_WINDOW (about_dialog), "gimp-about");
      gtk_window_set_title (GTK_WINDOW (about_dialog), _("About The GIMP"));
      gtk_window_set_position (GTK_WINDOW (about_dialog), GTK_WIN_POS_CENTER);
      gtk_window_set_resizable (GTK_WINDOW (about_dialog), FALSE);

      gimp_help_connect (about_dialog, gimp_standard_help_func,
			 GIMP_HELP_ABOUT_DIALOG, NULL);

      g_signal_connect (about_dialog, "destroy",
			G_CALLBACK (about_dialog_destroy),
			NULL);
      g_signal_connect (about_dialog, "unmap",
			G_CALLBACK (about_dialog_unmap),
			NULL);
      g_signal_connect (about_dialog, "button_press_event",
			G_CALLBACK (about_dialog_button),
			NULL);
      g_signal_connect (about_dialog, "key_press_event",
			G_CALLBACK (about_dialog_key),
			NULL);

      /*  dnd stuff  */
      gtk_drag_dest_set (about_dialog,
                         GTK_DEST_DEFAULT_MOTION |
                         GTK_DEST_DEFAULT_DROP,
                         NULL, 0,
                         GDK_ACTION_COPY);
      gimp_dnd_viewable_dest_add (about_dialog,
				  GIMP_TYPE_TOOL_INFO,
				  about_dialog_tool_drop, NULL);

      gtk_widget_set_events (about_dialog, GDK_BUTTON_PRESS_MASK);

      if (! about_dialog_load_logo (about_dialog))
	{
	  gtk_widget_destroy (about_dialog);
	  about_dialog = NULL;
	  return NULL;
	}

      vbox = gtk_vbox_new (FALSE, 1);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 1);
      gtk_container_add (GTK_CONTAINER (about_dialog), vbox);
      gtk_widget_show (vbox);

      aboutframe = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (aboutframe), GTK_SHADOW_IN);
      gtk_container_set_border_width (GTK_CONTAINER (aboutframe), 0);
      gtk_box_pack_start (GTK_BOX (vbox), aboutframe, TRUE, TRUE, 0);
      gtk_widget_show (aboutframe);

      logo_area = gtk_drawing_area_new ();
      gtk_widget_set_size_request (logo_area, logo_width, logo_height);
      gtk_widget_set_events (logo_area, GDK_EXPOSURE_MASK);
      gtk_container_add (GTK_CONTAINER (aboutframe), logo_area);
      gtk_widget_show (logo_area);

      g_signal_connect (logo_area, "expose_event",
			G_CALLBACK (about_dialog_logo_expose),
			NULL);

      gtk_widget_realize (logo_area);
      gdk_window_set_background (logo_area->window, &logo_area->style->black);

      label_text = g_strdup_printf (_("Version %s brought to you by"),
				    GIMP_VERSION);
      label = gtk_label_new (label_text);
      g_free (label_text);
      label_text = NULL;
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
      gtk_widget_show (label);

      label = gtk_label_new ("Spencer Kimball & Peter Mattis");
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
      gtk_widget_show (label);

      alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
      gtk_box_pack_start (GTK_BOX (vbox), alignment, FALSE, TRUE, 0);
      gtk_widget_show (alignment);

      aboutframe = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (aboutframe), GTK_SHADOW_IN);
      gtk_container_set_border_width (GTK_CONTAINER (aboutframe), 0);
      gtk_container_add (GTK_CONTAINER (alignment), aboutframe);
      gtk_widget_show (aboutframe);

      scroll_layout = gtk_widget_create_pango_layout (aboutframe, NULL);
      g_object_weak_ref (G_OBJECT (aboutframe),
                         (GWeakNotify) g_object_unref, scroll_layout);

      max_width = 0;
      for (i = 0; i < nscroll_texts; i++)
	{
          pango_layout_set_text (scroll_layout, scroll_text[i], -1);
	  pango_layout_get_pixel_size (scroll_layout,
                                       &scroll_text_widths[i], &height);
	  max_width = MAX (max_width, scroll_text_widths[i]);
	}
      for (i = 0; i < (sizeof (drop_text) / sizeof (drop_text[0])); i++)
	{
          pango_layout_set_text (scroll_layout, drop_text[i], -1);
          pango_layout_get_pixel_size (scroll_layout, &width, NULL);
	  max_width = MAX (max_width, width);
	}
      for (i = 0; i < (sizeof (hadja_text) / sizeof (hadja_text[0])); i++)
	{
          pango_layout_set_text (scroll_layout, hadja_text[i], -1);
          pango_layout_get_pixel_size (scroll_layout, &width, NULL);
	  max_width = MAX (max_width, width);
        }
      scroll_area = gtk_drawing_area_new ();
      gtk_widget_set_size_request (scroll_area, max_width + 6, height + 1);
      gtk_widget_set_events (scroll_area, GDK_BUTTON_PRESS_MASK);
      gtk_container_add (GTK_CONTAINER (aboutframe), scroll_area);
      gtk_widget_show (scroll_area);

      label =
	gtk_label_new (_("Visit http://www.gimp.org/ for more information"));
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
      gtk_widget_show (label);

      gtk_widget_realize (scroll_area);
      gdk_window_set_background (scroll_area->window,
				 &scroll_area->style->white);
    }

  if (! GTK_WIDGET_VISIBLE (about_dialog))
    {
      do_animation    = TRUE;
      do_scrolling    = FALSE;
      scroll_state    = 0;
      frame           = 0;
      offset          = 0;
      cur_scroll_text = 0;

      if (! double_speed && hadja_state != 7)
	{
	  GRand *gr = g_rand_new ();

	  for (i = 0; i < nscroll_texts; i++)
	    {
	      shuffle_array[i] = i;
	    }

	  for (i = 0; i < nscroll_texts; i++)
	    {
	      gint j;

	      j = g_rand_int_range (gr, 0, nscroll_texts);
	      if (i != j)
		{
		  gint t;

		  t = shuffle_array[j];
		  shuffle_array[j] = shuffle_array[i];
		  shuffle_array[i] = t;
		}
	    }

	  cur_scroll_text = g_rand_int_range (gr, 0, nscroll_texts);
          pango_layout_set_text (scroll_layout,
                                 scroll_text[cur_scroll_text], -1);

	  g_rand_free (gr);
	}
    }

  gtk_window_present (GTK_WINDOW (about_dialog));

  return about_dialog;
}

static gboolean
about_dialog_load_logo (GtkWidget *window)
{
  gchar     *filename;
  GdkPixbuf *pixbuf;
  GdkGC     *gc;
  gint       i, j, k;
  GRand     *gr;

  if (logo_pixmap)
    return TRUE;

  filename = g_build_filename (gimp_data_directory (), "images",
                               "gimp_logo.png", NULL);

  pixbuf = gdk_pixbuf_new_from_file (filename, NULL);

  g_free (filename);

  if (! pixbuf)
    return FALSE;

  logo_width  = gdk_pixbuf_get_width (pixbuf);
  logo_height = gdk_pixbuf_get_height (pixbuf);

  gtk_widget_realize (window);

  logo_pixmap = gdk_pixmap_new (window->window,
                                logo_width, logo_height,
                                gtk_widget_get_visual (window)->depth);

  gc = gdk_gc_new (logo_pixmap);

  gdk_draw_pixbuf (GDK_DRAWABLE (logo_pixmap), gc, pixbuf,
                   0, 0, 0, 0, logo_width, logo_height,
                   GDK_RGB_DITHER_NORMAL, 0, 0);

  g_object_unref (gc);
  g_object_unref (pixbuf);

  dissolve_width =
    (logo_width / ANIMATION_SIZE) +
    (logo_width % ANIMATION_SIZE == 0 ? 0 : 1);

  dissolve_height =
    (logo_height / ANIMATION_SIZE) +
    (logo_height % ANIMATION_SIZE == 0 ? 0 : 1);

  dissolve_map = g_new (guchar, dissolve_width * dissolve_height);

  gr = g_rand_new ();

  for (i = 0, k = 0; i < dissolve_height; i++)
    for (j = 0; j < dissolve_width; j++, k++)
      dissolve_map[k] = g_rand_int_range (gr, 0, ANIMATION_STEPS);

  g_rand_free (gr);

  return TRUE;
}

static void
about_dialog_destroy (GtkObject *object,
		      gpointer   data)
{
  about_dialog = NULL;
  about_dialog_unmap (NULL, NULL);
}

static void
about_dialog_unmap (GtkWidget *widget,
                    gpointer   data)
{
  if (timer)
    {
      g_source_remove (timer);
      timer = 0;
    }
}

static gint
about_dialog_logo_expose (GtkWidget      *widget,
			  GdkEventExpose *event,
			  gpointer        data)
{
  if (do_animation)
    {
      if (!timer)
	{
	  about_dialog_timer (widget);
	  timer = g_timeout_add (75, about_dialog_timer, NULL);
	}
    }
  else
    {
      /* If we draw beyond the boundaries of the pixmap, then X
	 will generate an expose area for those areas, starting
	 an infinite cycle. We now set allow_grow = FALSE, so
	 the drawing area can never be bigger than the preview.
         Otherwise, it would be necessary to intersect event->area
         with the pixmap boundary rectangle. */

      gdk_draw_drawable (widget->window,
			 widget->style->black_gc,
			 logo_pixmap,
			 event->area.x, event->area.y,
			 event->area.x, event->area.y,
			 event->area.width, event->area.height);
    }

  return FALSE;
}

static gint
about_dialog_button (GtkWidget      *widget,
		     GdkEventButton *event,
		     gpointer        data)
{
  if (timer)
    {
      g_source_remove (timer);
      timer = 0;
    }

  frame = 0;

  gtk_widget_hide (about_dialog);

  return FALSE;
}

static gint
about_dialog_key (GtkWidget      *widget,
		  GdkEventKey    *event,
		  gpointer        data)
{
  gint i;

  if (hadja_state == 7)
    return FALSE;

  switch (event->keyval)
    {
    case GDK_h:
    case GDK_H:
      if (hadja_state == 0 || hadja_state == 5)
	hadja_state++;
      else
	hadja_state = 1;
      break;
    case GDK_a:
    case GDK_A:
      if (hadja_state == 1 || hadja_state == 4 || hadja_state == 6)
	hadja_state++;
      else
	hadja_state = 0;
      break;
    case GDK_d:
    case GDK_D:
      if (hadja_state == 2)
	hadja_state++;
      else
	hadja_state = 0;
      break;
    case GDK_j:
    case GDK_J:
      if (hadja_state == 3)
	hadja_state++;
      else
	hadja_state = 0;
      break;
    default:
      hadja_state = 0;
    }

  if (hadja_state == 7)
    {
      scroll_text = hadja_text;
      nscroll_texts = G_N_ELEMENTS (hadja_text);

      for (i = 0; i < nscroll_texts; i++)
	{
	  shuffle_array[i] = i;
          pango_layout_set_text (scroll_layout, scroll_text[i], -1);
          pango_layout_get_pixel_size (scroll_layout,
                                       &scroll_text_widths[i], NULL);
	}

      scroll_state     = 0;
      cur_scroll_index = 0;
      cur_scroll_text  = 0;
      offset           = 0;
      pango_layout_set_text (scroll_layout, scroll_text[cur_scroll_text], -1);
    }

  return FALSE;
}

static void
about_dialog_tool_drop (GtkWidget    *widget,
			GimpViewable *viewable,
			gpointer      data)
{
  GdkPixmap *pixmap = NULL;
  GdkBitmap *mask   = NULL;
  gint       width  = 0;
  gint       height = 0;
  gint       i;

  if (do_animation)
    return;

  if (timer)
    g_source_remove (timer);

  timer = g_timeout_add (75, about_dialog_timer, NULL);

  frame        = 0;
  do_animation = TRUE;
  do_scrolling = FALSE;

  gdk_draw_rectangle (logo_pixmap,
		      logo_area->style->white_gc,
		      TRUE,
		      0, 0,
		      logo_area->allocation.width,
		      logo_area->allocation.height);

  pixmap = gdk_pixmap_create_from_xpm_d (widget->window,
					 &mask,
					 NULL,
					 wilber2_xpm);

  gdk_drawable_get_size (pixmap, &width, &height);

  if (logo_area->allocation.width  >= width &&
      logo_area->allocation.height >= height)
    {
      gint x, y;

      x = (logo_area->allocation.width  - width) / 2;
      y = (logo_area->allocation.height - height) / 2;

      gdk_gc_set_clip_mask (logo_area->style->black_gc, mask);
      gdk_gc_set_clip_origin (logo_area->style->black_gc, x, y);

      gdk_draw_drawable (logo_pixmap,
                         logo_area->style->black_gc,
                         pixmap, 0, 0,
                         x, y,
                         width, height);

      gdk_gc_set_clip_mask (logo_area->style->black_gc, NULL);
      gdk_gc_set_clip_origin (logo_area->style->black_gc, 0, 0);
    }

  g_object_unref (pixmap);
  g_object_unref (mask);

  scroll_text = drop_text;
  nscroll_texts = G_N_ELEMENTS (drop_text);

  for (i = 0; i < nscroll_texts; i++)
    {
      shuffle_array[i] = i;
      pango_layout_set_text (scroll_layout, scroll_text[i], -1);
      pango_layout_get_pixel_size (scroll_layout,
                                   &scroll_text_widths[i], NULL);
    }

  scroll_state     = 0;
  cur_scroll_index = 0;
  cur_scroll_text  = 0;
  offset           = 0;
  pango_layout_set_text (scroll_layout, scroll_text[cur_scroll_text], -1);

  double_speed = TRUE;
}

static gboolean
about_dialog_timer (gpointer data)
{
  gint i, j, k;
  gint return_val;

  return_val = TRUE;

  if (do_animation)
    {
      if (logo_area->allocation.width != 1)
	{
	  for (i = 0, k = 0; i < dissolve_height; i++)
	    for (j = 0; j < dissolve_width; j++, k++)
	      if (frame == dissolve_map[k])
		{
		  gdk_draw_drawable (logo_area->window,
				     logo_area->style->black_gc,
				     logo_pixmap,
				     j * ANIMATION_SIZE, i * ANIMATION_SIZE,
				     j * ANIMATION_SIZE, i * ANIMATION_SIZE,
				     ANIMATION_SIZE, ANIMATION_SIZE);
		}

	  frame += 1;

	  if (frame == ANIMATION_STEPS)
	    {
	      do_animation = FALSE;
	      do_scrolling = TRUE;
	      frame        = 0;

	      timer = g_timeout_add (75, about_dialog_timer, NULL);

	      return FALSE;
	    }
	}
    }

  if (do_scrolling)
    {
      if (! scroll_pixmap)
	scroll_pixmap = gdk_pixmap_new (scroll_area->window,
					scroll_area->allocation.width,
					scroll_area->allocation.height,
					-1);

      switch (scroll_state)
	{
	case 1:
	  scroll_state = 2;
	  timer = g_timeout_add (700, about_dialog_timer, NULL);
	  return_val = FALSE;
	  break;
	case 2:
	  scroll_state = 3;
	  timer = g_timeout_add (75, about_dialog_timer, NULL);
	  return_val = FALSE;
	  break;
	}

      if (offset > (scroll_text_widths[cur_scroll_text] +
		    scroll_area->allocation.width))
	{
	  scroll_state = 0;
	  cur_scroll_index += 1;
	  if (cur_scroll_index == nscroll_texts)
	    cur_scroll_index = 0;

	  cur_scroll_text = shuffle_array[cur_scroll_index];
          pango_layout_set_text (scroll_layout,
                                 scroll_text[cur_scroll_text], -1);
	  offset = 0;
	}

      gdk_draw_rectangle (scroll_pixmap,
			  scroll_area->style->white_gc,
			  TRUE, 0, 0,
			  scroll_area->allocation.width,
			  scroll_area->allocation.height);
      gdk_draw_layout (scroll_pixmap,
                       scroll_area->style->black_gc,
                       scroll_area->allocation.width - offset, 0,
                       scroll_layout);
      gdk_draw_drawable (scroll_area->window,
		 	 scroll_area->style->black_gc,
			 scroll_pixmap, 0, 0, 0, 0,
			 scroll_area->allocation.width,
			 scroll_area->allocation.height);

      offset += 15;
      if (scroll_state == 0)
	{
	  if (offset > ((scroll_area->allocation.width +
			 scroll_text_widths[cur_scroll_text]) / 2))
	    {
	      scroll_state = 1;
	      offset = (scroll_area->allocation.width +
			scroll_text_widths[cur_scroll_text]) / 2;
	    }
	}
    }

  return return_val;
}
