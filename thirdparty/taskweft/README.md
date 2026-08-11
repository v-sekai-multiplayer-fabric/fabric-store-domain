# taskweft, vendored

The `standalone/` directory of
[`v-sekai-multiplayer-fabric/nif`](https://github.com/v-sekai-multiplayer-fabric/nif), at
commit `758dc2af51` — the HTN planner with no Elixir and no NIF in it. Header-only C++20, MIT,
and the `LICENSE` beside this file is upstream's.

That repository is the one this directory tracks, and it is inside the organisation on
purpose: a build here does not rest on a repository nobody in the fabric can restore.

It may or may not have an upstream of its own — today it does — and nothing here should be
written as though the answer were fixed. Ask it, and sync it if it has one, before moving the
pin:

    gh repo view v-sekai-multiplayer-fabric/nif --json parent
    gh repo sync v-sekai-multiplayer-fabric/nif

Vendored rather than fetched, for the reason `store-plane` is: a repository that cannot build
without cloning a second one is a note, not a repository. `git subtree` cannot take a
subdirectory of an upstream, and the rest of that repository is a mix project, so this is a
copy of `standalone/` with the commit written down instead of a squashed subtree commit. The
guarantee is the same one; only the mechanism differs.

`src/planner.cpp` is the only thing here that includes any of it, and it reaches
`tw_planner.hpp` and what that pulls in — `tw_domain`, `tw_soltree`, `tw_state`, `tw_rebac`,
`tw_json`, `tw_value`, and `thirdparty/tsl_ordered_map.h`. The rest of the tree is upstream's
and is carried unedited so the next update is a copy rather than a merge:

- `tw_temporal.hpp` and `thirdparty/date/` are the Debt Clock's horizon, which has not landed
- `fine.hpp`, `tw_bridge.hpp`, `tw_loader.hpp`, `tw_mc_executor.hpp`, `tw_retriever.hpp` and
  `tw_hrr.hpp` are the NIF's own boundary and its JSON domain loading, which a domain written
  in C++ does not need

To update: copy `standalone/` over this directory again and change the commit above. Nothing
here is patched, so there is nothing to reapply.

**Nothing here is patched, and nothing here should be.** A fix to the planner is a pull
request to `v-sekai-multiplayer-fabric/nif`, and onward to that repository's own upstream if
it has one — which is a question to ask it rather than to answer here, and the reason this
paragraph names one repository instead of two. The planner is where every other tenant of it
would want the fix.

Editing a vendored copy makes the next sync a merge instead of a copy, and leaves the
improvement stranded in one game. If a change cannot wait for the planner, it belongs in
`src/planner.cpp` — the domain — rather than in these headers.
