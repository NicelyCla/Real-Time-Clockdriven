# Real-Time-Clockdriven
***
Development of a clock-driven scheduling library using priority, affinity, multithreading and deadline control. The application works on a single processor core (affinity 1).<br>
<img src="https://github.com/NicelyCla/Real-Time-Clockdriven/blob/main/clock-driven-scheduling.png" alt="meta-learning">


## Application launch
You need to compile the file with the make command. <br>
for a correct priority setting all executables must be run with sudo. <br>

Launching <b>sudo ./application-ok</b> will be show an optimal situation where there should be no deadline miss and the tasks will be optimally scheduled. 

Launching <b>sudo ./application-err_p</b> will be shown a deadline miss situation in periodic tasks, in this case a task is running too busy waiting compromising the the execution in the frame of some periodic task and causing deadline misses.<br>

Launching <b>sudo ./application-err_a</b> will be shown a deadline miss situation in the aperiodic task, the aperiodic task is called every so often during the execution of a periodic task. If a release of the periodic is requested without terminating the previous execution, there is a deadline miss of the aperiodic task. <br>
by uncommenting the <b>#define SLACK_STEALING_ON</b> line, you can choose to use or not to use slack stealing. It serves to minimize the average response time of the aperiodic task.
<br>

Depending on the machine on which it is launched, it is possible from time to time to have some deadline misses also in application-ok, especially in laptops without mains power. This is due to power saving settings that lower the CPU clock. A trick to mitigate this problem is to launch a terminal with the following command:<br>

------------------------------------
$ yes > /dev/null

------------------------------------
This will create an infinite loop that will force the processor to raise its clock.<br>
