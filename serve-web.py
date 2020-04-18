#!/usr/bin/env python3

# Sample code from: https://stackoverflow.com/questions/39801718/how-to-run-a-http-server-which-serves-a-specific-path

import http.server
import socketserver
import os

PORT = 1234

web_dir = os.path.join(os.path.dirname(__file__), 'web')
os.chdir('build-web/dist')

Handler = http.server.SimpleHTTPRequestHandler
httpd = socketserver.TCPServer(("", PORT), Handler)
print("serving at port", PORT)
httpd.serve_forever()
