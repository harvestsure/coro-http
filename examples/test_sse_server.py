#!/usr/bin/env python3
"""
Simple SSE test server for testing coro_http SSE functionality

Usage:
    python3 test_sse_server.py
    
Then run in another terminal:
    curl http://localhost:8888/events
    or
    ./build/example_sse_coro http://localhost:8888/events
"""

from http.server import HTTPServer, BaseHTTPRequestHandler
import time
import json
import sys
from datetime import datetime

class SSEHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/events':
            self.send_response(200)
            self.send_header('Content-Type', 'text/event-stream')
            self.send_header('Cache-Control', 'no-cache')
            self.send_header('Connection', 'keep-alive')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            
            self.log_message("SSE client connected")
            
            try:
                # Test event 1: Simple messages
                for i in range(3):
                    event_data = {
                        'message': f'Hello from SSE server - Message {i+1}',
                        'timestamp': datetime.now().isoformat()
                    }
                    self.wfile.write(f"event: message\n".encode())
                    self.wfile.write(f"id: {i+1}\n".encode())
                    self.wfile.write(f"data: {json.dumps(event_data)}\n\n".encode())
                    self.wfile.flush()
                    self.log_message(f"Sent message {i+1}")
                    time.sleep(1)
                
                # Test event 2: Multiline data
                multiline_data = """Line 1
Line 2
Line 3"""
                self.wfile.write(f"event: multiline\n".encode())
                self.wfile.write(f"id: 4\n".encode())
                self.wfile.write(f"data: {multiline_data.split(chr(10))[0]}\n".encode())
                self.wfile.write(f"data: {multiline_data.split(chr(10))[1]}\n".encode())
                self.wfile.write(f"data: {multiline_data.split(chr(10))[2]}\n\n".encode())
                self.wfile.flush()
                self.log_message("Sent multiline event")
                time.sleep(1)
                
                # Test event 3: Event with retry
                self.wfile.write(f"event: important\n".encode())
                self.wfile.write(f"id: 5\n".encode())
                self.wfile.write(f"retry: 5000\n".encode())
                self.wfile.write(f"data: This event suggests a 5 second retry\n\n".encode())
                self.wfile.flush()
                self.log_message("Sent retry event")
                time.sleep(1)
                
                # Test event 4: Custom fields
                self.wfile.write(f"event: custom\n".encode())
                self.wfile.write(f"id: 6\n".encode())
                self.wfile.write(f"custom_field: custom_value\n".encode())
                self.wfile.write(f"data: Event with custom fields\n\n".encode())
                self.wfile.flush()
                self.log_message("Sent custom field event")
                time.sleep(1)
                
                # Test event 5: Heartbeat comments
                for i in range(3):
                    self.wfile.write(b": heartbeat\n\n")
                    self.wfile.flush()
                    self.log_message(f"Sent heartbeat {i+1}")
                    time.sleep(1)
                
                # Stream end
                self.log_message("SSE stream ended")
                # Explicitly close connection
                self.wfile.close()
                
            except BrokenPipeError:
                self.log_message("Client disconnected")
            except ValueError:
                # File already closed, ignore
                pass
            
        elif self.path == '/':
            self.send_response(200)
            self.send_header('Content-Type', 'text/html; charset=utf-8')
            self.end_headers()
            
            html = """
            <!DOCTYPE html>
            <html>
            <head>
                <title>SSE Server Test</title>
                <style>
                    body { font-family: Arial, sans-serif; margin: 20px; }
                    #events { border: 1px solid #ccc; padding: 10px; height: 300px; overflow-y: auto; }
                    .event { padding: 5px; margin: 5px 0; background: #f0f0f0; border-radius: 3px; }
                </style>
            </head>
            <body>
                <h1>SSE Server - Test Page</h1>
                <p>Click button to start receiving SSE events...</p>
                <button onclick="startStream()">Start Stream</button>
                <button onclick="stopStream()">Stop Stream</button>
                <div id="events"></div>
                
                <script>
                    let eventSource = null;
                    
                    function startStream() {
                        if (eventSource) return;
                        
                        eventSource = new EventSource('/events');
                        document.getElementById('events').innerHTML = '<div class="event">已连接...</div>';
                        
                        eventSource.onmessage = function(event) {
                            addEvent('message', event.data);
                        };
                        
                        eventSource.addEventListener('multiline', function(e) {
                            addEvent('multiline', e.data);
                        });
                        
                        eventSource.addEventListener('important', function(e) {
                            addEvent('important', e.data);
                        });
                        
                        eventSource.addEventListener('custom', function(e) {
                            addEvent('custom', e.data);
                        });
                        
                        eventSource.onerror = function() {
                            addEvent('error', 'Connection error or closed');
                            eventSource.close();
                            eventSource = null;
                        };
                    }
                    
                    function stopStream() {
                        if (eventSource) {
                            eventSource.close();
                            eventSource = null;
                            addEvent('info', 'Connection closed');
                        }
                    }
                    
                    function addEvent(type, data) {
                        const div = document.getElementById('events');
                        const event = document.createElement('div');
                        event.className = 'event';
                        event.innerHTML = `<strong>${type}:</strong> ${data}`;
                        div.appendChild(event);
                        div.scrollTop = div.scrollHeight;
                    }
                </script>
            </body>
            </html>
            """
            self.wfile.write(html.encode())
        
        else:
            self.send_response(404)
            self.send_header('Content-Type', 'text/plain')
            self.end_headers()
            self.wfile.write(b'Not Found')
    
    def log_message(self, format, *args):
        # Print to stdout for debugging
        print(f"{self.log_date_time_string()} - {format % args}")


if __name__ == '__main__':
    host = 'localhost'
    port = 8888
    
    server = HTTPServer((host, port), SSEHandler)
    print(f"SSE test server started: http://{host}:{port}")
    print(f"")
    print(f"SSE endpoint: http://{host}:{port}/events")
    print(f"Web UI: http://{host}:{port}/")
    print(f"")
    print(f"Test from another terminal:")
    print(f"  curl http://{host}:{port}/events")
    print(f"  or edit sse_coro_example.cpp to use: http://{host}:{port}/events")
    print(f"")
    print(f"Press Ctrl+C to stop server...")
    
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n\nServer stopped")
        sys.exit(0)
