#include <iostream>
#include <fstream>
#include <sstream>
#include <elfio/elfio.hpp>
#include <cmdparser.hpp>

typedef unsigned char byte_t;
using namespace std;
using namespace ELFIO;

byte_t all_ids[208] = {
	0x88, 0x13, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x89, 0x13, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x8A, 0x13, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x8B, 0x13, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x8C, 0x13, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x8D, 0x13, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x8E, 0x13, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x8F, 0x13, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x90, 0x13, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x91, 0x13, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x92, 0x13, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x93, 0x13, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x94, 0x13, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x95, 0x13, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x96, 0x13, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x97, 0x13, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x98, 0x13, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x99, 0x13, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x9A, 0x13, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x9B, 0x13, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x9C, 0x13, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x9D, 0x13, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x9E, 0x13, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x9F, 0x13, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0xA0, 0x13, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA1, 0x13, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00
};


string api_names[26] =
{
	"Audio",
	"Call",
	"Camera",
	"File",
	"HTTP",
	"Record",
	"Sensor",
	"SIM card",
	"SMS(person)",
	"SMS(SP)",
	"TCP",
	"SysStorage",
	"Sec",
	"BitStream",
	"Contact",
	"LBS",
	"MMS",
	"ProMng",
	"SMSMng",
	"Video",
	"XML",
	"Payment",
	"SysFile",
	"BT",
	"UDP",
	"PUSH"
};


void fix_res_offsets(vector<byte_t>& buf, size_t res_offset, size_t res_size);
void add_tag(vector<byte_t>& buf, long id, const vector<byte_t>& data);
vector<byte_t> string_to_vector(string name);
vector<byte_t> mstring_to_vector(string name);
vector<byte_t> int_to_vector(unsigned long num);
vector<byte_t> api_names_to_vector(string apis);
void add_end(vector<byte_t>& buf, long tags_pos);

int main(int argc, char** argv)
{
	//Parsing comand line
	cli::Parser parser(argc, argv);

	{
		parser.set_required<std::string>("a", "axf", "Elf file from the compiler");
		parser.set_required<std::string>("r", "res", "Resource File");
		parser.set_required<std::string>("o", "out", "Out file");
		parser.set_optional<std::string>("tdn", "tag-develop-name", "No name", "Name of developer for tags");
		parser.set_optional<std::string>("tn", "tag-name", "No name", "Name of app for tags");
		parser.set_optional<std::string>("ti", "tag-imsi", "", "Name of app for tags");
		parser.set_optional<std::string>("tapi", "tag-api", "File SIM card ProMng", "List of required APIs (Audio Camera Call TCP File HTTP Sensor SIM card Record SMS(person) SMS(SP) BitStream Contact LBS MMS ProMng SMSMng Video XML Sec SysStorage Payment BT PUSH UDP SysFile)");
		parser.set_optional<int>("tr", "tag-ram", 512, "Ram size application required (in KB) for tags");
		parser.set_optional<int>("tb", "tag-background", 0, "App can work background? for tags");
	}

	parser.run_and_exit_if_error();

	//Files patchs
	string axf_path = parser.get<std::string>("a");
	string res_path = parser.get<std::string>("r");
	string out_path = parser.get<std::string>("o");

	//Tags parametrs
	string tag_develop_name = parser.get<std::string>("tdn");
	string tag_name = parser.get<std::string>("tn");
	string tag_imsi = parser.get<std::string>("ti");
	string tag_api = parser.get<std::string>("tapi");
	int tag_ram = parser.get<int>("tr");
	int tag_background = parser.get<int>("tb");

	//Open axf file
	elfio reader;

	if (!reader.load(axf_path)) {
		std::cout << "Can't find or process axf file " << axf_path << std::endl;
		return 1;
	}

	//Open res file
	vector<byte_t> res_buf;
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
	vector<byte_t> full_file_buf;
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
	add_tag(full_file_buf, 0x01, string_to_vector(tag_develop_name));			//developer name
	add_tag(full_file_buf, 0x02, int_to_vector(-1));							//app id
	add_tag(full_file_buf, 0x03, int_to_vector(1));								//key id
	add_tag(full_file_buf, 0x04, string_to_vector(tag_name));					//app name
	add_tag(full_file_buf, 0x05, int_to_vector(0x010000));						//version
	add_tag(full_file_buf, 0x0F, int_to_vector(tag_ram));						//ram required
	add_tag(full_file_buf, 0x10, int_to_vector(0));								//screan resolution 0=any
	add_tag(full_file_buf, 0x11, int_to_vector(0x9C400000));					//MRE engine version
	add_tag(full_file_buf, 0x12, string_to_vector(tag_imsi));					//IMSI
	add_tag(full_file_buf, 0x13, api_names_to_vector(tag_api));					//APIs 
	add_tag(full_file_buf, 0x16, int_to_vector(1));								//Input mode?
	add_tag(full_file_buf, 0x18, int_to_vector(tag_background));				//background run
	add_tag(full_file_buf, 0x19, mstring_to_vector(tag_name));					//app name in 3 languages
	add_tag(full_file_buf, 0x1C, int_to_vector(0));								//can rotate?
	add_tag(full_file_buf, 0x21, int_to_vector(6));								//file type (6=vxp(GCC))
	add_tag(full_file_buf, 0x22, int_to_vector(0));								//is zipped 
	add_tag(full_file_buf, 0x23, int_to_vector(0));								//using UCS2 in tags
	add_tag(full_file_buf, 0x25, int_to_vector(0));								//system file max size?
	add_tag(full_file_buf, 0x29, int_to_vector(0));								//is not use screen
	add_tag(full_file_buf, 0x00, vector<byte_t>());								//end of tags

	//Add end
	add_end(full_file_buf, tag_pos);

	ofstream out(out_path, ios_base::binary);
	if (!out.good())
		return 0;
	out.write((char*)full_file_buf.data(), full_file_buf.size());
	out.close();
}

void fix_res_offsets(vector<byte_t>& buf, size_t res_offset, size_t res_size)
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

void add_vector(vector<byte_t>& a, const vector<byte_t>& b)
{
	a.insert(a.end(), b.begin(), b.end());
}

void add_tag(vector<byte_t>& buf, long id, const vector<byte_t>& data)
{
	vector<byte_t> tag(8);
	tag.resize(8);
	*(long*)(tag.data()) = id;
	*(long*)(tag.data() + 4) = data.size();
	add_vector(buf, tag);
	add_vector(buf, data);
}

vector<byte_t> string_to_vector(string name)
{
	vector<byte_t> str(4 + name.size() + 1);
	memcpy(str.data() + 4, name.c_str(), name.size() + 1);
	*(long*)(str.data()) = name.size() + 1;
	return str;
}

vector<byte_t> int_to_vector(unsigned long num)
{
	vector<byte_t> n(4);
	*(unsigned long*)(n.data()) = num;
	return n;
}

vector<byte_t> api_names_to_vector(string apis)
{
	vector<byte_t> vec;
	for (int i = 0; i < 26; ++i)
		if (apis.find(api_names[i]) != string::npos) {
			add_vector(vec, int_to_vector(5000 + i));
			add_vector(vec, int_to_vector(1));
		}
	return vec;
}

vector<byte_t> mstring_to_vector(string name)
{
	vector<byte_t> str;
	for (int i = 0; i < 3; ++i) {
		add_vector(str, int_to_vector(i+1));
		add_vector(str, string_to_vector(name));
	}
		
	return str;
}

void add_end(vector<byte_t>& buf, long tags_pos)
{
	vector<byte_t> end= { 0xB4, 0x56, 0x44, 0x45, 0x31, 0x30, 0x1 };
	end.resize(0x56);
	*(long*)(end.data() + 0x56 - 12) = tags_pos;
	add_vector(buf, end);
}
