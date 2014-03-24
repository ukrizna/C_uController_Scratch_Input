Scratch_Input
=============

Code in C for microcontroller. Project is an alternate implementation of Scratch Input (Originally done by Dr. Chris Harrison).

Scratch Input Project:
The project tracks the unique scratches done by a user on a rough surface and converts that to unique commands to control applications.

Tracking - The tracking of scratches is done through peizoelectric crystal.
Processing - Amplifier, Monostable multivibrator to oonvert analog signals to digital signals.
Microcontroller - To compare and evaluate the scratch with the pre-existing scratch conditions. If a match is found, a corresponding command is identified and executed.

C Code:
It analyzes the digital signal and tries to find the match for one of the 4 possible commands.
