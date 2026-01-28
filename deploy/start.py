import http.server
import socketserver
import socket

PORT = 9000

Handler = http.server.SimpleHTTPRequestHandler

with socketserver.TCPServer(("0.0.0.0", PORT), Handler) as httpd:
    HOST = socket.gethostbyname(socket.gethostname())
    print(f"Server: http://{HOST}:{PORT}")
    httpd.serve_forever()