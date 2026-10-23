# Stellar Frontier — Remake

A ground-up, modern remake of Stellar Frontier (Stardock, 1997). This branch
starts clean: none of the original code or art is carried over. The only
assets brought across are the new music made for the Enhanced Edition.

The faithful port lives on other branches of this repository:

- `master` (tag `original-1.0`): the original edition, an exact copy of the
  1997 game running on Godot 4.
- `improved`: the Enhanced Edition (launcher, any resolution, Ogg music
  modes, LAN multiplayer in progress).

## Music carried over

`assets/music/remix/`: genre remixes of the eight original songs.

| File | Song | Style |
|---|---|---|
| remix1-space-ambient.ogg | 1 | Space ambient |
| remix2-synthwave.ogg | 2 | Synthwave |
| remix3-liquid-dnb.ogg | 3 | Liquid drum & bass |
| remix4-chiptune.ogg | 4 | Chiptune |
| remix5-edm-trance.ogg | 5 | EDM / trance |
| remix6-symphonic-metal.ogg | 6 | Symphonic metal |
| remix7-epic-orchestral.ogg | 7 | Epic orchestral |
| remix8-dubstep.ogg | 8 | Dubstep |
| remix8-darkwave.ogg | 8 | Darkwave (alternative) |

`assets/music/new/`: ten new tracks built from unused fragments of the
original songs.

| File | Style |
|---|---|
| new01-graveyard-orbit.ogg | Trip-hop |
| new02-across-the-heliopause.ogg | Progressive house |
| new03-lagrange-drift.ogg | Atmospheric jungle |
| new04-tritone-run.ogg | Big beat |
| new05-nebula-caravan.ogg | Goa trance |
| new06-black-shipyards.ogg | Industrial |
| new07-rimward-patrol.ogg | Cinematic electronica |
| new08-long-haul-to-kepler-gate.ogg | Detroit techno |
| new09-far-sun-meridian.ogg | Jarre-style space synth |
| new10-lantern-orbit.ogg | Ambient techno |

All tracks are Ogg Vorbis at about -16 LUFS and end on a strong final chord
so they loop cleanly. The scripts that generated them stay on the `improved`
branch under `tools/music/edm/`.

Open music notes: new06's shipyard hammer is to be subdued and locked to the
groove; song 8 still needs a pick between dubstep and darkwave.

## License

The music is derived from the original Stellar Frontier songs, so it falls
under the Stardock Shared Source Stellar Frontier License
(`stellar_frontier_shared_source_eula.txt`): non-commercial use only.
