#include "support/gcc8_c_support.h"
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <proto/translator.h>
#include <devices/narrator.h>


#define OLD_SPEECH
#define EMULATOR
#define TRANSLATION_BUFFER_LENGTH 1000
#define WIN_LEFT_EDGE 0
#define WIN_TOP_EDGE 0
#define WIN_WIDTH 640
#define WIN_MIN_WIDTH 640
#define WIN_HEIGHT 500
#define WIN_MIN_HEIGHT 500
#define BUTTON_GADGET_NUM 0
#define BUTTON_WIDTH 100
#define BUTTON_HEIGHT 30

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *TranslatorBase;
struct Window *window;
struct Screen *screen;
struct MsgPort *NarratorPort;
struct narrator_rb *NarratorIO;
struct Gadget *gadget;
BYTE audioChannels[4] = {3, 5, 10, 12};
LONG exitCode;
UBYTE translationBuffer[1000];

UWORD buttonBorderData[] = {
	0, 0, BUTTON_WIDTH + 1, 0, BUTTON_WIDTH + 1, BUTTON_HEIGHT + 1,
	0, BUTTON_HEIGHT + 1, 0, 0,
};
struct Border buttonBorder = {
	-1, -1, 1, 0, JAM1, 5, buttonBorderData, NULL,
};

struct IntuiText buttonText = {
	3, 3, JAM1, 8, 10, NULL, "Press me!\0", NULL
};

struct Gadget buttonGadget = {
	NULL, 20, 50, BUTTON_WIDTH, BUTTON_HEIGHT,
	GFLG_GADGHCOMP, GACT_RELVERIFY | GACT_IMMEDIATE,
	GTYP_BOOLGADGET, &buttonBorder, NULL, &buttonText, 0, NULL, BUTTON_GADGET_NUM, NULL,
};

typedef enum {
	EXIT_APPLICATION, ALERT_BUTTON_PRESSED, SPEAK_BUTTON_PRESSED
} Action;

void openLibraries();
void openDevices();
void closeLibraries();
void closeDevices();
void configureApp();
void initVideo();
void cleanExit(LONG);
void speakText(STRPTR text);
Action handleIDCMP(struct Window*);


enum ScreenMode {
	PAL_NON_INTERLACED = 256,
	NTSC_NON_INTERLACED = 200,
	PAL_INTERLACED = 512,
	NTSC_INTERLACED = 400
};

struct Config {
	enum ScreenMode screenMode;
} config;

int main() {
	exitCode = 0;
	SysBase = *((struct ExecBase**)4UL);
	ULONG signalMask, winSignal, signals;
	BOOL done = FALSE;
	STRPTR englishString = "Never gonna give you up, never gonna let you down, never going to run around and desert you";

	openLibraries();
	if (exitCode)
		goto exit;
	
	openDevices();
	if (exitCode)
		goto exit;

	configureApp();

	initVideo();
	if (exitCode)
		goto exit;

	winSignal = 1L << window->UserPort->mp_SigBit;
	signalMask = winSignal;

	Move(window->RPort, 30, 20);

	UBYTE article[] = "This is a test string!";

	Text(window->RPort, article, sizeof(article) - 1);

	do {
		signals = Wait(signalMask);

		if (signals & winSignal)
			switch (handleIDCMP(window)) {
				case EXIT_APPLICATION:
					done = TRUE;
					break;
				case ALERT_BUTTON_PRESSED:
					speakText(englishString);
					break;
				default:
					break;
			}
	} while (!done);

	cleanExit(RETURN_OK);

exit:
	return exitCode;
}

void openLibraries() {
	IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 37);
	if (IntuitionBase == NULL) {
		cleanExit(RETURN_ERROR);
		return;
	}
		

	GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 0);
	if (GfxBase == NULL) {
		cleanExit(RETURN_ERROR);
		return;
	}

	#ifdef OLD_SPEECH
		#ifdef EMULATOR
		TranslatorBase = (struct Library *)OpenLibrary("libs:old/translator.library", 0);
		#else
		TranslatorBase = (struct Library *)OpenLibrary("PROGDIR:libs/translator.library", 0);
		#endif
	#else
	TranslatorBase = (struct Library *)OpenLibrary("translator.library", 0);
	#endif
	if (TranslatorBase == NULL)
		cleanExit(RETURN_ERROR);
}

void openDevices() {
	NarratorPort = CreateMsgPort();
    if (!NarratorPort) {
		cleanExit(RETURN_ERROR);
		return;
	}

    NarratorIO = CreateIORequest(NarratorPort, sizeof(struct narrator_rb));
    if (!NarratorIO) {
		cleanExit(RETURN_ERROR);
		return;
	}

	#ifdef OLD_SPEECH
		#ifdef EMULATOR
		if (OpenDevice("devs:old/narrator.device", 0, (struct IORequest *)NarratorIO, 0L) != 0) {
		#else
		if (OpenDevice("PROGDIR:devs/narrator.device", 0, (struct IORequest *)NarratorIO, 0L) != 0) {
		#endif
	#else
	if (OpenDevice("narrator.device", 0, (struct IORequest *)NarratorIO, 0L) != 0) {
		NarratorIO->flags = NDF_NEWIORB;
	#endif
			cleanExit(RETURN_ERROR);
			return;
	}
}

void initVideo() {
	UWORD pens[] = {~0};

	screen = OpenScreenTags(NULL,
	SA_Pens, (ULONG)pens,
	SA_DisplayID, HIRES_KEY,
	SA_Depth, 2,
	SA_Title, (ULONG)"OpenAI Amiga",
	TAG_DONE);

	if (screen == NULL) {
		cleanExit(RETURN_ERROR);
		return;
	}

	SetRGB4(&(screen->ViewPort), 0, 0x0, 0x1, 0x2);
	SetRGB4(&(screen->ViewPort), 1, 0x0, 0x0, 0x0);
	SetRGB4(&(screen->ViewPort), 3, 0x0, 0xFF, 0x2);

	window = OpenWindowTags(NULL,
	WA_Left, WIN_LEFT_EDGE,
	WA_Top, WIN_TOP_EDGE,
	WA_Width, WIN_WIDTH,
	WA_Height, WIN_HEIGHT,
	WA_MinWidth, WIN_MIN_WIDTH,
	WA_MinHeight, WIN_MIN_HEIGHT,
	WA_MaxWidth, ~0,
	WA_MaxHeight, ~0,
	WA_Gadgets, (ULONG)&buttonGadget,
	WA_Title, (ULONG)"OpenAI Amiga",
	WA_CloseGadget, TRUE,
	WA_SizeGadget, FALSE,
	WA_DepthGadget, FALSE,
	WA_DragBar, FALSE,
	WA_Activate, TRUE,
	WA_NoCareRefresh, TRUE,
	WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP,
	WA_CustomScreen, screen,
	TAG_DONE);

	if (window == NULL)
		cleanExit(RETURN_ERROR);
}

void closeLibraries() {
	CloseLibrary(IntuitionBase);
	CloseLibrary(GfxBase);
	CloseLibrary(TranslatorBase);
}

void closeDevices() {
	 if (NarratorIO) {
		DeleteIORequest((struct IORequest *)NarratorIO);
		CloseDevice((struct IORequest *)NarratorIO);
	 }

	 if (NarratorPort)
	  DeleteMsgPort(NarratorPort);
}

void configureApp() {
	config.screenMode = PAL_NON_INTERLACED;
}

Action handleIDCMP(struct Window *window) {
	struct IntuiMessage *message = NULL;
	ULONG class;

	while (message = (struct IntuiMessage *)GetMsg(window->UserPort)) {
		class = message->Class;
		ReplyMsg((struct Message *)message);

		switch (class) {
			case IDCMP_CLOSEWINDOW:
				return EXIT_APPLICATION;
			case IDCMP_GADGETUP:
				return ALERT_BUTTON_PRESSED;
			default:
				break;
		}
	}

	return NULL;
}

void cleanExit(LONG returnValue) {
	// There seems to be a bug with the Exit() call causing the program to guru. Use dirty goto's for now
	CloseWindow(window);
	CloseScreen(screen);
	closeLibraries();
	closeDevices();
	exitCode = returnValue;
	// Exit(returnValue);
}

void speakText(STRPTR text) {
	LONG textLength = strlen(text);
	Translate(text, textLength, (STRPTR)&translationBuffer, TRANSLATION_BUFFER_LENGTH);
	LONG translatedStringLength = strlen(translationBuffer);

	NarratorIO->ch_masks = audioChannels;
	NarratorIO->nm_masks = sizeof(audioChannels);
	NarratorIO->message.io_Command= CMD_WRITE;
    NarratorIO->message.io_Data = translationBuffer;
    NarratorIO->message.io_Length = translatedStringLength;
    DoIO((struct IORequest *)NarratorIO);
}