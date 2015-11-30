ofxPd
=====
<p align="center">
<img src="https://raw.github.com/danomatika/ofxPd/master/ofxaddons_thumbnail.png"/>
</p>

Copyright (c) [Dan Wilcox](danomatika.com) 2011-2013

BSD Simplified License.

For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "LICENSE.txt," in this distribution.

See https://github.com/danomatika/ofxPd for documentation as well as the [OF forum post on ofxPd](http://forum.openframeworks.cc/t/ofxpd/6492)

This project has been supported by the [Frank-Ratchye STUDIO for Creative Inquiry at Carnegie Mellon University](http://studioforcreativeinquiry.org).

Description
-----------

ofxPd is an Open Frameworks addon for running an instance of the Pure Data audio environment within an OpenFrameworks application. Audio, messages, and [MIDI](http://en.wikipedia.org/wiki/Musical_Instrument_Digital_Interface) events can be passed to and from pure data patches and the library is thread safe.

[Pure Data](http://pure-data.info) is a graphical patching environment for audio and multimedia (note: the gui and graphics features are not within the scope of this addon) 

[OpenFrameworks](http://www.openframeworks.cc) is a cross platform open source toolkit for creative coding in C++

Build Requirements
------------------

To use ofxPd, first you need to download and install Open Frameworks. Development is against the latest version of Open Frameworks on github. Checkout a tag if you need an earlier, stable version.

[OF github repository](https://github.com/openframeworks/openFrameworks)

On Mac OSX, you will need to install Xcode from the Mac Developer Tools.

On Linux, you can use the Makefile and/or Codeblocks project files (without the "_win" suffix).

On Win, you will need [Codeblocks+MiniGW](http://www.codeblocks.org/downloads/26) and the [Win Codeblocks OF package](http://www.openframeworks.cc/download). Use the Codeblocks projects files with the "_win" suffix. Also, some versions of CB+MinGW are missing some libraries needed by OF (pthreads, etc). Make sure you've followed the [OF Win Codeblocks setup guide](http://openframeworks.cc/setup/codeblocks).

Installation
------------

Place ofxPd within a folder in the apps folder of the OF dir tree:

    openframeworks/addons/ofxPd

#### Which version to use?

If you are using a stable version (0062, 007, ...) of OpenFrameworks then you want to use a git tag of ofxPd for that version. You can select the tag in the Github "Current Branch" menu or clone and check it out using git.

For example, the following commands will clone ofxPd and switch to the OF 0062 tagged version:

    git clone git://github.com/danomatika/ofxPd.git
    cd ofxPd
    git checkout 0062

Running the Example Projects
----------------------------

The example projects are in the `pdExample` & `pdExampleIOS` folders.

Project files for the examples are not included so you will need to generate the project files for your operating system and development environment using the OF ProjectGenerator which is included with the OpenFrameworks distribution.

Point the ProjectGenerator to `addons/ofxPd`, change the project name to the **exact** name of the example you want to generate (ie `pdExample`), and make sure to choose `ofxPd` from the addons. Hitting "Generate Project" will populate that example with the project files you will need to build it.

### OSX

Open the Xcode project, select the "pdExample Debug" scheme, and hit "Run".

### Linux

Open the Code::Blocks .cbp and hit F9 to build. Optionally, you can build the example with the Makefile.

To build and run it on the terminal:

    make
    make run

### Windows

An example Codeblocks workspace is included. Visual Studio is not currently supported by the libpd source.

PitchShifter
------------

PitchShifter is a simple example application which uses an OF gui to control a pitch shifter within a PD patch. It's in `pitchShifter` and `pitchShifterIOS`. Like the basic example, you will have to generate the project files using the ProjectGenerator. 

How to Create a New ofxPd Project
---------------------------------

_Note: These instructions are for manually creating a new project from an existing ofxPd project and it's project files (aka Xcode, C::B, etc). You do not need to follow these steps if you use the ProjecGenerator app in which case you *may* need to add the C Flags as the PG currently seems to have a problem doing this. See the IDE specific instructions on how to do this._

To develop your own project based on ofxPd, either generate a new project with the ProjectGenerator or generate one of the examples, copy, and rename it. You probably want to put it in your apps folder, for example, after copying:

    openFrameworks/addons/ofxPd/pdExample/ => openFrameworks/apps/myApps/pdExample/

It must be 3 levels down in the openframeworks folder structure.

Then after renaming:

    openFrameworks/apps/myApps/myPdProject/

### For Xcode:

Rename the project in Xcode (do not rename the .xcodeproj file in Finder!): Xcode Menu->Project->Rename

### For Codeblocks (Win & Linux):

Rename the *.cbp and *.workspace files to the same name as the project folder. Open the workspace, readd the renamed project file, and remove the old project.

Adding ofxPd to an Existing Project
-----------------------------------

_Note: These instructions are for manually add ofxPd to an existing project. You do not need to follow these steps if you use the ProjecGenerator app in which case you *may* need to add the C Flags as the PG currently seems to have a problem doing this. See the IDE specific instructions on how to do this._

If you want to add ofxPd to another project, you need to make sure you include the src folder:

    openFrameworks/addons/ofxPd/src

You will also need to include some additional C Flags for building the libpd source:

	-DHAVE_UNISTD_H -DHAVE_ALLOCA_H -DUSEAPI_DUMMY -DPD -DLIBPD_EXTRA

_Note: **-DLIBPD_EXTRA** is optional if you do not need/use the externals in `libpd/pure-data/extra`_

### For Xcode:

* create a new group "ofxPd" 
* drag these directories from ofxpd into this new group: ofxPd/src
* add a search path to: `../../../addons/ofxPd/libs/libpd/pure-data/src` under Targets->YourApp->Build->Header Search Paths (make sure "All" is selected)
* under Targets->YourApp->Build->**Other C Flags** (make sure "All" is selected), add
	<pre>-DHAVE_UNISTD_H -DHAVE_ALLOCA_H -DUSEAPI_DUMMY -DPD -DLIBPD_EXTRA</pre>
  * _Note: Make sure you use Other **C** Flags! Other **C++** Flags will **not** work since libpd is written in C._

### For Linux (Makefiles & Codeblocks):

* edit addons.make in your project folder and add the following line to the end of the file: 
	<pre>ofxPd</pre>

### For Codeblocks (Win):

* add the ofxPd sources to the project:
	* right-click on your project in the project tree
	* select "Add Files Recursively ..."
	* navigate and choose the `ofxPd/src` & `ofx/libs` folder
	* remove `ofxPd/libs/libpd/pure-data/extra/pd~` as it dosen't currently build in Win CB
* add defines, search paths, and libraries to link:
	* right-click on your project in the project tree
	* select "Build options..."
	* make sure the project name is selected in the tree (not release or debug)
	* select the "Compiler settings" tab, add the following to the "#defines" tab:
	<pre>
	HAVE_UNISTD_H
	USEAPI_DUMMY
	LIBPD_EXTRA
	MSW
	PD
	PD_INTERNAL
	WINVER=0x502
	</pre>
	* select the "Search directories" tab, click add the search paths:
	<pre>
	..\\..\\..\addons\ofxPd\src
	..\\..\\..\addons\ofxPd\libs\libpd\pure-data\src
	..\\..\\..\addons\ofxPd\libs\libpd\libpd_wrapper
	</pre>
	* select the Linker settings" tab, add the following libraries:
	<pre>
	pthread
	</pre>
	
Notes
-----

### Audio Interfacing & Debugging Audio Issues

libpd as utilized in ofxPd does not handle any of the audio interfacing itself, but is called via the ofSoundStream system within OpenFrameworks. If you have any issues with the sound interface / audio apis themselves, please log an issue to the [OpenFrameworks Github repo](https://github.com/openframeworks/openFrameworks).

### Sample Rate

The sample rate is set to 44100 when initializing ofxPd in the examples. If your sample rate is higher, the playback pitch will be higher. Make sure the sample rate is the same as your system audio sample rate to hear the correct pitch.

For example: The default sample rate on Mac OSX is 96000. Running the app at 44100 results in double the playback pitch while initing ofxPd at 96000 gives the correct pitch.

### Running App in the Background on iOS

If you're using ofxPd to build an audio app on iOS, you probably want the app to keep running while in the background (aka switching between other apps or going to the home screen). You can enable this in Xcode by clicking on the Project in the project tree, selecting the "Capabilities" tab, and turning on the "Background Modes" switch, then checking "Audio, Airplay and Picture in Picture". Next, Set "Application does not run in background" to NO in the "Info" tab.

### Disabling Automatic Screen Locking on iOS

You may be building an audio app for iOS that you want to run without the automatic screen locking mechanism closing it. You can disable the screen lock timer by adding the following to your ofApp setup() function:

    [[UIApplication sharedApplication] setIdleTimerDisabled:YES];

Bugs & Errors
-------------

### Pitch is off on the iPhone 6S

The iPhone 6S hardware seems to prefer a sample rate of 48000 and calling ofSoundStreamSetup() with 44100 will not change that in versions of OF 0.8.4 and previous. This means ofxPd will be running at 44100 but the audio stream is actually 48000, resulting in a higher pitch coming out of your patches and a lower pitch going in.

The fix is to follow Apple's method of setting the *preferred* sample rate, then grabbing what the *actual* sample rate is afterwards. You can then use this real value in ofSoundStreamSetup() and ofxPd::init(). The pdExampleIOS has been updated to show how to do this. Hopefully, this funtionality will be added to OF in the future.

### File "tr1/memory" not found in Xcode

You just upgraded to OSX 10.9 and Xcode 5 right? The default compiler is now LLVM and you need to rebuild your Xcode project files so OF will build correctly. Ude the ProjectGenerator in the OF 0.8.0 download to regenerate the project:

* choose the _parent folder_ of your project folder
* set the name of the project
* add ofxPd as an add-on
* hit generate

Also note, currently the PG doesn't seem to set the C Flags correctly, so you might have to add them manually. See "Adding ofxPd to an Existing Project" on how to do this.

### Unknown type `t_float`, etc

The compiler doesn't recognize the internal Pd types because it's missing the C Flags needed to build libpd. See the section for your IDE in "Adding ofxPd to an Existing Project" on how to do this. 

### Undefined basic_ostream in XCode

If you get the following [linker error](http://www.openframeworks.cc/forum/viewtopic.php?f=8&t=5344&p=26537&hilit=Undefined+symbol#p26537) in XCode:

    Undefined symbols: "std::basic_ostream<char, std::char_traits<char> ...

you need to change the Base SDK to 10.6: Project > Edit Project Settings

### RtAudio Hang on Exit in 0062

RtAudio will hang on app exit in OF 0062. The only way to fix this is to make a small edit to the OF 0062 core by editing `lib/openFrameworks/sound/ofSoundStream.cpp` and commenting line 143 so close() is not called.

### "verbose" redefinition in Win Codeblocks

Currently, there is a static function in the videoinput lib on Win that conflicts with a #define in the Pure Data sources. The easy fix, until the OF core is updated, is to comment out line 115 in `libs\videoInput\include\videoInput.h`.

Note: This change hasn't been tested while using the ofVideoGrabber yet ... there is a slight chance it may cause a crash, be warned.

### "undefined reference to SetDllDirectory" in Win Codeblocks

Newer versions of libpd check the windows version, so this needs to be set via a define in your codeblocks project. If you have an old ofxPd project, this is not set, hence the error. See "Adding ofxPd to an Existing Project" above, and add the `WINVER=0x502` define to your project.
	
Adding Pure Data external libraries to ofxPd
--------------------------------------------

ofxPd only includes the standard set of Pure Data objects as found in the "Vanilla" version of PD. If you wish to include an external library from Pd-Extended, etc you need to include the source files in your project and call the library setup function after initializing ofxPd in order to load the lib.

### Adding external source files

The source files for externals included with Pd-Extended can be found in the Pure Data Subversion repository on Sourceforge. It is recommended that you use the latest Pd-Extended release branch as it will be more stable then the development version. See http://puredata.info/docs/developer/GettingPdSource

For example, if we want to include the zexy external in our project, we first download the sources files for the latest stable Pd-Extended (0.42 as of this writing) from the Subversion repository (make sure you have svn installed):

    svn checkout https://pure-data.svn.sourceforge.net/svnroot/pure-data/branches/pd-extended/0.42

The external sources can be found in the `externals` folder. For instance, the zexy sources are in `externals/zexy/src/`. Copy the .h and .c files into your project folder. In my case I create an externals folder in src folder of my project, something like `myProject/src/externals/zexy`. Then add these files to your ofxPd project.

Note: Some libraries may require external libraries of their own and/or special compile time definitions. Make sure you read the build documentation on the external and include these with your project. 

### Calling the external setup function

In order for libpd to use an external library, the library has to register itself on startup. This accomplished by calling the library's setup function which is named after the library followed by a "_setup" suffix: "library_setup()". The zexy setup function is simply "zexy_setup()". Call this setup function after initializing ofxPd in your app's setup() function:
<pre>
	if(!pd.init(numOutChannels, numInChannels, sampleRate, ticksPerBuffer)) {
		OF_EXIT_APP(1);
	}
	
	// load libs
	zexy_setup();
</pre>

If all goes well, you should see some sort of print from the library as it initializes:
<pre>
[zexy] part of zexy-2.2.3 (compiled: Aug  7 2011)
	Copyright (l) 1999-2008 IOhannes m zmölnig, forum::für::umläute & IEM
[&&~] part of zexy-2.2.3 (compiled: Aug  7 2011)
	Copyright (l) 1999-2008 IOhannes m zmölnig, forum::für::umläute & IEM
[.] part of zexy-2.2.3 (compiled: Aug  7 2011)
	Copyright (l) 1999-2008 IOhannes m zmölnig, forum::für::umläute & IEM
...
...
...
</pre>

For C++ and some C libraries, this is all your need. The project should compile and the external load fine. However, some pure C libraries are not written with explicit C++ support in mind and, for arcane reasons best not delved into here, the C++ compiler will not be able to find the library's setup function.  This is the case with zexy and the compiler error looks like this:
<pre>'zexy_setup' was not declared in this scope</pre>

In order for the C++ compiler to find the function, we need to add our own declaration. This can be done in your app .cpp file, a project header file, etc. In order to keep things organized, I create an "Externals.h" header file and place it in `myProject/src/externals`. Here I declare the zexy_setup function using a special syntax:
<pre>
#pragma once

extern "C" {
	void zexy_setup();
}
</pre>

The `extern "C"` keywords tell the compiler to look for a pure C function, not a C++ function. Make sure to include the "Externals.h" header file where you include "ofxPd.h". Add a setup function declaration for any other externals that need it here.

### External library licensing on iOS

Apple's iOS and App Store policies forbid dynamically linking libraries. As such, you cannot include any GPL licensed externals as the GPL expressly requires dynamic linking. Submitting an app using a GPL library is in violation of the GPL and will most likely result in your app being rejected from distribution in the App Store.

GPL patches, however, are not in violation of GPL distribution policies and can be included. They are not compiled into an application binary and can be replaced by the user.

Developing ofxPd
----------------

You can help develop ofxPd on GitHub: [https://github.com/danomatika/ofxPd](https://github.com/danomatika/ofxPd)

Create an account, clone or fork the repo, then request a push/merge.

If you find any bugs or suggestions please log them to GitHub as well.
