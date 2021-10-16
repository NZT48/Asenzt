#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <regex>
#include <iostream>
#include <fstream>

#pragma once

typedef struct symbol
{
    std::string name;
    bool local;
    unsigned int id;
    std::string section;
    unsigned int offset;
    //unsigned int size;

    symbol(std::string name, bool local, unsigned int id,
           std::string section, unsigned int offset)
    {
        this->name = name;
        this->local = local;
        this->id = id;
        this->section = section;
        this->offset = offset;
    }
} Symbol;

class Symbol_Table
{
public:
    Symbol_Table();
    ~Symbol_Table();

    bool insertSymbol(std::string name, bool local, std::string section, unsigned int offset);
    bool symbol_exists(std::string name);
    void update_symbol_to_global(std::string name);
    unsigned int get_symbol_id(std::string name);
    unsigned int get_symbol_offset(std::string name);

    std::string write_symbol_table();
    std::string debug_write_symbol_table();

private:
    unsigned int global_id = 0;
    std::map<std::string, Symbol> table;
};