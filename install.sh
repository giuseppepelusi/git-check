#!/usr/bin/env bash
set -e

# Configuration
REPO_URL="https://github.com/giuseppepelusi/git-check.git"
PROG_NAME="git-check"
MAN_PAGE="$PROG_NAME.1"

# Check for required commands
for cmd in git gcc; do
    if ! command -v $cmd >/dev/null 2>&1; then
        echo "Error: $cmd is required but not installed."
        exit 1
    fi
done

# Create temporary folder
TMP_DIR=$(mktemp -d)
git clone --quiet "$REPO_URL" "$TMP_DIR"

# Compile program
cd "$TMP_DIR"
gcc -o "$PROG_NAME" "src/$PROG_NAME.c"

# Install binary
sudo cp "$PROG_NAME" /usr/local/bin/
sudo chmod +x /usr/local/bin/"$PROG_NAME"

# Install man page
MAN_DIR="/usr/local/man/man1"
sudo mkdir -p "$MAN_DIR"
sudo cp "man/$MAN_PAGE" "$MAN_DIR/"

# Cleanup
cd - >/dev/null
rm -rf "$TMP_DIR"
echo "$PROG_NAME installed successfully"
