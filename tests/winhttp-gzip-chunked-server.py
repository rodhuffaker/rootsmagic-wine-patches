from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
import gzip,time
class Handler(BaseHTTPRequestHandler):
 protocol_version='HTTP/1.1'
 def do_GET(self):
  body=gzip.compress(b'{"probe":"ok"}')
  self.send_response(200)
  self.send_header('Content-Type','application/json')
  self.send_header('Content-Encoding','gzip')
  self.send_header('Transfer-Encoding','chunked')
  self.end_headers()
  self.wfile.write(('%x\r\n'%len(body)).encode()+body+b'\r\n');self.wfile.flush()
  time.sleep(.1)
  self.wfile.write(b'0\r\n\r\n');self.wfile.flush()
 def log_message(self,*args):pass
ThreadingHTTPServer(('127.0.0.1',19717),Handler).serve_forever()
