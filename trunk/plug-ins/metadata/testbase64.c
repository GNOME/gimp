/* Small test program to test the base64 encoding and decoding */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "base64.h"

static int
string_encode_decode (char *s)
{
  int  n;
  char encoded[300];
  char decoded[400];

  n = base64_encode (s, strlen (s), encoded, sizeof (encoded) - 1, 0);
  if (n < 0)
    {
      g_print ("base64 encoding failed for '%s'", s);
      return 1;
    }
  g_print ("'%s' -> '%s' (%d) ", s, encoded, n);
  n = base64_decode (encoded, strlen (encoded), decoded, sizeof (decoded) - 1,
                     FALSE);
  if (n < 0)
    {
      g_print ("\nbase64 decoding failed for '%s'", s);
      return 1;
    }
  if (! strcmp (s, decoded))
    g_print ("-> '%s' (%d)\n", decoded, n);
  else
    {
      g_print ("-> '%s' (%d) MISMATCH!\n", decoded, n);
      g_print ("decoded buffer does not match original!\n");
      return 1;
    }
  return 0;
}

static int
buffer_encode_decode (char *buf,
                      gint  buf_len,
                      gint  columns)
{
  int  n;
  char encoded[3000];
  char decoded[4000];

  n = base64_encode (buf, buf_len, encoded, sizeof (encoded) - 1, columns);
  if (n < 0)
    {
      g_print ("base64 encoding failed");
      return 1;
    }
  g_print ("buffer length %d -> encoded %d (columns: %d) ", buf_len, n,
           columns);
  n = base64_decode (encoded, strlen (encoded), decoded, sizeof (decoded) - 1,
                     FALSE);
  if (n < 0)
    {
      g_print ("\nbase64 decoding failed");
      return 1;
    }
  if ((n == buf_len) && ! memcmp (buf, decoded, n))
    g_print ("-> decoded %d match OK\n", n);
  else
    {
      g_print ("-> decoded %d MISMATCH!\n", n);
      g_print ("decoded buffer does not match original!\n");
      return 1;
    }
  return 0;
}

static int
test_decode (char     *encoded,
             char     *expected,
             gint      expected_len,
             gboolean  ignore_errors)
{
  int  n;
  char decoded[4000];

  n = base64_decode (encoded, strlen (encoded), decoded, sizeof (decoded) - 1,
                     ignore_errors);
  if (n < 0)
    {
      if (expected_len < 0)
        g_print ("'%s' failed as expected\n", encoded);
      else
        {
          g_print ("'%s' could not be decoded (length: %d, expected %d)\n",
                   encoded, n, expected_len);
          return 1;
        }
    }
  else if ((n == expected_len) && ! memcmp (expected, decoded, n))
    {
      if (ignore_errors)
        g_print ("'%s' decoded OK (%d) - invalid chars ignored\n", encoded, n);
      else
        g_print ("'%s' decoded OK (%d)\n", encoded, n);
    }
  else
    {
      g_print ("'%s' not decoded as expected (length: %d, expected %d)\n",
               encoded, n, expected_len);
      return 1;
    }
  return 0;
}

int
main (int   argc,
      char *argv[])
{
  int  failed = 0;
  int  i;
  char buf[1000];

  g_print ("Testing base64 encoding ...\n");
  failed += string_encode_decode ("");
  failed += string_encode_decode ("A");
  failed += string_encode_decode ("AB");
  failed += string_encode_decode ("ABC");
  failed += string_encode_decode ("ABCD");
  failed += string_encode_decode ("ABCDE");
  failed += string_encode_decode ("ABCDEF");
  failed += string_encode_decode ("ABCDEFG");
  failed += string_encode_decode ("ABCDEFGH");
  failed += string_encode_decode ("ABCDEFGHI");
  failed += string_encode_decode ("abcdefghik");
  failed += string_encode_decode ("1234567890abcdefghijklmnopqrstuvwxyz");
  failed += string_encode_decode ("«© Raphaël»");
  for (i = 0; i < sizeof (buf); i++)
    buf[i] = (char) (i % 0xff);
  failed += buffer_encode_decode (buf, sizeof (buf), 0);
  failed += buffer_encode_decode (buf, sizeof (buf), 76);
  failed += buffer_encode_decode (buf, sizeof (buf), 4);
  failed += buffer_encode_decode (buf, sizeof (buf), 1);
  for (i = 0; i < sizeof (buf); i++)
    buf[i] = (char) (0xff - (i % 0xff));
  failed += buffer_encode_decode (buf, 600, 0);
  failed += buffer_encode_decode (buf, 500, 0);
  failed += buffer_encode_decode (buf, 400, 0);
  failed += test_decode ("QUJD", "ABC", 3, FALSE);
  failed += test_decode (" Q\tU  J\nDR\rA==", "ABCD", 4, FALSE);
  failed += test_decode ("?", "", -1, FALSE);
  failed += test_decode ("?", "", 0, TRUE);
  failed += test_decode ("////", "\377\377\377", 3, FALSE);
  failed += test_decode ("---/./(/)/*", "", -1, FALSE);
  failed += test_decode ("---/./(/)/*", "\377\377\377", 3, TRUE);
  failed += test_decode ("AA==", "\0", 1, FALSE);
  failed += test_decode ("AAA=", "\0\0", 2, FALSE);
  if (failed > 0)
    {
      g_print ("%d test(s) failed!\n", failed);
      return EXIT_FAILURE;
    }
  g_print ("No problems detected.\n");
  return EXIT_SUCCESS;
}
