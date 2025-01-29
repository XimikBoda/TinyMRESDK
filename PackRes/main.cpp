#include <iostream>
#include <fstream>
#include <filesystem>
#include <cmdparser.hpp>
#include <cstring>

typedef unsigned char byte_t;

using namespace std;
namespace fs = filesystem;

bool load_file_to_vector(const string& path, vector<byte_t>& buf);
void add_vector(vector<byte_t>& a, const vector<byte_t>& b);
vector<byte_t> string_to_vector(string str);
vector<byte_t> string_with_length_to_vector(string name); // length (4 bytes) + str
vector<byte_t> int_to_vector(uint32_t num);

struct resourse {
	resourse() = default;
	resourse(const string& path);

	string name;
	vector<byte_t> buf;
	uint32_t offset = 0;
	uint32_t offset_offset = 0;
};

resourse::resourse(const string& path) {
	name = fs::path(path).filename().string();

	if (!load_file_to_vector(path, buf)) {
		cout << "Can't find or open file " << path << endl;
		exit(1);
	}
}

resourse AppLogo_gen() { // TODO
	resourse res;
	res.name = "Applogo.img";
	res.buf = vector<byte_t>({
		0x56, 0x52, 0x45, 0x41, 0x50, 0x50, 0x4C, 0x4F, 0x47, 0x4F, 0x30, 0x39, 0x42, 0x56, 0x52, 0x45, // temp
		0x50, 0x4E, 0x47, 0x08, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	}); // Empty

	return res;
}

resourse mre2_gen() { // TODO
	resourse res;
	res.name = "mre-2.0";
	res.buf = vector<byte_t>({ 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00 }); // Empty
	return res;
}
/*
Res structure:
C-String "AppLogo.img" + offset1 + size1
C-String name2 + offset2 + size2
...
C-String "mre-2.0" + offsetn + sizen
empty string ('\0')
mre-2.0 offsetn (again) + unknow (3-5 bytes)
res1
res2
...
mre-2.0 section
*/

int main(int argc, char** argv)
{
	//Parsing comand line
	cli::Parser parser(argc, argv);
	{
		parser.set_required<string>("o", "out", "Out file");
		parser.set_optional<bool>("e", "empty-logo", false, "Add empty AppLogo.img");
		parser.set_optional<vector<string>>("f", "files", {}, "Paths to files for packing");
	}

	parser.run_and_exit_if_error();

	string out_path = parser.get<string>("o");
	bool elogo = parser.get<bool>("e");
	vector<string> files = parser.get<vector<string>>("f");

	vector<resourse> resourses;
	resourses.reserve(files.size() + 2);

	if (elogo)
		resourses.push_back(AppLogo_gen());

	for (auto& el : files)
		resourses.push_back(resourse(el));

	resourses.push_back(mre2_gen());

	vector<byte_t> out_file;

	for (auto& el : resourses) { // Put names, offset (0), size 
		add_vector(out_file, string_to_vector(el.name)); // res name
		el.offset_offset = out_file.size();
		add_vector(out_file, int_to_vector(0)); // offset
		add_vector(out_file, int_to_vector(el.buf.size())); // size
	}

	add_vector(out_file, { 0x00 }); // empty string

	uint32_t mre2_res_offset_offset = out_file.size();
	add_vector(out_file, int_to_vector(0)); // mre2 offset
	add_vector(out_file, int_to_vector(0)); // reserved

	for (auto& el : resourses) { // Put resurses content and update offsets
		el.offset = out_file.size();
		add_vector(out_file, el.buf); // res content

		memcpy(out_file.data() + el.offset_offset, &el.offset, 4); // update offset
	}

	{ // temp add additional offset point
		memcpy(out_file.data() + mre2_res_offset_offset, &resourses[resourses.size() - 1].offset, 4);
		uint32_t size = resourses.size();
		memcpy(out_file.data() + out_file.size() - 4, &size, 4);
	}

	ofstream out(out_path, ios_base::binary);
	if (!out.good())
		return 1;
	out.write((char*)out_file.data(), out_file.size());
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

void add_vector(vector<byte_t>& a, const vector<byte_t>& b)
{
	a.insert(a.end(), b.begin(), b.end());
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

