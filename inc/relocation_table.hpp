#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <regex>
#include <iostream>
#include <fstream>

#pragma once

typedef struct relocation_record
{
    unsigned int symbol_id;
    std::string section;
    unsigned int offset;
    std::string type;

    relocation_record(
        std::string section, unsigned int offset, unsigned int symbol_id, std::string type)
    {
        this->type = type;
        this->symbol_id = symbol_id;
        this->section = section;
        this->offset = offset;
    }
} Relocation_record;

class Relocation_Table
{
public:
    Relocation_Table();
    ~Relocation_Table();

    unsigned int insert_relocation_record(std::string section, unsigned int offset, unsigned int symbol_id, std::string type);
    bool relocation_record_exists(unsigned int symbol_id);

    std::string write_relocation_table();
    std::string debug_write_relocation_table();

private:
    unsigned int global_id = 0;
    std::map<unsigned int, Relocation_record> table;
};