#!/usr/bin/python3

import os

print("Content-Type: text/html\r")
print("\r")
print("""<!DOCTYPE html>
<html>
<head><title>Hello CGI</title></head>
<body>
    <h1>Hello from Python CGI!</h1>
    <p>This page was generated dynamically.</p>
    <p>Server: webserv</p>
    <a href="/cgi-bin">← Back to CGI directory</a>
</body>
</html>""")
