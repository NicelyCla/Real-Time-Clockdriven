# Real-Time-Clockdriven
***
Development of a clock-driven scheduling library using priority, affinity, multithreading and deadline control. The application works on a single processor core (affinity 1).

## Application launch
You need to compile the file with the make command. <br>
Launching <b><i>application-ok</i></b> will be show an optimal situation where there should be no deadline miss and the tasks will be optimally scheduled. Depending on the machine on which it is launched, it is possible from time to time to have some deadline misses, especially in laptops without mains power. This is due to power saving settings that lower the CPU clock. A trick to mitigate this problem is to launch a terminal with the following command:<br>

------------------------------------
yes > /dev/null

------------------------------------
This will create an infinite loop that will force the processor to raise its clock.<br><br>
Launching <b><i>application-err_p</i></b> will be shown a deadline miss situation in periodic tasks, in this case a task is running too busy waiting compromising the execution and causing deadline misses in some periodic task.<br><br>

