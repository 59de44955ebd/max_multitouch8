/**
	@file
	multitouch8 - a max external
	valentin schmidt
*/

#include "ext.h"       // standard Max include, always required
#include "ext_obex.h"  // required for new style Max object

#include <Windows.h>

#define _USE_MATH_DEFINES
#include <math.h>

////////////////////////// object struct
typedef struct _multitouch
{
	t_object ob;  // the object itself (must be first)

	void *m_outlet_touch;
	void *m_outlet_gesture;

	WNDPROC m_oldproc;
	HWND m_hwnd;
	t_symbol *m_wincaption;

	char gesture_zoom;

	char gesture_rotate;
	char gesture_twoFingerTap;
	char gesture_pressAndTap;
	char gesture_pan_horizontal;
	char gesture_pan_vertical;
	char gesture_pan_with_gutter;
	char gesture_pan_with_inertia;

} t_multitouch;

///////////////////////// function prototypes
void *multitouch_new(t_symbol *s, long argc, t_atom *argv);
void multitouch_free(t_multitouch *x);

void multitouch_start(t_multitouch *x);
void multitouch_stop(t_multitouch *x);

void multitouch_assist(t_multitouch *x, void *b, long m, long a, char *s);

LRESULT CALLBACK MyWndProc(HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam);

//////////////////////// global class pointer variable
void *multitouch_class;

//##########################################################
//
//##########################################################
int C74_EXPORT main(void)
{
	multitouch_class = class_new("multitouch8", (method)multitouch_new, (method)multitouch_free,
			(long)sizeof(t_multitouch), 0L /* leave NULL!! */, A_GIMME, 0);

	class_addmethod(multitouch_class, (method)multitouch_start, "start", 0);
	class_addmethod(multitouch_class, (method)multitouch_stop, "stop", 0);
	class_addmethod(multitouch_class, (method)multitouch_assist, "assist", A_CANT, 0);

	CLASS_ATTR_CHAR(multitouch_class, "gesture_zoom", 0, t_multitouch, gesture_zoom);
	CLASS_ATTR_DEFAULT_SAVE(multitouch_class, "gesture_zoom", 0, "0");
	CLASS_ATTR_STYLE_LABEL(multitouch_class, "gesture_zoom", 0, "onoff", "Zoom gestures");

	CLASS_ATTR_CHAR(multitouch_class, "gesture_rotate", 0, t_multitouch, gesture_rotate);
	CLASS_ATTR_DEFAULT_SAVE(multitouch_class, "gesture_rotate", 0, "0");
	CLASS_ATTR_STYLE_LABEL(multitouch_class, "gesture_rotate", 0, "onoff", "Rotate gestures");

	CLASS_ATTR_CHAR(multitouch_class, "gesture_twoFingerTap", 0, t_multitouch, gesture_twoFingerTap);
	CLASS_ATTR_DEFAULT_SAVE(multitouch_class, "gesture_twoFingerTap", 0, "0");
	CLASS_ATTR_STYLE_LABEL(multitouch_class, "gesture_twoFingerTap", 0, "onoff", "Two-finger tap gestures");

	CLASS_ATTR_CHAR(multitouch_class, "gesture_pressAndTap", 0, t_multitouch, gesture_pressAndTap);
	CLASS_ATTR_DEFAULT_SAVE(multitouch_class, "gesture_pressAndTap", 0, "0");
	CLASS_ATTR_STYLE_LABEL(multitouch_class, "gesture_pressAndTap", 0, "onoff", "Press and tap gestures");

	CLASS_ATTR_CHAR(multitouch_class, "gesture_pan_horizontal", 0, t_multitouch, gesture_pan_horizontal);
	CLASS_ATTR_DEFAULT_SAVE(multitouch_class, "gesture_pan_horizontal", 0, "0");
	CLASS_ATTR_STYLE_LABEL(multitouch_class, "gesture_pan_horizontal", 0, "onoff", "Pan gestures - horizontal");

	CLASS_ATTR_CHAR(multitouch_class, "gesture_pan_vertical", 0, t_multitouch, gesture_pan_vertical);
	CLASS_ATTR_DEFAULT_SAVE(multitouch_class, "gesture_pan_vertical", 0, "0");
	CLASS_ATTR_STYLE_LABEL(multitouch_class, "gesture_pan_vertical", 0, "onoff", "Pan gestures - vertical");

	CLASS_ATTR_CHAR(multitouch_class, "pan_with_gutter", 0, t_multitouch, gesture_pan_with_gutter);
	CLASS_ATTR_DEFAULT_SAVE(multitouch_class, "pan_with_gutter", 0, "0");
	CLASS_ATTR_STYLE_LABEL(multitouch_class, "pan_with_gutter", 0, "onoff", "Pan gestures: with gutter");
	//limit perpendicular movement to primary direction until a threshold is reached to break out of the gutter

	CLASS_ATTR_CHAR(multitouch_class, "pan_with_inertia", 0, t_multitouch, gesture_pan_with_inertia);
	CLASS_ATTR_DEFAULT_SAVE(multitouch_class, "pan_with_inertia", 0, "0");
	CLASS_ATTR_STYLE_LABEL(multitouch_class, "pan_with_inertia", 0, "onoff", "Pan gestures: with inertia");
	//pan with inertia to smoothly slow when pan gestures stop

	class_register(CLASS_BOX, multitouch_class);

	object_post(NULL, "multitouch8 - © valentin schmidt - compiled %s", __TIMESTAMP__);
}

//##########################################################
//
//##########################################################
void multitouch_free(t_multitouch *x)
{
	;
}

//##########################################################
//
//##########################################################
void multitouch_assist(t_multitouch *x, void *b, long m, long a, char *s)
{
	if (m == 1)
	{
		sprintf(s, "methods: start/stop");
	}
	else if (m == 2)
	{
		switch (a)
		{
		case 0:
			strcpy(s, "pointer event as list");
			break;
		case 1:
			strcpy(s, "gesture event as list");
			break;
		}
	}
}

//##########################################################
//
//##########################################################
void *multitouch_new(t_symbol *s, long argc, t_atom *argv)
{
	t_multitouch *x = NULL;
	if ((x = (t_multitouch *)object_alloc(multitouch_class)))
	{
		x->m_oldproc = NULL;
		x->m_hwnd = NULL;

		if (argc > 0) {
			if ((argv)->a_type == A_SYM)
			{
				x->m_wincaption = atom_getsym(argv);
			}
			else
			{
				object_error((t_object *)x, "Forbidden argument");
			}
		}

		x->m_outlet_gesture = outlet_new(x, NULL);
		x->m_outlet_touch = outlet_new(x, NULL);
	}
	return (x);
}

//##########################################################
//
//##########################################################
void multitouch_start(t_multitouch *x)
{
	if (!x->m_oldproc)
	{

		x->m_hwnd = NULL;

		if (x->m_wincaption)
		{
			x->m_hwnd = FindWindowA(NULL, x->m_wincaption->s_name);
			if (!x->m_hwnd)
			{
				object_error((t_object *)x, "Failed to find window with title '%s'", x->m_wincaption->s_name);
			}
		}

		if (!x->m_hwnd)
		{
			// if no argument, use the patcher window
			object_post((t_object *)x, "No (valid) window title supplied, using patcher window");
			x->m_hwnd = main_get_client();
		}

		if (x->m_hwnd)
		{

			////////////////////////////////////////
			// Registers a window as no longer being touch-capable.
			// https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-unregistertouchwindow
			////////////////////////////////////////
			if (!UnregisterTouchWindow(x->m_hwnd))
			{
				object_error((t_object *)x, "UnregisterTouchWindow failed: %ld", GetLastError());
			}

			////////////////////////////////////////
			// Enable/disable gestures
			////////////////////////////////////////
			GESTURECONFIG * pGc = NULL;
			UINT uiGcs = 0;

			if (x->gesture_zoom || x->gesture_pan_horizontal || x->gesture_pan_vertical || x->gesture_rotate || x->gesture_twoFingerTap || x->gesture_pressAndTap)
			{
				uiGcs = 5;
				pGc = malloc(uiGcs * sizeof(GESTURECONFIG));

				// ZOOM
				pGc[0].dwID = GID_ZOOM;
				pGc[0].dwWant = x->gesture_zoom ? GC_ZOOM : 0;
				pGc[0].dwBlock = x->gesture_zoom ? 0 : GC_ZOOM;

				// PAN
				pGc[1].dwID = GID_PAN;
				pGc[1].dwWant = 0;
				pGc[1].dwBlock = 0;

				if (x->gesture_pan_horizontal)
					pGc[1].dwWant |= GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY;
				else
					pGc[1].dwBlock |= GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY;

				if (x->gesture_pan_vertical)
					pGc[1].dwWant |= GC_PAN_WITH_SINGLE_FINGER_VERTICALLY;
				else
					pGc[1].dwBlock |= GC_PAN_WITH_SINGLE_FINGER_VERTICALLY;

				if (x->gesture_pan_with_gutter)
					pGc[1].dwWant |= GC_PAN_WITH_GUTTER;
				else
					pGc[1].dwBlock |= GC_PAN_WITH_GUTTER;

				if (x->gesture_pan_with_inertia)
					pGc[1].dwWant |= GC_PAN_WITH_INERTIA;
				else
					pGc[1].dwBlock |= GC_PAN_WITH_INERTIA;

				// ROTATE
				pGc[2].dwID = GID_ROTATE;
				pGc[2].dwWant = x->gesture_rotate ? GC_ROTATE : 0;
				pGc[2].dwBlock = x->gesture_rotate ? 0 : GC_ROTATE;

				// TWOFINGERTAP
				pGc[3].dwID = GID_TWOFINGERTAP;
				pGc[3].dwWant = x->gesture_twoFingerTap ? GC_TWOFINGERTAP : 0;
				pGc[3].dwBlock = x->gesture_twoFingerTap ? 0 : GC_TWOFINGERTAP;

				// PRESSANDTAP
				pGc[4].dwID = GID_PRESSANDTAP;
				pGc[4].dwWant = x->gesture_pressAndTap ? GC_PRESSANDTAP : 0;
				pGc[4].dwBlock = x->gesture_pressAndTap ? 0 : GC_PRESSANDTAP;

			}
			else
			{
				// disable all gestures
				uiGcs = 1;
				pGc = malloc(uiGcs * sizeof(GESTURECONFIG));
				pGc[0].dwID = 0;
				pGc[0].dwWant = 0;
				pGc[0].dwBlock = GC_ALLGESTURES;
			}

			BOOL bResult = SetGestureConfig(x->m_hwnd, 0, uiGcs, pGc, sizeof(GESTURECONFIG));
			if (!bResult) {
				object_error((t_object *)x, "SetGestureConfig failed: %ld", GetLastError());
			}

			if (pGc)
			{
				free(pGc);
			}

			if (SetProp(x->m_hwnd, "multitouch", x))
			{
				x->m_oldproc = (WNDPROC)SetWindowLongPtr(x->m_hwnd, GWLP_WNDPROC, (LONG_PTR)MyWndProc);
				if (!x->m_oldproc)
				{
					object_error((t_object *)x, "SetWindowLongPtr failed");
				}
				else
				{
					// SUCCESS
					object_post((t_object *)x, "Started listening to touch events.");
				}
			}
			else
			{
				object_error((t_object *)x, "SetProp failed");
			}
		}
		else
		{
			object_error((t_object *)x, "Failed to get HWND");
		}
	}
	//else
	//{
	//	object_error((t_object *)x, "Already running");
	//}
}

//##########################################################
//
//##########################################################
void multitouch_stop(t_multitouch *x)
{
	if (x->m_oldproc && x->m_hwnd)
	{
		////////////////////////////////////////
		// Registers a window as being touch-capable.
		// https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-registertouchwindow
		////////////////////////////////////////
		if (!RegisterTouchWindow(x->m_hwnd, 0)) // TWF_FINETOUCH?
		{
			object_error((t_object *)x, "RegisterTouchWindow failed: %ld", GetLastError());
		}

		(WNDPROC)SetWindowLongPtr(x->m_hwnd, GWLP_WNDPROC, (LONG_PTR)x->m_oldproc);
		x->m_oldproc = NULL;
		x->m_hwnd = NULL;

		object_post((t_object *)x, "Stopped listening to touch events.");
	}
	//else
	//{
	//	object_error((t_object *)x, "Not running");
	//}
}

//##########################################################
//
//##########################################################
LRESULT CALLBACK MyWndProc(HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam)
{
	t_multitouch *x = (t_multitouch*)GetProp(hwnd, "multitouch");
	if (!x || !x->m_oldproc)
	{
		return DefMDIChildProc(hwnd, umsg, wParam, lParam);
	}

	switch (umsg)
	{
		case WM_POINTERUPDATE:
		case WM_POINTERDOWN:
		case WM_POINTERUP:
		//case WM_POINTERENTER:
		//case WM_POINTERLEAVE:
		{

			UINT32 pointerId = GET_POINTERID_WPARAM(wParam);

			// check type
			POINTER_INPUT_TYPE pointerType;
			if (!GetPointerType(pointerId, &pointerType))
			{
				object_error((t_object *)x, "GetPointerInfo returned error: %ld", GetLastError());
				pointerType = PT_POINTER; // default to PT_POINTER
			}

			if (pointerType == PT_TOUCH)
			{
				POINTER_INFO pointerInfo = { 0 };

				// Get frame id from current message
				if (GetPointerInfo(pointerId, &pointerInfo))
				{
					if (pointerInfo.pointerFlags & (POINTER_FLAG_CANCELED | POINTER_FLAG_CAPTURECHANGED))
					{
						//post("POINTER_FLAG_CANCELED");
					}
					else if (umsg==WM_POINTERUPDATE && !(pointerInfo.pointerFlags & POINTER_FLAG_INCONTACT))
					{
						//post("NOT POINTER_FLAG_INCONTACT");
					}
					else
					{

						POINT p = pointerInfo.ptPixelLocation;
						ScreenToClient(hwnd, &p);
						t_atom myList[4]; // 8
						atom_setlong(myList + 0, (t_atom_long)(umsg - 0x0245));
						atom_setlong(myList + 1, (t_atom_long)pointerInfo.pointerId);
						atom_setlong(myList + 2, (t_atom_long)p.x);
						atom_setlong(myList + 3, (t_atom_long)p.y);
						outlet_list(x->m_outlet_touch, 0L, 4, (t_atom *)&myList);
					}
				}
				else
				{
					object_error((t_object *)x, "GetPointerInfo returned error: %ld", GetLastError());
				}
			}
			else
			{
				object_error((t_object *)x, "Not a touch event: %ld", pointerType);
			}
		}

		// HACK!
		CallWindowProc(x->m_oldproc, hwnd, umsg, wParam, lParam); // returns 0

		return DefMDIChildProc(hwnd, umsg, wParam, lParam);
		break;

		case WM_GESTURE:
		{
			GESTUREINFO gestureInfo = { 0 };
			gestureInfo.cbSize = sizeof(gestureInfo);
			BOOL bResult = GetGestureInfo((HGESTUREINFO)lParam, &gestureInfo);

			if (!bResult)
			{
				DWORD err = GetLastError();
				object_error((t_object *)x, "GetGestureInfo returned error: %ld", err);
			}
			else
			{

				////////////////////////////////////////
				// In order to enable legacy support, messages with the GID_BEGIN and GID_END gesture
				// commands need to be forwarded using DefWindowProc.
				////////////////////////////////////////
				if (gestureInfo.dwID == GID_BEGIN || gestureInfo.dwID == GID_END)
				{
					CloseGestureInfoHandle((HGESTUREINFO)lParam);
					return DefWindowProc(hwnd, umsg, wParam, lParam);
				}

				if (gestureInfo.dwID != GID_BEGIN && gestureInfo.dwID != GID_END)
				{
					//GID_ZOOM 	3 	Indicates zoom start, zoom move, or zoom stop.The first GID_ZOOM command message begins a zoom but does not cause any zooming.The second GID_ZOOM command triggers a zoom relative to the state contained in the first GID_ZOOM.
					//GID_PAN 	4 	Indicates pan move or pan start.The first GID_PAN command indicates a pan start but does not perform any panning.With the second GID_PAN command message, the application will begin panning.
					//GID_ROTATE 	5 	Indicates rotate move or rotate start.The first GID_ROTATE command message indicates a rotate move or rotate start but will not rotate.The second GID_ROTATE command message will trigger a rotation operation relative to state contained in the first GID_ROTATE.
					//GID_TWOFINGERTAP 	6 	Indicates two - finger tap gesture.
					//GID_PRESSANDTAP 	7 	Indicates the press and tap gesture.

					t_atom myList[5];
					atom_setlong(myList + 0, (t_atom_long)gestureInfo.dwID-3); // 0..4

					atom_setlong(myList + 1, (t_atom_long)gestureInfo.dwFlags);

					if (gestureInfo.dwID == GID_ROTATE)
					{
						atom_setlong(myList + 2, (t_atom_long)(GID_ROTATE_ANGLE_FROM_ARGUMENT(gestureInfo.ullArguments) * -180 / M_PI));
					}
					else
					{
						atom_setlong(myList + 2, (t_atom_long)gestureInfo.ullArguments);
					}

					POINT p = { gestureInfo.ptsLocation.x, gestureInfo.ptsLocation.y };
					ScreenToClient(hwnd, &p);
					atom_setlong(myList + 3, (t_atom_long)p.x);
					atom_setlong(myList + 4, (t_atom_long)p.y);

					outlet_list(x->m_outlet_gesture, 0L, 5, (t_atom *)&myList);

					// consume?
					//CloseGestureInfoHandle((HGESTUREINFO)lParam);
					//return 0;
				}

				CloseGestureInfoHandle((HGESTUREINFO)lParam);
			}
		}
		break;

	}

	return CallWindowProc(x->m_oldproc, hwnd, umsg, wParam, lParam);
}
