import threading
import zmq
from http.server import BaseHTTPRequestHandler, HTTPServer

class TestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path.startswith("/chunk/1337"):
            response_body = b"TEST_GLTF_DATA"
            self.send_response(200)
            self.send_header('Content-type', 'model/gltf-binary')
            self.send_header('Content-Length', str(len(response_body)))
            self.end_headers()
            self.wfile.write(response_body)
        else:
            self.send_response(404)
            self.send_header('Content-Length', '9')
            self.end_headers()
            self.wfile.write(b"Not Found")

    def log_message(self, format, *args):
        return

def start_http_server():
    server_address = ('0.0.0.0', 8080)
    httpd = HTTPServer(server_address, TestHandler)
    print("[HTTP Server] Listening on port 8080")
    httpd.serve_forever()

def send_zmq_message():
    context = zmq.Context()
    socket = context.socket(zmq.PUSH)
    socket.connect("tcp://192.168.1.75:5555")
    print("[ZMQ] Sending message...")
    socket.send_string("Hello from Docker test server!")
    print("[ZMQ] Message sent")

if __name__ == "__main__":
    threading.Thread(target=start_http_server, daemon=True).start()

    import time
    time.sleep(2)

    send_zmq_message()

    print("[Main] Press Ctrl+C to exit.")
    while True:
        time.sleep(1)
