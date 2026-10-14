# The game

UNSW Battlecode 2026 is a programming contest. Each team writes one program, and that program controls a team of
**dragons**: snakes on a grid at the bottom of the sea. Games run with no human input. This page is our own short
summary of the rules as they stood in October 2026. The official rules are at
[game.battlecode.au/docs](https://game.battlecode.au/docs/overview).

## The board

- A grid of tiles that **wraps around** at every edge (a torus). The maps we played ran from 32 x 16 up to 64 x 64.
- **Kelp** sits on the edges between tiles and blocks movement.
- **Portals** also sit on edges and come in pairs. A head that crosses one portal edge comes out at its partner,
  which can be anywhere on the map.
- **Pearls** appear on tiles on hidden timers. A dragon can see each tile's countdown while the tile is in view.

## The dragons

- **No shared memory.** Every dragon runs its own copy of the team's program in its own process, so each one has
  to build its own picture of the map.
- **Vision.** Each turn a dragon sees a **7 x 7 window** around its head: tiles, pearls, countdowns, dragon parts
  and edges. It cannot see through portals. It answers with one action.
- **Time.** A turn has roughly 25 ms. The judge meters it in CPU points, with a limit of 100M points a turn. The
  official timeouts page says what happens past the limit.
- **Moving.** A dragon steps north, east, south or west, and the body follows the head. It can also **sprint**
  several steps in one turn. The first ⌈L/4⌉ steps are free (L is its length), and every extra step costs one
  segment.
- **Growing.** Eating a pearl adds one segment.
- **Dying.** A head that moves into kelp or into any body, its own included, dies. Two heads that meet both die.
  A dead dragon leaves a pearl on every second segment, ⌈L/2⌉ pearls in all.
- **Splitting.** A dragon can split in two if both parts are at least 2 long. The rear part, reversed, becomes a
  new dragon with a fresh copy of the program. A team can have at most **64 dragons** alive.
- **Sonar** is the only way to talk. A dragon can send up to four 64-bit messages a turn, one in each direction.
  A beam travels in a straight line, wrapping and passing through portals, and is delivered to the first dragon
  part it hits. That dragon may be on either team.

## Winning

- Each team's lowest-id dragon is its **queen**.
- A team with no dragons left loses.
- After **500 rounds**, the tiebreaks are, in order:
  1. the longer **queen** (a dead queen counts as 0);
  2. the **longest dragon**;
  3. the **total length** of all the team's dragons.

We checked this order against all 33 of our Qualifier games that went the distance, and it gives the official
winner in every one.

## The competition

- **The ladder** ranks every team by Elo on a public pool of maps. Ranked battles are five games, and the site also
  ran "autoscrims" between teams.
- **Tournaments** are best-of-7 matches on **maps nobody has seen before**. The Qualifier on 10 Oct 2026 was a
  seeded single-elimination bracket of 280 eligible teams. It sent ten teams to the Grand Final: the eight
  quarterfinalists, plus the two finalists of a second-chance bracket for the Round-of-16 losers.

## What the rules push you towards

- **Split early.** Every dragon is another mouth, and the cap of 64 comes quickly.
- **Map alone.** With no shared memory and narrow sonar beams, each dragon remembers the map for itself.
- **Trade heads.** A head-on collision kills both dragons, so a 3-long dragon can take out a 70-long one.
- **The queen is the most valuable piece.** A living queen of length 4 beats a dead one, whatever else is on the
  board.
- **Build one long dragon at the end.** It is the second tiebreak, so the late game is about feeding length into a
  single dragon.
