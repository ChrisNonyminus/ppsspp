#include "ManagedWrapper.h"
#include "../Core/MemMap.h"
#include "../Common/Common.h"
#include "../Common/CommonTypes.h"
#include "../Common/Swap.h"
#include "../Core/Opcode.h"
#include "../Core/SaveState.h"
#include "VanguardClient.h"
#include <string>
unsigned char ManagedWrapper::peekbyte(long long addr)
{
	return Memory::Read_U8(static_cast<u32>(addr));
}

void ManagedWrapper::pokebyte(long long addr, unsigned char val)
{
	Memory::Write_U8(val, static_cast<u32>(addr));
}

void ManagedWrapper::savesavestate(std::string filename)
{
	SaveState::Save(filename, -1);
}

void ManagedWrapper::loadsavestate(std::string filename)
{
	printf("Savestate file is %s", filename.c_str());
	SaveState::Load(filename, -1);
}

std::string ManagedWrapper::getstatepath()
{
	/*std::string state_file = settings.imgread.ImagePath;
	size_t lastindex = state_file.find_last_of('/');
#ifdef _WIN32
	size_t lastindex2 = state_file.find_last_of('\\');
	if (lastindex == std::string::npos)
		lastindex = lastindex2;
	else if (lastindex2 != std::string::npos)
		lastindex = std::max(lastindex, lastindex2);
#endif
	if (lastindex != std::string::npos)
		state_file = state_file.substr(lastindex + 1);
	lastindex = state_file.find_last_of('.');
	if (lastindex != std::string::npos)
		state_file = state_file.substr(0, lastindex);
	state_file = state_file + ".state";
	return get_writable_data_path(state_file);*/
	return "";
}
