/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * tips-parser.h - Parse the gimp-tips.xml file.
 * Copyright (C) 2002, 2008  Sven Neumann <sven@gimp.org>
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

#ifndef __TIPS_PARSER_H__
#define __TIPS_PARSER_H__


typedef struct _GimpTip GimpTip;

struct _GimpTip
{
  gchar *text;
  gchar *help_id;
};


GimpTip * gimp_tip_new        (const gchar  *title,
                               const gchar  *format,
                               ...) G_GNUC_PRINTF(2, 3);
void      gimp_tip_free       (GimpTip      *tip);

GList   * gimp_tips_from_file (GFile        *file,
                               GError      **error);
void      gimp_tips_free      (GList        *tips);


#endif /* __TIPS_PARSER_H__ */
