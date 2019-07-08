/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 2009 Martin Nordholts
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

#ifndef __TESTS_H__
#define __TESTS_H__

/* Automake doc says:
   "When no test protocol is in use, an exit status of 0 from a test
   script will denote a success, an exit status of 77 a skipped test,
   an exit status of 99 an hard error, and any other exit status will
   denote a failure."

   Unfortunately glib returns a SUCCESS when you skip tests, which is
   not a reliable test feedback. So we hard-code the SKIPPED return
   value. */
#define GIMP_EXIT_TEST_SKIPPED 77

Gimp * gimp_init_for_testing             (void);
Gimp * gimp_init_for_gui_testing         (gboolean  show_gui);
Gimp * gimp_init_for_gui_testing_with_rc (gboolean  show_gui,
                                          GFile    *gimprc);
void   gimp_test_run_temp_mainloop       (guint32   running_time);
void   gimp_test_run_mainloop_until_idle (void);
void   gimp_test_bail_if_no_display      (void);


#endif /* __TESTS_H__ */
