// ==================================================
// JKA_YBEProxy by Yberion
// Inspired by JMPProxy from DeathSpike (https://github.com/Deathspike/JMPProxy)
// ==================================================

#include "Proxy_Header.h"

gameImport_t* trap = NULL;
Proxy_t proxy = { 0 };

Q_EXPORT intptr_t vmMain(intptr_t command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4,
	intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11)
{
	switch (command)
	{
		case GAME_INIT:
		{
			// (int levelTime, int randomSeed, int restart)

			char fs_gameBuffer[MAX_OSPATH];
			size_t pathLength = 0;
			size_t tmpLength = 0;

			fs_gameBuffer[0] = 0;

			trap->Cvar_VariableStringBuffer(FS_GAME_CVAR, fs_gameBuffer, sizeof(fs_gameBuffer));

			pathLength = strlen(fs_gameBuffer);

			// length 0 means that fs_game is set to the default value which is the default base game folder name
			if (pathLength == 0)
			{
				pathLength += strlen(DEFAULT_BASE_GAME_FOLDER_NAME);
				memcpy(fs_gameBuffer, DEFAULT_BASE_GAME_FOLDER_NAME, pathLength);
			}

			tmpLength = strlen(PROXY_LIBRARY);
			memcpy(fs_gameBuffer + pathLength, PROXY_LIBRARY, tmpLength);
			pathLength += tmpLength;

			fs_gameBuffer[pathLength] = '\0';

			proxy.jampgameHandle = YbeProxy_OpenLibrary(fs_gameBuffer);

			if (proxy.jampgameHandle == NULL)
			{
				exit(EXIT_FAILURE);
			}
			else
			{
				// Engine -> Load Proxy dllEntry (store the original systemCall function pointer in originalSystemCall) ->
				// Get Original dllEntry -> send our own Proxy systemCall function pointer to the Original dllEntry
				// At the end of our Proxy systemCall function there's a call the Original systemCall function
				proxy.originalDllEntry = (dllEntryFuncPtr_t)YbeProxy_GetFunctionAddress(proxy.jampgameHandle, "dllEntry");
				// Engine -> Proxy vmMain -> Original vmMain
				proxy.originalVmMain = (vmMainFuncPtr_t)YbeProxy_GetFunctionAddress(proxy.jampgameHandle, "vmMain");

				// "Send our own Proxy systemCall function pointer to the Original dllEntry"
				proxy.originalDllEntry(Proxy_systemCall);
			}

			break;
		}
		case GAME_SHUTDOWN:
		{
			//(int restart)

			if (proxy.jampgameHandle)
			{
				// Send the shutdown signal to the original game module and store the response
				proxy.originalVmMainResponse = proxy.originalVmMain(command, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);
				
				// We can close our proxy library
				YbeProxy_CloseLibrary(proxy.jampgameHandle);

				// Return the response of the original game module after the shutdown
				return proxy.originalVmMainResponse;
			}

			break;
		}
		case GAME_CLIENT_CONNECT:
		{
			//(int clientNum, qboolean firstTime, qboolean isBot)

			if ((qboolean)arg1 && !(qboolean)arg2)
			{
				trap->SendServerCommand(arg0, va("print \"^5%s (^7%s^5) %s^7\n\"", YBEPROXY_NAME, YBEPROXY_VERSION, YBEPROXY_BY_AUTHOR));
			}
		}
		default:
			break;
	}

	return proxy.originalVmMain(command, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);
}

// The engine sends the system call function pointer the the game module through dllEntry
Q_EXPORT void dllEntry(systemCallFuncPtr_t systemCallFuncPtdr) {
	proxy.originalSystemCall = systemCallFuncPtdr;

	TranslateSystemcalls();
}