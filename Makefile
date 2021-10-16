#make file - this is a comment section
 
clean:  #target name
		gcc testsim.c sharedFunctions.c -o testsim
		gcc runsim.c  sharedFunctions.c -o runsim