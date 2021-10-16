#include <sstream>
#include <algorithm>
#include <exception>

#include "../inc/assembler.hpp"

Assembler::Assembler()
{

    this->input.open("ulaz.s", std::ios::out);
    this->output.open("izlaz.o", std::ios::binary | std::ios::out | std::ios::trunc);

    if (!input.is_open())
    {
        std::cout << "Input file error" << std::endl;
        exit(-1);
    }
    else if (!output.is_open())
    {
        std::cout << "Output file error" << std::endl;
        exit(-1);
    }
}

Assembler::Assembler(std::string SourceName, std::string DestName)
{

    this->input.open(SourceName, std::ios::out);
    this->output.open(DestName, std::ios::binary | std::ios::out | std::ios::trunc);

    if (!input.is_open())
    {
        std::cout << "Input file error" << std::endl;
        exit(-1);
    }
    else if (!output.is_open())
    {
        std::cout << "Output file error" << std::endl;
        exit(-1);
    }
}

bool Assembler::assemble()
{
    debug = false;
    std::string buff;
    std::stringstream lines;
    if (debug)
        std::cout << " === FIRST PASS === " << std::endl;

    first_pass();

    if (debug)
    {
        symbol_table.debug_write_symbol_table();
        section_table.debug_write_section_table();
        std::cout << " === SECOND PASS === " << std::endl;
    }

    second_pass();

    if (debug)
    {
        symbol_table.debug_write_symbol_table();
        section_table.debug_write_section_table();
        relocation_table.debug_write_relocation_table();
    }

    // Write to file
    output << symbol_table.write_symbol_table();
    output << section_table.write_section_table();
    output << relocation_table.write_relocation_table();

    // Close files
    input.close();
    output.close();

    if (global_error)
        return false;
    else
        return true;
}

void Assembler::first_pass()
{

    // Initialize data
    std::string line;
    int num_line = 0;
    location_counter = 0;
    current_section = "UND"; // Setup undefined as first section
    assembling = true;
    error_detected = false;
    global_error = false;

    while (std::getline(input, line))
    {

        /* ----- Reading and parsing input ----- */

        line = remove_comment_and_to_lower(line);
        //std::cout << "Line without comments: " << line << std::endl;
        // Tokenizing
        size_t start = line.find_first_not_of(" ,\t");
        size_t end = start;
        //std::cout << " Starting tokenization " << std::endl;

        // Split line into tokens
        while (start != std::string::npos)
        {
            end = line.find_first_of(" +,\t", start);
            std::string pushin_string = line.substr(start, end - start);
            //std::cout << "Adding string " << pushin_string << std::endl;
            tokenized_line.push_back(pushin_string);
            start = line.find_first_not_of(" +,\t", end);
        }
        //lines.insert(std::make_pair(num_line++, tokenized_line));
        //std::cout << "Finished tokenization of line" << std::endl;

        /* ----- Process every token ----- */
        for (token_iterator = tokenized_line.begin(); token_iterator != tokenized_line.end(); token_iterator++)
        {
            if (error_detected)
            {
                error_detected = false;
                global_error = true;
                break; // jump out from this line, and get next
            }

            std::string token = *token_iterator;
            if (debug)
                std::cout << location_counter << ": processing token -> " << token << std::endl;

            Token_type token_type = resolve_token_type(token);

            // Reset label flag
            if (token_type != TOK_LABEL)
            {
                label_defined = false;
            }

            switch (token_type)
            {
            case TOK_SECTION:
                if (debug)
                    std::cout << " Token is section " << std::endl;
                if (!first_pass_process_section(token))
                {
                    std::cout << "Error: processing section failed" << std::endl;
                    error_detected = true;
                }
                break;
            case TOK_LABEL:
                if (label_defined)
                {
                    std::cout << "Error: Label already defined " << std::endl;
                    error_detected = true;
                    break;
                }
                if (debug)
                    std::cout << "Token is label or section" << std::endl;
                // Removing ':' from label name
                token = token.substr(0, token.length() - 1);
                // Insert into symbol table
                if (!symbol_table.insertSymbol(token, true, current_section, location_counter))
                {
                    error_detected = true;
                    std::cout << "Error: Symbol already exists " << std::endl;
                    break;
                }
                label_defined = true;
                break;
            case TOK_SYMBOL:
                if (debug)
                    std::cout << "Token is symbol " << std::endl;
                if (!symbol_table.insertSymbol(token, true, current_section, location_counter))
                {
                    std::cout << "Error inserting symbol " << token << std::endl;
                    error_detected = true;
                    break;
                }
                break;
            case TOK_DIRECTIVE:
                if (debug)
                    std::cout << "Token is directive" << std::endl;
                if (!first_pass_process_directive(token))
                {
                    std::cout << "Error processing directive " << token << std::endl;
                    error_detected = true;
                    break;
                }
                break;
            case TOK_INSTRUCTION:
                if (!first_pass_process_instruction(token))
                {
                    std::cout << "Error processing instruction " << token << std::endl;
                    error_detected = true;
                    break;
                }
                if (debug)
                    std::cout << "Instruction processed, lc is " << location_counter << std::endl;
                break;
            default:
                if (debug)
                    std::cout << "No action" << std::endl;
                break;
            }
            //std::cout << "Getting new token in line" << std::endl;
            if (!assembling)
            {
                break;
            }
        }

        // Reset variables
        tokenized_line.clear();

        if (!assembling)
        {
            break;
        }
    }

    // Close and update section, reset variables
    section_table.updateSize(current_section, location_counter);
    location_counter = 0;
    assembling = false;
    current_section = "UND";
    error_detected = false;
    global_error = false;
}

void Assembler::second_pass()
{

    // Initialize data
    std::string line;
    int num_line = 0;
    location_counter = 0;
    current_section = "UND"; // Setup undefined as first section
    assembling = true;
    error_detected = false;
    global_error = false;

    // Move cursor to start of file
    input.clear();
    input.seekg(0, std::ios::beg);

    while (std::getline(input, line))
    {

        /* ----- Reading and parsing input ----- */

        line = remove_comment_and_to_lower(line);
        //std::cout << "Line without comments: " << line << std::endl;
        // Tokenizing
        size_t start = line.find_first_not_of(" ,\t");
        size_t end = start;
        //std::cout << " Starting tokenization " << std::endl;

        // Split line into tokens
        tokenized_line.clear();
        while (start != std::string::npos)
        {
            end = line.find_first_of(" +,\t", start);
            std::string pushin_string = line.substr(start, end - start);
            //std::cout << "Adding string " << pushin_string << std::endl;
            tokenized_line.push_back(pushin_string);
            start = line.find_first_not_of(" +,\t", end);
        }
        lines.insert(std::make_pair(num_line++, tokenized_line));
        //std::cout << "Finished tokenization of line" << std::endl;

        /* ----- Process every token ----- */
        for (token_iterator = tokenized_line.begin(); token_iterator != tokenized_line.end(); token_iterator++)
        {
            if (error_detected)
            {
                error_detected = false;
                global_error = true;
                break; // jump out from this line, and get next
            }
            std::string token = *token_iterator;

            if (debug)
                std::cout << location_counter << ": processing token (second pass) -> " << token << std::endl;

            Token_type token_type = resolve_token_type(token);
            switch (token_type)
            {
            case TOK_SECTION:
                if (debug)
                    std::cout << " Token is section " << std::endl;
                if (!second_pass_process_section(token))
                {
                    std::cout << "Error in second pass - processing section failed" << std::endl;
                    error_detected = true;
                }
                break;
            case TOK_DIRECTIVE:
                if (debug)
                    std::cout << "Token is directive" << std::endl;
                if (!second_pass_process_directive(token))
                {
                    std::cout << "Error in second pass processing directive: " << token << std::endl;
                    error_detected = true;
                }
                break;
            case TOK_INSTRUCTION:
                if (debug)
                    std::cout << "Token is instruction" << std::endl;
                if (!second_pass_process_instruction(token))
                {
                    std::cout << "Error in second pass, processing instruction " << token << std::endl;
                    error_detected = true;
                }
                break;
            default:
                if (debug)
                    std::cout << "No action" << std::endl;
                break;
            }
        }
        if (!assembling)
        {
            break;
        }
    }
}

std::string Assembler::remove_comment_and_to_lower(std::string line)
{
    std::string line_without_comment = line.substr(0, line.find("#"));
    std::transform(line_without_comment.begin(), line_without_comment.end(), line_without_comment.begin(), ::tolower);
    return line_without_comment;
}

Token_type Assembler::resolve_token_type(std::string value)
{
    Token_type type_t = TOK_UNDEFINED;
    std::regex label{"([a-zA-Z0-9_]*):"};
    std::regex section{"(\\.)(text|data|bss|rodata|section)"};
    std::regex directive{"\\.(global|extern|word|skip|equ|end)"};
    std::regex instruction{"(halt|int|iret|call|ret|jmp|jeq|jne|jgt|push|pop|xchg|add|sub|mul|div|cmp|not|and|or|xor|test|shl|shr|ldr|str)?"};
    std::regex symbol{("^([a-zA-Z_][a-zA-Z0-9]*)$")};
    std::regex registers{("(r0|r1|r2|r3|r4|r5|r6|r7|sp|pc|psw)")};
    std::regex end{"(\\.)(end)"};
    if (std::regex_match(value, registers))
        return Token_type::TOK_REGISTER;
    if (std::regex_match(value, label))
        return Token_type::TOK_LABEL;
    if (std::regex_match(value, directive))
        return Token_type::TOK_DIRECTIVE;
    if (std::regex_match(value, section))
        return Token_type::TOK_SECTION;
    if (std::regex_match(value, instruction))
        return Token_type::TOK_INSTRUCTION;
    if (std::regex_match(value, symbol))
        return Token_type::TOK_SYMBOL;
    if (std::regex_match(value, end))
        return Token_type::TOK_EOF;
    return type_t;
}

bool Assembler::first_pass_process_directive(std::string directive)
{
    if (directive.compare(".word") == 0)
    {
        if (debug)
            std::cout << "Processing word directive in first pass" << std::endl;
        // just increment location counter and skip symbols
        //std::cout << ".word directive: Location counter value is " << location_counter << std::endl;
        int num_of_symbols = 0;
        while (++token_iterator != tokenized_line.end())
            num_of_symbols++;
        location_counter += 2 * num_of_symbols;
        token_iterator--;
        //std::cout << ".word directive: After execution of directive" << location_counter << std::endl;
    }
    else if (directive.compare(".skip") == 0) // done
    {
        if (debug)
            std::cout << "Processing skip directive in first pass" << std::endl;
        //std::cout << ".skip directive: Location counter value is " << location_counter << std::endl;
        std::string next_token = *(++token_iterator);

        int bytes_to_skip = 0;

        try
        {
            bytes_to_skip = std::stoi(next_token);
        }
        catch (std::exception &e)
        {
            std::cout << "Standard exception: " << e.what() << std::endl;
            std::cout << "Execption in skip directive" << std::endl;
            return false;
        }

        //std::cout << bytes_to_skip << std::endl;
        location_counter += bytes_to_skip;
        //std::cout << ".skip directive: After execution of directive" << location_counter << std::endl;
    }
    else if (directive.compare(".global") == 0) // done
    {
        // In first pass no processing
        if (debug)
            std::cout << "No processing for global directive in first pass" << std::endl;
        while (++token_iterator != tokenized_line.end())
            ;
        token_iterator--;
    }
    else if (directive.compare(".extern") == 0) // done
    {
        try
        {
            if (debug)
                std::cout << "Processing extern directive" << std::endl;
            while (++token_iterator != tokenized_line.end())
            {
                std::string current_symbol = *token_iterator;
                if (debug)
                    std::cout << "Procssing extern symbol " << current_symbol << std::endl;
                if (!symbol_table.insertSymbol(current_symbol, true, "extern", 0))
                {
                    std::cout << "Error inserting symbol with extern " << current_symbol << std::endl;
                    return false;
                }
            }
            token_iterator--;
        }

        catch (std::exception &e)
        {
            std::cout << "Standard exception in extern directive: " << e.what() << std::endl;
            return false;
        }
    }
    else if (directive.compare(".equ") == 0) // done
    {
        try
        {

            if (debug)
                std::cout << "Processing equ directive in first pass" << std::endl;
            std::string smb_name = *(++token_iterator);
            std::string smb_literal = *(++token_iterator);
            //std::cout << "Processing equ directive with symbols: " << smb_name << " & " << smb_literal << std::endl;
            unsigned int abs_section_offest = section_table.insert_into_absolute_section(parse_literal(smb_literal));
            if (!symbol_table.insertSymbol(smb_name, true, "absolute", abs_section_offest))
            {
                std::cout << "Error inserting symbol with equ directive: " << smb_name << std::endl;
                return false;
            }
        }
        catch (std::exception &e)
        {
            std::cout << "Standard exception: " << e.what() << std::endl;
            return false;
        }
    }
    else if (directive.compare(".end") == 0)
    {
        if (debug)
            std::cout << "Processing end directive" << std::endl;
        // End of first pass
        assembling = false;
        return true;
    }
    //std::cout << "Exiting first pass process directive " << std::endl;
    return true;
}

bool Assembler::second_pass_process_directive(std::string directive)
{
    if (directive.compare(".word") == 0)
    {
        try
        {
            if (debug)
                std::cout << "Processing word directive in second pass" << std::endl;
            //std::cout << ".word directive: Location counter value is " << location_counter << std::endl;
            int num_of_symbols = 0;

            while (++token_iterator != tokenized_line.end())
            {

                if (!process_word_directive(*token_iterator))
                {
                    return false;
                }

                num_of_symbols++;
            }

            //std::cout << "Outside while loop in second pass process directive word " << std::endl;
            location_counter += 2 * num_of_symbols;
            token_iterator--;
            //std::cout << ".word directive: After execution of directive" << location_counter << std::endl;
        }
        catch (std::exception &e)
        {
            std::cout << "Standard exception in second pass of word directive: " << e.what() << std::endl;
            return false;
        }
    }
    else if (directive.compare(".skip") == 0)
    {
        try
        {
            if (debug)
                std::cout << "Processing skip directive in second pass" << std::endl;
            //std::cout << ".skip directive: Location counter value is " << location_counter << std::endl;
            int bytes_to_skip = std::stoi(*(++token_iterator));
            //std::cout << bytes_to_skip << std::endl;
            location_counter += bytes_to_skip;
            section_table.add_zeros_to_section(current_section, bytes_to_skip);
            //std::cout << ".skip directive: After execution of directive" << location_counter << std::endl;
        }
        catch (std::exception &e)
        {
            std::cout << "Standard exception in skip directive (second pass): " << e.what() << std::endl;
            return false;
        }
    }
    else if (directive.compare(".global") == 0)
    {
        try
        {
            if (debug)
                std::cout << "Processing global directive in second pass" << std::endl;
            // In second setup symbol that is global referencing to be global

            // get every simbol after global directive
            while (++token_iterator != tokenized_line.end())
            {
                std::string symbol = *token_iterator;
                if (symbol_table.symbol_exists(symbol))
                {
                    symbol_table.update_symbol_to_global(symbol);
                    //std::cout << "Symbol changed to global" << std::endl;
                }
                else
                {
                    std::cout << "Error, we have undefined symbol in global directive: " << symbol << std::endl;
                    return false;
                }
            }
            token_iterator--;
        }
        catch (std::exception &e)
        {
            std::cout << "Standard exception in global directive (second pass): " << e.what() << std::endl;
            return false;
        }
    }
    else if (directive.compare(".extern") == 0) // done
    {
        if (debug)
            std::cout << "Processing extern directive, no processing in second pass" << std::endl;
    }
    else if (directive.compare(".equ") == 0) // all processing done in first pass
    {
        if (debug)
            std::cout << "No processing for equ directive in second pass" << std::endl;
    }
    else if (directive.compare(".end") == 0)
    {
        if (debug)
            std::cout << "Processing end directive" << std::endl;
        // End of first pass
        assembling = false;
        return true;
    }
    return true;
}

bool Assembler::process_word_directive(std::string token)
{
    if (debug)
        std::cout << "Proccesing word " << token << std::endl;
    if (is_literal(token))
    {
        std::string parsed_literal = parse_literal(token);
        parsed_literal.insert(2, " ");
        section_table.append_bytecode(current_section, parsed_literal);
    }
    else // symbol
    {
        //std::cout << "Proccesing as symbol " << token << std::endl;
        std::string symbol = token;
        if (symbol_table.symbol_exists(symbol))
        {
            //std::cout << "Found symbol " << std::endl;
            unsigned int symbol_id = symbol_table.get_symbol_id(symbol);
            //std::cout << "Symbol id is " << symbol_id << std::endl;
            unsigned int reloc_id = relocation_table.insert_relocation_record(current_section, location_counter, symbol_id, "R_386_16");
            //std::cout << "Reloc id is " << reloc_id << std::endl;
            std::string reloc_id_in_hex = parse_literal(std::to_string(reloc_id));
            //std::cout << "Parsed literal " << reloc_id_in_hex << std::endl;
            reloc_id_in_hex.insert(2, " ");
            section_table.append_bytecode(current_section, reloc_id_in_hex);
            //std::cout << "After append bytecode " << std::endl;
        }
        else
        {
            std::cout << "Symbol: " << symbol << " in word directive does not exist" << std::endl;
            return false;
        }
    }
    return true;
}

bool Assembler::first_pass_process_section(std::string section)
{
    try
    {
        if (section.compare(".section") == 0)
        {
            // std::cout << "Getting section name" << std::endl;
            section = *(++token_iterator);
        }
        else
        {
            // in case that we have .text or .rodata
            section.erase(0, 1);
        }

        //std::cout << "Section name is " << section << std::endl;

        if (section.compare(current_section) == 0)
        {
            std::cout << "Error: section with same name again defined" << std::endl;
            return false;
        }

        //std::cout << "Updatding size of previous section: " << current_section << std::endl;
        section_table.updateSize(current_section, location_counter);

        current_section = section;
        location_counter = 0;
        // Adding section name to symbol table, not for now
        // symbol_table.insertSymbol(section, true, current_section, location_counter);
        section_table.insertSection(section, 0, 0); // check offest if it is used
        return true;
    }
    catch (std::exception &e)
    {
        std::cout << "Standard exception: " << e.what() << std::endl;
        std::cout << "Exception in processing section in first pass " << std::endl;
        return false;
    }
}

bool Assembler::second_pass_process_section(std::string section)
{
    if (section.compare(".section") == 0)
    {
        // get the section name
        // std::cout << "Getting section name" << std::endl;
        section = *(++token_iterator);
    }
    else
    {
        section.erase(0, 1);
    }

    //std::cout << "Section name is " << section << std::endl;

    if (section.compare(current_section) == 0)
    {
        std::cout << "Error: section with same name again defined" << std::endl;
        return false;
    }
    //std::cout << "Updated size of " << current_section << std::endl;
    current_section = section;
    location_counter = 0;
    return true;
}

bool Assembler::first_pass_process_instruction(std::string instruction)
{

    std::regex one_byte_instruction{"(halt|iret|ret)?"};
    std::regex two_bytes_instruction{"(int|xchg|add|sub|mul|div|cmp|not|and|or|xor|test|shl|shr)?"};
    std::regex three_bytes_or_more_instruction{"(call|jmp|jeq|jne|jgt|push|pop|ldr|str)?"};
    try
    {
        // Move location counter according to instruction size
        if (std::regex_match(instruction, one_byte_instruction))
            location_counter += 1;
        if (std::regex_match(instruction, two_bytes_instruction))
            location_counter += 2;
        if (std::regex_match(instruction, three_bytes_or_more_instruction))
        {
            increment_counter_for_three_or_more_bytes_instruction(instruction);
        }

        // Go to end of line
        auto token_iterator_in_front = token_iterator;
        while (++token_iterator_in_front != tokenized_line.end())
        {
            if (debug)
                std::cout << "Token inside instruction: " << *token_iterator_in_front << std::endl;
            token_iterator++;
        }

        return true;
    }
    catch (std::exception &e)
    {
        std::cout << "Standard exception: " << e.what() << std::endl;
        std::cout << "Exception in processing section in first pass " << std::endl;
        return false;
    }
}

bool Assembler::second_pass_process_instruction(std::string instruction)
{

    std::regex one_byte_instruction{"(halt|iret|ret)?"};
    std::regex two_bytes_instruction{"(int|xchg|add|sub|mul|div|cmp|not|and|or|xor|test|shl|shr)?"};
    std::regex three_bytes_instruction{"(call|jmp|jeq|jne|jgt|push|pop|ldr|str)?"};

    if (debug)
        std::cout << "Processing second pass instruction" << std::endl;

    // Move location counter according to instruction size
    if (std::regex_match(instruction, one_byte_instruction))
    {
        if (!process_one_byte_instruction(instruction))
            return false;
        location_counter += 1;
    }
    if (std::regex_match(instruction, two_bytes_instruction))
    {
        if (!process_two_byte_instruction(instruction))
            return false;
        location_counter += 2;
    }
    if (std::regex_match(instruction, three_bytes_instruction))
    {
        //std::cout << "Three or more byte instruction detected" << std::endl;
        if (!process_three_byte_instruction(instruction)) // location counter incremented inside
            return false;
    }

    return true;
}

bool Assembler::process_one_byte_instruction(std::string instruction)
{
    std::string instruction_bytes = "";
    if (instruction.compare("halt") == 0)
    {
        instruction_bytes = "00";
    }
    else if (instruction.compare("iret") == 0)
    {
        instruction_bytes = "20";
    }
    else if (instruction.compare("ret") == 0)
    {
        instruction_bytes = "40";
    }
    else
    {
        std::cout << "Error in processing one byte instruction " << std::endl;
        return false;
    }
    section_table.append_bytecode(current_section, instruction_bytes);
    return true;
}

bool Assembler::process_two_byte_instruction(std::string instruction)
{
    std::string first_byte = "";
    std::string second_byte = "";
    std::string instruction_bytes = "";
    if (instruction.compare("int") == 0)
    {
        first_byte = "10";

        // get next token and check if it is register
        std::string token = *(++token_iterator);
        Token_type token_type = resolve_token_type(token);
        if (token_type == TOK_REGISTER)
            second_byte = convert_char_to_hex(token[1]) + "F";
        else
        {
            std::cout << "Error in processing init instruction " << std::endl;
            return false;
        }
        instruction_bytes = first_byte + " " + second_byte;
    }
    else if (instruction.compare("xchg") == 0)
    {
        first_byte = "60";
        second_byte = resolve_registers();
        instruction_bytes = first_byte + " " + second_byte;
    }
    else if (
        instruction.compare("add") == 0 ||
        instruction.compare("sub") == 0 ||
        instruction.compare("mul") == 0 ||
        instruction.compare("div") == 0 ||
        instruction.compare("cmp") == 0)
    {
        first_byte = "7";
        if (instruction.compare("add") == 0)
            first_byte += "0";
        else if (instruction.compare("sub") == 0)
            first_byte += "1";
        else if (instruction.compare("mul") == 0)
            first_byte += "2";
        else if (instruction.compare("div") == 0)
            first_byte += "3";
        else if (instruction.compare("cmp") == 0)
            first_byte += "4";
        else
        {
            std::cout << "Unexpected behaviour in two byte instruction processing!" << std::endl;
            return false;
        }
        second_byte = resolve_registers();
        instruction_bytes = first_byte + " " + second_byte;
    }
    else if (
        instruction.compare("not") == 0 ||
        instruction.compare("and") == 0 ||
        instruction.compare("or") == 0 ||
        instruction.compare("xor") == 0 ||
        instruction.compare("test") == 0)
    {
        first_byte = "8";
        if (instruction.compare("not") == 0)
            first_byte += "0";
        else if (instruction.compare("and") == 0)
            first_byte += "1";
        else if (instruction.compare("or") == 0)
            first_byte += "2";
        else if (instruction.compare("xor") == 0)
            first_byte += "3";
        else if (instruction.compare("test") == 0)
            first_byte += "4";
        else
        {
            std::cout << "Unexpected behaviour in two byte instruction processing!" << std::endl;
            return false;
        }
        if (instruction.compare("not") == 0)
        {
            std::string token = *(++token_iterator); // get next token
            Token_type token_type = resolve_token_type(token);
            if (token_type == TOK_REGISTER)
                second_byte = convert_char_to_hex(token[1]) + "0"; // Setup 0 as default -> DDDD0000 where DDDD = destination and source register
        }
        else
            second_byte = resolve_registers();

        instruction_bytes = first_byte + " " + second_byte;
    }
    else if (
        instruction.compare("shl") == 0 ||
        instruction.compare("shr") == 0)
    {
        first_byte = "9";
        if (instruction.compare("shl") == 0)
            first_byte += "0";
        else if (instruction.compare("shr") == 0)
            first_byte += "1";
        else
        {
            std::cout << "Unexpected behaviour in two byte instruction processing!" << std::endl;
            return false;
        }
        second_byte = resolve_registers();
        instruction_bytes = first_byte + " " + second_byte;
    }
    else
    {
        std::cout << "Unrecognized instructions in two byte instruction processing!" << std::endl;
        return false;
    }

    section_table.append_bytecode(current_section, instruction_bytes);
    return true;
}

bool Assembler::process_three_byte_instruction(std::string instruction)
{

    std::regex jmp_instrutions{"(call|jmp|jeq|jne|jgt)?"};
    std::regex ldr_str_instructions{("(ldr|str)?")};
    std::regex push_pop_instructions{("(push|pop)?")};

    if (std::regex_match(instruction, jmp_instrutions))
    {
        if (!process_jmp_instructions(instruction))
            return false;
    }
    else if (std::regex_match(instruction, ldr_str_instructions))
    {
        if (!process_ldr_str_instructions(instruction))
            return false;
    }
    else if (std::regex_match(instruction, push_pop_instructions))
    {
        if (!process_push_pop_instructions(instruction))
            return false;
    }
    else
    {
        std::cout << "Unrecognized instruciton in three byte instruction processing!" << std::endl;
        return false;
    }
    return true;
}

bool Assembler::process_jmp_instructions(std::string instruction)
{
    std::string first_byte = "5";   // default value for all, except call
    std::string second_byte = "F0"; // default value for all that are not using reg
    std::string third_byte = "";
    std::string fourth_byte = "";
    std::string fifth_byte = "";
    std::string instruction_bytes = "";

    location_counter += 3; // for first three bytes

    // set up opcode byte
    if (instruction.compare("call") == 0)
        first_byte = "30";
    else if (instruction.compare("jmp") == 0)
        first_byte += "0";
    else if (instruction.compare("jeq") == 0)
        first_byte += "1";
    else if (instruction.compare("jne") == 0)
        first_byte += "2";
    else if (instruction.compare("jgt") == 0)
        first_byte += "3";
    else
    {
        std::cout << "Unrecognized jump instruction" << std::endl;
        return false;
    }

    // get next token
    std::string next_token = *(++token_iterator);
    //std::cout << "Got next token in jmp " << next_token << std::endl; //<< " and value at 0 " << next_token.at(0) << std::endl;

    if (next_token.at(0) == '%') // %symbol
    {
        //std::cout << "Proccessing jmp percent symbol" << std::endl;
        // parse to remove %
        next_token.erase(0, 1);
        //std::cout << "Substring passed" << std::endl;
        std::string symbol = next_token;
        second_byte = "F7";
        third_byte = "05";
        if (symbol_table.symbol_exists(symbol))
        {
            unsigned int symbol_id = symbol_table.get_symbol_id(symbol);
            unsigned int reloc_id = relocation_table.insert_relocation_record(current_section, location_counter, symbol_id, "R_386_PC16");
            std::string reloc_id_in_hex = parse_literal(std::to_string(reloc_id));
            fourth_byte += reloc_id_in_hex[0];
            fourth_byte += reloc_id_in_hex[1];
            fifth_byte += reloc_id_in_hex[2];
            fifth_byte += reloc_id_in_hex[3];
        }
        else
        {
            std::cout << "Symbol " << symbol << " does not exist" << std::endl;
            return false;
        }
        instruction_bytes = first_byte + " " + second_byte + " " + third_byte + " " + fourth_byte + " " + fifth_byte;
        location_counter += 2; // for fourth and fifth byte
    }
    else if (next_token.at(0) == '*') // *literal, *symbol, *reg, *[reg], *[reg + literal], *[reg + symbol]
    {
        if (debug)
            std::cout << "Proccessing jmp *literal, *symbol, *reg, *[reg], *[reg + literal], *[reg + symbol]" << std::endl;
        // parse to remove *
        next_token.erase(0, 1);
        //std::cout << "After ereasing" << next_token << std::endl;
        if (next_token.at(0) == '[') // *[reg], *[reg + literal], *[reg + symbol]
        {
            // parse to remove [
            next_token.erase(0, 1);

            // check ] exists
            if (next_token.at(next_token.length() - 1) == ']') // *[reg]
            {
                // remove ]
                next_token.erase(next_token.end() - 1);
                second_byte = "F" + convert_char_to_hex(convert_registers(next_token)[1]);
                third_byte = "02";
                instruction_bytes = first_byte + " " + second_byte + " " + third_byte;
            }
            else // *[reg + literal], *[reg + symbol]
            {
                second_byte = "F" + convert_char_to_hex(convert_registers(next_token)[1]);
                third_byte = "03";
                std::string second_operand = *(++token_iterator); // symbol or literal
                // remove ]
                second_operand.erase(second_operand.end() - 1);
                if (is_literal(second_operand))
                {
                    std::string parsed_literal = parse_literal(second_operand);
                    fourth_byte += parsed_literal[0];
                    fourth_byte += parsed_literal[1];
                    fifth_byte += parsed_literal[2];
                    fifth_byte += parsed_literal[3];
                }
                else
                {
                    std::string symbol = second_operand;
                    if (symbol_table.symbol_exists(symbol))
                    {
                        unsigned int symbol_id = symbol_table.get_symbol_id(symbol);
                        unsigned int reloc_id = relocation_table.insert_relocation_record(current_section, location_counter, symbol_id, "R_386_16");
                        std::string reloc_id_in_hex = parse_literal(std::to_string(reloc_id));
                        fourth_byte += reloc_id_in_hex[0];
                        fourth_byte += reloc_id_in_hex[1];
                        fifth_byte += reloc_id_in_hex[2];
                        fifth_byte += reloc_id_in_hex[3];
                    }
                    else
                    {
                        std::cout << "Symbol " << symbol << " does not exist" << std::endl;
                        return false;
                    }
                }

                instruction_bytes = first_byte + " " + second_byte + " " + third_byte + " " + fourth_byte + " " + fifth_byte;
                location_counter += 2; // for fourth and fifth byte
            }
        }
        else // *literal, *symbol, *reg
        {
            if (debug)
                std::cout << "Proccessing jmp *literal, *symbol, *reg" << std::endl;
            // check if it is reg
            Token_type type = resolve_token_type(next_token);
            if (type == TOK_REGISTER)
            {
                second_byte = "F" + convert_char_to_hex(convert_registers(next_token)[1]);
                third_byte = "01";
                instruction_bytes = first_byte + " " + second_byte + " " + third_byte;
            }
            else
            {
                third_byte = "04";
                if (is_literal(next_token))
                {
                    std::string parsed_literal = parse_literal(next_token);
                    fourth_byte += parsed_literal[0];
                    fourth_byte += parsed_literal[1];
                    fifth_byte += parsed_literal[2];
                    fifth_byte += parsed_literal[3];
                }
                else
                {

                    std::string symbol = next_token;
                    if (symbol_table.symbol_exists(symbol))
                    {
                        unsigned int symbol_id = symbol_table.get_symbol_id(symbol);
                        unsigned int reloc_id = relocation_table.insert_relocation_record(current_section, location_counter, symbol_id, "R_386_16");
                        std::string reloc_id_in_hex = parse_literal(std::to_string(reloc_id));
                        fourth_byte += reloc_id_in_hex[0];
                        fourth_byte += reloc_id_in_hex[1];
                        fifth_byte += reloc_id_in_hex[2];
                        fifth_byte += reloc_id_in_hex[3];
                    }
                    else
                    {
                        std::cout << "Symbol " << symbol << " does not exist" << std::endl;
                        return false;
                    }
                }
                instruction_bytes = first_byte + " " + second_byte + " " + third_byte + " " + fourth_byte + " " + fifth_byte;
                location_counter += 2; // for fourth and fifth byte
            }
        }
    }
    else // literal or symbol
    {
        //std::cout << "starting symbol or literal" << std::endl;
        third_byte = "00";
        //std::cout << "Processing literal or sybmbol, ";
        // check if literal
        if (is_literal(next_token))
        {

            //std::cout << "Processing literal and parsing, " << std::endl;
            std::string parsed_literal = parse_literal(next_token);
            //std::cout << "parsing finished";
        }
        else
        { // symbol
            // setup fourth and fifth byte
            std::string symbol = next_token;
            if (symbol_table.symbol_exists(symbol))
            {
                unsigned int symbol_id = symbol_table.get_symbol_id(symbol);
                unsigned int reloc_id = relocation_table.insert_relocation_record(current_section, location_counter, symbol_id, "R_386_16");
                std::string reloc_id_in_hex = parse_literal(std::to_string(reloc_id));
                fourth_byte += reloc_id_in_hex[0];
                fourth_byte += reloc_id_in_hex[1];
                fifth_byte += reloc_id_in_hex[2];
                fifth_byte += reloc_id_in_hex[3];
            }
            else
            {
                std::cout << "Symbol " << symbol << " does not exist" << std::endl;
                return false;
            }
        }
        instruction_bytes = first_byte + " " + second_byte + " " + third_byte + " " + fourth_byte + " " + fifth_byte;
        location_counter += 2; // for fourth and fifth byte
    }
    section_table.append_bytecode(current_section, instruction_bytes);
    return true;
}

bool Assembler::process_ldr_str_instructions(std::string instruction)
{
    std::string first_byte = "";
    std::string second_byte = "";
    std::string third_byte = "";
    std::string fourth_byte = "";
    std::string fifth_byte = "";
    std::string instruction_bytes = "";

    //std::cout << "Inside ldr/str " << std::endl;

    // setup opcode
    if (instruction.compare("ldr") == 0)
    {
        first_byte = "A0";
    }
    else if (instruction.compare("str") == 0)
    {
        first_byte = "B0";
    }
    else
    {
        std::cout << "Unexpected behaviour!" << std::endl;
        return false; // handle to go to next line
    }

    // getting the first register and setting it to first half of second byte
    std::string reg_token = *(++token_iterator);
    //std::cout << " My reg is " << reg_token << std::endl;

    if (resolve_token_type(reg_token) == TOK_REGISTER)
    {
        second_byte = convert_char_to_hex(convert_registers(reg_token)[1]);
    }
    else
    {
        std::cout << "Error no register for instruction" << std::endl;
        return false;
    }

    // Increment for first 2 bytes
    location_counter += 2;

    // Get operand
    std::string next_token = *(++token_iterator);

    if (next_token.at(0) == '$') // $<literal>, $<smybol>
    {
        // resolve if it is symbol or literal
        second_byte += "0"; // filling up second byte
        next_token.erase(0, 1);
        std::string parsed_token = next_token;
        unsigned int value;
        std::string value_in_hex;
        third_byte = "00";
        location_counter++; // increment for third byte

        // check if literal
        if (is_literal(next_token))
        {
            std::string parsed_literal = parse_literal(next_token);
            fourth_byte += parsed_literal[0];
            fourth_byte += parsed_literal[1];
            fifth_byte += parsed_literal[2];
            fifth_byte += parsed_literal[3];
        }
        else
        { // symbol
            // setup fourth and fifth byte
            std::string symbol = next_token;
            if (symbol_table.symbol_exists(symbol))
            {
                unsigned int symbol_id = symbol_table.get_symbol_id(symbol);
                unsigned int reloc_id = relocation_table.insert_relocation_record(current_section, location_counter, symbol_id, "R_386_16");
                std::string reloc_id_in_hex = parse_literal(std::to_string(reloc_id));
                fourth_byte += reloc_id_in_hex[0];
                fourth_byte += reloc_id_in_hex[1];
                fifth_byte += reloc_id_in_hex[2];
                fifth_byte += reloc_id_in_hex[3];
            }
            else
            {
                std::cout << "Symbol " << symbol << " does not exist" << std::endl;
                return false;
            }
        }
        instruction_bytes = first_byte + " " + second_byte + " " + third_byte + " " + fourth_byte + " " + fifth_byte;
        location_counter += 2; // for fourth and fifth byte
    }
    else if (next_token.at(0) == '%') // %<symbol> - pc relative
    {
        // check if symbol exists
        next_token.erase(0, 1);
        std::string symbol = next_token;
        second_byte += "7"; // filling up second byte - r7 == pc
        if (symbol_table.symbol_exists(symbol))
        {
            location_counter++;
            third_byte = "03"; // without update of regs and reg indirect with 16b move
            unsigned int symbol_id = symbol_table.get_symbol_id(symbol);
            unsigned int reloc_id = relocation_table.insert_relocation_record(current_section, location_counter, symbol_id, "R_386_PC16");
            std::string reloc_id_in_hex = parse_literal(std::to_string(reloc_id));
            fourth_byte += reloc_id_in_hex[0];
            fourth_byte += reloc_id_in_hex[1];
            fifth_byte += reloc_id_in_hex[2];
            fifth_byte += reloc_id_in_hex[3];
        }
        else
        {
            std::cout << "Symbol " << symbol << " does not exist" << std::endl;
            return false;
        }
        location_counter += 2;
        instruction_bytes = first_byte + " " + second_byte + " " + third_byte + " " + fourth_byte + " " + fifth_byte;
    }
    else if (resolve_token_type(next_token) == TOK_REGISTER)
    {
        // processing for second register
        second_byte += convert_char_to_hex(convert_registers(reg_token)[1]);
        third_byte = "01";
        location_counter++; // increment for third byte
        instruction_bytes = first_byte + " " + second_byte + " " + third_byte;
    }
    else if (next_token.at(0) == '[') // [<reg>], [<reg> + <literal>] and [<reg> + <symbol>]
    {
        second_byte += "0"; // filling up second byte
        if (debug)
            std::cout << "Detected [ in ldr/str processing for " << next_token << std::endl;
        // parse to remove [
        next_token.erase(0, 1);

        // check ] exists - if exists then it is reg
        if (next_token.at(next_token.length() - 1) == ']') // [reg]
        {
            // remove ]
            next_token.erase(next_token.end() - 1);

            if (resolve_token_type(next_token) != TOK_REGISTER)
            {
                std::cout << "Error second register for ldr/str not provided" << std::endl;
                return false;
            }
            second_byte = "F" + convert_char_to_hex(convert_registers(next_token)[1]);
            third_byte = "02";
            location_counter++;
            instruction_bytes = first_byte + " " + second_byte + " " + third_byte;
        }
        else // [reg + literal], [reg + symbol]
        {

            second_byte = "F" + convert_char_to_hex(convert_registers(next_token)[1]);
            third_byte = "03";
            location_counter++;

            std::string second_operand = *(++token_iterator); // symbol or literal
            if (debug)
                std::cout << "Second operand for str/ldr [reg + lit/sym] insruction: " << second_operand << std::endl;
            // remove ] from second operand
            second_operand.erase(second_operand.end() - 1);
            if (is_literal(second_operand))
            {
                std::string parsed_literal = parse_literal(second_operand);
                fourth_byte += parsed_literal[0];
                fourth_byte += parsed_literal[1];
                fifth_byte += parsed_literal[2];
                fifth_byte += parsed_literal[3];
            }
            else
            {
                std::string symbol = second_operand;
                if (symbol_table.symbol_exists(symbol))
                {
                    unsigned int symbol_id = symbol_table.get_symbol_id(symbol);
                    unsigned int reloc_id = relocation_table.insert_relocation_record(current_section, location_counter, symbol_id, "R_386_16");
                    std::string reloc_id_in_hex = parse_literal(std::to_string(reloc_id));
                    fourth_byte += reloc_id_in_hex[0];
                    fourth_byte += reloc_id_in_hex[1];
                    fifth_byte += reloc_id_in_hex[2];
                    fifth_byte += reloc_id_in_hex[3];
                }
                else
                {
                    std::cout << "Symbol " << symbol << " does not exist" << std::endl;
                    return false;
                }
            }

            instruction_bytes = first_byte + " " + second_byte + " " + third_byte + " " + fourth_byte + " " + fifth_byte;
            location_counter += 2; // for fourth and fifth byte
        }
    }
    else // <Literal> or <Symbol>
    {
        second_byte += "0"; // filling up second byte

        //std::string parsed_token = next_token.substr(0, next_token.length() - 1);
        third_byte = "04";
        location_counter++; // for third byte

        // check if literal
        if (is_literal(next_token))
        {
            std::string parsed_literal = parse_literal(next_token);
            fourth_byte += parsed_literal[0];
            fourth_byte += parsed_literal[1];
            fifth_byte += parsed_literal[2];
            fifth_byte += parsed_literal[3];
        }
        else
        { // symbol
            std::string symbol = next_token;
            if (symbol_table.symbol_exists(symbol))
            {
                unsigned int symbol_id = symbol_table.get_symbol_id(symbol);
                unsigned int reloc_id = relocation_table.insert_relocation_record(current_section, location_counter, symbol_id, "R_386_16");
                std::string reloc_id_in_hex = parse_literal(std::to_string(reloc_id));
                fourth_byte += reloc_id_in_hex[0];
                fourth_byte += reloc_id_in_hex[1];
                fifth_byte += reloc_id_in_hex[2];
                fifth_byte += reloc_id_in_hex[3];
            }
            else
            {
                std::cout << "Symbol " << symbol << " does not exist" << std::endl;
                return false;
            }
        }
        instruction_bytes = first_byte + " " + second_byte + " " + third_byte + " " + fourth_byte + " " + fifth_byte;
        location_counter += 2; // for fourth and fifth byte
    }

    section_table.append_bytecode(current_section, instruction_bytes);
    return true;
}

bool Assembler::process_push_pop_instructions(std::string instruction) // maybe done
{

    std::string first_byte = "";
    std::string second_byte = "";
    std::string third_byte = "";
    std::string instruction_bytes = "";
    std::string reg_to_byte = "";
    // get next token and check if it is register
    std::string token = *(++token_iterator); // get next token
    Token_type token_type = resolve_token_type(token);
    if (token_type == TOK_REGISTER)
        second_byte = convert_char_to_hex(token[1]) + convert_char_to_hex(convert_registers("sp")[1]); // regD, sp
    else
    {
        std::cout << "Expected regS for push or pop" << std::endl;
        return false;
    }

    if (instruction.compare("pop") == 0)
    {
        first_byte = "A0"; // ldr
        third_byte = "4";
    }
    else if (instruction.compare("push") == 0)
    {
        first_byte = "B0"; // str
        third_byte = "1";
    }
    else
    {
        std::cout << "Unrecognized instruction, expceted push or pop" << std::endl;
        return false;
    }

    third_byte += "1";     // reg dir addressing
    location_counter += 3; // check if not incremented for second byte already
    instruction_bytes = first_byte + " " + second_byte + " " + third_byte;
    section_table.append_bytecode(current_section, instruction_bytes);
    return true;
}

std::string Assembler::convert_char_to_hex(char char_num)
{
    try
    {
        int number = char_num - '0';
        //std::cout << "Converting " << number << " to integer" << std::endl;
        std::stringstream sstream;
        sstream << std::hex << number;
        std::string hex_num = sstream.str();
        //std::cout << hex_num << std::endl;
        return hex_num;
    }
    catch (std::exception &e)
    {
        std::cout << "Standard exception: " << e.what() << std::endl;
        std::cout << "Exception in converting char to hex " << std::endl;
        return "";
    }
}

std::string Assembler::convert_registers(std::string reg)
{
    if (reg.compare("psw") == 0)
        return "r8";
    else if (reg.compare("pc") == 0)
        return "r7";
    else if (reg.compare("sp") == 0)
        return "r6";
    else
        return reg;
}

std::string Assembler::resolve_registers()
{
    std::string reg_token_one = *(++token_iterator);
    std::string reg_token_two = *(++token_iterator);
    std::string ret_val = "";
    if (resolve_token_type(reg_token_one) == TOK_REGISTER && resolve_token_type(reg_token_two) == TOK_REGISTER)
    {

        ret_val = convert_char_to_hex(convert_registers(reg_token_one)[1]) +
                  convert_char_to_hex(convert_registers(reg_token_two)[1]);
    }
    else
    {
        std::cout << "Error no registers for instruction" << std::endl;
    }
    return ret_val;
}

bool Assembler::is_literal(std::string literal)
{
    if (!literal.empty() && std::all_of(literal.begin(), literal.end(), ::isdigit))
    {
        return true;
    }
    else if (literal.compare(0, 2, "0x") == 0 && literal.size() > 2 && literal.find_first_not_of("0123456789abcdefABCDEF", 2) == std::string::npos)
    {
        return true;
    }

    return false;
}

std::string Assembler::parse_literal(std::string literal)
{

    // Decimal number transforming
    if (!literal.empty() && std::all_of(literal.begin(), literal.end(), ::isdigit))
    {
        // Convert decimal to hex
        //std::cout << " It is decimal: " << literal << std::endl;
        int number = std::stoi(literal);
        std::stringstream sstream;
        sstream << std::hex << number;
        std::string hex_format_literal = sstream.str();

        // insert zeros
        std::string transformed_literal = "";
        while (hex_format_literal.size() % 2 != 0 || hex_format_literal.size() < 3)
            hex_format_literal.insert(0, "0");

        //std::cout << "Hex format literal: " << hex_format_literal << std::endl;
        for (int i = hex_format_literal.size(); i > 0; i = i - 2)
        {
            // hex number 0xF5A4
            // transforming in pairs of 2 bytes, replace places of first and last
            // first iteration: transformed_literal = F5
            // second iteration: transformed_literal = A4F5
            transformed_literal += hex_format_literal.substr(i - 2, 1);
            transformed_literal += hex_format_literal.substr(i - 1, 1);
        }
        // std::cout << "Transfomrted literal to hex " << transformed_literal << std::endl;
        return transformed_literal;
    }
    // Hex number transforming
    else if (literal.compare(0, 2, "0x") == 0 && literal.size() > 2 && literal.find_first_not_of("0123456789abcdefABCDEF", 2) == std::string::npos)
    {
        std::string hex_format_literal = literal.substr(2, literal.length());
        //std::cout << "Hex number without 0x : " << hex_format_literal << std::endl;
        std::string transformed_literal = "";

        while (hex_format_literal.size() % 2 != 0 || hex_format_literal.size() < 3)
            hex_format_literal.insert(0, "0");

        for (int i = hex_format_literal.size(); i > 0; i = i - 2)
        {
            // hex number 0xF5A4
            // first iteration: transformed_literal = F5
            // second iteration: transformed_literal = A4F5
            transformed_literal += hex_format_literal.substr(i - 2, 1);
            transformed_literal += hex_format_literal.substr(i - 1, 1);
        }
        //std::cout << "Transfomrted literal to hex " << transformed_literal << std::endl;
        return transformed_literal;
    }
    else
    {
        std::cout << "Error: Unidentified literal" << std::endl;
    }

    return literal;
}

void Assembler::increment_counter_for_three_or_more_bytes_instruction(std::string instruction)
{

    if (instruction.compare("push") == 0 || instruction.compare("pop") == 0)
        location_counter += 3;
    else if (instruction.compare("ldr") == 0 || instruction.compare("str") == 0)
    {
        std::string next_token = *(++token_iterator); // get regD
        next_token = *(++token_iterator);             // get Operand

        if (resolve_token_type(next_token) == TOK_REGISTER) // reg (for ldr/str)
        {
            location_counter += 3;
        }
        else if (next_token.at(0) == '[')
        {
            // parse to remove [
            next_token.erase(0, 1);
            // check ] exists
            if (next_token.at(next_token.length() - 1) == ']') // [reg]
            {
                location_counter += 3;
            }
            else
                location_counter += 5;
        }
        else // in other cases location counter is 5
        {
            //std::cout << "Incrementing LC for 5" << std::endl;
            location_counter += 5;
        }
    }
    else // jmp instructions, call, ldr, str
    {
        // get next token
        std::string next_token = *(++token_iterator); // get operand

        if (next_token.at(0) == '*') // *literal, *symbol, *reg, *[reg], *[reg + literal], *[reg + symbol]
        {
            next_token.erase(0, 1);
            if (next_token.at(0) == '[') // *[reg]
            {
                // parse to remove [
                next_token.erase(0, 1);
                // check ] exists
                if (next_token.at(next_token.length() - 1) == ']') // *[reg]
                {
                    location_counter += 3;
                }
                else
                    location_counter += 5;
            }
            else // *reg
            {
                if (resolve_token_type(next_token) == TOK_REGISTER)
                {
                    location_counter += 3;
                }
                else
                {
                    std::cout << "Unexpected case in increment_counter_for_three_or_more_bytes_instruction" << std::endl;
                }
            }
        }
        else // in other cases location counter is 5
        {
            location_counter += 5;
        }
    }
}