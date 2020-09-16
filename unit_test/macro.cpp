#include <stdlib.h>
#include <exception>
#include "utils_header.hh"

#ifdef _WIN32
#include "informer.hh"
#endif

#include "macro.hh"

loglevel_e loglevel;

DWORD WINAPI send_terminal(void *id) {
	Sleep(5000);
	PostThreadMessage((DWORD)id, POE_MESSAGE_TERMINAL, 0, 0);
	return 0;
}

int main(int argc, char **argv) {
	try {
		/* intercepte windows system event. */
		Sleep(500);
		loglevel = loglevel_e::MSG_DEBUG;
		informer::Ptr informer = informer::init();
		poe_log(MSG_DEBUG, "Macro test") << "address of informer " << informer;

		observer::Ptr master = observer::createNew();
		poe_log(MSG_DEBUG, "Macro test") << "address of master observer " << master;
		if (master->create_session(MARCO_STATUS_BOARDCAST))
			exit(EXIT_FAILURE);
		else
			poe_log(MSG_DEBUG, "Macro test") << "create \"macro status\" session";

		macro_passive::Ptr macro = macro_passive::createNew("macro test", 0x41, master);
		if (!macro)
			exit(EXIT_FAILURE);
		else
			poe_log(MSG_DEBUG, "Macro test") << "create new macro instace success";
		/* Set status of macro to recording */
		macro_status status = {
			.name = "macro test",
			.status = 0
		};
		if (master->publish(MARCO_STATUS_BOARDCAST, &status))
			exit(EXIT_FAILURE);
		poe_log(MSG_DEBUG, "Macro test") << "force \"macro test\" macro to recording status";
		unsigned long id;
		id = GetCurrentThreadId();
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)send_terminal, (void *)id, 0, &id);
		if (informer->intercept())
			exit(EXIT_FAILURE);
		else
			poe_log(MSG_DEBUG, "Macro test") << "start intcepte windows system message";
//		WaitForSingleObject(informer->get_threadID(), 10000);
		macro->show();

		status = {
			.name = "macro test",
			.status = 1
		};
		if (master->publish(MARCO_STATUS_BOARDCAST, &status))
			exit(EXIT_FAILURE);
		poe_log(MSG_DEBUG, "Macro test") << "force \"macro test\" macro to execute status";
		if (informer->intercept())
			exit(EXIT_FAILURE);
	} catch (std::exception &e) {
		poe_log(MSG_ERROR, "macro test") << e.what();
	}
	exit(EXIT_SUCCESS);
}
