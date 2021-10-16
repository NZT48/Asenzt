#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <regex>
#include <iostream>
#include <fstream>

#pragma once

typedef struct section
{
    std::string name;
    unsigned int size;
    unsigned int offset;
    std::string bytecode;

    section(std::string name, unsigned int size, unsigned int offset)
    {
        this->name = name;
        this->size = size;
        this->offset = offset;
        this->bytecode = "";
    }
} Section;

class Section_Table
{
public:
    Section_Table();
    ~Section_Table();

    bool insertSection(std::string name, unsigned int size, unsigned int offset);
    bool updateSize(std::string section, unsigned int lc);

    unsigned int insert_into_absolute_section(std::string literal);
    void add_zeros_to_section(std::string section, unsigned int number_of_bytes);
    void append_bytecode(std::string section, std::string bytes);

    std::string debug_write_section_table();
    std::string write_section_table();
    std::string to_lower(std::string s);

private:
    std::map<std::string, Section> table;
};