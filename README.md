![image](https://user-images.githubusercontent.com/16034536/166428164-f834d80b-f1b5-4433-adfe-22a2d1259b1d.png)

ANTPLOT
=======

These are the sources for the Antplot application. The code is written in Microsoft Visual C++. It makes use of intrinsic 64-bit instructions, and should work on any 64-bit PC, but has only been tested under Windows 10.

The executable file can be downloaded from the Github repository Releases section.

The app waits for single character commands (type h to display the list). To display the antplot of a reciprocal, use the v command, then enter an integer value. To increment the value and display the antplot of the next reciprocal, use the n command. To go back 2 values, use the b command.

By default the app comes up in reciprocal mode. To switch to factorial mode instead, use the m command.

The f command steps through figures which have been used in my paper "Antplot: Visualising Long Binary Strings Using a Variation of Langton's Ant". Please feel free to generate your own antplots, and experiment with the source code. The length of the calculations is governed by the #define at the top of Alu.h. The default value of 2048 means that we effectively have a 131072-bit ALU. Increasing this value will affect execution speed though.

The calculations of e and pi are not particularly optimal, but they seem to work OK for the purposes of generating their antplots.
