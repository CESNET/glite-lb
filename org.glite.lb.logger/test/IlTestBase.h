extern "C" {
#include "interlogd.h"
}

class IlTestBase {
public:
	static const char *msg;
	static const char *msg_enc;
	static const struct server_msg smsg;
};
