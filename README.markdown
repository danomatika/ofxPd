ofxPd: a Pure Data addon
===================================

[Dan Wilcox](danomatika.com) 2011

GPL v3

DESCRIPTION
-----------

ofxPd is an Open Frameworks addon for running an instance of the Pure Data audio enviornment within an OpenFrameworks application.

[Pure Data](http://pure-data.info/) is a graphical patching enviornment for audio and multimedia (note: the gui and graphics features are not within the scope of this addon) 

[OpenFrameworks](http://www.openframeworks.cc/) is a cross platform open source toolkit for creative coding in C++

WARNING: Code is not currently working, still being actively developed.

BUILD REQUIREMENTS
------------------

Currently, ofxPd is being developed on Mac OSX. You will need to install Xcode from the Mac Developer Tools. You will also need the latest version of OpenFrameworks (007 unstable) from the [https://github.com/openframeworks/openFrameworks](github repository).

BUILD AND INSTALLATION
----------------------

Place ofxPd within a folder in the apps folder of the OF dir tree:

`openframeworks/apps/myApps/ofxPd`

If you get the following [http://www.openframeworks.cc/forum/viewtopic.php?f=8&t=5344&p=26537&hilit=Undefined+symbol#p26537](linker error) in xcode:

`Undefined symbols: "std::basic_ostream<char, std::char_traits<char> ...`

you need to change the Base SDK to 10.6: Project > Edit Project Settings

USAGE
-----

DEVELOPING
----------

You can help develop ofxPd on GitHub: [https://github.com/danomatika/ofxPd](https://github.com/danomatika/ofxPd)

Create an account, clone or fork the repo, then request a push/merge.

If you find any bugs or suggestions please log them to GitHub as well.

FUTURE IDEAS/IMPROVEMENTS
-------------------------
