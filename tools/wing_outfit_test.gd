# Outfitting wingmen: the shipyard's ship tabs open a wingman's ship, its
# purse pays first and the pilot's credits cover the rest, the pilot can
# hand it credits, a far-off wingman has no tab, prizes are the pilot's to
# pay for, wingmen earn bounties; the story's Lantern roster keeps a fit
# from one operation to the next and is outfitted between operations.
#   godot --headless --path . -s tools/wing_outfit_test.gd
extends SceneTree

var main: Node
var failed := false


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	root.add_child(main)
	await process_frame
	main._start_free_flight()
	await process_frame
	await process_frame
	var w = main.world
	var p = w.player
	var base = w.starbase_of(p.race_id)
	p.global_position = base.global_position + Vector2(base.size + 80, 0)
	p.velocity = base.velocity
	p.credits = 1000
	p.rank = 6
	# a wingman beside the yard, with a purse of its own
	var wm = w.spawn_ship("Lt. Test", p.race_id, "Scout", "Test Wing", base.global_position + Vector2(base.size + 300, 200), base.velocity, 5)
	wm.rank = 5
	wm.credits = 300
	wm.get_node("AI").join(p, true)
	var far = w.spawn_ship("Lt. Far", p.race_id, "Scout", "Far Wing", base.global_position + Vector2(base.size + 9000, 0), base.velocity, 5)
	far.get_node("AI").join(p, true)
	var Wing = load("res://src/world/Wing.gd")
	_check(w.wing.outfittable(p, base) == [wm], "only the wingman near the yard can be outfitted (%d)" % w.wing.outfittable(p, base).size())
	_check(w.wing.earns(wm) and Wing.has_purse(wm), "a called wingman has a purse and earns bounties")
	w.try_refit(p)
	await process_frame
	await process_frame
	var b = main.builder
	_check(b != null and b.ship == p and b.crew.size() == 2, "the shipyard opens on the pilot's ship with a tab for the wingman")
	if b == null:
		quit(1)
		return
	_check(b._tab_label(wm).begins_with("Lt. Test · Scout · 300 cr"), "the wingman's tab shows its ship and purse (%s)" % b._tab_label(wm))
	b._command("ship_next")
	await process_frame
	b = main.builder
	_check(b.ship == wm and b.pilot == p and b.credits == 1300, "C opens the wingman's ship: its purse and the pilot's credits make the budget (%d)" % b.credits)
	# a Destroyer for the wingman: 300 of its own, 100 of the pilot's
	b._switch_hull("Destroyer")
	_check(b.hull == "Destroyer" and b.credits == 900, "a hull goes on the wingman's order")
	b._accept()
	_check(not b.confirm.is_empty() and b.confirm["text"].begins_with("Lt. Test pays 300 cr from their purse and you cover 100 cr."), "the bill says who pays what (%s)" % b.confirm.get("text", ""))
	b._answer(0)
	await process_frame
	b = main.builder
	_check(wm.design_name == "Destroyer" and wm.credits == 0 and p.credits == 900, "accepted: the wingman flies a Destroyer, its purse paid first (%d, you %d)" % [wm.credits, p.credits])
	_check(b != null and b.ship == p and "Destroyer is fitted" in b.status, "the yard goes back to the pilot's own ship (%s)" % (b.status if b != null else "closed"))
	# a gift of the pilot's credits
	b._cycle_ship(1)
	await process_frame
	b = main.builder
	b._give(100)
	b._give(100)
	b._give(-100)
	_check(b.gift == 100, "Give / Take back move the gift in steps of 100")
	b._accept()
	b._answer(0)
	await process_frame
	b = main.builder
	_check(wm.credits == 100 and p.credits == 800, "the gift moves 100 cr into the wingman's purse (%d, you %d)" % [wm.credits, p.credits])
	# an unaccepted order asks before the tab changes
	b._cycle_ship(1)
	await process_frame
	b = main.builder
	b._switch_hull("Scout")
	b._cycle_ship(1)
	_check(not b.confirm.is_empty() and b.confirm["title"] == "Switch ships?", "switching with an order open asks first")
	b._answer(1)
	_check(main.builder == b and b.hull == "Scout", "Keep editing stays on the wingman's ship")
	b._cycle_ship(1)
	b._answer(0)
	await process_frame
	b = main.builder
	_check(b.ship == p and wm.design_name == "Destroyer", "Discard and switch drops the order")
	# a wingman shot down while its tab is open: back to the pilot's ship
	b._cycle_ship(1)
	await process_frame
	wm.dead = true
	await process_frame
	await process_frame
	b = main.builder
	_check(b != null and b.ship == p and b.crew.size() == 1 and "shot down" in b.status, "a wingman lost with its tab open sends the yard back to the pilot (%s)" % (b.status if b != null else "closed"))
	wm.dead = false
	b._leave()
	await process_frame
	w.try_refit(p)
	await process_frame
	await process_frame
	b = main.builder
	# a prize is the pilot's own ship: the pilot's credits and rank count
	wm.is_prize = true
	_check(not Wing.has_purse(wm) and not w.wing.earns(wm), "a prize has no purse and earns no bounty")
	b._cycle_ship(1)
	await process_frame
	b = main.builder
	_check(b.credits == 800 and b._rank() == p.rank, "a prize's ship is paid from the pilot's credits, hulls by the pilot's rank")
	wm.is_prize = false
	b._leave()
	await process_frame
	# a kill pays a wingman the bounty
	var foe = w.spawn_ship("Foe", 2, "Scout", "Target", base.global_position + Vector2(4000, 0), base.velocity, 3)
	var before: int = wm.credits
	foe.last_attacker = wm
	w._on_ship_died(foe, wm)
	_check(wm.credits > before, "a wingman's kill pays it the bounty (%d -> %d)" % [before, wm.credits])
	await _roster(w, p, base)
	print("wing outfit test: %s" % ("FAILED" if failed else "passed"))
	quit(1 if failed else 0)


## The story's Lantern pilots keep their ships between operations.
func _roster(w, p, base) -> void:
	var StoryScript = load("res://src/campaign/Story.gd")
	StoryScript.interludes = false
	var story = StoryScript.new(w, {})
	w.story = story
	var brandt = w.spawn_ship("Lt. Brandt", 1, "Scout", "Lantern Two", base.global_position + Vector2(900, 0), base.velocity, 6)
	story.dress(brandt, 6)
	_check(story.roster.has("Lt. Brandt") and brandt.rank == 6 and brandt.credits == 450, "a Lantern pilot joins the roster with a rank and a purse (%d cr)" % brandt.credits)
	w.remove_ship(brandt)
	p.credits = 1000
	w.try_refit(p)
	await process_frame
	await process_frame
	var b = main.builder
	_check(b != null and b.crew.has("Lt. Brandt"), "between operations the yard has a tab for Brandt")
	b._switch_to("Lt. Brandt")
	await process_frame
	b = main.builder
	_check(b.ship.has_meta("roster") and b.ship.pilot_name == "Lt. Brandt" and b.credits == 1450, "Brandt's tab outfits his ship from the roster (%d)" % b.credits)
	var ghost = b.ship
	b._switch_hull("Destroyer")
	b._accept()
	b._answer(0)
	await process_frame
	_check(story.roster["Lt. Brandt"]["design"]["name"] == "Destroyer" and int(story.roster["Lt. Brandt"]["credits"]) == 50 and p.credits == 1000,
		"the roster keeps Brandt's Destroyer and his purse paid for it (%d)" % int(story.roster["Lt. Brandt"]["credits"]))
	_check(not is_instance_valid(ghost), "the stand-in ship is gone with its tab")
	main.builder._leave()
	await process_frame
	var again = w.spawn_ship("Lt. Brandt", 1, "Scout", "Lantern Two", base.global_position + Vector2(900, 0), base.velocity, 6)
	story.dress(again, 6)
	_check(again.design_name == "Destroyer" and again.credits == 50, "the next operation's Brandt flies the Destroyer")
	again.credits = 120
	story.remember(again)
	var loaded = StoryScript.new(w, story.pack())
	_check(int(loaded.roster["Lt. Brandt"]["credits"]) == 120 and loaded.roster["Lt. Brandt"]["design"]["name"] == "Destroyer", "the roster goes into the story save and back")
	w.remove_ship(again)
	w.story = null


func _check(ok: bool, what: String) -> void:
	print(("ok   " if ok else "FAIL ") + what)
	if not ok:
		failed = true
