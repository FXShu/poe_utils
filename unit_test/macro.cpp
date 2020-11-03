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
#if 1
		macro_passive_loop::Ptr macro =
			macro_passive_loop::createNew("macro test", 'Q', 'W', 4000, master);
#else
		macro_passive::Ptr macro = macro_passive::createNew("macro test", 'Q', master);
#endif
		if (!macro)
			exit(EXIT_FAILURE);
		else
			poe_log(MSG_DEBUG, "Macro test") << "create new macro instace success";
		/* Set status of macro to recording */
		macro_status status = {
			.name = "macro test",
			.status = MACRO_FLAGS_RECORD
		};
		if (master->publish(MARCO_STATUS_BOARDCAST, &status))
			exit(EXIT_FAILURE);
		poe_log(MSG_DEBUG, "Macro test") << "force \"macro test\" macro to recording status";
		unsigned long id;
		id = GetCurrentThreadId();
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)send_terminal, (void *)id, 0, &id);
		poe_log(MSG_DEBUG, "Macro test") << "start intcepte windows system message";
		
		if (informer->intercept())
			exit(EXIT_FAILURE);
		macro->show();
		/* name is nullptr for boardcast. */
		status = {
			.name = nullptr,
			.status = MACRO_FLAGS_ACTIVE
		};
		if (master->publish(MARCO_STATUS_BOARDCAST, &status))
			exit(EXIT_FAILURE);
		poe_log(MSG_DEBUG, "Macro test") << "force \"macro test\" macro to execute status";
		if (informer->intercept()) {
			poe_log(MSG_ERROR, "Informer") << "intercept message failed, exit";
			exit(EXIT_FAILURE);
		}
	} catch (std::exception &e) {
		poe_log(MSG_ERROR, "macro test") << e.what();
	}
	exit(EXIT_SUCCESS);
}
