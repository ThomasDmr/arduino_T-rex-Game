# arduino_T-rex-Game

## Project
The Google Chrome T-rex game is both fun and challenging and is the most played game in the world ! 
It is also really interesting in a pure exercise point of view, since the rules are simple, but it is a game you can only loose or play infinitely.

Some nice projects used Arduino to electro-mechanically play this game ([here](https://create.arduino.cc/projecthub/GAURAVK5/automated-chrome-dino-game-using-arduino-ab69c4) or [here](https://create.arduino.cc/projecthub/bastifantasti/arduino-playing-dino-game-7d548e)) and I thought it would be interesting to push this a little further in my course about asynchronous programing.
This approach allows for smarter obstacle detection and higher scores while remaining fun and technically interesting.

## Asynchronous approach
This code was developped for educational purposes to illustrate asynchronous programming. 
This allows to run several tasks like they were running in parallel. 
Using this, we can separate the fonctions of detecting obstacles and jumping which is a serious limitation in the non-asynchronous version. 
It also allows to better deal with color changes and the speed increase. 

## Results
To get an idea of what we can achieve, you can find one of my results [here](https://www.youtube.com/watch?v=cc15wAzq5-w)

See also : https://thomasdmr.com/portfolio/trex-game/
