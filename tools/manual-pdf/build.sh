#!/usr/bin/env bash
# Builds docs/deSIDuous-Manual.pdf from docs/MANUAL.md.
# Needs: the GitHub CLI (renders the Markdown exactly as GitHub does) and Microsoft Edge.
# Run from Git Bash on Windows:  tools/manual-pdf/build.sh
set -euo pipefail
cd "$(dirname "$0")/../.."
OUT="${TMPDIR:-/tmp}/desiduous-manual"
mkdir -p "$OUT"
GH="${GH:-/c/Program Files/GitHub CLI/gh.exe}"
EDGE="${EDGE:-/c/Program Files (x86)/Microsoft/Edge/Application/msedge.exe}"

# The cover page replaces the manual's logo, title and version lines, and the
# overview screenshot that the cover already shows.
sed -e '1,6d' -e '/images\/ui-overview.png/d' docs/MANUAL.md > "$OUT/body.md"
"$GH" api markdown -f mode=gfm -f context=squibsy/deSIDuous -F text=@"$OUT/body.md" > "$OUT/body.html"

# Point relative image paths at this checkout's docs folder
DOCS="file:///$(cygpath -m "$PWD/docs")/"
sed "s|<base href=\"[^\"]*\">|<base href=\"$DOCS\">|" tools/manual-pdf/head.html > "$OUT/manual.html"
cat "$OUT/body.html" tools/manual-pdf/tail.html >> "$OUT/manual.html"

"$EDGE" --headless=new --disable-gpu --allow-file-access-from-files --no-pdf-header-footer \
  --virtual-time-budget=8000 --print-to-pdf="$(cygpath -w "$PWD/docs/deSIDuous-Manual.pdf")" \
  "file:///$(cygpath -m "$OUT/manual.html")"
echo "wrote docs/deSIDuous-Manual.pdf"
