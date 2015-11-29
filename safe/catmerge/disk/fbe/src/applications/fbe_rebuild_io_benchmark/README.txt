How to run the tool:
1. Copy the .exe alone onto the array and run the following command:
   <file.exe> -rg <rg object id> -lun <lun object id> -io <io profile name/all> -count <number of iterations>
   optional attributes are: -iof <io file name>; -t <thread num>
2. RG object id should be the object id where the rebuild is happening. 
   This is used to calculate the rate at which the checkpoint is progressing
3. LUN object id should be the object id to which you want to generate the IO to.
   This will be used in the rdgen command
4. IO profile name is the profile name in the ioprofile.py file. 
   But most of the times you can specify "all" and it picks all the rdgen commands specified in the python file.
   The RDGEN command string already specified in the python file is made of patterns for the number of threads you want to run and the type of rdgen you want to run
   For eg: "rdgen -object_id <lun object id> -package_id sep -perf <-constant -align -seq -priority 2 w 0 8 10>"
   In the above example: <-constant -align -seq -priority 2 w 0 8 10> is fetched from the IO profile file and is appended to the rdgen base command.
   Look at step 6 on how to write this file.
   Here, 8 in the number of threads and this can be either hardcoded in the IO Profile file or you can use the optional attribute "-t"
5. Count is the number of iterations you want on all the IO profiles. IOs increase in arithmetic progression over the cycles.
   For example: if count is 5, and threads are 8.
   cycle 1: 8 threads
   cycle 2: 16 threads
   cycle 3: 32 threads
   cycle 4: 48 threads and so on...
   Note the count applies to all the IO profiles or for all threads specified.
6. IOf is the IO profile file that you write.
   Example: 32_LUN_8K_random_w:w 0 32 10:Random write with 8K block size, 32 threads on lun objects
   Format: <name of the IO profile>:<rdgen you want to run>:<description of the IO>
   Please make sure that the names are distinct. Here 32 is the number of threads; Lun is because rdgen is run on LUN objects;
   8k random write is the IO we want to run. So the format is: <thread num>_<object type>_<type of IO>
   w 0 32 10 : This becomes a part of the rdgen command
   
In the end, what will the program have:
Say, you have created a IO profile file with the following content:
  32_LUN_8K_random_w:-constant -align -seq -priority 2 w 0 32 10:Random write with 8K block size, 32 threads on lun objects
  40_LUN_8K_random_r:-constant -align -seq -priority 2 r 0 40 10:Random read with 8K block size, 40 threads on lun objects
  %(thread)d_LUN_8K_random_w:-constant -align -seq -priority 2 w 0 %(thread)d 10:Random write with 8K block size, %(thread)d threads on lun objects
The third line is interesting. This is a thread pattern rdgen. 
  If you add this line and give thread num "-t" when running the exe like "-t 8 16 32"
  Now the numbers 8, 16 and 32 will substitute for "%(thread)d" in that line.
This is what the tool is capable of generating in the end for you:
  32_LUN_8K_random_w
    command: rdgen -object_id <lun object id> -package_id sep -perf -constant -align -seq -priority 2 w 0 32 10
	description: Random write with 8K block size, 32 threads on lun objects
  40_LUN_8K_random_r
    command: rdgen -object_id <lun object id> -package_id sep -perf -constant -align -seq -priority 2 r 0 40 10
	description: Random read with 8K block size, 40 threads on lun objects
  8_LUN_8K_random_w
    command: rdgen -object_id <lun object id> -package_id sep -perf -constant -align -seq -priority 2 w 0 8 10
	description: Random write with 8K block size, 8 threads on lun objects
  16_LUN_8K_random_w
    command: rdgen -object_id <lun object id> -package_id sep -perf -constant -align -seq -priority 2 w 0 16 10
	description: Random write with 8K block size, 16 threads on lun objects
Here you can see that the first two rdgen commands are from the first two lines of the IO profile file.
The last two are generated from the last line of the IO profile file. 8 and 16 are from the "-t" thread num you specified.
Thread num 32 isn't used because there is already a 32_LUN-8k_random_w in the list of IO profiles.
That is why the names should be unique in the format specified.
   
What the tool does:
1. The tool generates IO using the RDGEN command
2. It generates the .ktr and .cpt files with rbatraces and checkpoint info in them respectively
3. It also logs all the commands that are being run on the command line (for eg., start rba, stop rba, run rdgen, so on..)

How to read the .cpt file:
1. For each IO profile, and for iteration in that IO profile run (based on -count) a .cpt file will be generated.
2. The .cpt file shows the timestamp, checkpoint progress fo the rdgen object
3. The file also has the mbps calculation based on this checkpoint progress and timestamp. Note it is in blocks.