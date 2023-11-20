#include <iostream>
#include <fstream>
#include <sstream>
#include <elfio/elfio.hpp>
#include <cmdparser.hpp>

using namespace std;
using namespace ELFIO;

void fix_res_offsets(vector<byte> &buf, size_t res_offset, size_t res_size);

int main(int argc, char** argv)
{
	//Parsing comand line
	cli::Parser parser(argc, argv);

	{
		parser.set_required<std::string>("a", "axf", "Elf file from the compiler");
		parser.set_required<std::string>("r", "res", "Resource File");
	}

	parser.run_and_exit_if_error();

	string axf_path = parser.get<std::string>("a");
	string res_path = parser.get<std::string>("r");

	//Open axf file
	elfio reader;

	if (!reader.load(axf_path)) {
		std::cout << "Can't find or process axf file " << axf_path << std::endl;
		return 1;
	}

	//Open res file
	vector<byte> res_buf;
	{
		std::ifstream res_in(res_path, ios::in | ios::binary | ios::ate);
		if (!res_in.is_open()) {
			std::cout << "Can't find or open res file " << res_path << std::endl;
			return 1;
		}
		size_t size = (size_t)res_in.tellg();
		res_in.seekg(0, ios::beg);
		res_buf.resize(size);
		res_in.read((char*)res_buf.data(), size);
		res_in.close();
	}

	//Add res section
	section* note_sec = reader.sections.add(".vm_res");
	note_sec->set_type(SHT_PROGBITS);
	note_sec->set_address(0);
	note_sec->set_flags(SHF_ALLOC);
	note_sec->append_data((char*)res_buf.data(), res_buf.size());

	//Getting buffer
	vector<byte> full_file_buf;
	{
		stringstream ss;
		reader.save(ss);

		full_file_buf.resize(ss.str().size());
		memcpy(full_file_buf.data(), ss.str().c_str(), ss.str().size());
	}

	fix_res_offsets(full_file_buf, note_sec->get_offset(), note_sec->get_size());
}

void fix_res_offsets(vector<byte> &buf, size_t res_offset, size_t res_size)
{
	size_t pos = res_offset;

	while (true) {
		string name((char*)buf.data() + pos);
		pos += name.size() + 1;

		if (name.size() == 0)
			break;


		long &offset = *(long*)(buf.data() + pos);
		pos += 4;

		long size = *(long*)(buf.data() + pos);
		pos += 4;

		offset += res_offset;
	}

	long &res2_offset = *(long*)(buf.data() + pos);
	res2_offset += res_offset;

	pos = res2_offset;

	while (true) {
		long id = *(long*)(buf.data() + pos);
		pos += 4;

		long& offset = *(long*)(buf.data() + pos);
		pos += 4;

		offset += res_offset;
		
		if (id == 0xFFFFFFFF)
			break;
	}
}


