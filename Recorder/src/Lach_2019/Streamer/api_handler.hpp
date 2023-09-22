//
// api_handler.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Martin Lach


#include <string>
#include <dirent.h>
#include "CStreamerController.h"

namespace http {
namespace server {

struct reply;
struct request;

/// The common handler for all incoming requests.
class api_handler
{
public:
	/// Handle a request and produce a reply.
	void handle_request(std::string req, reply& rep, CStreamerController * streamer);

private:
	std::string get_all();
	std::string get_levels(CStreamerController * streamer);
	std::string get_audio_files();
	std::string get_playlist_files();
	std::string get_playlist(std::string req);
	std::string save_playlist(std::string req);
	std::string record(std::string req);
	void set_gain(int index, int value);
	void set_phantom(int index, int value);
	void resolve_set(std::string req, int type);
};

} // namespace server
} // namespace http
