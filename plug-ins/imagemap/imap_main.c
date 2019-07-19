/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2006 Maurits Rijk  m.rijk@chello.nl
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

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <glib/gstdio.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h> /* for keyboard values */

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "imap_about.h"
#include "imap_circle.h"
#include "imap_commands.h"
#include "imap_default_dialog.h"
#include "imap_edit_area_info.h"
#include "imap_file.h"
#include "imap_main.h"
#include "imap_menu.h"
#include "imap_misc.h"
#include "imap_object.h"
#include "imap_polygon.h"
#include "imap_preview.h"
#include "imap_rectangle.h"
#include "imap_selection.h"
#include "imap_settings.h"
#include "imap_source.h"
#include "imap_statusbar.h"
#include "imap_stock.h"
#include "imap_string.h"

#include "libgimp/stdplugins-intl.h"


#define MAX_ZOOM_FACTOR 8
#define ZOOMED(x) (_zoom_factor * (x))
#define GET_REAL_COORD(x) ((x) / _zoom_factor)

static gint             zoom_in         (void);
static gint             zoom_out        (void);


/* Global variables */
static MapInfo_t   _map_info;
static PreferencesData_t _preferences = {CSIM, TRUE, FALSE, TRUE, TRUE, FALSE,
FALSE, TRUE, DEFAULT_UNDO_LEVELS, DEFAULT_MRU_SIZE};
static MRU_t *_mru;

static gint32        _drawable_id;
static GdkCursorType _cursor = GDK_TOP_LEFT_ARROW;
static gboolean     _show_url = TRUE;
static gchar       *_filename = NULL;
static gchar       *_image_name;
static gint        _image_width;
static gint        _image_height;
static GtkWidget   *_dlg;
static Preview_t   *_preview;
static Selection_t *_selection;
static StatusBar_t *_statusbar;
static ObjectList_t *_shapes;
static gint         _zoom_factor = 1;
static gboolean (*_button_press_func)(GtkWidget*, GdkEventButton*, gpointer);
static gpointer _button_press_param;

/* Declare local functions. */
static void  query  (void);
static void  run    (const gchar      *name,
                     gint              nparams,
                     const GimpParam  *param,
                     gint             *nreturn_vals,
                     GimpParam       **return_vals);
static gint  dialog (gint32            drawable_id);

const GimpPlugInInfo PLUG_IN_INFO = {
   NULL,                        /* init_proc */
   NULL,                        /* quit_proc */
   query,                       /* query_proc */
   run,                         /* run_proc */
};

static int run_flag = 0;


MAIN ()

static void query(void)
{
   static const GimpParamDef args[] = {
      {GIMP_PDB_INT32,    "run-mode", "The run mode { RUN-INTERACTIVE (0) }"},
      {GIMP_PDB_IMAGE,    "image",    "Input image (unused)"},
      {GIMP_PDB_DRAWABLE, "drawable", "Input drawable"},
   };
   static const GimpParamDef *return_vals = NULL;
   static int nreturn_vals = 0;

   gimp_install_procedure(PLUG_IN_PROC,
                          N_("Create a clickable imagemap"),
                          "",
                          "Maurits Rijk",
                          "Maurits Rijk",
                          "1998-2005",
                          N_("_Image Map..."),
                          "RGB*, GRAY*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), nreturn_vals,
                          args, return_vals);

   gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Web");
}

static void
run (const gchar      *name,
     gint              n_params,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
   static GimpParam values[1];
   GimpRunMode run_mode;
   gint32      drawable_id;
   GimpPDBStatusType status = GIMP_PDB_SUCCESS;

   INIT_I18N ();
   gegl_init (NULL, NULL);

   *nreturn_vals = 1;
   *return_vals = values;

   run_mode    = param[0].data.d_int32;
   drawable_id = param[2].data.d_drawable;

   _drawable_id = drawable_id;
   _image_name = gimp_image_get_name(param[1].data.d_image);
   _image_width = gimp_image_width(param[1].data.d_image);
   _image_height = gimp_image_height(param[1].data.d_image);

   _map_info.color = gimp_drawable_is_rgb(drawable_id);

   if (run_mode == GIMP_RUN_INTERACTIVE) {
      if (!dialog(drawable_id)) {
         /* The dialog was closed, or something similarly evil happened. */
         status = GIMP_PDB_EXECUTION_ERROR;
      }
   }

   values[0].type = GIMP_PDB_STATUS;
   values[0].data.d_status = status;
}

GtkWidget*
get_dialog(void)
{
  return _dlg;
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

   colors->interactive_fg.red = 0xFFFF;
   colors->interactive_fg.green = 0;
   colors->interactive_fg.blue = 0xFFFF;

   colors->interactive_bg.red = 0xFFFF;
   colors->interactive_bg.green = 0xFFFF;
   colors->interactive_bg.blue = 0;

   preferences_load(&_preferences);

   mru_set_size(_mru, _preferences.mru_size);
   command_list_set_undo_level(_preferences.undo_levels);
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
  gdk_window_set_cursor(gtk_widget_get_window (_dlg), NULL);
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
   preview_zoom(_preview, zoom_factor);
   statusbar_set_zoom(_statusbar, zoom_factor);
   remove_busy_cursor();
}

gint
get_real_coord(gint coord)
{
   return GET_REAL_COORD(coord);
}

void
draw_line(cairo_t *cr, gint x1, gint y1, gint x2, gint y2)
{
   cairo_move_to (cr, ZOOMED (x1) + .5, ZOOMED (y1) + .5);
   cairo_line_to (cr, ZOOMED (x2) + .5, ZOOMED (y2) + .5);
   cairo_stroke (cr);
}

void
draw_rectangle(cairo_t *cr, gboolean filled, gint x, gint y,
               gint width, gint height)
{
   cairo_rectangle (cr, ZOOMED (x) + (filled ? 0. : .5),
                        ZOOMED (y) + (filled ? 0. : .5),
                    ZOOMED (width), ZOOMED (height));
   if (filled)
      cairo_fill (cr);
   else
      cairo_stroke (cr);
}

void
draw_circle(cairo_t *cr, gint x, gint y, gint r)
{
   cairo_arc (cr, ZOOMED (x), ZOOMED (y), ZOOMED (r), 0., 2 * G_PI);
   cairo_stroke (cr);
}

void
draw_polygon(cairo_t *cr, GList *list)
{
   GList     *p;

   for (p = list; p; p = p->next) {
      GdkPoint *src = (GdkPoint*) p->data;
      cairo_line_to (cr, ZOOMED (src->x) + .5, ZOOMED (src->y) + .5);
   }
   cairo_close_path (cr);
   cairo_stroke (cr);
}

void
set_preview_color (GtkRadioAction *action, GtkRadioAction *current,
                   gpointer user_data)
{
  _map_info.show_gray = (gtk_radio_action_get_current_value (current) == 1);
   set_zoom(_zoom_factor);
}

void
preview_redraw(void)
{
  gtk_widget_queue_draw(_preview->preview);
}

void
set_zoom_factor (GtkRadioAction *action, GtkRadioAction *current,
                 gpointer user_data)
{
  gint factor = gtk_radio_action_get_current_value (current);
  set_zoom (factor + 1);
}

const gchar *
get_image_name(void)
{
   return _image_name;
}

const char*
get_filename(void)
{
   return _filename;
}

static gboolean
arrow_on_button_press (GtkWidget      *widget,
                       GdkEventButton *event,
                       gpointer        data)
{
  if (gdk_event_triggers_context_menu ((GdkEvent *) event))
    {
      do_popup_menu (event);
    }
  else if (event->button == 1)
    {
      if (event->type == GDK_2BUTTON_PRESS)
        edit_shape((gint) event->x, (gint) event->y);
      else
        select_shape(widget, event);
    }

  return FALSE;
}

static void
set_arrow_func(void)
{
   _button_press_func = arrow_on_button_press;
   _cursor = GDK_TOP_LEFT_ARROW;
}

static void
set_object_func(gboolean (*func)(GtkWidget*, GdkEventButton*,
                                 gpointer), gpointer param)
{
   _button_press_func = func;
   _button_press_param = param;
   _cursor = GDK_CROSSHAIR;
}

void
set_func(GtkRadioAction *action, GtkRadioAction *current,
         gpointer user_data)
{
  gint value = gtk_radio_action_get_current_value (current);
  switch (value)
    {
    case 0:
      set_arrow_func();
      break;
    case 1:
      set_object_func(object_on_button_press, get_rectangle_factory);
      break;
    case 2:
      set_object_func(object_on_button_press, get_circle_factory);
      break;
    case 3:
      set_object_func(object_on_button_press, get_polygon_factory);
      break;
    default:
      break;
    }
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

void
do_edit_selected_shape(void)
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
}

static void
main_set_title(const char *filename)
{
   char *title, *p;

   g_strreplace(&_filename, filename);
   p = filename ? g_filename_display_basename (filename) : (gchar *) _("<Untitled>");
   title = g_strdup_printf("%s - Image Map", p);
   if (filename)
     g_free (p);
   gtk_window_set_title(GTK_WINDOW(_dlg), title);
   g_free(title);
}

void
main_set_dimension(gint width, gint height)
{
   statusbar_set_dimension(_statusbar,
                           width / _zoom_factor, height / _zoom_factor);
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
   if (obj) {                   /* Start resizing */
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
         } else {               /* No Shift key pressed */
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

void
do_zoom_in(void)
{
   gint factor = zoom_in();
   menu_set_zoom_sensitivity(factor);
}

void
do_zoom_out(void)
{
   gint factor = zoom_out();
   menu_set_zoom_sensitivity(factor);
}

void
draw_shapes(cairo_t *cr)
{
   object_list_draw(_shapes, cr);
}

static void
clear_map_info(void)
{
   const gchar *author = g_get_real_name();

   if (!*author)
      author = g_get_user_name();
   g_strreplace(&_map_info.image_name, _image_name);
   g_strreplace(&_map_info.title, "map");
   g_strreplace(&_map_info.author, author);
   g_strreplace(&_map_info.default_url, "");
   g_strreplace(&_map_info.description, "");

   _map_info.map_format = _preferences.default_map_type;
   _map_info.show_gray = FALSE;
}

static void
do_data_changed_dialog(void (*continue_cb)(gpointer), gpointer param)
{
   GtkWidget *dialog = gtk_message_dialog_new
     (NULL,
      GTK_DIALOG_DESTROY_WITH_PARENT,
      GTK_MESSAGE_QUESTION,
      GTK_BUTTONS_YES_NO,
      _("Some data has been changed!"));
   gtk_message_dialog_format_secondary_text
     (GTK_MESSAGE_DIALOG (dialog),
      _("Do you really want to discard your changes?"));

   if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
     continue_cb (param);

   gtk_widget_destroy (dialog);
}

static void
check_if_changed(void (*func)(gpointer), gpointer param)
{
   if (object_list_get_changed (_shapes))
     do_data_changed_dialog (func, param);
   else
     func (param);
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
   preview_redraw();
   object_list_clear_changed(_shapes);
   command_list_remove_all();
}

static void
really_close(gpointer data)
{
   close_current();
}

void
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

void
do_quit(void)
{
   check_if_changed(really_quit, NULL);
}

void
do_undo(void)
{
   selection_freeze(_selection);
   last_command_undo();
   selection_thaw(_selection);
   preview_redraw();
}

void
do_redo(void)
{
   selection_freeze(_selection);
   last_command_redo();
   selection_thaw(_selection);
   preview_redraw();
}

void
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
   output(param, "-:Image map file created by GIMP Image Map plug-in\n");
   write_cern_comment(param, output);
   output(param, "-:GIMP Image Map plug-in by Maurits Rijk\n");
   write_cern_comment(param, output);
   output(param, "-:Please do not edit lines starting with \"#$\"\n");
   write_cern_comment(param, output);
   output(param, "VERSION:2.3\n");
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

   output(param, "<img src=\"%s\" width=\"%d\" height=\"%d\" border=\"0\" "
          "usemap=\"#%s\" />\n\n", _map_info.image_name,
          _image_width, _image_height, _map_info.title);
   output(param, "<map name=\"%s\">\n", _map_info.title);
   output(param,
          "<!-- #$-:Image map file created by GIMP Image Map plug-in -->\n");
   output(param, "<!-- #$-:GIMP Image Map plug-in by Maurits Rijk -->\n");
   output(param,
          "<!-- #$-:Please do not edit lines starting with \"#$\" -->\n");
   output(param, "<!-- #$VERSION:2.3 -->\n");
   output(param, "<!-- #$AUTHOR:%s -->\n", _map_info.author);

   description = g_strdup(_map_info.description);
   for (p = strtok(description, "\n"); p; p = strtok(NULL, "\n"))
      output(param, "<!-- #$DESCRIPTION:%s -->\n", p);
   g_free(description);

   object_list_write_csim(_shapes, param, output);
   if (*_map_info.default_url)
      output(param, "<area shape=\"default\" href=\"%s\" />\n",
             _map_info.default_url);
   output(param, "</map>\n");
}

static void
save_as_ncsa(gpointer param, OutputFunc_t output)
{
   char *p;
   gchar *description;

   output(param, "#$-:Image map file created by GIMP Image Map plug-in\n");
   output(param, "#$-:GIMP Image Map plug-in by Maurits Rijk\n");
   output(param, "#$-:Please do not edit lines starting with \"#$\"\n");
   output(param, "#$VERSION:2.3\n");
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

static void   save_to_file (gpointer    param,
                            const char *format,
                            ...) G_GNUC_PRINTF(2,3);

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
   FILE *out = g_fopen(filename, "w");
   if (out) {
      dump_output(out, save_to_file);
      fclose(out);

      statusbar_set_status(_statusbar, _("File \"%s\" saved."), filename);
      main_set_title(filename);
      object_list_clear_changed(_shapes);
   } else {
      do_file_error_dialog( _("Couldn't save file:"), filename);
   }
}

static void
do_image_size_changed_dialog(void)
{
   GtkWidget *dialog = gtk_message_dialog_new_with_markup
     (NULL,
      GTK_DIALOG_DESTROY_WITH_PARENT,
      GTK_MESSAGE_QUESTION,
      GTK_BUTTONS_YES_NO,
      "<span weight=\"bold\" size=\"larger\">%s</span>\n\n%s",
      _("Image size has changed."),
      _("Resize area's?"));

   if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
     {
       gint per_x = _image_width * 100 / _map_info.old_image_width;
       gint per_y = _image_height * 100 / _map_info.old_image_height;
       object_list_resize(_shapes, per_x, per_y);
     }

   preview_redraw();
   gtk_widget_destroy (dialog);
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
         do_image_size_changed_dialog();
      }
   } else if (load_ncsa(filename)) {
      _map_info.map_format = NCSA;
   } else if (load_cern(filename)) {
      _map_info.map_format = CERN;
   } else {
      do_file_error_dialog( _("Couldn't read file:"), filename);
      selection_thaw(_selection);
      close_current();
      return;
   }
   mru_set_first(_mru, filename);
   menu_build_mru_items(_mru);

   selection_thaw(_selection);
   main_set_title(filename);
   object_list_clear_changed(_shapes);
   preview_redraw();
}

void
load(const gchar *filename)
{
   static gchar *tmp_filename;
   g_strreplace(&tmp_filename, filename);
   check_if_changed(really_load, (gpointer) tmp_filename);
}

void
toggle_area_list(void)
{
   selection_toggle_visibility(_selection);
}

static gboolean
close_callback(GtkWidget *widget, gpointer data)
{
   do_quit();
   return TRUE;
}

static gboolean
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
         statusbar_set_status(_statusbar, _("URL: %s"), obj->url);
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
   return FALSE;
}

static void
preview_enter(GtkWidget *widget, GdkEventCrossing *event)
{
   preview_set_cursor(_preview, _cursor);
}

static void
preview_leave(GtkWidget *widget, GdkEventCrossing *event)
{
  gdk_window_set_cursor(gtk_widget_get_window (_dlg), NULL);
  statusbar_clear_xy(_statusbar);
}

static gboolean
button_press(GtkWidget* widget, GdkEventButton* event, gpointer data)
{
  if (_button_press_func)
    return _button_press_func(widget, event, _button_press_param);

  return FALSE;
}

/* A few global vars for key movement */

static guint _timeout;
static guint _keyval;
static gint _dx, _dy;

static void
move_sash_selected_objects(gint dx, gint dy, gboolean fast)
{
   if (fast) {
      dx *= 5;
      dy *= 5;
   }

   object_list_move_sash_selected(_shapes, dx, dy);

   preview_redraw ();
}

static void
move_selected_objects(gint dx, gint dy, gboolean fast)
{
   if (fast) {
      dx *= 5;
      dy *= 5;
   }
   _dx += dx;
   _dy += dy;

   object_list_move_selected(_shapes, dx, dy);

   preview_redraw ();
}

static gboolean
key_timeout_cb(gpointer data)
{
   switch (_keyval) {
   case GDK_KEY_Left:
   case GDK_KEY_Right:
   case GDK_KEY_Up:
   case GDK_KEY_Down:
      command_list_add(move_selected_command_new(_shapes, _dx, _dy));
      _dx = _dy = 0;
      break;
   }
   preview_redraw();

   _timeout = 0;
   return FALSE;
}

static gboolean
key_press_cb(GtkWidget *widget, GdkEventKey *event)
{
   gboolean handled = FALSE;
   gboolean shift = event->state & GDK_SHIFT_MASK;
   gboolean ctrl = event->state & GDK_CONTROL_MASK;
   Command_t *command;

   if (_timeout)
      g_source_remove(_timeout);
   _timeout = 0;

   switch (event->keyval) {
   case GDK_KEY_Left:
      if (ctrl)
         move_sash_selected_objects(-1, 0, shift);
      else
         move_selected_objects(-1, 0, shift);
      handled = TRUE;
      break;
   case GDK_KEY_Right:
      if (ctrl)
         move_sash_selected_objects(1, 0, shift);
      else
         move_selected_objects(1, 0, shift);
      handled = TRUE;
      break;
   case GDK_KEY_Up:
      if (ctrl)
         move_sash_selected_objects(0, -1, shift);
      else
         move_selected_objects(0, -1, shift);
      handled = TRUE;
      break;
   case GDK_KEY_Down:
      if (ctrl)
         move_sash_selected_objects(0, 1, shift);
      else
         move_selected_objects(0, 1, shift);
      handled = TRUE;
      break;
   case GDK_KEY_Tab:
      if (shift)
         command = select_prev_command_new(_shapes);
      else
         command = select_next_command_new(_shapes);
      command_execute(command);
      handled = TRUE;
      break;
   }
   if (handled)
      g_signal_stop_emission_by_name(widget, "key-press-event");

   return handled;
}

static gboolean
key_release_cb(GtkWidget *widget, GdkEventKey *event)
{
   _keyval = event->keyval;
   _timeout = g_timeout_add(250, key_timeout_cb, NULL);
   return FALSE;
}

static void
geometry_changed(Object_t *obj, gpointer data)
{
   preview_redraw();
}

static void
data_changed(Object_t *obj, gpointer data)
{
   preview_redraw();
   set_all_sensitivities();
}

static void
data_selected(Object_t *obj, gpointer data)
{
   set_all_sensitivities();
}

void
imap_help (void)
{
  gimp_standard_help_func ("plug-in-imagemap", NULL);
}

void
do_cut (void)
{
  command_execute (cut_command_new (_shapes));
}

void
do_copy (void)
{
  command_execute (copy_command_new (_shapes));
}

void
do_paste (void)
{
  command_execute (paste_command_new (_shapes));
}

void
do_select_all(void)
{
  command_execute (select_all_command_new (_shapes));
}

void
do_deselect_all(void)
{
  command_execute (unselect_all_command_new (_shapes, NULL));
}

void
do_clear(void)
{
  command_execute (clear_command_new(_shapes));
}

void
do_move_up(void)
{
  /* Fix me!
   Command_t *command = object_up_command_new(_current_obj->list,
                                              _current_obj);
   command_execute(command);
  */
}

void
do_move_down(void)
{
  /* Fix me!
   Command_t *command = object_down_command_new(_current_obj->list,
                                                _current_obj);
   command_execute(command);
  */
}

void
do_move_to_front(void)
{
  command_execute(move_to_front_command_new(_shapes));
}

void
do_send_to_back(void)
{
  command_execute(send_to_back_command_new(_shapes));
}

void
do_use_gimp_guides_dialog(void)
{
  command_execute (gimp_guides_command_new (_shapes, _drawable_id));
}

void
do_create_guides_dialog(void)
{
  command_execute (guides_command_new (_shapes));
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
dialog(gint32 drawable_id)
{
   GtkWidget    *dlg;
   GtkWidget    *hbox;
   GtkWidget    *main_vbox;
   GtkWidget    *tools;

   gimp_ui_init (PLUG_IN_BINARY, TRUE);

   set_arrow_func ();

   _shapes = make_object_list();

   _dlg = dlg = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_resizable(GTK_WINDOW(dlg), TRUE);

   main_set_title(NULL);
   gimp_help_connect (dlg, gimp_standard_help_func, PLUG_IN_PROC, NULL);

   gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);

   gimp_window_set_transient (GTK_WINDOW (dlg));

   g_signal_connect (dlg, "delete-event",
                     G_CALLBACK (close_callback), NULL);
   g_signal_connect (dlg, "key-press-event",
                     G_CALLBACK (key_press_cb), NULL);
   g_signal_connect (dlg, "key-release-event",
                     G_CALLBACK (key_release_cb), NULL);

   g_signal_connect (dlg, "destroy",
                     G_CALLBACK (gtk_main_quit),
                     NULL);

   main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
   gtk_container_add (GTK_CONTAINER (dlg), main_vbox);
   gtk_widget_show (main_vbox);

   init_stock_icons();

   /* Create menu */
   make_menu(main_vbox, dlg);

   /* Create toolbar */
   make_toolbar(main_vbox, dlg);

   /*  Dialog area  */
   hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
   gtk_box_pack_start (GTK_BOX (main_vbox), hbox, TRUE, TRUE, 0);
   gtk_widget_show(hbox);

   tools = make_tools(dlg);
   /* selection_set_edit_command(tools, factory_edit); */
   gtk_box_pack_start(GTK_BOX(hbox), tools, FALSE, FALSE, 0);

   _preview = make_preview(drawable_id);

   g_signal_connect(_preview->preview, "motion-notify-event",
                    G_CALLBACK(preview_move), NULL);
   g_signal_connect(_preview->preview, "enter-notify-event",
                    G_CALLBACK(preview_enter), NULL);
   g_signal_connect(_preview->preview, "leave-notify-event",
                    G_CALLBACK(preview_leave), NULL);
   g_signal_connect(_preview->preview, "button-press-event",
                    G_CALLBACK(button_press), NULL);
   gtk_box_pack_start (GTK_BOX (hbox), _preview->window, TRUE, TRUE, 0);

   object_list_add_geometry_cb(_shapes, geometry_changed, NULL);
   object_list_add_update_cb(_shapes, data_changed, NULL);
   object_list_add_add_cb(_shapes, data_changed, NULL);
   object_list_add_remove_cb(_shapes, data_changed, NULL);
   object_list_add_move_cb(_shapes, data_changed, NULL);
   object_list_add_select_cb(_shapes, data_selected, NULL);

   /* Selection */
   _selection = make_selection(_shapes);
   selection_set_move_up_command(_selection, factory_move_up);
   selection_set_move_down_command(_selection, factory_move_down);
   gtk_box_pack_start(GTK_BOX(hbox), _selection->container, FALSE, FALSE, 0);

   _statusbar = make_statusbar(main_vbox, dlg);
   statusbar_set_zoom(_statusbar, 1);

   gtk_widget_show(dlg);

   _mru = mru_create();
   init_preferences();

   clear_map_info();

   if (!mru_empty(_mru))
      menu_build_mru_items(_mru);

   gtk_main();

   return run_flag;
}
