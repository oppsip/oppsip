
#include "./wsHookInterface.h"
#include "./wsLuaHook.h"
#include "./wsServer.h"

int main(int argc, const char* argv[])
{
	wsLuaHook *hook = wsLuaHook::sGetInst();

	wsServer server(hook);

	return 0;
}
