#include <iostream>
#include <fstream>
#include <sstream>
#include <elfio/elfio.hpp>
#include <cmdparser.hpp>

using namespace std;
using namespace ELFIO;

void fix_res_offsets(vector<byte>& buf, size_t res_offset, size_t res_size);
void add_tag(vector<byte>& buf, long id, const vector<byte>& data);
vector<byte> string_to_vector(string name);
vector<byte> mstring_to_vector(string name);
vector<byte> int_to_vector(unsigned long num);
void add_end(vector<byte>& buf, long tags_pos);

int main(int argc, char** argv)
{
	//Parsing comand line
	cli::Parser parser(argc, argv);

	{
		parser.set_required<std::string>("a", "axf", "Elf file from the compiler");
		parser.set_required<std::string>("r", "res", "Resource File");
		parser.set_required<std::string>("o", "out", "Out file");
	}

	parser.run_and_exit_if_error();

	string axf_path = parser.get<std::string>("a");
	string res_path = parser.get<std::string>("r");
	string out_path = parser.get<std::string>("o");

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

	//Fix res offsets
	fix_res_offsets(full_file_buf, note_sec->get_offset(), note_sec->get_size());

	long tag_pos = full_file_buf.size();

	//Add tags
	add_tag(full_file_buf, 0x01, string_to_vector("Develop name"));
	add_tag(full_file_buf, 0x02, int_to_vector(-1));
	add_tag(full_file_buf, 0x03, int_to_vector(1));
	add_tag(full_file_buf, 0x04, string_to_vector("App name"));
	add_tag(full_file_buf, 0x05, int_to_vector(0x010000));
	add_tag(full_file_buf, 0x0F, int_to_vector(512));
	add_tag(full_file_buf, 0x10, int_to_vector(0));
	add_tag(full_file_buf, 0x11, int_to_vector(0x9C400000));
	add_tag(full_file_buf, 0x12, string_to_vector("1234567890"));
	add_tag(full_file_buf, 0x16, int_to_vector(1));
	add_tag(full_file_buf, 0x18, int_to_vector(0));
	//add_tag(full_file_buf, 0x19, mstring_to_vector("App name"));
	add_tag(full_file_buf, 0x1C, int_to_vector(0));
	add_tag(full_file_buf, 0x21, int_to_vector(6));
	add_tag(full_file_buf, 0x22, int_to_vector(0));
	add_tag(full_file_buf, 0x23, int_to_vector(0));
	add_tag(full_file_buf, 0x25, int_to_vector(0));
	add_tag(full_file_buf, 0x29, int_to_vector(0));
	add_tag(full_file_buf, 0x00, vector<byte>());

	//Add end
	add_end(full_file_buf, tag_pos);

	ofstream out(out_path, ios_base::binary);
	if (!out.good())
		return 0;
	out.write((char*)full_file_buf.data(), full_file_buf.size());
	out.close();
}

void fix_res_offsets(vector<byte>& buf, size_t res_offset, size_t res_size)
{
	size_t pos = res_offset;

	while (true) {
		string name((char*)buf.data() + pos);
		pos += name.size() + 1;

		if (name.size() == 0)
			break;


		long& offset = *(long*)(buf.data() + pos);
		pos += 4;

		long size = *(long*)(buf.data() + pos);
		pos += 4;

		offset += res_offset;
	}

	long& res2_offset = *(long*)(buf.data() + pos);
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

void add_vector(vector<byte>& a, const vector<byte>& b)
{
	a.insert(a.end(), b.begin(), b.end());
}

void add_tag(vector<byte>& buf, long id, const vector<byte>& data)
{
	vector<byte> tag(8);
	tag.resize(8);
	*(long*)(tag.data()) = id;
	*(long*)(tag.data() + 4) = data.size();
	add_vector(buf, tag);
	add_vector(buf, data);
}

vector<byte> string_to_vector(string name)
{
	vector<byte> str(4 + name.size() + 1);
	memcpy(str.data() + 4, name.c_str(), name.size() + 1);
	*(long*)(str.data()) = name.size() + 1;
	return str;
}

vector<byte> int_to_vector(unsigned long num)
{
	vector<byte> n(4);
	*(unsigned long*)(n.data()) = num;
	return n;
}

vector<byte> mstring_to_vector(string name)
{
	vector<byte> str;
	for (int i = 0; i < 3; ++i) {
		add_vector(str, int_to_vector(i+1));
		add_vector(str, string_to_vector(name));
	}
		
	return str;
}

void add_end(vector<byte>& buf, long tags_pos)
{
	vector<byte> end(0x56);
	unsigned char st_bytes[7] = { 0xB4, 0x56, 0x44, 0x45, 0x31, 0x30, 0x1 };
	memcpy(end.data(), st_bytes, 7);
	*(long*)(end.data() + 0x56 - 12) = tags_pos;
	add_vector(buf, end);
}
