from http.server import BaseHTTPRequestHandler,HTTPServer
import gzip
class Handler(BaseHTTPRequestHandler):
 def do_GET(self):
  body=gzip.compress(b'{"probe":"ok"}')
  self.send_response(200)
  self.send_header('Content-Type','application/json')
  self.send_header('Content-Encoding','gzip')
  self.send_header('Content-Length',str(len(body)))
  self.end_headers();self.wfile.write(body)
 def log_message(self,*args):pass
HTTPServer(('127.0.0.1',19717),Handler).serve_forever()
