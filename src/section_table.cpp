#include "../inc/section_table.hpp"
#include <map>
#include <sstream>

Section_Table::Section_Table()
{
    Section abs_sec = section("absolute", 0, 0);
    Section und_sec = section("und", 0, 0);
    table.insert(std::pair<std::string, Section>("und", und_sec));
    table.insert(std::pair<std::string, Section>("absolute", abs_sec));
}

Section_Table::~Section_Table()
{
}

std::string Section_Table::to_lower(std::string s)
{
    // Transform to lower case
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    // Remove ':' for labels
    if (std::string::npos != s.find(":"))
        return s.substr(0, s.length() - 1);
    else
        return s;
}

bool Section_Table::insertSection(std::string name, unsigned int size, unsigned int offset)
{
    Section sec = section(to_lower(name), size, offset);

    auto ret = table.insert(std::pair<std::string, Section>(to_lower(name), sec));
    return ret.second;
}

bool Section_Table::updateSize(std::string section, unsigned int lc)
{
    if (table.find(section) != table.end())
    {
        table.find(section)->second.size = lc;
        return true;
    }
    else
        return false;
}

std::string Section_Table::write_section_table()
{
    std::map<std::string, Section>::iterator it = table.begin();
    std::stringstream sstream;
    sstream << std::endl
            << "# Sections data" << std::endl;
    for (it = table.begin(); it != table.end(); ++it)
    {
        sstream << "#" << it->first;
        sstream << it->second.bytecode << std::endl;
    }

    return sstream.str();
}

std::string Section_Table::debug_write_section_table()
{
    std::map<std::string, Section>::iterator it = table.begin();
    std::cout << " === SECTION TABLE === " << std::endl;
    for (it = table.begin(); it != table.end(); ++it)
    {
        std::cout << "------------------------------" << std::endl;
        std::cout << "Section name: " << it->first << ", size:  " << it->second.size << std::endl; //<< ", LC offset: " << it->second.offset << '\n';
        std::cout << "Bytecode: " << it->second.bytecode << std::endl;
    }

    return "";
}

unsigned int Section_Table::insert_into_absolute_section(std::string literal)
{
    std::map<std::string, Section>::iterator it = table.find("absolute");
    unsigned int ret_value = it->second.size;
    it->second.size = it->second.size + literal.size();
    // transformin literal from ABCD to AB CD
    literal.insert(2, " ");
    it->second.bytecode = it->second.bytecode.append("\n" + literal);
    return ret_value;
}

void Section_Table::add_zeros_to_section(std::string section, unsigned int number_of_bytes)
{
    table.find(section)->second.bytecode = table.find(section)->second.bytecode.append("\n");
    for (int i = 0; i < number_of_bytes; i++)
    {
        table.find(section)->second.bytecode = table.find(section)->second.bytecode.append("00 ");
    }
}

void Section_Table::append_bytecode(std::string section, std::string bytes)
{
    std::map<std::string, Section>::iterator it = table.find(section);
    it->second.bytecode = it->second.bytecode.append("\n" + bytes);
}