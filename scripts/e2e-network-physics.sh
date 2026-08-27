#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${BUILD_DIR:-"$root_dir/build"}"
map_path="$root_dir/apps/battlegrid/maps/all-abilities.map"
port="${1:-47991}"
runtime_seconds="${E2E_RUNTIME_SECONDS:-5}"
log_dir="$(mktemp -d)"
server_pid=""
compute_pid=""

cleanup() {
    local status=$?
    for pid in "$compute_pid" "$server_pid"; do
        if [[ -n "$pid" ]] && kill -0 "$pid" 2>/dev/null; then
            kill -INT "$pid" 2>/dev/null || true
        fi
    done
    for pid in "$compute_pid" "$server_pid"; do
        if [[ -n "$pid" ]]; then
            wait "$pid" 2>/dev/null || true
        fi
    done
    if [[ "$status" -ne 0 ]]; then
        printf 'E2E test failed; logs retained in %s\n' "$log_dir" >&2
    else
        rm -rf "$log_dir"
    fi
    exit "$status"
}
trap cleanup EXIT

wait_for_log() {
    local pattern="$1"
    local file="$2"
    for _ in $(seq 1 100); do
        if grep -q -- "$pattern" "$file"; then
            return 0
        fi
        sleep 0.1
    done
    printf 'Timed out waiting for "%s" in %s\n' "$pattern" "$file" >&2
    return 1
}

cmake -S "$root_dir" -B "$build_dir" -DBUILD_DOC=OFF -DBUILD_TESTING=ON
cmake --build "$build_dir" --target battlegrid test_physics test_battlegrid net_tests --parallel 2
"$build_dir/bin/test_physics"
"$build_dir/bin/test_battlegrid"
"$build_dir/libs/libnet/tests/net_tests"

stdbuf -oL "$build_dir/bin/battlegrid" \
    --headless "$port" --map "$map_path" >"$log_dir/server.log" 2>&1 &
server_pid=$!
sleep 1
if ! kill -0 "$server_pid" 2>/dev/null; then
    cat "$log_dir/server.log" >&2
    exit 1
fi

stdbuf -oL "$build_dir/bin/battlegrid" \
    --compute "127.0.0.1:$port" >"$log_dir/compute.log" 2>&1 &
compute_pid=$!

wait_for_log 'Compute node connected, delegated' "$log_dir/server.log"
wait_for_log '\[Compute\] Received terrain from server\.' "$log_dir/compute.log"
wait_for_log '\[Compute\] Received [0-9][0-9]* agent assignments\.' "$log_dir/compute.log"

sleep "$runtime_seconds"
kill -INT "$compute_pid"
wait "$compute_pid"
compute_pid=""

wait_for_log 'Compute node disconnected, reclaimed' "$log_dir/server.log"
kill -INT "$server_pid"
wait "$server_pid"
server_pid=""

printf 'Networking and Box3D compute-node E2E test passed.\n'
