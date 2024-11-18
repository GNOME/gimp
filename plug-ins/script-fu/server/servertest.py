#!/usr/bin/env python3

# updated to python3 - cameron

# A simple client app to the ScriptFu server.
# Usually for testing.
# CLI, a REPL: submits each entered line to the server, then prints response.

import readline, socket, sys

if len(sys.argv) < 1 or len(sys.argv) > 3:
   print >>sys.stderr, "Usage: %s <host> <port>" % sys.argv[0]
   print >>sys.stderr, "       (if omitted connect to localhost, port 10008)"
   sys.exit(1)

HOST = "localhost"
PORT = 10008

try:
    HOST = sys.argv[1]
    try:
        PORT = int(sys.argv[2])
    except IndexError:
        pass
except IndexError:
    pass

addresses = socket.getaddrinfo(HOST, PORT, socket.AF_UNSPEC, socket.SOCK_STREAM)

connected = False

for addr in addresses:
    (family, socktype, proto, canonname, sockaddr) = addr

    numeric_addr = sockaddr[0]

    if canonname:
        print ("Trying %s ('%s')." % (numeric_addr, canonname))
    else:
        print ("Trying %s." % numeric_addr)

    try:
        sock = socket.socket(family, socket.SOCK_STREAM)
        sock.connect((HOST, PORT))
        connected = True
        break
    except:
        pass

if not connected:
    print ("Failed.")
    sys.exit(1)

try:
   cmd = input("Script-Fu-Remote - Testclient\n> ")

   while len(cmd) > 0:
      my_bytes = bytearray()
      my_bytes.append(71) # G=71
      my_bytes.append((len(cmd)) >> 8)
      my_bytes.append((len(cmd)) % 0xFF)
      my_bytes.extend(bytearray(cmd,"UTF-8"))
      sock.send(my_bytes)

      data = bytearray()
      while len(data) < 4:
         data.extend(sock.recv(4 - len(data)))

      if len(data) >= 4:
         if data[0] == 71: # MAGIC_BYTE 'G'=71
            l = (data[2] << 8) + data[3]
            msg = ""
            while len(msg) < l:
               msg += (sock.recv(l - len(msg))).decode("utf-8")
            if data[1]: # ERROR_BYTE
               print ("(ERR):", msg)
            else:
               print (" (OK):", msg)
            cmd = input("> ")
         else:
            print ("invalid magic: %s\n" % data)
      else:
         print ("short response: %s\n" % data)

except EOFError:
   print

sock.close()