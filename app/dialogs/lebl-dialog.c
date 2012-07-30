#include "config.h"

#include <string.h>
#include <math.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gimp-intl.h"

static const guint8 goatpb2[];
static const guint8 goatpb[];
static const guint8 wanda[];

/* phish code */
#define PHSHFRAMES 8
#define PHSHORIGWIDTH 288
#define PHSHORIGHEIGHT 22
#define PHSHWIDTH (PHSHORIGWIDTH/PHSHFRAMES)
#define PHSHHEIGHT PHSHORIGHEIGHT
#define PHSHCHECKTIMEOUT (g_random_int()%120*1000)
#define PHSHTIMEOUT 120
#define PHSHHIDETIMEOUT 80
#define PHSHXS 5
#define PHSHYS ((g_random_int() % 2) + 1)
#define PHSHXSHIDEFACTOR 2.5
#define PHSHYSHIDEFACTOR 2.5
#define PHSHPIXELSTOREMOVE(p) (p[3] < 55 || p[2] > 200)

static void
phsh_unsea(GdkPixbuf *gp)
{
        guchar *pixels = gdk_pixbuf_get_pixels (gp);
        int rs = gdk_pixbuf_get_rowstride (gp);
	int w = gdk_pixbuf_get_width (gp);
        int h = gdk_pixbuf_get_height (gp);
        int x, y;

        for (y = 0; y < h; y++, pixels += rs) {
                guchar *p = pixels;
                for (x = 0; x < w; x++, p+=4) {
                        if (PHSHPIXELSTOREMOVE(p))
                               p[3] = 0;
                }
        }
}

static GdkPixbuf *
get_phsh_frame (GdkPixbuf *pb, int frame)
{
        GdkPixbuf *newpb;

        newpb = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
				PHSHWIDTH, PHSHHEIGHT);
        gdk_pixbuf_copy_area (pb, frame * PHSHWIDTH, 0,
			      PHSHWIDTH, PHSHHEIGHT, newpb, 0, 0);

	return newpb;
}

typedef struct {
	gboolean live;
	int x, y;
} InvGoat;

typedef struct {
	gboolean good;
	int y;
	int x;
} InvShot;


static GtkWidget *geginv = NULL;
static GtkWidget *geginv_canvas = NULL;
static GtkWidget *geginv_label = NULL;
static GdkPixbuf *inv_goat1 = NULL;
static GdkPixbuf *inv_goat2 = NULL;
static GdkPixbuf *inv_phsh1 = NULL;
static GdkPixbuf *inv_phsh2 = NULL;
static int inv_phsh_state = 0;
static int inv_goat_state = 0;
static int inv_width = 0;
static int inv_height = 0;
static int inv_goat_width = 0;
static int inv_goat_height = 0;
static int inv_phsh_width = 0;
static int inv_phsh_height = 0;
#define INV_ROWS 3
#define INV_COLS 5
static InvGoat invs[INV_COLS][INV_ROWS] = { { { FALSE, 0, 0 } } };
static int inv_num = INV_ROWS * INV_COLS;
static double inv_factor = 1.0;
static int inv_our_x = 0;
static int inv_x = 0;
static int inv_y = 0;
static int inv_first_col = 0;
static int inv_last_col = INV_COLS-1;
static int inv_level = 0;
static int inv_lives = 0;
static gboolean inv_do_pause = FALSE;
static gboolean inv_reverse = FALSE;
static gboolean inv_game_over = FALSE;
static gboolean inv_left_pressed = FALSE;
static gboolean inv_right_pressed = FALSE;
static gboolean inv_fire_pressed = FALSE;
static gboolean inv_left_released = FALSE;
static gboolean inv_right_released = FALSE;
static gboolean inv_fire_released = FALSE;
static gboolean inv_paused = FALSE;
static GSList *inv_shots = NULL;
static guint inv_draw_idle = 0;

static void
inv_show_status (void)
{
	gchar *s, *t, *u, *v, *w;
	if (geginv == NULL)
		return;

	if (inv_game_over) {
		t = g_strdup_printf (_("<b>GAME OVER</b> at level %d!"),
				     inv_level+1); 	
		u = g_strdup_printf ("<big>%s</big>", t); 
		/* Translators: the first and third strings are similar to a
		 * title, and the second string is a small information text.
		 * The spaces are there only to separate all the strings, so
		 try to keep them as is. */
		s = g_strdup_printf (_("%1$s   %2$s   %3$s"),
				     u, _("Press 'q' to quit"), u);
		g_free (t);
		g_free (u);

	} else if (inv_paused) {
		t = g_strdup_printf("<big><b>%s</b></big>", _("Paused"));
		/* Translators: the first string is a title and the second
		 * string is a small information text. */
		s = g_strdup_printf (_("%1$s\t%2$s"),
				     t, _("Press 'p' to unpause"));
		g_free (t);

	} else {
		t = g_strdup_printf ("<b>%d</b>", inv_level+1);
		u = g_strdup_printf ("<b>%d</b>", inv_lives);
		v = g_strdup_printf (_("Level: %s,  Lives: %s"), t, u);
		w = g_strdup_printf ("<big>%s</big>", v);
		/* Translators: the first string is a title and the second
		 * string is a small information text. */
		s = g_strdup_printf (_("%1$s\t%2$s"), w,
				     _("Left/Right to move, Space to fire, 'p' to pause, 'q' to quit"));
		g_free (t);
		g_free (u);		
		g_free (v);
		g_free (w);

	}
	gtk_label_set_markup (GTK_LABEL (geginv_label), s);

	g_free (s);
}

static gboolean
inv_draw (gpointer data)
{
        gtk_widget_queue_draw (data);

        return FALSE;
}

static void
inv_queue_draw (GtkWidget *window)
{
       if (inv_draw_idle == 0)
               inv_draw_idle = g_idle_add (inv_draw, window);
}

static void
inv_draw_explosion (int x, int y)
{
        cairo_t *cr;
        int i;

        if ( ! gtk_widget_is_drawable (geginv_canvas))
                return;

        cr = gdk_cairo_create ( gtk_widget_get_window (geginv_canvas));

        cairo_set_source_rgb (cr, 1.0, 0.0, 0.0);

        for (i = 5; i < 100; i += 5) {
                cairo_arc (cr, x, y, i, 0, 2 * G_PI);
                cairo_fill (cr);
                gdk_flush ();
                g_usleep (50000);
        }

        cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);

        for (i = 5; i < 100; i += 5) {
                cairo_arc (cr, x, y, i, 0, 2 * G_PI);
                cairo_fill (cr);
                gdk_flush ();
                g_usleep (50000);
        }

        cairo_destroy (cr);

	inv_queue_draw (geginv);
}


static void
inv_do_game_over (void)
{
	GSList *li;

	inv_game_over = TRUE;
	
	for (li = inv_shots; li != NULL; li = li->next) {
		InvShot *shot = li->data;
		shot->good = FALSE;
	}

	inv_queue_draw (geginv);

	inv_show_status ();
}


static GdkPixbuf *
pb_scale (GdkPixbuf *pb, double scale)
{
	int w, h;
	
	if (scale == 1.0)
		return (GdkPixbuf *)g_object_ref ((GObject *)pb);

	w = gdk_pixbuf_get_width (pb) * scale;
	h = gdk_pixbuf_get_height (pb) * scale;

	return gdk_pixbuf_scale_simple (pb, w, h,
					GDK_INTERP_BILINEAR);
}

static void
refind_first_and_last (void)
{
	int i, j;

	for (i = 0; i < INV_COLS; i++) {
		gboolean all_null = TRUE;
		for (j = 0; j < INV_ROWS; j++) {
			if (invs[i][j].live) {
				all_null = FALSE;
				break;
			}
		}
		if ( ! all_null) {
			inv_first_col = i;
			break;
		}
	}

	for (i = INV_COLS-1; i >= 0; i--) {
		gboolean all_null = TRUE;
		for (j = 0; j < INV_ROWS; j++) {
			if (invs[i][j].live) {
				all_null = FALSE;
				break;
			}
		}
		if ( ! all_null) {
			inv_last_col = i;
			break;
		}
	}
}

static void
whack_gegl (int i, int j)
{
	if ( ! invs[i][j].live)
		return;

	invs[i][j].live = FALSE;
	inv_num --;

	if (inv_num > 0) {
		refind_first_and_last ();
	} else {
		inv_x = 70;
		inv_y = 70;
		inv_first_col = 0;
		inv_last_col = INV_COLS-1;
		inv_reverse = FALSE;

		g_slist_foreach (inv_shots, (GFunc)g_free, NULL);
		g_slist_free (inv_shots);
		inv_shots = NULL;

		for (i = 0; i < INV_COLS; i++) {
			for (j = 0; j < INV_ROWS; j++) {
				invs[i][j].live = TRUE;
				invs[i][j].x = 70 + i * 100;
				invs[i][j].y = 70 + j * 80;
			}
		}
		inv_num = INV_ROWS * INV_COLS;

		inv_level ++;

		inv_show_status ();
	}

	inv_queue_draw (geginv);
}

static gboolean
geginv_timeout (gpointer data)
{
	int i, j;
	int limitx1;
	int limitx2;
	int speed;
	int shots;
	int max_shots;

	if (inv_paused)
		return TRUE;

	if (geginv != data ||
	    inv_num <= 0 ||
	    inv_y > 700)
		return FALSE;

	limitx1 = 70 - (inv_first_col * 100);
	limitx2 = 800 - 70 - (inv_last_col * 100);

	if (inv_game_over) {
		inv_y += 30;
	} else {
		if (inv_num < (INV_COLS*INV_ROWS)/3)
			speed = 45+2*inv_level;
		else if (inv_num < (2*INV_COLS*INV_ROWS)/3)
			speed = 30+2*inv_level;
		else
			speed = 15+2*inv_level;

		if (inv_reverse) {
			inv_x -= speed;
			if (inv_x < limitx1) {
				inv_reverse = FALSE;
				inv_x = (limitx1 + (limitx1 - inv_x));
				inv_y += 30+inv_level;
			}
		} else {
			inv_x += speed;
			if (inv_x > limitx2) {
				inv_reverse = TRUE;
				inv_x = (limitx2 - (inv_x - limitx2));
				inv_y += 30+inv_level;
			}
		}
	}

	for (i = 0; i < INV_COLS; i++) {
		for (j = 0; j < INV_ROWS; j++) {
			if (invs[i][j].live) {
				invs[i][j].x = inv_x + i * 100;
				invs[i][j].y = inv_y + j * 80;

				if ( ! inv_game_over &&
				    invs[i][j].y >= 570) {
					inv_do_game_over ();
				} else if ( ! inv_game_over &&
					   invs[i][j].y >= 530 &&
					   invs[i][j].x + 40 > inv_our_x - 25 &&
					   invs[i][j].x - 40 < inv_our_x + 25) {
					whack_gegl (i,j);
					inv_lives --;
					inv_draw_explosion (inv_our_x, 550);
					if (inv_lives <= 0) {
						inv_do_game_over ();
					} else {
						g_slist_foreach (inv_shots, (GFunc)g_free, NULL);
						g_slist_free (inv_shots);
						inv_shots = NULL;
						inv_our_x = 400;
						inv_do_pause = TRUE;
						inv_show_status ();
					}
				}
			}
		}
	}

	shots = 0;
	max_shots = (g_random_int () >> 3) % (2+inv_level);
	while ( ! inv_game_over && shots < MIN (max_shots, inv_num)) {
		int i = (g_random_int () >> 3) % INV_COLS;
		for (j = INV_ROWS-1; j >= 0; j--) {
			if (invs[i][j].live) {
				InvShot *shot = g_new0 (InvShot, 1);

				shot->good = FALSE;
				shot->x = invs[i][j].x + (g_random_int () % 6) - 3;
				shot->y = invs[i][j].y + inv_goat_height/2 + (g_random_int () % 3);

				inv_shots = g_slist_prepend (inv_shots, shot);
				shots++;
				break;
			}
		}
	}

	inv_goat_state = (inv_goat_state+1) % 2;

	inv_queue_draw (geginv);

	g_timeout_add (((inv_num/4)+1) * 100, geginv_timeout, geginv);

	return FALSE;
}

static gboolean
find_gegls (int x, int y)
{
	int i, j;

	/* FIXME: this is stupid, we can do better */
	for (i = 0; i < INV_COLS; i++) {
		for (j = 0; j < INV_ROWS; j++) {
			int ix = invs[i][j].x;
			int iy = invs[i][j].y;

			if ( ! invs[i][j].live)
				continue;

			if (y >= iy - 30 &&
			    y <= iy + 30 &&
			    x >= ix - 40 &&
			    x <= ix + 40) {
				whack_gegl (i, j);
				return TRUE;
			}
		}
	}

	return FALSE;
}


static gboolean
geginv_move_timeout (gpointer data)
{
	GSList *li;
	static int shot_inhibit = 0;

	if (inv_paused)
		return TRUE;

	if (geginv != data ||
	    inv_num <= 0 ||
	    inv_y > 700)
		return FALSE;

	inv_phsh_state = (inv_phsh_state+1)%10;

	/* we will be drawing something */
	if (inv_shots != NULL)
		inv_queue_draw (geginv);

	li = inv_shots;
	while (li != NULL) {
		InvShot *shot = li->data;

		if (shot->good) {
			shot->y -= 30;
			if (find_gegls (shot->x, shot->y) ||
			    shot->y <= 0) {
				GSList *list = li;
				/* we were restarted */
				if (inv_shots == NULL)
					return TRUE;
				li = li->next;
				g_free (shot);
				inv_shots = g_slist_delete_link (inv_shots, list);
				continue;
			}
		} else /* bad */ {
			shot->y += 30;
			if ( ! inv_game_over &&
			    shot->y >= 535 &&
			    shot->y <= 565 &&
			    shot->x >= inv_our_x - 25 &&
			    shot->x <= inv_our_x + 25) {
				inv_lives --;
				inv_draw_explosion (inv_our_x, 550);
				if (inv_lives <= 0) {
					inv_do_game_over ();
				} else {
					g_slist_foreach (inv_shots, (GFunc)g_free, NULL);
					g_slist_free (inv_shots);
					inv_shots = NULL;
					inv_our_x = 400;
					inv_do_pause = TRUE;
					inv_show_status ();
					return TRUE;
				}
			}

			if (shot->y >= 600) {
				GSList *list = li;
				li = li->next;
				g_free (shot);
				inv_shots = g_slist_delete_link (inv_shots, list);
				continue;
			}
		}

		li = li->next;
	}

	if ( ! inv_game_over) {
		if (inv_left_pressed && inv_our_x > 100) {
			inv_our_x -= 20;
			inv_queue_draw (geginv);
		} else if (inv_right_pressed && inv_our_x < 700) {
			inv_our_x += 20;
			inv_queue_draw (geginv);
		}
	}

	if (shot_inhibit > 0)
		shot_inhibit--;

	if ( ! inv_game_over && inv_fire_pressed && shot_inhibit == 0) {
		InvShot *shot = g_new0 (InvShot, 1);

		shot->good = TRUE;
		shot->x = inv_our_x;
		shot->y = 540;

		inv_shots = g_slist_prepend (inv_shots, shot);

		shot_inhibit = 5;

		inv_queue_draw (geginv);
	}

	if (inv_left_released)
		inv_left_pressed = FALSE;
	if (inv_right_released)
		inv_right_pressed = FALSE;
	if (inv_fire_released)
		inv_fire_pressed = FALSE;

	return TRUE;
}

static gboolean
inv_key_press (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	switch (event->keyval) {
	case GDK_KEY_Left:
	case GDK_KEY_KP_Left:
	case GDK_KEY_Pointer_Left:
		inv_left_pressed = TRUE;
		inv_left_released = FALSE;
		return TRUE;
	case GDK_KEY_Right:
	case GDK_KEY_KP_Right:
	case GDK_KEY_Pointer_Right:
		inv_right_pressed = TRUE;
		inv_right_released = FALSE;
		return TRUE;
	case GDK_KEY_space:
	case GDK_KEY_KP_Space:
		inv_fire_pressed = TRUE;
		inv_fire_released = FALSE;
		return TRUE;
	default:
		break;
	}
	return FALSE;
}

static gboolean
inv_key_release (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	switch (event->keyval) {
	case GDK_KEY_Left:
	case GDK_KEY_KP_Left:
	case GDK_KEY_Pointer_Left:
		inv_left_released = TRUE;
		return TRUE;
	case GDK_KEY_Right:
	case GDK_KEY_KP_Right:
	case GDK_KEY_Pointer_Right:
		inv_right_released = TRUE;
		return TRUE;
	case GDK_KEY_space:
	case GDK_KEY_KP_Space:
		inv_fire_released = TRUE;
		return TRUE;
	case GDK_KEY_q:
	case GDK_KEY_Q:
		gtk_widget_destroy (widget);
		return TRUE;
	case GDK_KEY_p:
	case GDK_KEY_P:
		inv_paused = ! inv_paused;
		inv_show_status ();
		return TRUE;
	default:
		break;
	}
	return FALSE;
}

static gboolean
ensure_creatures (void)
{
        GdkPixbuf *pb, *pb1;

	if (inv_goat1 != NULL)
		return TRUE;

	pb = gdk_pixbuf_new_from_inline (-1, wanda, TRUE, NULL);
	if (pb == NULL)
		return FALSE;

	pb1 = get_phsh_frame (pb, 1);
	inv_phsh1 = pb_scale (pb1, inv_factor);
	g_object_unref (G_OBJECT (pb1));
	phsh_unsea (inv_phsh1);

	pb1 = get_phsh_frame (pb, 2);
	inv_phsh2 = pb_scale (pb1, inv_factor);
	g_object_unref (G_OBJECT (pb1));
	phsh_unsea (inv_phsh2);

	g_object_unref (G_OBJECT (pb));

	pb = gdk_pixbuf_new_from_inline (-1, goatpb, TRUE, NULL);
	if (pb == NULL) {
		g_object_unref (G_OBJECT (inv_phsh1));
		g_object_unref (G_OBJECT (inv_phsh2));
		return FALSE;
	}

	inv_goat1 = pb_scale (pb, inv_factor * 0.66);
	g_object_unref (pb);

	pb = gdk_pixbuf_new_from_inline (-1, goatpb2, TRUE, NULL);
	if (pb == NULL) {
		g_object_unref (G_OBJECT (inv_goat1));
		g_object_unref (G_OBJECT (inv_phsh1));
		g_object_unref (G_OBJECT (inv_phsh2));
		return FALSE;
	}

	inv_goat2 = pb_scale (pb, inv_factor * 0.66);
	g_object_unref (pb);

	inv_goat_width = gdk_pixbuf_get_width (inv_goat1);
	inv_goat_height = gdk_pixbuf_get_height (inv_goat1);
	inv_phsh_width = gdk_pixbuf_get_width (inv_phsh1);
	inv_phsh_height = gdk_pixbuf_get_height (inv_phsh1);

	return TRUE;
}

static void
geginv_destroyed (GtkWidget *w, gpointer data)
{
	geginv = NULL;
}

static gboolean
inv_expose (GtkWidget *widget, GdkEventExpose *event)
{
        cairo_t *cr;
	GdkPixbuf *goat;
	GSList *li;
	int i, j;

	if (geginv == NULL) {
		inv_draw_idle = 0;
		return TRUE;
	}

	if ( ! gtk_widget_is_drawable (geginv_canvas))
		return TRUE;

        cr = gdk_cairo_create (gtk_widget_get_window (geginv_canvas));

        cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
        cairo_paint (cr);

	if (inv_goat_state == 0)
		goat = inv_goat1;
	else
		goat = inv_goat2;

	for (i = 0; i < INV_COLS; i++) {
		for (j = 0; j < INV_ROWS; j++) {
			int x, y;
			if ( ! invs[i][j].live)
				continue;

			x = invs[i][j].x*inv_factor - inv_goat_width/2,
			y = invs[i][j].y*inv_factor - inv_goat_height/2,

                        gdk_cairo_set_source_pixbuf (cr, goat, x, y);
                        cairo_rectangle (cr,
                                         x, y,
                                         inv_goat_width,
                                         inv_goat_height);
                        cairo_fill (cr);
		}
	}

	for (li = inv_shots; li != NULL; li = li->next) {
		InvShot *shot = li->data;

                cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
                cairo_rectangle (cr,
                                 (shot->x-1)*inv_factor,
                                 (shot->y-4)*inv_factor,
                                 3, 8);
                cairo_fill (cr);
	}

	if ( ! inv_game_over) {
		GdkPixbuf *phsh;

		if (inv_phsh_state < 5) {
			phsh = inv_phsh1;
		} else {
			phsh = inv_phsh2;
		}

                gdk_cairo_set_source_pixbuf (cr, phsh,
                                             inv_our_x*inv_factor - inv_phsh_width/2,
                                             550*inv_factor - inv_phsh_height/2);
                cairo_rectangle (cr,
                                 inv_our_x*inv_factor - inv_phsh_width/2,
                                 550*inv_factor - inv_phsh_height/2,
                                 inv_phsh_width,
                                 inv_phsh_height);
                cairo_fill (cr);
	}

        cairo_destroy (cr);

	gdk_flush ();

	if (inv_do_pause) {
		g_usleep (G_USEC_PER_SEC);
		inv_do_pause = FALSE;
	}

	inv_draw_idle = 0;
	return TRUE;
}

gboolean gimp_lebl_dialog (void);

gboolean
gimp_lebl_dialog (void)
{
	GtkWidget *vbox;
	int i, j;

	if (geginv != NULL) {
		gtk_window_present (GTK_WINDOW (geginv));
		return FALSE;
	}

	inv_width = 800;
	inv_height = 600;

	if (inv_width > gdk_screen_get_width (gdk_screen_get_default ()) * 0.9) {
		inv_width = gdk_screen_get_width (gdk_screen_get_default ()) * 0.9;
		inv_height = inv_width * (600.0/800.0);
	}

	if (inv_height > gdk_screen_get_height (gdk_screen_get_default ()) * 0.9) {
		inv_height = gdk_screen_get_height (gdk_screen_get_default ()) * 0.9;
		inv_width = inv_height * (800.0/600.0);
	}

	inv_factor = (double)inv_width / 800.0;

	if ( ! ensure_creatures ())
		return FALSE;

	geginv = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position (GTK_WINDOW (geginv), GTK_WIN_POS_CENTER);
	gtk_window_set_title (GTK_WINDOW (geginv), _("Killer GEGLs from Outer Space"));
	g_object_set (G_OBJECT (geginv), "resizable", FALSE, NULL);
	g_signal_connect (G_OBJECT (geginv), "destroy",
			  G_CALLBACK (geginv_destroyed),
			  NULL);

	geginv_canvas = gtk_drawing_area_new ();
	gtk_widget_set_size_request (geginv_canvas, inv_width, inv_height);

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add (GTK_CONTAINER (geginv), vbox);
	gtk_box_pack_start (GTK_BOX (vbox), geginv_canvas, TRUE, TRUE, 0);

	geginv_label = gtk_label_new ("");
	gtk_box_pack_start (GTK_BOX (vbox), geginv_label, FALSE, FALSE, 0);

	inv_our_x = 400;
	inv_x = 70;
	inv_y = 70;
	inv_first_col = 0;
	inv_level = 0;
	inv_lives = 3;
	inv_last_col = INV_COLS-1;
	inv_reverse = FALSE;
	inv_game_over = FALSE;
	inv_left_pressed = FALSE;
	inv_right_pressed = FALSE;
	inv_fire_pressed = FALSE;
	inv_left_released = FALSE;
	inv_right_released = FALSE;
	inv_fire_released = FALSE;
	inv_paused = FALSE;

	gtk_widget_add_events (geginv, GDK_KEY_RELEASE_MASK);

	g_signal_connect (G_OBJECT (geginv), "key_press_event",
			  G_CALLBACK (inv_key_press), NULL);
	g_signal_connect (G_OBJECT (geginv), "key_release_event",
			  G_CALLBACK (inv_key_release), NULL);
	g_signal_connect (G_OBJECT (geginv_canvas), "expose_event",
			  G_CALLBACK (inv_expose), NULL);

	g_slist_foreach (inv_shots, (GFunc)g_free, NULL);
	g_slist_free (inv_shots);
	inv_shots = NULL;

	for (i = 0; i < INV_COLS; i++) {
		for (j = 0; j < INV_ROWS; j++) {
			invs[i][j].live = TRUE;
			invs[i][j].x = 70 + i * 100;
			invs[i][j].y = 70 + j * 80;
		}
	}
	inv_num = INV_ROWS * INV_COLS;

	g_timeout_add (((inv_num/4)+1) * 100, geginv_timeout, geginv);
	g_timeout_add (90, geginv_move_timeout, geginv);

	inv_show_status ();

	gtk_widget_show_all (geginv);
  return FALSE;
}

/* GdkPixbuf RGB C-Source image dump 1-byte-run-length-encoded */

#ifdef __SUNPRO_C
#pragma align 4 (goatpb)
#endif
#ifdef __GNUC__
static const guint8 goatpb[] __attribute__ ((__aligned__ (4))) = 
#else
static const guint8 goatpb[] = 
#endif
{ ""
  /* Pixbuf magic (0x47646b50) */
  "GdkP"
  /* length: header (24) + pixel_data (8825) */
  "\0\0\"\221"
  /* pixdata_type (0x2010001) */
  "\2\1\0\1"
  /* rowstride (396) */
  "\0\0\1\214"
  /* width (132) */
  "\0\0\0\204"
  /* height (104) */
  "\0\0\0h"
  /* pixel_data: */
  "\377\377\377\377\377\377\377\377\377\377\377\377\231\377\377\377\2\305"
  "\311\305\322\326\322\377\377\377\377\202\377\377\377\4\371\372\371tw"
  "tVXV\303\306\303\377\377\377\377\202\377\377\377\4\227\232\227!!!<=<"
  "\252\255\252\224\377\377\377\3\321\324\321\216\221\216\335\341\335\351"
  "\377\377\377\5\351\354\351fhf'''CDC\236\242\236\223\377\377\377\3\226"
  "\231\226\34\35\34\214\216\214\352\377\377\377\5\272\276\272IKI[][IJI"
  "\260\263\260\222\377\377\377\4\212\214\212-.-[][\337\343\337\352\377"
  "\377\377\5lolnqn\207\212\207Y[Y\277\302\277\220\377\377\377\5\365\367"
  "\365xzx\\]\\KMK\310\313\310\352\377\377\377\6\254\257\254Z\\Z\301\305"
  "\301\212\215\212`b`\315\320\315\217\377\377\377\5\337\342\337`b`\202"
  "\205\202VXV\251\254\251\352\377\377\377\6\331\335\331_a_\227\232\227"
  "\325\330\325fhf\206\211\206\217\377\377\377\5\336\342\336hjh\235\240"
  "\235jlj\220\223\220\353\377\377\377\6\216\220\216nqn\354\360\354\250"
  "\253\250VYV\336\342\336\216\377\377\377\6\336\342\336oqo\257\262\257"
  "uxusvs\374\374\374\352\377\377\377\6\305\311\305QTQ\310\314\310\336\342"
  "\336dfd\251\254\251\216\377\377\377\6\316\322\316jlj\303\307\303y{yt"
  "vt\374\374\374\352\377\377\377\7\355\360\355ege\215\217\215\353\357\353"
  "\230\233\230iki\344\350\344\215\377\377\377\5\271\274\271ege\320\323"
  "\320|~|\214\216\214\354\377\377\377\6\232\235\232`a`\343\346\343\324"
  "\330\324WYW\304\307\304\215\377\377\377\5\222\224\222qsq\325\331\325"
  "ada\243\246\243\354\377\377\377\6\320\323\320TUT\310\313\310\377\377"
  "\377prp\221\224\221\214\377\377\377\6\365\367\365`b`\222\224\222\330"
  "\333\330QRQ\261\264\261\237\377\377\377\2\320\323\320\341\345\341\313"
  "\377\377\377\7\365\367\365Y[Y\264\267\264\377\377\377\233\236\233_a_"
  "\326\332\326\213\377\377\377\6\236\240\236Y[Y\315\321\315\312\316\312"
  "WYW\307\312\307\232\377\377\377\7\272\275\272\234\236\234\322\325\322"
  "\377\377\377\336\342\336^`^\245\250\245\314\377\377\377\6fhf\264\267"
  "\264\377\377\377\307\313\307_`_\273\275\273\212\377\377\377\7\273\277"
  "\273cec\254\257\254\353\357\353\233\235\233prp\351\354\351\231\377\377"
  "\377\10\331\335\331jlj:;:\210\212\210\334\340\334\212\215\212+++\232"
  "\234\232\313\377\377\377\3\335\341\335WYW\274\300\274\202\377\377\377"
  "\2\300\303\300\337\343\337\211\377\377\377\7\244\247\244fhf\250\253\250"
  "\362\366\362\340\344\340iki\247\251\247\232\377\377\377\2\277\302\277"
  "Z\\Z\202fhf\4\214\216\214PRPMOM\236\241\236\313\377\377\377\5\266\271"
  "\266ada\334\340\334\377\377\377\355\360\355\211\377\377\377\11\356\360"
  "\356\230\233\230WYW\250\253\250\355\361\355\364\370\364\261\264\261W"
  "YW\327\333\327\232\377\377\377\10\247\252\247qtq\242\244\242*+*GIG\242"
  "\245\242rtr\236\241\236\313\377\377\377\7\272\276\272\243\246\243\356"
  "\362\356\341\344\341\202\205\202\200\203\200\272\275\272\202\377\377"
  "\377\15\337\343\337\225\227\225\305\310\305\377\377\377\335\340\335\205"
  "\210\205[][\251\253\251\354\360\354\370\374\370\343\347\343vxv}\200}"
  "\233\377\377\377\10\177\202\177z}z\307\313\307dfd\215\220\215\337\343"
  "\337{}{\232\235\232\313\377\377\377\26\334\340\334\344\350\344\254\256"
  "\254\237\242\237\256\261\256{}{TVT\266\271\266\377\377\377\340\344\340"
  "z|z]_]\235\240\235\314\320\314\213\215\213\305\311\305\363\366\363\370"
  "\374\370\356\362\356\222\225\222Y[Y\302\306\302\232\377\377\377\11\365"
  "\367\365[][\242\245\242\360\364\360\350\354\350\356\362\356\362\366\362"
  "|~|\217\222\217\312\377\377\377\26\276\301\276\204\207\204\305\311\305"
  "cec\230\233\230\365\371\365\343\347\343`a`MOM\252\255\252\357\363\357"
  "\351\355\351\240\243\240OQO\326\331\326\355\361\355\366\372\366\364\370"
  "\364\335\341\335\231\234\231NPN\242\245\242\233\377\377\377\4\306\311"
  "\306WYW\301\304\301\353\356\353\202\353\357\353\3\351\355\351\177\201"
  "\177\237\241\237\311\377\377\377\26\306\311\306jlj\214\217\214\306\312"
  "\306cec\273\276\273\370\374\370\353\357\353prp\37\37\37\231\234\231\361"
  "\365\361\370\374\370\316\321\316UWU\314\320\314\367\373\367\347\353\347"
  "\252\254\252lnlVXV\245\250\245\234\377\377\377\11\225\227\225121ikiz"
  "|zyzyy{y\211\213\211iki\224\227\224\310\377\377\377\26\334\337\334rt"
  "r\217\222\217\345\350\345\333\336\333aca\247\253\247\360\364\360\305"
  "\310\305JLJ+++\252\255\252\360\364\360\366\372\366\314\320\314UWU\275"
  "\301\275\261\264\261nonYZY\215\217\215\313\317\313\234\377\377\377\12"
  "\371\372\371rurWYWwyw\207\211\207\203\205\203\205\210\205\220\222\220"
  "kmk\225\230\225\307\377\377\377\25\322\326\322vyv\177\202\177\340\343"
  "\340\377\377\377\355\360\355mpmvyv\224\227\224]_]egehkh[][\243\246\243"
  "\274\277\274\210\213\210GIG\255\260\255vyv\207\212\207\321\324\321\236"
  "\377\377\377\3\312\315\312\\^\\\307\312\307\205\377\377\377\2qsq\236"
  "\241\236\306\377\377\377\4\324\330\324tvtsus\331\335\331\203\377\377"
  "\377\15\245\250\245XZXkmk\232\235\232\326\331\326\327\332\327\215\220"
  "\215fhfbdbTVT\77@\77\253\256\253\327\333\327\240\377\377\377\3\242\244"
  "\242[][\362\363\362\205\377\377\377\2jlj\234\240\234\305\377\377\377"
  "\4\335\341\335hjhcec\314\320\314\205\377\377\377\2\322\326\322\344\350"
  "\344\204\377\377\377\6\333\337\333\306\312\306\214\217\214666`b`\332"
  "\336\332\237\377\377\377\3\371\372\371y{y\200\203\200\206\377\377\377"
  "\2]_]\241\244\241\304\377\377\377\4\322\326\322oqocec\313\317\313\215"
  "\377\377\377\5\243\247\243VYV\215\220\215Y\\Y\254\257\254\237\377\377"
  "\377\3\326\331\326hjh\253\256\253\206\377\377\377\2dfd\261\265\261\303"
  "\377\377\377\4\351\354\351y{y[][\315\321\315\215\377\377\377\7\304\307"
  "\304OQO\215\217\215\341\345\341\201\204\201z}z\361\363\361\236\377\377"
  "\377\3\264\267\264gig\315\321\315\205\377\377\377\3\341\345\341cdc\274"
  "\277\274\303\377\377\377\3\226\231\226ehe\303\307\303\215\377\377\377"
  "\10\321\325\321kmk\217\222\217\361\363\361\377\377\377\273\276\273cf"
  "c\310\313\310\236\377\377\377\3\206\211\206\201\204\201\374\374\374\205"
  "\377\377\377\3\315\321\315YZY\313\317\313\302\377\377\377\3\260\264\260"
  "MOM\255\260\255\215\377\377\377\4\315\321\315hjhfhf\351\354\351\202\377"
  "\377\377\3\344\350\344rtr\215\217\215\235\377\377\377\3\351\354\351["
  "\\[\244\246\244\206\377\377\377\3\275\300\275OPO\320\324\320\301\377"
  "\377\377\3\322\325\322bdb\237\242\237\216\377\377\377\3{~{gig\307\313"
  "\307\204\377\377\377\3\240\243\240WYW\337\343\337\234\377\377\377\3\304"
  "\307\304OQO\304\310\304\206\377\377\377\3\273\276\273ege\334\340\334"
  "\301\377\377\377\3\226\230\226[][\374\374\374\215\377\377\377\3\231\234"
  "\231\\^\\\305\311\305\205\377\377\377\3\317\322\317RTR\262\265\262\234"
  "\377\377\377\3\235\240\235oqo\355\360\355\206\377\377\377\2\251\254\251"
  "z|z\302\377\377\377\2\205\207\205~\201~\214\377\377\377\4\374\374\374"
  "\222\225\222BCB\254\257\254\207\377\377\377\3\222\225\222jlj\335\340"
  "\335\232\377\377\377\3\351\354\351npn\242\245\242\207\377\377\377\2\213"
  "\216\213{~{\302\377\377\377\2\210\212\210\211\214\211\214\377\377\377"
  "\4\247\252\247XZXPQP\263\267\263\207\377\377\377\3\331\335\331cec\234"
  "\237\234\232\377\377\377\3\304\310\304\\^\\\320\324\320\207\377\377\377"
  "\2~\201~\211\214\211\255\377\377\377\6\324\330\324\272\275\272\245\250"
  "\245\267\272\267\300\304\300\317\323\317\217\377\377\377\2\204\206\204"
  "xzx\213\377\377\377\6\307\313\307SUS\224\227\224\213\215\213iki\337\343"
  "\337\207\377\377\377\3\241\244\241aca\272\275\272\231\377\377\377\2\233"
  "\236\233npn\210\377\377\377\2{~{\246\251\246\253\377\377\377\12\326\332"
  "\326\216\221\216XZXZ\\Zrtroqo\\^\\STS\220\223\220\355\360\355\215\377"
  "\377\377\2\220\223\220dgd\212\377\377\377\7\273\276\273VXV\202\205\202"
  "\344\350\344\332\336\332UXU\245\251\245\207\377\377\377\5\365\367\365"
  "\221\224\221^`^\261\264\261\365\367\365\226\377\377\377\3\340\344\340"
  "svs\233\236\233\207\377\377\377\3\335\341\335dgd\270\274\270\251\377"
  "\377\377\14\327\333\327\225\230\225]_]jmj\247\252\247\325\331\325\377"
  "\377\377\371\372\371\326\332\326\245\250\245UWU\241\244\241\215\377\377"
  "\377\3\274\277\274SUS\322\326\322\210\377\377\377\4\312\315\312fifux"
  "u\341\344\341\202\377\377\377\3\220\223\220npn\324\330\324\207\377\377"
  "\377\5\341\344\341\241\244\241WYW\224\226\224\344\350\344\225\377\377"
  "\377\3\305\310\305gig\303\306\303\207\377\377\377\3\322\326\322UWU\277"
  "\303\277\246\377\377\377\7\351\354\351\305\311\305\233\236\233npnmom"
  "\241\244\241\335\341\335\206\377\377\377\2\216\220\216npn\215\377\377"
  "\377\3\340\344\340[][\232\234\232\207\377\377\377\4\365\367\365rtr}\177"
  "}\330\333\330\203\377\377\377\3\324\330\324aca\235\240\235\211\377\377"
  "\377\4\243\247\243acay{y\325\330\325\224\377\377\377\3\232\235\232sv"
  "s\340\344\340\207\377\377\377\3\303\306\303Y[Y\320\324\320\207\377\377"
  "\377\3\365\367\365\351\354\351\334\340\334\204\333\337\333\2\330\334"
  "\330\315\320\315\204\312\315\312\1\320\324\320\210\333\337\333\16\316"
  "\322\316\311\314\311\274\277\274\265\271\265\252\255\252\247\252\247"
  "\244\247\244\220\223\220uwuhjh[][mpm\240\243\240\323\326\323\210\377"
  "\377\377\3\263\266\263KLK\340\343\340\215\377\377\377\3\243\246\243_"
  "a_\321\324\321\205\377\377\377\4\324\330\324\205\210\205wyw\331\335\331"
  "\205\377\377\377\3\241\244\241`b`\335\341\335\211\377\377\377\7\314\317"
  "\314qsq\\]\\\213\216\213\273\276\273\333\337\333\342\345\342\212\377"
  "\377\377\1\340\343\340\202\334\340\334\5\315\321\315\274\277\274\257"
  "\263\257_`_\234\236\234\210\377\377\377\2\253\256\253KMK\202\225\230"
  "\225\12\204\206\204z}z\177\201\177\201\203\201\200\203\200susmomegeZ"
  "\\Z\\_\\\203cfc\1gjg\204lnl\4jljdfdmpmege\205cfc\14egebdbegebcb]^]Z\\"
  "Zdfdmpm\204\206\204\212\215\212\242\245\242\307\313\307\213\377\377\377"
  "\3\313\317\313WYW\326\332\326\215\377\377\377\4\365\367\365knksus\277"
  "\302\277\203\377\377\377\4\274\277\274^_^ege\330\334\330\206\377\377"
  "\377\3\332\336\332aba\257\262\257\212\377\377\377\10\336\342\336\241"
  "\244\241knk`a`dfdnpn\206\211\206\224\227\224\202\224\226\224\203\221"
  "\224\221\13\223\226\223\222\225\222\204\206\204hkhfhfnpnjmj`b`PRPKLK"
  "\307\313\307\210\377\377\377\16\226\231\226CDCorovyvvxvuxu\203\206\203"
  "\212\214\212\224\227\224\245\250\245\254\257\254\253\256\253\262\265"
  "\262\270\273\270\203\277\303\277\2\306\312\306\324\330\324\203\327\332"
  "\327\4\325\330\325\311\314\311\312\315\312\301\304\301\205\277\303\277"
  "\3\301\304\301\307\313\307\320\323\320\202\327\332\327\3\331\335\331"
  "\336\342\336\344\350\344\217\377\377\377\3\322\326\322Z\\Z\313\317\313"
  "\216\377\377\377\11\301\305\301prpXZXz|z\245\250\245\223\227\223OQO8"
  "98cec\210\377\377\377\3\220\222\220kmk\341\344\341\213\377\377\377\6"
  "\335\341\335\311\314\311\241\244\241\231\233\231\227\232\227\201\204"
  "\201\202\177\201\177\203svs\2|~|\207\212\207\202\231\234\231\6\244\247"
  "\244\261\264\261\303\307\303\303\306\303psp{}{\211\377\377\377\4\202"
  "\205\202}\200}\337\343\337\361\363\361\262\377\377\377\3\272\276\272"
  "RTR\333\337\333\220\377\377\377\10\311\314\311\207\212\207cecRTRrtr\245"
  "\250\245NPN\331\335\331\207\377\377\377\3\331\334\331SUS\250\253\250"
  "\235\377\377\377\3\304\310\304UXU\274\277\274\210\377\377\377\3\335\341"
  "\335psp\245\250\245\263\377\377\377\3\270\273\270kmk~\201~\223\377\377"
  "\377\6\326\332\326{~{}\177}\303\307\303RTR\305\311\305\210\377\377\377"
  "\3wzwuxu\365\367\365\234\377\377\377\2\226\231\226lnl\211\377\377\377"
  "\3\316\321\316hjh\300\303\300\261\377\377\377\5\324\327\324\220\222\220"
  "[][\201\204\201\326\331\326\223\377\377\377\6\332\336\332^`^\264\267"
  "\264\336\341\336XZX\236\241\236\210\377\377\377\3\247\252\247Y[Y\317"
  "\323\317\233\377\377\377\3\331\334\331jlj\232\234\232\211\377\377\377"
  "\3\262\265\262iki\323\327\323\257\377\377\377\6\365\367\365\233\236\233"
  "`b`knk\270\273\270\355\360\355\224\377\377\377\6\240\243\240[][\345\350"
  "\345\354\360\354sus\222\225\222\210\377\377\377\3\324\327\324]^]\237"
  "\242\237\233\377\377\377\3\232\235\232dfd\323\326\323\211\377\377\377"
  "\3\213\215\213sus\365\367\365\256\377\377\377\5\314\320\314tvtbdb\241"
  "\244\241\340\344\340\225\377\377\377\7\371\372\371vxv\214\216\214\366"
  "\372\366\364\370\364\221\224\221\202\205\202\211\377\377\377\2\202\205"
  "\202uxu\232\377\377\377\3\351\354\351aca\225\230\225\212\377\377\377"
  "\2uwu\207\212\207\256\377\377\377\4\361\363\361|\177|hjh\330\333\330"
  "\227\377\377\377\7\330\334\330aca\256\260\256\370\374\370\364\370\364"
  "\221\225\221y{y\211\377\377\377\3\265\270\265VYV\335\341\335\231\377"
  "\377\377\3\254\257\254Y\\Y\316\322\316\211\377\377\377\3\365\367\365"
  "]_]\245\250\245\256\377\377\377\3\306\311\306`b`\266\272\266\230\377"
  "\377\377\7\264\267\264_a_\324\327\324\370\374\370\341\345\341lnl\237"
  "\242\237\211\377\377\377\3\324\330\324WYW\274\300\274\230\377\377\377"
  "\3\327\333\327aba\236\241\236\212\377\377\377\3\303\307\303VXV\316\321"
  "\316\256\377\377\377\3\240\243\240Z[Z\334\337\334\230\377\377\377\7\223"
  "\227\223prp\344\347\344\357\363\357\242\246\242UWU\325\330\325\212\377"
  "\377\377\2nqn\227\232\227\230\377\377\377\3\241\244\241[][\333\337\333"
  "\212\377\377\377\3\240\243\240iki\344\350\344\256\377\377\377\3\244\247"
  "\244\\^\\\334\337\334\230\377\377\377\6twt\205\207\205\354\360\354\276"
  "\302\276UXU\212\215\212\213\377\377\377\2\227\232\227\204\206\204\227"
  "\377\377\377\3\333\336\333kmk\215\220\215\213\377\377\377\2~\201~\203"
  "\206\203\257\377\377\377\3\270\274\270hjh\330\334\330\230\377\377\377"
  "\6[][\233\236\233\277\303\277`b`\223\226\223\374\374\374\213\377\377"
  "\377\3\247\252\247iki\355\360\355\226\377\377\377\3\234\237\234ded\322"
  "\325\322\212\377\377\377\3\337\342\337dfd\244\247\244\257\377\377\377"
  "\3\320\323\320fhf\303\307\303\227\377\377\377\6\332\335\332JLJ\202\205"
  "\202fif\223\225\223\355\360\355\214\377\377\377\3\247\252\247efe\344"
  "\350\344\225\377\377\377\3\355\360\355cec\223\226\223\213\377\377\377"
  "\3\307\313\307UWU\310\313\310\257\377\377\377\3\323\327\323Z\\Z\261\265"
  "\261\227\377\377\377\5\301\305\3019:9CEC\203\205\203\371\372\371\215"
  "\377\377\377\3\247\252\247hkh\355\360\355\225\377\377\377\3\302\305\302"
  "^`^\315\321\315\213\377\377\377\3\247\252\247[][\361\363\361\257\377"
  "\377\377\3\337\342\337oqo\254\257\254\227\377\377\377\1\340\343\340\202"
  "\260\263\260\1\371\372\371\216\377\377\377\2\221\223\221\202\205\202"
  "\226\377\377\377\2\340\344\340\303\306\303\214\377\377\377\2\235\240"
  "\235\217\222\217\261\377\377\377\2\201\203\201\202\204\202\251\377\377"
  "\377\2tvt\217\222\217\244\377\377\377\2\325\331\325\334\340\334\261\377"
  "\377\377\2\234\240\234ege\250\377\377\377\3\365\367\365knk\253\256\253"
  "\327\377\377\377\3\306\312\306XYX\321\325\321\247\377\377\377\3\315\320"
  "\315TVT\315\321\315\327\377\377\377\3\374\374\374prp\231\234\231\247"
  "\377\377\377\3\300\303\300WXW\333\337\333\330\377\377\377\3\230\233\230"
  "dfd\351\354\351\246\377\377\377\2\246\251\246bdb\331\377\377\377\3\310"
  "\313\310RTR\305\311\305\246\377\377\377\2\212\214\212rur\332\377\377"
  "\377\2gjg\223\225\223\245\377\377\377\3\361\363\361qsq\214\216\214\332"
  "\377\377\377\3\214\216\214mom\344\350\344\244\377\377\377\3\335\341\335"
  "fif\247\252\247\332\377\377\377\3\303\306\303`b`\305\311\305\244\377"
  "\377\377\3\322\326\322jlj\300\304\300\306\377\377\377\2\374\374\374\341"
  "\344\341\222\377\377\377\3\335\341\335ege\223\226\223\244\377\377\377"
  "\3\306\312\306bdb\302\306\302\305\377\377\377\3\340\344\340\215\220\215"
  "qtq\212\377\377\377\1\314\320\314\210\377\377\377\2\230\233\230hjh\244"
  "\377\377\377\3\227\233\227ehe\326\332\326\303\377\377\377\6\351\354\351"
  "\277\303\277wyw676787\341\345\341\210\377\377\377\3\257\262\257XZX\260"
  "\262\260\207\377\377\377\3\305\310\305OQO\305\310\305\242\377\377\377"
  "\3\337\343\337Y[Y\237\242\237\302\377\377\377\10\337\343\337\243\246"
  "\243gjg]^]\177\202\177xzxJKJ\351\354\351\210\377\377\377\4`b`ABASUS\316"
  "\321\316\206\377\377\377\3\365\367\365ili\207\211\207\242\377\377\377"
  "\3\252\255\252`a`\330\334\330\300\377\377\377\11\312\315\312\241\243"
  "\241npn_b_\220\222\220\315\320\315\377\377\377\237\242\237fif\210\377"
  "\377\377\6\303\307\303RSR\247\252\247orovxv\344\350\344\206\377\377\377"
  "\3\242\246\242WYW\332\336\332\240\377\377\377\3\361\363\361prp\222\225"
  "\222\277\377\377\377\6\311\314\311\222\225\222[][jlj\216\221\216\316"
  "\322\316\203\377\377\377\2\211\214\211z|z\210\377\377\377\6\213\216\213"
  "y{y\371\372\371\310\314\310Z\\Z\244\247\244\206\377\377\377\3\331\335"
  "\331WYW\267\272\267\240\377\377\377\3\277\302\277VXV\321\325\321\226"
  "\377\377\377\1\374\374\374\203\341\345\341\2\344\350\344\365\367\365"
  "\237\377\377\377\10\334\340\334\305\311\305\232\235\232iki]_]mom\260"
  "\264\260\340\344\340\204\377\377\377\3\361\363\361wyw\221\225\221\207"
  "\377\377\377\3\317\323\317XZX\270\273\270\202\377\377\377\3\245\250\245"
  "_`_\305\310\305\206\377\377\377\2|\177|\210\212\210\240\377\377\377\2"
  "\217\222\217hjh\211\377\377\377\3\337\342\337\260\264\260\331\334\331"
  "\210\377\377\377\4\300\303\300\267\273\267\231\233\231\202\204\202\203"
  "svs\12vxv{}{\200\202\200\204\207\204\202\205\202}\200}\217\222\217\235"
  "\240\235\256\261\256\266\271\266\202\265\270\265\3\266\271\266\277\303"
  "\277\304\307\304\211\310\313\310\1\304\307\304\202\301\304\301\14\267"
  "\272\267\265\270\265\266\271\266\243\247\243\214\217\214\201\204\201"
  "dfd^`^EFEcdc\301\304\301\365\367\365\206\377\377\377\3\331\335\331ik"
  "i\253\256\253\207\377\377\377\3\241\244\241cec\374\374\374\202\377\377"
  "\377\1\336\342\336\202prp\206\377\377\377\3\254\257\254bdb\322\325\322"
  "\236\377\377\377\3\345\350\345iki\210\213\210\211\377\377\377\3\235\240"
  "\235;=;\237\242\237\207\377\377\377\5\325\330\325gig\213\216\213|\177"
  "|\236\241\236\204\253\256\253\16\244\247\244\222\224\222\210\212\210"
  "\202\204\202suswyw|~|x{xikicec\\^\\PRP[][`b`\205ege\5dfdPQPMNMaca`b`"
  "\202]_]\13XZX`b`hjhknkuwu\203\206\203\236\242\236\300\304\300\236\241"
  "\236svs\355\360\355\207\377\377\377\3\316\322\316Y[Y\265\270\265\206"
  "\377\377\377\3\336\342\336nqn\214\217\214\204\377\377\377\3\275\300\275"
  "PRP\255\261\255\205\377\377\377\3\321\325\321Y[Y\245\250\245\236\377"
  "\377\377\3\316\321\316cfc\273\276\273\210\377\377\377\4\341\345\341Y"
  "ZY:;:\203\206\203\207\377\377\377\4\310\313\310[^[\317\322\317\374\374"
  "\374\214\377\377\377\6\374\374\374\336\342\336\333\336\333\330\333\330"
  "\320\323\320\320\324\320\206\321\324\321\4\313\317\313bdbvxv\310\313"
  "\310\203\321\324\321\4\324\327\324\331\335\331\335\341\335\351\354\351"
  "\204\377\377\377\3\310\313\310TVT\315\320\315\207\377\377\377\3\312\316"
  "\312cfc\304\307\304\206\377\377\377\3\256\261\256_a_\312\316\312\205"
  "\377\377\377\3\233\235\233`b`\332\336\332\205\377\377\377\2\206\211\206"
  "twt\236\377\377\377\3\255\260\255gig\331\335\331\210\377\377\377\4\226"
  "\231\226kmk\177\201\177bdb\207\377\377\377\3\310\313\310MOM\313\316\313"
  "\232\377\377\377\2aba\232\235\232\214\377\377\377\3\351\354\351cec\271"
  "\274\271\207\377\377\377\3\271\275\271lol\326\332\326\206\377\377\377"
  "\2|~|\207\211\207\206\377\377\377\3\336\342\336Z\\Z\244\247\244\205\377"
  "\377\377\3\274\277\274XZX\334\340\334\235\377\377\377\3\210\213\210y"
  "{y\362\363\362\207\377\377\377\6\321\325\321]_]\260\263\260\273\276\273"
  "STS\325\331\325\206\377\377\377\3\310\313\310DFD\304\307\304\231\377"
  "\377\377\3\374\374\374[][\245\250\245\215\377\377\377\2gig\232\235\232"
  "\207\377\377\377\3\242\245\242fhf\336\342\336\205\377\377\377\3\330\333"
  "\330WYW\265\270\265\207\377\377\377\3\213\216\213y|y\365\367\365\204"
  "\377\377\377\3\335\341\335`b`\261\264\261\234\377\377\377\3\374\374\374"
  "^_^\247\252\247\210\377\377\377\6\223\226\223bcb\341\345\341\332\335"
  "\332^`^\264\270\264\206\377\377\377\3\312\316\312IJI\304\307\304\231"
  "\377\377\377\3\317\323\317QRQ\275\300\275\215\377\377\377\2{}{\205\207"
  "\205\207\377\377\377\3\230\233\230lnl\344\350\344\205\377\377\377\3\241"
  "\243\241jkj\342\345\342\207\377\377\377\3\276\301\276[][\272\275\272"
  "\205\377\377\377\3\221\224\221svs\374\374\374\233\377\377\377\3\320\323"
  "\320RTR\305\311\305\207\377\377\377\3\314\320\314bdb\243\246\243\202"
  "\377\377\377\2x{x\207\212\207\206\377\377\377\3\323\326\323XZX\304\307"
  "\304\231\377\377\377\3\275\300\275LML\317\322\317\215\377\377\377\3\225"
  "\231\225hkh\340\344\340\206\377\377\377\2\226\231\226\200\203\200\205"
  "\377\377\377\3\335\340\335]_]\232\235\232\210\377\377\377\3\371\372\371"
  "{~{sus\205\377\377\377\3\307\313\307JLJ\306\311\306\233\377\377\377\3"
  "\253\257\253]_]\336\342\336\207\377\377\377\3\225\230\225jlj\326\331"
  "\326\202\377\377\377\3\240\244\240cec\341\345\341\205\377\377\377\3\327"
  "\333\327`a`\304\307\304\231\377\377\377\3\273\276\273^`^\331\334\331"
  "\215\377\377\377\3\262\264\262hjh\325\331\325\206\377\377\377\202\202"
  "\204\202\205\377\377\377\3\274\277\274RTR\316\322\316\211\377\377\377"
  "\3\257\263\257KMK\330\333\330\205\377\377\377\2_`_\246\251\246\233\377"
  "\377\377\2~\200~\212\215\212\207\377\377\377\3\337\343\337XZX\236\241"
  "\236\203\377\377\377\3\272\276\272UVU\324\327\324\205\377\377\377\3\336"
  "\342\336gig\267\272\267\231\377\377\377\3\251\254\251ege\336\342\336"
  "\215\377\377\377\3\307\312\307aba\303\306\303\206\377\377\377\2y{y\220"
  "\223\220\205\377\377\377\2\217\221\217vxv\212\377\377\377\3\334\340\334"
  "bdb\231\234\231\205\377\377\377\2\212\214\212|~|\232\377\377\377\3\324"
  "\330\324WYW\276\301\276\207\377\377\377\3\247\252\247cec\334\337\334"
  "\203\377\377\377\3\337\343\337WXW\265\270\265\205\377\377\377\3\351\354"
  "\351mom\254\260\254\231\377\377\377\3\247\252\247prp\361\363\361\215"
  "\377\377\377\3\321\325\321cec\270\274\270\206\377\377\377\2cfc\221\224"
  "\221\204\377\377\377\3\334\337\334mom\245\251\245\213\377\377\377\3\253"
  "\256\253aca\335\341\335\204\377\377\377\3\256\261\256_a_\327\333\327"
  "\231\377\377\377\3\271\275\271TVT\334\337\334\206\377\377\377\3\334\337"
  "\334cec\230\233\230\205\377\377\377\2kmk\225\230\225\206\377\377\377"
  "\2vxv\246\251\246\231\377\377\377\3\266\271\266_a_\331\335\331\215\377"
  "\377\377\3\327\333\327npn\272\275\272\206\377\377\377\2dfd\234\237\234"
  "\204\377\377\377\3\274\277\274dfd\315\320\315\213\377\377\377\3\333\337"
  "\333UWU\257\262\257\204\377\377\377\3\302\306\302UWU\273\276\273\231"
  "\377\377\377\2\221\224\221gjg\207\377\377\377\3\242\245\242XZX\334\337"
  "\334\205\377\377\377\3\211\214\211vxv\365\367\365\205\377\377\377\2q"
  "sq\225\230\225\231\377\377\377\3\325\330\325RTR\260\263\260\215\377\377"
  "\377\3\317\323\317cec\271\274\271\206\377\377\377\2gig\242\245\242\204"
  "\377\377\377\3\230\233\230knk\341\345\341\214\377\377\377\11\204\207"
  "\204\177\201\177\374\374\374\377\377\377\345\350\345\274\300\274gig."
  "..\227\232\227\230\377\377\377\3\365\367\365vxv\216\221\216\206\377\377"
  "\377\3\330\334\330fif\223\226\223\206\377\377\377\3\263\266\263cec\322"
  "\325\322\205\377\377\377\2kmk\202\204\202\232\377\377\377\3x{xz}z\335"
  "\340\335\214\377\377\377\3\252\255\252\\^\\\312\315\312\206\377\377\377"
  "\2kmk\271\275\271\204\377\377\377\2rur\215\220\215\215\377\377\377\11"
  "\271\275\271\\^\\\253\256\253\227\232\227iki\\^\\~\200~hihnqn\230\377"
  "\377\377\3\334\340\334hjh\257\262\257\206\377\377\377\3\242\245\242`"
  "b`\316\321\316\206\377\377\377\3\316\321\316bdb\270\273\270\205\377\377"
  "\377\2z}z\204\207\204\232\377\377\377\3\303\306\303^`^\212\214\212\214"
  "\377\377\377\2prp\205\207\205\206\377\377\377\3\336\342\336aca\304\310"
  "\304\204\377\377\377\2gig\254\257\254\215\377\377\377\12\337\343\337"
  "ijiBDBbdb\231\233\231\312\315\312\350\354\350\246\251\246SUS\365\367"
  "\365\227\377\377\377\3\303\307\303fhf\317\322\317\205\377\377\377\3\371"
  "\372\371aca\221\224\221\207\377\377\377\3\344\350\344npn\225\230\225"
  "\205\377\377\377\2\214\216\214\207\211\207\233\377\377\377\4\257\262"
  "\257Y[Y\254\257\254\374\374\374\211\377\377\377\3\231\234\231aca\314"
  "\320\314\206\377\377\377\3\326\332\326VXV\303\307\303\203\377\377\377"
  "\3\321\325\321PRP\304\307\304\216\377\377\377\4\230\233\230DFD\313\316"
  "\313\361\365\361\202\367\373\367\3\313\317\313WZW\327\332\327\227\377"
  "\377\377\3\246\252\246hkh\334\340\334\205\377\377\377\3\262\265\262Y"
  "[Y\322\325\322\210\377\377\377\2\214\217\214nqn\205\377\377\377\2\221"
  "\224\221~\201~\233\377\377\377\6\333\337\333^`^:;:\206\211\206\301\304"
  "\301\336\342\336\205\377\377\377\4\324\330\324\213\216\213686}\200}\207"
  "\377\377\377\11\322\326\322HJH\223\226\223\264\267\264\272\276\272\306"
  "\311\306\223\226\223RTR\324\330\324\216\377\377\377\11\324\327\324Z\\"
  "Z\301\304\301\357\363\357\326\332\326\355\361\355\342\346\342`b`\257"
  "\262\257\227\377\377\377\2\216\221\216|\177|\206\377\377\377\2z|z\212"
  "\214\212\211\377\377\377\2\240\243\240^`^\205\377\377\377\3\247\252\247"
  "gig\341\345\341\232\377\377\377\20\265\270\265OPO\215\217\215kmkWYWm"
  "om\222\225\222\245\250\245\247\252\247\240\243\240\212\215\212]_]QSQ"
  "oroiki\335\341\335\206\377\377\377\11\312\315\312020WXWegehjhmom<=<^"
  "`^\351\354\351\217\377\377\377\10|~|\211\214\211\267\273\267]`]\223\226"
  "\223\312\315\312uxu\215\220\215\227\377\377\377\2wyw\202\205\202\205"
  "\377\377\377\3\310\313\310VWV\305\311\305\211\377\377\377\3\303\307\303"
  "QSQ\317\323\317\204\377\377\377\3\261\263\261OQO\326\332\326\232\377"
  "\377\377\20\226\231\226lol\261\264\261\202\204\202\262\265\262\240\243"
  "\240\201\204\201egefifqtq\202\204\202pspsvs\263\266\263Y[Y\313\316\313"
  "\206\377\377\377\3\310\313\310PSP\270\274\270\202\327\332\327\3\322\325"
  "\322tvty|y\220\377\377\377\10\262\265\262XZXsusSUSdgdWZWFHFnpn\227\377"
  "\377\377\2svs\234\237\234\205\377\377\377\2\230\233\230npn\212\377\377"
  "\377\3\336\342\336cec\253\256\253\204\377\377\377\3\270\273\270WYW\325"
  "\330\325\232\377\377\377\5{}{\200\201\200\225\230\225vyv\356\360\356"
  "\206\377\377\377\5\250\253\250oqo\321\325\321]_]\241\244\241\206\377"
  "\377\377\10\310\313\310_b_\324\330\324\360\364\360\364\370\364\350\354"
  "\350sus\232\235\232\220\377\377\377\11\341\345\341HJH131\205\207\205"
  "\341\345\341\241\244\241IJItvt\374\374\374\226\377\377\377\2fhf\257\262"
  "\257\204\377\377\377\3\355\360\355qsq\221\223\221\213\377\377\377\11"
  "\177\201\177\203\206\203\371\372\371\333\337\333\256\261\256\215\217"
  "\215\237\242\237\\]\\\306\311\306\232\377\377\377\4\246\251\246XZX[]"
  "[\222\225\222\207\377\377\377\5\250\254\250hkh\310\313\310VXV\220\223"
  "\220\206\377\377\377\10\272\275\272^a^\264\267\264\226\230\226\311\314"
  "\311\331\335\331VXV\257\262\257\221\377\377\377\3kmk\40!\40\245\250\245"
  "\202\377\377\377\2\323\327\323\330\333\330\227\377\377\377\3^_^\221\223"
  "\221\334\340\334\203\377\377\377\3\317\322\317_a_\263\265\263\213\377"
  "\377\377\11\251\254\251@A@qtq`a`_a_\207\211\207\275\300\275PRP\250\253"
  "\250\232\377\377\377\4\337\343\337\205\210\205WYW\271\274\271\207\377"
  "\377\377\5\303\306\303WYWabaACA\236\241\236\206\377\377\377\10\251\254"
  "\251Z\\Ziki565rur\274\300\274aba\321\325\321\221\377\377\377\3\235\240"
  "\2359:9\264\267\264\233\377\377\377\11OQO454TVT\200\203\200\256\261\256"
  "\314\320\314\253\256\253dgd\324\330\324\213\377\377\377\11\321\324\321"
  ":;:jlj\263\266\263\343\347\343\365\371\365\355\361\355fhf\234\237\234"
  "\234\377\377\377\1\365\367\365\211\377\377\377\4\303\306\303\251\254"
  "\251\264\267\264\345\350\345\206\377\377\377\10\243\246\243#$#KLK\233"
  "\236\233RTRfhfKMK\341\345\341\221\377\377\377\2\333\337\333\277\302\277"
  "\234\377\377\377\11]_]\211\213\211\237\241\237prpacaWYWOQOprp\355\360"
  "\355\213\377\377\377\4\355\360\355VWV\251\254\251\364\370\364\202\367"
  "\373\367\3\365\371\365wywy|y\260\377\377\377\10\221\223\221\31\32\31"
  "\215\220\215\361\363\361\211\215\211565MOM\374\374\374\257\377\377\377"
  "\10cec\262\265\262\362\366\362\347\353\347\327\332\327\263\266\263IK"
  "I~\200~\215\377\377\377\11rtr\220\222\220\345\351\345\263\266\263\263"
  "\267\263\347\353\347\227\233\227oro\340\344\340\257\377\377\377\7\254"
  "\257\254dfd\322\326\322\377\377\377\322\325\322PRPege\260\377\377\377"
  "\10fhf\257\262\257\357\363\357\345\351\345\364\370\364\355\361\355gi"
  "g\251\255\251\215\377\377\377\11\222\224\222prp\271\274\271\77@\77DE"
  "D~\200~\230\232\230XZX\312\315\312\264\377\377\377\2\245\250\245\247"
  "\252\247\260\377\377\377\10iki\241\244\241\271\274\271rsr\312\315\312"
  "\340\344\340WYW\267\273\267\215\377\377\377\11\266\271\266]_]vxvVXV\247"
  "\252\247lnlBDBGHG\301\305\301\346\377\377\377\10wzw\213\216\213jlj**"
  "*\201\203\201\277\303\277WYW\312\315\312\215\377\377\377\11\323\326\323"
  "YZY454\231\234\231\377\377\377\324\327\324jmj:<:\250\253\250\346\377"
  "\377\377\10uwuJKJKMK\225\230\225[\\[xzxZ\\Z\335\340\335\215\377\377\377"
  "\4\362\363\362gjg121\324\327\324\203\377\377\377\2\306\311\306\341\345"
  "\341\346\377\377\377\10jmj\36\37\36}\177}\336\342\336vxv010ded\355\360"
  "\355\216\377\377\377\2\246\252\246\177\202\177\354\377\377\377\7jkj7"
  "97\301\304\301\377\377\377\262\266\262,-,\177\201\177\220\377\377\377"
  "\1\365\367\365\354\377\377\377\2\277\303\277\257\262\257\203\377\377"
  "\377\2\272\275\272\321\324\321\377\377\377\377\377\377\377\377\365\377"
  "\377\377"};


/* GdkPixbuf RGBA C-Source image dump 1-byte-run-length-encoded */

#ifdef __SUNPRO_C
#pragma align 4 (goatpb2)
#endif
#ifdef __GNUC__
static const guint8 goatpb2[] __attribute__ ((__aligned__ (4))) = 
#else
static const guint8 goatpb2[] = 
#endif
{ ""
  /* Pixbuf magic (0x47646b50) */
  "GdkP"
  /* length: header (24) + pixel_data (12435) */
  "\0\0""0\253"
  /* pixdata_type (0x2010002) */
  "\2\1\0\2"
  /* rowstride (528) */
  "\0\0\2\20"
  /* width (132) */
  "\0\0\0\204"
  /* height (104) */
  "\0\0\0h"
  /* pixel_data: */
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\231\377"
  "\377\377\377\2\305\311\305\377\322\326\322\377\377\377\377\377\377\202"
  "\377\377\377\377\4\371\372\371\377twt\377VXV\377\303\306\303\377\377"
  "\377\377\377\377\202\377\377\377\377\4\227\232\227\377!!!\377<=<\377"
  "\252\255\252\377\224\377\377\377\377\3\321\324\321\377\216\221\216\377"
  "\335\341\335\377\351\377\377\377\377\5\351\354\351\377fhf\377'''\377"
  "CDC\377\236\242\236\377\223\377\377\377\377\3\226\231\226\377\34\35\34"
  "\377\214\216\214\377\352\377\377\377\377\5\272\276\272\377IKI\377[]["
  "\377IJI\377\260\263\260\377\222\377\377\377\377\4\212\214\212\377-.-"
  "\377[][\377\337\343\337\377\352\377\377\377\377\5lol\377nqn\377\207\212"
  "\207\377Y[Y\377\277\302\277\377\220\377\377\377\377\5\365\367\365\377"
  "xzx\377\\]\\\377KMK\377\310\313\310\377\352\377\377\377\377\6\254\257"
  "\254\377Z\\Z\377\301\305\301\377\212\215\212\377`b`\377\315\320\315\377"
  "\217\377\377\377\377\5\337\342\337\377`b`\377\202\205\202\377VXV\377"
  "\251\254\251\377\352\377\377\377\377\6\331\335\331\377_a_\377\227\232"
  "\227\377\325\330\325\377fhf\377\206\211\206\377\217\377\377\377\377\5"
  "\336\342\336\377hjh\377\235\240\235\377jlj\377\220\223\220\377\353\377"
  "\377\377\377\6\216\220\216\377nqn\377\354\360\354\377\250\253\250\377"
  "VYV\377\336\342\336\377\216\377\377\377\377\6\336\342\336\377oqo\377"
  "\257\262\257\377uxu\377svs\377\374\374\374\377\352\377\377\377\377\6"
  "\305\311\305\377QTQ\377\310\314\310\377\336\342\336\377dfd\377\251\254"
  "\251\377\216\377\377\377\377\6\316\322\316\377jlj\377\303\307\303\377"
  "y{y\377tvt\377\374\374\374\377\352\377\377\377\377\7\355\360\355\377"
  "ege\377\215\217\215\377\353\357\353\377\230\233\230\377iki\377\344\350"
  "\344\377\215\377\377\377\377\5\271\274\271\377ege\377\320\323\320\377"
  "|~|\377\214\216\214\377\354\377\377\377\377\6\232\235\232\377`a`\377"
  "\343\346\343\377\324\330\324\377WYW\377\304\307\304\377\215\377\377\377"
  "\377\5\222\224\222\377qsq\377\325\331\325\377ada\377\243\246\243\377"
  "\354\377\377\377\377\6\320\323\320\377TUT\377\310\313\310\377\377\377"
  "\377\377prp\377\221\224\221\377\214\377\377\377\377\6\365\367\365\377"
  "`b`\377\222\224\222\377\330\333\330\377QRQ\377\261\264\261\377\354\377"
  "\377\377\377\7\365\367\365\377Y[Y\377\264\267\264\377\377\377\377\377"
  "\233\236\233\377_a_\377\326\332\326\377\213\377\377\377\377\6\236\240"
  "\236\377Y[Y\377\315\321\315\377\312\316\312\377WYW\377\307\312\307\377"
  "\355\377\377\377\377\6fhf\377\264\267\264\377\377\377\377\377\307\313"
  "\307\377_`_\377\273\275\273\377\212\377\377\377\377\7\273\277\273\377"
  "cec\377\254\257\254\377\353\357\353\377\233\235\233\377prp\377\351\354"
  "\351\377\354\377\377\377\377\3\335\341\335\377WYW\377\274\300\274\377"
  "\202\377\377\377\377\2\300\303\300\377\337\343\337\377\211\377\377\377"
  "\377\7\244\247\244\377fhf\377\250\253\250\377\362\366\362\377\340\344"
  "\340\377iki\377\247\251\247\377\223\377\377\377\377\3\371\372\371\377"
  "\355\357\355\377\376\376\376\377\327\377\377\377\377\5\266\271\266\377"
  "ada\377\334\340\334\377\377\377\377\377\355\360\355\377\211\377\377\377"
  "\377\11\356\360\356\377\230\233\230\377WYW\377\250\253\250\377\355\361"
  "\355\377\364\370\364\377\261\264\261\377WYW\377\327\333\327\377\223\377"
  "\377\377\377\3\335\337\335\377\256\261\256\377\307\311\307\377\327\377"
  "\377\377\377\7\272\276\272\377\243\246\243\377\356\362\356\377\341\344"
  "\341\377\202\205\202\377\200\203\200\377\272\275\272\377\202\377\377"
  "\377\377\15\337\343\337\377\225\227\225\377\305\310\305\377\377\377\377"
  "\377\335\340\335\377\205\210\205\377[][\377\251\253\251\377\354\360\354"
  "\377\370\374\370\377\343\347\343\377vxv\377}\200}\377\223\377\377\377"
  "\377\5\375\375\375\377\322\325\322\377MOM\377\205\207\205\377\330\331"
  "\330\377\326\377\377\377\377\26\334\340\334\377\344\350\344\377\254\256"
  "\254\377\237\242\237\377\256\261\256\377{}{\377TVT\377\266\271\266\377"
  "\377\377\377\377\340\344\340\377z|z\377]_]\377\235\240\235\377\314\320"
  "\314\377\213\215\213\377\305\311\305\377\363\366\363\377\370\374\370"
  "\377\356\362\356\377\222\225\222\377Y[Y\377\302\306\302\377\222\377\377"
  "\377\377\7\356\357\356\377\366\366\366\377\310\313\310\377aca\377WXW"
  "\377\237\242\237\377\356\356\356\377\324\377\377\377\377\26\276\301\276"
  "\377\204\207\204\377\305\311\305\377cec\377\230\233\230\377\365\371\365"
  "\377\343\347\343\377`a`\377MOM\377\252\255\252\377\357\363\357\377\351"
  "\355\351\377\240\243\240\377OQO\377\326\331\326\377\355\361\355\377\366"
  "\372\366\377\364\370\364\377\335\341\335\377\231\234\231\377NPN\377\242"
  "\245\242\377\222\377\377\377\377\10\314\315\314\377\241\243\241\377\247"
  "\252\247\377\277\302\277\377^`^\377lnl\377\205\210\205\377\262\264\262"
  "\377\323\377\377\377\377\26\306\311\306\377jlj\377\214\217\214\377\306"
  "\312\306\377cec\377\273\276\273\377\370\374\370\377\353\357\353\377p"
  "rp\377\37\37\37\377\231\234\231\377\361\365\361\377\370\374\370\377\316"
  "\321\316\377UWU\377\314\320\314\377\367\373\367\377\347\353\347\377\252"
  "\254\252\377lnl\377VXV\377\245\250\245\377\222\377\377\377\377\12\365"
  "\365\365\377\233\235\233\377EFE\377gig\377kmk\377cfc\377\266\271\266"
  "\377\213\215\213\377\213\216\213\377\302\304\302\377\321\377\377\377"
  "\377\26\334\337\334\377rtr\377\217\222\217\377\345\350\345\377\333\336"
  "\333\377aca\377\247\253\247\377\360\364\360\377\305\310\305\377JLJ\377"
  "+++\377\252\255\252\377\360\364\360\377\366\372\366\377\314\320\314\377"
  "UWU\377\275\301\275\377\261\264\261\377non\377YZY\377\215\217\215\377"
  "\313\317\313\377\223\377\377\377\377\13\361\363\361\377\250\253\250\377"
  "]_]\377}\177}\377@B@\377bdb\377\273\276\273\377\322\325\322\377~\200"
  "~\377\231\233\231\377\334\335\334\377\317\377\377\377\377\25\322\326"
  "\322\377vyv\377\177\202\177\377\340\343\340\377\377\377\377\377\355\360"
  "\355\377mpm\377vyv\377\224\227\224\377]_]\377ege\377hkh\377[][\377\243"
  "\246\243\377\274\277\274\377\210\213\210\377GIG\377\255\260\255\377v"
  "yv\377\207\212\207\377\321\324\321\377\226\377\377\377\377\13\313\315"
  "\313\377\204\206\204\377\212\215\212\377\247\252\247\377\227\231\227"
  "\377\352\356\352\377\355\361\355\377\257\262\257\377y{y\377\234\237\234"
  "\377\362\362\362\377\315\377\377\377\377\4\324\330\324\377tvt\377sus"
  "\377\331\335\331\377\203\377\377\377\377\15\245\250\245\377XZX\377km"
  "k\377\232\235\232\377\326\331\326\377\327\332\327\377\215\220\215\377"
  "fhf\377bdb\377TVT\377\77@\77\377\253\256\253\377\327\333\327\377\230"
  "\377\377\377\377\13\355\355\355\377\226\231\226\377{~{\377\303\307\303"
  "\377\354\360\354\377\353\357\353\377\302\305\302\377\213\215\213\377"
  "uwu\377\200\203\200\377\270\272\270\377\314\377\377\377\377\4\335\341"
  "\335\377hjh\377cec\377\314\320\314\377\205\377\377\377\377\2\322\326"
  "\322\377\344\350\344\377\204\377\377\377\377\6\333\337\333\377\306\312"
  "\306\377\214\217\214\377666\377`b`\377\332\336\332\377\231\377\377\377"
  "\377\13\303\304\303\377prp\377\261\264\261\377\345\350\345\377\251\253"
  "\251\377{}{\377\205\210\205\377\261\263\261\377\200\202\200\377\216\221"
  "\216\377\316\320\316\377\312\377\377\377\377\4\322\326\322\377oqo\377"
  "cec\377\313\317\313\377\215\377\377\377\377\5\243\247\243\377VYV\377"
  "\215\220\215\377Y\\Y\377\254\257\254\377\231\377\377\377\377\14\366\367"
  "\366\377\205\207\205\377{}{\377\201\203\201\377\177\201\177\377\203\205"
  "\203\377\312\313\312\377\377\377\377\377\323\324\323\377knk\377\233\236"
  "\233\377\347\350\347\377\310\377\377\377\377\4\351\354\351\377y{y\377"
  "[][\377\315\321\315\377\215\377\377\377\377\7\304\307\304\377OQO\377"
  "\215\217\215\377\341\345\341\377\201\204\201\377z}z\377\361\363\361\377"
  "\230\377\377\377\377\6\363\363\363\377\253\256\253\377\77@\77\377dfd"
  "\377\230\231\230\377\345\345\345\377\203\377\377\377\377\4\255\256\255"
  "\377z}z\377\276\301\276\377\372\373\372\377\307\377\377\377\377\3\226"
  "\231\226\377ehe\377\303\307\303\377\215\377\377\377\377\10\321\325\321"
  "\377kmk\377\217\222\217\377\361\363\361\377\377\377\377\377\273\276\273"
  "\377cfc\377\310\313\310\377\231\377\377\377\377\4\311\312\311\377mpm"
  "\377\210\213\210\377\371\371\371\377\205\377\377\377\377\3\211\213\211"
  "\377\223\225\223\377\332\335\332\377\306\377\377\377\377\3\260\264\260"
  "\377MOM\377\255\260\255\377\215\377\377\377\377\4\315\321\315\377hjh"
  "\377fhf\377\351\354\351\377\202\377\377\377\377\3\344\350\344\377rtr"
  "\377\215\217\215\377\231\377\377\377\377\4\370\371\370\377\212\214\212"
  "\377\225\227\225\377\360\361\360\377\204\377\377\377\377\5\367\370\367"
  "\377\311\315\311\377`a`\377\257\262\257\377\353\355\353\377\304\377\377"
  "\377\377\3\322\325\322\377bdb\377\237\242\237\377\216\377\377\377\377"
  "\3{~{\377gig\377\307\313\307\377\204\377\377\377\377\3\240\243\240\377"
  "WYW\377\337\343\337\377\230\377\377\377\377\4\362\363\362\377\260\263"
  "\260\377cec\377\322\323\322\377\205\377\377\377\377\5\342\344\342\377"
  "\232\234\232\377klk\377\327\333\327\377\373\374\373\377\303\377\377\377"
  "\377\3\226\230\226\377[][\377\374\374\374\377\215\377\377\377\377\3\231"
  "\234\231\377\\^\\\377\305\311\305\377\205\377\377\377\377\3\317\322\317"
  "\377RTR\377\262\265\262\377\231\377\377\377\377\4\313\314\313\377~\200"
  "~\377\237\241\237\377\370\371\370\377\205\377\377\377\377\3\314\316\314"
  "\377\213\215\213\377\245\247\245\377\304\377\377\377\377\2\205\207\205"
  "\377~\201~\377\214\377\377\377\377\4\374\374\374\377\222\225\222\377"
  "BCB\377\254\257\254\377\207\377\377\377\377\3\222\225\222\377jlj\377"
  "\335\340\335\377\230\377\377\377\377\4\370\371\370\377\217\221\217\377"
  "\217\222\217\377\331\334\331\377\205\377\377\377\377\4\362\363\362\377"
  "\254\257\254\377\201\204\201\377\312\313\312\377\303\377\377\377\377"
  "\2\210\212\210\377\211\214\211\377\214\377\377\377\377\4\247\252\247"
  "\377XZX\377PQP\377\263\267\263\377\207\377\377\377\377\3\331\335\331"
  "\377cec\377\234\237\234\377\230\377\377\377\377\5\364\364\364\377\273"
  "\276\273\377mom\377\314\316\314\377\376\376\376\377\205\377\377\377\377"
  "\4\325\326\325\377\205\210\205\377\210\213\210\377\364\364\364\377\255"
  "\377\377\377\377\6\324\330\324\377\272\275\272\377\245\250\245\377\267"
  "\272\267\377\300\304\300\377\317\323\317\377\217\377\377\377\377\2\204"
  "\206\204\377xzx\377\213\377\377\377\377\6\307\313\307\377SUS\377\224"
  "\227\224\377\213\215\213\377iki\377\337\343\337\377\207\377\377\377\377"
  "\3\241\244\241\377aca\377\272\275\272\377\230\377\377\377\377\4\321\322"
  "\321\377\210\213\210\377\240\243\240\377\373\373\373\377\206\377\377"
  "\377\377\3\275\277\275\377|\177|\377\246\251\246\377\253\377\377\377"
  "\377\12\326\332\326\377\216\221\216\377XZX\377Z\\Z\377rtr\377oqo\377"
  "\\^\\\377STS\377\220\223\220\377\355\360\355\377\215\377\377\377\377"
  "\2\220\223\220\377dgd\377\212\377\377\377\377\7\273\276\273\377VXV\377"
  "\202\205\202\377\344\350\344\377\332\336\332\377UXU\377\245\251\245\377"
  "\207\377\377\377\377\5\365\367\365\377\221\224\221\377^`^\377\261\264"
  "\261\377\365\367\365\377\226\377\377\377\377\3\340\344\340\377svs\377"
  "\233\236\233\377\207\377\377\377\377\3\336\342\336\377dgd\377\270\274"
  "\270\377\251\377\377\377\377\14\327\333\327\377\225\230\225\377]_]\377"
  "jmj\377\247\252\247\377\325\331\325\377\377\377\377\377\371\372\371\377"
  "\326\332\326\377\245\250\245\377UWU\377\241\244\241\377\215\377\377\377"
  "\377\3\274\277\274\377SUS\377\322\326\322\377\210\377\377\377\377\4\312"
  "\315\312\377fif\377uxu\377\341\344\341\377\202\377\377\377\377\3\220"
  "\223\220\377npn\377\324\330\324\377\207\377\377\377\377\5\341\344\341"
  "\377\241\244\241\377WYW\377\224\226\224\377\344\350\344\377\225\377\377"
  "\377\377\3\305\310\305\377gig\377\303\306\303\377\207\377\377\377\377"
  "\3\322\326\322\377UWU\377\277\303\277\377\246\377\377\377\377\7\351\354"
  "\351\377\305\311\305\377\233\236\233\377npn\377mom\377\241\244\241\377"
  "\335\341\335\377\206\377\377\377\377\2\216\220\216\377npn\377\215\377"
  "\377\377\377\3\340\344\340\377[][\377\232\234\232\377\207\377\377\377"
  "\377\4\365\367\365\377rtr\377}\177}\377\330\333\330\377\203\377\377\377"
  "\377\3\324\330\324\377aca\377\235\240\235\377\211\377\377\377\377\4\243"
  "\247\243\377aca\377y{y\377\325\330\325\377\224\377\377\377\377\3\232"
  "\235\232\377svs\377\340\344\340\377\207\377\377\377\377\3\303\306\303"
  "\377Y[Y\377\320\324\320\377\207\377\377\377\377\3\365\367\365\377\351"
  "\354\351\377\334\340\334\377\204\333\337\333\377\2\330\334\330\377\315"
  "\320\315\377\204\312\315\312\377\1\320\324\320\377\210\333\337\333\377"
  "\16\316\322\316\377\311\314\311\377\274\277\274\377\265\271\265\377\252"
  "\255\252\377\247\252\247\377\244\247\244\377\220\223\220\377uwu\377h"
  "jh\377[][\377mpm\377\240\243\240\377\323\326\323\377\210\377\377\377"
  "\377\3\263\266\263\377KLK\377\340\343\340\377\215\377\377\377\377\3\243"
  "\246\243\377_a_\377\321\324\321\377\205\377\377\377\377\4\324\330\324"
  "\377\205\210\205\377wyw\377\331\335\331\377\205\377\377\377\377\3\241"
  "\244\241\377`b`\377\335\341\335\377\211\377\377\377\377\7\314\317\314"
  "\377qsq\377\\]\\\377\213\216\213\377\273\276\273\377\333\337\333\377"
  "\342\345\342\377\212\377\377\377\377\1\340\343\340\377\202\334\340\334"
  "\377\5\315\321\315\377\274\277\274\377\257\263\257\377_`_\377\234\236"
  "\234\377\210\377\377\377\377\2\253\256\253\377KMK\377\202\225\230\225"
  "\377\12\204\206\204\377z}z\377\177\201\177\377\201\203\201\377\200\203"
  "\200\377sus\377mom\377ege\377Z\\Z\377\\_\\\377\203cfc\377\1gjg\377\204"
  "lnl\377\4jlj\377dfd\377mpm\377ege\377\205cfc\377\14ege\377bdb\377ege"
  "\377bcb\377]^]\377Z\\Z\377dfd\377mpm\377\204\206\204\377\212\215\212"
  "\377\242\245\242\377\307\313\307\377\213\377\377\377\377\3\313\317\313"
  "\377WYW\377\326\332\326\377\215\377\377\377\377\4\365\367\365\377knk"
  "\377sus\377\277\302\277\377\203\377\377\377\377\4\274\277\274\377^_^"
  "\377\227\231\227\377\341\344\341\377\206\377\377\377\377\3\332\336\332"
  "\377aba\377\257\262\257\377\212\377\377\377\377\10\336\342\336\377\241"
  "\244\241\377knk\377`a`\377dfd\377npn\377\206\211\206\377\224\227\224"
  "\377\202\224\226\224\377\203\221\224\221\377\13\223\226\223\377\222\225"
  "\222\377\204\206\204\377hkh\377fhf\377npn\377jmj\377`b`\377PRP\377KL"
  "K\377\307\313\307\377\210\377\377\377\377\16\226\231\226\377CDC\377o"
  "ro\377vyv\377vxv\377uxu\377\203\206\203\377\212\214\212\377\224\227\224"
  "\377\245\250\245\377\254\257\254\377\253\256\253\377\262\265\262\377"
  "\270\273\270\377\203\277\303\277\377\2\306\312\306\377\324\330\324\377"
  "\203\327\332\327\377\4\325\330\325\377\311\314\311\377\312\315\312\377"
  "\301\304\301\377\205\277\303\277\377\3\301\304\301\377\307\313\307\377"
  "\320\323\320\377\202\327\332\327\377\3\331\335\331\377\336\342\336\377"
  "\344\350\344\377\217\377\377\377\377\3\322\326\322\377Z\\Z\377\313\317"
  "\313\377\216\377\377\377\377\12\301\305\301\377prp\377XZX\377z|z\377"
  "\245\250\245\377\223\227\223\377IKI\377qsq\377\221\223\221\377\342\345"
  "\342\377\207\377\377\377\377\3\220\222\220\377kmk\377\341\344\341\377"
  "\213\377\377\377\377\6\335\341\335\377\311\314\311\377\241\244\241\377"
  "\231\233\231\377\227\232\227\377\201\204\201\377\202\177\201\177\377"
  "\203svs\377\2|~|\377\207\212\207\377\202\231\234\231\377\6\244\247\244"
  "\377\261\264\261\377\303\307\303\377\303\306\303\377psp\377{}{\377\211"
  "\377\377\377\377\4\202\205\202\377}\200}\377\337\343\337\377\361\363"
  "\361\377\262\377\377\377\377\3\272\276\272\377RTR\377\333\337\333\377"
  "\220\377\377\377\377\11\311\314\311\377\207\212\207\377cec\377\274\275"
  "\274\377\251\252\251\377\245\250\245\377Z\\Z\377\257\263\257\377\351"
  "\353\351\377\206\377\377\377\377\3\331\334\331\377SUS\377\250\253\250"
  "\377\235\377\377\377\377\3\304\310\304\377UXU\377\274\277\274\377\210"
  "\377\377\377\377\3\335\341\335\377psp\377\245\250\245\377\263\377\377"
  "\377\377\3\270\273\270\377kmk\377~\201~\377\223\377\377\377\377\7\361"
  "\363\361\377\246\247\246\377\214\215\214\377\244\247\244\377\225\230"
  "\225\377svs\377\265\267\265\377\207\377\377\377\377\3wzw\377uxu\377\365"
  "\367\365\377\234\377\377\377\377\2\226\231\226\377lnl\377\211\377\377"
  "\377\377\3\316\321\316\377hjh\377\300\303\300\377\261\377\377\377\377"
  "\5\324\327\324\377\220\222\220\377[][\377\201\204\201\377\326\331\326"
  "\377\223\377\377\377\377\10\371\371\371\377\303\306\303\377xzx\377\257"
  "\261\257\377\337\342\337\377uwu\377\213\216\213\377\335\336\335\377\206"
  "\377\377\377\377\3\247\252\247\377Y[Y\377\317\323\317\377\233\377\377"
  "\377\377\3\331\334\331\377jlj\377\232\234\232\377\211\377\377\377\377"
  "\3\262\265\262\377iki\377\323\327\323\377\257\377\377\377\377\6\365\367"
  "\365\377\233\236\233\377`b`\377knk\377\270\273\270\377\355\360\355\377"
  "\225\377\377\377\377\7\340\342\340\377uxu\377\257\262\257\377\353\356"
  "\353\377\275\300\275\377\213\216\213\377\241\243\241\377\206\377\377"
  "\377\377\3\324\327\324\377]^]\377\237\242\237\377\233\377\377\377\377"
  "\3\232\235\232\377dfd\377\323\326\323\377\211\377\377\377\377\3\213\215"
  "\213\377sus\377\365\367\365\377\256\377\377\377\377\5\314\320\314\377"
  "tvt\377bdb\377\241\244\241\377\340\344\340\377\227\377\377\377\377\10"
  "\334\336\334\377\207\211\207\377\225\227\225\377\366\372\366\377\360"
  "\364\360\377\227\233\227\377\206\210\206\377\342\343\342\377\206\377"
  "\377\377\377\2\202\205\202\377uxu\377\232\377\377\377\377\3\351\354\351"
  "\377aca\377\225\230\225\377\212\377\377\377\377\2uwu\377\207\212\207"
  "\377\256\377\377\377\377\4\361\363\361\377|\177|\377hjh\377\330\333\330"
  "\377\231\377\377\377\377\10\373\373\373\377\202\204\202\377\223\225\223"
  "\377\341\344\341\377\361\365\361\377\254\260\254\377\202\204\202\377"
  "\325\327\325\377\206\377\377\377\377\3\265\270\265\377VYV\377\335\341"
  "\335\377\231\377\377\377\377\3\254\257\254\377Y\\Y\377\316\322\316\377"
  "\211\377\377\377\377\3\365\367\365\377]_]\377\245\250\245\377\256\377"
  "\377\377\377\3\306\311\306\377`b`\377\266\272\266\377\232\377\377\377"
  "\377\10\362\364\362\377\242\245\242\377\202\204\202\377\330\334\330\377"
  "\360\364\360\377\260\263\260\377`b`\377\334\335\334\377\206\377\377\377"
  "\377\3\324\330\324\377WYW\377\274\300\274\377\230\377\377\377\377\3\327"
  "\333\327\377aba\377\236\241\236\377\212\377\377\377\377\3\303\307\303"
  "\377VXV\377\316\321\316\377\256\377\377\377\377\3\240\243\240\377Z[Z"
  "\377\334\337\334\377\232\377\377\377\377\10\370\371\370\377\267\273\267"
  "\377ili\377\313\315\313\377\346\352\346\377\230\233\230\377twt\377\333"
  "\334\333\377\207\377\377\377\377\2nqn\377\227\232\227\377\230\377\377"
  "\377\377\3\241\244\241\377[][\377\333\337\333\377\212\377\377\377\377"
  "\3\240\243\240\377iki\377\344\350\344\377\256\377\377\377\377\3\244\247"
  "\244\377\\^\\\377\334\337\334\377\233\377\377\377\377\7\305\307\305\377"
  "\201\204\201\377\246\251\246\377\333\337\333\377\212\215\212\377\227"
  "\232\227\377\373\373\373\377\207\377\377\377\377\2\227\232\227\377\204"
  "\206\204\377\227\377\377\377\377\3\333\336\333\377kmk\377\215\220\215"
  "\377\213\377\377\377\377\2~\201~\377\203\206\203\377\257\377\377\377"
  "\377\3\270\274\270\377hjh\377\330\334\330\377\233\377\377\377\377\7\355"
  "\355\355\377svs\377\216\220\216\377\272\276\272\377\177\201\177\377\300"
  "\303\300\377\371\372\371\377\207\377\377\377\377\3\247\252\247\377ik"
  "i\377\355\360\355\377\226\377\377\377\377\3\234\237\234\377ded\377\322"
  "\325\322\377\212\377\377\377\377\3\337\342\337\377dfd\377\244\247\244"
  "\377\257\377\377\377\377\3\320\323\320\377fhf\377\303\307\303\377\234"
  "\377\377\377\377\5\235\236\235\377rur\377~\201~\377\202\205\202\377\344"
  "\345\344\377\210\377\377\377\377\3\247\252\247\377efe\377\344\350\344"
  "\377\225\377\377\377\377\3\355\360\355\377cec\377\223\226\223\377\213"
  "\377\377\377\377\3\307\313\307\377UWU\377\310\313\310\377\257\377\377"
  "\377\377\3\323\327\323\377Z\\Z\377\261\265\261\377\233\377\377\377\377"
  "\6\373\373\373\377\307\311\307\377GIG\377QTQ\377\221\223\221\377\361"
  "\362\361\377\210\377\377\377\377\3\247\252\247\377hkh\377\355\360\355"
  "\377\225\377\377\377\377\3\302\305\302\377^`^\377\315\321\315\377\213"
  "\377\377\377\377\3\247\252\247\377[][\377\361\363\361\377\257\377\377"
  "\377\377\3\337\342\337\377oqo\377\254\257\254\377\234\377\377\377\377"
  "\5\326\331\326\377\177\202\177\377\211\213\211\377\323\325\323\377\375"
  "\375\375\377\210\377\377\377\377\2\221\223\221\377\202\205\202\377\226"
  "\377\377\377\377\2\340\344\340\377\303\306\303\377\214\377\377\377\377"
  "\2\235\240\235\377\217\222\217\377\261\377\377\377\377\2\201\203\201"
  "\377\202\204\202\377\234\377\377\377\377\3\363\364\363\377\334\337\334"
  "\377\334\335\334\377\212\377\377\377\377\2tvt\377\217\222\217\377\244"
  "\377\377\377\377\2\325\331\325\377\334\340\334\377\261\377\377\377\377"
  "\2\234\240\234\377ege\377\250\377\377\377\377\3\365\367\365\377knk\377"
  "\253\256\253\377\327\377\377\377\377\3\306\312\306\377XYX\377\321\325"
  "\321\377\247\377\377\377\377\3\315\320\315\377TVT\377\315\321\315\377"
  "\327\377\377\377\377\3\374\374\374\377prp\377\231\234\231\377\247\377"
  "\377\377\377\3\300\303\300\377WXW\377\333\337\333\377\330\377\377\377"
  "\377\3\230\233\230\377dfd\377\351\354\351\377\246\377\377\377\377\2\246"
  "\251\246\377bdb\377\331\377\377\377\377\3\310\313\310\377RTR\377\305"
  "\311\305\377\246\377\377\377\377\2\212\214\212\377rur\377\331\377\377"
  "\377\377\3\340\340\340\377kmk\377\217\221\217\377\245\377\377\377\377"
  "\3\361\363\361\377qsq\377\214\216\214\377\331\377\377\377\377\4\316\317"
  "\316\377\210\212\210\377\251\253\251\377\350\352\350\377\244\377\377"
  "\377\377\3\335\341\335\377fif\377\247\252\247\377\306\377\377\377\377"
  "\3\376\376\376\377\370\370\370\377\374\374\374\377\220\377\377\377\377"
  "\4\346\347\346\377\233\236\233\377\201\204\201\377\306\307\306\377\244"
  "\377\377\377\377\3\357\361\357\377z|z\377\300\304\300\377\305\377\377"
  "\377\377\4\370\371\370\377\304\306\304\377\261\263\261\377\360\360\360"
  "\377\220\377\377\377\377\5\370\371\370\377\316\320\316\377\204\207\204"
  "\377\224\226\224\377\371\372\371\377\242\377\377\377\377\5\333\334\333"
  "\377\251\254\251\377tvt\377\245\250\245\377\375\375\375\377\304\377\377"
  "\377\377\5\234\236\234\377^`^\377VXV\377\345\346\345\377\376\376\376"
  "\377\210\377\377\377\377\1\373\374\373\377\207\377\377\377\377\4\365"
  "\365\365\377\256\261\256\377]_]\377\303\306\303\377\240\377\377\377\377"
  "\6\365\366\365\377\332\334\332\377\213\216\213\377iki\377\255\261\255"
  "\377\333\335\333\377\303\377\377\377\377\6\351\354\351\377\207\211\207"
  "\377|~|\377]^]\377EFE\377\344\350\344\377\210\377\377\377\377\3\347\350"
  "\347\377\307\312\307\377\353\354\353\377\207\377\377\377\377\5\352\354"
  "\352\377sus\377\200\202\200\377\346\347\346\377\375\375\375\377\235\377"
  "\377\377\377\6\375\375\375\377\324\327\324\377\207\212\207\377z}z\377"
  "\254\256\254\377\372\372\372\377\302\377\377\377\377\10\337\343\337\377"
  "\243\246\243\377gjg\377\310\311\310\377\317\320\317\377\207\211\207\377"
  "oqo\377\363\364\363\377\207\377\377\377\377\6\366\366\366\377\241\243"
  "\241\377fhf\377\221\223\221\377\335\336\335\377\372\373\372\377\205\377"
  "\377\377\377\5\375\375\375\377\266\271\266\377npn\377\254\257\254\377"
  "\356\357\356\377\235\377\377\377\377\5\310\312\310\377\223\226\223\377"
  "}\177}\377\267\271\267\377\371\371\371\377\222\377\377\377\377\1\374"
  "\374\374\377\256\377\377\377\377\11\312\315\312\377\241\243\241\377n"
  "pn\377_b_\377\265\266\265\377\371\371\371\377\337\340\337\377\211\214"
  "\211\377\226\230\226\377\207\377\377\377\377\10\371\371\371\377\313\315"
  "\313\377{}{\377zzz\377Y[Y\377\222\224\222\377\322\325\322\377\370\371"
  "\370\377\205\377\377\377\377\4\347\351\347\377\226\231\226\377\207\211"
  "\207\377\321\322\321\377\233\377\377\377\377\6\370\371\370\377\326\326"
  "\326\377\177\202\177\377uwu\377\305\310\305\377\355\356\355\377\222\377"
  "\377\377\377\4\357\357\357\377\317\321\317\377\334\335\334\377\376\376"
  "\376\377\252\377\377\377\377\13\311\314\311\377\222\225\222\377[][\377"
  "jlj\377\216\221\216\377\316\322\316\377\377\377\377\377\372\373\372\377"
  "\304\306\304\377\202\205\202\377\264\266\264\377\207\377\377\377\377"
  "\10\342\343\342\377\225\230\225\377\217\220\217\377\321\323\321\377\234"
  "\237\234\377\177\201\177\377\221\223\221\377\326\330\326\377\205\377"
  "\377\377\377\5\375\375\375\377\322\323\322\377\205\207\205\377\235\236"
  "\235\377\354\355\354\377\231\377\377\377\377\6\367\367\367\377\315\320"
  "\315\377rtr\377\216\220\216\377\304\305\304\377\366\367\366\377\222\377"
  "\377\377\377\7\375\375\375\377\333\335\333\377hkh\377\266\271\266\377"
  "\363\363\363\377\376\376\376\377\374\374\374\377\203\341\345\341\377"
  "\2\344\350\344\377\365\367\365\377\237\377\377\377\377\10\334\340\334"
  "\377\305\311\305\377\232\235\232\377iki\377]_]\377mom\377\260\264\260"
  "\377\340\344\340\377\203\377\377\377\377\4\355\357\355\377\244\247\244"
  "\377\210\213\210\377\317\321\317\377\206\377\377\377\377\12\362\363\362"
  "\377\262\265\262\377\203\205\203\377\274\275\274\377\374\375\374\377"
  "\355\357\355\377\275\277\275\377\202\205\202\377\214\216\214\377\327"
  "\331\327\377\205\377\377\377\377\4\362\362\362\377\250\253\250\377km"
  "k\377\300\303\300\377\230\377\377\377\377\5\375\375\375\377\271\273\271"
  "\377\205\210\205\377\200\203\200\377\315\320\315\377\213\377\377\377"
  "\377\1\375\375\375\377\211\377\377\377\377\6\336\340\336\377\212\215"
  "\212\377\216\221\216\377\336\340\336\377\340\340\340\377\202\204\202"
  "\377\203svs\377\12vxv\377{}{\377\200\202\200\377\204\207\204\377\202"
  "\205\202\377}\200}\377\217\222\217\377\235\240\235\377\256\261\256\377"
  "\266\271\266\377\202\265\270\265\377\3\266\271\266\377\277\303\277\377"
  "\304\307\304\377\211\310\313\310\377\1\304\307\304\377\202\301\304\301"
  "\377\14\267\272\267\377\265\270\265\377\266\271\266\377\243\247\243\377"
  "\214\217\214\377\201\204\201\377dfd\377^`^\377EFE\377cdc\377\301\304"
  "\301\377\365\367\365\377\205\377\377\377\377\4\340\343\340\377\207\212"
  "\207\377\223\226\223\377\342\343\342\377\206\377\377\377\377\4\323\325"
  "\323\377\204\207\204\377\236\240\236\377\346\347\346\377\202\377\377"
  "\377\377\5\366\366\366\377\271\275\271\377npn\377\203\204\203\377\367"
  "\370\367\377\205\377\377\377\377\4\333\337\333\377qsq\377\210\213\210"
  "\377\364\364\364\377\226\377\377\377\377\6\343\345\343\377\255\260\255"
  "\377x{x\377\206\211\206\377\330\333\330\377\364\365\364\377\211\377\377"
  "\377\377\6\375\376\375\377\377\377\377\377\330\332\330\377\377\377\377"
  "\377\364\365\364\377\373\373\373\377\206\377\377\377\377\7\362\362\362"
  "\377\265\270\265\377RUR\377\303\306\303\377\371\371\371\377\317\320\317"
  "\377\272\274\272\377\203\253\256\253\377\16\244\247\244\377\222\224\222"
  "\377\210\212\210\377\202\204\202\377sus\377wyw\377|~|\377x{x\377iki\377"
  "cec\377\\^\\\377PRP\377[][\377`b`\377\205ege\377\5dfd\377PQP\377MNM\377"
  "aca\377`b`\377\202]_]\377\13XZX\377`b`\377hjh\377knk\377uwu\377\203\206"
  "\203\377\236\242\236\377\300\304\300\377\236\241\236\377svs\377\355\360"
  "\355\377\206\377\377\377\377\4\327\332\327\377y|y\377\246\251\246\377"
  "\356\357\356\377\205\377\377\377\377\5\356\360\356\377\242\245\242\377"
  "|\177|\377\321\322\321\377\376\376\376\377\203\377\377\377\377\6\361"
  "\362\361\377\256\260\256\377]_]\377\235\241\235\377\351\352\351\377\374"
  "\375\374\377\204\377\377\377\377\4\262\264\262\377uwu\377\301\304\301"
  "\377\366\367\366\377\223\377\377\377\377\6\376\376\376\377\353\353\353"
  "\377\252\256\252\377aca\377\246\251\246\377\334\335\334\377\213\377\377"
  "\377\377\6\337\342\337\377\251\253\251\377\273\275\273\377\330\332\330"
  "\377\265\271\265\377\327\330\327\377\207\377\377\377\377\4\326\330\326"
  "\377rtr\377\217\221\217\377\341\342\341\377\214\377\377\377\377\6\374"
  "\374\374\377\336\342\336\377\333\336\333\377\330\333\330\377\320\323"
  "\320\377\320\324\320\377\206\321\324\321\377\4\313\317\313\377bdb\377"
  "vxv\377\310\313\310\377\203\321\324\321\377\4\324\327\324\377\331\335"
  "\331\377\335\341\335\377\351\354\351\377\204\377\377\377\377\3\310\313"
  "\310\377TVT\377\315\320\315\377\206\377\377\377\377\4\304\307\304\377"
  "svs\377\302\305\302\377\370\371\370\377\205\377\377\377\377\4\310\312"
  "\310\377vxv\377\235\240\235\377\355\356\355\377\205\377\377\377\377\5"
  "\360\361\360\377\257\260\257\377sus\377\252\255\252\377\353\354\353\377"
  "\204\377\377\377\377\4\337\341\337\377\216\220\216\377\222\225\222\377"
  "\334\336\334\377\222\377\377\377\377\6\376\376\376\377\362\364\362\377"
  "\237\241\237\377z}z\377\234\236\234\377\347\351\347\377\212\377\377\377"
  "\377\11\342\344\342\377\271\273\271\377\213\215\213\377mnm\377\360\360"
  "\360\377\334\335\334\377GIG\377\225\230\225\377\371\371\371\377\206\377"
  "\377\377\377\5\355\356\355\377\253\257\253\377ege\377\311\313\311\377"
  "\376\376\376\377\230\377\377\377\377\2aba\377\232\235\232\377\214\377"
  "\377\377\377\3\351\354\351\377cec\377\271\274\271\377\205\377\377\377"
  "\377\4\375\375\375\377\246\251\246\377iki\377\332\336\332\377\206\377"
  "\377\377\377\4\221\223\221\377xzx\377\333\335\333\377\374\375\374\377"
  "\206\377\377\377\377\5\351\353\351\377\234\237\234\377\200\202\200\377"
  "\305\307\305\377\374\374\374\377\203\377\377\377\377\5\366\367\366\377"
  "\302\304\302\377\203\206\203\377\247\251\247\377\373\373\373\377\221"
  "\377\377\377\377\6\342\344\342\377\251\255\251\377|~|\377\232\235\232"
  "\377\372\372\372\377\376\376\376\377\210\377\377\377\377\6\352\353\352"
  "\377\314\316\314\377\220\223\220\377fhf\377\231\233\231\377\316\317\316"
  "\377\202\377\377\377\377\4sss\377jlj\377\271\272\271\377\371\372\371"
  "\377\206\377\377\377\377\4\332\335\332\377y{y\377\243\245\243\377\345"
  "\346\345\377\227\377\377\377\377\3\374\374\374\377[][\377\245\250\245"
  "\377\215\377\377\377\377\2gig\377\232\235\232\377\205\377\377\377\377"
  "\4\361\361\361\377\224\227\224\377y{y\377\345\350\345\377\204\377\377"
  "\377\377\4\374\374\374\377\325\330\325\377gig\377\247\252\247\377\210"
  "\377\377\377\377\5\375\375\375\377\331\332\331\377\213\216\213\377\217"
  "\221\217\377\337\341\337\377\204\377\377\377\377\4\357\357\357\377\245"
  "\250\245\377dfd\377\315\320\315\377\220\377\377\377\377\5\346\347\346"
  "\377\255\261\255\377iki\377\252\255\252\377\341\342\341\377\210\377\377"
  "\377\377\7\367\370\367\377\347\351\347\377\242\245\242\377xzx\377\206"
  "\211\206\377\252\254\252\377\341\343\341\377\203\377\377\377\377\5\326"
  "\326\326\377\207\212\207\377iji\377\322\326\322\377\374\375\374\377\205"
  "\377\377\377\377\4\356\357\356\377\260\264\260\377|~|\377\301\304\301"
  "\377\227\377\377\377\377\3\317\323\317\377QRQ\377\275\300\275\377\215"
  "\377\377\377\377\2{}{\377\205\207\205\377\205\377\377\377\377\4\344\344"
  "\344\377\220\223\220\377\230\232\230\377\365\367\365\377\204\377\377"
  "\377\377\4\355\355\355\377\236\240\236\377y{y\377\327\332\327\377\211"
  "\377\377\377\377\4\365\366\365\377\274\276\274\377npn\377\235\237\235"
  "\377\205\377\377\377\377\4\354\356\354\377_`_\377\242\245\242\377\372"
  "\372\372\377\217\377\377\377\377\4\246\251\246\377~\202~\377\241\244"
  "\241\377\341\343\341\377\207\377\377\377\377\11\374\374\374\377\363\364"
  "\363\377\302\305\302\377\207\212\207\377{}{\377\225\227\225\377\316\320"
  "\316\377\377\377\377\377\375\375\375\377\204\377\377\377\377\4\316\320"
  "\316\377{}{\377\223\227\223\377\324\325\324\377\205\377\377\377\377\5"
  "\376\376\376\377\343\347\343\377}\200}\377\235\240\235\377\343\344\343"
  "\377\226\377\377\377\377\3\275\300\275\377LML\377\317\322\317\377\215"
  "\377\377\377\377\3\225\231\225\377hkh\377\340\344\340\377\204\377\377"
  "\377\377\3\320\321\320\377\205\207\205\377\260\261\260\377\204\377\377"
  "\377\377\5\363\364\363\377\267\272\267\377wyw\377\254\256\254\377\366"
  "\367\366\377\212\377\377\377\377\5\372\373\372\377\225\231\225\377bd"
  "b\377\324\326\324\377\373\373\373\377\204\377\377\377\377\4\234\235\234"
  "\377\177\201\177\377\331\332\331\377\373\374\373\377\215\377\377\377"
  "\377\5\271\272\271\377\177\202\177\377\227\231\227\377\361\362\361\377"
  "\373\374\373\377\206\377\377\377\377\10\367\370\367\377\316\320\316\377"
  "\234\236\234\377~\200~\377|\177|\377\302\305\302\377\361\361\361\377"
  "\371\371\371\377\206\377\377\377\377\6\362\363\362\377\306\310\306\377"
  "{~{\377\212\215\212\377\350\353\350\377\376\376\376\377\204\377\377\377"
  "\377\4\366\367\366\377\274\276\274\377\207\211\207\377\270\272\270\377"
  "\226\377\377\377\377\3\273\276\273\377^`^\377\331\334\331\377\215\377"
  "\377\377\377\3\262\264\262\377hjh\377\325\331\325\377\204\377\377\377"
  "\377\3\274\275\274\377\204\206\204\377\304\305\304\377\204\377\377\377"
  "\377\4\340\341\340\377\217\221\217\377\205\210\205\377\331\333\331\377"
  "\214\377\377\377\377\5\317\322\317\377uxu\377\224\227\224\377\342\343"
  "\342\377\376\376\376\377\203\377\377\377\377\4\311\313\311\377\177\201"
  "\177\377\241\244\241\377\351\353\351\377\214\377\377\377\377\4\340\341"
  "\340\377uxu\377\214\217\214\377\336\337\336\377\206\377\377\377\377\10"
  "\376\376\376\377\333\335\333\377\256\262\256\377\177\201\177\377\\^\\"
  "\377\255\260\255\377\336\337\336\377\366\367\366\377\212\377\377\377"
  "\377\4\256\261\256\377rur\377\253\256\253\377\352\354\352\377\205\377"
  "\377\377\377\4\372\372\372\377xzx\377\211\214\211\377\344\344\344\377"
  "\225\377\377\377\377\3\251\254\251\377ege\377\336\342\336\377\215\377"
  "\377\377\377\3\307\312\307\377aba\377\303\306\303\377\204\377\377\377"
  "\377\3\240\241\240\377\201\204\201\377\324\325\324\377\203\377\377\377"
  "\377\5\374\374\374\377\276\277\276\377\201\203\201\377\271\272\271\377"
  "\370\370\370\377\214\377\377\377\377\5\362\363\362\377\266\270\266\377"
  "\202\205\202\377\246\251\246\377\354\356\354\377\203\377\377\377\377"
  "\4\343\345\343\377\225\230\225\377wyw\377\315\317\315\377\214\377\377"
  "\377\377\3\212\214\212\377\220\223\220\377\312\314\312\377\206\377\377"
  "\377\377\7\357\360\357\377\334\335\334\377\202\204\202\377jlj\377\227"
  "\231\227\377\311\312\311\377\361\362\361\377\214\377\377\377\377\4\340"
  "\342\340\377\237\242\237\377pqp\377\277\302\277\377\206\377\377\377\377"
  "\3\257\260\257\377xzx\377\261\262\261\377\225\377\377\377\377\3\247\252"
  "\247\377prp\377\361\363\361\377\215\377\377\377\377\3\321\325\321\377"
  "cec\377\270\274\270\377\204\377\377\377\377\3\214\215\214\377\213\216"
  "\213\377\343\344\343\377\203\377\377\377\377\4\343\345\343\377\217\221"
  "\217\377\221\224\221\377\337\340\337\377\216\377\377\377\377\13\351\352"
  "\351\377\244\247\244\377vxv\377\307\311\307\377\377\377\377\377\374\374"
  "\374\377\351\353\351\377\267\272\267\377]^]\377JJJ\377\240\243\240\377"
  "\213\377\377\377\377\4\263\264\263\377cdc\377\243\246\243\377\366\367"
  "\366\377\204\377\377\377\377\7\372\372\372\377\373\373\373\377\250\252"
  "\250\377vxv\377\207\211\207\377\263\265\263\377\344\345\344\377\217\377"
  "\377\377\377\5\345\351\345\377z|z\377\213\216\213\377\324\326\324\377"
  "\374\374\374\377\204\377\377\377\377\4\355\355\355\377~\201~\377\206"
  "\210\206\377\360\360\360\377\224\377\377\377\377\3\266\271\266\377_a"
  "_\377\331\335\331\377\215\377\377\377\377\3\327\333\327\377npn\377\272"
  "\275\272\377\204\377\377\377\377\3{}{\377\230\233\230\377\361\361\361"
  "\377\203\377\377\377\377\4\304\307\304\377twt\377\265\270\265\377\363"
  "\364\363\377\216\377\377\377\377\14\374\374\374\377\336\340\336\377~"
  "\201~\377\216\220\216\377\353\353\353\377\323\324\323\377\233\236\233"
  "\377svs\377|~|\377pqp\377gjg\377\371\372\371\377\211\377\377\377\377"
  "\5\351\352\351\377\\^\\\377@A@\377\177\201\177\377\353\355\353\377\204"
  "\377\377\377\377\6\314\316\314\377\221\222\221\377\201\203\201\377\232"
  "\234\232\377\336\340\336\377\373\373\373\377\221\377\377\377\377\4\306"
  "\307\306\377\201\204\201\377\217\221\217\377\343\345\343\377\205\377"
  "\377\377\377\4\266\267\266\377\210\212\210\377\272\274\272\377\370\371"
  "\370\377\223\377\377\377\377\3\325\330\325\377RTR\377\260\263\260\377"
  "\215\377\377\377\377\3\317\323\317\377cec\377\272\275\272\377\204\377"
  "\377\377\377\3mom\377\261\265\261\377\375\375\375\377\203\377\377\377"
  "\377\4\232\234\232\377nqn\377\326\332\326\377\374\374\374\377\220\377"
  "\377\377\377\13\263\266\263\377aca\377\223\225\223\377\203\206\203\377"
  "\203\205\203\377\252\255\252\377\334\340\334\377\264\267\264\377dgd\377"
  "\313\316\313\377\371\371\371\377\210\377\377\377\377\15\226\227\226\377"
  "\177\201\177\377\214\216\214\377tvt\377\223\226\223\377\343\344\343\377"
  "\377\377\377\377\360\362\360\377\301\303\301\377\212\214\212\377sus\377"
  "\305\310\305\377\355\356\355\377\223\377\377\377\377\5\376\376\376\377"
  "\260\262\260\377psp\377\253\256\253\377\347\350\347\377\204\377\377\377"
  "\377\5\346\347\346\377\225\230\225\377qsq\377\332\336\332\377\373\373"
  "\373\377\223\377\377\377\377\3x{x\377z}z\377\335\340\335\377\214\377"
  "\377\377\377\3\252\255\252\377\\^\\\377\321\324\321\377\202\377\377\377"
  "\377\4\373\373\373\377\321\325\321\377mom\377\307\313\307\377\203\377"
  "\377\377\377\4\365\365\365\377svs\377\223\226\223\377\373\374\373\377"
  "\221\377\377\377\377\13\340\344\340\377\205\206\205\377NPN\377uxu\377"
  "\274\277\274\377\345\351\345\377\365\371\365\377\333\337\333\377\204"
  "\207\204\377\235\240\235\377\345\347\345\377\207\377\377\377\377\15\307"
  "\310\307\377uwu\377\264\267\264\377\332\336\332\377\227\231\227\377v"
  "yv\377\253\256\253\377\324\327\324\377\300\302\300\377lnl\377\213\215"
  "\213\377\321\322\321\377\375\375\375\377\225\377\377\377\377\4\341\343"
  "\341\377\241\244\241\377}\200}\377\263\265\263\377\205\377\377\377\377"
  "\4\301\303\301\377twt\377\233\236\233\377\351\353\351\377\223\377\377"
  "\377\377\3\303\306\303\377^`^\377\212\214\212\377\214\377\377\377\377"
  "\2prp\377\205\207\205\377\203\377\377\377\377\4\365\366\365\377\271\274"
  "\271\377prp\377\320\323\320\377\202\377\377\377\377\4\374\374\374\377"
  "\331\332\331\377rtr\377\272\275\272\377\222\377\377\377\377\13\373\374"
  "\373\377\302\304\302\377oqo\377\217\222\217\377\334\340\334\377\347\353"
  "\347\377\344\350\344\377\343\347\343\377\245\250\245\377\202\204\202"
  "\377\305\307\305\377\206\377\377\377\377\14\366\367\366\377z|z\377\226"
  "\231\226\377\317\322\317\377\354\360\354\377\343\347\343\377\237\241"
  "\237\377XZX\377\203\206\203\377}\200}\377\220\222\220\377\311\313\311"
  "\377\227\377\377\377\377\5\373\373\373\377\341\345\341\377\202\204\202"
  "\377x{x\377\333\334\333\377\204\377\377\377\377\5\346\347\346\377\237"
  "\241\237\377jlj\377\321\324\321\377\376\376\376\377\223\377\377\377\377"
  "\4\257\262\257\377Y[Y\377\254\257\254\377\374\374\374\377\211\377\377"
  "\377\377\3\231\234\231\377aca\377\314\320\314\377\203\377\377\377\377"
  "\12\357\360\357\377\242\245\242\377fif\377\257\262\257\377\324\326\324"
  "\377\341\343\341\377\334\336\334\377\240\243\240\377tvt\377\325\327\325"
  "\377\223\377\377\377\377\12\355\356\355\377\253\256\253\377\200\202\200"
  "\377\264\270\264\377\261\265\261\377{\177{\377\237\242\237\377\233\236"
  "\233\377ili\377\233\234\233\377\206\377\377\377\377\14\255\256\255\377"
  "}\200}\377\223\226\223\377\257\262\257\377\260\263\260\377\360\364\360"
  "\377\322\326\322\377xzx\377[][\377\204\207\204\377\333\337\333\377\370"
  "\371\370\377\230\377\377\377\377\5\376\376\376\377\306\307\306\377\202"
  "\205\202\377\223\225\223\377\354\355\354\377\203\377\377\377\377\5\353"
  "\353\353\377\254\257\254\377mom\377\230\232\230\377\334\335\334\377\223"
  "\377\377\377\377\6\333\337\333\377^`^\377:;:\377\206\211\206\377\301"
  "\304\301\377\336\342\336\377\205\377\377\377\377\4\324\330\324\377\213"
  "\216\213\377686\377}\200}\377\204\377\377\377\377\12\345\347\345\377"
  "\202\205\202\377JKJ\377rtr\377\210\213\210\377\230\233\230\377\222\225"
  "\222\377mpm\377\212\215\212\377\346\350\346\377\223\377\377\377\377\13"
  "\376\377\376\377\347\347\347\377\211\214\211\377wyw\377\201\204\201\377"
  "WYW\377hkh\377ehe\377JLJ\377z|z\377\375\375\375\377\204\377\377\377\377"
  "\14\323\324\323\377hjh\377PQP\377bdb\377KKK\377\210\212\210\377\331\334"
  "\331\377\320\323\320\377`b`\377~\201~\377\325\327\325\377\370\371\370"
  "\377\232\377\377\377\377\7\365\365\365\377\264\267\264\377XZX\377\260"
  "\263\260\377\355\355\355\377\370\370\370\377\324\326\324\377\202\231"
  "\234\231\377\3\227\232\227\377jlj\377\257\261\257\377\223\377\377\377"
  "\377\20\265\270\265\377OPO\377\215\217\215\377kmk\377WYW\377mom\377\222"
  "\225\222\377\245\250\245\377\247\252\247\377\240\243\240\377\212\215"
  "\212\377]_]\377QSQ\377oro\377iki\377\335\341\335\377\203\377\377\377"
  "\377\12\336\340\336\377\200\202\200\377\200\203\200\377\251\254\251\377"
  "\247\251\247\377\232\235\232\377klk\377WYW\377\254\256\254\377\364\366"
  "\364\377\224\377\377\377\377\12\376\376\376\377\300\303\300\377SUS\377"
  "HJH\377z|z\377\333\337\333\377\271\274\271\377\207\211\207\377\253\256"
  "\253\377\366\366\366\377\204\377\377\377\377\12\203\204\203\377<><\377"
  "NPN\377svs\377qsq\377~\200~\377\307\312\307\377{}{\377\216\221\216\377"
  "\315\317\315\377\235\377\377\377\377\15\343\346\343\377\233\236\233\377"
  "\202\205\202\377\264\266\264\377\327\331\327\377\216\220\216\377dfd\377"
  "\265\270\265\377\344\350\344\377\177\202\177\377\200\203\200\377\321"
  "\322\321\377\371\372\371\377\221\377\377\377\377\20\226\231\226\377l"
  "ol\377\261\264\261\377\202\204\202\377\262\265\262\377\240\243\240\377"
  "\201\204\201\377ege\377fif\377qtq\377\202\204\202\377psp\377svs\377\263"
  "\266\263\377Y[Y\377\313\316\313\377\203\377\377\377\377\11\327\331\327"
  "\377|\177|\377\255\261\255\377\337\343\337\377\347\352\347\377\336\342"
  "\336\377\224\227\224\377|~|\377\323\324\323\377\226\377\377\377\377\11"
  "\355\360\355\377wyw\377343\377\202\204\202\377\350\351\350\377\377\377"
  "\377\377\354\355\354\377\355\357\355\377\374\374\374\377\203\377\377"
  "\377\377\13\345\347\345\377\232\235\232\377dfd\377\253\255\253\377\327"
  "\332\327\377\177\201\177\377lnl\377\200\203\200\377vxv\377\306\311\306"
  "\377\374\374\374\377\235\377\377\377\377\15\373\374\373\377\353\353\353"
  "\377\212\214\212\377XYX\377jlj\377\204\206\204\377\313\317\313\377\361"
  "\365\361\377\365\371\365\377\300\303\300\377\201\205\201\377\215\220"
  "\215\377\336\341\336\377\221\377\377\377\377\5{}{\377\200\201\200\377"
  "\225\230\225\377vyv\377\356\360\356\377\206\377\377\377\377\5\250\253"
  "\250\377oqo\377\321\325\321\377]_]\377\241\244\241\377\203\377\377\377"
  "\377\11\304\307\304\377nqn\377\253\256\253\377\265\270\265\377\326\332"
  "\326\377\341\345\341\377\202\205\202\377\224\227\224\377\351\352\351"
  "\377\227\377\377\377\377\4\262\263\262\377TVT\377}\177}\377\336\337\336"
  "\377\210\377\377\377\377\11\332\333\332\377\352\353\352\377\361\362\361"
  "\377\332\334\332\377rur\377@A@\377]_]\377\300\303\300\377\356\357\356"
  "\377\240\377\377\377\377\14\316\320\316\377y{y\377QRQ\377\240\243\240"
  "\377\354\360\354\377\346\352\346\377\316\322\316\377\337\343\337\377"
  "\231\234\231\377dfd\377\254\257\254\377\354\355\354\377\220\377\377\377"
  "\377\4\246\251\246\377XZX\377[][\377\222\225\222\377\207\377\377\377"
  "\377\5\250\254\250\377hkh\377\310\313\310\377VXV\377\220\223\220\377"
  "\203\377\377\377\377\11\254\257\254\377^`^\377wyw\377XYX\377\226\230"
  "\226\377\312\315\312\377cec\377\255\260\255\377\370\370\370\377\227\377"
  "\377\377\377\4\340\342\340\377\254\257\254\377\301\303\301\377\376\376"
  "\376\377\213\377\377\377\377\5\355\356\355\377aca\377^_^\377\254\256"
  "\254\377\357\361\357\377\241\377\377\377\377\14\365\366\365\377\313\316"
  "\313\377XYX\377\236\241\236\377\337\342\337\377\256\261\256\377svs\377"
  "klk\377wyw\377PRP\377oqo\377\306\311\306\377\220\377\377\377\377\4\337"
  "\343\337\377\205\210\205\377WYW\377\271\274\271\377\207\377\377\377\377"
  "\5\303\306\303\377WYW\377aba\377ACA\377\250\253\250\377\202\377\377\377"
  "\377\11\367\367\367\377\230\233\230\377+,+\377UVU\377wyw\377cfc\377\225"
  "\230\225\377_a_\377\325\331\325\377\230\377\377\377\377\3\372\372\372"
  "\377\356\357\356\377\364\364\364\377\214\377\377\377\377\5\361\362\361"
  "\377\306\311\306\377\273\275\273\377\377\377\377\377\376\376\376\377"
  "\242\377\377\377\377\14\370\371\370\377\241\242\241\377\202\204\202\377"
  "\237\242\237\377\210\213\210\377VXV\377\222\225\222\377\203\206\203\377"
  "]`]\377LNL\377\274\277\274\377\367\370\367\377\221\377\377\377\377\1"
  "\365\367\365\377\211\377\377\377\377\4\303\306\303\377\251\254\251\377"
  "\264\267\264\377\353\355\353\377\202\377\377\377\377\11\351\351\351\377"
  "y{y\377/0/\377\220\223\220\377\302\305\302\377ili\377QRQ\377ege\377\351"
  "\354\351\377\250\377\377\377\377\2\373\373\373\377\367\370\367\377\245"
  "\377\377\377\377\13\344\344\344\377\222\224\222\377hjh\377vxv\377npn"
  "\377\332\333\332\377\347\350\347\377\325\326\325\377\341\343\341\377"
  "\345\347\345\377\374\374\374\377\241\377\377\377\377\11\347\350\347\377"
  "\226\230\226\377~\201~\377\324\326\324\377\346\350\346\377\220\223\220"
  "\377FHF\377\203\204\203\377\375\375\375\377\320\377\377\377\377\5\313"
  "\315\313\377\200\202\200\377HIH\377lnl\377\332\334\332\377\250\377\377"
  "\377\377\6\362\362\362\377\371\371\371\377\370\371\370\377\274\276\274"
  "\377wzw\377\256\257\256\377\321\377\377\377\377\5\360\361\360\377\300"
  "\302\300\377\\_\\\377LNL\377\354\355\354\377\253\377\377\377\377\3\347"
  "\350\347\377\311\313\311\377\340\341\340\377\322\377\377\377\377\4\370"
  "\370\370\377\256\261\256\377\232\235\232\377\343\344\343\377\377\377"
  "\377\377\377\202\377\377\377\377\3\364\364\364\377\375\375\375\377\372"
  "\373\372\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\355"
  "\377\377\377\377"};


/* GdkPixbuf RGBA C-Source image dump 1-byte-run-length-encoded */

#ifdef __SUNPRO_C
#pragma align 4 (wanda)
#endif
#ifdef __GNUC__
static const guint8 wanda[] __attribute__ ((__aligned__ (4))) = 
#else
static const guint8 wanda[] = 
#endif
{ ""
  /* Pixbuf magic (0x47646b50) */
  "GdkP"
  /* length: header (24) + pixel_data (17491) */
  "\0\0Dk"
  /* pixdata_type (0x2010002) */
  "\2\1\0\2"
  /* rowstride (1152) */
  "\0\0\4\200"
  /* width (288) */
  "\0\0\1\40"
  /* height (22) */
  "\0\0\0\26"
  /* pixel_data: */
  "\204A|\322\270\6""9m\270\277@z\317\271A|\322\2708l\267\2779m\271\276"
  "A|\322\270\227A|\322\271\206@|\322\271\1""8n\272\277\202@|\322\271\2"
  "3c\251\303<t\305\274\214@|\322\271\224@|\322\272\4""9o\274\277@|\322"
  "\272@|\322\2730]\235\310\225@|\322\273\214\77|\322\274\4:r\302\300>{"
  "\320\274\77|\322\2741a\245\307\203\77|\322\274\1@{\316\275\202\77|\322"
  "\274\203@|\322\274\6Ix\300\300\233b0\355\254]\21\371\243a#\362fo\211"
  "\317Ez\306\277\216@|\322\274\202@|\323\275\5""8m\271\303\77z\320\276"
  "@|\323\2758l\271\3038m\273\303\236@|\323\275\1""9n\274\302\202@|\323"
  "\275\2""3d\252\307<t\306\300\240@|\323\275\1""9o\275\302\202@|\323\275"
  "\1""0]\236\312\242@|\323\275\4:q\301\301@|\323\275\77{\321\2752b\247"
  "\310\202@|\323\275\1A{\317\276\205@|\323\275\5Ix\301\301\233b1\355\254"
  "]\21\371\243a$\362fo\212\317\203@|\323\275\1Dz\312\277\211@|\323\275"
  "\202C~\322\257\202C~\322\260\12C}\320\261Is\255\302C~\322\260Bg\235\306"
  "=d\234\305C~\322\260Bg\240\277hq\213\304_s\232\300E}\314\262\212C~\322"
  "\260\2J{\305\263E}\320\260\215D~\322\260\11Hy\301\270Fx\300\270D~\322"
  "\260\77Y\177\324@o\264\272D~\322\260:h\252\274E}\320\260D}\316\262\220"
  "C~\322\261\206B~\322\261\206B~\322\262\10Ht\263\301B~\322\262A|\316\263"
  "8Mj\336B~\322\2629h\254\276C|\320\262C|\316\263\210B}\322\262\2B}\322"
  "\263B}\317\264\223B}\322\263\25B|\321\263Ht\264\302B}\322\263B\\\203"
  "\326Ym\224\305\211gN\334\224V\32\366\261Z\5\376\260Y\5\375\253_\26\366"
  "\230d7\346dq\220\305Pw\265\272\250^\30\364\331\235X\376\367\307\205\377"
  "\353\263m\377\265b\17\375\210dJ\335\221a:\344Hz\305\266\203B}\322\263"
  "\213B}\322\264\12B|\320\265Ir\256\305B}\322\264Ag\236\311=d\234\310B"
  "}\322\264Bg\241\303fq\214\307^r\233\303D|\314\265\202B}\322\264\210B"
  "}\322\265\2I{\305\270C|\320\265\215B}\322\265\11Ey\302\276Cw\301\275"
  "A}\322\266=Y\200\327=o\265\277A}\322\2668h\253\301B|\320\266B|\316\267"
  "\234A}\322\266\1Gt\264\304\202A}\322\266\5""9Mi\341A}\322\266:l\262\277"
  "\77y\314\267B|\316\267\211A}\322\267\1A}\317\270\206A}\322\267\206A|"
  "\322\267\210A|\322\270\22Iu\265\306A|\322\270Au\301\276KVl\336\210fP"
  "\337\224V\33\366\260X\2\376\260Y\5\375\253_\26\366\227d8\350cp\221\311"
  "Ow\266\277\250^\30\365\330\231R\375\367\306\203\377\352\261h\377\264"
  "a\15\375\177g]\332\202A|\322\271\1\201fY\334\211A|\322\271\205E\200\322"
  "\247\24i\203\247\317O\202\310\260j\213\271\304\213\177z\344\255f\40\363"
  "\247~T\376\327\243f\374\326\240d\375\273m#\375\246b\37\356zlj\311I}\311"
  "\251E\200\322\250wmp\310\236e/\346\246b!\356\254`\30\364\263b\26\371"
  "\260Y\5\375\223b:\337\216E\200\322\250\14E\177\322\250p\207\246\325E"
  "\177\322\250\203\223\253\337i}\240\305\207iV\331\222iD\363\260[\11\374"
  "\262[\10\376\254_\25\364\216gH\331fr\221\274\204E\177\322\250\7K|\305"
  "\253^v\240\267Yw\253\263Rz\267\260L|\304\254F~\320\250E\177\322\250\216"
  "E\200\322\251\14_\202\264\303Y\204\300\272U\205\310\264\207\217\232\352"
  "\206hU\324\220nO\364\260Z\7\374\260X\2\376\256]\15\371\243c&\353\210"
  "hR\326Xw\253\265\202E\177\322\252\7mp\204\302\245b#\355\260[\10\373\261"
  "Z\7\376\257^\20\371\242_\37\356er\220\276\212D\177\322\252\207D\200\322"
  "\252\25g\203\251\317P\201\304\264\220\177r\347\254vA\374\356\301\210"
  "\377\306\264\234\377\375\345\301\377\375\346\303\377\372\334\261\377"
  "\361\307\216\377\312\206@\374\261[\6\376\336\244_\376\375\327\240\377"
  "\374\307z\377\365\305\204\377\260[\10\373Oz\273\261\255[\13\371\252^"
  "\24\365[v\245\270\216D\177\322\253\24i\203\250\321N\201\311\263i\213"
  "\272\307\212\177z\346\255f\40\364\245zP\376\325\235]\374\321\225S\374"
  "\264^\13\375\245a\37\357yll\314H}\311\256D\200\322\254vmq\312\235e0\347"
  "\245b\"\356\252_\26\365\257[\14\371\260X\4\375\222b;\340\205D\200\322"
  "\254\206D\177\322\255\204D~\322\255\13o\206\247\327D~\322\255\203\223"
  "\253\341h}\241\311\206iX\333\222iD\364\260[\11\374\262[\10\376\253_\25"
  "\365\215gJ\333dr\222\300\204D~\322\255\6J{\306\260]u\241\273Wv\254\270"
  "Py\270\264K{\304\261E}\320\255\211D~\322\255\205B~\322\255\15C~\323\256"
  "Y\201\271\303\\\204\275\302L\201\315\264\212\221\233\357\202h[\325\217"
  "oQ\364\257[\10\373\261X\2\376\256\\\14\372\243b&\355\211fO\332Xv\247"
  "\272\202C~\323\256\10fq\215\302\242c'\354\257[\10\373\261Z\7\376\257"
  "_\17\372\244^\40\357jp\207\304C~\323\256\220C~\322\257\22Q~\277\275e"
  "\204\260\315\210jW\332\244\210n\376\354\276\203\377\311\266\235\377\372"
  "\343\300\377\375\345\302\377\372\333\256\377\360\305\213\377\311\204"
  "<\374\260Y\3\376\335\241Y\376\375\330\242\377\374\311~\377\365\304\203"
  "\377\260[\11\373Ny\274\266\202C~\322\260\2\256X\4\374Tw\261\271\207C"
  "~\322\260\1D~\322\260\205H\201\322\236\24Fz\304\243ELV\346\220Z(\360"
  "lbW\376\345\321\262\377\250\236\217\377\373\336\264\377\375\341\267\377"
  "\376\345\277\377\371\333\262\377\335\254r\375\265g\34\371\247`\33\356"
  "\304~7\373\354\272|\377\362\306\214\377\365\315\226\377\334\243`\376"
  "\250`\34\356K~\310\242\204G\201\322\237\2I\200\313\241O}\301\243\211"
  "G\201\322\237\27Aa\215\277JYp\324sZF\352zaF\376\270\235y\377\313\270"
  "\234\377\375\346\303\377\375\347\305\377\373\337\266\377\356\301\207"
  "\376\307\202:\373\247a\35\355oq\202\271hs\217\265\231e8\335\261[\7\373"
  "\266b\16\372\266c\21\373\262[\5\374\261W\0\377\256X\4\374\200j_\311G"
  "\200\317\241\215G\201\322\240\24F\200\321\241BN^\342y`N\333`WN\377\321"
  "\253z\376\223\213\177\377\373\343\300\377\375\350\307\377\375\344\276"
  "\377\370\326\247\377\354\276\203\376\307\2028\375\246d%\354\251`\32\360"
  "\275p#\373\353\263k\377\371\315\215\377\370\312\211\377\266b\16\374\214"
  "hN\322\203G\201\322\241\213G\201\323\242\204G\200\323\242\26Nu\261\255"
  "YF5\372\266\207T\375}wm\377\361\326\257\377\240\223\200\377\373\324\232"
  "\377\375\326\235\377\375\330\242\377\375\336\260\377\375\346\303\377"
  "\374\341\274\377\375\325\231\377\374\307{\377\374\307y\377\317\214D\374"
  "\202i\\\313F\200\322\242\234d1\342\300i\15\377\254]\20\366M}\304\246"
  "\210F\200\322\242\205F\200\322\243\24Dy\304\250ELW\350\217Z)\361lbV\376"
  "\345\317\257\377\250\237\220\377\373\340\270\377\375\344\277\377\376"
  "\346\302\377\367\321\232\377\326\232T\374\261_\17\371\246`\34\356\304"
  "~6\374\353\266s\377\360\300\202\377\365\311\216\377\332\237Y\376\247"
  "`\35\357J}\311\246\217F\200\322\244\27@a\216\303JYq\326sZG\353zaF\376"
  "\270\235y\377\313\270\234\377\375\346\303\377\375\347\305\377\373\337"
  "\266\377\356\301\207\376\307\202:\373\247a\35\356nq\204\276gr\221\271"
  "\230e9\337\261[\7\373\266b\16\372\266c\21\373\261[\5\375\261W\0\377\256"
  "X\4\374\177j`\314F\177\317\246\216F\200\322\245\23ANb\341uaU\332cWL\376"
  "\310\243t\376\223\213~\377\373\343\277\377\375\350\307\377\375\344\277"
  "\377\371\330\251\377\354\300\206\376\311\206<\375\252e$\357\245a\37\356"
  "\273m\36\373\351\260g\377\371\314\214\377\371\315\215\377\272g\23\374"
  "\221gB\332\220E\200\321\246\202E\200\322\247\15O{\275\255iJ-\370\235"
  "vL\375\227\214|\377\312\267\233\377\245\227\201\377\360\314\227\377\375"
  "\327\236\377\375\331\244\377\375\337\262\377\375\347\305\377\374\341"
  "\274\377\375\326\234\377\202\374\310|\377\2\317\214C\374\201j^\316\202"
  "E\200\322\247\3G\177\315\250\261W\0\377\231b3\343\210E\200\322\250\205"
  "J\203\322\226\23hu\223\253\203F\13\373\30\25\20\37793*\377B2\36\3778"
  ",\34\377\321\244f\377\375\325\233\377\375\326\234\377\375\330\241\377"
  "\375\340\265\377\375\345\303\377\365\325\252\377\374\316\217\377\375"
  "\324\232\377\374\306u\377\365\307\210\377\261^\17\371W}\264\237\206J"
  "\203\322\226\2\251Z\17\362sp|\264\210J\203\322\226\25mp\202\262B$\6\375"
  "+#\31\377\26\24\20\377I8\40\377y\\6\377\354\302\211\377\375\326\234\377"
  "\375\331\244\377\375\337\261\377\375\346\303\377\372\333\257\377\333"
  "\245c\374\275r%\372\354\305\226\376\372\325\240\377\375\325\234\377\364"
  "\274o\377\270b\14\373\243a$\345^x\247\244\207I\202\322\227\210I\203\322"
  "\230\23\\z\251\245\210K\21\367\31\24\16\37793+\377:-\35\3776+\34\377"
  "\321\244h\377\375\325\233\377\375\330\242\377\375\333\252\377\375\341"
  "\270\377\375\347\306\377\372\334\262\377\361\301\203\377\375\342\272"
  "\377\374\321\222\377\375\322\226\377\305\1775\374\216hK\316\211I\202"
  "\322\231\1K\201\313\233\210I\202\322\231\11L\201\312\233\245a\37\351"
  "\250~L\376\30\26\23\377;3&\377\77""1\35\3778,\34\377\303\230[\377\375"
  "\325\232\377\203\375\326\234\377\13\374\324\231\377\374\317\215\377\374"
  "\266Q\377\375\317\214\377\270i\32\372ax\240\251I\203\321\231~ld\301\305"
  "s\30\373\306s\30\373\224f\77\326\215J\202\322\232\23gt\224\256\204G\14"
  "\373\30\25\20\37793*\377B2\36\3778,\34\377\321\244f\377\375\325\233\377"
  "\375\330\241\377\375\337\261\377\375\345\301\377\375\344\301\377\365"
  "\324\247\377\374\317\217\377\375\326\234\377\374\313\204\377\365\305"
  "\203\377\260\\\14\371U}\265\244\214H\203\322\233\204H\202\322\234\25"
  "kp\204\266B$\6\375+#\31\377\26\24\20\377I8\40\377y\\6\377\354\302\211"
  "\377\375\326\234\377\375\331\244\377\375\337\261\377\375\346\303\377"
  "\372\333\257\377\333\245c\374\275r%\372\354\305\226\376\372\325\240\377"
  "\375\325\234\377\364\274o\377\267b\14\373\243a%\346]z\250\251\204I\203"
  "\322\234\213I\201\322\234\23Xz\263\245\213O\24\364\34\26\17\377<5,\377"
  "/&\32\3775*\34\377\311\236b\377\375\325\233\377\375\330\242\377\375\333"
  "\251\377\375\341\267\377\375\347\306\377\373\336\265\377\361\301\202"
  "\377\375\341\271\377\374\323\225\377\375\322\225\377\312\206>\374\224"
  "e\77\327\211H\201\322\235\1J\200\313\240\210H\201\322\236\11K\177\313"
  "\240\245a\40\352\326\237\\\376D>5\3774-#\377\34\27\21\377R<!\377\201"
  "a8\377\356\305\211\377\202\375\326\234\377\7\375\326\235\377\374\324"
  "\232\377\374\317\215\377\374\266Q\377\375\321\220\377\266d\22\372^w\242"
  "\256\202G\201\322\237\4L~\306\242\261X\1\376\264`\16\373et\224\263\207"
  "G\201\322\237\204M\204\321\215\12`z\250\232\260\\\12\367\366\321\236"
  "\377\312\242s\377/\34\11\377\6\5\3\377\2\1\0\377\234~S\377\323\241_\377"
  "\375\326\233\377\202\375\326\234\377\7\374\326\234\377\374\325\231\377"
  "\374\315\210\377\373\270S\377\374\300i\377\335\237W\376\224gB\314\206"
  "L\204\322\216\4N\202\312\220\261V\0\377\261W\0\377\232c4\325\204L\204"
  "\322\216\202L\203\322\216\11xpu\261\272k\34\373\373\334\261\377pH\36"
  "\377\25\17\12\377\1\1\1\377*!\25\377\336\265y\377\352\267q\377\203\375"
  "\326\234\377\11\374\334\253\377\375\343\273\377\375\336\260\377\374\311"
  "}\377\374\306v\377\367\306\200\377\266d\20\373\203la\272L\203\322\216"
  "\210M\204\322\217\2lv\220\246\223d;\320\205M\204\322\217\5az\247\235"
  "\266g\31\366\367\322\241\377\314\247{\3770\35\12\377\202\6\5\3\377\3"
  "\241\203V\377\323\241_\377\375\326\233\377\202\375\326\234\377\7\374"
  "\325\234\377\374\331\245\377\374\327\242\377\374\301k\377\374\277f\377"
  "\353\265p\377\251`\31\353\210K\203\322\220\3or\207\252\235b0\331\254"
  "Y\13\365\210K\204\323\221\30\232d5\327\323\223J\374\375\346\301\377\324"
  "\252t\3777\40\12\377\11\7\5\377\0\0\0\377w`\77\377\321\242b\377\372\320"
  "\224\377\374\323\226\377\374\321\222\377\374\317\215\377\374\316\210"
  "\377\374\314\203\377\373\326\241\377\261X\3\376R\177\301\226K\203\321"
  "\221`y\244\240\263]\10\373\365\2478\377\261Z\5\373T~\275\227\213K\203"
  "\321\221\12^z\251\236\260^\15\370\367\323\242\377\312\241r\377/\34\11"
  "\377\6\5\3\377\2\1\0\377\234~S\377\323\241_\377\375\326\233\377\202\375"
  "\326\234\377\7\374\330\241\377\374\326\234\377\374\315\210\377\373\270"
  "S\377\374\305u\377\332\230K\375\223gC\316\204K\203\322\222\7L\202\315"
  "\223gu\225\247\201k`\277\224e\77\321\221fB\317\215fI\313xou\265\205K"
  "\203\322\223\11vpw\264\272k\34\373\373\334\261\377pH\36\377\25\17\12"
  "\377\1\1\1\377*!\25\377\336\265y\377\352\267q\377\203\375\326\234\377"
  "\10\374\334\253\377\375\343\273\377\375\336\260\377\374\311}\377\374"
  "\306v\377\367\306\200\377\266d\20\373\201lc\275\211J\203\322\223\2iu"
  "\221\251\222c=\322\203J\203\322\223\202J\203\322\224\12Z|\262\236\261"
  "d\33\363\364\316\231\376\326\263\210\3775\40\12\377\7\6\4\377\2\2\1\377"
  "\216sK\377\322\243a\377\374\324\230\377\202\375\326\234\377\11\374\325"
  "\233\377\374\331\246\377\374\330\243\377\374\303o\377\374\275a\377\357"
  "\274x\377\254^\23\362J\202\317\225J\203\322\224\206J\203\323\225\4it"
  "\220\253\231d6\330\256X\7\371J\202\320\226\207J\203\323\225\22\231d6"
  "\330\321\217D\373\375\346\302\377\373\326\240\377f@\27\377\23\16\11\377"
  "\1\1\1\377%\36\23\377\336\265x\377\351\266p\377\374\323\226\377\374\321"
  "\222\377\374\317\215\377\374\316\210\377\374\316\206\377\373\325\235"
  "\377\261W\1\376Q\177\303\233\202J\203\322\226\4S~\275\234\261Y\3\375"
  "\345\236S\376\247^\33\352\207J\203\322\226\203M\207\321\204\24V\201\275"
  "\212\254]\22\357\356\300\205\376\375\343\273\377\310~1\377\317\331\345"
  "\377\361\363\366\377\224\235\247\377.)\"\377\356\300|\377\354\266k\377"
  "\374\323\225\377\374\321\222\377\374\317\215\377\374\316\210\377\374"
  "\314\204\377\374\312\200\377\374\317\216\377\310\206C\374\207jV\271\206"
  "N\206\321\205\5Z\201\270\215\260X\2\374\265[\4\377\307~9\376yru\250\204"
  "N\207\322\205\25\177pj\256\300v)\373\374\340\271\377\353\277\210\377"
  "\312\254\220\377\360\363\367\377\323\331\337\377WVU\377\207qR\377\356"
  "\265f\377\374\325\232\377\374\323\225\377\374\321\222\377\374\317\215"
  "\377\374\316\210\377\374\314\204\377\374\312\177\377\374\321\224\377"
  "\306}4\375\245^\34\344[\177\263\217\206M\206\321\206\6V\200\275\214\205"
  "jZ\267\254\\\20\356\272i\33\372\264^\14\376T\201\301\213\203M\206\321"
  "\206\26^}\256\222\270k\36\366\371\327\250\377\375\344\277\377\310~1\377"
  "\317\331\345\377\361\363\366\377\224\235\247\377.)\"\377\356\300|\377"
  "\354\266k\377\374\323\225\377\374\321\222\377\374\317\215\377\374\316"
  "\210\377\374\314\204\377\374\312\200\377\375\331\244\377\311\2017\374"
  "\233b0\325[~\264\220N\205\321\207\203N\204\321\210\7^}\255\224\215hM"
  "\300\256[\13\363\274l\36\371\340\223J\376\264^\13\376S\201\306\213\202"
  "N\204\321\210\204M\204\321\210\31\177mh\262\276t%\371\375\344\275\377"
  "\375\331\244\377\321\214A\377\314\324\336\377\363\366\371\377\243\254"
  "\270\37772,\377\323\254q\377\350\253\\\377\374\316\211\377\374\314\204"
  "\377\374\312\200\377\374\310{\377\374\306w\377\375\333\252\377\350\257"
  "g\375\257]\17\362rs\203\244Q\203\311\212\261X\1\376\374\260A\377\304"
  "p\25\373\202la\265\205N\205\321\210\205M\204\322\211\24V\177\276\217"
  "\254]\22\360\357\303\213\376\375\341\270\377\310~1\377\317\331\345\377"
  "\361\363\366\377\224\235\247\377.)\"\377\356\300|\377\354\266k\377\374"
  "\323\225\377\374\321\222\377\374\317\215\377\374\316\210\377\374\314"
  "\204\377\374\312\200\377\375\324\232\377\303y0\374\206jX\274\202M\205"
  "\320\212\11_|\252\226\227e9\317\263\\\12\373\276p&\372\322\207<\373\342"
  "\225L\377\343\230P\377\272g\25\375~ni\262\204M\205\320\212\25}ok\261"
  "\300v)\373\374\340\271\377\353\277\210\377\312\254\220\377\360\363\367"
  "\377\323\331\337\377WVU\377\207qR\377\356\265f\377\374\325\232\377\374"
  "\323\225\377\374\321\222\377\374\317\215\377\374\316\210\377\374\314"
  "\204\377\374\312\177\377\374\321\224\377\306}4\375\244^\34\346Y\177\264"
  "\224\206L\206\321\213\6U\200\276\221\204j\\\272\253\\\21\357\272i\33"
  "\372\264^\14\376T\201\302\220\203M\205\321\213\25W\177\272\222\264h\36"
  "\362\366\323\243\377\375\345\301\377\315\205:\377\315\326\342\377\363"
  "\365\370\377\234\245\260\3773.'\377\336\264u\377\351\261f\377\374\323"
  "\226\377\374\321\222\377\374\317\215\377\374\316\211\377\374\314\204"
  "\377\374\312\200\377\375\330\242\377\317\213C\374\236`)\334\\}\256\227"
  "\204L\204\321\214\7Z}\263\225\207jT\277\255\\\17\361\271e\25\372\334"
  "\214=\376\266b\21\374[}\261\226\204L\204\321\214\202M\204\321\215\32"
  "~mj\265\275q\40\371\375\344\275\377\375\332\250\377\353\267v\377\313"
  "\256\223\377\360\363\370\377\323\330\337\377TTR\377\213uT\377\355\264"
  "d\377\374\316\211\377\374\314\204\377\374\312\200\377\374\310{\377\374"
  "\307x\377\375\334\254\377\347\256f\375\257]\17\363qs\205\250M\204\321"
  "\215zop\263\301s%\372\372\277\177\377\262Z\6\374]|\256\232\206L\204\322"
  "\216\203Q\210\321|\27\230g:\305\325\224J\374\375\345\277\377\365\306"
  "\210\377\330\237f\377c\207\266\377^k{\377V}\257\377\305\305\303\377\344"
  "\266r\377\341\240I\377\374\316\210\377\374\314\204\377\374\312\200\377"
  "\374\310{\377\374\306v\377\374\304s\377\374\307{\377\372\327\243\377"
  "\321\222M\373\236e2\316g{\235\217U\203\305\201\202P\207\321}\6Z\201\271"
  "\205\204m^\255\264\\\7\377\346\236Y\377\335\231V\376\214iP\266\203P\207"
  "\321}\27a~\254\211\261\\\10\371\373\335\264\377\375\337\261\377\330\223"
  "J\377\243\264\311\377l\203\241\377b{\234\377\243\270\321\377\262\245"
  "\221\377\374\275a\377\365\302z\377\374\316\210\377\374\314\204\377\374"
  "\312\200\377\374\310{\377\374\306v\377\374\304s\377\374\315\211\377\373"
  "\327\242\377\336\241Z\374\263^\13\372\234c2\315\202hz\234\220\11\201"
  "nf\252\227e:\306\252^\30\345\267f\27\373\321\212G\373\356\253j\377\371"
  "\267v\377\267c\24\371b~\247\214\203P\207\321~\40\243c(\327\362\311\224"
  "\376\375\344\277\377\365\306\211\377\330\237f\377c\207\266\377^k{\377"
  "V}\257\377\305\305\303\377\344\266r\377\341\240I\377\374\316\210\377"
  "\374\314\204\377\374\312\200\377\374\310{\377\374\306v\377\374\304s\377"
  "\374\317\215\377\374\330\243\377\342\245\\\375\262[\11\372\236d1\317"
  "mz\224\225\177pl\250\246`\35\337\263\\\11\373\326\214F\373\361\264z\377"
  "\371\276\203\377\371\273\177\377\263[\7\371e}\244\216\205Q\210\321\177"
  "\32R\205\314\201\256]\16\361\365\313\221\377\375\335\257\377\372\320"
  "\225\377\322\217M\377h\211\264\377_m}\377W\177\262\377\313\314\315\377"
  "\324\256w\377\343\241I\377\374\311}\377\374\307x\377\374\305s\377\374"
  "\303o\377\373\301j\377\373\301n\377\374\324\227\377\367\311\210\377\273"
  "j\31\372\245`\40\335\261W\0\377\374\264K\377\343\224.\377\246d$\335\212"
  "P\210\321\200\37\227g<\307\330\233V\374\375\343\274\377\365\306\210\377"
  "\330\237f\377c\207\266\377^k{\377V}\257\377\305\305\303\377\344\266r"
  "\377\341\240I\377\374\316\210\377\374\314\204\377\374\312\200\377\374"
  "\310{\377\374\306v\377\374\304s\377\374\315\210\377\372\325\237\377\313"
  "\2047\373\236e1\321\235d1\321\265a\17\373\331\220G\375\365\260l\377\370"
  "\247Y\377\365\231@\377\363\215*\377\370\253`\377\274n#\373d{\244\220"
  "\203O\206\320\202\27`}\254\216\260\\\10\371\373\335\264\377\375\337\261"
  "\377\330\223J\377\243\264\311\377l\203\241\377b{\234\377\243\270\321"
  "\377\262\245\221\377\374\275a\377\365\302z\377\374\316\210\377\374\314"
  "\204\377\374\312\200\377\374\310{\377\374\306v\377\374\304s\377\374\315"
  "\211\377\373\327\242\377\336\241Z\374\263^\13\373\234c3\317\202gy\236"
  "\224\11\200mh\254\226d<\310\252^\30\346\263]\12\373\315\1775\373\354"
  "\245_\377\371\266u\377\267c\24\372b|\250\220\203P\205\321\202\40\233"
  "e5\316\356\302\210\376\375\345\301\377\367\315\221\377\324\225T\377f"
  "\210\265\377_l|\377V~\262\377\312\312\311\377\334\262u\377\342\241J\377"
  "\374\316\210\377\374\314\204\377\374\312\200\377\374\310{\377\374\306"
  "w\377\374\304s\377\374\315\210\377\374\331\246\377\344\252a\375\263]"
  "\12\373\236c-\324mw\221\232zqs\247\244`\"\334\261[\10\373\317\1770\373"
  "\355\241Y\377\371\260j\377\371\266u\377\270a\17\370ly\222\232\205O\207"
  "\321\204\33Q\205\314\205\256]\16\362\365\311\215\377\375\337\262\377"
  "\375\326\234\377\327\222J\377\240\262\312\377n\204\241\377d}\236\377"
  "\247\272\323\377\257\241\215\377\374\275a\377\365\275n\377\374\307x\377"
  "\374\305s\377\374\303o\377\373\301j\377\373\302p\377\374\324\231\377"
  "\366\307\204\377\272g\25\372\245a!\337\261Z\6\372\351\247]\377\372\301"
  "\201\377\276m\34\371us}\245\206N\206\321\205\202R\211\321s\40h}\236\205"
  "\263]\12\372\373\332\254\377\375\332\250\377\371\316\225\377\332\226"
  "S\377\205\234\273\3776e\242\377\200\230\266\377\340\340\334\377\302\235"
  "m\377\343\240E\377\374\312\201\377\374\306v\377\374\304s\377\374\302"
  "n\377\373\277f\377\374\272Z\377\374\266N\377\374\270U\377\373\314\212"
  "\377\340\252k\374\302{5\370\272l\40\374\265c\23\367\261^\16\364\272j"
  "\36\374\312\203=\372\355\260u\377\370\255e\377\330\224Q\374\212kW\252"
  "\203S\210\322t\6\245b$\325\351\267v\375\375\343\273\377\375\330\241\377"
  "\344\243^\377\320\303\267\377\202Bl\242\377\32\311\316\322\377\332\330"
  "\323\377\350\243C\377\371\305{\377\374\310}\377\374\306v\377\374\304"
  "s\377\374\302n\377\374\277e\377\374\272Y\377\374\266N\377\374\273[\377"
  "\374\312\202\377\370\313\214\377\335\231Q\375\270e\24\373\272h\32\371"
  "\316\203;\370\343\237\\\376\361\273\207\377\370\305\223\377\370\261l"
  "\377\366\2255\377\371\265s\377\265\\\7\370m{\223\213\202R\211\320u!z"
  "sy\227\315\210@\373\375\347\305\377\375\330\244\377\371\316\225\377\332"
  "\226S\377\205\234\273\3776e\242\377\200\230\266\377\340\340\334\377\302"
  "\235m\377\343\240E\377\374\312\201\377\374\306v\377\374\304s\377\374"
  "\302n\377\373\277c\377\373\270X\377\374\264N\377\374\273^\377\375\315"
  "\207\377\370\312\212\377\333\225I\375\266b\20\373\303s$\367\350\235V"
  "\377\370\301\213\377\372\276\204\377\370\247Y\377\366\2267\377\371\265"
  "t\377\263Z\3\372f{\242\207\205Q\207\321w\32\206l\\\251\305|.\371\375"
  "\343\273\377\374\325\234\377\373\322\227\377\330\217G\377\231\254\303"
  "\3776e\241\377p\215\260\377\336\336\333\377\300\245\202\377\342\233="
  "\377\374\306x\377\373\301l\377\373\276c\377\374\265N\377\374\261C\377"
  "\374\257=\377\374\257>\377\374\276e\377\371\311\210\377\345\250a\377"
  "\261W\1\377\373\270U\377\357\246C\377\257_\20\356\205R\210\321w\204R"
  "\207\321x\40g{\240\211\265a\17\372\373\333\256\377\375\330\243\377\371"
  "\316\225\377\332\226S\377\205\234\273\3776e\242\377\200\230\266\377\340"
  "\340\334\377\302\235m\377\343\240E\377\374\312\201\377\374\306v\377\374"
  "\304s\377\374\302n\377\373\277f\377\374\272Z\377\374\266N\377\374\276"
  "c\377\373\321\225\377\335\241[\374\336\235V\375\367\273x\377\370\241"
  "G\377\364}\12\377\362w\0\377\361w\0\377\363x\0\377\370\241M\377\307y"
  "/\370|rr\237\203Q\210\322y\6\244c&\327\351\267v\375\375\343\273\377\375"
  "\330\241\377\344\243^\377\320\303\267\377\202Bl\242\377\32\311\316\322"
  "\377\332\330\323\377\350\243C\377\371\305{\377\374\310}\377\374\306v"
  "\377\374\304s\377\374\302n\377\374\277e\377\374\272Y\377\374\266N\377"
  "\374\273[\377\374\312\202\377\370\313\214\377\335\231Q\375\270e\24\373"
  "\272h\32\371\313}1\371\340\220C\376\354\242Z\377\367\262o\377\370\241"
  "N\377\366\212\"\377\371\264r\377\265\\\7\370kz\226\217\202P\210\321z"
  "!qw\212\224\306~3\373\375\347\305\377\375\331\245\377\373\322\232\377"
  "\330\221J\377\221\246\300\3776e\241\377v\221\263\377\337\337\334\377"
  "\302\243y\377\342\236C\377\374\312\201\377\374\306w\377\374\304s\377"
  "\374\302n\377\373\277e\377\373\271X\377\374\265N\377\374\273]\377\374"
  "\313\204\377\371\314\215\377\336\232M\375\266c\23\372\300p\40\370\344"
  "\225G\376\366\261m\377\370\244S\377\366\214%\377\365}\10\377\371\264"
  "q\377\266\\\5\370oy\217\223\205Q\211\321{\33\205m^\254\304y)\371\375"
  "\344\275\377\374\325\234\377\374\325\233\377\342\241Y\377\315\302\270"
  "\377Al\241\377Dm\242\377\310\315\322\377\326\323\317\377\347\242B\377"
  "\371\300q\377\373\301n\377\373\276c\377\374\265N\377\374\261C\377\374"
  "\257=\377\374\257\77\377\374\300j\377\371\310\207\377\344\242V\377\365"
  "\270n\377\372\271n\377\371\263e\377\320\2000\373\211jU\263\206P\207\321"
  "}\202U\212\321k\21\247b\"\322\342\245^\375\374\341\266\377\374\324\227"
  "\377\374\327\237\377\347\242Z\377\334\272\226\377\321\316\307\377\313"
  "\313\305\377\320\304\262\377\316\210E\377\363\272m\377\374\306y\377\373"
  "\300i\377\373\275^\377\375\264K\377\374\257\77\377\204\374\257>\377\13"
  "\372\266Z\377\372\266d\377\371\264h\377\366\262m\377\363\260o\377\371"
  "\262l\377\371\271{\377\370\243S\377\367\227;\377\315\210D\371\204pc\232"
  "\202T\213\321k\22\200ql\224\276q\"\371\375\345\300\377\374\326\236\377"
  "\374\327\237\377\367\312\212\377\341\233U\377\327\315\277\377\314\314"
  "\306\377\315\313\302\377\330\256\201\377\330\232L\377\374\314\206\377"
  "\374\302o\377\373\301h\377\373\272W\377\373\262G\377\374\257=\377\204"
  "\374\257>\377\37\372\251=\377\372\262[\377\372\273u\377\371\263k\377"
  "\370\256f\377\370\254b\377\370\254d\377\367\234E\377\366\217+\377\365"
  "\212\"\377\365\256i\377\261Y\3\371W\211\312oU\213\320mX\210\310o\260"
  "_\21\355\372\334\261\377\374\336\260\377\374\324\227\377\374\327\237"
  "\377\347\242Z\377\334\272\226\377\321\316\307\377\313\313\305\377\320"
  "\304\262\377\316\210E\377\363\272m\377\374\306y\377\373\300g\377\373"
  "\271V\377\373\261F\377\205\374\257>\377\13\372\254C\377\372\266c\377"
  "\371\276z\377\371\264m\377\367\243R\377\367\232@\377\370\241N\377\367"
  "\232@\377\366\231\77\377\363\254h\377\261Y\5\366\206T\212\320n\17\260"
  "]\14\361\364\312\220\377\374\331\245\377\374\320\217\377\374\325\232"
  "\377\353\252`\377\336\263\207\377\321\316\306\377\313\313\305\377\320"
  "\307\270\377\325\221P\377\352\256_\377\373\304p\377\374\266M\377\374"
  "\260\77\377\205\374\257>\377\6\373\255A\377\372\260P\377\262Z\2\377\372"
  "\272_\377\371\270[\377\261Y\3\373\211S\212\321o\21\246c#\324\343\253"
  "h\375\374\336\257\377\374\324\227\377\374\327\237\377\347\242Z\377\334"
  "\272\226\377\321\316\307\377\313\313\305\377\320\304\262\377\316\210"
  "E\377\363\272m\377\374\306y\377\373\300i\377\373\275^\377\375\264K\377"
  "\374\257\77\377\203\374\257>\377\6\374\262G\377\373\271`\377\372\255"
  "Q\377\367\221\36\377\366\202\12\377\363x\0\377\202\362w\0\377\30\363"
  "x\0\377\366\220-\377\341\224I\376\237b,\310T\212\321pT\212\322q~qp\231"
  "\276q\"\371\375\345\300\377\374\326\236\377\374\327\237\377\367\312\212"
  "\377\341\233U\377\327\315\277\377\314\314\306\377\315\313\302\377\330"
  "\256\201\377\330\232L\377\374\314\206\377\374\302o\377\373\301h\377\373"
  "\272W\377\373\262G\377\374\257=\377\204\374\257>\377\40\372\251=\377"
  "\372\262[\377\372\273u\377\371\263j\377\370\241M\377\367\2256\377\366"
  "\211\40\377\365|\7\377\365y\0\377\365\200\16\377\365\256i\377\261Y\4"
  "\371U\207\312sS\211\320qT\207\316r\253_\25\346\370\326\251\377\374\337"
  "\263\377\374\323\226\377\374\327\240\377\351\250^\377\335\267\216\377"
  "\321\317\307\377\313\313\305\377\321\305\265\377\324\217K\377\355\266"
  "h\377\374\307z\377\373\301h\377\373\271Y\377\373\262H\377\374\257=\377"
  "\204\374\257>\377\6\372\254A\377\372\266b\377\371\275x\377\371\264k\377"
  "\367\2268\377\365\177\14\377\202\365y\0\377\4\365\201\21\377\365\254"
  "e\377\261X\3\373T\207\313u\205R\211\321s\17\257]\14\361\364\310\215\377"
  "\374\332\250\377\374\320\217\377\374\323\225\377\367\307\201\377\341"
  "\233U\377\327\315\277\377\314\314\306\377\315\313\302\377\330\256\201"
  "\377\326\226H\377\373\306w\377\374\266N\377\374\260\77\377\205\374\257"
  ">\377\7\373\256D\377\373\266\\\377\370\250F\377\364\220\36\377\367\246"
  "K\377\337\222C\377\234c1\307\206S\211\321t\21Y\213\321bY\212\314c\261"
  "[\6\366\365\307\207\377\374\324\231\377\374\316\210\377\374\322\224\377"
  "\371\313\214\377\356\247]\377\353\236R\377\346\243_\377\355\237R\377"
  "\364\267k\377\373\313\204\377\374\272Y\377\373\262F\377\374\257=\377"
  "\205\374\257>\377\35\374\260A\377\373\271a\377\372\266e\377\370\2358"
  "\377\365\200\11\377\360w\1\377\362\215+\377\365\244T\377\366\2245\377"
  "\370\244T\377\277r)\372n}\230wV\214\321cX\215\321c\213mZ\230\305|1\371"
  "\375\330\242\377\374\320\213\377\374\317\213\377\374\327\236\377\364"
  "\271s\377\354\237R\377\347\243_\377\354\236R\377\357\251]\377\372\307"
  "\201\377\373\303o\377\374\266O\377\374\257A\377\207\374\257>\377\14\372"
  "\2441\377\371\232(\377\370\2347\377\367\224/\377\364\212$\377\363\223"
  "8\377\363\235L\377\363\227>\377\363\215)\377\365\216(\377\351\243`\377"
  "\252^\31\326\202W\214\322d\17Y\212\313f\261\\\11\363\367\315\222\377"
  "\375\324\225\377\374\316\210\377\374\322\224\377\371\313\214\377\356"
  "\247]\377\353\236R\377\346\243_\377\355\237R\377\364\267k\377\373\311"
  "\200\377\375\266O\377\374\257A\377\207\374\257>\377\13\372\2441\377\371"
  "\232(\377\370\2358\377\367\2307\377\364\212%\377\363\2238\377\363\235"
  "L\377\363\227>\377\364\232B\377\351\243^\377\251_\31\330\206W\214\320"
  "f\15\257_\20\350\361\277|\377\375\315\211\377\374\313\203\377\374\317"
  "\214\377\372\315\215\377\360\250^\377\353\235P\377\346\244`\377\354\235"
  "Q\377\363\261b\377\373\305t\377\374\261D\377\207\374\257>\377\6\373\262"
  "M\377\365\264b\377\266^\7\377\374\302l\377\371\274f\377\261Y\3\373\210"
  "W\213\321f\20W\212\314g\261\\\10\367\367\316\227\377\374\317\217\377"
  "\374\316\210\377\374\322\224\377\371\313\214\377\356\247]\377\353\236"
  "R\377\346\243_\377\355\237R\377\364\267k\377\373\313\204\377\374\272"
  "Y\377\373\262F\377\374\257=\377\205\374\257>\377\14\374\260A\377\373"
  "\270_\377\372\266e\377\371\254X\377\370\244N\377\367\232@\377\366\220"
  "-\377\366\206\32\377\365z\2\377\365\177\14\377\361\254i\377\257\\\13"
  "\355\202V\212\321h\17\211m\\\233\305|2\371\375\330\242\377\374\320\213"
  "\377\374\317\213\377\374\327\236\377\364\271s\377\354\237R\377\347\243"
  "_\377\354\236R\377\357\251]\377\372\307\201\377\373\303o\377\374\266"
  "O\377\374\257A\377\207\374\257>\377\14\372\2441\377\371\232(\377\370"
  "\2347\377\367\217%\377\363x\0\377\361w\0\377\360v\0\377\361w\0\377\362"
  "w\0\377\365\204\25\377\351\243`\377\251^\32\330\202W\213\322i\17V\212"
  "\320i\260^\16\355\363\307\211\377\374\323\227\377\374\316\210\377\374"
  "\322\222\377\372\317\220\377\357\251_\377\353\236P\377\345\244`\377\354"
  "\236R\377\363\265i\377\373\312\201\377\374\267R\377\374\257@\377\207"
  "\374\257>\377\13\372\2442\377\371\232(\377\370\2358\377\367\2240\377"
  "\364y\2\377\361w\0\377\360v\0\377\361w\0\377\362\203\26\377\355\244^"
  "\377\255^\22\343\206V\213\321j\15\257_\21\351\361\276z\377\375\316\213"
  "\377\374\313\203\377\374\312\202\377\374\323\225\377\363\265k\377\353"
  "\237R\377\346\244`\377\353\235Q\377\360\251Y\377\372\300n\377\374\267"
  "S\377\207\374\257>\377\10\373\263P\377\373\276o\377\372\256S\377\364"
  "\223$\377\367\242D\377\343\231M\377\245`!\321U\212\321k\205T\213\321"
  "k\202Y\216\320Y\14\236d0\263\322\212:\373\375\313\203\377\374\277e\377"
  "\374\306w\377\374\315\211\377\374\322\225\377\372\311\204\377\372\304"
  "|\377\374\313\205\377\373\310z\377\373\264K\377\207\374\257>\377\14\374"
  "\262F\377\374\312\202\377\340\243_\375\325\214>\373\362\257f\377\371"
  "\263l\377\367\2269\377\362\2203\377\362\241S\377\365\2236\377\364\257"
  "k\377\265c\22\367\203Z\216\321Z\15\\\214\312\\\261]\12\357\360\274y\377"
  "\374\276c\377\375\302k\377\375\310~\377\374\323\226\377\374\317\216\377"
  "\372\305~\377\373\310\177\377\374\313\202\377\373\274`\377\374\256>\377"
  "\210\374\257>\377\15\373\256=\377\372\256I\377\372\300z\377\350\246a"
  "\377\356\254j\377\371\271{\377\365\250_\377\363\240Q\377\364\227=\377"
  "\365\215(\377\365\217+\377\335\235]\374\230e9\251\202Y\215\317[\15X\216"
  "\320\\\231g9\255\320\212<\372\375\311\201\377\374\274^\377\375\302m\377"
  "\374\315\211\377\374\322\225\377\372\311\204\377\372\302z\377\373\307"
  "{\377\373\304p\377\374\262F\377\210\374\257>\377\14\373\256=\377\372"
  "\254E\377\372\300y\377\351\246_\377\356\253h\377\371\273\177\377\365"
  "\252a\377\363\240Q\377\364\227=\377\366\233C\377\343\237]\376\235e2\263"
  "\206Z\215\320]\14\225hB\243\314\2043\373\372\306x\377\374\272Y\377\374"
  "\303n\377\374\313\202\377\374\321\221\377\373\311\200\377\372\302v\377"
  "\373\305s\377\374\303q\377\374\264J\377\207\374\257>\377\7\374\275a\377"
  "\367\304\202\377\307|.\373\304t\37\377\374\305s\377\364\270f\377\261"
  "]\11\361\203Y\216\320]\206X\215\321^\14\235e3\265\330\230Q\374\374\303"
  "p\377\374\277e\377\374\306w\377\374\315\211\377\374\322\225\377\372\311"
  "\204\377\372\304|\377\374\313\205\377\373\310z\377\373\264K\377\207\374"
  "\257>\377\15\374\262F\377\374\312\202\377\340\243^\374\310\1775\367\320"
  "\2035\370\325\2032\374\333\2064\376\343\225I\377\353\245a\377\370\253"
  "`\377\365\204\26\377\362\260p\377\260[\12\354\202X\215\321_\15Z\213\312"
  "a\261]\12\360\360\274y\377\374\276c\377\375\302k\377\375\310~\377\374"
  "\323\226\377\374\317\216\377\372\305~\377\373\310\177\377\374\313\202"
  "\377\373\274`\377\374\256>\377\210\374\257>\377\15\373\256=\377\372\256"
  "I\377\372\300z\377\350\246a\377\356\250b\377\370\255d\377\364\2212\377"
  "\360y\6\377\362w\0\377\364x\0\377\365\205\30\377\335\235]\374\227e=\254"
  "\203X\215\322`\14\222kG\243\313\2013\371\375\314\205\377\374\273\\\377"
  "\374\301m\377\374\315\210\377\374\322\225\377\373\312\205\377\372\303"
  "z\377\373\307z\377\373\304s\377\374\262I\377\211\374\257>\377\13\372"
  "\253C\377\373\277w\377\353\250b\377\354\245\\\377\371\260j\377\365\224"
  "8\377\360z\10\377\362w\0\377\364\204\31\377\346\240\\\377\242c'\302\204"
  "X\214\320a\202Y\213\321b\14\224hE\246\314\2032\373\374\307}\377\374\272"
  "Y\377\374\303n\377\374\306w\377\374\320\217\377\373\314\205\377\372\301"
  "v\377\373\301o\377\374\304t\377\374\272Z\377\207\374\257>\377\10\374"
  "\277f\377\367\304\200\377\316\2058\373\343\241X\376\367\276|\377\367"
  "\263i\377\322\2024\373\222hF\247\206V\214\321c\202[\221\320Q\13q\200"
  "\225d\262Z\5\372\316\2069\377\370\261N\377\374\261C\377\374\265J\377"
  "\374\271U\377\371\270\\\377\354\251O\377\355\243@\377\371\253;\377\203"
  "\374\257>\377\2\365\2468\377\370\252:\377\202\374\257>\377\15\374\265"
  "N\377\372\313\211\377\321\217G\372\256_\21\336\241d*\262\261Y\4\366\304"
  "n\33\367\351\227H\377\370\276\205\377\371\305\223\377\367\256i\377\363"
  "\262s\377\265d\25\360\204]\220\321R\13\227jA\232\301s\"\373\342\244X"
  "\377\374\261C\377\374\256>\377\374\261E\377\373\267S\377\374\271Z\377"
  "\371\263O\377\361\244;\377\370\252:\377\203\374\257>\377\2\365\2468\377"
  "\370\252:\377\203\374\257>\377\16\373\260C\377\372\303t\377\363\277~"
  "\377\305z-\371\257\\\15\343\260Z\10\353\300p#\370\347\245d\376\370\306"
  "\226\377\364\264u\377\365\240N\377\366\227;\377\340\232X\376\236d/\255"
  "\203[\217\317S\13m\203\240b\262Y\5\373\314\2057\377\366\261M\377\374"
  "\257>\377\374\260\77\377\374\264J\377\374\267T\377\370\264S\377\356\243"
  "\77\377\366\2509\377\203\374\257>\377\2\365\2468\377\370\252:\377\203"
  "\374\257>\377\15\373\256\77\377\371\276j\377\366\303\202\377\312~-\371"
  "\260\\\14\345\260[\7\355\303s'\370\353\253m\377\370\311\232\377\364\257"
  "l\377\365\242Q\377\336\233[\375\230f<\240\204Z\216\320T\202\\\217\320"
  "T\13n\203\243b\262[\7\372\304z,\377\363\250A\377\374\257=\377\374\260"
  "C\377\373\264L\377\374\270T\377\366\263S\377\354\242@\377\366\2479\377"
  "\202\374\257>\377\15\362\2435\377\355\2362\377\371\254=\377\370\254="
  "\377\367\253<\377\366\275i\377\354\272u\377\267d\21\371\247]\30\317\324"
  "\2149\374\374\305s\377\352\251U\377\257`\23\335\211[\216\320U\13q\200"
  "\227h\266c\22\372\316\206:\377\370\254@\377\374\261C\377\374\265J\377"
  "\374\271U\377\371\270\\\377\354\251O\377\355\243@\377\371\253;\377\203"
  "\374\257>\377\2\365\2468\377\370\252:\377\202\374\257>\377\16\374\265"
  "N\377\372\313\211\377\320\214C\371\255_\22\335\211p_\206\217lO\222\226"
  "h\77\240\235c1\256\246b\"\300\260[\11\352\325\2056\373\370\254b\377\360"
  "\256m\377\261[\13\347\203\\\216\317W\13\226jC\235\301s\"\373\342\244"
  "X\377\374\261C\377\374\256>\377\374\261E\377\373\267S\377\374\271Z\377"
  "\371\263O\377\361\244;\377\370\252:\377\203\374\257>\377\2\365\2468\377"
  "\370\252:\377\203\374\257>\377\16\373\260C\377\372\303t\377\363\277~"
  "\377\305z-\371\257\\\15\344\260Z\11\353\277m\36\370\344\230M\376\366"
  "\260l\377\363\235L\377\364\217-\377\366\217+\377\340\232X\376\236d0\257"
  "\203[\217\317W\13f\206\262`\261Z\4\372\313\2002\377\365\261Q\377\374"
  "\257>\377\374\257\77\377\374\263I\377\374\267R\377\370\264T\377\356\244"
  "@\377\365\2479\377\203\374\257>\377\2\366\2478\377\367\2519\377\203\374"
  "\257>\377\15\373\256=\377\371\276g\377\367\305\203\377\317\2023\371\261"
  "]\13\351\261\\\12\351\276k\32\370\344\227K\377\367\261n\377\362\231C"
  "\377\364\216,\377\342\235Y\376\235e3\257\206Y\216\320Y\13k\202\245g\261"
  "[\6\373\305{-\377\364\254E\377\374\257=\377\374\260C\377\373\262H\377"
  "\373\265N\377\362\255O\377\357\251H\377\372\257B\377\202\374\257>\377"
  "\16\362\2435\377\355\2362\377\371\254=\377\370\254=\377\367\253=\377"
  "\366\300p\377\355\272v\377\266b\16\371\235e1\262\254^\23\334\334\225"
  "I\375\371\277\177\377\273g\26\370~v{z\206[\215\321Z\203_\221\320H\37"
  "\252c\34\302\315\214H\375\271o\"\377\275j\22\377\277l\21\377\301t\40"
  "\377\322\220@\377\344\2324\377\360\246;\377\357\246:\377\356\245:\377"
  "\355\244:\377\324\204\"\377\303o\22\377\343\2312\377\351\2419\377\351"
  "\254S\377\343\272\177\377\277w-\370\251b\35\301c\215\277M^\222\320Ie"
  "\213\275N\226kE\213\261[\14\337\263[\6\372\312z.\367\337\220D\376\347"
  "\231M\377\267b\21\374}{\203c\203]\220\321I!f\213\273O\257]\16\342\305"
  "\202>\376\276p\40\377\307v\31\377\310w\31\377\306v\36\377\331\231M\377"
  "\341\233=\377\357\244:\377\357\246:\377\356\245:\377\355\244:\377\324"
  "\204\"\377\277k\17\377\322\203!\377\331\213(\377\334\221,\377\340\240"
  "I\377\345\267y\377\327\234U\374\264]\12\370\233g:\231^\220\313K^\221"
  "\321J\214q]z\254^\23\320\265`\16\371\317\2029\370\341\231S\377\357\253"
  "j\377\344\231P\377\254_\24\315\204^\221\321J\37\244d(\262\311\211I\374"
  "\274t)\377\275k\22\377\300m\22\377\305x\"\377\325\230N\377\334\2259\377"
  "\355\2428\377\357\246:\377\356\245:\377\355\244:\377\324\204\"\377\300"
  "l\20\377\332\214)\377\346\2355\377\351\2419\377\350\246F\377\351\274"
  "{\377\344\256h\377\267a\17\370\241e.\247`\215\306M_\216\314L\217nT\201"
  "\256\\\16\332\274m\"\371\342\235Z\375\363\266{\377\345\232R\377\253`"
  "\26\312\207]\221\320L\31\251c\37\301\323\236f\375\271q(\377\264`\11\377"
  "\272g\17\377\274m\26\377\321\222J\377\334\232H\377\352\241:\377\354\244"
  ":\377\353\2439\377\332\215)\377\275h\15\377\344\2345\377\347\2408\377"
  "\346\240;\377\345\263i\377\331\242_\376\261]\12\367\217nS\203\250b\37"
  "\300\344\244S\377\374\306w\377\313\177+\370\227iA\223\211^\220\320L\27"
  "]\221\320M\251c\35\304\320\224V\375\271n!\377\275i\17\377\277l\21\377"
  "\301t\40\377\322\220@\377\344\2324\377\360\246;\377\357\246:\377\356"
  "\245:\377\355\244:\377\324\204\"\377\303o\22\377\343\2312\377\351\241"
  "9\377\351\254S\377\343\272\177\377\276u+\370\250b\40\300`\216\303P]\221"
  "\320M\204\\\217\321N\5j\206\257X\261Y\2\367\364\257n\377\316~0\370\241"
  "b(\261\203\\\217\321N!e\212\274T\256^\17\343\305\202>\376\276p\40\377"
  "\307v\31\377\310w\31\377\306v\36\377\331\231M\377\341\233=\377\357\244"
  ":\377\357\246:\377\356\245:\377\355\244:\377\323\204\"\377\300k\17\377"
  "\323\203!\377\331\213(\377\334\221,\377\340\240I\377\345\267y\377\327"
  "\233T\375\263]\12\370\232g<\234^\217\311P^\220\317O\213q`}\254^\24\321"
  "\265_\15\371\315}/\370\340\222H\377\357\247d\377\344\231P\377\253_\25"
  "\317\204]\217\317O\37\237g2\247\306\205A\373\275v/\377\275j\23\377\300"
  "m\21\377\304v\37\377\324\227M\377\333\224:\377\354\2428\377\357\246:"
  "\377\356\245:\377\355\244:\377\327\210%\377\277k\17\377\330\212'\377"
  "\346\2355\377\351\2419\377\350\245D\377\351\273x\377\346\262m\377\270"
  "d\21\367\242c*\262a\213\301T\\\220\320P\212r_\177\255^\22\325\265`\16"
  "\372\332\212<\375\361\251d\377\350\233Q\377\256]\21\330\207\\\220\320"
  "P\32\250c\40\303\323\233b\375\267n#\377\265a\13\377\273i\20\377\301u"
  "#\377\325\231P\377\341\234@\377\355\244:\377\354\244:\377\353\2439\377"
  "\332\215)\377\275h\15\377\344\2345\377\347\2408\377\346\240<\377\345"
  "\265p\377\331\243_\376\261\\\10\370\214nW\206[\217\320Q\210qb\177\301"
  "o\36\366\366\266o\377\261X\2\372`\215\311T\206]\220\321R\203b\224\320"
  "@\23k\213\265G\256^\20\332\331\255}\374\345\313\253\377\350\307\233\377"
  "\346\265n\377\336\2326\377\335\2316\377\334\2316\377\333\2305\377\332"
  "\2275\377\307{\36\377\263X\0\377\303b\1\377\264Z\1\377\274h\22\377\301"
  "\202=\377\260b\26\372\241f.\233\206a\223\320@\6s\206\241M\225kH}\250"
  "_\36\262\260^\17\320\262[\10\343\243`#\252\204a\223\320@\25o\206\246"
  "K\266i\35\342\332\257\200\375\331\266\216\377\342\276\217\377\350\275"
  "\200\377\337\235=\377\335\2316\377\334\2316\377\333\2305\377\332\227"
  "5\377\307{\36\377\264X\0\377\316f\0\377\312g\2\377\306d\4\377\300d\10"
  "\377\275h\20\377\267_\12\377\262]\7\357z}\216U\205`\221\321A\6v\201\232"
  "Q\227iA\205\254_\26\302\263[\7\350\262Y\2\367\257[\11\342\204a\222\321"
  "B\25a\221\312C\254`\27\311\330\255~\374\347\317\261\377\352\313\242\377"
  "\351\274|\377\336\2339\377\335\2316\377\334\2316\377\333\2305\377\332"
  "\2275\377\307{\36\377\263X\0\377\311e\1\377\277a\3\377\264[\1\377\267"
  "^\6\377\275m\33\377\276m\32\374\262]\12\351\207ukh\205`\223\317C\5\206"
  "wof\252^\27\301\264\\\11\372\310q\35\373\262Z\4\360\207`\223\317C\32"
  "j\212\263J\263b\23\346\343\302\234\376\351\322\264\377\352\314\243\377"
  "\352\305\217\377\337\242H\377\333\2305\377\332\2275\377\331\2265\377"
  "\321\213,\377\263Z\1\377\263Y\0\377\316\212+\377\324\226:\377\324\247"
  "f\377\301\213G\374\257Y\6\361\205ulga\220\310D\262[\6\363\364\273l\377"
  "\362\265c\377\262Z\5\367k\210\256L_\222\317C\211^\223\320D\23g\213\266"
  "K\256^\20\333\335\266\216\374\345\312\247\377\350\306\227\377\346\265"
  "n\377\336\2326\377\335\2316\377\334\2316\377\333\2305\377\332\2275\377"
  "\307{\36\377\263X\0\377\303b\1\377\264Z\1\377\274h\21\377\301\202<\377"
  "\257b\24\372\240f2\234\210`\221\320E\4\261Y\4\361\317\210B\373\257\\"
  "\13\335h\212\274J\204`\221\320E\25m\205\250P\265i\36\343\332\257\200"
  "\375\331\266\216\377\342\276\217\377\350\275\200\377\337\235=\377\335"
  "\2316\377\334\2316\377\333\2305\377\332\2275\377\307{\35\377\266Z\1\377"
  "\317g\1\377\312g\2\377\306d\4\377\300d\10\377\275h\20\377\266_\12\377"
  "\262]\7\357x~\222Z\203_\220\321F\202^\221\317F\6s\202\234U\225jD\210"
  "\253`\27\304\262[\7\351\262Y\2\367\257[\11\343\205^\221\317F\24\252a"
  "\35\276\324\250x\372\346\315\257\377\352\313\243\377\351\276\200\377"
  "\337\235<\377\335\2316\377\334\2316\377\333\2305\377\332\2275\377\312"
  "~!\377\262X\0\377\311e\1\377\300a\3\377\265Z\2\377\266]\5\377\275l\32"
  "\377\276n\33\375\261]\6\356\212scr\205`\222\317G\7\177yze\250a\34\274"
  "\263\\\6\371\310r\35\373\262X\2\370_\217\314H]\222\317G\205_\221\320"
  "H\23i\211\265O\263b\23\347\342\277\226\376\351\323\265\377\353\314\241"
  "\377\350\273|\377\334\2339\377\333\2305\377\332\2275\377\331\2265\377"
  "\321\213,\377\263Z\1\377\263Y\0\377\316\212+\377\324\226;\377\325\251"
  "k\377\301\212E\375\257Y\6\362\204vpm\202^\222\320I\4\233f6\232\315o\15"
  "\372\317o\12\373\253_\27\307\207^\222\320I\204d\224\3177\24u\203\235"
  "D\257[\10\345\306\217L\375\314\250t\377\313\236Z\377\312\2166\377\311"
  "\2141\377\310\2131\377\307\2121\377\302\202)\377\266`\10\377\317f\0\377"
  "\365y\0\377\363z\4\377\344z\25\377\320y\40\377\262[\7\361W}\243BW\205"
  "\267=`\221\305:\212c\226\3178\205e\224\3208\24x\205\234E\261]\13\341"
  "\317\231\\\373\316\252w\377\313\236[\377\312\2178\377\311\2141\377\310"
  "\2131\377\307\2121\377\301\200'\377\266a\11\377\266Z\1\377\323i\1\377"
  "\357v\0\377\365{\4\377\367\225*\377\372\257K\377\373\277i\377\300s!\367"
  "\233h9\203\212c\225\3219\1g\221\277=\205c\225\3219\24n\216\266\77\256"
  "]\15\331\301\206D\373\312\246q\377\313\234X\377\312\2165\377\311\214"
  "1\377\310\2131\377\307\2121\377\302\202)\377\265`\11\377\276_\1\377\361"
  "w\0\377\365y\0\377\365\200\17\377\363\2304\377\354\244L\377\271f\22\374"
  "}qn\\]\214\314<\207b\223\321:\4y\202\214N\246a\"\244\260X\3\363y\202"
  "\220M\207a\224\317;\5\201{zV\260[\7\365\316\241h\377\313\237_\377\312"
  "\2179\377\202\311\2141\377\20\310\2131\377\306\210/\377\267f\15\377\317"
  "h\0\377\332l\0\377\270c\14\377\302\231^\377\267y3\372\255[\11\344pqx"
  "WW\203\273@\220mRn\272e\14\365\365\264Z\377\267c\16\370\232i>\201\213"
  "c\223\317;\26b\224\320<s\204\241H\257[\12\346\313\233a\375\313\247o\377"
  "\313\227K\377\312\2141\377\311\2141\377\310\2131\377\307\2121\377\302"
  "\202)\377\266`\10\377\317f\0\377\365y\0\377\363}\13\377\344{\27\377\317"
  "x\36\377\262[\5\362Uw\245GV\205\274A^\217\305>b\224\320<\205`\222\320"
  "=\3\260Y\5\352\250c#\254d\216\277A\206`\222\320=\24s\205\240J\261]\14"
  "\341\317\231]\373\316\252w\377\313\236[\377\312\2178\377\311\2141\377"
  "\310\2131\377\307\2121\377\301\200(\377\267a\11\377\266Z\1\377\323i\1"
  "\377\357v\0\377\365{\4\377\367\225*\377\372\257K\377\373\277i\377\300"
  "s!\367\231i=\206\212b\223\321>\1f\217\301B\205a\222\321>\24f\214\304"
  "A\256_\22\320\276\200>\372\312\245p\377\313\236Z\377\312\2166\377\311"
  "\2141\377\310\2131\377\307\2121\377\303\202*\377\266b\13\377\274]\1\377"
  "\360w\0\377\365y\0\377\365\177\15\377\363\2261\377\354\244K\377\275k"
  "\27\374\205kYn^\215\310A\207`\223\317\77\4u\204\231N\243d(\240\260Y\4"
  "\360\202xw\\\207`\223\317\77\5\177{}Y\260Z\6\365\315\236_\377\313\237"
  "_\377\312\220;\377\202\311\2141\377\22\310\2131\377\306\210/\377\267"
  "f\15\377\317h\0\377\332l\0\377\270c\15\377\302\233a\377\267w1\372\255"
  "[\11\344pp|[Z\207\277Db\221\314A_\224\320@\256`\23\314\310j\11\377\262"
  "Z\4\362s\204\233O_\224\320@\206a\223\320@\203g\224\316.\202e\226\316"
  "/\25\201~\177D\260^\17\313\250]\20\367\257\202J\377\267\226g\377\265"
  "\216U\377\264\213M\377\264\210G\377\262\204C\377\261\207L\377\263\\\6"
  "\377\347t\5\377\365{\4\377\366\206\21\377\372\255N\377\301r!\366|Y:q"
  "Cg\213@Mp\233;U\177\2566b\221\3051\216d\227\317/\17w\210\235:\252b\34"
  "\242\261]\11\367\270\203D\373\274\232i\377\266\221]\377\264\216T\377"
  "\264\213M\377\262\206G\377\262\216X\377\263\213V\377\266o'\377\263Y\2"
  "\377\307h\14\377\352\232<\377\202\374\277f\377\4\356\265i\377\260]\15"
  "\330S}\2607`\216\3062\212f\225\3200\205d\227\3211\25w\204\233>\254b\27"
  "\264\245X\13\367\253}D\376\267\224`\377\265\214P\377\264\211H\377\264"
  "\206B\377\262\202>\377\262\207J\377\262o&\377\267\\\4\377\350w\12\377"
  "\366\214\36\377\371\234)\377\374\274]\377\315\2079\367\213\\.\205Jo\240"
  ">P}\2629]\216\3114\207c\224\3212\2n\212\2609q\207\246<\210c\224\3212"
  "\25\223nOd\261Z\5\360\255t2\373\266\221Z\377\267\212G\377\266\205<\377"
  "\265\2015\377\264}.\377\265c\15\377\344v\14\377\365\201\20\377\301c\11"
  "\377\260\\\10\372\246Z\17\311MYiQB`\205EKk\223@\251^\25\272\275f\13\375"
  "\264[\4\370\245d&\223\203e\226\3162\213d\224\3173\25\177~\204H\260^\20"
  "\314\256g\40\370\260\207T\377\267\221[\377\265\211I\377\264\206B\377"
  "\264\204;\377\262\201:\377\261\202B\377\263[\5\377\347v\11\377\365~\12"
  "\377\366\210\26\377\372\261V\377\277n\32\366{Z\77tCi\222DLq\241\77S\177"
  "\264:`\220\3125\204b\225\3204\1\215s^\\\210b\225\3204\20e\223\3205v\206"
  "\242@\251b\37\245\261]\11\367\270\203D\374\274\232i\377\266\221]\377"
  "\264\216T\377\264\213M\377\262\206G\377\262\216X\377\263\213V\377\266"
  "o'\377\263Y\2\377\307h\14\377\352\232<\377\202\374\277f\377\5\356\265"
  "i\377\260]\16\331U\177\256<a\217\3027e\223\3205\216c\225\3215\25r\212"
  "\253>\253a\34\255\247X\12\370\252{@\376\266\224a\377\265\215P\377\264"
  "\211I\377\264\206B\377\262\202=\377\261\207J\377\262r*\377\265[\4\377"
  "\345v\11\377\366\214\35\377\371\233(\377\374\272Z\377\324\220A\371\223"
  "Z(\223Ln\234CR{\254>]\212\3009\207b\223\3226\3i\214\274;r\204\242Bb\223"
  "\3226\207d\224\3177\26\222oSh\261Z\5\361\254p,\373\266\217V\377\267\212"
  "G\377\266\205<\377\265\2016\377\264}/\377\265c\16\377\344v\14\377\365"
  "\201\20\377\301c\10\377\260[\7\373\245Z\21\312MZoVDc\215JHp\231FRy\250"
  "A]\210\273<\261Y\3\363\262[\6\352\213ud]\210c\226\3178\207i\226\315&"
  "\24\230e:b\256Y\6\343\255d\27\367\260v4\372\246u;\376\250|E\377\260\203"
  "K\377\247r3\371\251[\15\373\270]\6\376\362\214$\377\370\225!\377\373"
  "\272\\\377\316\210<\370\202Q#\2026NmA;Y{<Fc\2156Ot\2440a\213\302*\215"
  "g\230\316'\202e\225\317'\25}vs>\250]\24\255\264^\13\371\271p%\365\262"
  "s2\367\270\200@\373\311\217K\374\277x0\366\264^\13\373\255Y\7\347\211"
  "M\25\245\203M\27\232\255Z\7\344\273k\30\366\342\240P\376\336\236Q\375"
  "\253\\\16\307Bc\2156Jt\2440[\213\302*e\225\317'\217h\227\320(\24\215"
  "mOS\255[\12\326\253a\24\367\253o-\370\243p4\376\246x>\377\255~B\377\250"
  "p0\374\245`\31\367\254V\2\373\263X\1\374\335\211-\375\373\274b\377\374"
  "\304r\377\306y'\365\177T,z9^\177>Dh\2218Lz\2502\\\220\305,\212f\231\321"
  ")\205i\226\315*\202d\226\315*\23v\206\2325\252^\24\260\251[\16\371\266"
  "\177<\377\250{@\377\244|F\377\251\201K\377\264]\7\377\360\210#\377\365"
  "}\11\377\357\213,\377\262Z\4\371)\77YP,GbK3PiFEViF\257X\3\357\252^\22"
  "\266\210q\\M\221g\230\316*\24\226h=e\256Z\7\344\261m%\370\260x:\372\246"
  "v>\376\251}G\377\260\202I\377\250q3\372\252\\\20\373\270\\\4\376\363"
  "\217*\377\370\227&\377\373\275c\377\314\2044\367\201Q'\2049SuC=^\203"
  ">Hh\2268Qz\2552b\220\312,\204e\225\317+\213h\227\317,\24\177xxC\250^"
  "\26\257\264^\13\371\271p%\366\262s2\367\270\200@\373\311\217K\375\277"
  "x0\366\264^\13\373\255Z\7\350\211M\27\247\203N\33\234\256Z\10\345\273"
  "k\30\366\342\240P\376\336\236Q\375\253\\\17\311Ig\216;Qx\2435c\220\307"
  ".\216f\225\320,\202d\226\321-\25\206p^Q\255]\14\320\253`\23\367\253o"
  ",\367\243o4\375\246w>\377\256\177C\377\250q1\374\244a\32\367\252V\2\373"
  "\263X\1\374\331\203(\375\373\272a\377\374\303o\377\314\2031\366\207U"
  "&\210=X\177BGd\216=Ns\2427_\212\2770d\226\321-\220g\224\316.\24w\206"
  "\2369\252^\25\261\250Y\13\371\266}7\377\250y=\377\244zB\377\251\201I"
  "\377\264]\7\377\360\210#\377\365}\11\377\357\213,\377\262Z\5\3710E]U"
  "3LfP6QpK>\\}EHi\215\77wj_S\247]\25\256s\211\2458\211e\226\316/\207i\226"
  "\321\35\24W{\247#Fd\210+\213V$m\225S\26\212\233S\17\243\245X\15\277\244"
  "X\13\306\202H\22\227P6\"p\222L\11\272\313x$\371\373\267_\377\372\304"
  "v\377\265`\14\370RA3X-=V>6H_8>Xr1Hg\213*Z\207\264\"\205f\231\314\36\212"
  "j\233\315\36\11U\177\242$Bg\213*ZXS\77uQ.]\177P\37v\211N\25\217\216M"
  "\17\240oC\32\2030-,`\202\30#.]\11\32&5V\37,;QcA%n\244W\14\275\260Y\3"
  "\362bM\77O>Xw1Gd\216+^\203\266#\217g\230\316\37\26k\232\320\40Wx\247"
  "&Eh\220,\203V0a\217S\34\202\227S\21\236\243W\15\275\244X\14\307\206J"
  "\22\236]=\36|\40'/bA1%q\252X\5\342\315\2021\371\356\264f\377\262Z\4\363"
  "\77ALG5Lf9=Vz2Eh\220,\\\206\261$k\232\320\40\220h\227\321!\23a\226\312"
  "\"Pv\241)\204\\3_\243Y\20\252\253[\14\312\253X\6\343\261Y\3\365\276i"
  "\27\375\367\2244\377\366\221/\377\351\227C\377\257Y\7\361#3BP%7KJ-@W"
  "CUOILeUIJFg\207/[|\252'\213l\231\322!\206i\226\315\"\24Yy\245(Ji\211"
  "0\212V(o\222T\31\214\232S\20\245\244X\15\300\243X\14\307\202H\24\232"
  "N8%s\222M\12\273\311s\34\371\373\273g\377\372\303t\377\264^\11\370SD"
  "9Z0D]\77:Uo9A_\2023Ks\234,`\220\300%\217g\230\316#\11W|\247)Fl\215/\\"
  "Z]BrU5a\200R$z\210O\30\222\215O\22\243pF\36\206223e\202\34*7a\11\36,"
  "=[$3EUcF)r\243X\15\277\260Y\3\362`RFSB^\1776Jo\2170_\214\262(\206j\232"
  "\317$\212h\227\320$\24]\202\256)Jj\2170~V7a\216U\37\203\225T\24\236\242"
  "W\16\274\245W\14\311\211L\23\242_\77!\200(+2f:1*p\250W\7\334\311|*\371"
  "\360\266j\377\262Z\5\365MIJR5No>\77[z8Gk\2162[\177\260*\217f\231\321"
  "%\202i\226\315&\23h\217\312'St\246.\206]:b\242Y\23\254\253[\15\313\253"
  "Y\7\344\261Y\3\365\276i\27\375\367\2244\377\366\221/\377\351\227C\377"
  "\257Z\10\361'3HT*:QN2D]G7Kk@=W{:Ne\2066\\y\255,\202i\226\315&\210g\230"
  "\316'\1h\227\320\24\206m\233\321\25\24[\204\255\31Jb\203\37""5Pk&.EV"
  ",)9N1%3E7\35-:=\33*6B\31$/F\"&*L\254W\5\341\341\244Z\376\303u#\367\240"
  "U\13\260$2\77""8)9N1.EV,5Pk&Lf\210\36h\226\305\26\217j\236\323\25\11"
  "[\204\255\31Jb\203\37""5Pk&.EV,-8Q2(2D8\40-=>\36)9C\35$3F\202\33\"0I"
  "\11\35$3F\36)9C\40-=>(1C949L43DZ-<Pr&Rb\213\37n\220\307\27\207o\231\314"
  "\26\211k\234\316\27\11^\204\263\33Md\213!:Uu'1H^.-<U3(5G9\40""0@\77\36"
  ")8D\34'5G\202\33%3J\11$+1K\235T\13\261\261X\2\376|I\33n,:S40F\\/9Rr("
  "Md\213!f\216\301\31\220h\227\317\30\202l\233\321\30\22R{\234\37<]\177"
  "&1Mc.,\77X4\"8I;\243S\7\276\336\2117\376\357\233J\377\321|)\372\260Y"
  "\5\361^<\33p\34+9G\37/>A'9J:-AZ33Of-<]\177&[\210\266\34\202l\233\321"
  "\30\217i\236\322\31\23U\177\262\36Fc\215$6Us*/Ed0)<S7!6K=\36.AB\34*<"
  "H\36(5L\"(/R\253W\4\342\337\236N\375\301r\35\367\237V\15\262&<M;/BU6"
  "5Jd0>]|)Rp\226\"\213m\231\314\32\205j\234\315\33\11Z\213\264\37Kn\220"
  "%;^v+4Nh1.EX7&\77P<\"6EB\40""2@G\37,:J\202\35+8M\11\37,:J\40""2@G\"6"
  "EB%:O=4ES94Nh1;^v+Mj\224$d\221\310\34\220g\230\317\33\11]\210\273\36"
  "Kg\220%<[\177*5Jj0.@X7)>S=&6IB#1CH\",=K\202\35*:N\11\40-:M\230S\16\252"
  "\261X\2\376\203N\33|-D[88Lk2>Zw-Ik\223&i\225\312\35\207k\232\320\34\213"
  "i\226\321\35\22Uq\242$A^\202+8Lp22D_8'7O@\243S\11\277\336\2117\376\357"
  "\233J\377\321|)\372\260Y\5\361]<!r\"/@K(6HF+;O@2H_8=Qp2Gd\210+a\207\264"
  "\"\212l\231\314\36\210p\237\317\14\21b\211\304\15\77Y\177\24""1DX\32"
  ")1J\37#*\77$\30%7)\27#5+\25%0/\25%/0X:\32J\261Z\4\363\231Q\13\215\30"
  "+>)\32.C&&6M!1Nb\32Fq\233\22\215i\226\322\15\205q\234\325\15\10m\221"
  "\310\16Ha\205\25""8K^\33/7O\40\"0D%\37+>)\34(9,\33%0/\202\32%/0\10\33"
  "%0/\34(9,\37+>)!.C&.6M!:Nb\32Uq\233\22q\234\325\15\222k\241\311\16\10"
  "f\231\273\17Eh\177\26""6H[\34.=M!'4G'#/A+!,7.\37)41\202\36(32\7\37)4"
  "1!,7.nE\36L'4G'+:P#\77Qd\34Yr\231\24\212s\231\314\17\212m\236\316\20"
  "\21]\206\256\23Dbu\32""6E\\!':N'F<.9\260W\0\364\264^\11\370\260W\1\350"
  "\221M\11\230\77-\"K\32#0:\34%36\36(82\".\77,(5P&7\77_\40Mc\220\27\222"
  "h\227\321\20\22o\233\323\21Qs\242\26\77[v\34""4Ca\"':U'\"4E,\37.>1\35"
  ",:4\33%77\33$68V8\40Q\260X\2\364\226Q\14\221\40""0A/#5G+(<W&7Gg\40Mn"
  "\233\27\204o\233\323\21\216j\237\312\22\21]\223\273\23Gp\216\31""7Og"
  "\40)DY%%>P)!7B.\37""4>1\36-<3\35,:4!+>5\",\77""4#3B2%6F/);M+.C]&\77O"
  "o\40Ut\237\30\216p\231\314\23\205l\235\316\23\10f\231\277\24Nu\223\32"
  "=Ul!.I]&*BU*&<H.#3B2\"1\77""4\202!0>5\10\"1\77""4#3B2lJ\"P)AM+.I]&\77"
  "Wo\40U\177\237\30l\235\316\23\223h\227\320\24\21X\205\261\27D]\177\36"
  "0Kg%.EV,I@;<\260W\0\364\264^\11\370\260X\1\350\220N\14\232>4)N\34-9>"
  "\"/<;%3F6)9N1/GX+7Rn%Uz\240\33\211m\233\321\25\202j\236\323\25\207\200"
  "\200\277\3\204f\231\314\4\14HHm\7..E\13$$6\16\34\34*\22\31\31&\24\27"
  "\27\"\26\30\30$\25^5\25!\37\37/\20'':\15\77\77_\10ff\231\5\207f\231\314"
  "\4\220\200\252\325\4\14Hmm\7.EE\13$66\16\34**\22\31&&\24\27\"\"\26\27"
  "\".\26\31&3\24\36-<\21$6H\16""8Uq\11U\177\252\6\221m\222\333\5\207\200"
  "\237\277\6\14Uqq\11:NN\15/\77\77\20&33\24\"..\26\37**\30!,,\27$00\25"
  "*88\22""3DD\17Lff\12m\221\221\7\203\200\237\277\6\224q\252\306\7\7_\237"
  "\237\10""3UU\17$<<\25\35':\32\31\"*\36\26\36%\"\25\34#$\202\24\33\"%"
  "\6\25\34#$\27\36&!\31\"*\36\36(=\31(5P\23E\\\213\13\212f\231\314\7\213"
  "t\242\321\10\7q\215\306\11Uj\224\14""8Fc\22,7M\27'1:\32#+4\35\40)1\37"
  "\202\37'/\40\6Q8\40,$-6\34(3G\31""3\77f\24H[\221\16f\177\314\12\224j"
  "\225\325\11\202v\235\330\12\14bu\234\15KZx\21\77Lf\24""7BX\27""3=G\31"
  "/8B\33""1:D\32""5\77U\30""9E\\\26CPk\23[m\221\16s\213\271\13\210v\235"
  "\330\12\220m\244\310\12\14Nu\234\15\77_\177\20""3Lf\24.E\\\26(=G\31%"
  "8B\33':D\32""3GQ\31""9Qh\26\77Yr\24Oo\217\20j\224\277\14\221f\231\314"
  "\13\206p\237\317\14\7b\211\304\15\77Y\177\24""3G[\31)1J\37$+A#\32':'"
  "\30%7)\202\30$6*\6\30%7)\32(<&$+A#*3L\36""7Mn\27Oo\237\20\202p\237\317"
  "\14\212i\226\322\15"};


