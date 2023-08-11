#!/bin/bash
SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"
python3 "$SCRIPT_DIR/client.py" 500
cat "$SCRIPT_DIR/echo_test.txt" | python3 "$SCRIPT_DIR/verify_echo.py"
