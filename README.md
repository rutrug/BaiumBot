# BaiumBot: Terran macro extension based on CommandCenter
![Baium Screenshot](https://vignette.wikia.nocookie.net/starcraft/images/4/4c/ArcturusMengsk_SC2_Art2.jpg/revision/latest?cb=20110814133340)

WIP!

Goals for now:
* Prepull for building constructions
* Creating complete wallins
* Effective and efficient building placement

Used libraries:
* Overseer library (https://gitlab.com/OverStarcraft/Overseer)

# CommandCenter: AI Bot for Broodwar and Starcraft II

CommandCenter is a StarCraft AI bot that can play both StarCraft: Broodwar and StarCraft 2.

CommandCenter is written in C++ using [BWAPI](https://github.com/bwapi/bwapi) and Blizzard's [StarCraft II AI API](https://github.com/Blizzard/s2client-api). It provides many wrapper functions around both APIs that allow it to perform the same functionality in both games via the same source code. It is written by [David Churchill](http://www.cs.mun.ca/~dchurchill/), Assistant Professor of [Computer Science](https://www.cs.mun.ca/) at Memorial University, and organizer of the [AIIDE StarCraft AI Competition](http://www.cs.mun.ca/~dchurchill/starcraftaicomp/).

CommandCenter is based on the architecture of [UAlbertaBot](https://github.com/davechurchill/ualbertabot/wiki), and is intended to be an easy to use architecture for you to quickly modify, play with, and build your own bot. The bot itself does not contain much in the way of hard-coded strategy or tactics, however it provides a good starting point for you to implement your own strategies for any race. 

CommandCenter currently provides the following features:
* Plays both Starcraft games with the same source code
* Plays all 3 races with generalized micro controllers for combat units
* Performs online map analysis, extracting information such as base locations and expansions
* Keeps track of all previously seen enemy units and their last known locations on the map
* Has a WorkerManager which manages resource gathering and worker allocation / buiding
* Is able to carry out predefined build-orders written in a configuration file
* Allows you to easily create your own build-orders and modify them on the fly in-game
* Contains a building placement algorithm, finding the closest buildable location to a given position for a given building 
* Scouts the map with a worker unit, discovering where the enemy base is located
* Once a specific condition has been reached (having 12 combat units, by default), it will commence an attack, sending waves of units at the enemy base
* Squads can be formed, consisting of multiple units following a specific order such as attack or defend a given location


