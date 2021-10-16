#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <regex>

#include "symbol_table.hpp"
#include "section_table.hpp"
#include "relocation_table.hpp"

#pragma once

typedef enum
{
    TOK_UNDEFINED = 0,
    TOK_EOF,
    TOK_SYMBOL,
    TOK_INSTRUCTION,
    TOK_DIRECTIVE,
    TOK_REGISTER,
    TOK_LABEL,
    TOK_SECTION,
} Token_type;

class Assembler
{
public:
    Assembler();
    Assembler(std::string SourceName, std::string DestName);

    bool assemble();

private:
    void first_pass();
    void second_pass();
    void process_line();

    bool first_pass_process_directive(std::string directive);
    bool second_pass_process_directive(std::string directive);
    bool first_pass_process_section(std::string section);
    bool second_pass_process_section(std::string section);
    bool first_pass_process_instruction(std::string instruction);
    bool second_pass_process_instruction(std::string instruction);

    bool process_word_directive(std::string token);
    bool process_one_byte_instruction(std::string instruction);
    bool process_two_byte_instruction(std::string instruction);
    bool process_three_byte_instruction(std::string instruction);
    bool process_jmp_instructions(std::string instruction);
    bool process_ldr_str_instructions(std::string instruction);
    bool process_push_pop_instructions(std::string instruction);

    void increment_counter_for_three_or_more_bytes_instruction(std::string instruction);

    std::string remove_comment_and_to_lower(std::string line);
    std::string convert_char_to_hex(char str_num);
    Token_type resolve_token_type(std::string token);
    std::string convert_registers(std::string reg);
    std::string resolve_registers();
    std::string parse_literal(std::string literal);
    bool is_literal(std::string literal);

    std::ifstream input;
    std::ofstream output;

    Symbol_Table symbol_table;
    Section_Table section_table;
    Relocation_Table relocation_table;

    bool label_defined;
    bool error_detected;
    bool global_error;
    bool assembling;
    bool debug;
    unsigned int location_counter;
    std::string current_section;
    std::vector<std::string>::iterator token_iterator;
    std::vector<std::string> tokenized_line;
    std::map<int, std::vector<std::string>> lines;
};