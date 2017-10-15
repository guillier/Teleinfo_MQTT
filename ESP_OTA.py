#!/usr/bin/env python3

#
# The MIT License (MIT)
# Copyright (c) 2015-2017 François GUILLIER <dev @ guillier . org>
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

# The arduino software must be loaded and sketch compiled
# before lauching this software !


# http://esp8266.github.io/Arduino/versions/2.3.0/doc/ota_updates/readme.html

from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib import parse
import hashlib
import os
import sys

# HTTPRequestHandler class
class RequestHandler(BaseHTTPRequestHandler):

  # GET
  def do_GET(self):
        parsed_path = parse.urlparse(self.path)
        print(parsed_path, self.path)

        m = hashlib.md5()
        try:
            with open('/tmp/{}{}'.format(build_dir, parsed_path.path), 'rb') as f:
                firmware = f.read()

            m.update(firmware);
            print('local firmware: ' + m.hexdigest())
        except:
            print("""Can't load local firmware""")
            return


        for name, value in self.headers.items():
            #print(name, value)
            if name[:10] == 'x-ESP8266-':
                print(name[10:], '->', value)

        if self.headers.get('x-ESP8266-version') == m.hexdigest():
            self.send_response(304)
            return

        # Send response status code
        self.send_response(200)

        # Send headers
        self.send_header('Content-type','application/octet-stream')
        self.send_header('Content-Disposition','attachment; filename=firmware.ino')
        self.send_header('Content-Length', len(firmware))
        self.end_headers()

        self.wfile.write(firmware)
        return

build_dir = ""
for d in os.listdir('/tmp'):
    if d[:14] == 'arduino_build_' and d > build_dir:
        build_dir = d

if len(build_dir) == 0:
    print("""Can't find Arduino Build Directory""")
    sys.exit(1)

server_address = ('0.0.0.0', 8888)
httpd = HTTPServer(server_address, RequestHandler)
print('running server...')
httpd.serve_forever()
