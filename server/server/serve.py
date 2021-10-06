#!/usr/bin/env python3
import sys
import os
import signal
#import requests
#import urllib.parse

#from pyrfc3339 import parse
#from datetime import datetime
#import pytz

from http.server import SimpleHTTPRequestHandler, HTTPServer

from weather import Weather

def handler(signum, frame):
    print("Sigterm - exiting")
    sys.exit(1)
signal.signal(signal.SIGTERM, handler)

class RequestHandler(SimpleHTTPRequestHandler):
    def do_GET(self):
        print(self.headers.items())

        api_key=self.headers.get('apikey', os.getenv('APIKEY') )
        lon=self.headers.get('lon', 151.176422 )
        lat=self.headers.get('lat', -33.889648 )
        tz=self.headers.get( 'tz',  "Australia/Sydney" )

        if not api_key:
            self.send_response(400)
            message =  "Missing APIKEY"
            self.wfile.write(bytes(message, "utf8"))
            return


        composer = Weather(
            api_key=api_key,
            lat=lat, long=lon,
            timezone=tz
            )
        img = composer.render()

        # Send headers
        self.send_response(200)
        self.send_header('Content-type','image/png')
        self.end_headers()

        # Write content as utf-8 data
        img.seek( 0 )
        self.wfile.write( img.read() )
        return

def run():
    print('starting server...')
    server_address = ('0.0.0.0', 8003)
    httpd = HTTPServer(server_address, RequestHandler)
    print('running server...')
    sys.stdout.flush()
    httpd.serve_forever()

if __name__ == "__main__":
    run()
