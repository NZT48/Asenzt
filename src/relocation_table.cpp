#include "../inc/relocation_table.hpp"
#include <sstream>

Relocation_Table::Relocation_Table()
{
}

Relocation_Table::~Relocation_Table()
{
}

unsigned int Relocation_Table::insert_relocation_record(std::string section, unsigned int offset,
                                                        unsigned int symbol_id, std::string type)
{
    Relocation_record smb = relocation_record(section, offset, symbol_id, type);

    auto ret = table.insert(std::pair<unsigned int, Relocation_record>(++global_id, smb));
    return global_id;
}

std::string Relocation_Table::write_relocation_table()
{
    std::map<unsigned int, Relocation_record>::iterator it = table.begin();
    std::stringstream sstream;

    sstream << std::endl
            << "#Relocation table" << std::endl;
    sstream << "#Rel id | Sym id | LC offset | Type | section" << std::endl;
    sstream << "#-------------------------------------------------" << std::endl;
    for (it = table.begin(); it != table.end(); ++it)
    {
        sstream << "   " << it->first << "    |   " << it->second.symbol_id << "    |     " << it->second.offset << "     | " << it->second.type << " | " << it->second.section << std::endl;
    }
    sstream << std::endl;
    return sstream.str();
}

std::string Relocation_Table::debug_write_relocation_table()
{
    std::map<unsigned int, Relocation_record>::iterator it = table.begin();

    std::cout << " === RELOCATION TABLE === " << std::endl;
    std::cout << " Reloc id | Symbol ID | LC offset | Type | section" << std::endl;
    std::cout << "-------------------------------------------------" << std::endl;
    for (it = table.begin(); it != table.end(); ++it)
    {
        std::cout << it->first << "    | " << it->second.symbol_id << " |   " << it->second.offset << "   |   " << it->second.type << "   | " << it->second.section << std::endl;
    }

    return "";
}

bool Relocation_Table::relocation_record_exists(unsigned int symbol_id)
{
    if (table.find(symbol_id) != table.end())
    {
        return true;
    }
    else
        return false;
}
