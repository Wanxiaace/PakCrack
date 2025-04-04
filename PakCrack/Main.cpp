#include <iostream>
#include "PakInterface.h"

int main() {
	try {
		sgf::PakInterface::PopcapPak pak("main.pak");
		pak.DumpAllFiles("dump");
	}
	catch (std::exception& e) {
		std::cout << e.what() << std::endl;
	}

	return EXIT_SUCCESS;
}