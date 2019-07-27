/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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


typedef GimpValueArray * (* GimpRunFunc) (GimpProcedure        *procedure,
                                          const GimpValueArray *args);


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
};


GType            gimp_procedure_get_type           (void) G_GNUC_CONST;

GimpProcedure  * gimp_procedure_new                (const gchar       *name,
                                                    GimpRunFunc        run_func);

void             gimp_procedure_set_strings        (GimpProcedure     *procedure,
                                                    const gchar       *menu_label,
                                                    const gchar       *blurb,
                                                    const gchar       *help,
                                                    const gchar       *help_id,
                                                    const gchar       *author,
                                                    const gchar       *copyright,
                                                    const gchar       *date,
                                                    const gchar       *image_types);

const gchar    * gimp_procedure_get_name           (GimpProcedure     *procedure);
const gchar    * gimp_procedure_get_menu_label     (GimpProcedure     *procedure);
const gchar    * gimp_procedure_get_blurb          (GimpProcedure     *procedure);
const gchar    * gimp_procedure_get_help           (GimpProcedure     *procedure);
const gchar    * gimp_procedure_get_help_id        (GimpProcedure     *procedure);
const gchar    * gimp_procedure_get_author         (GimpProcedure     *procedure);
const gchar    * gimp_procedure_get_copyright      (GimpProcedure     *procedure);
const gchar    * gimp_procedure_get_date           (GimpProcedure     *procedure);
const gchar    * gimp_procedure_get_image_types    (GimpProcedure     *procedure);

void             gimp_procedure_add_menu_path      (GimpProcedure     *procedure,
                                                    const gchar       *menu_path);
GList          * gimp_procedure_get_menu_paths     (GimpProcedure     *procedure);

void             gimp_procedure_add_argument       (GimpProcedure     *procedure,
                                                    GParamSpec        *pspec);
void             gimp_procedure_add_return_value   (GimpProcedure     *procedure,
                                                    GParamSpec        *pspec);

GParamSpec    ** gimp_procedure_get_arguments      (GimpProcedure     *procedure,
                                                    gint              *n_arguments);
GParamSpec    ** gimp_procedure_get_return_values  (GimpProcedure     *procedure,
                                                    gint              *n_return_values);

GimpValueArray * gimp_procedure_new_arguments      (GimpProcedure     *procedure);
GimpValueArray * gimp_procedure_new_return_values  (GimpProcedure     *procedure,
                                                    GimpPDBStatusType  status,
                                                    const GError      *error);

GimpValueArray * gimp_procedure_run                (GimpProcedure     *procedure,
                                                    GimpValueArray    *args);


G_END_DECLS

#endif  /*  __GIMP_PROCEDURE_H__  */
