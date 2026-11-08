#!/usr/bin/env bash
# =============================================================================
# Downloads a set of graphs from https://github.com/YAGkP/YAGkP-Graphs
#
#   bash data/download_graphs.sh [set]       # set: walshaw (default)
#
# The set is the archive <set>.tar.gz of the release <set>. It is unpacked into
# data/<set>/ (the old contents are removed), and data/<set>_list.txt lists
# the graphs of the set. Commenting a line out with # excludes the graph from
# the benchmark
# =============================================================================
set -euo pipefail

DATA_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SET="${1:-walshaw}"
URL="https://github.com/YAGkP/YAGkP-Graphs/releases/download/$SET/$SET.tar.gz"
SET_DIR="$DATA_DIR/$SET"
LIST_FILE="$DATA_DIR/${SET}_list.txt"
TMP_DIR="$SET_DIR.download" # the old set is replaced only after a successful download
trap 'rm -rf "$TMP_DIR"' EXIT

echo "Downloading $URL"
rm -rf "$TMP_DIR"
mkdir -p "$TMP_DIR"
if command -v curl >/dev/null 2>&1; then
	curl -fL --progress-bar "$URL" | tar -xz -C "$TMP_DIR"
else
	wget -q --show-progress -O - "$URL" | tar -xz -C "$TMP_DIR"
fi

chmod 644 "$TMP_DIR"/*.mtx # the archive may carry odd permissions from its sources
rm -rf "$SET_DIR"
mv "$TMP_DIR" "$SET_DIR"

(cd "$SET_DIR" && ls -1 ./*.mtx | sed 's|^\./||; s|\.mtx$||') >"$LIST_FILE"
echo "Done: $(wc -l <"$LIST_FILE") graphs in data/$SET/, list: data/${SET}_list.txt"