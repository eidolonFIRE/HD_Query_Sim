all : HD_scheduleTester.exe

HD_scheduleTester.exe : HD_schedule.cpp
	g++ HD_schedule.cpp -o HD_scheduleTester.exe -static


clean :
	rm *.o
	rm *.exe