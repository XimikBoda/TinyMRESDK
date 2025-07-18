#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <elfio/elfio.hpp>
#include <cmdparser.hpp>
#include <openssl/md5.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

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
	"SIM_card",
	"SMS_person",
	"SMS_SP",
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

map<string, int> filetypes = {
	{"vxp (ADS)"s, 0},
	{"vsm (ADS)"s, 1},
	{"svxp (ADS)"s, 5},
	{"vxp (RVCT)"s, 2},
	{"vsm (RVCT)"s, 3},
	{"vso (RVCT)"s, 4},
	{"vxp (GCC)"s, 6},
	{"vso (GCC)"s, 7},
	{"vsm (GCC)"s, 8}
};

bool load_file_to_vector(const string& path, vector<byte_t>& buf);
void add_vector(vector<byte_t>& a, const vector<byte_t>& b);
void fix_res_offsets(vector<byte_t>& buf, size_t res_offset, size_t res_size);
void add_tag(vector<byte_t>& buf, int32_t id, const vector<byte_t>& data);
vector<byte_t> string_to_vector(string str);
vector<byte_t> string_with_length_to_vector(string name); // length (4 bytes) + str
vector<byte_t> mstring_to_vector(string name);
vector<byte_t> int_to_vector(uint32_t num);
vector<byte_t> api_names_to_vector(string apis);
void add_end(vector<byte_t>& buf, int32_t tags_pos, vector<byte_t> signature);

int main(int argc, char** argv)
{
	//Parsing comand line
	cli::Parser parser(argc, argv);

	{
		parser.set_required<string>("a", "axf", "Elf (or dll) file from the compiler");
		parser.set_required<string>("r", "res", "Resource File");
		parser.set_required<string>("o", "out", "Out file");
		parser.set_required<string>("tdn", "tag-develop-name" "Name of developer for tags");
		parser.set_required<string>("tn", "tag-name", "Name of app for tags");
		parser.set_optional<string>("crt", "cert", "", "Cert file (must be setup correct certid and non-negative appid)");
		parser.set_optional<string>("ti", "tag-imsi", "", "Name of app for tags");
		parser.set_optional<string>("tapi", "tag-api", "File SIM card ProMng", "List of required APIs (Audio Camera Call TCP File HTTP Sensor SIM card Record SMS(person) SMS(SP) BitStream Contact LBS MMS ProMng SMSMng Video XML Sec SysStorage Payment BT PUSH UDP SysFile)");
		parser.set_optional<string>("ty", "tag-type", "vxp", "Type of file (vxp, vso, vsm, svxp)");
		parser.set_optional<string>("tc", "tag-compiler", "GCC", "Type of used compiler (GCC, RVCT, ADS)");
		parser.set_optional<int>("tr", "tag-ram", 512, "Ram size application required (in KB) for tags");
		parser.set_optional<int>("tb", "tag-background", 0, "App can work background? for tags");
		parser.set_optional<int>("tai", "tag-appid", -1, "App id, special values: -1 is personal (imsi), 0 is in developing");
		parser.set_optional<int>("tci", "tag-certid", 1, "Cert id use from you cert, 1 is personal (imsi)");
	}

	parser.run_and_exit_if_error();

	//Files patchs
	string axf_path = parser.get<string>("a");
	string res_path = parser.get<string>("r");
	string out_path = parser.get<string>("o");
	string cert_path = parser.get<string>("crt");

	//Tags parametrs
	string tag_develop_name = parser.get<string>("tdn");
	string tag_name = parser.get<string>("tn");
	string tag_imsi = parser.get<string>("ti");
	string tag_api = parser.get<string>("tapi");
	string tag_type = parser.get<string>("ty");
	string tag_compiler = parser.get<string>("tc");
	int tag_ram = parser.get<int>("tr");
	int tag_background = parser.get<int>("tb");
	int tag_appid = parser.get<int>("tai");
	int tag_certid = parser.get<int>("tci");
	int tag_filetype = 6;

	{
		auto filetype = filetypes.find(tag_type + " (" + tag_compiler + ")");
		if (filetype == filetypes.end()) {
			cout << "Wrong tag type or tag compiler" << endl;
			return 1;
		}
		tag_filetype = filetype->second;
	}

	cout << "Develop name: " << tag_develop_name << '\n';
	cout << "App name: " << tag_name << '\n';
	cout << "App apis: " << tag_api << '\n';
	cout << "App ram size (in kb): " << tag_ram << '\n';
	cout << "App background: " << (tag_background ? "true" : "false") << '\n';
	cout << "App type: " << tag_type << " (" << tag_filetype << ")" << '\n';

	vector<byte_t> full_file_buf;

	//Open res file
	vector<byte_t> res_buf;
	if (!load_file_to_vector(res_path, res_buf)) {
		cout << "Can't find or open res file " << res_path << endl;
		return 1;
	}

	if (axf_path.length() < 4) {
		cout << "axf (or dll) file path to short " << axf_path << endl;
		return 1;
	}

	string axf_extension = axf_path.substr(axf_path.length() - 4, 4);

	if (axf_extension == ".axf"s) { //Open axf file
		elfio reader;

		if (!reader.load(axf_path)) {
			cout << "Can't find or process axf file " << axf_path << endl;
			return 1;
		}

		//Add res section
		section* note_sec = reader.sections.add(".vm_res");
		note_sec->set_type(SHT_PROGBITS);
		note_sec->set_address(0);
		note_sec->set_flags(SHF_ALLOC);
		note_sec->append_data((char*)res_buf.data(), res_buf.size());

		//Getting buffer
		{
			stringstream ss;
			reader.save(ss);

			full_file_buf.resize(ss.str().size());
			memcpy(full_file_buf.data(), ss.str().c_str(), ss.str().size());
		}

		//Fix res offsets
		fix_res_offsets(full_file_buf, note_sec->get_offset(), note_sec->get_size());
	}
	else if (axf_extension == ".dll"s) {// open dll
		if (!load_file_to_vector(axf_path, full_file_buf)) {
			cout << "Can't find or open dll file " << axf_path << endl;
			return 1;
		}
		size_t res_pos = full_file_buf.size();

		add_vector(full_file_buf, string_to_vector(".vm_res"));
		add_vector(full_file_buf, res_buf);
		add_vector(full_file_buf, int_to_vector(res_pos)); // ".vm_res" offset

		fix_res_offsets(full_file_buf, res_pos + 8, res_buf.size()); // 8 is ".vm_res\0"

		if (tag_type == "vsm")
			tag_filetype = 5; // IDK why
	}

	int32_t tag_pos = full_file_buf.size();

	//Add tags
	add_tag(full_file_buf, 0x01, string_to_vector(tag_develop_name));			//developer name
	add_tag(full_file_buf, 0x02, int_to_vector(tag_appid));						//app id
	add_tag(full_file_buf, 0x03, int_to_vector(tag_certid));					//key id
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
	add_tag(full_file_buf, 0x21, int_to_vector(tag_filetype));					//file type
	add_tag(full_file_buf, 0x22, int_to_vector(0));								//is zipped 
	add_tag(full_file_buf, 0x23, int_to_vector(0));								//using UCS2 in tags
	add_tag(full_file_buf, 0x25, int_to_vector(0));								//system file max size?
	add_tag(full_file_buf, 0x29, int_to_vector(0));								//is not use screen
	add_tag(full_file_buf, 0x00, vector<byte_t>());								//end of tags

	vector<byte_t> signature(64);

	if (tag_appid != -1 && tag_certid != 1) {
		cout << "Creating signature\n";

		byte_t double_hash[32], hash_of_double_hash[16];

		MD5(full_file_buf.data(), tag_pos, double_hash);
		MD5(full_file_buf.data() + tag_pos, full_file_buf.size() - tag_pos, double_hash + 16);
		MD5(double_hash, 32, hash_of_double_hash);

		FILE* fp = fopen(cert_path.c_str(), "rb");
		if (!fp) {
			cout << "Can't open cert file " << res_path << endl;
			return 1;
		}
		RSA* rsa = PEM_read_RSAPrivateKey(fp, NULL, NULL, NULL);
		ERR_print_errors_fp(stderr);
		fclose(fp);

		if (!rsa) {
			cout << "Can't init RSA key" << endl;
			return 1;
		}

		if (RSA_private_encrypt(16, hash_of_double_hash, signature.data(), rsa, RSA_PKCS1_PADDING) < 0) {
			cout << "Can't encrypt" << endl;
			return 1;
		}
		RSA_free(rsa);
	}

	//Add end
	add_end(full_file_buf, tag_pos, signature);

	ofstream out(out_path, ios_base::binary);
	if (!out.good())
		return 1;
	out.write((char*)full_file_buf.data(), full_file_buf.size());
	out.close();
}

bool load_file_to_vector(const string& path, vector<byte_t>& buf) {
	ifstream file(path, ios::in | ios::binary | ios::ate);
	if (!file.is_open()) {
		return 0;
	}
	size_t size = (size_t)file.tellg();
	file.seekg(0, ios::beg);
	buf.resize(size);
	file.read((char*)buf.data(), size);
	file.close();
	return 1;
}

void fix_res_offsets(vector<byte_t>& buf, size_t res_offset, size_t res_size)
{
	size_t pos = res_offset;

	while (true) {
		string name((char*)buf.data() + pos);
		pos += name.size() + 1;

		if (name.size() == 0)
			break;


		int32_t& offset = *(int32_t*)(buf.data() + pos);
		pos += 4;

		int32_t size = *(int32_t*)(buf.data() + pos);
		pos += 4;

		offset += res_offset;
	}

	int32_t& res2_offset = *(int32_t*)(buf.data() + pos);
	res2_offset += res_offset;

	pos = res2_offset;

	while (true) {
		int32_t id = *(int32_t*)(buf.data() + pos);
		pos += 4;

		int32_t& offset = *(int32_t*)(buf.data() + pos);
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

void add_tag(vector<byte_t>& buf, int32_t id, const vector<byte_t>& data)
{
	vector<byte_t> tag(8);
	tag.resize(8);
	*(int32_t*)(tag.data()) = id;
	*(int32_t*)(tag.data() + 4) = data.size();
	add_vector(buf, tag);
	add_vector(buf, data);
}

vector<byte_t> string_to_vector(string str)
{
	vector<byte_t> vstr(str.size() + 1);
	memcpy(vstr.data(), str.c_str(), str.size() + 1);
	return vstr;
}

vector<byte_t> string_with_length_to_vector(string name)
{
	vector<byte_t> str(4 + name.size() + 1);
	memcpy(str.data() + 4, name.c_str(), name.size() + 1);
	*(int32_t*)(str.data()) = name.size() + 1;
	return str;
}

vector<byte_t> int_to_vector(uint32_t num)
{
	vector<byte_t> n(4);
	*(uint32_t*)(n.data()) = num;
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
		add_vector(str, int_to_vector(i + 1));
		add_vector(str, string_with_length_to_vector(name));
	}

	return str;
}

void add_end(vector<byte_t>& buf, int32_t tags_pos, vector<byte_t> signature)
{
	signature.resize(64);

	add_vector(buf, { 0xB4, 0x56, 0x44, 0x45, 0x31, 0x30, 0x01, 0x00, 0x00, 0x00 }); // const
	add_vector(buf, signature);
	add_vector(buf, int_to_vector(tags_pos));
	add_vector(buf, vector<byte_t>(8));
}
