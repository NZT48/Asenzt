#include "../inc/symbol_table.hpp"
#include <sstream>

Symbol_Table::Symbol_Table()
{
}

Symbol_Table::~Symbol_Table()
{
}

std::string to_lower(std::string s)
{
    // Transform to lower case
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    // Remove ':' for labels
    if (std::string::npos != s.find(":"))
        return s.substr(0, s.length() - 1);
    else
        return s;
}

bool Symbol_Table::insertSymbol(std::string name, bool local,
                                std::string section, unsigned int offset)
{
    unsigned int id = global_id++;
    Symbol smb = symbol(to_lower(name), local, id, section, offset);

    auto ret = table.insert(std::pair<std::string, Symbol>(to_lower(name), smb));
    return ret.second;
}

std::string Symbol_Table::write_symbol_table()
{
    std::map<std::string, Symbol>::iterator it = table.begin();
    std::stringstream sstream;

    sstream << "#Symbol Table" << std::endl;
    sstream << "#Symbol name | ID | LC offset | isLocal | section" << std::endl;
    sstream << "#-------------------------------------------------" << std::endl;
    for (it = table.begin(); it != table.end(); ++it)
    {
        sstream << it->first << "    | " << it->second.id << " |   " << it->second.offset << "   |   " << it->second.local << "   | " << it->second.section << std::endl;
    }
    return sstream.str();
}

std::string Symbol_Table::debug_write_symbol_table()
{
    std::map<std::string, Symbol>::iterator it = table.begin();
    std::cout << " === SYMBOL TABLE === " << std::endl;
    std::cout << " Symbol name | ID | LC offset | isLocal | section" << std::endl;
    std::cout << "-------------------------------------------------" << std::endl;
    for (it = table.begin(); it != table.end(); ++it)
    {
        std::cout << it->first << "    | " << it->second.id << " |   " << it->second.offset << "   |   " << it->second.local << "   | " << it->second.section << std::endl;
    }

    return "";
}

bool Symbol_Table::symbol_exists(std::string symbol_name)
{
    if (table.find(symbol_name) != table.end())
    {
        return true;
    }
    else
        return false;
}

unsigned int Symbol_Table::get_symbol_id(std::string symbol_name)
{
    return (table.find(symbol_name))->second.id;
}

unsigned int Symbol_Table::get_symbol_offset(std::string symbol_name)
{
    return (table.find(symbol_name))->second.offset;
}

void Symbol_Table::update_symbol_to_global(std::string symbol_name)
{
    std::map<std::string, Symbol>::iterator it = table.find(symbol_name);
    it->second.local = false;
}