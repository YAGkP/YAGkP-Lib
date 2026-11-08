#!/usr/bin/env bash
# =============================================================================
# Graph sets from https://github.com/YAGkP/YAGkP-Graphs. Help: --help
# =============================================================================
set -euo pipefail

DATA_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="YAGkP/YAGkP-Graphs"
API_URL="https://api.github.com/repos/$REPO/releases?per_page=100"

usage() {
	cat <<EOF
Usage: bash data/graphs_manager.sh <command>

Graph sets for YAGkP are kept in https://github.com/$REPO:
every set is the archive <set>.tar.gz of the release <set>, one Matrix Market
(.mtx) file per graph. An installed set is the directory data/<set>/.

Commands:
  --list            the sets available for download; installed ones are marked
  --install <set>   download the set into data/<set>/ (replaces an old copy)
  --remove <set>    delete data/<set>/
  --help            show this help
EOF
}

die() {
	echo "Error: $*" >&2
	exit 1
}

# fetch <url>: print the contents of the url to stdout
fetch() {
	if command -v curl >/dev/null 2>&1; then
		curl -fsSL "$1"
	elif command -v wget >/dev/null 2>&1; then
		wget -qO- "$1"
	else
		die "curl or wget is required"
	fi
}

# installed_sets: the names of the sets in data/, one per line
installed_sets() {
	local dir
	for dir in "$DATA_DIR"/*/; do
		[ -d "$dir" ] || continue
		ls "$dir"*.mtx >/dev/null 2>&1 && basename "$dir"
	done
}

list_sets() {
	local releases
	if ! releases="$(fetch "$API_URL")"; then
		echo "Could not reach $API_URL" >&2
		echo "Installed sets: $(installed_sets | tr '\n' ' ')" >&2
		exit 1
	fi

	# The JSON is read without a parser: every "key": goes on its own line, and
	# the fields are taken line by line: the release tag, the size of its
	# archive <tag>.tar.gz and the first line of the release description,
	# without markdown
	echo "$releases" | sed 's/\([{,][[:space:]]*\)"\([a-z_]*\)":/\1\n"\2":/g' |
		awk -v installed=" $(installed_sets | tr '\n' ' ')" '
		function value(line) {
			sub(/^[^:]*: *"?/, "", line)
			sub(/"?[]}{,[:space:]]*$/, "", line)
			return line
		}
		/"tag_name":/ { tag = value($0); archive = 0; size = "" }
		/"name":/ { archive = (value($0) == tag ".tar.gz") }
		/"size":/ { if (archive) { size = value($0); archive = 0 } }
		/"body":/ {
			text = value($0)
			sub(/\\[rn].*/, "", text)
			gsub(/\*\*/, "", text)
			gsub(/\]\([^)]*\)/, "", text)
			gsub(/\[/, "", text)
			gsub(/\\"/, "\"", text)
			if (length(text) > 64) text = substr(text, 1, 61) "..."
			mark = index(installed, " " tag " ") ? "  [installed]" : ""
			mb = (size == "") ? "?" : sprintf("%.1f MB", size / 1e6)
			printf "%-10s %9s  %s%s\n", tag, mb, text, mark
		}'
}

# check_name <set>: a set name is a release tag: letters, digits, _ and -
check_name() {
	[[ "$1" =~ ^[A-Za-z0-9_-]+$ ]] || die "'$1' is not a set name"
}

install_set() {
	local set="$1"
	check_name "$set"
	local url="https://github.com/$REPO/releases/download/$set/$set.tar.gz"
	local set_dir="$DATA_DIR/$set"
	TMP_DIR="$set_dir.download" # the old copy is replaced only after a successful download
	trap 'rm -rf "$TMP_DIR"' EXIT

	echo "Downloading $url"
	rm -rf "$TMP_DIR"
	mkdir -p "$TMP_DIR"
	if ! fetch "$url" 2>/dev/null | tar -xz -C "$TMP_DIR" 2>/dev/null; then
		if ! fetch "https://api.github.com/repos/$REPO/releases/tags/$set" >/dev/null 2>&1; then
			die "there is no set '$set'. Available sets: bash data/graphs_manager.sh --list"
		fi
		die "could not download $url"
	fi
	chmod 644 "$TMP_DIR"/*.mtx
	rm -rf "$set_dir"
	mv "$TMP_DIR" "$set_dir"
	local graphs=("$set_dir"/*.mtx)
	echo "Done: ${#graphs[@]} graphs in data/$set/"
}

remove_set() {
	local set="$1"
	check_name "$set"
	[ -d "$DATA_DIR/$set" ] ||
		die "set '$set' is not installed. Installed sets: $(installed_sets | tr '\n' ' ')"
	rm -rf "${DATA_DIR:?}/$set"
	echo "Removed data/$set/"
}

case "${1:-}" in
	--list)
		[ $# -eq 1 ] || die "--list takes no arguments"
		list_sets
		;;
	--install)
		[ $# -eq 2 ] || die "usage: bash data/graphs_manager.sh --install <set>"
		install_set "$2"
		;;
	--remove)
		[ $# -eq 2 ] || die "usage: bash data/graphs_manager.sh --remove <set>"
		remove_set "$2"
		;;
	--help | -h)
		usage
		;;
	*)
		usage >&2
		exit 1
		;;
esac
