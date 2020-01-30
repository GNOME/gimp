#include "config.h"

#include <string.h>
#include <math.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "lebl-dialog.h"

#include "gimp-intl.h"

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
        inv_draw_idle = 0;

        if (geginv)
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

	pb = gdk_pixbuf_new_from_resource ("/org/gimp/lebl-dialog/wanda.png",
                                           NULL);
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

	pb = gdk_pixbuf_new_from_resource ("/org/gimp/lebl-dialog/gegl-1.png",
                                           NULL);
	if (pb == NULL) {
		g_object_unref (G_OBJECT (inv_phsh1));
		g_object_unref (G_OBJECT (inv_phsh2));
		return FALSE;
	}

	inv_goat1 = pb_scale (pb, inv_factor * 0.66);
	g_object_unref (pb);

	pb = gdk_pixbuf_new_from_resource ("/org/gimp/lebl-dialog/gegl-2.png",
                                           NULL);
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
