/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadialog.h
 * Copyright (C) 2000-2003 Michael Natterer <mitch@ligma.org>
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
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_DIALOG_H__
#define __LIGMA_DIALOG_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define LIGMA_TYPE_DIALOG            (ligma_dialog_get_type ())
#define LIGMA_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DIALOG, LigmaDialog))
#define LIGMA_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DIALOG, LigmaDialogClass))
#define LIGMA_IS_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DIALOG))
#define LIGMA_IS_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DIALOG))
#define LIGMA_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DIALOG, LigmaDialogClass))


typedef struct _LigmaDialogPrivate LigmaDialogPrivate;
typedef struct _LigmaDialogClass   LigmaDialogClass;

struct _LigmaDialog
{
  GtkDialog          parent_instance;

  LigmaDialogPrivate *priv;
};

struct _LigmaDialogClass
{
  GtkDialogClass  parent_class;

  /* Padding for future expansion */
  void (* _ligma_reserved1) (void);
  void (* _ligma_reserved2) (void);
  void (* _ligma_reserved3) (void);
  void (* _ligma_reserved4) (void);
  void (* _ligma_reserved5) (void);
  void (* _ligma_reserved6) (void);
  void (* _ligma_reserved7) (void);
  void (* _ligma_reserved8) (void);
};


GType       ligma_dialog_get_type           (void) G_GNUC_CONST;

GtkWidget * ligma_dialog_new                (const gchar    *title,
                                            const gchar    *role,
                                            GtkWidget      *parent,
                                            GtkDialogFlags  flags,
                                            LigmaHelpFunc    help_func,
                                            const gchar    *help_id,
                                            ...) G_GNUC_NULL_TERMINATED;

GtkWidget * ligma_dialog_new_valist         (const gchar    *title,
                                            const gchar    *role,
                                            GtkWidget      *parent,
                                            GtkDialogFlags  flags,
                                            LigmaHelpFunc    help_func,
                                            const gchar    *help_id,
                                            va_list         args);

GtkWidget * ligma_dialog_add_button         (LigmaDialog     *dialog,
                                            const gchar    *button_text,
                                            gint            response_id);
void        ligma_dialog_add_buttons        (LigmaDialog     *dialog,
                                            ...) G_GNUC_NULL_TERMINATED;
void        ligma_dialog_add_buttons_valist (LigmaDialog     *dialog,
                                            va_list         args);

gint        ligma_dialog_run                (LigmaDialog     *dialog);

void        ligma_dialog_set_alternative_button_order_from_array
                                           (LigmaDialog     *dialog,
                                            gint            n_buttons,
                                            gint           *order);

/*  for internal use only!  */
void        ligma_dialogs_show_help_button  (gboolean        show);

/* ligma_dialog_set_alternative_button_order() doesn't need a dedicated
 * wrapper function because anyway it won't be introspectable.
 * GObject-Introspection bindings will have to use
 * ligma_dialog_set_alternative_button_order_from_array().
 */
#define ligma_dialog_set_alternative_button_order(d,f...) \
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;                      \
  gtk_dialog_set_alternative_button_order(d,f);          \
  G_GNUC_END_IGNORE_DEPRECATIONS;


G_END_DECLS

#endif /* __LIGMA_DIALOG_H__ */
