#include <stdlib.h>
#include <exception>
#include "utils_header.hh"

#ifdef _WIN32
#include "informer.hh"
#endif

#include "macro.hh"

loglevel_e loglevel;

DWORD WINAPI send_terminal(void *id) {
	Sleep(500);
	PostThreadMessage((DWORD)id, POE_MESSAGE_TERMINAL, 0, 0);
	return 0;
}

int main(int argc, char **argv) {
	try {
		/* intercepte windows system event. */
		Sleep(500);
		loglevel = loglevel_e::MSG_DEBUG;
		informer::Ptr informer = informer::init();

		observer::Ptr master = observer::createNew();
		if (master->create_session(MARCO_STATUS_BOARDCAST))
			exit(EXIT_FAILURE);
		else
			poe_log(MSG_DEBUG, "Macro test") << "create \"macro status\" session";

		macro_flask::Ptr macro = 
			macro_flask::createNew("macro test", 'Z', 'Z', master);
		if (!macro)
			exit(EXIT_FAILURE);
		else
			poe_log(MSG_DEBUG, "Macro test") << "create new macro instace success";
		macro->add_flask("Silver Flask", '4', 3600);
		macro->add_flask("Atziri's Promise", '5', 7200);
		macro->add_flask("Quicksilver Flask", '6', 3600);
		macro->add_flask("The Wise Oak", '7', 10800);
		macro->add_flask("Eternal Mana Flask", '8', 1800);
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
