/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_PROCEDURE_H__
#define __GIMP_PROCEDURE_H__


#include "core/gimpviewable.h"


typedef GimpValueArray * (* GimpMarshalFunc) (GimpProcedure         *procedure,
                                              Gimp                  *gimp,
                                              GimpContext           *context,
                                              GimpProgress          *progress,
                                              const GimpValueArray  *args,
                                              GError               **error);


#define GIMP_TYPE_PROCEDURE            (gimp_procedure_get_type ())
#define GIMP_PROCEDURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PROCEDURE, GimpProcedure))
#define GIMP_PROCEDURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PROCEDURE, GimpProcedureClass))
#define GIMP_IS_PROCEDURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PROCEDURE))
#define GIMP_IS_PROCEDURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PROCEDURE))
#define GIMP_PROCEDURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PROCEDURE, GimpProcedureClass))


typedef struct _GimpProcedureClass GimpProcedureClass;

struct _GimpProcedure
{
  GimpViewable      parent_instance;

  GimpPDBProcType   proc_type;      /* Type of procedure              */

  gboolean          static_help;    /* Are the strings allocated?     */
  gchar            *blurb;          /* Short procedure description    */
  gchar            *help;           /* Detailed help instructions     */
  gchar            *help_id;        /* Help ID                        */

  gboolean          static_attribution;
  gchar            *authors;        /* Authors field                  */
  gchar            *copyright;      /* Copyright field                */
  gchar            *date;           /* Date field                     */

  gchar            *deprecated;     /* Replacement if deprecated      */

  gchar            *label;          /* Cached label string            */

  gint32            num_args;       /* Number of procedure arguments  */
  GParamSpec      **args;           /* Array of procedure arguments   */

  gint32            num_values;     /* Number of return values        */
  GParamSpec      **values;         /* Array of return values         */

  GimpMarshalFunc   marshal_func;   /* Marshaller for internal procs  */
};

struct _GimpProcedureClass
{
  GimpViewableClass parent_class;

  const gchar    * (* get_label)      (GimpProcedure   *procedure);
  const gchar    * (* get_menu_label) (GimpProcedure   *procedure);
  const gchar    * (* get_blurb)      (GimpProcedure   *procedure);
  const gchar    * (* get_help_id)    (GimpProcedure   *procedure);
  gboolean         (* get_sensitive)  (GimpProcedure   *procedure,
                                       GimpObject      *object,
                                       const gchar    **reason);

  GimpValueArray * (* execute)        (GimpProcedure   *procedure,
                                       Gimp            *gimp,
                                       GimpContext     *context,
                                       GimpProgress    *progress,
                                       GimpValueArray  *args,
                                       GError         **error);
  void             (* execute_async)  (GimpProcedure   *procedure,
                                       Gimp            *gimp,
                                       GimpContext     *context,
                                       GimpProgress    *progress,
                                       GimpValueArray  *args,
                                       GimpDisplay     *display);
};


GType            gimp_procedure_get_type           (void) G_GNUC_CONST;

GimpProcedure  * gimp_procedure_new                (GimpMarshalFunc   marshal_func);

void             gimp_procedure_set_help           (GimpProcedure    *procedure,
                                                    const gchar      *blurb,
                                                    const gchar      *help,
                                                    const gchar      *help_id);
void             gimp_procedure_set_static_help    (GimpProcedure    *procedure,
                                                    const gchar      *blurb,
                                                    const gchar      *help,
                                                    const gchar      *help_id);
void             gimp_procedure_take_help          (GimpProcedure    *procedure,
                                                    gchar            *blurb,
                                                    gchar            *help,
                                                    gchar            *help_id);

void             gimp_procedure_set_attribution    (GimpProcedure    *procedure,
                                                    const gchar      *authors,
                                                    const gchar      *copyright,
                                                    const gchar      *date);
void             gimp_procedure_set_static_attribution
                                                   (GimpProcedure    *procedure,
                                                    const gchar      *authors,
                                                    const gchar      *copyright,
                                                    const gchar      *date);
void             gimp_procedure_take_attribution   (GimpProcedure    *procedure,
                                                    gchar            *authors,
                                                    gchar            *copyright,
                                                    gchar            *date);

void             gimp_procedure_set_deprecated     (GimpProcedure    *procedure,
                                                    const gchar      *deprecated);

const gchar    * gimp_procedure_get_label          (GimpProcedure    *procedure);
const gchar    * gimp_procedure_get_menu_label     (GimpProcedure    *procedure);
const gchar    * gimp_procedure_get_blurb          (GimpProcedure    *procedure);
const gchar    * gimp_procedure_get_help           (GimpProcedure    *procedure);
const gchar    * gimp_procedure_get_help_id        (GimpProcedure    *procedure);
gboolean         gimp_procedure_get_sensitive      (GimpProcedure    *procedure,
                                                    GimpObject       *object,
                                                    const gchar     **reason);

void             gimp_procedure_add_argument       (GimpProcedure    *procedure,
                                                    GParamSpec       *pspec);
void             gimp_procedure_add_return_value   (GimpProcedure    *procedure,
                                                    GParamSpec       *pspec);

GimpValueArray * gimp_procedure_get_arguments      (GimpProcedure    *procedure);
GimpValueArray * gimp_procedure_get_return_values  (GimpProcedure    *procedure,
                                                    gboolean          success,
                                                    const GError     *error);

GimpProcedure  * gimp_procedure_create_override    (GimpProcedure    *procedure,
                                                    GimpMarshalFunc   new_marshal_func);

GimpValueArray * gimp_procedure_execute            (GimpProcedure    *procedure,
                                                    Gimp             *gimp,
                                                    GimpContext      *context,
                                                    GimpProgress     *progress,
                                                    GimpValueArray   *args,
                                                    GError          **error);
void             gimp_procedure_execute_async      (GimpProcedure    *procedure,
                                                    Gimp             *gimp,
                                                    GimpContext      *context,
                                                    GimpProgress     *progress,
                                                    GimpValueArray   *args,
                                                    GimpDisplay      *display,
                                                    GError          **error);

gint             gimp_procedure_name_compare       (GimpProcedure    *proc1,
                                                    GimpProcedure    *proc2);



#endif  /*  __GIMP_PROCEDURE_H__  */
