/*---------------------------------------------------------------------------------

	Simple console print demo which prints to both screens
	-- dovoto

---------------------------------------------------------------------------------*/
#include <nds.h>
#include <stdio.h>
#include <dswifi9.h>

#include <sys/types.h>
#include <netinet/in.h>

#include <sys/socket.h>
#include <netdb.h>


// creates structure to hold x and y positions from touch screen
touchPosition touch;

// allocated memory to initialize consoles to
PrintConsole topScreen;
PrintConsole bottomScreen;

//---------------------------------------------------------------------------------
Wifi_AccessPoint* findAP(void){
//---------------------------------------------------------------------------------

	int selected = 0;
	int i;
	int count = 0, displaytop = 0;

	static Wifi_AccessPoint ap;

	Wifi_ScanMode(); //this allows us to search for APs

	int pressed = 0;
	do {
		scanKeys();

		//find out how many APs there are in the area
		count = Wifi_GetNumAP();


		consoleClear();

		iprintf("%d APs detected\n\n", count);

		int displayend = displaytop + 10;
		if (displayend > count) displayend = count;

		//display the APs to the user
		for(i = displaytop; i < displayend; i++) {
			Wifi_AccessPoint ap;

			Wifi_GetAPData(i, &ap);

			// display the name of the AP
			iprintf("%s %.29s\n  Wep:%s Sig:%i\n",
				i == selected ? "*" : " ",
				ap.ssid,
				ap.flags & WFLAG_APDATA_WEP ? "Yes " : "No ",
				ap.rssi * 100 / 0xD0);

		}

		pressed = keysDown();
		//move the selection asterick
		if(pressed & KEY_UP) {
			selected--;
			if(selected < 0) {
				selected = 0;
			}
			if(selected<displaytop) displaytop = selected;
		}

		if(pressed & KEY_DOWN) {
			selected++;
			if(selected >= count) {
				selected = count - 1;
			}
			displaytop = selected - 9;
			if (displaytop<0) displaytop = 0;
		}
		swiWaitForVBlank();
	} while(!(pressed & KEY_A));

	//user has made a choice so grab the ap and return it
	Wifi_GetAPData(selected, &ap);

	return &ap;
}

void Wifi_init_and_connect(void) {
	int status = ASSOCSTATUS_DISCONNECTED;

	//consoleClear();
	//consoleSetWindow(NULL, 0,0,32,24);

	Wifi_AccessPoint* ap = findAP();

	//consoleClear();
	//consoleSetWindow(NULL, 0,0,32,10);

	iprintf("Connecting to %s\n", ap->ssid);

	//this tells the wifi lib to use dhcp for everything
	Wifi_SetIP(0,0,0,0,0);
	char wepkey[64];
	int wepmode = WEPMODE_NONE;
	if (ap->flags & WFLAG_APDATA_WEP) {
		iprintf("Enter Wep Key\n");
		while (wepmode == WEPMODE_NONE) {
			scanf("%s",wepkey);
			if (strlen(wepkey)==13) {
				wepmode = WEPMODE_128BIT;
			} else if (strlen(wepkey) == 5) {
				wepmode = WEPMODE_40BIT;
			} else {
				iprintf("Invalid key!\n");
			}
		}
		Wifi_ConnectAP(ap, wepmode, 0, (u8*)wepkey);
	} else {
		Wifi_ConnectAP(ap, WEPMODE_NONE, 0, 0);
	}
	consoleClear();
	while(status != ASSOCSTATUS_ASSOCIATED && status != ASSOCSTATUS_CANNOTCONNECT) {

		status = Wifi_AssocStatus();
		int len = strlen(ASSOCSTATUS_STRINGS[status]);
		iprintf("\x1b[0;0H\x1b[K");
		iprintf("\x1b[0;%dH%s", (32-len)/2,ASSOCSTATUS_STRINGS[status]);

		scanKeys();

		if(keysDown() & KEY_B) break;

		swiWaitForVBlank();
	}

	// char url[256];

	if(status == ASSOCSTATUS_ASSOCIATED) {
		u32 ip = Wifi_GetIP();
		iprintf("\nCONNECTED!!!\n");
		iprintf("\nip: [%lui.%lui.%lui.%lui]\n", (ip ) & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF);

	} else {
		iprintf("\nConnection failed!\n");
	}
}

static char senderIP[17];
int portnum = 55056;

// this casllback runs with every keypress to make sure the text you type is visible
void OnKeyPressed(int key) {
   if(key > 0)
      iprintf("%c", key);
}

void SelectIPandPort(){
	//consoleDemoInit(); 
	Keyboard *kbd = 	keyboardDemoInit();
	kbd->OnKeyPressed = OnKeyPressed;
	consoleSelect(&topScreen);
	char * defaultIP = "192.168.0.101";
	iprintf("Would you like to used the \ndefault IP of %s?\n\nPress A for using the default\nor B to set your own\n\n",defaultIP);

	int IP = 0;
	int Port = 0;
	int keys = 0;

	while(!IP) {
		scanKeys();
		keys = keysDown();
		if(keys & KEY_A) {
			// default IP setting
			snprintf(senderIP,sizeof(senderIP),defaultIP);
			IP = 1;
		} else if (keys & KEY_B) {
			scanf("%s",senderIP);
			iprintf("Is %s correct?\nPress A to confirm or B to enter again\n",senderIP);
			scanKeys();
			while (!(keys & KEY_B || keys & KEY_A)){
				scanKeys();
				keys = keysDown();
			}
			if (keys & KEY_A) {
				iprintf("Confirmed\n");
				IP = 1;
			}
		}
	}

	iprintf("\nWould you like to used the\ndefault Port of %i?\n\nPress A for using the default\nor B to set your own\n\n",portnum);

	while(!Port) {
		scanKeys();
		keys = keysDown();
		if(keys & KEY_A) {
			// default Port setting remains
			Port = 1;
		} else if (keys & KEY_B) {
			scanf("%i",&portnum);
			iprintf("Is %i correct?\nPress A to confirm or B to enter again\n",portnum);
			scanKeys();
			while (!(keys & KEY_B || keys & KEY_A)){
				scanKeys();
				keys = keysDown();
			}
			if (keys & KEY_A) {
				iprintf("Confirmed\n");
				Port = 1;
			}
		}
	}	



}

void setup(void) {


	Wifi_InitDefault(false);

	videoSetMode(MODE_0_2D);
	videoSetModeSub(MODE_0_2D);

	// assigns Bank A (128 kB) to the top screen
	vramSetBankA(VRAM_A_MAIN_BG);
	// assigns Bank C (128 kB) to the bottom screen
	vramSetBankC(VRAM_C_SUB_BG);

	// initializes consoles for top and bottom screens
	consoleInit(&topScreen, 3,BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
	consoleInit(&bottomScreen, 3,BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);



	Wifi_init_and_connect();
	consoleSelect(&topScreen);
	iprintf("\n\n\tNintendo DS Command Center \n\tRemote Control Test\n\n");
	
	SelectIPandPort();
	// Keyboard in IP selection messes up lower screen
	consoleInit(&bottomScreen, 3,BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);




}

int fd; // UDP socket value
struct sockaddr_in myaddr;
struct sockaddr_in servaddr;    /* server address */

int16_t touchpx = 0;
int16_t touchpy = 0;

int16_t matrix_x = 0;
int16_t matrix_y = 0;

static char packet[32];

void sendStatus(const char *message) {
	snprintf(packet,sizeof(packet),message);
	consoleSelect(&topScreen);
	iprintf("%s\n",packet);
	/* send a message to the server */
	if (sendto(fd, packet, strlen(packet), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		consoleSelect(&topScreen);
		iprintf("sendto failed\n");
		consoleSelect(&bottomScreen);
	}
}



//---------------------------------------------------------------------------------
int main(void) {
//---------------------------------------------------------------------------------

	setup();

	// initialize keypad interrupt
	// irqInit();
	// irqEnable(IRQ_KEYS);
	// irqClear(IRQ_KEYS);
	// irqSet(IRQ_KEYS,keyInterruptRoutine);


	// Setup UDP socket to send data

	// setup UDP
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("\ncannot create socket\n");
		return 1;
	} else {
		iprintf("\nUDP socket established\n");
	}


	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(portnum);

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		iprintf("\nbind failed\n");
		return 1;
	}


	/* fill in the server's address and data */
	memset((char*)&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(portnum);
	// servaddr.sin_addr.s_addr = htonl(0xC0A800FA);
	inet_aton(senderIP,&servaddr.sin_addr);


	consoleSelect(&bottomScreen);


// Run Loop

	while(1) {
		scanKeys();
		touchRead(&touch);
		
		consoleSelect(&bottomScreen);
		iprintf("\x1b[10;0HTouch x = %04i, %04i\n", touch.rawx, touch.px);
		iprintf("Touch y = %04i, %04i\n", touch.rawy, touch.py);

		static int keys;
		keys  = keysDown();
		if(keysHeld() & KEY_TOUCH) {

			if (touchpx != touch.px || touchpy != touch.py) {
				touchpx = touch.px;
				touchpy = touch.py;

				matrix_x = (touchpx+1)*7/256;
				matrix_y = (touchpy+1)*7/192;
				snprintf(packet,sizeof(packet),"X%.1dY%.1d",matrix_x,matrix_y);

				consoleSelect(&topScreen);
				iprintf("SENDING: %s\n",packet);
				consoleSelect(&bottomScreen);
				if (sendto(fd, packet, strlen(packet), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
					consoleSelect(&topScreen);
					iprintf("sendto failed\n");
					consoleSelect(&bottomScreen);
					return 0;
				}
			}

		}
		if(keys & KEY_A) {
			sendStatus("A Pressed");
		}
		if(keys & KEY_B) {
			sendStatus("B Pressed");
		}
		if(keys & KEY_X) {
			sendStatus("X Pressed");
		}
		if(keys & KEY_Y) {
			sendStatus("Y Pressed");
		}
		if(keys & KEY_UP) {
			sendStatus("UP Pressed");
		}
		if(keys & KEY_DOWN) {
			sendStatus("DOWN Pressed");
		}
		if(keys & KEY_LEFT) {
			sendStatus("LEFT Pressed");
		}
		if(keys & KEY_RIGHT) {
			sendStatus("RIGHT Pressed");
		}
		if(keys & KEY_START) {
			sendStatus("START Pressed");
		}
		if(keys & KEY_SELECT) {
			sendStatus("SELECT Pressed");
		}
		if(keys & KEY_R) {
			sendStatus("RIGHT SHOULDER Pressed");		}
		if(keys & KEY_L) {
			sendStatus("LEFT SHOULDER Pressed");
		}
		if(keys & KEY_LID) {
			sendStatus("LID closed");
		}
		swiWaitForVBlank();
	}

	return 0;
}
