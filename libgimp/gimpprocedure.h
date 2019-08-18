/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpprocedure.h
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
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

#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

#ifndef __GIMP_PROCEDURE_H__
#define __GIMP_PROCEDURE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * GimpRunFunc:
 * @procedure: the #GimpProcedure that runs.
 * @args:      the @procedure's arguments.
 * @run_data:  the run_data given in gimp_procedure_new().
 *
 * The run function is run during the lifetime of the GIMP session,
 * each time a plug-in procedure is called.
 *
 * Returns: (transfer full): the @procedure's return values.
 *
 * Since: 3.0
 **/
typedef GimpValueArray * (* GimpRunFunc) (GimpProcedure        *procedure,
                                          const GimpValueArray *args,
                                          gpointer              run_data);


#define GIMP_TYPE_PROCEDURE            (gimp_procedure_get_type ())
#define GIMP_PROCEDURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PROCEDURE, GimpProcedure))
#define GIMP_PROCEDURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PROCEDURE, GimpProcedureClass))
#define GIMP_IS_PROCEDURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PROCEDURE))
#define GIMP_IS_PROCEDURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PROCEDURE))
#define GIMP_PROCEDURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PROCEDURE, GimpProcedureClass))


typedef struct _GimpProcedureClass   GimpProcedureClass;
typedef struct _GimpProcedurePrivate GimpProcedurePrivate;

struct _GimpProcedure
{
  GObject               parent_instance;

  GimpProcedurePrivate *priv;
};

struct _GimpProcedureClass
{
  GObjectClass parent_class;

  void             (* install)   (GimpProcedure        *procedure);
  void             (* uninstall) (GimpProcedure        *procedure);

  GimpValueArray * (* run)       (GimpProcedure        *procedure,
                                  const GimpValueArray *args);

  /* Padding for future expansion */
  void (*_gimp_reserved1) (void);
  void (*_gimp_reserved2) (void);
  void (*_gimp_reserved3) (void);
  void (*_gimp_reserved4) (void);
  void (*_gimp_reserved5) (void);
  void (*_gimp_reserved6) (void);
  void (*_gimp_reserved7) (void);
  void (*_gimp_reserved8) (void);
};


GType            gimp_procedure_get_type           (void) G_GNUC_CONST;

GimpProcedure  * gimp_procedure_new                (GimpPlugIn           *plug_in,
                                                    const gchar          *name,
                                                    GimpPDBProcType       proc_type,
                                                    GimpRunFunc           run_func,
                                                    gpointer              run_data,
                                                    GDestroyNotify        run_data_destroy);

GimpPlugIn     * gimp_procedure_get_plug_in        (GimpProcedure        *procedure);
const gchar    * gimp_procedure_get_name           (GimpProcedure        *procedure);
GimpPDBProcType  gimp_procedure_get_proc_type      (GimpProcedure        *procedure);

void             gimp_procedure_set_image_types    (GimpProcedure        *procedure,
                                                    const gchar          *image_types);
const gchar    * gimp_procedure_get_image_types    (GimpProcedure        *procedure);

void             gimp_procedure_set_menu_label     (GimpProcedure        *procedure,
                                                    const gchar          *menu_label);
const gchar    * gimp_procedure_get_menu_label     (GimpProcedure        *procedure);

void             gimp_procedure_add_menu_path      (GimpProcedure        *procedure,
                                                    const gchar          *menu_path);
GList          * gimp_procedure_get_menu_paths     (GimpProcedure        *procedure);

void             gimp_procedure_set_icon_name      (GimpProcedure        *procedure,
                                                    const gchar          *icon_name);
void             gimp_procedure_set_icon_file      (GimpProcedure        *procedure,
                                                    GFile                *file);
void             gimp_procedure_set_icon_pixbuf    (GimpProcedure        *procedure,
                                                    GdkPixbuf            *pixbuf);

GimpIconType     gimp_procedure_get_icon_type      (GimpProcedure        *procedure);
const gchar    * gimp_procedure_get_icon_name      (GimpProcedure        *procedure);
GFile          * gimp_procedure_get_icon_file      (GimpProcedure        *procedure);
GdkPixbuf      * gimp_procedure_get_icon_pixbuf    (GimpProcedure        *procedure);

void             gimp_procedure_set_documentation  (GimpProcedure        *procedure,
                                                    const gchar          *blurb,
                                                    const gchar          *help,
                                                    const gchar          *help_id);
const gchar    * gimp_procedure_get_blurb          (GimpProcedure        *procedure);
const gchar    * gimp_procedure_get_help           (GimpProcedure        *procedure);
const gchar    * gimp_procedure_get_help_id        (GimpProcedure        *procedure);

void             gimp_procedure_set_attribution    (GimpProcedure        *procedure,
                                                    const gchar          *authors,
                                                    const gchar          *copyright,
                                                    const gchar          *date);
const gchar    * gimp_procedure_get_authors        (GimpProcedure        *procedure);
const gchar    * gimp_procedure_get_copyright      (GimpProcedure        *procedure);
const gchar    * gimp_procedure_get_date           (GimpProcedure        *procedure);

void             gimp_procedure_add_argument       (GimpProcedure        *procedure,
                                                    GParamSpec           *pspec);
void             gimp_procedure_add_argument_from_property
                                                   (GimpProcedure        *procedure,
                                                    GObject              *config,
                                                    const gchar          *prop_name);
void             gimp_procedure_add_return_value   (GimpProcedure        *procedure,
                                                    GParamSpec           *pspec);
void             gimp_procedure_add_return_value_from_property
                                                   (GimpProcedure        *procedure,
                                                    GObject              *config,
                                                    const gchar          *prop_name);

GParamSpec    ** gimp_procedure_get_arguments      (GimpProcedure        *procedure,
                                                    gint                 *n_arguments);
GParamSpec    ** gimp_procedure_get_return_values  (GimpProcedure        *procedure,
                                                    gint                 *n_return_values);

GimpValueArray * gimp_procedure_new_arguments      (GimpProcedure        *procedure);
GimpValueArray * gimp_procedure_new_return_values  (GimpProcedure        *procedure,
                                                    GimpPDBStatusType     status,
                                                    GError               *error);

GimpValueArray * gimp_procedure_run                (GimpProcedure        *procedure,
                                                    GimpValueArray       *args);

void             gimp_procedure_extension_ready    (GimpProcedure        *procedure);


G_END_DECLS

#endif  /*  __GIMP_PROCEDURE_H__  */
