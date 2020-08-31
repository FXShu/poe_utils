#ifndef __MACRO_IO_HH
#define __MACRO_IO_HH

struct keyboard {
#ifdef _WIN32
	WPARAM event; /* uint32 */
	LPARAM info; /* long */
#else /* _WIN32 */
	u32 event;
	void *info;
#endif /* _WIN32 */
};
#endif /* __MACRO_IO_HH */
