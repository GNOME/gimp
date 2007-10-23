#!/usr/bin/env python

import readline, socket, sys

if len (sys.argv) == 1:
   HOST = 'localhost'
   PORT = 10008
elif len (sys.argv) == 3:
   HOST = sys.argv[1]
   PORT = int (sys.argv[2])
else:
   print >> sys.stderr, "Usage: %s <host> <port>" % sys.argv[0]
   print >> sys.stderr, "       (if omitted connect to localhost, port 10008)"
   sys.exit ()


sock = socket.socket (socket.AF_INET, socket.SOCK_STREAM)
sock.connect ((HOST, PORT))

try:
   cmd = raw_input ("Script-Fu-Remote - Testclient\n> ")

   while len (cmd) > 0:
      sock.send ('G%c%c%s' % (len (cmd) / 256, len (cmd) % 256, cmd))

      data = ""
      while len (data) < 4:
         data += sock.recv (4 - len (data))

      if len (data) >= 4:
         if data[0] == 'G':
            l = ord (data[2]) * 256 + ord (data[3])
            msg = ""
            while len (msg) < l:
               msg += sock.recv (l - len (msg))
            if ord (data[1]):
               print "(ERR):", msg
            else:
               print " (OK):", msg
         else:
            print "invalid magic: %s\n" % data
      else:
         print "short response: %s\n" % data
      cmd = raw_input ("> ")

except EOFError:
   print

sock.close
