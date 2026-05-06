#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
gh_bin="${GH_BIN:-$HOME/.local/bin/gh}"
ca_bundle="${GH_CA_BUNDLE:-$HOME/.local/gh-certs/ca-bundle.pem}"

if [[ ! -x "$gh_bin" ]]; then
	echo "gh not found at: $gh_bin" >&2
	exit 1
fi

if [[ -f "$ca_bundle" ]]; then
	export SSL_CERT_FILE="$ca_bundle"
fi

cd "$repo_root"

"$gh_bin" auth status >/dev/null 2>&1 || {
	echo "gh is not authenticated." >&2
	echo "Run: SSL_CERT_FILE=$ca_bundle $gh_bin auth login -h github.com -w --git-protocol https" >&2
	exit 1
}

echo "gh auth ok"
echo "Run tests before creating a PR, for example:"
echo "  ctest --output-on-failure"
echo "Then create a PR with:"
echo "  $gh_bin pr create --base main --head <branch> --title '<title>' --body '<body>'"
