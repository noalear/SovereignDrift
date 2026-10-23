## ShotOwner -- on a client, a stand-in for the shooter of a missile whose
## owner is not a ship we know (a planet's colony guns).  Missile and
## World only ask it for owner_race.
extends Node

var owner_race := -1
