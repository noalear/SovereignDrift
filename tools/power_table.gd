# Prints each empire's stock warships' reactor budget (made / drawn, and
# the shields' share of the draw), for tuning shield power.
#   godot --headless --path . -s tools/power_table.gd
extends SceneTree


func _init() -> void:
	call_deferred("_run")


func _run() -> void:
	var gd: Node = root.get_node("GameData")
	var races := ["Arcean", "Terran", "Drengin", "Pirate"]
	print("hull            race     shields                    made  drawn  shields  overload")
	for hull in ["Scout", "Destroyer", "Raider", "Cruiser", "BattleCruiser", "Battleship", "Dreadnought", "Freighter", "DropShip"]:
		for r in 4:
			if r == 3 and not gd.STOCK_LOADOUTS[3].has(hull):
				continue
			var d: Dictionary = gd.race_fit(hull, r)
			var st: Dictionary = gd.stats_for(d)
			var names := []
			for id in d["modules"]:
				if gd.MODULES[id]["func"] == "shield":
					names.append(gd.MODULES[id]["name"])
			print("%-15s %-8s %-26s %5d  %5d  %7d  %.2f" % [hull, races[r], ", ".join(names), st["power_gen"], st["power_draw"], st["shield_draw"], st["overload"]])
	quit(0)
