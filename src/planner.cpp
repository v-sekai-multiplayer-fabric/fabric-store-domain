// The planner plane, in process.
//
// `commission()` used to walk `VENUES[]` and build the first thing it could afford. That
// array is in price order, so the Queen's whole game was a sort by cost. This replaces it
// with an HTN domain over the ward: tasks that say what she is trying to do, and methods that
// say what that decomposes into, ordered by what a venue does rather than what it costs.
//
// It is a symbolic planner and that is why it can be in a game that must replay. `check` mode
// plays a ward twice and compares fingerprints, so the same seed and the same state have to
// produce the same plan. A planner that sampled would turn that oracle into an anecdote.
//
// It is not the Sparks' brain either. They read the board and choose for themselves, weighing
// payout against risk against their own wear, and they keep doing it. The Queen plans; the
// Sparks decide.
//
// SPDX-License-Identifier: Apache-2.0

#include "planner.h"

#include "tw_planner.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace {

// The venues, by what they are for rather than by what they cost. These are indices into
// `VENUES[]` in queen.c, and the order below is the order the methods argue about.
enum {
	TAVERN = 0, // rest, so a Spark will take a longer odds contract
	DEN = 1,    // frames repaired, so wear stops ending careers
	RAILS = 2,  // more contracts reach the board
	PLAZA = 3,  // salvage sells for what it is worth
	CHAPEL = 4, // a failure costs a cycle, not a Spark
	ROW = 5,    // word gets out, and Sparks arrive
};

// A Frame this worn is a Spark about to stop earning, and a ward that keeps building for
// income while its people fall apart earns nothing with the income.
constexpr int64_t WEAR_THAT_CANNOT_WAIT = 60;

const char *const VAR_BUILT = "built";
const char *const VAR_COST = "cost";
const char *const VAR_WARD = "ward";

TwValue key(int venue) { return TwValue(static_cast<int64_t>(venue)); }

int64_t ward_num(const std::shared_ptr<TwState> &s, const char *what) {
	return s->get_nested(VAR_WARD, TwValue(what)).as_int();
}

bool is_built(const std::shared_ptr<TwState> &s, int venue) {
	return s->get_nested(VAR_BUILT, key(venue)).as_int() != 0;
}

// ── The one action that costs anything ────────────────────────────────────────
//
// Its preconditions are the rules `commission()` used to carry inline. The reserve especially:
// a ward that cannot pay its Sparks loses them, and a ward with no Sparks earns nothing and
// owes the same. As a precondition it is something the planner can reason around instead of
// a `continue` in a loop.
std::shared_ptr<TwState> act_commission(std::shared_ptr<TwState> s, std::vector<TwValue> args) {
	if (args.size() != 1 || !args[0].is_int()) return nullptr;
	const int64_t v = args[0].as_int();
	if (v < 0 || v >= PLAN_NVENUES) return nullptr;
	if (is_built(s, static_cast<int>(v))) return nullptr;

	const int64_t cost = s->get_nested(VAR_COST, args[0]).as_int();
	const int64_t treasury = ward_num(s, "treasury");
	const int64_t reserve = ward_num(s, "reserve");
	if (treasury < cost + reserve) return nullptr;

	auto next = s->copy();
	next->set_nested(VAR_BUILT, args[0], TwValue(ward_num(s, "cycle")));
	next->set_nested(VAR_WARD, TwValue("treasury"), TwValue(treasury - cost));
	return next;
}

// She waits. This is a real act and not the absence of one, which is why it is an action: a
// plan that ends in `hold` is a Queen who decided to hold, and it is distinguishable from a
// planner that could not answer.
std::shared_ptr<TwState> act_hold(std::shared_ptr<TwState> s, std::vector<TwValue>) {
	return s->copy();
}

// ── Methods ───────────────────────────────────────────────────────────────────

using Tasks = std::optional<std::vector<TwTask>>;

TwTask call(const char *name) { return TwCall{name, {}}; }
TwTask commission(int venue) { return TwCall{"commission", {key(venue)}}; }

// "Buy this, then carry on with the same task." The recursion is what lets one plan hold
// several commissions while each is still checked against the treasury the previous one left
// behind, so a plan never spends scrip the ward will not have.
Tasks next_of(const std::shared_ptr<TwState> &s, const char *task, int venue) {
	if (is_built(s, venue)) return std::nullopt;
	return std::vector<TwTask>{commission(venue), call(task)};
}

// Hold the scrip. This is the task #3 names and the one that makes the rest mean anything.
//
// Without it the planner reproduced the sort it replaced. When the Rails were unbuilt but
// unaffordable this cycle, the earning task simply failed, the next task was tried, and she
// bought the Tavern because the Tavern was cheap — which is a sort by price wearing a plan's
// clothes. Saving has to be an act she can choose, and it has to stop the cheaper thing from
// happening instead, so each line of business either commissions, or holds, and does not fall
// through to the next while it still wants something.

// Make the ward earn. The Rails put more contracts on the board and the Plaza pays what
// salvage is actually worth, so these two are the ward's income and nothing else is. In price
// order they are third and fourth; here they are first, and she waits for them.
Tasks m_rails(std::shared_ptr<TwState> s, std::vector<TwValue>) {
	return next_of(s, "make_the_ward_earn", RAILS);
}
Tasks m_plaza(std::shared_ptr<TwState> s, std::vector<TwValue>) {
	return next_of(s, "make_the_ward_earn", PLAZA);
}
Tasks m_save_to_earn(std::shared_ptr<TwState> s, std::vector<TwValue>) {
	if (!is_built(s, RAILS) || !is_built(s, PLAZA)) return std::vector<TwTask>{call("hold")};
	return std::nullopt;
}
// The ward earns as well as it can. Now the people who do the earning.
Tasks m_then_the_sparks(std::shared_ptr<TwState>, std::vector<TwValue>) {
	return std::vector<TwTask>{call("keep_the_sparks_working")};
}

// Keep the Sparks working. The Den repairs the Frame, the Tavern rests them so they will take
// longer odds, and the Chapel makes a failure survivable — the Den first and before the
// Chapel, because wear is the thing that ends a career and a bad cycle only costs one.
Tasks m_den(std::shared_ptr<TwState> s, std::vector<TwValue>) {
	return next_of(s, "keep_the_sparks_working", DEN);
}
Tasks m_tavern(std::shared_ptr<TwState> s, std::vector<TwValue>) {
	return next_of(s, "keep_the_sparks_working", TAVERN);
}
Tasks m_chapel(std::shared_ptr<TwState> s, std::vector<TwValue>) {
	return next_of(s, "keep_the_sparks_working", CHAPEL);
}
Tasks m_save_for_sparks(std::shared_ptr<TwState> s, std::vector<TwValue>) {
	if (!is_built(s, TAVERN) || !is_built(s, DEN) || !is_built(s, CHAPEL))
		return std::vector<TwTask>{call("hold")};
	return std::nullopt;
}
Tasks m_then_the_word(std::shared_ptr<TwState>, std::vector<TwValue>) {
	return std::vector<TwTask>{call("let_word_get_out")};
}

// Let word get out. Only worth anything while the slice can still carry another Spark: a ward
// at its budget that built the Row would have paid five hundred scrip for an announcement
// nobody can answer.
Tasks m_row(std::shared_ptr<TwState> s, std::vector<TwValue>) {
	if (is_built(s, ROW) || ward_num(s, "room_to_grow") <= 0) return std::nullopt;
	return std::vector<TwTask>{commission(ROW)};
}
Tasks m_wait(std::shared_ptr<TwState>, std::vector<TwValue>) {
	return std::vector<TwTask>{call("hold")};
}

// ── What she is trying to do ──────────────────────────────────────────────────
//
// Two ways to run a ward, and the planner picks between them on the state. Ordinarily income
// comes first, because a ward that cannot earn cannot do anything else either. But once the
// worst Frame in the ward is far enough gone, repairing it comes before earning — the horizon
// entering as a guard, which is as far as this goes until the Debt Clock becomes a real
// deadline.
//
// It commissions the Den itself rather than handing on to `keep_the_sparks_working`. The Den
// is the venue that repairs a Frame; the Tavern only makes a Spark bolder, and a ward that
// answered worn-out Frames by buying the Tavern would have made them bolder and no less worn.
// If she cannot afford the Den yet she falls through to earning, which is how she affords it.
Tasks m_repair_first(std::shared_ptr<TwState> s, std::vector<TwValue>) {
	if (is_built(s, DEN)) return std::nullopt;
	if (ward_num(s, "worst_wear") < WEAR_THAT_CANNOT_WAIT) return std::nullopt;
	return std::vector<TwTask>{commission(DEN), call("keep_the_sparks_working")};
}

Tasks m_earn_first(std::shared_ptr<TwState>, std::vector<TwValue>) {
	return std::vector<TwTask>{call("make_the_ward_earn")};
}

const TwDomain &ward_domain() {
	// Built once. It holds no state — every fact the planner uses arrives in the TwState — so
	// one copy is right and rebuilding it each cycle would only cost.
	static const TwDomain domain = [] {
		TwDomain d;
		d.actions["commission"] = act_commission;
		d.actions["hold"] = act_hold;

		// Method order is decision order: the planner tries them in the order they are pushed
		// and backtracks, so each list below is the argument about what matters most. Each task
		// ends either in a commission, in holding for what it still wants, or in handing on to
		// the task that matters next — and never in falling through while it still wants
		// something, which is what kept turning the plan back into a sort by price.
		d.task_methods["run_the_ward"] = {m_repair_first, m_earn_first};
		d.task_methods["make_the_ward_earn"] = {m_rails, m_plaza, m_save_to_earn,
		                                        m_then_the_sparks};
		d.task_methods["keep_the_sparks_working"] = {m_den, m_tavern, m_chapel, m_save_for_sparks,
		                                             m_then_the_word};
		d.task_methods["let_word_get_out"] = {m_row, m_wait};
		return d;
	}();
	return domain;
}

std::shared_ptr<TwState> state_of(const ward_view_t &v) {
	auto s = std::make_shared<TwState>();

	TwValue::Dict built, cost;
	for (int i = 0; i < PLAN_NVENUES; i++) {
		built[key(i).to_string()] = TwValue(static_cast<int64_t>(v.built[i]));
		cost[key(i).to_string()] = TwValue(static_cast<int64_t>(v.cost[i]));
	}
	s->set_var(VAR_BUILT, TwValue(std::move(built)));
	s->set_var(VAR_COST, TwValue(std::move(cost)));

	TwValue::Dict ward;
	ward["cycle"] = TwValue(static_cast<int64_t>(v.cycle));
	ward["treasury"] = TwValue(static_cast<int64_t>(v.treasury));
	ward["debt"] = TwValue(static_cast<int64_t>(v.debt));
	ward["nsparks"] = TwValue(static_cast<int64_t>(v.nsparks));
	ward["reserve"] = TwValue(static_cast<int64_t>(v.reserve));
	ward["worst_wear"] = TwValue(static_cast<int64_t>(v.worst_wear));
	ward["room_to_grow"] = TwValue(static_cast<int64_t>(v.room_to_grow));
	s->set_var(VAR_WARD, TwValue(std::move(ward)));

	return s;
}

} // namespace

// A ward is six venues. The search is nothing, and the budget below exists so that it can
// never become something without saying so.
//
// `tw_plan()` would be the obvious call, but it bounds its search on a wall clock and returns
// "no plan" when the clock runs out — the same answer it gives when no plan exists. On a busy
// machine that would quietly play a different game, which is the one failure this whole
// repository is arranged to catch. So the budget is ours to inspect, and a budget that fired
// is a refusal.
int queen_plan(const ward_view_t *view, int *venue) {
	if (!view || !venue) return 1;
	*venue = -1;

	TwBudget budget = TwBudget::from_now(std::chrono::milliseconds(1000));
	TwFailCache fail_cache;
	TwSuccessCache success_cache;
	TwMethodStats method_stats;

	const std::optional<std::vector<TwCall>> plan =
	    tw_seek_plan(state_of(*view), {call("run_the_ward")}, ward_domain(), TW_MAX_DEPTH,
	                 nullptr, budget, &fail_cache, &success_cache, &method_stats);

	if (budget.fired) return 1;
	if (!plan) return 1;

	// She pays for one thing per cycle and plans again the next. The rest of the plan is her
	// intent rather than a commitment, which is what makes it a plan and not a rule: next
	// cycle the ward is different and she is entitled to change her mind.
	for (const TwCall &c : *plan) {
		if (c.name == "commission" && c.args.size() == 1 && c.args[0].is_int()) {
			*venue = static_cast<int>(c.args[0].as_int());
			return 0;
		}
	}
	return 0; // a plan of nothing but `hold`: she waits, and that is an answer
}
