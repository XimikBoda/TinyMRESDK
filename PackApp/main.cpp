#include <iostream>
#include <elfio/elfio.hpp>
#include <cmdparser.hpp>

using namespace std;
using namespace ELFIO;


int main(int argc, char** argv)
{
	cli::Parser parser(argc, argv);

	{
		parser.set_required<std::string>("a", "axf", "Elf file from the compiler");
		parser.set_required<std::string>("r", "res", "Resource File");
	}

	parser.run_and_exit_if_error();

	string axf_path = parser.get<std::string>("a");

	elfio reader;

	if (!reader.load(axf_path)) {
		std::cout << "Can't find or process ELF file " << axf_path << std::endl;
		return 1;
	}

}


