//
// Created by onioin on 05/03/25.
//

#ifndef CACHEPASS_CFGPARSER_H
#define CACHEPASS_CFGPARSER_H

#include "llvm/Support/CommandLine.h"

#include "json.h"
#include "fileio.h"

#include <string>
#include <sstream>
//#include <iostream>
#include <stdlib.h>

typedef struct CCFG {
    int* s;
    int* E;
    int* b;
    char* pol;
    char* name;
} CCFG_t;

bool parseCCFG(llvm::cl::Option &O, llvm::StringRef Arg, CCFG_t &V);

template <> class llvm::cl::parser<CCFG_t> : public basic_parser<CCFG_t>{
public:
    parser(llvm::cl::Option &O) : basic_parser(O) {}

    bool parse(llvm::cl::Option &O, llvm::StringRef ArgName,
               llvm::StringRef Arg, CCFG_t &V);

    llvm::StringRef getValueName() const override {return "Cache Config";}

    void printOptionDiff(const llvm::cl::Option &O, CCFG_t V,
                         llvm::cl::OptionValue<CCFG_t> Default, size_t GlobalWidth) const;

    void anchor() override {};

};

bool llvm::cl::parser<CCFG_t>::parse(llvm::cl::Option &O, llvm::StringRef ArgName,
                                    llvm::StringRef Arg, CCFG_t &V){
    return parseCCFG(O, Arg, V);
}


bool parseCCFGList(){
    return false;
}

bool parseCCFG(llvm::cl::Option &O, llvm::StringRef Arg, CCFG_t &V){
    char* cfg_buf = readfile(Arg.str().c_str());
    auto* result = (json_parse_result_t*) malloc(sizeof(json_parse_result_t));
    json_value_t* json_root =
            json_parse_ex(cfg_buf, strlen(cfg_buf), json_parse_flags_allow_simplified_json,
                          NULL, NULL, result);
    if(json_parse_error_none != result->error){
        std::stringstream err;
        err << "An error occurred while parsing the JSON:\n\t";
        switch(result->error){
            case json_parse_error_expected_comma_or_closing_bracket:
                err << "Expected a comma or a closing '}' or ']' in file: " << Arg.str() <<
                " at " << result->error_line_no << ":" << result->error_row_no;
                break;
            case json_parse_error_expected_colon:
                err << "Expected a colon to separate a name/value pair in file: " << Arg.str() <<
                " at " << result->error_line_no << ":" << result->error_row_no;
                break;
            case json_parse_error_expected_opening_quote:
                err << "Expected string to begin with '\"' in file: " << Arg.str() <<
                " at " << result->error_line_no << ":" << result->error_row_no;
                break;
            case json_parse_error_invalid_string_escape_sequence:
                err << "Invalid escape sequence in file: " << Arg.str() <<
                " at " << result->error_line_no << ":" << result->error_row_no;
                break;
            case json_parse_error_invalid_number_format:
                err << "Invalid number format in file: " << Arg.str() <<
                " at " << result->error_line_no << ":" << result->error_row_no;
                break;
            case json_parse_error_invalid_value:
                err << "Invalid value in file: " << Arg.str() <<
                " at " << result->error_line_no << ":" << result->error_row_no;
                break;
            case json_parse_error_premature_end_of_buffer:
                err << "File: " << Arg.str() << " ended before expected";
                break;
            case json_parse_error_invalid_string:
                err << "Malformed string in file: " + Arg.str() << Arg.str() <<
                " at " << result->error_line_no << ":" << result->error_row_no;
                break;
            case json_parse_error_allocator_failed:
                err << "Malloc failed :(";
                break;
            case json_parse_error_unexpected_trailing_characters:
                err << "Unexpected characters at end of file: " << Arg.str();
                break;
            case json_parse_error_unknown:
                err << "An unknown error occurred in file: " << Arg.str() <<
                " at " << result->error_line_no << ":" << result->error_row_no;
                break;
        }
        err << "\n";
        return O.error(err.str());
    }

    //std::cout << (char*) json_write_pretty(json_root, NULL, NULL, NULL) << std::endl;

    if(json_type_object != json_root->type){
        return O.error("Top level of the parsed JSON should be an object\n");
    }
    json_object_t* object = (json_object_t*) (json_root->payload);
    if(5 != object->length){
        std::stringstream err;
        err << "Expected 5 name/value pairs, received " << object->length << ".\n";
        return O.error(err.str());
    }
    auto* curr = object->start;

    while(NULL != curr){
        auto* name = curr->name;
        void* val = (void*) curr->value;

        switch(((json_value_t*) val)->type){
            case json_type_string:
                val = (void*) json_value_as_string((json_value_t*) val);
                if(!val){
                    return O.error("An unexpected error occurred while parsing\n");
                }
                if(!strcasecmp(name->string, "name")){
                    V.name = (char*) malloc(((json_string_t*) val)->string_size + 1);
                    memcpy(V.name, ((json_string_t*) val)->string,
                           ((json_string_t*) val)->string_size);
                    V.name[((json_string_t*) val)->string_size] = 0;
                    break;
                }
                if(!strcasecmp(name->string, "policy")){
                    V.pol = (char*) malloc(((json_string_t*) val)->string_size + 1);
                    memcpy(V.pol, ((json_string_t*) val)->string,
                           ((json_string_t*) val)->string_size);
                    V.pol[((json_string_t*) val)->string_size] = 0;
                    break;
                }
                return O.error("An unexpected name/value pair of type string was encountered\n");
            case json_type_number:
                val = (void*) json_value_as_number((json_value_t*) val);
                if(!val){
                    return O.error("An unexpected error occurred while parsing\n");
                }
                if(!strcasecmp(name->string, "s")){
                    V.s = (int*) malloc(4);
                    *(V.s) = atoi(((json_number_t*) val)->number);
                    break;
                }
                if(!strcasecmp(name->string, "b")){
                    V.b = (int*) malloc(4);
                    *(V.b) = atoi(((json_number_t*) val)->number);
                    break;
                }
                if(!strcasecmp(name->string, "E")){
                    V.E = (int*) malloc(4);
                    *(V.E) = atoi(((json_number_t*) val)->number);
                    break;
                }
                return O.error("An unexpected name/value pair of type number was encountered\n");
            default:
                return O.error("An invalid data type was encountered\n");
        }

        curr = curr->next;
    }

    free(cfg_buf);
    free(json_root);
    return false;
}

void llvm::cl::parser<CCFG_t>::printOptionDiff(const llvm::cl::Option &O, CCFG_t V,
                                              llvm::cl::OptionValue<CCFG_t> D,
                                              size_t GlobalWidth) const {
    printOptionName(O, GlobalWidth);
    std::string Str("hello world");
    //TODO: put something useful in Str

    outs() << "= " << Str << "\n";

}

void CCFGPrint(std::ostream &OS, CCFG_t cfg){
    OS << "Name: " << cfg.name << "\n";
    OS << "Policy: " << *cfg.pol << "\n";
    OS << "Set Bits: " << *cfg.s << "\n";
    OS << "Block Bits: " << *cfg.b << "\n";
    OS << "Associativity: " << *cfg.E << "\n\n";
}

#endif //CACHEPASS_CFGPARSER_H
