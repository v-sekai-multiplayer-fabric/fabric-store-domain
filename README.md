# datasource-queen

Durable state. A **data source** implements a repository, and this one is packed by
what a caller can reach and what it cannot.

| member | what it does | where it runs |
| --- | --- | --- |
| `queen` | the game, and the store's caller. A plane: a process with no networking. | here, in `src/` |
| `taskweft` | the planner plane. The Queen's one decision, as an HTN domain over the ward. | here, in `src/planner.cpp` |
| `fabric-store-plane` | SQLite over a VFS whose pages live in FoundationDB, on rivet's Depot layout | on its caller's machine |
| FoundationDB | the pages, and every durable transaction. Below the planes, not one of them. | anywhere |
| `versitygw` | the S3-compatible endpoint FoundationDB backs up to with `fdbbackup` | with FoundationDB |

## Run it

    ./build/queen play  200 20260811 8     play a ward and see what became of it
    ./build/queen check 200 20260811 8     play it twice and hold it to its arithmetic

    docker compose run --rm ci             the whole of .github/workflows/ci.yml
    docker compose run --rm shell          same container, cluster up, at a prompt

## State

**The game builds and plays.** CI plays two wards on two seeds against a live FoundationDB and
holds them to their arithmetic, so the domain has a workload that runs on every push.

**Not deployed.** There is no Fly machine definition yet for FoundationDB and versitygw, and
that is the only part of this with a machine of its own, since the plane ships with whatever
calls it. `fabric-store-plane#17` tracks it.

## The design

`docs/design.md` holds the rest, unchanged: what needs a ring and what does not, why the store
holds no local file, what this is not, the `queen` tenant and why a game is the right one, and
how to run CI here. `rfd/0085` is the setting and `rfd/0112` the planner decision.

This README and `service-physics`'s carried the same 114 lines. That copy is gone from
`service-physics`, which is a clone that has not been written yet, and this is the one place
those sections live.
