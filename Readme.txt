Configuration:
==========================================================================

1. Install OpenCV. Download from OpenCV website http://opencv.org/releases.html. Run the installation exe to extract files under a directory, e.g. C:/.
2. Edit Environment Variables. 
   System Properties -> Advanced -> Environment Variables -> User Variables: Add a new user variable named opencv: C:/opencv/build. If opencv is installed in other directories, then change the value of the variable accordingly
   System Properties -> Advanced -> Environment Variables -> System Variables: Edit the Path variable: %opencv%\x64\vc14\bin\.
3. Visual Studio Configuration 
    For Debug Configuration:
    Inside Visual Studio. Right Click Project Illegal_parking_detection.
    VC++ Directories -> Include Directories: C:\opencv\build\include\opencv2
											 C:\opencv\build\include\opencv
											 C:\opencv\build\include
    VC++ Directories -> Library Directories: C:\opencv\build\x64\vc14\lib
	Linker -> General -> Additional Library Directories: C:\opencv\build\x64\vc14\lib
	Linker -> Input -> Additional Dependencies: opencv_world310d.lib

Run the program:
=============================================================================
1. Use windows command window, go to directories where the compiled Illegal_parking_detection.exe is. If you are using x64 platform, and you keep the other project configuration the same, your exe should be under projectfolder/x64/Debug/.
2. Make sure these 4 files are under the same directories with exe: haar_left.xml haar_side.xml haar_front.xml YIH_Video_Cut_3.mpg.
   the 3 haar_*.xml files are car classfier trained by opencv haar cascade training program. Instructions on how to train your own classifier using haar cascade training can be found here:
   https://achuwilson.wordpress.com/2011/07/01/create-your-own-haar-classifier-for-detecting-objects-in-opencv/
   the last file is the surveillance video that you wish to process.
3. Type below command to run the illegal parking detection program:
   Illegal_parking_detection.exe haar_left.xml haar_side.xml haar_front.xml YIH_Video_Cut_3.mpg.
   You can replace the argument with your own classifier file names or your own surveillance video names.
