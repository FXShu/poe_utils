#include <stdlib.h>
#include <unistd.h>
#include <exception>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#ifdef _WIN32
#include "informer.hh"
#endif

#include "utils_header.hh"
#include "macro.hh"
#include "supervisor.hh"

#define FILE_NAME_MAX 64 
#define DEFAULT_FILE "flask_db.json"
loglevel_e loglevel;

DWORD WINAPI send_terminal(void *id) {
	Sleep(500);
	PostThreadMessage((DWORD)id, POE_MESSAGE_TERMINAL, 0, 0);
	return 0;
}

void help(void) {
	std::cout << "[Usage]:" << std::endl;
	std::cout << "\tflask_marco.exe [-f <database name>] [-d <log_level>]..." << std::endl;
	std::cout << "\t\t-f = database file specific (json format)" << std::endl;
	std::cout << "\t\t-d = increase debugging verbosity, must be 0 ~ 4" << std::endl;
	std::cout << "\t\t-h = print this helping information" << std::endl;
}

int main(int argc, char **argv) {
	try {
		char c;
		char file[FILE_NAME_MAX];
		/* intercepte windows system event. */
		loglevel = loglevel_e::MSG_ERROR;
		informer::Ptr informer = informer::init();
		strcpy(file, DEFAULT_FILE);
		for (;;) {
			c = getopt(argc, argv, "d:f:h");
			if (c < 0)
				break;
			switch(c) {
			int level;
			case 'd':
				level = atoi(optarg);
				if (level > MSG_EXCESSIVE || level < MSG_ERROR) {
					poe_log(MSG_ERROR, "Main") << "invalid log level";
					help();
					exit(EXIT_FAILURE);
				}
				loglevel = static_cast<loglevel_e>(level);
			break;
			case 'f':
				if (strlen(optarg) > FILE_NAME_MAX || strlen(optarg) <= 0) {
					poe_log(MSG_ERROR, "Main") << "invalid file name";
					help();
					exit(EXIT_FAILURE);
				}
				strcpy(file, optarg);
				std::cout << "database file: " << file << std::endl;
			break;
			case 'h':
				help();
				exit(EXIT_SUCCESS);
			break;
			default:
				help();
				exit(EXIT_FAILURE);
			}
		}
		macro_supervisor::Ptr macro_owner = macro_supervisor::createNew("macro owner");
		boost::property_tree::ptree root;
		boost::property_tree::ptree macro_root;
		boost::property_tree::read_json(file, root);
		macro_root = root.get_child("poe_database");
		macro_owner->deploy(macro_root);
		boost::property_tree::ptree tree;
		/* Set status of macro to recording */
		macro_status status = {
			.name = "flask_marco",
			.status = MACRO_FLAGS_RECORD
		};
		if (macro_owner->publish(MARCO_STATUS_BOARDCAST, &status))
			exit(EXIT_FAILURE);
		unsigned long id;
		id = GetCurrentThreadId();
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)send_terminal, (void *)id, 0, &id);
		
		if (informer->intercept())
			exit(EXIT_FAILURE);
		/* name is nullptr for boardcast. */
		status = {
			.name = nullptr,
			.status = MACRO_FLAGS_ACTIVE
		};
		if (macro_owner->publish(MARCO_STATUS_BOARDCAST, &status))
			exit(EXIT_FAILURE);
		if (informer->intercept()) {
			poe_log(MSG_ERROR, "Informer") << "intercept message failed, exit";
			exit(EXIT_FAILURE);
		}
	} catch (std::exception &e) {
		poe_log(MSG_ERROR, "macro test") << e.what();
	}
	exit(EXIT_SUCCESS);
}
