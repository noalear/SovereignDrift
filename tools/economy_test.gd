# Economy test (the Resources update, phase 1): every body's richness
# matches the design table, colonies produce into capped stockpiles, Minor
# and Major colonies refine Alloys, the planet card shows it all, and the
# stockpiles survive the network packing.
#   godot --headless --path . -s tools/economy_test.gd
extends SceneTree

var main: Node
var failed := false

## The design doc's table: body -> [metals, ice, fuel, food, exotics].
const EXPECTED := {
	"Sol": [0, 0, 0, 0, 5], "Mercury": [5, 0, 0, 0, 2], "Venus": [3, 0, 0, 0, 3],
	"Earth": [3, 3, 0, 5, 0], "Luna": [3, 1, 0, 1, 0], "Mars": [3, 1, 0, 1, 0],
	"Phobos": [4, 0, 0, 0, 1], "Deimos": [4, 0, 0, 0, 1],
	"Ceres": [4, 1, 0, 0, 3], "Pallas": [4, 1, 0, 0, 3], "Vesta": [4, 1, 0, 0, 3], "Juno": [4, 1, 0, 0, 1],
	"Jupiter": [0, 0, 5, 0, 0], "Io": [2, 0, 0, 0, 4], "Europa": [1, 5, 0, 2, 0],
	"Ganymede": [2, 3, 0, 2, 0], "Callisto": [2, 3, 0, 2, 0], "Saturn": [0, 0, 5, 0, 0],
	"Titan": [2, 2, 3, 1, 0], "Rhea": [1, 4, 0, 2, 0], "Uranus": [0, 0, 4, 0, 0],
	"Neptune": [0, 0, 4, 0, 0], "Triton": [1, 4, 1, 2, 0], "Pluto": [1, 4, 1, 2, 0],
}


func _init() -> void:
	call_deferred("_start")


func _check(ok: bool, what: String) -> void:
	print(("PASS  " if ok else "FAIL  ") + what)
	if not ok:
		failed = true


func _start() -> void:
	main = load("res://src/Main.tscn").instantiate()
	root.add_child(main)
	await process_frame
	await process_frame
	if OS.get_environment("SF_TABLE") == "1":
		_print_table()
	main._start_game()
	await process_frame
	await process_frame
	var w: World = main.world
	_check(w.economy != null, "the world has an economy")
	# richness
	var bad := []
	for p in w.planets:
		if not EXPECTED.has(p.body_name):
			continue
		var want: Array = EXPECTED[p.body_name]
		var got := []
		for k in Economy.RAW:
			got.append(p.richness[k])
		if got != want:
			bad.append("%s %s (want %s)" % [p.body_name, got, want])
	_check(bad.is_empty(), "richness matches the design table" + ("" if bad.is_empty() else ": " + ", ".join(bad)))
	# seeding
	var earth: Planet = w.planet_by_name["Earth"]
	_check(earth.stock["food"] > 0.0 and earth.stock["metals"] > 0.0, "Earth starts with food and metals (%d, %d)" % [earth.stock["food"], earth.stock["metals"]])
	_check(w.planet_by_name["Jupiter"].stock["fuel"] == 0.0, "nobody holds Jupiter's fuel at the start")
	# production over two minutes
	var before: float = earth.stock["metals"]
	var rates := Economy.production(earth)
	Economy.step(earth, 120.0)
	# less what the refinery eats: two Metals an Alloy (Earth holds Fuel for its shield)
	_check(absf(earth.stock["metals"] - minf(before + (rates["metals"] - 2.0 * rates.get("alloys", 0.0)) * 2.0, Economy.cap(earth))) < 0.5, "Earth makes %.1f metals a minute" % rates["metals"])
	_check(rates["food"] > rates["metals"], "Earth is the Terran breadbasket (food %.1f a minute)" % rates["food"])
	# the cap
	Economy.step(earth, 60.0 * 600.0)
	_check(earth.stock["food"] <= Economy.cap(earth) + 0.01, "stock stops at the cap (%d)" % int(Economy.cap(earth)))
	# a silenced colony makes nothing
	var mars: Planet = w.planet_by_name["Mars"]
	mars.silenced = 20.0
	var m0: float = mars.stock["metals"]
	Economy.step(mars, 60.0)
	_check(mars.stock["metals"] == m0, "a knocked-out colony makes nothing")
	mars.silenced = 0.0
	# refining: Titan has fuel, a Major colony refines
	var titan: Planet = w.planet_by_name["Titan"]
	titan.stock["metals"] = 100.0
	titan.stock["fuel"] = 100.0
	titan.stock["alloys"] = 0.0
	Economy.step(titan, 60.0)
	_check(titan.stock["alloys"] > 7.0, "Titan refines alloys (%.1f in a minute)" % titan.stock["alloys"])
	# the card
	var lines: Array = earth.info_lines(1)
	var text := ""
	for l in lines:
		text += l[0] + "\n"
	_check("Yields" in text and "Stock" in text and "Making" in text, "Earth's card shows yields, stock and output")
	var enemy_text := ""
	for l in earth.info_lines(2):
		enemy_text += l[0] + "\n"
	_check("unknown" in enemy_text and not "Making" in enemy_text, "an enemy sees no numbers")
	var jtext := ""
	for l in w.planet_by_name["Jupiter"].info_lines(1):
		jtext += l[0] + "\n"
	_check("Fuel 5" in jtext and "skimmer" in jtext, "Jupiter's card points at a skimmer station")
	# the host's tick runs
	var e0: float = w.planet_by_name["Ganymede"].stock["ice"]
	w.planet_by_name["Ganymede"].stock["ice"] = 0.0
	for i in 130:
		await physics_frame
	_check(w.planet_by_name["Ganymede"].stock["ice"] > 0.0, "the world's tick produces (Ganymede ice %.2f)" % w.planet_by_name["Ganymede"].stock["ice"])
	# network packing
	var packed: Array = w.economy.pack()
	var copy := {}
	for p in w.planets:
		copy[p] = p.stock.duplicate()
		p.stock = Economy.empty_stock()
	w.economy.unpack(packed)
	var same := true
	for p in w.planets:
		for k in Economy.RESOURCES:
			if absf(p.stock[k] - copy[p][k]) > 0.01:
				same = false
	_check(same, "stockpiles survive packing for the network")
	# empire totals
	var t: Dictionary = w.economy.totals(1)
	_check(t["stock"]["food"] > 0.0 and w.economy.holdings(1).size() >= 2, "the Terran ledger lists %d worlds" % w.economy.holdings(1).size())
	print("economy test %s" % ("FAILED" if failed else "passed"))
	quit(1 if failed else 0)


func _print_table() -> void:
	for row in root.get_node("GameData").SOL:
		var r := Economy.richness(row[5], row[0], row[6], row[7], row[4], float(row[8]))
		print("%-10s %s" % [row[1], [r["metals"], r["ice"], r["fuel"], r["food"], r["exotics"]]])
