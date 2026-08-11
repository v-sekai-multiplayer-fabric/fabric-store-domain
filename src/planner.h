// The Queen's one decision, asked of a planner rather than answered by a sort.
//
// This is the seam. The planner does not open a database and never will: `queen` reads the
// ward, hands over what it says, and does every write itself. That is what keeps the planner
// honest about reading state through an API, and it is the shape it already needs for the day
// it stops being a function call and becomes a ring hop.
//
// SPDX-License-Identifier: Apache-2.0
#ifndef QUEEN_PLANNER_H
#define QUEEN_PLANNER_H

#ifdef __cplusplus
extern "C" {
#endif

#define PLAN_NVENUES 6

// What the Queen knows when she decides. Everything here is a `SELECT` away, which is the
// point: she plans from what anyone could read, not from a private number in the process.
typedef struct {
	int cycle;
	int treasury;
	int debt;
	int nsparks;
	int reserve;                // what she must keep back to pay the ward for a cycle
	int built[PLAN_NVENUES];    // the cycle each venue was built, 0 if it was not
	int cost[PLAN_NVENUES];
	int worst_wear;             // the most worn Frame in the ward
	int room_to_grow;           // Sparks the slice could still carry
} ward_view_t;

// The venue to commission this cycle, or -1 to hold.
//
// Returns 0 when she decided, and non-zero when the planner could not answer at all. That is
// a refusal rather than a hold, and the caller must treat it as one: a Queen who cannot plan
// is not a Queen who chose to wait, and quietly turning the first into the second is how a
// game stops replaying and nobody notices.
int queen_plan(const ward_view_t *view, int *venue);

#ifdef __cplusplus
}
#endif

#endif
