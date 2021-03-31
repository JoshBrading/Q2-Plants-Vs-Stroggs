
This is the complete source code for Quake 2, version 3.19, buildable with
visual C++ 6.0.  The linux version should be buildable, but we haven't
tested it for the release.

The code is all licensed under the terms of the GPL (gnu public license).  
You should read the entire license, but the gist of it is that you can do 
anything you want with the code, including sell your new version.  The catch 
is that if you distribute new binary versions, you are required to make the 
entire source code available for free to everyone.

The primary intent of this release is for entertainment and educational 
purposes, but the GPL does allow commercial exploitation if you obey the 
full license.  If you want to do something commercial and you just can't bear 
to have your source changes released, we could still negotiate a separate 
license agreement (for $$$), but I would encourage you to just live with the 
GPL.

All of the Q2 data files remain copyrighted and licensed under the 
original terms, so you cannot redistribute data from the original game, but if 
you do a true total conversion, you can create a standalone game based on 
this code.

Thanks to Robert Duffy for doing the grunt work of building this release.

John Carmack
Id Software
 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 

# **Plants Vs. Strogg’s**

A Plants Vs. Zombies mod for Quake 2. Plants defend the players house from zombies, if the zombies reach the house the player loses.
Note: This ONLY works with the provided map, "The Standoff" in the bonus folder.
Type "start" in cmd to start the game.
Use left and right bracket to cycle plants.

## **Plants**

*Sunflower
 *Produces 25 sun per click
*Wall-nut
 *Deal no damage and takes awhile to break down
*Peashooter
 *Shoots a single projectile per shot
*Repeater
 *Shoots projectiles in bursts of 2
*Threepeater
 *Shoots 3 projectiles across 3 lanes per shot
*Potato Mines
 *Landmine that takes awhile to charge, Kills anything that steps on it
*Cherry Bombs
 *Explodes a few seconds after being placed
 

## Stroggs (Zombies)

- Zombie
	- Basic zombie, dies fairly quickly
- Cone Head Zombie
	-   Takes more damage to kill
- Pole Vaulting Zombie
	-   Can vault over wall-nuts but has no health boost
- Peashooter Zombie
	-   Takes the same damage to kill as the basic zombie but can also shoot projectiles at the plants

## **Gameplay**

- 5 x 9 play area
- Top down camera
- Zombies can only walk straight ahead, plants can only shoot straight ahead
- Sun drops
-  Clicking sun drops gives the player 25-50 suns
- Store
-  spend sun to buy plants to place on the map
-  Click to place plants
- End game
-  If the Strogg's (zombies) make their way through all the plants and off the game board the game is lost

## **Deliverables**
- PvZ mechanics
- 7 plants
- 4 zombie types
- Plant shop


![Video Demo](https://user-images.githubusercontent.com/50903485/113087649-17891c80-91b2-11eb-8f9f-78f8faf2cd3a.mp4)


