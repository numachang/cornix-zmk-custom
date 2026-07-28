#!/usr/bin/env bash
#
# Build Cornix ZMK firmware locally, using the exact container image that CI
# uses (zmkfirmware/zmk-build-arm:stable, as referenced by ZMK's shared
# .github/workflows/build-user-config.yml).
#
# Usage:
#   ./docker-build.sh init          one-time: fetch zmk + zephyr + modules (~3 GB)
#   ./docker-build.sh list          show build targets parsed from build.yaml
#   ./docker-build.sh build         build every active target
#   ./docker-build.sh build left    build only targets whose name matches "left"
#   ./docker-build.sh update        re-run west update (after config/west.yml churn)
#   ./docker-build.sh shell         interactive shell inside the workspace
#   ./docker-build.sh clean         drop build outputs, keep fetched sources
#
# Output .uf2 files land in ./firmware/, which .gitignore already excludes.
#
# Environment overrides: ZMK_DOCKER_IMAGE, ZMK_WORKSPACE
#
# Three repo-specific quirks are handled here; all three were hit for real
# while bringing this up:
#
#  1. The west workspace lives OUTSIDE this repo. This repo ships its own
#     zephyr/module.yml, so a workspace rooted here would collide with the
#     Zephyr tree that `west update` clones into <topdir>/zephyr. CI dodges
#     this the same way, by using $TMPDIR/zmk-config as the workspace root.
#  2. `west zephyr-export` runs before every build, in the same container.
#     It writes the CMake package registry to $HOME/.cmake, which is /root
#     here and does not survive a --rm container. Skip it and CMake fails
#     with "Could not find a package configuration file provided by Zephyr".
#  3. git is forced onto HTTP/1.1. The treeless clone CI uses
#     (--filter=tree:0) fetches trees on demand at checkout time, and that
#     reliably died against GitHub over HTTP/2 with "RPC failed; curl 92".

set -euo pipefail

IMAGE="${ZMK_DOCKER_IMAGE:-zmkfirmware/zmk-build-arm:stable}"
REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WS="${ZMK_WORKSPACE:-$HOME/zmk-workspace-cornix}"

# Shared git hardening + Zephyr CMake registry, needed in every container.
PRELUDE='
  git config --global --add safe.directory "*"
  git config --global http.version HTTP/1.1
  git config --global http.postBuffer 524288000
  git config --global fetch.parallel 1
'

dock() {
  local tty=()
  if [ -t 0 ] && [ -t 1 ]; then tty=(-it); fi
  # ${a[@]+"${a[@]}"} rather than "${a[@]}": macOS ships bash 3.2, where an
  # empty array expanded under `set -u` aborts with "unbound variable".
  docker run --rm ${tty[@]+"${tty[@]}"} \
    -v "$WS:/ws" -v "$REPO:/config-repo" -w /ws \
    "$@"
}

# One line per target: board US shield US snippet US artifact-name.
# The separator is ASCII US (0x1f), not tab: tab counts as IFS whitespace, so
# `read` would collapse the run of two tabs produced by an empty snippet field
# and silently shift every later field left.
SEP=$'\x1f'

read_targets() {
  docker run --rm -i -v "$REPO:/config-repo" "$IMAGE" python3 - <<'PY'
import yaml
m = yaml.safe_load(open('/config-repo/build.yaml')) or {}
for e in (m.get('include') or []):
    board = e.get('board', '')
    shield = e.get('shield') or ''
    snippet = e.get('snippet') or ''
    name = e.get('artifact-name') or (
        (shield.replace(' ', '+') + '-' if shield else '') + board.replace('/', '_'))
    print('\x1f'.join([board, shield, snippet, name]))
PY
}

build_one() {
  local board=$1 shield=$2 snippet=$3 art=$4
  echo "=== $art  (board=$board shield=${shield:--} snippet=${snippet:--}) ==="
  dock -e BOARD="$board" -e SHIELD="$shield" -e SNIPPET="$snippet" -e ART="$art" \
    "$IMAGE" bash -c "
      set -eu
      $PRELUDE
      west zephyr-export
      args=(-s zmk/app -d \"/ws/build/\$ART\" -b \"\$BOARD\")
      if [ -n \"\$SNIPPET\" ]; then args+=(-S \"\$SNIPPET\"); fi
      args+=(-- -DZMK_CONFIG=/config-repo/config -DZMK_EXTRA_MODULES=/config-repo)
      if [ -n \"\$SHIELD\" ]; then args+=(-DSHIELD=\"\$SHIELD\"); fi
      set -x
      west build \"\${args[@]}\"
      set +x
      mkdir -p /config-repo/firmware
      if [ -f \"/ws/build/\$ART/zephyr/zmk.uf2\" ]; then
        cp \"/ws/build/\$ART/zephyr/zmk.uf2\" \"/config-repo/firmware/\$ART.uf2\"
        echo \"-> firmware/\$ART.uf2\"
      else
        cp \"/ws/build/\$ART/zephyr/zmk.bin\" \"/config-repo/firmware/\$ART.bin\"
        echo \"-> firmware/\$ART.bin\"
      fi
    "
}

case "${1:-}" in
  init)
    mkdir -p "$WS"
    dock "$IMAGE" bash -c "
      set -eux
      $PRELUDE
      mkdir -p /ws/config
      cp -R /config-repo/config/* /ws/config/
      [ -d /ws/.west ] || west init -l /ws/config
      n=0
      until [ \$n -ge 3 ]; do
        west update --fetch-opt=--filter=tree:0 && break
        n=\$((n+1)); echo \"--- west update failed, retry \$n/3 ---\"
      done
      [ \$n -lt 3 ] || { echo 'west update failed after 3 retries' >&2; exit 1; }
      west zephyr-export
      west list
    "
    ;;

  update)
    dock "$IMAGE" bash -c "
      set -eux
      $PRELUDE
      cp -R /config-repo/config/* /ws/config/
      west update --fetch-opt=--filter=tree:0
      west zephyr-export
      west list
    "
    ;;

  list)
    while IFS="$SEP" read -r board shield snippet art; do
      printf '%s\n    board=%s  shield=%s  snippet=%s\n' \
        "$art" "$board" "${shield:--}" "${snippet:--}"
    done < <(read_targets)
    ;;

  build)
    filter="${2:-}"
    built=0
    while IFS="$SEP" read -r board shield snippet art; do
      case "$art" in *"$filter"*) ;; *) continue ;; esac
      build_one "$board" "$shield" "$snippet" "$art"
      built=$((built + 1))
    done < <(read_targets)
    if [ "$built" -eq 0 ]; then
      echo "no target matched '${filter}'; try: $0 list" >&2
      exit 1
    fi
    echo "built $built target(s)"
    ;;

  shell)
    dock "$IMAGE" bash
    ;;

  clean)
    rm -rf "${WS:?}/build"
    echo "removed $WS/build"
    ;;

  *)
    sed -n '3,20p' "${BASH_SOURCE[0]}" | sed 's/^#\{1,\} \{0,1\}//'
    exit 1
    ;;
esac
