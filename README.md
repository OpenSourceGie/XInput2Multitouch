# XInput2Multitouch

## Introduction
This program will take touch data from a Touch enabled monitor and control simple 3d shapes.  Right now it starts as a cube, and one
finger will cause it to spin.  If you add a second finger it will split into two pyramids and each finger will control how they spin.
This version of the program will also output debug information for each finger.

## Background
There isn't a lot of information out there on using the Multitouch capabilities of X11.  An extension to the XInput API was made
a few years back and was named XInput2.  We hope this program serves as an example of how to enable X11 Multitouch Events, as well
as process them.

## How to build
We have provided the CodeBlocks project.  So you can build it with CodeBlocks on Linux.  It does rely on the X11 and OpenGL dev libraries, so 
you will need to download those into your Linux installation before building.  You will also need a Multi Touch Enabled Monitor to generate
the Multitouch Events.

## Future
* We found some performance issues which we were unable to root cause.  We did see X taking quite a bit of CPU as more fingers were added
and tracked.  We need to get to the bottom of this to find the bottleneck.  We are hoping it isn't the XInput2 architecture, but
instead the way we are rendering the 3d shapes through OpenGL.  So some more investigation needs to be performed to isolate the
bottleneck.
* We would like for the 3d shape to split up for each new touch added.  Would be fun to get this to work.
* We would like to add some physics to calculate distance, velocity, acceleration, and direction of the touch events.
* We would like to add gesture processing which would process the XInput2 Multitouch Events and convert them to other XInput Events
* Multitouch Event recording and playback for testing purposes without the need for a Multitouch Monitor.

