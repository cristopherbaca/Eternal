# Living-world prototype

The seeded atlas streams landmarks, towns, residents and wildlife around the player. Fog and discoveries persist. Weather and day/night follow the saved clock.

| Key | Action |
| --- | --- |
| F | Enter a nearby site; interact with chest/book/bed or exit inside |
| Y | Talk to a nearby resident: work, rumors, trading, hiring or civic actions |
| J / L | Quest journal / learned history |
| U / G | Mastery, talents and crafting / gather local materials |
| M | Atlas: WASD pan, +/- zoom, C center, Tab select, Enter travel to visited site |
| I | Inventory: J/K select, Enter equip/remove |

Visit the starter village, approach a resident and press Y. Choose Work, then J to select the destination on the atlas. For dungeon jobs, enter with F, defeat the guardians, open the chest with F and return to the giver. Rumors reveal destinations but do not unlock fast travel. Enter peaceful buildings through their doors with F.

Use G to collect local materials once per chunk per game day. U shows recipe costs and talent requirements. Carrying capacity is 60; sell or equip through the existing inventory and shop panels. Theft witnessed by residents incurs a town bounty. Guards offer a fine or a time/XP penalty.

Saves include discoveries, cleared interiors, NPC balances, quests, lore, talents, materials and bounties. Unloaded ambient wildlife regenerates. Shops have renewable stock; relationships are generated references rather than autonomous social simulation. These features extend the existing renderer and combat; this is still a prototype.

Repeat Work requests from the same resident cycle through dungeon recovery, letter delivery and escort travel. Delivery carries the sealed letter automatically: speak with Y to the named recipient (marked ? nearby), then return to the giver. An escort is the gold `e` on the map. Stay within 35 units, clear nearby enemies, and lead the traveler around obstacles to the destination. Fast travel and interiors are disabled while escorting. The traveler waits if you run ahead; return to their map position to resume. Escorts currently cannot be injured and return to their home schedule after arrival. J shows detailed objectives and points back to the giver when ready to collect the reward. Save v6 preserves active quest progress and still reads versions 1–5.
