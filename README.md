# Real-Time-Clockdriven
***
Development of a clock-driven scheduling library using priority, affinity, multithreading and deadline control. The application works on a single processor core (affinity 1).

## Application launch
the application-ok.cpp schedule file is an optimal situation where there should be no deadline miss. Depending on the machine on which it is launched, it is possible from time to time to have some deadline misses, especially in laptops without mains power. This is due to power saving settings that lower the CPU clock. A trick to mitigate this problem is to launch a terminal with the following command:<br>

------------------------------------
yes > /dev/null

------------------------------------
This will create an infinite loop that will force the processor to raise its clock.

