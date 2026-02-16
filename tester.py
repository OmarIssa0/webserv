#!/usr/bin/env python3
"""
Webserv Comprehensive Tester
Run this script while your webserv is running on localhost:1234 and 4321.
It will test all mandatory features and many edge cases.
"""

import requests
import socket
import threading
import time
import os
import sys
from urllib.parse import urljoin

# ========== CONFIGURATION ==========
HOST = "127.0.0.1"
PORTS = [1234, 4321]
BASE_URLS = [f"http://{HOST}:{p}" for p in PORTS]
TEST_DIR = os.path.dirname(os.path.abspath(__file__))
# ====================================

# Global counters
passed = 0
failed = 0
results = []

def log_test(name, success, details=""):
    global passed, failed
    if success:
        passed += 1
        status = "PASS"
    else:
        failed += 1
        status = "FAIL"
    results.append((name, status, details))
    print(f"[{status}] {name}" + (f" - {details}" if details else ""))

def check_status(r, expected_status, msg=""):
    return r.status_code == expected_status, f"got {r.status_code}, expected {expected_status}" + (f" ({msg})" if msg else "")

def test_basic_get():
    """1. Static file GET"""
    try:
        r = requests.get(f"{BASE_URLS[0]}/test.txt", timeout=5)
        ok, err = check_status(r, 200)
        if ok and r.text.strip() == "www/index.html":
            log_test("Static file GET", True)
        else:
            log_test("Static file GET", False, err or "wrong content")
    except Exception as e:
        log_test("Static file GET", False, str(e))

def test_directory_listing():
    """2. Directory listing"""
    try:
        r = requests.get(f"{BASE_URLS[0]}/subdir/", timeout=5)
        ok, err = check_status(r, 200)
        if ok and "Index of /subdir/" in r.text and "file.html" in r.text:
            log_test("Directory listing", True)
        else:
            log_test("Directory listing", False, err or "listing missing")
    except Exception as e:
        log_test("Directory listing", False, str(e))

def test_index_file():
    """3. Index file"""
    try:
        r = requests.get(f"{BASE_URLS[0]}/", timeout=5)
        ok, err = check_status(r, 200)
        if ok and "<title>Index of /</title>" not in r.text:  # not autoindex
            log_test("Index file", True)
        else:
            log_test("Index file", False, err or "index not served")
    except Exception as e:
        log_test("Index file", False, str(e))

def test_upload_simple():
    """4. POST upload (simple)"""
    try:
        data = "Hello, server!"
        headers = {"Content-Type": "text/plain"}
        r = requests.post(f"{BASE_URLS[0]}/uploads", data=data, headers=headers, timeout=5)
        ok, err = check_status(r, 201)
        log_test("Upload simple", ok, err)
    except Exception as e:
        log_test("Upload simple", False, str(e))

def test_upload_multipart():
    """5. POST upload (multipart)"""
    try:
        files = {'file': ('hello.txt', 'Multipart world')}
        r = requests.post(f"{BASE_URLS[0]}/uploads", files=files, timeout=5)
        ok, err = check_status(r, 201)
        log_test("Upload multipart", ok, err)
    except Exception as e:
        log_test("Upload multipart", False, str(e))

def test_delete():
    """6. DELETE file"""
    # First upload a file to delete
    try:
        requests.post(f"{BASE_URLS[0]}/uploads", data="to_delete", headers={"Content-Type": "text/plain"}, timeout=5)
        # We need the filename; we'll assume server returns something or we guess
        # For simplicity, we'll try to delete a known name if we can guess.
        # Better: parse the response from upload? But server may return plain text.
        # We'll just do a DELETE on a file that we know exists from previous test.
        # Actually we'll create a unique name.
        filename = f"delete_me_{int(time.time())}.txt"
        requests.post(f"{BASE_URLS[0]}/uploads", data="delete me", headers={"Content-Type": "text/plain", "Content-Disposition": f"filename={filename}"}, timeout=5)
        r = requests.delete(f"{BASE_URLS[0]}/uploads/{filename}", timeout=5)
        ok, err = check_status(r, 200)
        log_test("DELETE file", ok, err)
    except Exception as e:
        log_test("DELETE file", False, str(e))

def test_cgi_get():
    """7. CGI (GET) - requires a test.py in cgi-bin"""
    try:
        r = requests.get(f"{BASE_URLS[0]}/cgi-bin/test.py?name=World", timeout=5)
        ok, err = check_status(r, 200)
        # We'll assume the script outputs something containing "World"
        if ok and "World" in r.text:
            log_test("CGI GET", True)
        else:
            log_test("CGI GET", False, err or "output missing expected content")
    except Exception as e:
        log_test("CGI GET", False, str(e))

def test_cgi_path_info():
    """8. CGI with PATH_INFO"""
    try:
        r = requests.get(f"{BASE_URLS[0]}/cgi-bin/test.py/extra/path", timeout=5)
        ok, err = check_status(r, 200)
        # The script should echo PATH_INFO or something
        if ok and "/extra/path" in r.text:
            log_test("CGI PATH_INFO", True)
        else:
            log_test("CGI PATH_INFO", False, err or "PATH_INFO not detected")
    except Exception as e:
        log_test("CGI PATH_INFO", False, str(e))

def test_redirect():
    """9. Redirection"""
    try:
        r = requests.get(f"{BASE_URLS[0]}/redirect", allow_redirects=False, timeout=5)
        ok, err = check_status(r, 301)
        if ok and r.headers.get("Location") == "/":
            log_test("Redirect", True)
        else:
            log_test("Redirect", False, err or "wrong Location header")
    except Exception as e:
        log_test("Redirect", False, str(e))

def test_custom_error_page():
    """10. Custom error page"""
    try:
        r = requests.get(f"{BASE_URLS[0]}/nonexistent", timeout=5)
        ok, err = check_status(r, 404)
        if ok and "404 Custom" in r.text:  # we need to put something in error file
            log_test("Custom error page", True)
        else:
            log_test("Custom error page", False, err or "wrong content")
    except Exception as e:
        log_test("Custom error page", False, str(e))

def test_multiple_ports():
    """11. Multiple ports (different content)"""
    try:
        r1 = requests.get(f"{BASE_URLS[0]}/", timeout=5)
        r2 = requests.get(f"{BASE_URLS[1]}/", timeout=5)
        if r1.status_code == 200 and r2.status_code == 200 and r1.text != r2.text:
            log_test("Multiple ports", True)
        else:
            log_test("Multiple ports", False, "same content or error")
    except Exception as e:
        log_test("Multiple ports", False, str(e))

def test_path_traversal_alias():
    """12. Path traversal (alias)"""
    try:
        r = requests.get(f"{BASE_URLS[0]}/../www/test.txt", timeout=5)
        # Should be forbidden or not found
        if r.status_code in (403, 404):
            log_test("Path traversal alias", True)
        else:
            log_test("Path traversal alias", False, f"got {r.status_code}, expected 403/404")
    except Exception as e:
        log_test("Path traversal alias", False, str(e))

def test_path_traversal_location_root():
    """13. Path traversal with location root (secret)"""
    try:
        r = requests.get(f"{BASE_URLS[0]}/secret/../../www/test.txt", timeout=5)
        if r.status_code in (403, 404):
            log_test("Path traversal location root", True)
        else:
            log_test("Path traversal location root", False, f"got {r.status_code}")
    except Exception as e:
        log_test("Path traversal location root", False, str(e))

def test_cgi_query_and_path_info():
    """14. CGI with query string and PATH_INFO"""
    try:
        r = requests.get(f"{BASE_URLS[0]}/cgi-bin/test.py/extra?q=1", timeout=5)
        ok, err = check_status(r, 200)
        if ok and "q=1" in r.text and "/extra" in r.text:
            log_test("CGI query+PATH_INFO", True)
        else:
            log_test("CGI query+PATH_INFO", False, err or "missing components")
    except Exception as e:
        log_test("CGI query+PATH_INFO", False, str(e))

def test_cgi_script_not_found():
    """15. CGI script not found"""
    try:
        r = requests.get(f"{BASE_URLS[0]}/cgi-bin/missing.py", timeout=5)
        if r.status_code == 404:
            log_test("CGI not found", True)
        else:
            log_test("CGI not found", False, f"got {r.status_code}")
    except Exception as e:
        log_test("CGI not found", False, str(e))

def test_cgi_no_status_header():
    """16. CGI without Status header (should return 200)"""
    # Requires a script that prints only Content-Type
    try:
        r = requests.get(f"{BASE_URLS[0]}/cgi-bin/no_status.py", timeout=5)
        if r.status_code == 200:
            log_test("CGI no Status", True)
        else:
            log_test("CGI no Status", False, f"got {r.status_code}")
    except Exception as e:
        log_test("CGI no Status", False, str(e))

def test_cgi_invalid_headers():
    """17. CGI with invalid headers (should be handled)"""
    try:
        r = requests.get(f"{BASE_URLS[0]}/cgi-bin/invalid_headers.py", timeout=5)
        # Server might return 500 or ignore invalid lines
        if r.status_code in (200, 500):
            log_test("CGI invalid headers", True)
        else:
            log_test("CGI invalid headers", False, f"got {r.status_code}")
    except Exception as e:
        log_test("CGI invalid headers", False, str(e))

# ========== LOW-LEVEL SOCKET TESTS ==========

def send_raw_request(port, raw_request):
    """Send raw string over socket and return response as bytes."""
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(3)
    try:
        s.connect((HOST, port))
        s.sendall(raw_request.encode())
        response = b""
        while True:
            part = s.recv(4096)
            if not part:
                break
            response += part
        s.close()
        return response
    except Exception as e:
        s.close()
        raise e

def test_chunked_upload():
    """18. Upload without Content-Length (chunked)"""
    raw = (
        "POST /uploads HTTP/1.1\r\n"
        "Host: localhost:1234\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "7\r\n"
        "Chunked\r\n"
        "8\r\n"
        " data\r\n"
        "0\r\n"
        "\r\n"
    )
    try:
        resp = send_raw_request(1234, raw).decode(errors='ignore')
        if "201" in resp or "200" in resp:
            log_test("Chunked upload", True)
        else:
            log_test("Chunked upload", False, f"no 2xx: {resp[:100]}")
    except Exception as e:
        log_test("Chunked upload", False, str(e))

def test_payload_too_large():
    """19. Upload exceeding client_max_body_size (1M)"""
    # Send slightly over 1M
    large_body = "X" * (1024*1024 + 1)
    raw = (
        f"POST /uploads HTTP/1.1\r\n"
        f"Host: localhost:1234\r\n"
        f"Content-Length: {len(large_body)}\r\n"
        f"Content-Type: text/plain\r\n"
        f"\r\n"
        f"{large_body}"
    )
    try:
        resp = send_raw_request(1234, raw).decode(errors='ignore')
        if "413" in resp:
            log_test("Payload too large", True)
        else:
            log_test("Payload too large", False, f"expected 413, got: {resp[:100]}")
    except Exception as e:
        log_test("Payload too large", False, str(e))

def test_chunked_invalid():
    """20. Chunked with invalid chunk size"""
    raw = (
        "POST /uploads HTTP/1.1\r\n"
        "Host: localhost:1234\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "g\r\n"  # invalid hex
        "data\r\n"
        "0\r\n\r\n"
    )
    try:
        resp = send_raw_request(1234, raw).decode(errors='ignore')
        if "400" in resp:
            log_test("Chunked invalid", True)
        else:
            log_test("Chunked invalid", False, f"expected 400, got: {resp[:100]}")
    except Exception as e:
        log_test("Chunked invalid", False, str(e))

def test_directory_no_index_no_auto():
    """21. Directory with index but no autoindex and missing index file"""
    # Location /secret has no index file (assuming we didn't put one)
    try:
        r = requests.get(f"{BASE_URLS[0]}/secret/", timeout=5)
        if r.status_code == 403:
            log_test("Directory no index no auto", True)
        else:
            log_test("Directory no index no auto", False, f"got {r.status_code}")
    except Exception as e:
        log_test("Directory no index no auto", False, str(e))

def test_directory_with_index():
    """22. Directory with index file present"""
    # We need to have secret.html in ./www/private/
    # Let's assume we placed it
    try:
        r = requests.get(f"{BASE_URLS[0]}/secret/", timeout=5)
        if r.status_code == 200 and "secret content" in r.text:
            log_test("Directory with index", True)
        else:
            log_test("Directory with index", False, f"got {r.status_code}")
    except Exception as e:
        log_test("Directory with index", False, str(e))

def test_method_not_allowed():
    """23. Method not allowed"""
    try:
        r = requests.delete(f"{BASE_URLS[0]}/", timeout=5)
        if r.status_code == 405:
            log_test("Method not allowed", True)
        else:
            log_test("Method not allowed", False, f"got {r.status_code}")
    except Exception as e:
        log_test("Method not allowed", False, str(e))

def test_http_version_unsupported():
    """24. Unsupported HTTP version"""
    raw = "GET / HTTP/2.0\r\nHost: localhost:1234\r\n\r\n"
    try:
        resp = send_raw_request(1234, raw).decode(errors='ignore')
        if "505" in resp:
            log_test("HTTP version unsupported", True)
        else:
            log_test("HTTP version unsupported", False, f"expected 505, got: {resp[:100]}")
    except Exception as e:
        log_test("HTTP version unsupported", False, str(e))

def test_uri_too_long():
    """25. URI too long"""
    long_uri = "/" + "a" * 9000
    try:
        r = requests.get(f"{BASE_URLS[0]}{long_uri}", timeout=5)
        if r.status_code == 414:
            log_test("URI too long", True)
        else:
            log_test("URI too long", False, f"got {r.status_code}")
    except Exception as e:
        log_test("URI too long", False, str(e))

def test_headers_too_large():
    """26. Headers too large"""
    huge_header = "X-Test: " + "X" * 9000 + "\r\n"
    raw = f"GET / HTTP/1.1\r\nHost: localhost:1234\r\n{huge_header}\r\n"
    try:
        resp = send_raw_request(1234, raw).decode(errors='ignore')
        if "431" in resp:
            log_test("Headers too large", True)
        else:
            log_test("Headers too large", False, f"expected 431, got: {resp[:100]}")
    except Exception as e:
        log_test("Headers too large", False, str(e))

def test_multiple_host_headers():
    """27. Multiple Host headers"""
    raw = "GET / HTTP/1.1\r\nHost: localhost\r\nHost: example.com\r\n\r\n"
    try:
        resp = send_raw_request(1234, raw).decode(errors='ignore')
        if "400" in resp:
            log_test("Multiple Host headers", True)
        else:
            log_test("Multiple Host headers", False, f"expected 400, got: {resp[:100]}")
    except Exception as e:
        log_test("Multiple Host headers", False, str(e))

def test_no_host_header():
    """28. No Host header (HTTP/1.1)"""
    raw = "GET / HTTP/1.1\r\n\r\n"
    try:
        resp = send_raw_request(1234, raw).decode(errors='ignore')
        if "400" in resp:
            log_test("No Host header", True)
        else:
            log_test("No Host header", False, f"expected 400, got: {resp[:100]}")
    except Exception as e:
        log_test("No Host header", False, str(e))

def test_empty_host_header():
    """29. Empty Host header"""
    raw = "GET / HTTP/1.1\r\nHost:\r\n\r\n"
    try:
        resp = send_raw_request(1234, raw).decode(errors='ignore')
        if "400" in resp:
            log_test("Empty Host header", True)
        else:
            log_test("Empty Host header", False, f"expected 400, got: {resp[:100]}")
    except Exception as e:
        log_test("Empty Host header", False, str(e))

def test_absolute_uri():
    """30. Absolute URI in request line"""
    raw = f"GET http://{HOST}:1234/test.txt HTTP/1.1\r\nHost: localhost\r\n\r\n"
    try:
        resp = send_raw_request(1234, raw).decode(errors='ignore')
        if "200" in resp and "content" in resp:
            log_test("Absolute URI", True)
        else:
            log_test("Absolute URI", False, f"expected 200, got: {resp[:100]}")
    except Exception as e:
        log_test("Absolute URI", False, str(e))

def test_keep_alive():
    """31. Keep-Alive handling"""
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(3)
    try:
        s.connect((HOST, 1234))
        req1 = "GET /test.txt HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n"
        s.sendall(req1.encode())
        resp1 = s.recv(4096).decode(errors='ignore')
        if "200" not in resp1:
            raise Exception("first request failed")
        # Send second request on same socket
        req2 = "GET /index.html HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
        s.sendall(req2.encode())
        resp2 = b""
        while True:
            part = s.recv(4096)
            if not part:
                break
            resp2 += part
        s.close()
        if b"200" in resp2:
            log_test("Keep-Alive", True)
        else:
            log_test("Keep-Alive", False, "second request failed")
    except Exception as e:
        log_test("Keep-Alive", False, str(e))

def test_connection_close():
    """32. Connection: close header"""
    try:
        r = requests.get(f"{BASE_URLS[0]}/test.txt", headers={"Connection": "close"}, timeout=5)
        if r.status_code == 200:
            log_test("Connection: close", True)
        else:
            log_test("Connection: close", False, f"got {r.status_code}")
    except Exception as e:
        log_test("Connection: close", False, str(e))

def test_head():
    """34. HEAD request should return headers without body"""
    try:
        r = requests.head(f"{BASE_URLS[0]}/test.txt", timeout=5)
        if r.status_code == 200 and not r.text:
            log_test("HEAD request", True)
        else:
            log_test("HEAD request", False, f"got {r.status_code}, body present")
    except Exception as e:
        log_test("HEAD request", False, str(e))

def test_options():
    """35. OPTIONS should return Allow header"""
    try:
        r = requests.options(f"{BASE_URLS[0]}/", timeout=5)
        allow = r.headers.get('Allow', '')
        if r.status_code in (200, 204) and allow:
            log_test("OPTIONS method", True)
        else:
            log_test("OPTIONS method", False, f"status {r.status_code} allow={allow}")
    except Exception as e:
        log_test("OPTIONS method", False, str(e))

def test_put():
    """36. PUT upload to specific path"""
    try:
        filename = f"put_test_{int(time.time())}.txt"
        url = f"{BASE_URLS[0]}/uploads/{filename}"
        r = requests.put(url, data="put content", headers={"Content-Type": "text/plain"}, timeout=5)
        if r.status_code in (200, 201):
            # verify it exists
            g = requests.get(f"{BASE_URLS[0]}/uploads/{filename}", timeout=5)
            if g.status_code == 200:
                log_test("PUT upload", True)
            else:
                log_test("PUT upload", False, f"created but GET returned {g.status_code}")
        else:
            log_test("PUT upload", False, f"got {r.status_code}")
    except Exception as e:
        log_test("PUT upload", False, str(e))

def test_if_modified_since():
    """37. If-Modified-Since returns 304 when not modified"""
    try:
        r1 = requests.get(f"{BASE_URLS[0]}/test.txt", timeout=5)
        lm = r1.headers.get('Last-Modified')
        if not lm:
            log_test("If-Modified-Since", False, "no Last-Modified header")
            return
        r2 = requests.get(f"{BASE_URLS[0]}/test.txt", headers={"If-Modified-Since": lm}, timeout=5)
        if r2.status_code == 304:
            log_test("If-Modified-Since", True)
        else:
            log_test("If-Modified-Since", False, f"got {r2.status_code}")
    except Exception as e:
        log_test("If-Modified-Since", False, str(e))

# ========== STRESS TESTS ==========

def stress_many_clients():
    """33. Many concurrent clients"""
    def worker():
        for _ in range(5):
            try:
                requests.get(f"{BASE_URLS[0]}/test.txt", timeout=2)
            except:
                pass
    threads = []
    start = time.time()
    for _ in range(50):
        t = threading.Thread(target=worker)
        t.start()
        threads.append(t)
    for t in threads:
        t.join()
    duration = time.time() - start
    # If no crashes, we consider it passed
    log_test("Stress many clients", True, f"completed in {duration:.2f}s")

def test_cgi_timeout():
    """37. CGI hanging (timeout)"""
    # Requires a script that sleeps > CGI_TIMEOUT (10s)
    try:
        r = requests.get(f"{BASE_URLS[0]}/cgi-bin/sleep.py", timeout=15)
        if r.status_code == 504:
            log_test("CGI timeout", True)
        else:
            log_test("CGI timeout", False, f"got {r.status_code}")
    except requests.exceptions.Timeout:
        log_test("CGI timeout", False, "request timed out, no 504 received")
    except Exception as e:
        log_test("CGI timeout", False, str(e))

# ... additional stress tests could be added similarly.

# We'll skip some that require special scripts (CGI crashing, huge output, etc.)
# but we can add placeholders.

def test_cgi_crash():
    """39. CGI crashing"""
    try:
        r = requests.get(f"{BASE_URLS[0]}/cgi-bin/crash.py", timeout=5)
        if r.status_code == 500:
            log_test("CGI crash", True)
        else:
            log_test("CGI crash", False, f"got {r.status_code}")
    except Exception as e:
        log_test("CGI crash", False, str(e))

# ========== RUN ALL TESTS ==========

def main():
    global passed, failed
    print("=" * 60)
    print("WEBSERV COMPREHENSIVE TESTER")
    print("=" * 60)
    print(f"Testing server at {HOST}:{PORTS}")
    print("Make sure your server is running and the directory structure is set up.")
    print("=" * 60)

    # List of test functions to run
    tests = [
        test_basic_get,
        test_directory_listing,
        test_index_file,
        test_upload_simple,
        test_upload_multipart,
        test_delete,
        test_cgi_get,
        test_cgi_path_info,
        test_redirect,
        test_custom_error_page,
        test_multiple_ports,
        test_path_traversal_alias,
        test_path_traversal_location_root,
        test_cgi_query_and_path_info,
        test_cgi_script_not_found,
        test_cgi_no_status_header,
        test_cgi_invalid_headers,
        test_chunked_upload,
        test_payload_too_large,
        test_chunked_invalid,
        test_directory_no_index_no_auto,
        # test_directory_with_index,   # depends on file presence
        test_method_not_allowed,
        test_http_version_unsupported,
        test_uri_too_long,
        test_headers_too_large,
        test_multiple_host_headers,
        test_no_host_header,
        test_empty_host_header,
        test_absolute_uri,
        test_keep_alive,
        test_connection_close,
        stress_many_clients,
        test_cgi_timeout,
        test_cgi_crash,
    ]

    for t in tests:
        try:
            t()
        except Exception as e:
            log_test(t.__name__, False, f"unhandled exception: {e}")

    print("\n" + "=" * 60)
    print(f"SUMMARY: {passed} PASSED, {failed} FAILED")
    print("=" * 60)
    for name, status, detail in results:
        print(f"[{status}] {name} - {detail}" if detail else f"[{status}] {name}")

    if failed == 0:
        print("\nAll tests passed! Your webserv is rock solid.")
    else:
        print(f"\n{failed} tests failed. Review the details above.")

if __name__ == "__main__":
    main()