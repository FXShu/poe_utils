#ifdef _WIN32
#define _WIN32_WINNT 0x0600
#endif /* _WIN32 */
#define DPP_NO_DEPRECATED ON
#include <dpp/dpp.h>


const std::string BOT_TOKEN = "MTM4ODc0NDU0MTQ4NTY2NjM0NA.GdBfha.BVwJjIDRC0miXOOkHaPQGKmgc82LnVzKEB1cAw";
const dpp::snowflake ANNOUNCE_ID = 1388748000137838635;

int main(int argc, char **argv) {
	dpp::cluster bot(BOT_TOKEN, dpp::i_default_intents | dpp::i_message_content);

	std::cout << "hello world" << std::endl; 
	bot.on_log(dpp::utility::cout_logger());

	bot.on_ready([&bot](const dpp::ready_t &event) {
		std::cout << "Bot is ready!" << std::endl;
		bot.message_create(dpp::message(ANNOUNCE_ID, "test for the event notificaition"));
	});

	bot.start(dpp::st_wait);
}
