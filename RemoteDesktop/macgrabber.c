#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <Carbon/Carbon.h>
#include <IOSurface/IOSurface.h>
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <IOKit/pwr_mgt/IOPM.h>
#include <pthread.h>

#include "xgrabber.h"
#include "../thirdparty/libvncserver/rfb/keysym.h"

void error_for_exit(char *str);

typedef struct XGrabber {
    /* The multi-sceen display number chosen by the user */
    int displayNumber;
    /* The corresponding multi-sceen display ID */
    CGDirectDisplayID displayID;

    dispatch_queue_t dispatchQueue;
    CGDisplayStreamRef stream;

    char *frameBuffer;
    pthread_mutex_t frameBuffer_mutex;

    int width, height, depth;

    CGEventSourceRef eventSource;
    CFMutableDictionaryRef charKeyMap;
    CFMutableDictionaryRef charShiftKeyMap;
    CFMutableDictionaryRef charAltGrKeyMap;
    CFMutableDictionaryRef charShiftAltGrKeyMap;

    int isShiftDown;
    int isAltGrDown;
} XGrabber;

static int specialKeyMap[] = {
    /* "Special" keys */
    XK_space,             49,      /* Space */
    XK_Return,            36,      /* Return */
    XK_Delete,           117,      /* Delete */
    XK_Tab,               48,      /* Tab */
    XK_Escape,            53,      /* Esc */
    XK_Caps_Lock,         57,      /* Caps Lock */
    XK_Num_Lock,          71,      /* Num Lock */
    XK_Scroll_Lock,      107,      /* Scroll Lock */
    XK_Pause,            113,      /* Pause */
    XK_BackSpace,         51,      /* Backspace */
    XK_Insert,           114,      /* Insert */

    /* Cursor movement */
    XK_Up,               126,      /* Cursor Up */
    XK_Down,             125,      /* Cursor Down */
    XK_Left,             123,      /* Cursor Left */
    XK_Right,            124,      /* Cursor Right */
    XK_Page_Up,          116,      /* Page Up */
    XK_Page_Down,        121,      /* Page Down */
    XK_Home,             115,      /* Home */
    XK_End,              119,      /* End */

    /* Numeric keypad */
    XK_KP_0,              82,      /* KP 0 */
    XK_KP_1,              83,      /* KP 1 */
    XK_KP_2,              84,      /* KP 2 */
    XK_KP_3,              85,      /* KP 3 */
    XK_KP_4,              86,      /* KP 4 */
    XK_KP_5,              87,      /* KP 5 */
    XK_KP_6,              88,      /* KP 6 */
    XK_KP_7,              89,      /* KP 7 */
    XK_KP_8,              91,      /* KP 8 */
    XK_KP_9,              92,      /* KP 9 */
    XK_KP_Enter,          76,      /* KP Enter */
    XK_KP_Decimal,        65,      /* KP . */
    XK_KP_Add,            69,      /* KP + */
    XK_KP_Subtract,       78,      /* KP - */
    XK_KP_Multiply,       67,      /* KP * */
    XK_KP_Divide,         75,      /* KP / */

    /* Function keys */
    XK_F1,               122,      /* F1 */
    XK_F2,               120,      /* F2 */
    XK_F3,                99,      /* F3 */
    XK_F4,               118,      /* F4 */
    XK_F5,                96,      /* F5 */
    XK_F6,                97,      /* F6 */
    XK_F7,                98,      /* F7 */
    XK_F8,               100,      /* F8 */
    XK_F9,               101,      /* F9 */
    XK_F10,              109,      /* F10 */
    XK_F11,              103,      /* F11 */
    XK_F12,              111,      /* F12 */

    /* Modifier keys */
    XK_Shift_L,           56,      /* Shift Left */
    XK_Shift_R,           56,      /* Shift Right */
    XK_Control_L,         59,      /* Ctrl Left */
    XK_Control_R,         59,      /* Ctrl Right */
    XK_Meta_L,            58,      /* Logo Left (-> Option) */
    XK_Meta_R,            58,      /* Logo Right (-> Option) */
    XK_Alt_L,             55,      /* Alt Left (-> Command) */
    XK_Alt_R,             55,      /* Alt Right (-> Command) */
    XK_ISO_Level3_Shift,  61,      /* Alt-Gr (-> Option Right) */
    0x1008FF2B,           63,      /* Fn */

    /* Weirdness I can't figure out */
#if 0
    XK_3270_PrintScreen,     105,     /* PrintScrn */
    ???  94,          50,      /* International */
    XK_Menu,              50,      /* Menu (-> International) */
#endif
};

static int keyboardInit(struct XGrabber *grabber)
{
    size_t i, keyCodeCount=128;
    const UCKeyboardLayout *keyboardLayout;

    // Check and make sure assistive devices is enabled.
    if (AXIsProcessTrustedWithOptions != NULL) {
	// New accessibility API 10.9 and later.
	const void * keys[] = { kAXTrustedCheckOptionPrompt };
	const void * values[] = { kCFBooleanTrue };

	CFDictionaryRef options = CFDictionaryCreate(
		kCFAllocatorDefault,
		keys,
		values,
		sizeof(keys) / sizeof(*keys),
		&kCFCopyStringDictionaryKeyCallBacks,
		&kCFTypeDictionaryValueCallBacks);

	int is_enabled = AXIsProcessTrustedWithOptions(options);
	if (!is_enabled) {
	    error_for_exit("This application needs to post input events, but it\n"
		       "does not have the necessary system permission.\n"
		       "Please check if the program has been given permission\n"
		       "to control your computer in 'System Preferences'->\n"
		       "'Security & Privacy'->'Privacy'->'Accessibility'.");
	    return FALSE;
	}
    }

//    if (!AXIsProcessTrusted()) {
//        error_for_exit("This application needs to post input events, but it\n"
//		       "does not have the necessary system permission.\n"
//		       "Please check if the program has been given permission\n"
//		       "to control your computer in 'System Preferences'->\n"
//		       "'Security & Privacy'->'Privacy'->'Accessibility'.");
//        return FALSE;
//    }

    TISInputSourceRef currentKeyboard = TISCopyCurrentKeyboardInputSource();

    if(!currentKeyboard) {
	fprintf(stderr, "Could not get current keyboard info\n");
	return FALSE;
    }

    keyboardLayout = (const UCKeyboardLayout *)CFDataGetBytePtr(TISGetInputSourceProperty(currentKeyboard, kTISPropertyUnicodeKeyLayoutData));

    printf("Found keyboard layout '%s'\n", CFStringGetCStringPtr(TISGetInputSourceProperty(currentKeyboard, kTISPropertyInputSourceID), kCFStringEncodingUTF8));

    grabber->charKeyMap = CFDictionaryCreateMutable(kCFAllocatorDefault, keyCodeCount, &kCFCopyStringDictionaryKeyCallBacks, NULL);
    grabber->charShiftKeyMap = CFDictionaryCreateMutable(kCFAllocatorDefault, keyCodeCount, &kCFCopyStringDictionaryKeyCallBacks, NULL);
    grabber->charAltGrKeyMap = CFDictionaryCreateMutable(kCFAllocatorDefault, keyCodeCount, &kCFCopyStringDictionaryKeyCallBacks, NULL);
    grabber->charShiftAltGrKeyMap = CFDictionaryCreateMutable(kCFAllocatorDefault, keyCodeCount, &kCFCopyStringDictionaryKeyCallBacks, NULL);

    if(!grabber->charKeyMap || !grabber->charShiftKeyMap || !grabber->charAltGrKeyMap || !grabber->charShiftAltGrKeyMap) {
	fprintf(stderr, "Could not create keymaps\n");
	return FALSE;
    }

    /* Loop through every keycode to find the character it is mapping to. */
    for (i = 0; i < keyCodeCount; ++i) {
	UInt32 deadKeyState = 0;
	UniChar chars[4];
	UniCharCount realLength;
	UInt32 m, modifiers[] = {0, kCGEventFlagMaskShift, kCGEventFlagMaskAlternate, kCGEventFlagMaskShift|kCGEventFlagMaskAlternate};

	/* do this for no modifier, shift and alt-gr applied */
	for(m = 0; m < sizeof(modifiers) / sizeof(modifiers[0]); ++m) {
	    UCKeyTranslate(keyboardLayout,
			   i,
			   kUCKeyActionDisplay,
			   (modifiers[m] >> 16) & 0xff,
			   LMGetKbdType(),
			   kUCKeyTranslateNoDeadKeysBit,
			   &deadKeyState,
			   sizeof(chars) / sizeof(chars[0]),
			   &realLength,
			   chars);

	    CFStringRef string = CFStringCreateWithCharacters(kCFAllocatorDefault, chars, 1);
	    if(string) {
		switch(modifiers[m]) {
		case 0:
		    CFDictionaryAddValue(grabber->charKeyMap, string, (const void *)i);
		    break;
		case kCGEventFlagMaskShift:
		    CFDictionaryAddValue(grabber->charShiftKeyMap, string, (const void *)i);
		    break;
		case kCGEventFlagMaskAlternate:
		    CFDictionaryAddValue(grabber->charAltGrKeyMap, string, (const void *)i);
		    break;
		case kCGEventFlagMaskShift|kCGEventFlagMaskAlternate:
		    CFDictionaryAddValue(grabber->charShiftAltGrKeyMap, string, (const void *)i);
		    break;
		}

		CFRelease(string);
	    }
	}
    }

    CFRelease(currentKeyboard);

    return TRUE;
}


struct XGrabber *GrabberInit(int *w, int *h, int *d)
{
    CGDisplayCount displayCount;
    CGDirectDisplayID displays[32];
    int bitsPerSample = 8;
    int i;

    struct XGrabber *grabber = (struct XGrabber *) malloc(sizeof(struct XGrabber));

    grabber->eventSource = CGEventSourceCreate(kCGEventSourceStatePrivate);

    if (!keyboardInit(grabber)) {
	free(grabber);
	return NULL;
    }

    grabber->displayNumber = -1;

    /* grab the active displays */
    CGGetActiveDisplayList(32, displays, &displayCount);
    for (i = 0; i < displayCount; i++) {
	CGRect bounds = CGDisplayBounds(displays[i]);
	printf("Found %s display %d at (%d,%d) and a resolution of %dx%d\n", (CGDisplayIsMain(displays[i]) ? "primary" : "secondary"), i, (int)bounds.origin.x, (int)bounds.origin.y, (int)bounds.size.width, (int)bounds.size.height);
    }

    if(grabber->displayNumber < 0) {
	printf("Using primary display as a default\n");
	grabber->displayID = CGMainDisplayID();
    } else if (grabber->displayNumber < displayCount) {
	printf("Using specified display %d\n", grabber->displayNumber);
	grabber->displayID = displays[grabber->displayNumber];
    } else {
	fprintf(stderr, "Specified display %d does not exist\n", grabber->displayNumber);
	free(grabber);
	return NULL;
    }

    grabber->width = CGDisplayPixelsWide(grabber->displayID);
    grabber->height = CGDisplayPixelsHigh(grabber->displayID);
    grabber->depth = 4;

    grabber->frameBuffer = (char *)malloc(grabber->width * grabber->height * grabber->depth);
    if (!grabber->frameBuffer) {
	free(grabber);
	return NULL;
    }

    grabber->dispatchQueue = dispatch_queue_create("projector.app.mac", NULL);
    grabber->stream = CGDisplayStreamCreateWithDispatchQueue(grabber->displayID,
								     CGDisplayPixelsWide(grabber->displayID),
								     CGDisplayPixelsHigh(grabber->displayID),
								     'BGRA',
								     nil,
								     grabber->dispatchQueue,
								    ^(CGDisplayStreamFrameStatus status,
									uint64_t displayTime,
									IOSurfaceRef frameSurface,
									CGDisplayStreamUpdateRef updateRef) {

									if (status == kCGDisplayStreamFrameStatusFrameComplete && frameSurface != NULL) {
									    const CGRect *updatedRects;
									    size_t updatedRectsCount;
									    size_t r;

//									    if(startTime>0 && time(0)>startTime+maxSecsToConnect) {
//										 serverShutdown(0);
//									    }

									    /*
										Copy new frame to back buffer.
									    */
									    IOSurfaceLock(frameSurface, kIOSurfaceLockReadOnly, NULL);

									    pthread_mutex_lock(&grabber->frameBuffer_mutex);

									    memcpy(grabber->frameBuffer,
										IOSurfaceGetBaseAddress(frameSurface),
										CGDisplayPixelsWide(grabber->displayID) *  CGDisplayPixelsHigh(grabber->displayID) * 4);

									    pthread_mutex_unlock(&grabber->frameBuffer_mutex);

									    IOSurfaceUnlock(frameSurface, kIOSurfaceLockReadOnly, NULL);
									}

								    });
    if(grabber->stream) {
	pthread_mutex_init(&grabber->frameBuffer_mutex, NULL);
	CGDisplayStreamStart(grabber->stream);
    } else {
	error_for_exit("Could not get screen contents. Check if the application has \n"
		       "been given screen recording permisssions in 'System Preferences'->\n"
		       "'Security & Privacy'->'Privacy'->'Screen Recording'.");
	free(grabber);
	return NULL;
  }

    *w = grabber->width;
    *h = grabber->height;
    *d = grabber->depth;

    return grabber;
}

void GrabberFinish(struct XGrabber *cfg)
{
    CGDisplayStreamStop(cfg->stream);
    pthread_mutex_destroy(&cfg->frameBuffer_mutex);
    free(cfg->frameBuffer);
    free(cfg);
}

int GrabberGetScreen(struct XGrabber *cfg, int x, int y, int w, int h, void (*cb)(void *arg, void *fb), void *arg)
{
    pthread_mutex_lock(&cfg->frameBuffer_mutex);

    cb(arg, cfg->frameBuffer);

    pthread_mutex_unlock(&cfg->frameBuffer_mutex);

    return 0;
}

void GrabberMouseEvent(struct XGrabber *cfg, uint32_t buttonMask, int x, int y)
{
    CGPoint position;
    CGRect displayBounds = CGDisplayBounds(cfg->displayID);
    CGEventRef mouseEvent = NULL;

//    undim();

    position.x = x + displayBounds.origin.x;
    position.y = y + displayBounds.origin.y;

    /* map buttons 4 5 6 7 to scroll events as per https://github.com/rfbproto/rfbproto/blob/master/rfbproto.rst#745pointerevent */
    if(buttonMask & (1 << 3))
	mouseEvent = CGEventCreateScrollWheelEvent(cfg->eventSource, kCGScrollEventUnitLine, 2, 1, 0);
    if(buttonMask & (1 << 4))
	mouseEvent = CGEventCreateScrollWheelEvent(cfg->eventSource, kCGScrollEventUnitLine, 2, -1, 0);
    if(buttonMask & (1 << 5))
	mouseEvent = CGEventCreateScrollWheelEvent(cfg->eventSource, kCGScrollEventUnitLine, 2, 0, 1);
    if(buttonMask & (1 << 6))
	mouseEvent = CGEventCreateScrollWheelEvent(cfg->eventSource, kCGScrollEventUnitLine, 2, 0, -1);

    if (mouseEvent) {
	CGEventPost(kCGSessionEventTap, mouseEvent);
	CFRelease(mouseEvent);
    }
    else {
	/*
	  Use the deprecated CGPostMouseEvent API here as we get a buttonmask plus position which is pretty low-level
	  whereas CGEventCreateMouseEvent is expecting higher-level events. This allows for direct injection of
	  double clicks and drags whereas we would need to synthesize these events for the high-level API.
	 */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	CGPostMouseEvent(position, TRUE, 3,
			 (buttonMask & (1 << 0)) ? TRUE : FALSE,
			 (buttonMask & (1 << 2)) ? TRUE : FALSE,
			 (buttonMask & (1 << 1)) ? TRUE : FALSE);
#pragma clang diagnostic pop
    }
}

void GrabberKeyboardEvent(struct XGrabber *cfg, int down, uint32_t keySym)
{
    int i;
    CGKeyCode keyCode = -1;
    CGEventRef keyboardEvent;
    int specialKeyFound = 0;

//    undim();

    /* look for special key */
    for (i = 0; i < (sizeof(specialKeyMap) / sizeof(int)); i += 2) {
        if (specialKeyMap[i] == keySym) {
            keyCode = specialKeyMap[i+1];
            specialKeyFound = 1;
            break;
        }
    }

    if(specialKeyFound) {
	/* keycode for special key found */
	keyboardEvent = CGEventCreateKeyboardEvent(cfg->eventSource, keyCode, down);
	/* save state of shifting modifiers */
	if(keySym == XK_ISO_Level3_Shift)
	    cfg->isAltGrDown = down;
	if(keySym == XK_Shift_L || keySym == XK_Shift_R)
	    cfg->isShiftDown = down;

    } else {
	/* look for char key */
	size_t keyCodeFromDict;
	CFStringRef charStr = CFStringCreateWithCharacters(kCFAllocatorDefault, (UniChar*)&keySym, 1);
	if (charStr) {
	    CFMutableDictionaryRef keyMap = cfg->charKeyMap;
	    if(cfg->isShiftDown && !cfg->isAltGrDown)
		keyMap = cfg->charShiftKeyMap;
	    if(!cfg->isShiftDown && cfg->isAltGrDown)
		keyMap = cfg->charAltGrKeyMap;
	    if(cfg->isShiftDown && cfg->isAltGrDown)
		keyMap = cfg->charShiftAltGrKeyMap;

	    if (CFDictionaryGetValueIfPresent(keyMap, charStr, (const void **)&keyCodeFromDict)) {
		/* keycode for ASCII key found */
		keyboardEvent = CGEventCreateKeyboardEvent(cfg->eventSource, keyCodeFromDict, down);
	    } else {
		/* last resort: use the symbol's utf-16 value, does not support modifiers though */
		keyboardEvent = CGEventCreateKeyboardEvent(cfg->eventSource, 0, down);
		CGEventKeyboardSetUnicodeString(keyboardEvent, 1, (UniChar*)&keySym);
	    }
	    CFRelease(charStr);
	}
    }

    /* Set the Shift modifier explicitly as MacOS sometimes gets internal state wrong and Shift stuck. */
    CGEventSetFlags(keyboardEvent, CGEventGetFlags(keyboardEvent) & (cfg->isShiftDown ? kCGEventFlagMaskShift : ~kCGEventFlagMaskShift));

    CGEventPost(kCGSessionEventTap, keyboardEvent);
    CFRelease(keyboardEvent);
}
