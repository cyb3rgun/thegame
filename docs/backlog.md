# Backlog

Index of everything known to be built. Items marked (G02) belong to season G02.

## Simulator Core

- [ ] Ballistics model, camera and scenario agnostic
- [ ] Fire control (trigger modes, refire, magazine)
- [ ] Hit scoring service shared by all scenarios (G02)
- [ ] Session statistics (hits, misses, reaction times)
- [ ] Scenario package format: GameMode, data assets, input context (G02)

## Scenarios

- [ ] Western: saloon doors, undead bandits, living customers must not be shot
- [ ] Zombie: first entertainment scenario, shared core for three sub modes
- [ ] Zombie first person sub mode
- [ ] Zombie top down sub mode
- [ ] Zombie target range sub mode
- [ ] Olympic: precision, real physics, no time dilation
- [ ] Scenario selection screen in the style of arcade lightgun cabinets

## Modules

- [ ] Door module: twelve door circle, three visible, friend or foe under time pressure (G02)
- [ ] Door module: draw timing bonus, waiting for the draw scores higher (G02)
- [ ] Door module: Western dressing (saloon doors)
- [ ] Door module: Zombie dressing
- [ ] Horde module: chain ignition crowds
- [ ] On rails module (open option): guided flight or drive with mass targets

## Weapons

- [ ] Pistol placeholder from the first person template hooked to scoring (G02)
- [ ] Digital twin of the real CYB3RGUN weapon
- [ ] Improvised torch attachment: pulsed bursts, five metre flame cone
- [ ] Torch chain ignition between burning enemies
- [ ] Torch fire light attracts enemies
- [ ] Torch self destruction roll with heat bonus and hiss warning
- [ ] Torch radial damage on detonation
- [ ] Overclock time dilation, diegetic, coupled to the HOLD trigger

## Enemies

- [ ] Placeholder occupants: red hostile, green friendly basic shapes (G02)
- [ ] Undead standard horde
- [ ] Undead bear (tank)
- [ ] Undead chicken (swarm)
- [ ] Undead clown (disruptor, horn audio signature)
- [ ] Machine turrets
- [ ] Machine drones

## UI

- [ ] Score and wave display for the door range (G02, on screen debug text only)
- [ ] Scenario selection menu
- [ ] Session statistics screen
- [ ] Ironic safety splash screen spoken by the arena AI (open option)

## Audio

- [ ] Weapon fire and hit feedback
- [ ] Clown horn signature
- [ ] Torch hiss warning before detonation
- [ ] Door open and close sounds

## Infrastructure

- [ ] Door range level Lvl_DoorRange built via MCP (G02)
- [ ] Door range settings data asset with timings and probabilities (G02)
- [ ] USB HID controller input from the real CYB3RGUN
- [ ] IMU aiming input from the real CYB3RGUN
- [ ] Laser tag event fusion
- [ ] Free asset sourcing before any purchase
