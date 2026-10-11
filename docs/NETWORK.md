# Multiplayer protocol

`game_server` runs the same deterministic 60 Hz simulation as offline mode and publishes state every third tick (20 Hz). The client buffers snapshots and interpolates positions with 100 ms of presentation delay. Only the server changes health, mana, cooldowns, enemy AI, boss phases, inventory, loot and XP. The offline game does not require a socket or server.

```sh
./build/game_server --port 7777
./build/eternal_rpg --connect 127.0.0.1:7777 --name Senje
./build/eternal_rpg --connect 127.0.0.1:7777 --name Elowen --class Cleric
```

Enter opens chat. `/who` lists player IDs, `/invite ID` invites another player, `/accept` joins their group. Invited players appear in party panels. All players participate in friendly cooperative PvE; nearby players share kill XP. Press `B` to travel to the boss before the encounter starts.

The server listens on loopback by default. For trusted LAN play, use `--bind 0.0.0.0` and connect to the host's LAN IPv4 address. This is an eight-client, in-memory cooperative prototype: no accounts, persistent realm database, encryption, matchmaking or Internet-scale deployment.

Frames: 4-byte unsigned big-endian byte length followed by UTF-8/ASCII text. Maximum payload 512 KiB, bounded queues, idle handshake timeout, command rate limit, sanitized chat. Commands are small and the server clamps direction/speed and checks costs/cooldowns. Movement input expires after 180 ms without renewal. Connections handle split/combined frames and partial sends.

Client sends `HELLO 1 "name" classIndex`; server replies `WELCOME id seed`. Commands: `MOVE x y`, `CAST slot aimX aimY targetId`, `CLASS index`, `LOOT`, `EQUIP inventoryIndex`, `RESPAWN`, `TRAVEL boss`, `CHAT "text"`, `INVITE id`, `ACCEPT`. There is no command for setting positions, HP, MP or gold directly. Snapshots contain entity, effect, loot and combat-text records; `NOTICE` carries chat/system messages.

Validation: `python3 tools/network_smoke.py build` starts a disposable loopback server, verifies movement expiry, rejects impossible input, checks authoritative MP, exchanges chat/invites, then runs two actual terminal clients headlessly. Socket tests require permission to bind loopback in restricted environments.
