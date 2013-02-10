HammerQt Synthetic Environment
========
Copyright Simon Butler (2010)

Introduction
------------

This is the simulator for my [PhD thesis](http://simon-butler.com/Thesis). 

Compilation
-----------

You will need to install [Delta3D](http://www.delta3d.org) and all of its dependencies. You also need my [clustering library](https://github.com/pthimon/clustering.)

Tested using Delta3D version 2.2.0 and OpenSUSE 12.1

Additional dependencies are:
* `libgsl`
* `libagg`
* `libqwt5`

See `hammerQt.pro` for the full list of dependencies. Edit `custom.pri` to specify where your Delta3D installation is.

To compile, first copy the [data files](http://simon-butler.com/hammerQt-data.tar.gz) to the hammerQt directory, then run:
`qmake`
`make`

Execution
---------

To run:
`./hammerQt --gui`

For more details of the program operation run
`./hammerQt --help`

Controls
--------

Controls for 3D view:
* Left mouse button: Select unit (click and drag to select multiple), click again to set destination, ctrl+click to set waypoint
* Right mouse button: Move camera
* Middle mouse button: Rotate camera
* Middle mouse scroll: Zoom camera
