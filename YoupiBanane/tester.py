#!/usr/bin/env python3
import sys
import os

# 1. Output the required CGI headers (must end with a blank line)
print("Content-Type: text/plain; charset=utf-8")
print()

# 2. Get the content length from the environment variables
content_length_str = os.environ.get('CONTENT_LENGTH')
request_method = os.environ.get('REQUEST_METHOD')

if request_method == 'POST':
    if content_length_str:
        try:
            # Convert the content length to an integer
            length = int(content_length_str)
            
            print(f"--- SUCCESS: Received POST request with {length} bytes ---")
            
            # 3. Read the exact number of bytes from standard input
            # Reading exactly 'length' prevents the script from hanging forever
            body = sys.stdin.read(length)
            
            print("--- POST BODY CONTENT ---")
            print(body)
            print("--- END OF CONTENT ---")
            
        except ValueError:
            print("Error: CONTENT_LENGTH is not a valid integer.")
    else:
        print("Error: No CONTENT_LENGTH provided by the server.")
else:
    print(f"This script expects a POST request, but received a {request_method} request.")
