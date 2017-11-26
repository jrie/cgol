# cgol
A Conway game of life simulator


## Welcome to Conway's Game of life ##

This is a competition entry for the IT-Talents.de coding competition.
By Jan R. - Version date: September 25th 2017.

Enter "cgol -h" to show the help.


### GENERAL HELP

This is Conway's Game of life.
You can either paint living cells, using the mouse in paint mode
or drag and drop a 32 bit png with alpha channel into the game window,
this will generate a cell map varying from the image.

Images should be equal in there dimensions in order to reach the best effect.
For example a one by one ratio, divisble by the cells in x and y to the image pixel dimensions.
For generation you could use The Gimp or save out images from the game using "s" key.
Example images are incuded which are all consisting of 32bit PNG images with alpha.

### SAVING IMAGES AND DRAG AND DROP

If you want to save an image, you can do so by pressing "s" key.
It is then saved to the folder "saved_images/TIMESTAMP.png".
The routine saves everything except the grid and the message displayed inside the game window.
Those images can be reimported into the application, like other images, by drag and drop.

Note that CAIRO backend png's, switch "-cb", cannot directly be imported as they are missing the alpha channel.
You have to use The Gimp or similiar to add an alpha channel, then import is working!

### INFO PANEL USAGE

If the history is enabled ("h" key) - and the info panel enabled,
three bars are shown at the bottom of the screen. When the mouse is moved on
top of those bars, either:
	- Stable cells are shown, which didnt change in this turn or survived
	- Born cells. cells which did become born during a turn
	- Dead cells, which died during this turn

If you hover over the corresponding bar, you see the exact cells highlighted.
And there counts are always displayed on the bars.

### AVAILABLE COMMANDS
-h				This help
-c5 ... 250			Amount of cells in x and y by one number from 5 to 250 (default: 50)
-ct0.0 ... 1.0			Floating point value 0.5 for 50 percent color threshold (default: 0.85)
				(rgb added together and averaged) for living cell image generation, below the set value
-mfc0.0 ... 1.0			Maximum fit cells for random number generator (default 0.4)

Start options of boolean type either 0/1 or t/f AND (also in game options/keybindings):

-gBOOL	+[KEY]			Grid enabled (t)rue or 1 or disabled (f)alse or 0 - ("g" key in game)
-aBOOL	+[KEY]			Animations enabled or disabled ("a" key in game to toggle)
-htBOOL	+[KEY]			History enabled or disabled (Toggled using "h" key in game)
-iBOOL	+[KEY]			Draw infopanel when history is enabled and one turn is processed (Toggled with "i" key)
-r	+[KEY]			Should the game start with a random playboard, be regenerated with a
				random playboard ("r" key in game)

-cb				Should cairo's png functions be used instead of libpng to save images
				Cairo backend images cannot be imported directly! Please read the help for more information.

[KEY] "s"			Saves the actual scene, including the info panel if displayed ("s" key in game)
[KEY] "+" (not numpad)		Decrease game ticks and increase game speed ("+" key in game)
[KEY] ","			Go back in history, if enabled ("," key in game)
[KEY] "."			Go forward in history, if enabled ("." key in game)
[KEY] "c"			Clear game board ("c" key in game)
[KEY] "p"			Paint mode ("p" key in game)
[KEY] "Space"			Play/Pause the game ("space" key in game)

