#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <windows.h>
#include <d3d9.h>

#include "xgrabber.h"
#include "keysym.h"

typedef struct XGrabber {
    int width, height, depth;
    int mouseButtons;
    uint32_t modKeys;

    IDirect3D9 *d3d;
    IDirect3DDevice9 *device;
    IDirect3DSurface9 *surface;
    D3DDISPLAYMODE mode;

    char *frameBuffer;
} XGrabber;

struct XGrabber *GrabberInit(int *w, int *h, int *d)
{
    struct XGrabber *grabber = (struct XGrabber *) malloc(sizeof(struct XGrabber));

    grabber->mouseButtons = 0;

    grabber->width = GetSystemMetrics(SM_CXSCREEN);
    grabber->height = GetSystemMetrics(SM_CYSCREEN);
    grabber->depth = 4;

    grabber->frameBuffer = (char *)malloc(grabber->width * grabber->height * grabber->depth);
    if (!grabber->frameBuffer) {
	free(grabber);
	return NULL;
    }

    memset(grabber->frameBuffer, 0, grabber->width * grabber->height * grabber->depth);

    *w = grabber->width;
    *h = grabber->height;
    *d = grabber->depth;

    UINT adapter = D3DADAPTER_DEFAULT;
    D3DPRESENT_PARAMETERS parameters = { 0 };

    grabber->d3d = nullptr;
    grabber->device = nullptr;
    grabber->surface = nullptr;

    // init D3D and get screen size
    grabber->d3d = Direct3DCreate9(D3D_SDK_VERSION);
    grabber->d3d->GetAdapterDisplayMode(adapter, &grabber->mode);

    parameters.Windowed = TRUE;
    parameters.BackBufferCount = 1;
    parameters.BackBufferHeight = grabber->mode.Height;
    parameters.BackBufferWidth = grabber->mode.Width;
    parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    parameters.hDeviceWindow = NULL;

    grabber->d3d->CreateDevice(adapter, D3DDEVTYPE_HAL, NULL, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &parameters, &grabber->device);
    grabber->device->CreateOffscreenPlainSurface(grabber->mode.Width, grabber->mode.Height, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &grabber->surface, nullptr);

    return grabber;
}

void GrabberFinish(struct XGrabber *cfg)
{
    if (cfg->surface != nullptr) {
	cfg->surface->Release();
    }

    if (cfg->device != nullptr) {
	cfg->device->Release();
    }
    if (cfg->d3d != nullptr) {
	cfg->d3d->Release();
    }

    free(cfg->frameBuffer);
    free(cfg);
}

int GrabberGetScreen(struct XGrabber *cfg, int x, int y, int w, int h, void (*cb)(void *arg, void *fb), void *arg)
{
    D3DLOCKED_RECT rc;

    if (cb) {
	cb(arg, cfg->frameBuffer);
    }

    cfg->device->GetFrontBufferData(0, cfg->surface);

    cfg->surface->LockRect(&rc, NULL, 0);

    memcpy(cfg->frameBuffer, rc.pBits, cfg->width * cfg->height * cfg->depth);

    cfg->surface->UnlockRect();

    return 0;
}

#define MOUSE_LB 1
#define MOUSE_MB 2
#define MOUSE_RB 4
#define MOUSE_WD 8
#define MOUSE_WU 16

void GrabberMouseEvent(struct XGrabber *cfg, uint32_t buttons, int x, int y)
{
    INPUT input = {0};

    double fx = x * (65535.0f / (double)cfg->width);
    double fy = y * (65535.0f / (double)cfg->height);

    input.type = INPUT_MOUSE;
    input.mi.dwExtraInfo = GetMessageExtraInfo();
    input.mi.time = 0;
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    input.mi.dx = fx;
    input.mi.dy = fy;
    SendInput(1, &input, sizeof(INPUT));

    input.mi.dwFlags = 0;

    if ((cfg->mouseButtons & MOUSE_LB) != (buttons & MOUSE_LB)) {
	if (buttons & MOUSE_LB) {
	    input.mi.dwFlags |= MOUSEEVENTF_LEFTDOWN;
	} else {
	    input.mi.dwFlags |= MOUSEEVENTF_LEFTUP;
	}
    }

    if ((cfg->mouseButtons & MOUSE_RB) != (buttons & MOUSE_RB)) {
	if (buttons & MOUSE_RB) {
	    input.mi.dwFlags |= MOUSEEVENTF_RIGHTDOWN;
	} else {
	    input.mi.dwFlags |= MOUSEEVENTF_RIGHTUP;
	}
    }

    if ((cfg->mouseButtons & MOUSE_MB) != (buttons & MOUSE_MB)) {
	if (buttons & MOUSE_MB) {
	    input.mi.dwFlags |= MOUSEEVENTF_MIDDLEDOWN;
	} else {
	    input.mi.dwFlags |= MOUSEEVENTF_MIDDLEUP;
	}
    }

    if (buttons & MOUSE_WD) {
	input.mi.dwFlags |= MOUSEEVENTF_WHEEL;
	input.mi.mouseData = WHEEL_DELTA;
    }

    if (buttons & MOUSE_WU) {
	input.mi.dwFlags |= MOUSEEVENTF_WHEEL;
	input.mi.mouseData = -WHEEL_DELTA;
    }

    SendInput(1, &input, sizeof(INPUT));

    cfg->mouseButtons = buttons;
}

static short convertVNCKeyToVK(uint32_t key)
{
    switch(key) {
    case XK_Escape: return VK_ESCAPE;
    case XK_F1: return VK_F1;
    case XK_F2: return VK_F2;
    case XK_F3: return VK_F3;
    case XK_F4: return VK_F4;
    case XK_F5: return VK_F5;
    case XK_F6: return VK_F6;
    case XK_F7: return VK_F7;
    case XK_F8: return VK_F8;
    case XK_F9: return VK_F9;
    case XK_F10: return VK_F10;
    case XK_F11: return VK_F11;
    case XK_F12: return VK_F12;
    case XK_Print: return VK_PRINT;
    case XK_Insert: return VK_INSERT;
    case XK_Delete: return VK_DELETE;
    case XK_BackSpace: return VK_BACK;
    case XK_Tab: return VK_TAB;
    case XK_Return: return VK_RETURN;
    case XK_Shift_R: return VK_RSHIFT;
    case XK_Shift_L: return VK_SHIFT;
    case XK_Control_R: return VK_CONTROL;
    case XK_Control_L: return VK_CONTROL;
    case XK_Meta_R: return VK_RWIN;
    case XK_Meta_L: return VK_LWIN;
    case XK_Alt_R: return VK_RMENU;
    case XK_Alt_L: return VK_MENU;
    case XK_Up: return VK_UP;
    case XK_Down: return VK_DOWN;
    case XK_Left: return VK_LEFT;
    case XK_Right: return VK_RIGHT;
    case XK_Page_Up: return VK_PRIOR;
    case XK_Page_Down: return VK_NEXT;
    case XK_Home: return VK_HOME;
    case XK_End: return VK_END;
    case XK_Select: return VK_SELECT;
    case XK_Execute: return VK_EXECUTE;
    case XK_Help: return VK_HELP;
    case XK_Break: return VK_CANCEL;

    case XK_KP_Space: return VK_SPACE;
    case XK_KP_Tab: return VK_TAB;
//    case XK_Caps_Lock: return VK_CAPITAL;
    case XK_KP_Enter: return VK_RETURN;
    case XK_KP_F1: return VK_F1;
    case XK_KP_F2: return VK_F2;
    case XK_KP_F3: return VK_F3;
    case XK_KP_F4: return VK_F4;
    case XK_KP_Home: return VK_HOME;
    case XK_KP_Left: return VK_LEFT;
    case XK_KP_Up: return VK_UP;
    case XK_KP_Right: return VK_RIGHT;
    case XK_KP_Down: return VK_DOWN;
    case XK_KP_End: return VK_END;
    case XK_KP_Page_Up: return VK_PRIOR;
    case XK_KP_Page_Down: return VK_NEXT;
    case XK_KP_Begin: return VK_CLEAR;
    case XK_KP_Insert: return VK_INSERT;
    case XK_KP_Delete: return VK_DELETE;
  // XXX XK_KP_Equal should map in the same way as ascii '='
    case XK_KP_Multiply: return VK_MULTIPLY;
    case XK_KP_Add: return VK_ADD;
    case XK_KP_Separator: return VK_SEPARATOR;
    case XK_KP_Subtract: return VK_SUBTRACT;
    case XK_KP_Decimal: return VK_DECIMAL;
    case XK_KP_Divide: return VK_DIVIDE;

    case XK_KP_0: return VK_NUMPAD0;
    case XK_KP_1: return VK_NUMPAD1;
    case XK_KP_2: return VK_NUMPAD2;
    case XK_KP_3: return VK_NUMPAD3;
    case XK_KP_4: return VK_NUMPAD4;
    case XK_KP_5: return VK_NUMPAD5;
    case XK_KP_6: return VK_NUMPAD6;
    case XK_KP_7: return VK_NUMPAD7;
    case XK_KP_8: return VK_NUMPAD8;
    case XK_KP_9: return VK_NUMPAD9;
    }

    return 0;
}

#define KEY_CTRL 1
#define KEY_SHIFT 2
#define KEY_ALT 4
#define KEY_META 8

void GrabberKeyboardEvent(struct XGrabber *cfg, int down, uint32_t keysym)
{
    short s;
    INPUT input = {0};

    if ((keysym >= 32 && keysym <= 126) ||
        (keysym >= 160 && keysym <= 255)) {
	s = VkKeyScan((char)keysym);
	if (s == -1) {
	    fprintf(stderr, "ignoring unrecognised Latin-1 keysym %d\n", keysym);
	    return;
	}

//	fprintf(stderr, "%d %04X\n", down, s);
    } else {
//	fprintf(stderr, "%d %08X\n", down, keysym);
	s = convertVNCKeyToVK(keysym);
	if (s == 0) {
	    fprintf(stderr, "ignoring unrecognised keysym %d\n", keysym);
	    return;
	}
    }

    input.type = INPUT_KEYBOARD;

    if (down) {
	if (s & 0x100) {
	    input.ki.wVk = VK_SHIFT;
	    SendInput(1, &input, sizeof(INPUT));
	}

	input.ki.wVk = s & 0xff;
	SendInput(1, &input, sizeof(INPUT));
    } else {
	input.ki.dwFlags = KEYEVENTF_KEYUP;

	input.ki.wVk = s & 0xff;
	SendInput(1, &input, sizeof(INPUT));

	if (s & 0x100) {
	    input.ki.wVk = VK_SHIFT;
	    SendInput(1, &input, sizeof(INPUT));
	}
    }
}
