/*
 * This is a plug-in for the GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-1999 Maurits Rijk  lpeek.mrijk@consunet.nl
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
 *
 */

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gdk/gdkkeysyms.h" /* for keyboard values */
#include "gtk/gtk.h"

#include "libgimp/gimp.h"
#include "libgimp/stdplugins-intl.h"

#include "imap_about.h"
#include "imap_circle.h"
#include "imap_cmd_clear.h"
#include "imap_cmd_copy.h"
#include "imap_cmd_cut.h"
#include "imap_cmd_create.h"
#include "imap_cmd_guides.h"
#include "imap_cmd_move.h"
#include "imap_cmd_move_down.h"
#include "imap_cmd_move_sash.h"
#include "imap_cmd_move_selected.h"
#include "imap_cmd_move_to_front.h"
#include "imap_cmd_move_up.h"
#include "imap_cmd_object_move.h"
#include "imap_cmd_paste.h"
#include "imap_cmd_select.h"
#include "imap_cmd_select_all.h"
#include "imap_cmd_select_next.h"
#include "imap_cmd_select_prev.h"
#include "imap_cmd_select_region.h"
#include "imap_cmd_send_to_back.h"
#include "imap_cmd_unselect.h"
#include "imap_cmd_unselect_all.h"
#include "imap_default_dialog.h"
#include "imap_edit_area_info.h"
#include "imap_file.h"
#include "imap_grid.h"
#include "imap_main.h"
#include "imap_menu.h"
#include "imap_object.h"
#include "imap_polygon.h"
#include "imap_popup.h"
#include "imap_preview.h"
#include "imap_rectangle.h"
#include "imap_selection.h"
#include "imap_settings.h"
#include "imap_source.h"
#include "imap_statusbar.h"
#include "imap_string.h"
#include "imap_toolbar.h"
#include "imap_tools.h"

#define MAX_ZOOM_FACTOR 8
#define ZOOMED(x) (_zoom_factor * (x))
#define GET_REAL_COORD(x) ((x) / _zoom_factor)

/* Global variables */
static MapInfo_t   _map_info;
static PreferencesData_t _preferences = {CSIM, TRUE, FALSE, TRUE, TRUE, FALSE,
FALSE, DEFAULT_UNDO_LEVELS, DEFAULT_MRU_SIZE};
static MRU_t *_mru;

static GdkCursorType _cursor;
static gboolean	    _show_url = TRUE;
static gchar	   *_filename = NULL;
static char	   *_image_name;
static gint	   _image_width;
static gint	   _image_height;
static GtkWidget   *_dlg;
static Preview_t   *_preview;
static Selection_t *_selection;
static StatusBar_t *_statusbar;
static ToolBar_t   *_toolbar;
static ObjectList_t *_shapes;
static gint	    _zoom_factor = 1;
static void (*_button_press_func)(GtkWidget*, GdkEventButton*, gpointer);
static gpointer _button_press_param;

/* Declare local functions. */
static void query (void);
static void run (char *name,
		 int nparams,
		 GParam * param,
		 int *nreturn_vals,
		 GParam ** return_vals);
static gint dialog(GDrawable *drawable);

GPlugInInfo PLUG_IN_INFO = {
   NULL,			/* init_proc */
   NULL,			/* quit_proc */
   query,			/* query_proc */
   run,				/* run_proc */
};

static int run_flag = 0;


MAIN ()

static void query()
{
   static GParamDef args[] = {
      {PARAM_INT32, "run_mode", "Interactive"},
      {PARAM_IMAGE, "image", "Input image (unused)"},
      {PARAM_DRAWABLE, "drawable", "Input drawable"},
   };
   static GParamDef *return_vals = NULL;
   static int nargs = sizeof (args) / sizeof (args[0]);
   static int nreturn_vals = 0;
   
   INIT_I18N();

   gimp_install_procedure("plug_in_imagemap",
			  _("Creates a clickable imagemap."),
			  "",
			  "Maurits Rijk",
			  "Maurits Rijk",
			  "1998-1999",
			  N_("<Image>/Filters/Web/ImageMap..."),
			  "RGB*, GRAY*, INDEXED*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
run(char *name, int n_params, GParam *param, int *nreturn_vals,
    GParam **return_vals)
{
   static GParam values[1];
   GDrawable *drawable;
   GRunModeType run_mode;
   GStatusType status = STATUS_SUCCESS;

   INIT_I18N_UI();

   *nreturn_vals = 1;
   *return_vals = values;
   
   /*  Get the specified drawable  */
   drawable = gimp_drawable_get(param[2].data.d_drawable);
   _image_name = gimp_image_get_filename(param[1].data.d_image);
   _image_width = gimp_image_width(param[1].data.d_image);
   _image_height = gimp_image_height(param[1].data.d_image);

   _map_info.color = gimp_drawable_is_rgb(drawable->id);

   run_mode = (GRunModeType) param[0].data.d_int32;
   
   if (run_mode == RUN_INTERACTIVE) {
      if (!dialog(drawable)) {
	 /* The dialog was closed, or something similarly evil happened. */
	 status = STATUS_EXECUTION_ERROR;
      }
   }
      
   if (status == STATUS_SUCCESS) {
      gimp_drawable_detach(drawable);
   }
   
   values[0].type = PARAM_STATUS;
   values[0].data.d_status = status;
}

MRU_t*
get_mru(void)
{
   if (!_mru)
      _mru = mru_create();
   return _mru;
}

MapInfo_t*
get_map_info(void)
{
   return &_map_info;
}

PreferencesData_t*
get_preferences(void)
{
   return &_preferences;
}

static void
init_preferences(void)
{
   GdkColormap *colormap = gdk_window_get_colormap(_dlg->window);
   ColorSelData_t *colors = &_preferences.colors;

   colors->normal_fg.red = 0;
   colors->normal_fg.green = 0xFFFF;
   colors->normal_fg.blue = 0;
   
   colors->normal_bg.red = 0;
   colors->normal_bg.green = 0;
   colors->normal_bg.blue = 0xFFFF;
   
   colors->selected_fg.red = 0xFFFF;
   colors->selected_fg.green = 0;
   colors->selected_fg.blue = 0;
   
   colors->selected_bg.red = 0;
   colors->selected_bg.green = 0;
   colors->selected_bg.blue = 0xFFFF;
   
   preferences_load(&_preferences);

   gdk_color_alloc(colormap, &colors->normal_fg);
   gdk_color_alloc(colormap, &colors->normal_bg);
   gdk_color_alloc(colormap, &colors->selected_fg);
   gdk_color_alloc(colormap, &colors->selected_bg);

   _preferences.normal_gc = gdk_gc_new(_preview->preview->window);
   _preferences.selected_gc = gdk_gc_new(_preview->preview->window);

   gdk_gc_set_line_attributes(_preferences.normal_gc, 1, GDK_LINE_DOUBLE_DASH,
			      GDK_CAP_BUTT, GDK_JOIN_BEVEL);
   gdk_gc_set_line_attributes(_preferences.selected_gc, 1, 
			      GDK_LINE_DOUBLE_DASH, GDK_CAP_BUTT, 
			      GDK_JOIN_BEVEL);
   
   gdk_gc_set_foreground(_preferences.normal_gc, &colors->normal_fg);
   gdk_gc_set_background(_preferences.normal_gc, &colors->normal_bg);
   gdk_gc_set_foreground(_preferences.selected_gc, &colors->selected_fg);
   gdk_gc_set_background(_preferences.selected_gc, &colors->selected_bg);

   mru_set_size(_mru, _preferences.mru_size);
   command_list_set_undo_level(_preferences.undo_levels);
}

/* Get yellow for tooltips */
GdkColor*
get_yellow(void)
{
   static GdkColor *yellow;

   if (!yellow) {
      static GdkColor color;
      GdkColormap *colormap = gdk_window_get_colormap(_dlg->window);

      color.red = 61669;
      color.green = 59113;
      color.blue = 35979;
      gdk_color_alloc(colormap, &color);
      yellow = &color;
   }
   return yellow;
}

gint
get_image_width(void)
{
   return _image_width;
}

gint
get_image_height(void)
{
   return _image_height;
}

void 
set_busy_cursor(void)
{
   preview_set_cursor(_preview, GDK_WATCH);
}

void 
remove_busy_cursor(void)
{
   gdk_window_set_cursor(_dlg->window, NULL);
}

static gint
zoom_in(void)
{
   if (_zoom_factor < MAX_ZOOM_FACTOR) {
      set_zoom(_zoom_factor + 1);
      menu_set_zoom(_zoom_factor);
   }
   return _zoom_factor;
}

static gint
zoom_out(void)
{
   if (_zoom_factor > 1) {
      set_zoom(_zoom_factor - 1);
      menu_set_zoom(_zoom_factor);
   }
   return _zoom_factor;
}

void
set_zoom(gint zoom_factor)
{
   set_busy_cursor();
   _zoom_factor = zoom_factor;
   toolbar_set_zoom_sensitivity(_toolbar, zoom_factor);
   preview_zoom(_preview, zoom_factor);
   statusbar_set_zoom(_statusbar, zoom_factor);
   remove_busy_cursor();
}

void
main_toolbar_set_grid(gboolean active)
{
   toolbar_set_grid(_toolbar, active);
}

gint
get_real_coord(gint coord)
{
   return GET_REAL_COORD(coord);
}

void
draw_line(GdkWindow *window, GdkGC *gc, gint x1, gint y1, gint x2, gint y2)
{
   gdk_draw_line(window, gc, ZOOMED(x1), ZOOMED(y1), ZOOMED(x2), ZOOMED(y2));
}

void
draw_rectangle(GdkWindow *window, GdkGC	*gc, gint filled, gint x, gint y,
	       gint width, gint	height)
{
   gdk_draw_rectangle(window, gc, filled, ZOOMED(x), ZOOMED(y),
		      ZOOMED(width), ZOOMED(height));
}

void
draw_arc(GdkWindow *window, GdkGC *gc, gint filled, gint x, gint y,
	 gint width, gint height, gint angle1, gint angle2)
{
   gdk_draw_arc(window, gc, filled, ZOOMED(x), ZOOMED(y),
		ZOOMED(width), ZOOMED(height), angle1, angle2);
}

void
draw_circle(GdkWindow *window, GdkGC *gc, gint filled, gint x, gint y, gint r)
{
   draw_arc(window, gc, filled, x - r, y - r, 2 * r, 2 * r, 0, 360 * 64);
}

void
draw_polygon(GdkWindow *window, GdkGC *gc, GList *list)
{
   gint       npoints = g_list_length(list);
   GdkPoint  *points = g_new(GdkPoint, npoints);
   GdkPoint  *des = points;
   GList     *p;

   for (p = list; p; p = p->next, des++) {
      GdkPoint *src = (GdkPoint*) p->data;
      des->x = ZOOMED(src->x);
      des->y = ZOOMED(src->y);
   }
   gdk_draw_polygon(window, gc, FALSE, points, npoints);
   g_free(points);
}

static gboolean _preview_redraw_blocked = FALSE;
static gboolean _pending_redraw = FALSE;

void
preview_freeze(void)
{
   _preview_redraw_blocked = TRUE;
}

void
preview_thaw(void)
{
   _preview_redraw_blocked = FALSE;
   if (_pending_redraw) {
      _pending_redraw = FALSE;
      redraw_preview();
   }
}

void 
redraw_preview(void)
{
   if (_preview_redraw_blocked)
      _pending_redraw = TRUE;
   else
      preview_redraw(_preview);
}

static void
set_preview_gray(void)
{
   _map_info.show_gray = TRUE;
   set_zoom(_zoom_factor);
}

static void
set_preview_color(void)
{
   _map_info.show_gray = FALSE;
   set_zoom(_zoom_factor);
}

const char*
get_image_name(void)
{
   return _image_name;
}

const char*
get_filename(void)
{
   return _filename;
}

void
set_arrow_func(void)
{
   _button_press_func = arrow_on_button_press;
   _cursor = GDK_TOP_LEFT_ARROW;
}

static void 
set_object_func(void (*func)(GtkWidget*, GdkEventButton*, 
			     gpointer), gpointer param)
{
   _button_press_func = func;
   _button_press_param = param;
   _cursor = GDK_CROSSHAIR;
}

void
set_rectangle_func(void)
{
   set_object_func(object_on_button_press, get_rectangle_factory);
}

void
set_circle_func(void)
{
   set_object_func(object_on_button_press, get_circle_factory);
}

void
set_polygon_func(void)
{
   set_object_func(object_on_button_press, get_polygon_factory);
}

void
add_shape(Object_t *obj)
{
   object_list_append(_shapes, obj);
}

ObjectList_t*
get_shapes(void)
{
   return _shapes;
}

void
update_shape(Object_t *obj)
{
   object_list_update(_shapes, obj);
}

static void
edit_selected_shape(void)
{
   object_list_edit_selected(_shapes);
}

void
do_popup_menu(GdkEventButton *event)
{
   gint x = GET_REAL_COORD((gint) event->x);
   gint y = GET_REAL_COORD((gint) event->y);
   Object_t *obj = object_list_find(_shapes, x, y);
   if (obj) {
      obj->class->do_popup(obj, event);
   } else {
      do_main_popup_menu(event);
   }
}

static void
set_all_sensitivities(void)
{
   gint count = object_list_nr_selected(_shapes);
   menu_shapes_selected(count);
   toolbar_shapes_selected(_toolbar, count);
   tools_set_sensitive(count);
}

static void
main_set_title(const char *filename)
{
   char title[256];
   char *p;
   
   g_strreplace(&_filename, filename);
   p = (filename) ? g_basename(filename) : _("<Untitled>");
   sprintf(title, "%s - ImageMap 1.3", p);
   gtk_window_set_title(GTK_WINDOW(_dlg), title);
}

void 
main_set_dimension(gint width, gint height)
{
   statusbar_set_dimension(_statusbar, width / _zoom_factor, 
			   height / _zoom_factor);
}

void
main_clear_dimension(void)
{
   statusbar_clear_dimension(_statusbar);
}

void
show_url(void)
{
   _show_url = TRUE;
}

void
hide_url(void)
{
   _show_url = FALSE;
   statusbar_clear_status(_statusbar);
}

void 
select_shape(GtkWidget *widget, GdkEventButton *event)
{
   Object_t *obj;
   gint x = GET_REAL_COORD((gint) event->x);
   gint y = GET_REAL_COORD((gint) event->y);
   MoveSashFunc_t sash_func;

   obj = object_list_near_sash(_shapes, x, y, &sash_func);
   if (obj) {			/* Start resizing */
      Command_t *command = move_sash_command_new(widget, obj, x, y, sash_func);
      command_execute(command);
   } else {
      Command_t *command;

      obj = object_list_find(_shapes, x, y);
      if (obj) {
	 if (event->state & GDK_SHIFT_MASK) {
	    if (obj->selected)
	       command = unselect_command_new(obj);
	    else
	       command = select_command_new(obj);
	 } else {		/* No Shift key pressed */
	    if (obj->selected) {
	       command = unselect_all_command_new(_shapes, obj);
	    } else {
	       Command_t *sub_command;

	       command = subcommand_start(NULL);
	       sub_command = unselect_all_command_new(_shapes, NULL);
	       command_add_subcommand(command, sub_command);
	       sub_command = select_command_new(obj);
	       command_add_subcommand(command, sub_command);
	       command_set_name(command, sub_command->name);
	       subcommand_end();
	    }
	 }
	 command_execute(command);

	 command = move_command_new(_preview, obj, x, y);
	 command_execute(command);
      } else { /* Start selection rectangle */
	 command = select_region_command_new(widget, _shapes, x, y);
	 command_execute(command);
      }
   }
}

void
edit_shape(gint x, gint y)
{
   Object_t *obj;

   x = GET_REAL_COORD(x);
   y = GET_REAL_COORD(y);

   obj = object_list_find(_shapes, x, y);
   if (obj) {
      object_select(obj);
      object_edit(obj, TRUE);
   }
}

static void
toolbar_grid(void)
{
   gint grid = toggle_grid();
   popup_check_grid(grid);
   menu_check_grid(grid);
}

static void
menu_zoom_in(void)
{
   gint factor = zoom_in();
   menu_set_zoom_sensitivity(factor);
   popup_set_zoom_sensitivity(factor);
}

static void
menu_zoom_out(void)
{
   gint factor = zoom_out();
   menu_set_zoom_sensitivity(factor);
   popup_set_zoom_sensitivity(factor);
}

void 
draw_shapes(GtkWidget *preview)
{
   if (!_preview_redraw_blocked)
      object_list_draw(_shapes, preview->window);
}

static void
clear_map_info(void)
{
   gchar *author = g_get_real_name();

   if (!*author)
      author = g_get_user_name();
   g_strreplace(&_map_info.image_name, _image_name);
   g_strreplace(&_map_info.title, "map");
   g_strreplace(&_map_info.author, author);
   g_strreplace(&_map_info.default_url, "");
   g_strreplace(&_map_info.description, "");

   _map_info.map_format = CSIM;
   _map_info.show_gray = FALSE;
}

static void
do_data_changed_dialog(void (*continue_cb)(gpointer), gpointer param)
{
   static DefaultDialog_t *dialog;

   if (!dialog) {
      dialog = make_default_dialog(_("Data changed"));
      default_dialog_hide_apply_button(dialog);
      default_dialog_set_label(
	 dialog,
	 _("   Some data has been changed.   \n"
	   "Do you really want to continue?"));
   }
   default_dialog_set_ok_cb(dialog, continue_cb, param);
   default_dialog_show(dialog);
}

static void
check_if_changed(void (*func)(gpointer), gpointer param)
{
   if (object_list_get_changed(_shapes))
      do_data_changed_dialog(func, param);
   else
      func(param);
}

static void
close_current(void)
{
   selection_freeze(_selection);
   object_list_remove_all(_shapes);
   selection_thaw(_selection);
   clear_map_info();
   main_set_title(NULL);
   set_all_sensitivities();
   redraw_preview();
   object_list_clear_changed(_shapes);
   command_list_remove_all();
}

static void
really_close(gpointer data)
{
   close_current();
}

static void
do_close(void)
{
   check_if_changed(really_close, NULL);
}

static void
really_quit(gpointer data)
{
   preferences_save(&_preferences);
   run_flag = 1;
   gtk_widget_destroy(_dlg);
}

static void
do_quit(void)
{
   check_if_changed(really_quit, NULL);
}

static void
do_undo(void)
{
   preview_freeze();
   selection_freeze(_selection);
   last_command_undo();
   selection_thaw(_selection);
   preview_thaw();
}

static void
do_redo(void)
{
   preview_freeze();
   selection_freeze(_selection);
   last_command_redo();
   selection_thaw(_selection);
   preview_thaw();
}

static void
save(void)
{
   if (_filename)
      save_as(_filename);
   else
      do_file_save_as_dialog();
}

static void
write_cern_comment(gpointer param, OutputFunc_t output)
{
   output(param, "rect (4096,4096) (4096,4096) imap:#$");   
}

static void
save_as_cern(gpointer param, OutputFunc_t output)
{
   char *p;
   gchar *description;
   gchar *next_token;

   write_cern_comment(param, output);
   output(param, "-:Image Map file created by GIMP Imagemap Plugin\n");
   write_cern_comment(param, output);
   output(param, "-:GIMP Imagemap Plugin by Maurits Rijk\n");
   write_cern_comment(param, output);
   output(param, "-:Please do not edit lines starting with \"#$\"\n");
   write_cern_comment(param, output);
   output(param, "VERSION:1.3\n");
   write_cern_comment(param, output);
   output(param, "TITLE:%s\n", _map_info.title);
   write_cern_comment(param, output);
   output(param, "AUTHOR:%s\n", _map_info.author);
   write_cern_comment(param, output);
   output(param, "FORMAT:cern\n");
   
   description = g_strdup(_map_info.description);
   next_token = description;
   for (p = strtok (next_token, "\n"); p; p = strtok(NULL, "\n")) {
      write_cern_comment(param, output);
      output(param, "DESCRIPTION:%s\n", p);
   }
   g_free(description);

   if (*_map_info.default_url)
      output(param, "default %s\n", _map_info.default_url);
   object_list_write_cern(_shapes, param, output);
}

static void
save_as_csim(gpointer param, OutputFunc_t output)
{
   char *p;
   gchar *description;
   
   output(param, "<IMG SRC=\"%s\" WIDTH=%d HEIGHT=%d BORDER=0 "
	  "USEMAP=\"#%s\">\n\n", _map_info.image_name,
	  _image_width, _image_height, _map_info.title);
   output(param, "<MAP NAME=\"%s\">\n", _map_info.title);
   output(param, 
	  "<!-- #$-:Image Map file created by GIMP Imagemap Plugin -->\n");
   output(param, "<!-- #$-:GIMP Imagemap Plugin by Maurits Rijk -->\n");
   output(param, 
	  "<!-- #$-:Please do not edit lines starting with \"#$\" -->\n");
   output(param, "<!-- #$VERSION:1.3 -->\n");
   output(param, "<!-- #$AUTHOR:%s -->\n", _map_info.author);
   
   description = g_strdup(_map_info.description);
   for (p = strtok(description, "\n"); p; p = strtok(NULL, "\n"))
      output(param, "<!-- #$DESCRIPTION:%s -->\n", p);
   g_free(description);
   
   object_list_write_csim(_shapes, param, output);
   if (*_map_info.default_url)
      output(param, "<AREA SHAPE=\"DEFAULT\" HREF=\"%s\">\n",
	     _map_info.default_url);
   output(param, "</MAP>\n");
}

static void
save_as_ncsa(gpointer param, OutputFunc_t output)
{
   char *p;
   gchar *description;

   output(param, "#$-:Image Map file created by GIMP Imagemap Plugin\n");
   output(param, "#$-:GIMP Imagemap Plugin by Maurits Rijk\n");
   output(param, "#$-:Please do not edit lines starting with \"#$\"\n");
   output(param, "#$VERSION:1.3\n");
   output(param, "#$TITLE:%s\n", _map_info.title);
   output(param, "#$AUTHOR:%s\n", _map_info.author);
   output(param, "#$FORMAT:ncsa\n");
   
   description = g_strdup(_map_info.description);
   for (p = strtok(description, "\n"); p; p = strtok(NULL, "\n"))
      output(param, "#$DESCRIPTION:%s\n", p);
   g_free(description);

   if (*_map_info.default_url)
      output(param, "default %s\n", _map_info.default_url);
   object_list_write_ncsa(_shapes, param, output);
}

static void 
save_to_file(gpointer param, const char* format, ...)
{
   va_list ap;

   va_start(ap, format);
   vfprintf((FILE*)param, format, ap);
   va_end(ap);
}

void
dump_output(gpointer param, OutputFunc_t output)
{
   if (_map_info.map_format == NCSA)
      save_as_ncsa(param, output);
   else if (_map_info.map_format == CERN)
      save_as_cern(param, output);
   else if (_map_info.map_format == CSIM)
      save_as_csim(param, output);
}

void
save_as(const gchar *filename)
{
   FILE *out = fopen(filename, "w");
   if (out) {
      dump_output(out, save_to_file);
      fclose(out);

      statusbar_set_status(_statusbar, "File \"%s\" saved.", filename);
      main_set_title(filename);
      object_list_clear_changed(_shapes);
   } else {
      do_file_error_dialog("Couldn't save file:", filename);
   }
}

static void
resize_image_ok_cb(gpointer data)
{
   gint per_x = _image_width * 100 / _map_info.old_image_width;
   gint per_y = _image_height * 100 / _map_info.old_image_height;
   object_list_resize(_shapes, per_x, per_y);
   preview_thaw();
}

static void
resize_image_cancel_cb(gpointer data)
{
   preview_thaw();
}

static void
do_image_size_changed_dialog(void)
{
   static DefaultDialog_t *dialog;

   if (!dialog) {
      dialog = make_default_dialog("Image size changed");
      default_dialog_hide_apply_button(dialog);
      default_dialog_set_label(
	 dialog,
	 "   Image size has changed.   \n"
	 "Resize Area's?");

      default_dialog_set_ok_cb(dialog, resize_image_ok_cb, NULL);
      default_dialog_set_cancel_cb(dialog, resize_image_cancel_cb, NULL);
   }
   default_dialog_show(dialog);

}

static void
really_load(gpointer data)
{
   gchar *filename = (gchar*) data;
   close_current();

   selection_freeze(_selection);
   _map_info.old_image_width = _image_width;
   _map_info.old_image_height = _image_height;
   if (load_csim(filename)) {
      _map_info.map_format = CSIM;
      if (_image_width != _map_info.old_image_width ||
	  _image_height != _map_info.old_image_height) {
	 preview_freeze();
	 do_image_size_changed_dialog();
      }
   } else if (load_ncsa(filename)) {
      _map_info.map_format = NCSA;
   } else if (load_cern(filename)) {
      _map_info.map_format = CERN;
   } else {
      do_file_error_dialog("Couldn't read file:", filename);
      selection_thaw(_selection);
      close_current();
      return;
   }
   mru_set_first(_mru, filename);
   menu_build_mru_items(_mru);

   selection_thaw(_selection);
   main_set_title(filename);
   object_list_clear_changed(_shapes);
   redraw_preview();
}

void
load(const gchar *filename)
{
   static gchar *tmp_filename;
   g_strreplace(&tmp_filename, filename);
   check_if_changed(really_load, (gpointer) tmp_filename);
}

static void 
toggle_area_list(void)
{
   selection_toggle_visibility(_selection);
}

static void
close_callback(GtkWidget *widget, gpointer data)
{
   gtk_main_quit();
}

static void
preview_move(GtkWidget *widget, GdkEventMotion *event)
{
   gint x = GET_REAL_COORD((gint) event->x);
   gint y = GET_REAL_COORD((gint) event->y);
   static Object_t *prev_obj = NULL;
   Object_t *obj = object_list_find(_shapes, x, y);

   statusbar_set_xy(_statusbar, x, y);
   if (obj != prev_obj) {
      prev_obj = obj;
      if (obj && _show_url) {
	 statusbar_set_status(_statusbar, " URL: %s", obj->url);
      } else {
	 statusbar_clear_status(_statusbar);
      }
   }
#ifdef _NOT_READY_YET_
   if (!obj) {
      if (grid_near_x(x)) {
	 preview_set_cursor(_preview, GDK_SB_H_DOUBLE_ARROW);
      } else if (grid_near_y(y)) {
	 preview_set_cursor(_preview, GDK_SB_V_DOUBLE_ARROW);
      } else {
	 preview_set_cursor(_preview, _cursor);
      }
   }
#endif
}

static void
preview_enter(GtkWidget *widget, GdkEventCrossing *event)
{
   preview_set_cursor(_preview, _cursor);
}

static void
preview_leave(GtkWidget *widget, GdkEventCrossing *event)
{
   gdk_window_set_cursor(_dlg->window, NULL);
   statusbar_clear_xy(_statusbar);
}

static void 
button_press(GtkWidget* widget, GdkEventButton* event, gpointer data)
{
   _button_press_func(widget, event, _button_press_param);
}

/* A few global vars for key movement */

static gint _timeout;
static guint _keyval;
static gint _dx, _dy;

static void
move_selected_objects(gint dx, gint dy, gboolean fast)
{
   if (fast) {
      dx *= 5;
      dy *= 5;
   }
   _dx += dx;
   _dy += dy;

   gdk_gc_set_function(_preferences.normal_gc, GDK_EQUIV);
   gdk_gc_set_function(_preferences.selected_gc, GDK_EQUIV);
   object_list_draw_selected(_shapes, _preview->preview->window);
   object_list_move_selected(_shapes, dx, dy);
   object_list_draw_selected(_shapes, _preview->preview->window);
   gdk_gc_set_function(_preferences.normal_gc, GDK_COPY);
   gdk_gc_set_function(_preferences.selected_gc, GDK_COPY);
}

static gboolean
key_timeout_cb(gpointer data)
{
   switch (_keyval) {
   case GDK_Left:
   case GDK_Right:
   case GDK_Up:
   case GDK_Down:
      command_list_add(move_selected_command_new(_shapes, _dx, _dy));
      _dx = _dy = 0;
      break;
   }
   preview_thaw();
   return FALSE;
}

static gboolean 
key_press_cb(GtkWidget *widget, GdkEventKey *event)
{
   gint handled = FALSE;
   gboolean shift = event->state & GDK_SHIFT_MASK;
   Command_t *command;

   preview_freeze();
   if (_timeout)
      gtk_timeout_remove(_timeout);

   switch (event->keyval) {
   case GDK_Left:
      move_selected_objects(-1, 0, shift);
      handled = TRUE;
      break;
   case GDK_Right:
      move_selected_objects(1, 0, shift);
      handled = TRUE;
      break;
   case GDK_Up:
      move_selected_objects(0, -1, shift);
      handled = TRUE;
      break;
   case GDK_Down:
      move_selected_objects(0, 1, shift);
      handled = TRUE;
      break;
   case GDK_Tab:
      if (shift)
	 command = select_prev_command_new(_shapes);
      else
	 command = select_next_command_new(_shapes);
      command_execute(command);
      handled = TRUE;
      break;
   }
   if (handled)
      gtk_signal_emit_stop_by_name(GTK_OBJECT(widget), "key_press_event");

   return handled;
}

static gboolean 
key_release_cb(GtkWidget *widget, GdkEventKey *event)
{
   _keyval = event->keyval;
   _timeout = gtk_timeout_add(250, key_timeout_cb, NULL);
   return FALSE;
}

static void
geometry_changed(Object_t *obj, gpointer data)
{
   redraw_preview();		/* Fix me! */
}

static void
data_changed(Object_t *obj, gpointer data)
{
   set_all_sensitivities();
}

static void
data_selected(Object_t *obj, gpointer data)
{
   set_all_sensitivities();
}

static Command_t*
factory_file_open_dialog(void)
{
   return command_new(do_file_open_dialog);
}

static Command_t*
factory_save(void)
{
   return command_new(save);
}

static Command_t*
factory_save_as(void)
{
   return command_new(do_file_save_as_dialog);
}

static Command_t*
factory_preferences_dialog(void)
{
   return command_new(do_preferences_dialog);
}

static Command_t*
factory_close(void)
{
   return command_new(do_close);
}

static Command_t*
factory_quit(void)
{
   return command_new(do_quit);
}


static Command_t*
factory_undo(void)
{
   return command_new(do_undo);
}

static Command_t*
factory_redo(void)
{
   return command_new(do_redo);
}

static Command_t*
factory_cut(void)
{
   return cut_command_new(_shapes);
}

static Command_t*
factory_copy(void)
{
   return copy_command_new(_shapes);
}

static Command_t*
factory_paste(void)
{
   return paste_command_new(_shapes);
}

static Command_t*
factory_select_all(void)
{
   return select_all_command_new(_shapes);
}

static Command_t*
factory_clear(void)
{
   return clear_command_new(_shapes);
}

static Command_t*
factory_edit(void)
{
   return command_new(edit_selected_shape);
}

static Command_t*
factory_toggle_area_list(void)
{
   return command_new(toggle_area_list);
}

static Command_t*
factory_source_dialog(void)
{
   return command_new(do_source_dialog);
}

static Command_t*
factory_preview_color(void)
{
   return command_new(set_preview_color);
}

static Command_t*
factory_preview_gray(void)
{
   return command_new(set_preview_gray);
}

static Command_t*
factory_menu_zoom_in(void)
{
   return command_new(menu_zoom_in);
}

static Command_t*
factory_menu_zoom_out(void)
{
   return command_new(menu_zoom_out);
}

static Command_t*
factory_zoom_in(void)
{
   return command_new(zoom_in);
}

static Command_t*
factory_zoom_out(void)
{
   return command_new(zoom_out);
}

static Command_t*
factory_settings_dialog(void)
{
   return command_new(do_settings_dialog);
}

static Command_t*
factory_move_to_front(void)
{
   return move_to_front_command_new(_shapes);
}

static Command_t*
factory_send_to_back(void)
{
   return send_to_back_command_new(_shapes);
}

static Command_t*
factory_toolbar_grid(void)
{
   return command_new(toolbar_grid);
}

static Command_t*
factory_grid_settings_dialog(void)
{
   return command_new(do_grid_settings_dialog);
}

static Command_t*
factory_create_guides_dialog(void)
{
   return guides_command_new(_shapes);
}

static Command_t*
factory_about_dialog(void)
{
   return command_new(do_about_dialog);
}

static Command_t*
factory_move_up(void)
{
   return move_up_command_new(_shapes);
}

static Command_t*
factory_move_down(void)
{
   return move_down_command_new(_shapes);
}

static gint
dialog(GDrawable *drawable)
{
   GtkWidget 	*dlg;
   GtkWidget 	*hbox;
   GtkWidget 	*main_vbox;
   Tools_t	*tools;
   gchar 	**argv;
   gint 	argc = 1;
   Menu_t	*menu;
   PopupMenu_t  *popup;

   argv = g_new(gchar *, 1);
   argv[0] = g_strdup("imagemap");
   
   gtk_init(&argc, &argv);
   gtk_rc_parse(gimp_gtkrc());

   gdk_set_use_xshm(gimp_use_xshm());
   gtk_preview_set_gamma(gimp_gamma());
   
   gtk_widget_set_default_visual(gtk_preview_get_visual());
   gtk_widget_set_default_colormap(gdk_rgb_get_cmap());

   _shapes = make_object_list();

   _dlg = dlg = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_policy(GTK_WINDOW(dlg), TRUE, TRUE, FALSE);
   gtk_widget_realize(dlg);

   main_set_title(NULL);

   gtk_window_set_position(GTK_WINDOW(dlg), GTK_WIN_POS_MOUSE);
   gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
		      (GtkSignalFunc) close_callback, NULL);
   gtk_signal_connect(GTK_OBJECT(dlg), "key_press_event", 
		      (GtkSignalFunc) key_press_cb, NULL);
   gtk_signal_connect(GTK_OBJECT(dlg), "key_release_event", 
		      (GtkSignalFunc) key_release_cb, NULL);

   main_vbox = gtk_vbox_new(FALSE, 1);
   gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 1);
   gtk_container_add(GTK_CONTAINER(dlg), main_vbox);
   gtk_widget_show(main_vbox);

   /* Create menu */
   menu = make_menu(main_vbox, dlg);
   menu_set_open_command(menu, factory_file_open_dialog);
   menu_set_save_command(menu, factory_save);
   menu_set_save_as_command(menu, factory_save_as);
   menu_set_preferences_command(menu, factory_preferences_dialog);
   menu_set_close_command(menu, factory_close);
   menu_set_quit_command(menu, factory_quit);
   menu_set_undo_command(menu, factory_undo);
   menu_set_redo_command(menu, factory_redo);
   menu_set_cut_command(menu, factory_cut);
   menu_set_copy_command(menu, factory_copy);
   menu_set_paste_command(menu, factory_paste);
   menu_set_select_all_command(menu, factory_select_all);
   menu_set_clear_command(menu, factory_clear);
   menu_set_edit_erea_info_command(menu, factory_edit);
   menu_set_area_list_command(menu, factory_toggle_area_list);
   menu_set_source_command(menu, factory_source_dialog);
   menu_set_color_command(menu, factory_preview_color);
   menu_set_gray_command(menu, factory_preview_gray);
   menu_set_zoom_in_command(menu, factory_menu_zoom_in);
   menu_set_zoom_out_command(menu, factory_menu_zoom_out);
   menu_set_edit_map_info_command(menu, factory_settings_dialog);
   menu_set_grid_settings_command(menu, factory_grid_settings_dialog);
   menu_set_create_guides_command(menu, factory_create_guides_dialog);
   menu_set_about_command(menu, factory_about_dialog);

   /* Create popup */
   popup = create_main_popup_menu();
   popup_set_zoom_in_command(popup, factory_zoom_in);
   popup_set_zoom_out_command(popup, factory_zoom_out);
   popup_set_edit_map_info_command(popup, factory_settings_dialog);
   popup_set_grid_settings_command(popup, factory_grid_settings_dialog);
   popup_set_create_guides_command(popup, factory_create_guides_dialog);
   popup_set_paste_command(popup, factory_paste);

   /* Create toolbar */
   _toolbar = make_toolbar(main_vbox, dlg);
   toolbar_set_open_command(_toolbar, factory_file_open_dialog);
   toolbar_set_save_command(_toolbar, factory_save);
   toolbar_set_preferences_command(_toolbar, factory_preferences_dialog);
   toolbar_set_undo_command(_toolbar, factory_undo);
   toolbar_set_redo_command(_toolbar, factory_redo);
   toolbar_set_cut_command(_toolbar, factory_cut);
   toolbar_set_copy_command(_toolbar, factory_copy);
   toolbar_set_paste_command(_toolbar, factory_paste);
   toolbar_set_zoom_in_command(_toolbar, factory_zoom_in);
   toolbar_set_zoom_out_command(_toolbar, factory_zoom_out);
   toolbar_set_edit_map_info_command(_toolbar, factory_settings_dialog);
   toolbar_set_move_to_front_command(_toolbar, factory_move_to_front);
   toolbar_set_send_to_back_command(_toolbar, factory_send_to_back);
   toolbar_set_grid_command(_toolbar, factory_toolbar_grid);

   /*  Dialog area  */
   hbox = gtk_hbox_new(FALSE, 1);
   gtk_container_add(GTK_CONTAINER(main_vbox), hbox);
   gtk_widget_show(hbox);

   tools = make_tools(dlg);
   selection_set_delete_command(tools, factory_clear);
   selection_set_edit_command(tools, factory_edit);
   gtk_box_pack_start(GTK_BOX(hbox), tools->container, FALSE, FALSE, 0);

   _preview = make_preview(drawable);
   add_preview_motion_event(_preview, (GtkSignalFunc) preview_move);
   add_enter_notify_event(_preview, (GtkSignalFunc) preview_enter);
   add_leave_notify_event(_preview, (GtkSignalFunc) preview_leave);
   add_preview_button_press_event(_preview, (GtkSignalFunc) button_press);
   gtk_container_add(GTK_CONTAINER(hbox), _preview->window);

   object_list_add_geometry_cb(_shapes, geometry_changed, NULL);
   object_list_add_update_cb(_shapes, data_changed, NULL);
   object_list_add_add_cb(_shapes, data_changed, NULL);
   object_list_add_remove_cb(_shapes, data_changed, NULL);
   object_list_add_move_cb(_shapes, data_changed, NULL);
   object_list_add_select_cb(_shapes, data_selected, NULL);

   /* Selection */
   _selection = make_selection(dlg, _shapes);
   selection_set_move_up_command(_selection, factory_move_up);
   selection_set_move_down_command(_selection, factory_move_down);
   selection_set_delete_command(_selection, factory_clear);
   selection_set_edit_command(_selection, factory_edit);
   gtk_box_pack_start(GTK_BOX(hbox), _selection->container, FALSE, FALSE, 0);

   _statusbar = make_statusbar(main_vbox, dlg);
   statusbar_set_zoom(_statusbar, 1);

   clear_map_info();

   gtk_widget_show(dlg);

   _mru = mru_create();
   init_preferences();
   if (!mru_empty(_mru))
      menu_build_mru_items(_mru);

   gtk_main();
   gdk_flush();
   
   return run_flag;
}
