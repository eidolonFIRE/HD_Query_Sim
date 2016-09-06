//=============================================================================\
//     Caleb Johnson  ::  HD scheduler  :: 2/21/2013                           |
//                                                                             |
//       Info:  Simple simulation for measuring hard disk querying             |
//              strategies and the timing involved.                            |
//                                                                             |
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -|
//    Maintinance Log:                                                         |
//    - 2/26/2013 - Caleb Johnson:                                             |
//               Document created. Initial setup.                              |
//                                                                             |
//    - 2/28/2013 - Caleb Johnson:                                             |
//               Revamped basically everything. V2                             |
//                                                                             |
//    - 3/01/2013 - Caleb Johnson:                                             |
//               Added command line arguments for changing params and options. |
//                                                                             |
//    - 9/06/2016 - Caleb Johnson:                                             |
//               Cleaned up formating  for uploda to github.                   |
//                                                                             |
//=============================================================================/

#define BLACK   0
#define BLUE    1
#define GREEN   2
#define CYAN    3
#define RED     4
#define PURPLE  5
#define BROWN   6
#define WHITE   7
#define GRAY    8
#define LBLUE   9
#define LGREEN  10
#define LCYAN   11
#define LRED    12
#define LPURPLE 13
#define YELLOW  14
#define BWHITE  15

// standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <vector>

// libs for drawing
#include <windows.h>

using namespace std;


//=== global vars
int sectormax = 1024;         // highest sector in memory... and lowest is 0
int buffersize = 8;           // how large the ongoing list of querries is

int X = 2, Y = 1;             // values used for calculating cost. 
                              // X is constant, Y is multiplied by sector delta

int totalruns = 1000;         // total number of runs to go through
bool stepmode = false;        // when true: program will step through each query
bool showdisplay = true;      // hide all visible output
bool savehistogram = false;   // write histogram to file?

//=== query buffers for each algorithm
struct query
{
	int sector;
	int wait;
};
vector<query> sectors_FCFS;
vector<query> sectors_CSL;
vector<query> sectors_SL;
vector<query> sectors_SSTF;

//=== current positions of the heads
int head_FCFS = 0;
int head_CSL = 0;
int head_SL = 0;
int head_SSTF = 0;

//=== Control vars for: SL
int SL_dir = 1;                // direction of the scan

int CSL_loops = 0;

//=== overall stats
int avgmw = 0;                 // the average mean wait of all algorithms
int mw_min, mw_max;            // mean wait time ranges
int histogram[1024];

//=== mean wait time for each algorithm
int mw_FCFS = 0;
int mw_CSL = 0;
int mw_SL = 0;
int mw_SSTF = 0;

//=== total wait for each algorithm
int sum_FCFS = 0;
int sum_CSL = 0;
int sum_SL = 0;
int sum_SSTF = 0;




//==============================================================================|
//      these are used to set text color and move text cursor                   |
//                                                                              |
//------------------------------------------------------------------------------|

void gotoXY(int x, int y)
{
	HANDLE screen_buffer_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD coord;
	coord.X = x;
	coord.Y = y;
	SetConsoleCursorPosition(screen_buffer_handle, coord);
}

void setForeColor(int c)
{
	static HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	WORD wAttrib = 0;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hConsole,&csbi);
	wAttrib = csbi.wAttributes;
	wAttrib &= ~(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
	if (c & 1) wAttrib |= FOREGROUND_BLUE;
	if (c & 2) wAttrib |= FOREGROUND_GREEN;
	if (c & 4) wAttrib |= FOREGROUND_RED;
	if (c & 8) wAttrib |= FOREGROUND_INTENSITY;
	SetConsoleTextAttribute(hConsole, wAttrib);
}




//==============================================================================|
//      General Drawing functions.                                              |
//                                                                              |
//------------------------------------------------------------------------------|

void drawbanner()
{
	// Banner
	system("cls");

	setForeColor(GRAY);
	printf("\n     \xC9");
	for (int i = 0; i < 68; i++) printf("\xCD");
	printf("\xBB\n     \xBA");
	setForeColor(CYAN);

	// message
	printf("   Thank you for choosing Caleb's HDD strategy tester!    v2.1    ");

	setForeColor(GRAY);
	printf("  \xBA\n     \xC8");
	for (int i = 0; i < 68; i++) printf("\xCD");
	printf("\xBC");
	setForeColor(WHITE);
}



void printlist(vector<query> list, int x, int y)
{
	//=== print header
	gotoXY(x, y);
	printf("\xDA\xC4\xC4\xC4\xC4\xC4\xC2\xC4\xC4\xC4\xC4\xC4\xC4\xBF");
	
	//=== print entries
	for (int t = 0; t < buffersize; t++)
	{
		gotoXY(x, y + t + 1);
		setForeColor(WHITE);
		if (t < list.size())
		{
			if (list[t].wait < (avgmw + mw_max) / 2
			 && list[t].wait > (avgmw + mw_min) / 2) setForeColor(LGREEN);
			if (list[t].wait > (avgmw + mw_max) / 2) setForeColor(YELLOW);
			if (list[t].wait > mw_max) setForeColor(LRED);
			printf("\xB3%5d\xB3%6d\xB3", list[t].sector, list[t].wait);
		}
		else
		printf("\xB3     \xB3      \xB3");
	}

	//=== print footer
	gotoXY(x, y + buffersize + 1);
	setForeColor(WHITE);
	printf("\xC0\xC4\xC4\xC4\xC4\xC4\xC1\xC4\xC4\xC4\xC4\xC4\xC4\xD9");
}


void drawgraph(int value, int x, int y, int l)
{
	gotoXY(x, y);
	printf("\xC3");                             // start
	for (int x = 0; x < l; x++)
	{
		if (value > mw_min + x * (mw_max - mw_min) / l) printf("\xDB");    // fill
		else printf("\xC4");                    // open
	}
	printf("\xB4");                             // end
}


//==============================================================================|
//    Search Algorithums.                                                       |
//                                                                              |
//------------------------------------------------------------------------------|

//=== First Come First Serve ===
void FCFS()
{
	//=== Always use the query at the top of the buffer
	int cost = X + Y * abs(sectors_FCFS[0].sector - head_FCFS);    // calculate the cost time
	
	for (int t = 1; t < sectors_FCFS.size(); t++) 
		sectors_FCFS[t].wait += cost;                    // Add wait time to all entries in buffer
	sum_FCFS += sectors_FCFS[0].wait;                    // add wait time to stats
	head_FCFS = sectors_FCFS[0].sector;                  // move the head
	sectors_FCFS.erase(sectors_FCFS.begin());            // clear entry from buffer
}

//=== circular scan loop ===
void CSL()
{
	int cost = 0;

	//=== scan buffer for next query
	int nextquery = sectormax + 1;
	int nextqueryindex = -1;

	while (nextqueryindex == -1)
	{
		for (int search = 0; search < sectors_CSL.size(); search++)
		{
			if (sectors_CSL[search].sector >= head_CSL 
			 && sectors_CSL[search].sector <= nextquery)
			{
				nextquery = sectors_CSL[search].sector;
				nextqueryindex = search;
			}
		}

		//=== if it didn't find anything, loop around
		if (nextqueryindex == -1)
		{
			//--- add cost to loop head around
			cost += X + Y * abs(1024 - head_CSL);
			head_CSL = 0;
			CSL_loops++;                        // add a loop to stats
		}
	}

	//=== use found entry and go from there as normal...
	cost += X + Y * abs(sectors_CSL[nextqueryindex].sector - head_CSL);    // calculate the cost time

	for (int t = 0; t < sectors_CSL.size(); t++) 
		if (t != nextqueryindex) sectors_CSL[t].wait += cost;        // Add wait time to all entries in buffer
	sum_CSL += sectors_CSL[nextqueryindex].wait;                     // add wait time to stats
	head_CSL = sectors_CSL[nextqueryindex].sector;                   // move the head
	sectors_CSL.erase(sectors_CSL.begin() + nextqueryindex);         // clear entry from buffer
}


//=== scan loop ===
void SL()
{
	//=== scan buffer for next query
	int nextquery;
	int nextqueryindex = -1;

	while (nextqueryindex == -1)
	{
		if (SL_dir == 1) nextquery = sectormax + 1;
		if (SL_dir == -1) nextquery = -1;
		for (int search = 0; search < sectors_SL.size(); search++)
		{
			if (SL_dir == 1 && sectors_SL[search].sector >= head_SL && sectors_SL[search].sector <= nextquery
			 || SL_dir ==-1 && sectors_SL[search].sector <= head_SL && sectors_SL[search].sector >= nextquery)
			{
				nextquery = sectors_SL[search].sector;
				nextqueryindex = search;
			}
		}
		//=== if it didn't find anything, toggle direction
		if (nextqueryindex == -1)
		{
			if (SL_dir == -1) SL_dir = 1;
			else SL_dir = -1;
		}
	}
	
	//=== use found entry and go from there as normal...
	int cost = X + Y * abs(sectors_SL[nextqueryindex].sector - head_SL);    // calculate the cost time

	for (int t = 0; t < sectors_SL.size(); t++) 
		if (t != nextqueryindex) sectors_SL[t].wait += cost;          // Add wait time to all entries in buffer
	sum_SL += sectors_SL[nextqueryindex].wait;                        // add wait time to stats
	head_SL = sectors_SL[nextqueryindex].sector;                      // move the head
	sectors_SL.erase(sectors_SL.begin() + nextqueryindex);            // clear entry from buffer
}

//=== Shortest seek time first (lowest cost first) ===
void SSTF()
{
	//=== scan buffer for next query
	int nextquery = 1024;
	int nextqueryindex = 0;

	for (int search = 0; search < sectors_SL.size(); search++)
	{
		if (abs(head_SSTF - sectors_SSTF[search].sector) < nextquery)
		{
			nextquery = abs(head_SSTF - sectors_SSTF[search].sector);
			nextqueryindex = search;
		}
	}
	//=== use found entry and go from there as normal...
	int cost = X + Y * abs(sectors_SSTF[nextqueryindex].sector - head_SSTF);    // calculate the cost time
	for (int t = 0; t < sectors_SSTF.size(); t++) 
		if (t != nextqueryindex) sectors_SSTF[t].wait += cost;            // Add wait time to all entries in buffer
	sum_SSTF += sectors_SSTF[nextqueryindex].wait;                        // add wait time to stats
	head_SSTF = sectors_SSTF[nextqueryindex].sector;                      // move the head
	sectors_SSTF.erase(sectors_SSTF.begin() + nextqueryindex);            // clear entry from buffer
}







//==============================================================================|
//      Main loop.                                |
//                                        |
//------------------------------------------------------------------------------|

int main(int argc, char** argv)
{
	int blink = 0;                // used for blinking text warnings
	
	
	//=== Read args!
	for (int i = 1; i < argc; i++) 
	{
		if (argv[i][0] == '-') 
		{
			switch (argv[i][1]) 
			{
				case 'h':    
					savehistogram = true;        // option: save txt file
					break;
				case 'r':
					totalruns = atoi(argv[i+1]);    // param: change number of runs
					break;
				case 'd':
					showdisplay = false;        // option: run silently
					break;
				case 's':
					stepmode = true;        // option: step through each query
					break;
				case 'b':
					buffersize = atoi(argv[i+1]);    // param: change buffer size
					break;
			}
		}
	}
	
	
	
	
	
	
	//--- clear histogram
	for (int t = 0; t < 1024; t++) histogram[t] = 0;
	
	//--- used for generating random numbers
	int a = rand()%(sectormax - 50), ar = 50, ac = 10;
	
	drawbanner();
	
	//=== Initialy Fill the buffers ===
	for (int runs = 0; runs < buffersize; runs++)
	{
		query temp;
		temp.wait = 0;
		temp.sector = rand()%sectormax;
		
		histogram[temp.sector]++;
		
		sectors_FCFS.push_back(temp);
		sectors_CSL.push_back(temp);
		sectors_SL.push_back(temp);
		sectors_SSTF.push_back(temp);
	}
	
	
	//------------------------------------
	//=== Run through LOTS of querries ===
	for (int runs = 1; runs <= totalruns; runs++)
	{
		//=== Run Querries
		FCFS();
		CSL();
		SL();
		SSTF();
		
		//=== re-populate buffers
		if (runs <= totalruns - buffersize)
		{
			query temp;
			temp.wait = 0;

			int mode = rand()%100;
			
			//--- 20% random outlyers
			if (mode >=  0 && mode < 20) temp.sector = rand()%sectormax;
			
			//--- 80% area bursts
			if (mode >= 20 && mode < 100)
			{
				temp.sector = rand()%ar + a;
				ac--;
				if (ac <=0)
				{
					ac = rand()%50 + 5;
					ar = rand()%30 + 1;
					a = rand()%(sectormax - ar);
				}
			}
			
			histogram[temp.sector]++;        // add data to historgram logs
			
			sectors_FCFS.push_back(temp);        // add an entry to the buffers
			sectors_CSL.push_back(temp);
			sectors_SL.push_back(temp);
			sectors_SSTF.push_back(temp);
		}
		
		//=== calculate stats
		mw_FCFS = sum_FCFS / runs;
		mw_CSL = sum_CSL / runs;
		mw_SL = sum_SL / runs;
		mw_SSTF = sum_SSTF / runs;
		avgmw = (mw_FCFS + mw_CSL + mw_SL + mw_SSTF) / 4;
		
		//=== find value ranges
		mw_min = avgmw;
		mw_max = avgmw;
		mw_min = mw_FCFS < mw_min ? mw_FCFS : mw_min;
		mw_max = mw_FCFS > mw_max ? mw_FCFS : mw_max;
		mw_min = mw_CSL < mw_min ? mw_CSL : mw_min;
		mw_max = mw_CSL > mw_max ? mw_CSL : mw_max;
		mw_min = mw_SL < mw_min ? mw_SL : mw_min;
		mw_max = mw_SL > mw_max ? mw_SL : mw_max;
		mw_min = mw_SSTF < mw_min ? mw_SSTF : mw_min;
		mw_max = mw_SSTF > mw_max ? mw_SSTF : mw_max;
		
		if (showdisplay || stepmode)
		{
			//=== Table Labels
			gotoXY(8, 7);
			printf("FCFS");
			
			gotoXY(28, 7);
			printf("CSL");
			
			gotoXY(48, 7);
			printf("SL");
			
			gotoXY(68, 7);
			printf("SSTF");
			
			//=== Draw Tables and graph
			printlist(sectors_FCFS, 3, 8);
			printlist(sectors_CSL, 23, 8);
			printlist(sectors_SL, 43, 8);
			printlist(sectors_SSTF, 63, 8);
			
			//=== more data
			gotoXY(24, 11 + buffersize);
			printf("header: %4d ", head_CSL);
			gotoXY(24, 12 + buffersize);
			printf(" loops: %4d ", CSL_loops);
			
			gotoXY(44, 11 + buffersize);
			printf("header: %4d ", head_SL);
			gotoXY(44, 12 + buffersize);
			printf("direction: %s", (SL_dir > 0 ? ">>" : "<<"));
			
			setForeColor(BLACK);
			int starve = 0;
			for (int s = 0; s < sectors_SSTF.size(); s++) if (sectors_SSTF[s].wait > mw_max) starve++;
			if (starve > 0)
			{
				setForeColor(RED);
				if (starve == 1) setForeColor(LRED);
				if (starve > 1)
				{
					blink++;
					if (blink > 10) blink = 0;
					if (blink > 5) setForeColor(LRED);
				}
			}
			gotoXY(65, 11 + buffersize);
			printf("\xAE STARVE \xAF");
			setForeColor(WHITE);
			
			//=== draw graph values
			gotoXY(5, 15 + buffersize);
			printf("\xDA %d ", mw_min);
			gotoXY(5, 16 + buffersize);
			printf("V");
			
			gotoXY(33, 15 + buffersize);
			printf("\xDA %d ", ((mw_min + mw_max) / 2));
			gotoXY(33, 16 + buffersize);
			printf("V");
			
			gotoXY(61, 15 + buffersize);
			printf("\xDA %d ", mw_max);
			gotoXY(61, 16 + buffersize);
			printf("V");
			
			drawgraph(mw_FCFS, 5, 17 + buffersize, 55);
			printf(" FCFS: %d ", mw_FCFS);
			drawgraph(mw_CSL, 5, 19 + buffersize, 55);
			printf("  CSL: %d ", mw_CSL);
			drawgraph(mw_SL, 5, 21 + buffersize, 55);
			printf("   SL: %d ", mw_SL);
			drawgraph(mw_SSTF, 5, 23 + buffersize, 55);
			printf(" SSTF: %d ", mw_SSTF);
		}
		
		if (stepmode)
		{
			printf("\n\n");
			system("pause");
		}
	}
	
	if (savehistogram)
	{
		//=== write histogram to file
		ofstream outfile("HD_sim_histogram.txt");
		for (int t = 0; t < 1024; t++) outfile << histogram[t] << endl;
		outfile.close();
	}
	
	printf("\n");
	system("pause");
	return 0;
}