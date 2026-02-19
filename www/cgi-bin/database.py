#!/usr/bin/env python3

import os
import json
import sys
import urllib.parse

print("Content-Type: application/json")
print()

path = "tests/dataStore/users.json"

def read_body():
    length = int(os.environ.get("CONTENT_LENGTH", 0))
    if length <= 0:
        return ""
    return os.read(0, length).decode()

def load_users():
    if not os.path.exists(path):
        return []
    try:
        with open(path, "r") as f:
            return json.load(f)
    except Exception:
        return []

def save_users(users):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        json.dump(users, f)

method = os.environ.get("REQUEST_METHOD", "GET").upper()
content_type = os.environ.get("CONTENT_TYPE", "")

if method == "POST":
    body = read_body()
    data = {}
    # support application/json and form-urlencoded
    if content_type.startswith("application/json"):
        try:
            data = json.loads(body)
        except Exception:
            data = {}
    else:
        data = dict(urllib.parse.parse_qsl(body))

    username = data.get("username")
    password = data.get("password")

    if not username or not password:
        print(json.dumps({"status": "error", "message": "missing_fields"}))
        sys.exit(0)

    users = load_users()
    users.append({"username": username, "password": password})
    save_users(users)
    # Return all users (without passwords) after successful POST
    safe = [{"username": u.get("username", "")} for u in users]
    print(json.dumps({"status": "ok", "users": safe}))
    sys.exit(0)

if method == "GET":
    users = load_users()
    safe = [{"username": u.get("username", "")} for u in users]
    print(json.dumps({"status": "ok", "users": safe}))
    sys.exit(0)

print(json.dumps({"status": "error", "message": "unsupported_method"}))
