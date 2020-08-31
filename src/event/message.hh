#ifndef __EVENT_MESSAGE_HH
#define __EVENT_MESSAGE_HH

/* session */
#define POE_KEYBOARD_EVENT "poe_keyboard_event"
#define POE_MOUSE_EVENT "poe_mouse_event"
#define POE_MESSAGE_EVENT "poe_message_event"

/* Those messge should be send from control center. */
#ifdef _WIN32
enum {
	POE_MESSAGE_TERMINAL  = (WM_USER + 0x0001),
	POE_MESSAGE_MAXIMUN
};
#endif /* _WIN32 */

#endif /* __EVENT_MESSAGE_HH */
