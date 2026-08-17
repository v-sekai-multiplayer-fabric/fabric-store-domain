# datasource-queen

This repository holds two processes. `queen` is a settlement game. The data source is SQLite on a
VFS that keeps its pages in FoundationDB. The two link into one process.

`README.md` gives the design. The comments in `src/queen.c` give the reasons. Record decisions
in the `multiplayer-fabric-manuals` repository.

## Build

The build needs a FoundationDB client, SQLite headers, CMake, and a C/C++ toolchain.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Set `WEFT_FDB_CLUSTER_FILE` to the cluster file before you run `queen`. A run without a live
cluster fails at start.

## Run

```sh
./build/queen play  200 20260811 8     # play one ward
./build/queen check 200 20260811 8     # play one ward twice, then hold it to its arithmetic
./build/queen shard  40 31337   60     # play a game of more than one ward
```

The arguments are cycles, seed, and Sparks, in that order.

## Checks

CI plays the game. A run that holds its invariants is the test.

Run the full CI on this machine in a container:

```sh
docker compose run --rm ci        # the whole of .github/workflows/ci.yml
docker compose run --rm shell     # the same container, cluster up, at a prompt
```

The Compose plugin is absent on some machines. On a machine with rootless podman, use the
quadlet in `ci/fabric-store-ci.container` instead.

CI MUST use a real FoundationDB, a real transport, and real certificates. Do not add a
test-only path. A door that only CI opens proves nothing about production.

## Limits

These constants bound the design. Read the value from the source. Do not copy a value into new
code.

| Constant           | Value | Where                          | What it bounds                        |
| ------------------ | ----- | ------------------------------ | ------------------------------------- |
| `SLICE_ENTITIES`   | 64    | `src/queen.c`                  | Entities in one subscriber slice      |
| `WARD_ENTITIES`    | 1800  | `src/ward.h`                   | Entities in one zone, from `AbyssalSLA.lean` |
| `WARD_HEADROOM`    | 400   | `src/ward.h`                   | Of those, the ghosts a neighbour replicates in |
| `SPARKS_PER_WARD`  | 1384  | `src/queen.c`                  | Sparks in one ward                    |
| `BOARD_SIZE`       | 6     | `src/queen.c`                  | Contracts on the board, plus 3 with the Rails |
| `MAX_WARDS`        | 8     | `src/queen.c`                  | Wards in one `shard` run              |
| `TXN_MAX_PARTS`    | 16    | `thirdparty/store-plane/fdb_vfs.c` | Databases in one group commit     |
| `WARD_TICK_HZ`     | 20    | `src/queen.c`                  | Publish rate, in real time            |
| `PAGE`             | 4096  | `thirdparty/store-plane/fdb_vfs.c` | SQLite page size                  |

A ward MUST NOT hold more Sparks than `SPARKS_PER_WARD`. The code refuses a larger ward. The
refusal is a feature. Past that limit, add a second ward.

The board bounds the work. At most 9 Sparks act in one cycle. A larger ward does not raise this
number.

## Invariants

Every change MUST keep these three properties. CI asserts all three.

1. Scrip is conserved. `treasury + purses + paid to the clock + built with == issued`.
2. Salvage is unique. No item is in two Sparks' hands.
3. One seed makes one ward. `check` plays a seed twice and compares a fingerprint.

A cycle MUST NOT advance on wall-clock time. Wall-clock time breaks the replay check.

A change MUST NOT alter the RNG draw order. A new draw moves every later number and changes
every seed's ward.

## Storage

The store keeps no local file. SQLite runs with a VFS whose pages live in FoundationDB.

Set `PRAGMA journal_mode=MEMORY`. This stops SQLite from writing a journal behind the VFS.

Do not open a database file on a disk. A local file makes storage stateful. Stateful storage
does not migrate.

## Transactions

A commit is one FoundationDB transaction. A commit costs about one round trip.

Group the writes of one cycle. Autocommit makes each statement its own round trip.

A payment spans two databases. Use the parallel commit protocol. Both sides MUST land, or
neither.

One group holds at most `TXN_MAX_PARTS` databases. `weft_txn_join` answers `SQLITE_FULL` past
that limit. Handle the refusal. Do not assume the limit holds.

## Wire format

Use bitpacked bytes on the hot path. Use JSON-LD as CBOR everywhere else.

Do not send plain JSON text.

WebTransport needs no framing layer. A datagram is one message. A stream FIN is the boundary.

Do not propose `webtransportd`. The Queen terminates QUIC in her own process.

## Vendored code

`thirdparty/` holds vendored subtrees.

| Path                       | Source                                        |
| -------------------------- | --------------------------------------------- |
| `thirdparty/store-plane`   | `datasource-store`                          |
| `thirdparty/gateway-edge`  | `transport-gateway-c`                         |
| `thirdparty/taskweft`      | `nif`, the `standalone/` headers              |

Vendor only from a `v-sekai-multiplayer-fabric` repository. A vendored copy MUST be
byte-identical to its source. Send a fix upstream first. Then vendor the fix.

## Key files

| Path                                | Purpose                                       |
| ----------------------------------- | --------------------------------------------- |
| `src/queen.c`                       | The game, the ward, and the cycle             |
| `src/planner.cpp`                   | The Queen's HTN domain                        |
| `thirdparty/store-plane/fdb_vfs.c`  | The VFS, and the parallel commit protocol     |
| `thirdparty/store-plane/bench_vfs.c`| The VFS benchmark                             |
| `ci/inside.sh`                      | The CI steps, in order                        |
| `.github/workflows/ci.yml`          | The CI job                                    |
| `docker-compose.yml`                | CI in a container on a developer machine      |

## Conventions

- Do not use the word "mint" in this project. Check branch names also. A rename of a head
  branch closes its PR.
- Put a runbook in the unit file. Put a reason in a code comment. Put a decision in the
  manuals repository. `README.md` is not a manual.
- Print all of a list, or print a count and a note. Do not print part of a list.
- Do not hardcode an absolute filesystem path. Use an environment variable.
- Build out of the source tree. The host `build/` is not the container `build/`.
