/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdialog.h
 * Copyright (C) 2000-2003 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_DIALOG_H__
#define __GIMP_DIALOG_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_DIALOG            (gimp_dialog_get_type ())
#define GIMP_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DIALOG, GimpDialog))
#define GIMP_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DIALOG, GimpDialogClass))
#define GIMP_IS_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DIALOG))
#define GIMP_IS_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DIALOG))
#define GIMP_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DIALOG, GimpDialogClass))


typedef struct _GimpDialogPrivate GimpDialogPrivate;
typedef struct _GimpDialogClass   GimpDialogClass;

struct _GimpDialog
{
  GtkDialog          parent_instance;

  GimpDialogPrivate *priv;
};

struct _GimpDialogClass
{
  GtkDialogClass  parent_class;

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
  void (* _gimp_reserved5) (void);
  void (* _gimp_reserved6) (void);
  void (* _gimp_reserved7) (void);
  void (* _gimp_reserved8) (void);
};


GType       gimp_dialog_get_type           (void) G_GNUC_CONST;

GtkWidget * gimp_dialog_new                (const gchar    *title,
                                            const gchar    *role,
                                            GtkWidget      *parent,
                                            GtkDialogFlags  flags,
                                            GimpHelpFunc    help_func,
                                            const gchar    *help_id,
                                            ...) G_GNUC_NULL_TERMINATED;

GtkWidget * gimp_dialog_new_valist         (const gchar    *title,
                                            const gchar    *role,
                                            GtkWidget      *parent,
                                            GtkDialogFlags  flags,
                                            GimpHelpFunc    help_func,
                                            const gchar    *help_id,
                                            va_list         args);

GtkWidget * gimp_dialog_add_button         (GimpDialog     *dialog,
                                            const gchar    *button_text,
                                            gint            response_id);
void        gimp_dialog_add_buttons        (GimpDialog     *dialog,
                                            ...) G_GNUC_NULL_TERMINATED;
void        gimp_dialog_add_buttons_valist (GimpDialog     *dialog,
                                            va_list         args);

gint        gimp_dialog_run                (GimpDialog     *dialog);

/*  for internal use only!  */
void        gimp_dialogs_show_help_button  (gboolean        show);


/*  since we don't know yet what to do about alternative button order,
 *  just hide the warnings for now...
 */
#define gimp_dialog_set_alternative_button_order(d,f...) \
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;                      \
  gtk_dialog_set_alternative_button_order(d,f);          \
  G_GNUC_END_IGNORE_DEPRECATIONS;

#define gimp_dialog_set_alternative_button_order_from_array(d,n,o) \
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;                                \
  gtk_dialog_set_alternative_button_order_from_array(d,n,o);       \
  G_GNUC_END_IGNORE_DEPRECATIONS;


G_END_DECLS

#endif /* __GIMP_DIALOG_H__ */
