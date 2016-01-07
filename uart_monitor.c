#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>      ///////headrer for UART ***connection from your user space application and the Kermit console 
#include <time.h>   ///// header for software timer
#include <sys/sysinfo.h> /////returns information on overall system statistics
#include "../../user-modules/timer_driver/timer_ioctl.h"

#define BUFFER_SIZE 640*480*4
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define CHAR_MIN 0
#define CHAR_MAX 255

//**  ioctl calls to retrieve information from the timer driver 
struct timer_ioctl_data data; // data structure to access timer_ioctl_data members
int fd; // file descriptor for timer driver
int i;
time_t currentTime;
time_t LastfiveSeconds = 0;
float typeNumberCounter = 0;
int AVE_typedNumberCounter=0;
int character_number[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}; 
	
//function to read the timer value
__u32 read_timer()
{
	data.offset = TIMER_REG;
	ioctl(fd, TIMER_READ_REG, &data);
	return data.data;
}

//signal handler function
void sigint_handler(int signum)
{
	printf("Received Signal, signum=%d (%s)\n", signum, strsignal(signum));

	if (fd)
	{
		// Turn off timer and reset device
		data.offset = CONTROL_REG;
		data.data = 0x0;
		ioctl(fd, TIMER_WRITE_REG, &data);
		
		printf("Final timer value:%f\n", (float)read_timer()/100000000); /////////////////////////final time
		
		// Close device file
		close(fd);
	}

	exit(EXIT_SUCCESS);
}

struct stats
{
	int readCharCnt;
	int writeCharCnt;
	int charNumberCnt;
	int charUpperCnt;
	int charLowerCnt;
	int charSpaceCnt;
	int charWordCnt;
	float charPerSec;
	float avgReadTime;
	float avgWriteTime;
	int upTimeSecs;
	float upTimeMins;
};

struct image
{
	int *mem_loc;
	char *rgb;
	int *argb_loc;
	char *argb;
	int image_fd;
	struct stat sb;
	int height;
	int width;
};

struct rect
{
	int xpos;
	int ypos;
	int height;
	int width;
};

struct color
{
	int rcolor;
	int gcolor;
	int bcolor;
	int acolor;
};

struct circle
{
	int xpos;
	int ypos;
	int radius;
};

void drawRect(int *displayBuffer, struct rect *r, struct color *c)
{
	int xpos, ypos;
	char agbr[4];
	int *agbr_full;
	agbr_full = (int *)agbr;

	// Invalid variable displayBuffer
	if (!displayBuffer)
		return;

	// Invalid struct rect
	if (!r)
		return;

	// Invalid struct color
	if (!c)
		return;

	for (ypos = r->ypos; ypos < r->height + r->ypos; ypos++)
	{
		for (xpos = r->xpos; xpos < r->width + r->xpos; xpos++)
		{
			agbr[0] = (c->rcolor) & 0xFF;
			agbr[1] = (c->gcolor) & 0xFF;
			agbr[2] = (c->bcolor) & 0xFF;
			agbr[3] = (c->acolor) & 0xFF;
			if ((xpos >= 0 && xpos < SCREEN_WIDTH) && (ypos >=0 && ypos < SCREEN_HEIGHT))
			{
				displayBuffer[xpos+(640*ypos)] = *agbr_full;
			}
		}
	}

	return;
}

void drawRectEmpty(int *displayBuffer, struct rect *r, struct color *c)
{
	int xpos, ypos;
	char agbr[4];
	int *agbr_full;
	agbr_full = (int *)agbr;

	// Invalid variable displayBuffer
	if (!displayBuffer)
		return;

	// Invalid struct rect
	if (!r)
		return;

	// Invalid struct color
	if (!c)
		return;

	for (ypos = r->ypos; ypos < r->height + r->ypos; ypos++)
	{
		for (xpos = r->xpos; xpos < r->width + r->ypos; xpos++)
		{
			if (xpos == r->xpos || ypos == r->ypos || xpos == r->width + r->ypos - 1 || ypos == r->height + r->ypos - 1)
			{
				agbr[0] = (c->rcolor) & 0xFF;
				agbr[1] = (c->gcolor) & 0xFF;
				agbr[2] = (c->bcolor) & 0xFF;
				agbr[3] = (c->acolor) & 0xFF;
				if ((xpos >= 0 && xpos < SCREEN_WIDTH) && (ypos >=0 && ypos < SCREEN_HEIGHT))
				{
					displayBuffer[xpos+(640*ypos)] = *agbr_full;
				}
			}
		}
	}

	return;
}

void drawCircle(int *displayBuffer, struct circle *cir, struct color *c)
{
	int xpos, ypos;
	char agbr[4];
	int *agbr_full;
	agbr_full = (int *)agbr;

	// Invalid variable displayBuffer
	if (!displayBuffer)
		return;

	// Invalid struct circle
	if (!cir)
		return;

	// Invalid struct color
	if (!c)
		return;

	for (ypos = cir->ypos-cir->radius; ypos < cir->ypos+cir->radius; ypos++)
	{
		int ciry;
		if (ypos < cir->ypos)
		{
			ciry = cir->ypos - ypos;
		}
		else
		{
			ciry = ypos - cir->ypos;
		}
		for (xpos = cir->xpos-cir->radius; xpos < cir->xpos+cir->radius; xpos++)
		{
			int cirx;
			if (xpos < cir->xpos)
			{
				cirx = cir->xpos - xpos;
			}
			else
			{
				cirx = xpos - cir->xpos;
			}
			if (cirx*cirx <= cir->radius*cir->radius - ciry*ciry)
			{
				agbr[0] = (c->rcolor) & 0xFF;
				agbr[1] = (c->gcolor) & 0xFF;
				agbr[2] = (c->bcolor) & 0xFF;
				agbr[3] = (c->acolor) & 0xFF;
				if ((xpos >= 0 && xpos < SCREEN_WIDTH) && (ypos >=0 && ypos < SCREEN_HEIGHT))
				{
					displayBuffer[xpos+(640*ypos)] = *agbr_full;
				}
			}
		}
	}

	return;
}

void drawImage(struct image *img, int *displayBuffer, struct rect *r)
{
	char agbr[4];
	int *agbr_full;
	agbr_full = (int *)agbr;
	int xpos, ypos;

	// Invalid struct image
	if (!img)
		return;

	// Invalid variable displayBuffer
	if (!displayBuffer)
		return;

	// Invalid struct rect
	if (!r)
		return;

	for (ypos = r->ypos; ypos < r->height + r->ypos; ypos++)
	{
		for (xpos = r->xpos; xpos < r->width + r->xpos; xpos++)
		{
			int imgXpos = (xpos - r->xpos)%img->width;
			int imgYpos = (ypos - r->ypos)%img->height;
			int imgPos = (imgXpos + imgYpos*img->width) * 4;
			agbr[0] = img->argb[imgPos+0];
			agbr[1] = img->argb[imgPos+1];
			agbr[2] = img->argb[imgPos+2];
			agbr[3] = img->argb[imgPos+3];
			if ((xpos >= 0 && xpos < SCREEN_WIDTH) && (ypos >=0 && ypos < SCREEN_HEIGHT))
			{
				displayBuffer[xpos+(640*ypos)] = *agbr_full;
			}
		}
	}

	return;
}

void drawSubImage(struct image *img, int *displayBuffer, struct rect *rpos, struct rect *rchar)
{
	char agbr[4];
	int *agbr_full;
	agbr_full = (int *)agbr;
	int xpos, ypos;

	// Invalid struct image
	if (!img)
		return;

	// Invalid variable displayBuffer
	if (!displayBuffer)
		return;

	// Invalid struct rect
	if (!rpos)
		return;

	// Invalid struct rect
	if (!rchar)
		return;

	for (ypos = rpos->ypos; ypos < rchar->height + rpos->ypos; ypos++)
	{
		for (xpos = rpos->xpos; xpos < rchar->width + rpos->xpos; xpos++)
		{
			float rtemp, btemp, gtemp;
			char curColor[4];
			int test;
			test = displayBuffer[xpos+(640*ypos)];
			curColor[0] = (test >> 16) & 0x0FF;
			curColor[1] = (test >>  8) & 0x0FF;
			curColor[2] = (test >>  1) & 0x0FF;

			int imgXpos = (rchar->xpos + xpos - rpos->xpos)%img->width;
			int imgYpos = (rchar->ypos + ypos - rpos->ypos)%img->height;
			int imgPos = (imgXpos + imgYpos*img->width) * 4;
                        rtemp = (float)img->argb[imgPos+0]*((float)img->argb[imgPos+3]/255)+(float)curColor[0]*(1-(float)img->argb[imgPos+3]/255);
                        gtemp = (float)img->argb[imgPos+1]*((float)img->argb[imgPos+3]/255)+(float)curColor[1]*(1-(float)img->argb[imgPos+3]/255);
                        btemp = (float)img->argb[imgPos+2]*((float)img->argb[imgPos+3]/255)+(float)curColor[2]*(1-(float)img->argb[imgPos+3]/255);
			agbr[0] = (int)(rtemp) & 0xFF;
			agbr[1] = (int)(gtemp) & 0xFF;
			agbr[2] = (int)(btemp) & 0xFF;
			agbr[3] = (00) & 0xFF;
			if ((xpos >= 0 && xpos < SCREEN_WIDTH) && (ypos >=0 && ypos < SCREEN_HEIGHT))
			{
				displayBuffer[xpos+(640*ypos)] = *agbr_full;
			}
		}
	}

	return;
}

void drawChar(char c, struct image *img, int *displayBuffer, struct rect *rpos)
{
	struct rect rchar;
	// finds a proper position of a subimage in ascii table image and drow them
	rchar.ypos = 23*((0x0F & (c >> 4))%16) + 1;
	rchar.xpos = 12*((0x0F & (c >> 0))%16) + 1;
	rchar.width = 11;
	rchar.height = 22;

	drawSubImage(img, displayBuffer, rpos, &rchar);

	return;
}

void convertImage(struct image *img)
{
	char agbr[4];
	int *agbr_full;
	agbr_full = (int *)agbr;
	int counter = 0;
	int pixel;

	int iTemp[img->height * img->width - 1];
	img->argb_loc = (int *)iTemp;

	for (pixel = 0; pixel < img->height*img->width; pixel++)
	{
		agbr[0] = img->rgb[counter++];
		agbr[1] = img->rgb[counter++];
		agbr[2] = img->rgb[counter++];
		agbr[3] = (00) & 0xFF;
		img->argb_loc[pixel] = *agbr_full;
	}

	printf("pixel:%d, counter:%d, size:%d\n",pixel, counter, img->sb.st_size);

	img->argb = (char *) img->argb_loc;

	return;
}

void convertImage2(struct image *img)
{
	char agbr[4];
	int *agbr_full;
	agbr_full = (int *)agbr;
	int counter = 0;
	int pixel;

	int iTemp[img->height * img->width - 1];
	img->argb_loc = (int *)iTemp;

	for (pixel = 0; pixel < img->height*img->width; pixel++)
	{
		agbr[0] = img->rgb[counter++];
		agbr[1] = img->rgb[counter++];
		agbr[2] = img->rgb[counter++];
		agbr[3] = img->rgb[counter++];
		img->argb_loc[pixel] = *agbr_full;
	}

	printf("pixel:%d, counter:%d, size:%d\n",pixel, counter, img->sb.st_size);

	img->argb = (char *) img->argb_loc;

	return;
}

void convertNumberToString(int number, char *numberChar)
{
	//to parse each digit of integer and store in array
	int intTemp;
	intTemp = (int)number/100000;
	numberChar[5] = (char)(intTemp+48);
	number -= intTemp*100000;
	intTemp = (int)number/10000;
	numberChar[4] = (char)(intTemp+48);
	number -= intTemp*10000;
	intTemp = (int)number/1000;
	numberChar[3] = (char)(intTemp+48);
	number -= intTemp*1000;
	intTemp = (int)number/100;
	numberChar[2] = (char)(intTemp+48);
	number -= intTemp*100;
	intTemp = (int)number/10;
	numberChar[1] = (char)(intTemp+48);
	number -= intTemp*10;
	intTemp = (int)number/1;
	numberChar[0] = (char)(intTemp+48);
	number -= intTemp;

	//printf("Test:%c%c%c%c%c%c\n",numberChar[5],numberChar[4],numberChar[3],numberChar[2],numberChar[1],numberChar[0]);

	return;
}

void convertDecimalToString(int number, char *numberChar)
{
	int intTemp;
	intTemp = (int)number/100000000;
	numberChar[9] = (char)(intTemp+48);
	number -= intTemp*100000000;
	intTemp = (int)number/10000000;
	numberChar[8] = (char)(intTemp+48);
	number -= intTemp*10000000;
	intTemp = (int)number/1000000;
	numberChar[7] = (char)(intTemp+48);
	number -= intTemp*1000000;
	numberChar[6] = (char)(46);
	intTemp = (int)number/100000;
	numberChar[5] = (char)(intTemp+48);
	number -= intTemp*100000;
	intTemp = (int)number/10000;
	numberChar[4] = (char)(intTemp+48);
	number -= intTemp*10000;
	intTemp = (int)number/1000;
	numberChar[3] = (char)(intTemp+48);
	number -= intTemp*1000;
	intTemp = (int)number/100;
	numberChar[2] = (char)(intTemp+48);
	number -= intTemp*100;
	intTemp = (int)number/10;
	numberChar[1] = (char)(intTemp+48);
	number -= intTemp*10;
	intTemp = (int)number/1;
	numberChar[0] = (char)(intTemp+48);
	number -= intTemp;

	return;
}

void screenSetup(struct image *ascii, int *buffer, struct rect *rChar)
{
	int titleLength = 17;
	char string[titleLength];
	char string2[2];
	int count;

	rChar->xpos = SCREEN_WIDTH/2 + 10;
	rChar->ypos = 10;
	rChar->height = 100;
	rChar->width = 100;
	sprintf(string, "Statistics       ");
	for (count = 0; count < titleLength; count++)
	{
		drawChar(string[count], ascii, buffer, rChar);
		rChar->xpos += 11;
	}
	rChar->xpos = SCREEN_WIDTH/2 + 10;
	rChar->ypos += 22;
	sprintf(string, "Number Count:    ");
	for (count = 0; count < titleLength; count++)
	{
		drawChar(string[count], ascii, buffer, rChar);
		rChar->xpos += 11;
	}
	rChar->xpos = SCREEN_WIDTH/2 + 10;
	rChar->ypos += 22;
	sprintf(string, "Upper Count:     ");
	for (count = 0; count < titleLength; count++)
	{
		drawChar(string[count], ascii, buffer, rChar);
		rChar->xpos += 11;
	}
	rChar->xpos = SCREEN_WIDTH/2 + 10;
	rChar->ypos += 22;
	sprintf(string, "Lower Count:     ");
	for (count = 0; count < titleLength; count++)
	{
		drawChar(string[count], ascii, buffer, rChar);
		rChar->xpos += 11;
	}
	rChar->xpos = SCREEN_WIDTH/2 + 10;
	rChar->ypos += 22;
	sprintf(string, "Chars/sec:       ");
	for (count = 0; count < titleLength; count++)
	{
		drawChar(string[count], ascii, buffer, rChar);
		rChar->xpos += 11;
	}
	rChar->xpos = SCREEN_WIDTH/2 + 10;
	rChar->ypos += 22;
	sprintf(string, "Avg Read Time:   ");
	for (count = 0; count < titleLength; count++)
	{
		drawChar(string[count], ascii, buffer, rChar);
		rChar->xpos += 11;
	}
	rChar->xpos = SCREEN_WIDTH/2 + 10;
	rChar->ypos += 22;
	sprintf(string, "Up Time (secs):  ");
	for (count = 0; count < titleLength; count++)
	{
		drawChar(string[count], ascii, buffer, rChar);
		rChar->xpos += 11;
	}
	rChar->xpos = SCREEN_WIDTH/2 + 10;
	rChar->ypos += 22;
	sprintf(string, "Up Time (mins):  ");
	for (count = 0; count < titleLength; count++)
	{
		drawChar(string[count], ascii, buffer, rChar);
		rChar->xpos += 11;
	}

	rChar->xpos = 450;
	rChar->ypos = 410;
	rChar->height = 100;
	rChar->width = 100;
	sprintf(string2, "Reads ");
	for (count = 0; count < 6; count++)
	{
		drawChar(string2[count], ascii, buffer, rChar);
		rChar->xpos += 11;
	}

	rChar->xpos = 530;
	rChar->ypos = 410;
	rChar->height = 100;
	rChar->width = 100;
	sprintf(string2, "Writes");
	for (count = 0; count < 6; count++)
	{
		drawChar(string2[count], ascii, buffer, rChar);
		rChar->xpos += 11;
	}

	return;
}

//////////////////////////////////////////////  MAIN  ///////////////////////////////////////////////////
int main(void)
{
	//Register handler for SIGINT, sent when pressing CTRL+C at terminal
	signal(SIGINT, &sigint_handler);	 
	 
	//open the timer driver, check to see if it opens correctly
	if (!(fd = open("/dev/timer_driver", O_RDWR)))
	{
	 	 perror("open");
	 	 exit(EXIT_FAILURE);
	 }

	/////////////////////////////// Read value from timer and print local time /////////////////////////////////

	printf("*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*\n");
	printf("Initial HW Timer Value = %f\n", (float)read_timer()/100000000); //////////////////////////////////////// INITIAL HW TIME
	 
	time_t init_t; //start
	time(&init_t);
	 
	printf("Initial SW Timer Value = %f\n", init_t); /////////////////////////////////INITIAL SW TIME

	// Set control bits to enable timer, count up
	data.offset = CONTROL_REG;
	data.data = ENT0; //enables the timer
	ioctl(fd, TIMER_WRITE_REG, &data);

	char c; //to print characters
	char p_c; //previous character

	int serial_fd; 
	struct termios tio; 

	//variables for software clock
	time_t start_t; //start
	time_t end_t;	//finish
	float time_diff; //difference
	float total_t = 0; //total time
	 
	float console_time = 0;
	float timeInmins = 0;
	float wordsPermins =0;

	//variables for Hardware clock
	float HWS_t;    //start
	float HWE_t;	//finish
	float HWDiff_t;	//difference
	float HWTot_t = 0;		//total time

	int fd;
	int *buffer;
	struct rect rWriteArea;
	struct rect rScreen;
	struct rect rChar;
	struct circle cir;
	struct image ascii;
	int intChar;
	char chr;
	struct stats st;
	struct color cDflt;
	struct rect gRead;
	struct rect gWrite;
	struct rect rBox;
	struct color cRead;
	struct color cWrite;
	struct color cBox;

	st.readCharCnt = 0;
	st.writeCharCnt = 0;
	st.charNumberCnt = 0;
	st.charUpperCnt = 0;
	st.charLowerCnt = 0;
	st.charPerSec = 0;
	cDflt.rcolor = 100;
	cDflt.gcolor = 100;
	cDflt.bcolor = 100;
	cDflt.acolor = 0;

	fd = open("/dev/vga_driver",O_RDWR);
	buffer = (int*)mmap(NULL, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	// ASCII Image Setup
	ascii.image_fd = open("ASCII_192x368.raw",O_RDONLY);
	fstat (ascii.image_fd, &ascii.sb);
	ascii.mem_loc = (int*)mmap(NULL, ascii.sb.st_size, PROT_READ, MAP_SHARED, ascii.image_fd, 0);
	ascii.rgb = (char *)ascii.mem_loc;
	ascii.height = 368;
	ascii.width = 192;
	convertImage2(&ascii);

	// Clear Screen
	rScreen.xpos = 0;
	rScreen.ypos = 0;
	rScreen.height = SCREEN_HEIGHT;
	rScreen.width = SCREEN_WIDTH;
	drawRect(buffer, &rScreen, &cDflt);

	sleep(1);	

	//Setup Write Area box
	rBox.xpos = 0;
	rBox.ypos = 0;
	rBox.height = SCREEN_HEIGHT;
	rBox.width = SCREEN_WIDTH/2;
	cBox.rcolor = 0;
	cBox.gcolor = 0;
	cBox.bcolor = 0;
	cBox.acolor = 0;
	drawRectEmpty(buffer, &rBox, &cBox);


	rWriteArea.xpos = 1;
	rWriteArea.ypos = 1;
	rWriteArea.height = SCREEN_HEIGHT - 1;
	rWriteArea.width = SCREEN_WIDTH/2 - 1;
	screenSetup(&ascii, buffer, &rChar);

	///////////////////////////////   UART (send/recieved chars)   ////////////////////////////////////////
	//*** connection from your user space application and the Kermit console 
	memset(&tio, 0, sizeof(tio));
	if(!memset(&tio, 0, sizeof(tio)))
	{
	 	 perror("Memory Set error");
	 	 exit(EXIT_FAILURE);
	}

	//open the serial port, check to see whether it opens correctly
	serial_fd = open("/dev/ttyPS0",O_RDWR);
	if(serial_fd == -1)printf("Failed to open serial port... :( \n");
	 
	tcgetattr(serial_fd, &tio);
	cfsetospeed(&tio, B115200);
	cfsetispeed(&tio, B115200);
	tcsetattr(serial_fd, TCSANOW, &tio);

	while (1)
	{
		time(&currentTime);

		////////////////time from system start
		system("uptime");

		//////////////////////////////////////////////////////start software clock
		time(&start_t);
		read(serial_fd, &c, 1); //characters recieved from kermit
		time(&LastfiveSeconds);
		time(&end_t); //software clock

		////////////////////////////////  user created martix /////////////////////////////////////
		if (c >=48 && c <=57 )
		{	
			st.charNumberCnt++;
		}
		else if(c >=97 && c <=122)
		{
			st.charLowerCnt++;
		}	
		else if(c >=65 && c <=90)
		{
			st.charUpperCnt++;
		}
		//printf("Char_periovous   %c\n\n", p_c);	
		//printf("Char   %c\n\n",  c);
		else if(c == 32 && p_c !=32)
		{
			st.charSpaceCnt++;
			st.charWordCnt = st.charSpaceCnt + 1;	
		}

		//Start Character Display
		if (c == 10) //capturing enter key
		{
			//advance y positing if enter is captured
			rWriteArea.xpos = 0;
			rWriteArea.ypos += 22; 
			if (rWriteArea.ypos + 22 > rWriteArea.height) 
			{
				rWriteArea.ypos = 0; // advance to next line on a screen
				drawRect(buffer, &rWriteArea, &cDflt);
			}
		}

		else
		{
			st.writeCharCnt ++;
			drawChar(c, &ascii, buffer, &rWriteArea);
			if (rWriteArea.xpos + 22 > rWriteArea.width) // if line is filled then advance to next line
			{
				rWriteArea.xpos = 0;
				rWriteArea.ypos += 22;
				if (rWriteArea.ypos + 22 > rWriteArea.height)
				{
					rWriteArea.ypos = 0;
					drawRect(buffer, &rWriteArea, &cDflt);
				}
			}
			else
			{
				rWriteArea.xpos += 11; // advance to next space in a row
			}
		}

		p_c = c;
	
		//for(i =0; i <10 ; i++)
		//{
 		//	printf("characters####################################:%d\n",character_number[i]);
		//}

		time_diff = (float) end_t - (float)start_t;
		total_t = total_t + time_diff;
		st.readCharCnt ++;
		console_time = (float) end_t - (float) init_t; ////////////////// CONSOLE TIME 
		timeInmins = console_time/60;
		wordsPermins = st.charWordCnt / timeInmins; //////////////////////// WORDS/MIN


		// Bar Graph for Reads
		gRead.xpos = 450;
		gRead.ypos = 400 - (int)st.readCharCnt/10;
		gRead.width = 66;
		gRead.height = (int)st.readCharCnt/10;
		cRead.rcolor = 255;
		cRead.gcolor = 0;
		cRead.bcolor = 0;
		cRead.acolor = 0;
		drawRect(buffer, &gRead, &cRead);

		// Display number of Reads
		rChar.xpos = 450;
		rChar.ypos = 432;
		rChar.height = 22;
		rChar.width = 66;
		drawRect(buffer, &rChar, &cDflt);
		char nbrBuffer[5];
		char decBuffer[9];
		convertNumberToString(st.readCharCnt,nbrBuffer);
		int count;
		for (count = 5; count >= 0; count--)
		{
			char cTemp = nbrBuffer[count];
			drawChar(cTemp, &ascii, buffer, &rChar);
			rChar.xpos += 11;
		}

		// Bar Graph for Writes
		gWrite.xpos = 530;
		gWrite.ypos = 400 - (int)st.writeCharCnt/10;
		gWrite.width = 66;
		gWrite.height = (int)st.writeCharCnt/10;
		cWrite.rcolor = 0;
		cWrite.gcolor = 0;
		cWrite.bcolor = 255;
		cWrite.acolor = 0;
		drawRect(buffer, &gWrite, &cWrite);

		// Display number of Writes
		rChar.xpos = 530;
		rChar.ypos = 432;
		rChar.height = 22;
		rChar.width = 66;
		drawRect(buffer, &rChar, &cDflt);
		convertNumberToString(st.writeCharCnt,nbrBuffer);
		for (count = 5; count >= 0; count--)
		{
			char cTemp = nbrBuffer[count];
			drawChar(cTemp, &ascii, buffer, &rChar);
			rChar.xpos += 11;
		}

		// Display number count
		rChar.xpos = SCREEN_WIDTH - 11*7;
		rChar.ypos = 32;
		rChar.height = 22;
		rChar.width = 66;
		drawRect(buffer, &rChar, &cDflt);
		convertNumberToString(st.charNumberCnt,nbrBuffer);
		for (count = 5; count >= 0; count--)
		{
			char cTemp = nbrBuffer[count];
			drawChar(cTemp, &ascii, buffer, &rChar);
			rChar.xpos += 11;
		}

		// Display upper count
		rChar.xpos = SCREEN_WIDTH - 11*7;
		rChar.ypos += 22;
		rChar.height = 22;
		rChar.width = 66;
		drawRect(buffer, &rChar, &cDflt);
		convertNumberToString(st.charUpperCnt,nbrBuffer);
		for (count = 5; count >= 0; count--)
		{
			char cTemp = nbrBuffer[count];
			drawChar(cTemp, &ascii, buffer, &rChar);
			rChar.xpos += 11;
		}

		// Display lower count
		rChar.xpos = SCREEN_WIDTH - 11*7;
		rChar.ypos += 22;
		rChar.height = 22;
		rChar.width = 66;
		drawRect(buffer, &rChar, &cDflt);
		convertNumberToString(st.charLowerCnt,nbrBuffer);
		for (count = 5; count >= 0; count--)
		{
			char cTemp = nbrBuffer[count];
			drawChar(cTemp, &ascii, buffer, &rChar);
			rChar.xpos += 11;
		}

		// Display characters per second
		st.charPerSec = st.readCharCnt/total_t*1000000;
		rChar.xpos = SCREEN_WIDTH - 11*11;
		rChar.ypos += 22;
		rChar.height = 22;
		rChar.width = 110;
		drawRect(buffer, &rChar, &cDflt);
		convertDecimalToString(st.charPerSec,decBuffer);
		for (count = 9; count >= 0; count--)
		{
			char cTemp = decBuffer[count];
			drawChar(cTemp, &ascii, buffer, &rChar);
			rChar.xpos += 11;
		}

		// Display average read time
		st.avgReadTime = total_t/st.readCharCnt*1000000;
		rChar.xpos = SCREEN_WIDTH - 11*11;
		rChar.ypos += 22;
		rChar.height = 22;
		rChar.width = 110;
		drawRect(buffer, &rChar, &cDflt);
		convertDecimalToString(st.avgReadTime,decBuffer);
		for (count = 9; count >= 0; count--)
		{
			char cTemp = decBuffer[count];
			drawChar(cTemp, &ascii, buffer, &rChar);
			rChar.xpos += 11;
		}

		// Display up time seconds
		st.upTimeSecs = (int)console_time;
		rChar.xpos = SCREEN_WIDTH - 11*7;
		rChar.ypos += 22;
		rChar.height = 22;
		rChar.width = 66;
		drawRect(buffer, &rChar, &cDflt);
		convertNumberToString(st.upTimeSecs,nbrBuffer);
		for (count = 5; count >= 0; count--)
		{
			char cTemp = nbrBuffer[count];
			drawChar(cTemp, &ascii, buffer, &rChar);
			rChar.xpos += 11;
		}

		// Display up time seconds
		st.upTimeMins = timeInmins*1000000;
		rChar.xpos = SCREEN_WIDTH - 11*11;
		rChar.ypos += 22;
		rChar.height = 22;
		rChar.width = 110;
		drawRect(buffer, &rChar, &cDflt);
		convertDecimalToString(st.upTimeMins,decBuffer);
		for (count = 9; count >= 0; count--)
		{
			char cTemp = decBuffer[count];
			drawChar(cTemp, &ascii, buffer, &rChar);
			rChar.xpos += 11;
		}

		//////////////////////////////// AVG read time/////////////////////////////////////////////
		//printf("*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*\n");
		//printf("Char   %c\n\n",  c);
		//printf("number:%d\n",char_number);
		//printf("Lower case alphabet:%d\n",char_alpha_Low);
		//printf("Higher case alphabet:%d\n\n",char_alpha_High);
		//printf("number of character per second:%f\n",charCount/total_t);
		//printf("amount of time taken to read character:%f\n",time_diff);
		//printf("Average read time:%f\n",total_t/charCount); /// SW

		// Set control bits to enable timer, count up
		data.offset = CONTROL_REG;
		data.data = ENT0; //enables the timer
		ioctl(fd, TIMER_WRITE_REG, &data);

		// start HW timer
		HWS_t = (float)read_timer();
		//printf("Start HW timer=%f\n",HWS_t/100000000);
		write(serial_fd, &c, 1);  //character sent to kermit

		//Clear control bits to disable HW timer
		data.offset = CONTROL_REG;
		data.data = 0x0;
		ioctl(fd, TIMER_WRITE_REG, &data);

		//end HW timer, read the value
		HWE_t = (float)read_timer();
		//printf("end HW timer =%f\n",HWE_t/100000000);
		HWDiff_t = HWE_t - HWS_t;
		HWTot_t = HWTot_t + HWDiff_t;
		 
		//printf("time for kermit to write character:%f\n",(HWDiff_t)/100000000);
		//printf("Average write time:%f\n",(HWTot_t/charCount)/100000000); ///HW
		//printf("time from console start:%f\n",console_time);
		//printf("time in minutes:%f\n",timeInmins);
		//printf("number of words per minute:%f\n",wordsPermins);

		//////////////////////////////// number counter in last 5 sec////////////////////////////
		switch (c)
		{
		case '0':
			character_number[0]++;
			break;
		case '1':
			character_number[1]++;
			break;
		case '2':
			character_number[2]++;
			break;
		case '3':
			character_number[3]++;
			break;
		case '4':
			character_number[4]++;
			break;
		case '5':
			character_number[5]++;
			break;
		case '6':
			character_number[6]++;
			break;
		case '7':
			character_number[7]++;
			break;
		case '8':
			character_number[8]++;
			break;
		case '9':
			character_number[9]++;
			break;				
		}

		if((currentTime - LastfiveSeconds) >= 5.0)
		{
			typeNumberCounter = character_number[0] + character_number[1] + character_number[2] + character_number[3] + character_number[4] + character_number[5] + character_number[6] + character_number[7] + character_number[8] + character_number[9];

			 AVE_typedNumberCounter = typeNumberCounter / 5.0 ;
			 printf("+++number count for last 5 sec:%f\n",AVE_typedNumberCounter);

			 LastfiveSeconds = currentTime; 
		}
	}

	return 0;
}

