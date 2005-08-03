/* A small test program to check our MD5 implementation.
 * The test program computes the MD5 digest of some strings
 * as given in section A.5 of RFC 1321, reproduced below.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "gimpmd5.h"


static const gchar * test[7][2] =
{
  { "", "d41d8cd98f00b204e9800998ecf8427e" },
  { "a", "0cc175b9c0f1b6a831c399e269772661" },
  { "abc", "900150983cd24fb0d6963f7d28e17f72" },
  { "message digest", "f96b697d7cb7938d525a2f31aaf161d0" },
  { "abcdefghijklmnopqrstuvwxyz", "c3fcd3d76192e4007dfb496cca67e13b" },
  { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "d174ab98d277d9f5a5611c2c9f419d9f" },
  { "12345678901234567890123456789012345678901234567890123456789012345678901234567890", "57edf4a22be3c955ac49da2e2107b67a" }
};

int
main (void)
{
  gboolean correct = TRUE;
  guchar   digest[16];
  gint     i, j;

  g_print ("Testing the GIMP MD5 implementation ...\n\n");

  for (i = 0; i < 7; ++i)
    {
      gimp_md5_get_digest (test[i][0], strlen (test[i][0]), digest);

      g_print ("MD5 (\"%s\") = ", test[i][0]);

      for (j = 0; j < 16; j++)
        {
          gchar buf[4];

          g_snprintf (buf, 3, "%02x", digest[j]);
          g_print (buf);

          if (strncmp (buf, test[i][1] + j*2, 2))
            correct = FALSE;
        }
      g_print ("\n");

      if (!correct)
        {
          g_print ("\nWRONG digest!! "
                   "Please report to http://bugzilla.gnome.org/\n");
          return EXIT_FAILURE;
        }
    }

  g_print ("\nLooks good.\n\n");

  return EXIT_SUCCESS;
}

