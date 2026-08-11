#!/usr/bin/env bash
# Runs inside the container. Mirrors the steps of .github/workflows/ci.yml in order.
set -euo pipefail

export DEBIAN_FRONTEND=noninteractive

step() { printf '\n\033[1;36m== %s\033[0m\n' "$*"; }

step "Toolchain"
apt-get update -qq
apt-get install -y -qq --no-install-recommends \
  ca-certificates curl cmake g++ make libsqlite3-dev >/dev/null

step "FoundationDB ${FDB_VERSION} (cached in ~/fdb-debs on the host)"
base="https://github.com/apple/foundationdb/releases/download/${FDB_VERSION}"
[ -f ~/fdb-debs/clients.deb ] || curl -fsSL -o ~/fdb-debs/clients.deb \
  "${base}/foundationdb-clients_${FDB_VERSION}-1_amd64.deb"
[ -f ~/fdb-debs/server.deb ] || curl -fsSL -o ~/fdb-debs/server.deb \
  "${base}/foundationdb-server_${FDB_VERSION}-1_amd64.deb"
# The server postinst wants to hand the process to systemd, which a container has not got.
# Failing there is expected; the files are unpacked either way and we start it ourselves.
dpkg -i ~/fdb-debs/clients.deb >/dev/null 2>&1 || true
dpkg -i ~/fdb-debs/server.deb  >/dev/null 2>&1 || true

step "Start a single-node cluster"
mkdir -p /var/lib/foundationdb/data /var/log/foundationdb /etc/foundationdb
echo 'docker:docker@127.0.0.1:4500' > /etc/foundationdb/fdb.cluster
fdbserver -C /etc/foundationdb/fdb.cluster -p 127.0.0.1:4500 \
  --datadir /var/lib/foundationdb/data --logdir /var/log/foundationdb \
  --class unset >/var/log/foundationdb/fdbserver.stdout 2>&1 &
sleep 2
fdbcli --exec 'configure new single memory' >/dev/null 2>&1 || true
for _ in $(seq 1 30); do
  fdbcli --exec 'status minimal' 2>/dev/null | grep -q 'The database is available' && break
  sleep 2
done
fdbcli --exec 'status minimal'

step "Build"
# Out of the mounted tree: the host is Windows and its build/ is not this one's.
cmake -S /repo -B /build -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build /build -j"$(nproc)"

if [ "${CI_MODE:-play}" = shell ]; then
  step "Shell — the cluster is up, ./build is /build"
  exec bash
fi

step "Play a ward, and hold it to its arithmetic"
/build/queen check 200 20260811 8

step "Play a larger ward on another seed"
/build/queen check 120 990099 16

step "The ward must have been built"
out=$(/build/queen play 150 4242 8)
echo "$out"
echo "$out" | grep -q "venues: .*Tavern" || {
  echo "the Queen never commissioned anything in 150 cycles"; exit 1; }

# And that she decided rather than sorted. `venues:` is in the order she paid, and the Rails
# cost 260 against the Tavern's 120 — so Rails-before-Tavern is an order no sort by price can
# produce. It is the one assertion that fails the moment the planner stops deciding, and it
# caught exactly that: the first domain fell through to the cheap venue whenever the Rails were
# unaffordable this cycle, and reproduced price order while looking like a plan.
step "The Queen decided, rather than sorted"
echo "$out" | grep -q "venues:.*Transit Rails.*Cycle's End Tavern" || {
  echo "the Tavern came before the Rails, which is price order: she is sorting, not planning"
  exit 1; }

# Word gets out, and Sparks arrive. The Broadcast Row had no effect at all until now, which
# made it the one venue a Queen who reasons about value would never buy.
step "Word got out, and the ward grew"
echo "$out" | grep -q "arrived" || {
  echo "the Broadcast Row brought nobody, so it is still five hundred scrip for nothing"
  exit 1; }

# A game too large for one subscriber's slice. 60 Sparks is two wards, and the run moves one
# Spark between them halfway through. The sum has to hold across both wards, which is the
# part a migration is able to break.
step "A game of two wards, and a Spark that moves between them"
set +e
out=$(/build/queen shard 40 31337 60 2>&1); rc=$?
set -e
echo "$out"
[ "$rc" -eq 0 ] || { echo "the shard run exited $rc"; exit 1; }
echo "$out" | grep -q "moved to ward 1" || {
  echo "no Spark migrated, so the shard run proved nothing"; exit 1; }
echo "$out" | grep -q "every ward is honest" || {
  echo "the wards did not add up"; exit 1; }
echo "$out" | grep -q "no two share a name" || {
  echo "two wards gave out the same wire id, or nobody checked"; exit 1; }

# One ward may not exceed the slice budget by itself. The refusal is the feature.
step "One ward refuses to exceed a slice"
if /build/queen play 5 1 60 2>/dev/null; then
  echo "a 60-Spark ward was allowed, and its venues would fall out of every slice"; exit 1
fi
echo "  refused, as it should"

printf '\n\033[1;32m== ci passed\033[0m\n'
