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
		iprintf("\nip: [%i.%i.%i.%i]\n", (ip ) & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF);

	} else {
		iprintf("\nConnection failed!\n");
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


	consoleSelect(&topScreen);
	iprintf("\n\n\tNintendo DS Command Center Wifi Test\n");
	iprintf("\tTest by James Grantham\n");
	iprintf("\twww.mrgrantham.com");
	Wifi_init_and_connect();






}

int fd; // UDP socket value
int16_t portnum = 55056;
struct sockaddr_in myaddr;
struct sockaddr_in servaddr;    /* server address */
char *my_message = "this is a test message";

int16_t touchpx = 0;
int16_t touchpy = 0;

int16_t matrix_x = 0;
int16_t matrix_y = 0;

void keyInterruptRoutine() {
	iprintf("interrupted\n");

	/* send a message to the server */
	if (sendto(fd, my_message, strlen(my_message), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		consoleSelect(&topScreen);
		iprintf("sendto failed\n");
		consoleSelect(&bottomScreen);
		return 0;
	}
	// irqClear(IRQ_KEYS);
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
		return;
	} else {
		iprintf("\nUDP socket established\n");
	}


	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(portnum);

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		iprintf("\nbind failed\n");
		return;
	}


	/* fill in the server's address and data */
	memset((char*)&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(portnum);
	// servaddr.sin_addr.s_addr = htonl(0xC0A800FA);
	inet_aton("192.168.0.250",&servaddr.sin_addr);


	consoleSelect(&bottomScreen);

	static char packet[32];

// Run Loop

	while(1) {
		scanKeys();
		touchRead(&touch);

		iprintf("\x1b[10;0HTouch x = %04i, %04i\n", touch.rawx, touch.px);
		iprintf("Touch y = %04i, %04i\n", touch.rawy, touch.py);


		if(keysHeld() & KEY_TOUCH) {

			if (touchpx != touch.px || touchpy != touch.py) {
				touchpx = touch.px;
				touchpy = touch.py;

				matrix_x = (touchpx+1)*7/256;
				matrix_y = (touchpy+1)*7/192;
				snprintf(packet,sizeof(packet),"X%0.1dY%0.1d",matrix_x,matrix_y);

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
		if(keysHeld() & KEY_A) {
			keyInterruptRoutine();
		}
		swiWaitForVBlank();
	}

	return 0;
}
