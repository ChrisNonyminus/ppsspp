#include <string>
class ManagedWrapper
{
public:
	static unsigned char peekbyte(long long addr);
	static void pokebyte(long long addr, unsigned char val);
	static void savesavestate(std::string filename);
	static void loadsavestate(std::string filename);
	static std::string getstatepath();
};
