//
// Created by onioin on 05/03/25.
//

#ifndef CACHEPASS_CFGPARSER_H
#define CACHEPASS_CFGPARSER_H

#include "llvm/Support/CommandLine.h"

#include "json.h"
#include "fileio.h"

#include <string>
#include <iostream>

//TODO: lift policy enum out of this file
enum EvictPol {
    lfu = 0, lru = 1, fifo = 2, rnd = 3
};

typedef struct CCFG {
    uint8_t s;
    uint8_t E;
    uint8_t b;
    uint8_t pol;
    char* name;
} CCFG_t;

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
    std::cout << ArgName.str() << ": " << Arg.str() << std::endl;
    return false;
}

uint8_t policyParse(const char* policy);

bool parseCCFGList(){
    return false;
}

bool parseCCFG(llvm::cl::Option &O, llvm::StringRef Arg, CCFG_t &V){
    char* cfg_buf = readfile(Arg.str().c_str());
    json_parse_result_t* result;
    json_value_t* json_root =
            json_parse_ex(cfg_buf, strlen(cfg_buf), json_parse_flags_allow_simplified_json,
                          NULL, NULL, result);
    if(json_parse_error_none != result->error){
        std::string err;
        switch(result->error){
            case json_parse_error_expected_comma_or_closing_bracket:
                err.assign("Expected a comma or a closing '}' or ']' in file: " + Arg.str() +
                " at " + result->error_line_no + ":" + result->error_row_no);
            case json_parse_error_expected_colon:
                err.assign("Expected a colon to separate a name/value pair in file: " + Arg.str() +
                " at " + result->error_line_no + ":" + result->error_row_no);
            case json_parse_error_expected_opening_quote:
                err.assign("Expected string to begin with '\"' in file: " + Arg.str() +
                " at " + result->error_line_no + ":" + result->error_row_no);
            case json_parse_error_invalid_string_escape_sequence:
                err.assign("Invalid escape sequence in file: " + Arg.str() +
                " at " + result->error_line_no + ":" + result->error_row_no);
            case json_parse_error_invalid_number_format:
                err.assign("Invalid number format in file: " + Arg.str() +
                " at " + result->error_line_no + ":" + result->error_row_no);
            case json_parse_error_invalid_value:
                err.assign("Invalid value in file: " + Arg.str() +
                " at " + result->error_line_no + ":" + result->error_row_no);
            case json_parse_error_premature_end_of_buffer:
                err.assign("File: " + Arg.str() + " ended before expected");
            case json_parse_error_invalid_string:
                err.assign("Malformed string in file: " + Arg.str() +
                " at " + result->error_line_no + ":" + result->error_row_no);
            case json_parse_error_allocator_failed:
                err.assign("Malloc failed :(");
            case json_parse_error_unexpected_trailing_characters:
                err.assign("Unexpected characters at end of file: " + Arg.str());
            case json_parse_error_unknown:
                err.assign("An unknown error occurred in file: " + Arg.str() +
                " at " + result->error_line_no + ":" + result->error_row_no);
        }
        return O.error("An error occurred while parsing the JSON: \n"
                       "    " + err + "\n");
    }

    if(json_type_object != json_root->type){
        return O.error("Top level of the parsed JSON should be an object\n");
    }
    json_object_t* object = (json_object_t*) (json_root->payload);
    if(5 != object->length){
        return O.error("JSON object contains too " +
            (5<object->length ? "many ": "few ") +
            "name/value pairs\n");
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
                if(!strcmp(tolower(name->string), "name")){
                    V.name = malloc(((json_string_t*) val)->string_size);
                    memcpy(V.name, ((json_string_t*) val)->string,
                           ((json_string_t*) val)->string_size);
                    break;
                }
                if(!strcmp(tolower(name->string), "policy")){
                    V.pol = policyParse(((json_string_t*) val)->string);
                    if(255 == V.pol){
                        return O.error("An invalid cache replacement policy was entered\n");
                    }
                    break;
                }
                return O.error("An unexpected name/value pair of type string was encountered\n");
            case json_type_number:
                val = (void*) json_value_as_number((json_value_t*) val);
                if(!val){
                    return O.error("An unexpected error occurred while parsing\n");
                }
                break;
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

uint8_t policyParse(const char* policy){
    if(!strcmp(tolower(policy), "lfu")){
        return lfu;
    }
    if(!strcmp(tolower(policy), "lru")){
        return lru;
    }
    if(!strcmp(tolower(policy), "fifo")){
        return fifo;
    }
    if(!strcmp(tolower(policy), "rand") || !strcmp(tolower(policy), "random")){
        return rnd;
    }
    return 255;
}

#endif //CACHEPASS_CFGPARSER_H
