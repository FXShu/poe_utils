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

std::string version("2.0.0");
DWORD WINAPI send_terminal(void *id) {
	Sleep(500);
	PostThreadMessage((DWORD)id, POE_MESSAGE_TERMINAL, 0, 0);
	return 0;
}

void help(void) {
	std::cout << "[Usage]:" << std::endl;
	std::cout << "\tmanagement.exe [-f <database name>] [-d <log_level>]..." << std::endl;
	std::cout << "\t\t-f = database file specific (json format)" << std::endl;
	std::cout << "\t\t-d = increase debugging verbosity, must be 0 ~ 4" << std::endl;
	std::cout << "\t\t-h = print this helping information" << std::endl;
	std::cout << "\t\t-v = print the current version information" << std::endl;
}

void welcome(void) {
	std::cout << "===============================================================" << std::endl;
	std::cout << "                MacroSim - Configurable Macro Engine" << std::endl;
	std::cout << " Version:       " << version << std::endl;
	std::cout << " Author:        [FX Shu, KT Li]" << std::endl;
	std::cout << " Purpose:       A lightweight and flexible CLI tool that lets users" << std::endl;
	std::cout << "                define and run custom macros to simulate keyboard" << std::endl;
	std::cout << "                and mouse events. Automate tasks, test inputs, or" << std::endl;
	std::cout << "                script your environment with ease.\n" << std::endl;
	std::cout << " Repository:    https://github.com/FXShu/poe_utils\n" << std::endl;
	std::cout << " Get started by loading your macro configuration file." << std::endl;
	std::cout << " Type '-h' to view available options and commands.\n" << std::endl;
	std::cout << "===============================================================" << std::endl;
}

int main(int argc, char **argv) {
	try {
		char c;
		char file[FILE_NAME_MAX];
		/* intercepte windows system event. */
		loglevel = loglevel_e::MSG_ERROR;
		auto queue = std::make_shared<ThreadsafeQueue<struct instruction_event>>();
		auto mtx = std::make_shared<std::mutex>();
		auto cv = std::make_shared<std::condition_variable>();
		auto ready = std::make_shared<bool>(false);
		informer::Ptr informer = informer::init(queue, mtx, cv, ready);
		strcpy(file, DEFAULT_FILE);
		welcome();
		for (;;) {
			c = getopt(argc, argv, "d:f:hv");
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
			case 'v':
				std::cout << version << std::endl;
				exit(EXIT_SUCCESS);
			break;
			default:
				help();
				exit(EXIT_FAILURE);
			}
		}

		macro_supervisor::Ptr macro_owner =
			macro_supervisor::createNew("macro owner", queue, mtx, cv, ready);
		boost::property_tree::ptree root;
		boost::property_tree::ptree macro_root;
		boost::property_tree::read_json(file, root);
		macro_root = root.get_child("poe_database");
		macro_owner->deploy(macro_root);
		boost::property_tree::ptree tree;
		macro_owner->work();
#if 0
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
#endif
		poe_log(MSG_INFO, "manager") << "Set all of macros into active state";
		macro_status status = {
			.name = nullptr,
			.status = MACRO_FLAGS_ACTIVE
		};
		struct instruction_event event;
		event.topic = MARCO_STATUS_BOARDCAST;
		event.context = std::shared_ptr<void>(&status);
		if (macro_owner->publish(event))
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
