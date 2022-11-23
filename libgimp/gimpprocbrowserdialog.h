/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaprocbrowserdialog.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__LIGMA_UI_H_INSIDE__) && !defined (LIGMA_COMPILATION)
#error "Only <libligma/ligmaui.h> can be included directly."
#endif

#ifndef __LIGMA_PROC_BROWSER_DIALOG_H__
#define __LIGMA_PROC_BROWSER_DIALOG_H__

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */


#define LIGMA_TYPE_PROC_BROWSER_DIALOG            (ligma_proc_browser_dialog_get_type ())
#define LIGMA_PROC_BROWSER_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PROC_BROWSER_DIALOG, LigmaProcBrowserDialog))
#define LIGMA_PROC_BROWSER_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PROC_BROWSER_DIALOG, LigmaProcBrowserDialogClass))
#define LIGMA_IS_PROC_BROWSER_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PROC_BROWSER_DIALOG))
#define LIGMA_IS_PROC_BROWSER_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PROC_BROWSER_DIALOG))
#define LIGMA_PROC_BROWSER_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PROC_BROWSER_DIALOG, LigmaProcBrowserDialogClass))


typedef struct _LigmaProcBrowserDialogPrivate LigmaProcBrowserDialogPrivate;
typedef struct _LigmaProcBrowserDialogClass   LigmaProcBrowserDialogClass;

struct _LigmaProcBrowserDialog
{
  LigmaDialog                    parent_instance;

  LigmaProcBrowserDialogPrivate *priv;
};

struct _LigmaProcBrowserDialogClass
{
  LigmaDialogClass  parent_class;

  void (* selection_changed) (LigmaProcBrowserDialog *dialog);
  void (* row_activated)     (LigmaProcBrowserDialog *dialog);

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


GType       ligma_proc_browser_dialog_get_type     (void) G_GNUC_CONST;

GtkWidget * ligma_proc_browser_dialog_new          (const gchar  *title,
                                                   const gchar  *role,
                                                   LigmaHelpFunc  help_func,
                                                   const gchar  *help_id,
                                                   ...) G_GNUC_NULL_TERMINATED;

gchar     * ligma_proc_browser_dialog_get_selected (LigmaProcBrowserDialog *dialog);


G_END_DECLS

#endif  /* __LIGMA_PROC_BROWSER_DIALOG_H__ */
