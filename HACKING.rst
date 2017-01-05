

At least in digram mode, colors are used to indicate which is new or
old in a data block. things at the front are yellow, theyll be the
first to disapear. blue stuff is new.


├── resources			# Just icons and basic app ressources.
├── include			# See src.
├── src
│   ├── data			# copyBits, toString, repack stuff.
│   ├── db			# a DB I guess, auto extract w/ /parser/
│   ├── dbif			# DB interface.
│   ├── parser			# parse PNG, pyc.
│   ├── ui
│   ├── util
│   │   ├── encoders
│   │   ├── sampling
│   │   └── settings
│   └── visualisation		# All algos live here.
│       └── shaders		# And shaders are here.
│           ├── digram
│           ├── minimap
│           └── trigram
└── test			# Simply unit tests, etc look later.


# something to note:

 - for digrams a histogram is built in C++ (digram.cc / initTextures())
 - but not for trigrams, for those we juste paint all the dots in the
   shader and count on cumulative transparency to create a histogram
   effect.

   This also makes the projection harder.
