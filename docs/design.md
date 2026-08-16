# The store design

Moved out of `README.md`, which is capped at 40 lines. The text is unchanged, so it
still uses the vocabulary RFD 0111 retires — plane, edge plane, and domain. Converting
it is a separate pass, because a rename inside an argument can change what it argues.

## What needs a ring, and what does not

A caller reaches the store plane over iceoryx2, and **iceoryx2 is shared memory**. So the
plane sits on the machine its caller sits on, and it is not a thing to deploy on its own.
`fly/fly.toml` in `fabric-store-plane` says the same from the other side: a second Fly app
cannot be the other end of a ring.

**There is no ring here yet, and it would be wrong to imply one.** `queen` links the VFS
into its own process rather than speaking to a plane over iceoryx2, because the
thread-per-core loop that would answer is `fabric-store-plane#5` and is not running. So the
two are one process today. When that loop lands, the game reaches it over the ring instead
and nothing above changes — which is the point of the caller never opening a database.

What needs no ring is **FoundationDB**. A commit is one FoundationDB transaction, about 1 ms,
and everything reaching the store already pays that, so the pages may be their own machine,
their own region, or several. That is the part of this domain that deploys by itself, and
`fabric-zone-domain` does not care where it lands.

This file used to say the whole domain needed no ring, and so that the store plane could live
anywhere. That was true of the pages and false of the plane, and the two are not the same
thing. See `fabric-store-plane#15`.

## Why the store holds no local file

SQLite runs with a VFS whose pages are in FoundationDB, so there is no database file on any
disk, and an actor's database moves between machines with no copy and no restore.
`PRAGMA journal_mode=MEMORY` stops SQLite writing one behind the VFS. rivet lists the same rule
as binding, for the same reason: a local file makes storage stateful and not migratable.

## What it is not

It is not two domains. SQLite and FoundationDB are not separable here, because the VFS is the
bridge between them: splitting them would put a ring hop inside a page read, and a page read is
already one FoundationDB round trip. FoundationDB is a database the store plane is a client
of. It sits below the planes rather than among them, so it is not a member the way a plane is,
and calling it a service would put it in the same word as a plane and a domain.

## A Queen of the Gyre

The domain has a tenant now, and it is a game.

`queen` is a settlement game with no renderer, no client and no engine. The ward is a SQLite
database over the store plane's VFS, every Spark is a database of their own, and a cycle is
a transaction. What you can see of the game is what you can `SELECT`.

    ./build/queen play  200 20260811 8     play a ward and see what became of it
    ./build/queen check 200 20260811 8     play it twice and hold it to its arithmetic

It is shaped after the commission-and-wait games, where the monarch pays for buildings and
then waits: the Queen commissions venues in the Commons and the Under-Market, the Sparks
read the contract board and choose for themselves, and she never takes a contract. A loop
like that is a state machine over days with nothing in the critical path to draw, which is
the one game shape genuinely better as a database than as an engine.

The Queen plans rather than sorts. Her decision is an HTN domain over the ward, from
`v-sekai-multiplayer-fabric/nif`'s `standalone/` headers vendored into `thirdparty/taskweft`,
and the Sparks still choose for themselves. The domain and why it is symbolic are in
`src/planner.cpp`; `#3` is the issue and `rfd/0112` the decision.

The setting is `rfd/0085`, so none of it is borrowed. Sparks are digitised people in rented
Frames on a failing ring-station, and the antagonist is the Debt Clock — entropy and
compound interest rather than a dark lord. The tension is that every scrip paid to the
creditor is one that did not become a venue, and a venue is what makes the next cycle earn
more.

### Why a game is the right tenant

The store plane's claim was that commits scale with the number of actors committing at once,
and it had never run. A ward of sixteen Sparks is sixteen databases committing every cycle,
and **paying one of them is a transfer between two databases** — the parallel commit
protocol, reached by paying somebody rather than by a fixture built to reach it.

The invariants are the game's own arithmetic, which is what makes them worth checking:

- scrip is conserved: `treasury + purses + paid to the clock + built with == issued`
- salvage is unique, so no item is in two Sparks' hands
- one seed makes one ward, checked by playing it twice and comparing

The first of those caught a real bug the first time it ran: commissioning a venue moved
scrip out of the ward and nothing recorded where it went.

CI plays two wards and asserts all three, and then asserts something an invariant cannot —
that the Queen actually got to build something. A balance where she never can would pass
every check above while being no game at all.

### Running CI here

CI needs an Ubuntu toolchain and a live FoundationDB, which is not what a developer's machine
is — least of all a Windows one. So the machine lends a container the repository and nothing
else, and the container is the same `ubuntu-24.04` with the same FDB the workflow uses:

    docker compose run --rm ci        # the whole of .github/workflows/ci.yml
    docker compose run --rm shell     # same container, cluster up, at a prompt

The FoundationDB packages land in a named volume, so only the first run downloads them. The
build goes to `/build` inside the container rather than into the mounted tree, because the
host's `build/` is not this one's.

On a machine with rootless podman and no compose plugin, the same run is a quadlet:
`ci/fabric-store-ci.container` documents itself.

## State

**The game builds and plays.** CI plays two wards on two seeds against a live FoundationDB
and holds them to their arithmetic, so the domain has a workload that runs on every push.

**Not deployed.** There is no Fly machine definition yet for FoundationDB and versitygw, and
that is the only part of this with a machine of its own, since the plane ships with whatever
calls it. `fabric-store-plane#17` tracks it.

`fabric-store-plane#19` asked whether one plane on somebody else's machine is a thin thing to
call a domain. It is less thin than it was: there are two planes now, `queen` and the store,
and they are in one process rather than on a ring. Whether that counts is still the open
question, and having a real caller is what makes it worth answering rather than academic.
