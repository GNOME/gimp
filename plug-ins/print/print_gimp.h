/*
 * "$Id$"
 *
 *   Print plug-in for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu). and Steve Miller (smiller@rni.net
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * Revision History:
 *
 *   See ChangeLog
 */

#ifndef __PRINT_GIMP_H__
#define __PRINT_GIMP_H__

#ifdef __GNUC__
#define inline __inline__
#endif


/*
 * All Gimp-specific code is in this file.
 */

#define PLUG_IN_VERSION		"4.2"
#define PLUG_IN_NAME		"Print"

typedef struct		/**** Printer List ****/
{
  int	active;			/* Do we know about this printer? */
  char	name[128];		/* Name of printer */
  stp_vars_t v;
} gp_plist_t;

#define THUMBNAIL_MAXW	(128)
#define THUMBNAIL_MAXH	(128)

extern gint    thumbnail_w, thumbnail_h, thumbnail_bpp;
extern guchar *thumbnail_data;
extern gint    adjusted_thumbnail_bpp;
extern guchar *adjusted_thumbnail_data;

extern stp_vars_t           vars;
extern gint             plist_count;	   /* Number of system printers */
extern gint             plist_current;     /* Current system printer */
extern gp_plist_t         *plist;		  /* System printers */
extern gint32           image_ID;
extern const gchar     *image_filename;
extern gint             image_width;
extern gint             image_height;
extern stp_printer_t current_printer;
extern gint             runme;
extern gint             saveme;

extern GtkWidget *gimp_color_adjust_dialog;
extern GtkWidget *dither_algo_combo;
extern stp_vars_t *pv;

/*
 * Function prototypes
 */

/* How to create an Image wrapping a Gimp drawable */
extern void  printrc_save (void);

extern stp_image_t *Image_GimpDrawable_new(GimpDrawable *drawable);
extern int add_printer(const gp_plist_t *key, int add_only);
extern void initialize_printer(gp_plist_t *printer);
extern void gimp_update_adjusted_thumbnail (void);
extern void gimp_plist_build_combo         (GtkWidget     *combo,
					    gint          num_items,
					    stp_param_t   *items,
					    const gchar   *cur_item,
					    const gchar	  *def_value,
					    GCallback      callback,
					    gint          *callback_id);

extern void gimp_invalidate_frame(void);
extern void gimp_invalidate_preview_thumbnail(void);
extern void gimp_do_color_updates    (void);
extern void gimp_redraw_color_swatch (void);
extern void gimp_build_dither_combo  (void);
extern void gimp_create_color_adjust_window  (void);
extern void gimp_update_adjusted_thumbnail   (void);
extern void gimp_create_main_window (void);
extern void gimp_set_color_sliders_active(int active);
extern void gimp_writefunc (void *file, const char *buf, size_t bytes);
extern void set_adjustment_tooltip(GtkObject *adjustment,
				   const gchar *tip, const gchar *private);

#endif  /* __PRINT_GIMP_H__ */
