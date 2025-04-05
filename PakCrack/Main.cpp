#include <iostream>
#include "PakInterface.h"

int main() {
	std::string fucktxt = "FUCK EVERYTHING!!!";
	try {
		sgf::PakInterface::PopcapPak pak("main.pak");
		pak.AddFile("fuck.txt", fucktxt.c_str(), fucktxt.size(),1);
		pak.RemoveFile("images/Almanac.png");
		pak.DumpAllFiles("dump");
	}
	catch (std::exception& e) {
		std::cout << e.what() << std::endl;
	}

	return EXIT_SUCCESS;
}