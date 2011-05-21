ofxPd: a Pure Data addon
===================================

[Dan Wilcox](danomatika.com) 2011

DESCRIPTION
-----------

ofxPd is an Open Frameworks addon for running an instance of the Pure Data audio enviornment within an OpenFrameworks application.

[Pure Data](http://pure-data.info/) is a graphical patching enviornment for audio and multimedia (note: the gui and graphics features are not within the scope of this addon) 

[OpenFrameworks](http://www.openframeworks.cc/) is a cross platform open source toolkit for creative coding in C++

WARNING: Code is working, but still being actively developed.

BUILD REQUIREMENTS
------------------

To use ofxPd, first you need to download and install Open Frameworks. ofxPdExample-beta.xcodeproj is developed against the latest version of Open Frameworks on github, while ofxPdExample.xcodeproj will work with the 0062 release.

[github repository])https://github.com/openframeworks/openFrameworks)

Currently, ofxPd is being developed on Mac OSX. You will need to install Xcode from the Mac Developer Tools.

The code should work on other platforms, but requires platform specific project files to be built.

BUILD AND INSTALLATION
----------------------

Place ofxPd within a folder in the apps folder of the OF dir tree:

`openframeworks/addons/ofxPd`

### Bugs

#### Undefined basic_ostream

If you get the following [linker error](http://www.openframeworks.cc/forum/viewtopic.php?f=8&t=5344&p=26537&hilit=Undefined+symbol#p26537) in xcode:

`Undefined symbols: "std::basic_ostream<char, std::char_traits<char> ...`

you need to change the Base SDK to 10.6: Project > Edit Project Settings

#### RtAudio Hang on Exit in 0062

RtAudio will hang on app exit in OF 0062. The only way to fix this is to make a small edit to the OF 0062 core by editing /lib/openFrameworks/sound/ofSoundStream.cpp and commenting line 143 so close() is not called.

### How to Create a New ofxPd Project

To develop your own project based on ofxPd, simply copy the example project and rename it. You probably want to put it in your apps folder, for example, after copying:

openFrameworks/addons/ofxPd/example/ => openFrameworks/apps/myApps/example/

It must be 3 levels down in the openframeworks folder structure.

Then after renaming:

openFrameworks/apps/myApps/myPdProject/

On Mac, rename the project in XCode (do not rename the .xcodeproj file in Finder!): XCode Menu->Project->Rename

### Adding ofxpd to an Existing Project

If you want to add ofxPd to another project, you need to make sure you include the src folder:
<pre>
openFrameworks/addons/ofxPd/src
</pre>

You will also need to include some additional C Flags for building the libpd source:

#### For XCode:

* create a new group "ofxPd" * drag these directories from ofxpd into this new group: ofxPd/src & ofxPd/libs
* add a search path to: ../../../addons/ofxPd/libs/libpd/pure-data/src under Targets->YourApp->Build->Library Search Paths (make sure All Configurations and All Settings are selected)
* under Targets->YourApp->Build->Other C Flags (make sure All Configurations and All Settings are selected), add
	<pre>-DHAVE_UNISTD_H -DUSEAPI_DUMMY -DPD -dynamiclib -ldl -lm</pre>

#### For Linux:

* edit addons.make in your project folder and add the following line to the end of the file: 
	<pre>ofxPd</pre>
* edit config.make in your project folder and change the lines for USER_CFLAGS, USER_LDFLAGS and USER_LIBS to:
	<pre>USER_CFLAGS = -DHAVE_UNISTD_H -DUSEAPI_DUMMY -DPD -shared</pre>
	<pre>USER_LDFLAGS = --export-dynamic</pre>
	<pre>USER_LIBS = -ldl -lm</pre>

DEVELOPING
----------

You can help develop ofxPd on GitHub: [https://github.com/danomatika/ofxPd](https://github.com/danomatika/ofxPd)

Create an account, clone or fork the repo, then request a push/merge.

If you find any bugs or suggestions please log them to GitHub as well.

FUTURE IDEAS/IMPROVEMENTS
-------------------------
