/**
 * INI��ȡ����
 */
#include <iostream>
#include <fstream>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "ini_parser.h"


using namespace std;

KMQ_DECLARATION_START

// ��'['��']'���section���ɹ�����true�ұ���section������ֵ��sq_section�����'['��']'
bool 
ini_parser_t::find_section(const char *p, string &section, string &sq_section, string &sec_body)
{
	if (*p != '[') return false;
	
	// possible section begins
	const char *sec_head = p;
	p++;
	while (*p && *p != '#' && *p != ']') p++;
	if (*p == ']') {
		// possible section found
		const char *sec_end = p;
		p++;
		
		while (*p && isspace(*p)) p++;
		if (*p == '\0' || *p == '#') {
			// a real section found
			if (!sq_section.empty()) {
				// save the old section body
				m_sections[sq_section] = sec_body;
				sec_body.clear();
			}
			sq_section.assign(sec_head, sec_end-sec_head+1);
			sec_head++;
			section.assign(sec_head, sec_end-sec_head);
			return true;
		}
	}
	
	return false;
}

// �Ѵ�p��ʼ��len���ֽ�׷�ӵ�sq_section�����lenΪ-1������Ҫ����p�ĳ���
void 
ini_parser_t::append_sq_section(string &sq_section, string &sec_body, const char *p, int len)
{
	if (len < 0) {
		char *end = (char *)strchr(p, '#');
		if (end != NULL)
			len = end - p;
		else
			len = strlen(p);
	}
	// trim tailing blanks
	for (--len; len>=0 && isspace(p[len]); len--);
	len++;
	
	if (len <= 0) return;
	sec_body.append(p, len);
	sec_body.append("\n");
	/*
	stringMap::iterator pos = m_sections.find(sq_section);
	if (pos == m_sections.end()) {
		m_sections[sq_section] = string(p, len) + "\n";
	} else {
		pos->second.append(p, len);
		pos->second.append("\n");
	}*/
}

// ��p��ʼ������section��"name = value"������һ�е�β��
const char *
ini_parser_t::find_pair(const char *p, string &section)
{
	if (*p == '=') {
		// no name given, goto the end of line
		while (*p && *p != '#') p++;
		return p;
	}
	
	const char *name_head = p;
	p++;
	while (*p && *p != '#' && *p != '=') p++;
	if (*p == '#' || *p == '\0')
		return p;
	
	if (*p == '=') {
		// name found
		const char *name_end = p - 1;
		p++;
		
		while (name_end > name_head && isspace(*name_end)) name_end--;
		name_end++;
		
		if (name_end == name_head) {
			// goto the end of line
			while (*p && *p != '#') p++;
			return p;
		}
		
		// save the name
		string name(name_head, name_end-name_head);
		
		// begin to find value
		string value;
		
		while (*p && isspace(*p)) p++;
		if (*p) {
			const char *value_head = p;
			p++;
			while (*p && *p != '#') p++;
			for (p--; p>value_head && isspace(*p); p--);
			p++;
			
			if (p > value_head)
				value.assign(value_head, p-value_head);
		}
		
		m_ini[section][name] = value;
		return p;
	}
	
	// find the end of line
	while (*p && *p != '#') p++;
	return p;
}

ini_parser_t::ini_parser_t()
    :m_none("{[NONE]}"),NOTHING("")
{
}

/**
 * ���������ļ�(ͬʱ���ԭ�е�������Ϣ)
 * @param profile  INI�ļ���
 */
void 
ini_parser_t::load(const char *profile)
{
	// �����������
	m_ini.clear();
	m_sections.clear();
	
	if (profile == NULL || profile[0] == '\0') return;
	ifstream f(profile);
	if (!f) return;
	
	string section;    // ����section����
	string sq_section; // ����'[section]'
	string sec_body;   // ����'[section]'�µ����зǿա���ע��
	
	// ���д��������ļ�
	string line;
	while (f.good() && getline(f, line)) {
		if (line.empty()) continue;
		
		const char* p = line.c_str();
		while (*p && isspace(*p)) p++; // skip heading blanks
		if (*p == '\0' || *p == '#') continue; // skip blank line or comment line
		
		if (find_section(p, section, sq_section, sec_body)) continue;
		
		if (section.empty()) {
			if (sq_section.empty()) continue;
			
			// the line is data of section
			append_sq_section(sq_section, sec_body, p);
			continue;
		}
		
		// try to find name = value
		const char *line_head = p;
		const char *line_end = find_pair(p, section);
		
		// append to section
		append_sq_section(sq_section, sec_body, line_head, line_end-line_head);
	}
	
	if (!sq_section.empty())
		m_sections[sq_section] = sec_body;
}

void 
ini_parser_t::load(const string &profile)
{
	load(profile.c_str());
}

ini_parser_t::ini_parser_t(const char *profile)
{
	load(profile);
}

ini_parser_t::ini_parser_t(const string &profile)
{
	load(profile.c_str());
}

ini_parser_t::~ini_parser_t()
{
}
	
/**
 * ��ȡһ���ַ���
 * @param section  section��
 * @param key      ������
 * @param def      ������������ʱ��ȱʡֵ
 * @return ָ��������ֵ
 */
const string &
ini_parser_t::get_string(const string &section, const string &key, const string &def)const
{
	sectionMap::const_iterator pos = m_ini.find(section);
	if (pos == m_ini.end()) {
		return def;
	}
	stringMap::const_iterator sec_pos = pos->second.find(key);
	if (sec_pos == pos->second.end()) {
		return def;
	}
	return sec_pos->second;
}

const char *
ini_parser_t::get_string(const char *section, const char *key, const char *def)const
{
	string s(section);
	string k(key);
	
	const string &r = get_string(s, k, m_none);
	if (r == m_none) return def;
	return r.c_str();
}

/**
 * ��ȡһ������ֵ
 * @param section  section��
 * @param key      ������
 * @param def      ������������ʱ��ȱʡֵ
 * @return ָ��������ֵ
 */
int 
ini_parser_t::get_int(const string &section, const string &key, int def)const
{
	const string &s = get_string(section, key, m_none);
	if (s == m_none) {
		return def;
	}
	return atoi(s.c_str());
}

int 
ini_parser_t::get_int(const char *section, const char *key, int def)const
{
	const char *i = get_string(section, key);
	if (i == NULL) return def;
	return atoi(i);
}

/**
 * ��ȡһ���Ǹ�����ֵ
 * @param section  section��
 * @param key      ������
 * @param def      ������������ʱ��ȱʡֵ
 * @return ָ��������ֵ
 */
unsigned 
ini_parser_t::get_unsigned(const string &section, const string &key, unsigned def)const
{
	const string &s = get_string(section, key, m_none);
	if (s == m_none) {
		return def;
	}
	return strtoul(s.c_str(), NULL, 10);
}

unsigned 
ini_parser_t::get_unsigned(const char *section, const char *key, unsigned def)const
{
	const char *i = get_string(section, key);
	if (i == NULL) return def;
	return strtoul(i, NULL, 10);
}

/**
 * ��ȡһ������ֵ������ֵ���ַ���Ϊ"Y"/"YES"/"T"/"True"��Ϊ�棬����Ϊ��
 * @param section  section��
 * @param key      ������
 * @param def      ������������ʱ��ȱʡֵ
 * @return ָ��������ֵ
 */
bool 
ini_parser_t::get_bool(const string &section, const string &key, bool def)const
{
	const string &s = get_string(section, key, m_none);
	if (s == m_none) {
		return def;
	}
	const char *cs = s.c_str();
	if (!strcasecmp(cs, "Y") || 
		!strcasecmp(cs, "YES") ||
		!strcasecmp(cs, "T") ||
		!strcasecmp(cs, "True"))
	{
		return true;
	}
	
	if (atoi(cs)) return true;
	return false;
}

bool 
ini_parser_t::get_bool(const char *section, const char *key, bool def)const
{
	string s(section);
	string k(key);
	return get_bool(s, k, def);
}

/**
 * ��ȡĳһ��section����
 * @param section   ָ����section��
 * @return ��Ӧ������
 */
string &
ini_parser_t::get_section(const string &section)
{
	string s;
	s.append("[");
	s.append(section);
	s.append("]");
	
	stringMap::iterator pos = m_sections.find(s);
	if (pos == m_sections.end()) return NOTHING;
	return pos->second;
}

const char *
ini_parser_t::get_section(const char *section)
{
	string s(section);
	string &r = get_section(s);
	if (r == NOTHING) return NULL;
	return r.c_str();
}

/**
 * ����ini�����ݣ�������
 */
void 
ini_parser_t::dump() const
{
	sectionMap::const_iterator pos;
	stringMap::const_iterator p;
	cout << "=========== Dump key by key ===========" << endl;
	for (pos = m_ini.begin(); pos != m_ini.end(); pos++) {
		cout << '[' << pos->first << ']' << endl;
		for (p=pos->second.begin(); p!=pos->second.end(); p++) {
			cout << p->first << " = " << p->second << endl;
		}
		cout << endl;
	}
	
	cout << "=========== Dump section by section ===========" << endl;
	for (p = m_sections.begin(); p != m_sections.end(); p++) {
		cout << p->first << endl;
		cout << p->second << endl;
	}
}

#ifdef _TEST_
int main(int argc, char *argv[])
{
	if (argc == 1) {
		printf("usage: %s <ini>\n", argv[0]);
		return 1;
	}

	utility::INIParser ini(argv[1]);
	ini.dump();
	return 0;
}
#endif


}
