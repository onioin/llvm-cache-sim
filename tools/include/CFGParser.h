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
#include <assert.h>


class CCFG_t{
private:
    char* s_;
    char* E_;
    char* b_;
    char* pol_;
    char* name_;
public:
    enum field{
        s, E, b, pol, name
    };
    CCFG_t() {s_ = nullptr; E_ = nullptr; b_ = nullptr; pol_ = nullptr; name_ = nullptr;}
    ~CCFG_t() {
        if(s_) delete s_;
        if(E_) delete E_;
        if(b_) delete b_;
        if(pol_) delete pol_;
        if(name_) delete name_;
    }
    void setField(field which, const char* val, size_t bytes){
        switch(which){
            case s:
                if(s_) delete s_;
                s_ = (char*) malloc(bytes+1);
                memcpy(s_, val, bytes);
                s_[bytes] = 0; //just in case
                break;
            case E:
                if(E_) delete E_;
                E_ = (char*) malloc(bytes+1);
                memcpy(E_, val, bytes);
                E_[bytes] = 0;
                break;
            case b:
                if(b_) delete b_;
                b_ = (char*) malloc(bytes+1);
                memcpy(b_, val, bytes);
                b_[bytes] = 0;
                break;
            case pol:
                if(pol_) delete pol_;
                pol_ = (char*) malloc(bytes+1);
                memcpy(pol_, val, bytes);
                pol_[bytes] = 0;
                break;
            case name:
                if(name_) delete name_;
                name_ = (char*) malloc(bytes+1);
                memcpy(name_, val, bytes);
                name_[bytes] = 0;
                break;
        }
    }
    char* getField(field which){
        switch(which){
            case s:
                assert(s_ && "Cannot get an empty field");
                return s_;
            case E:
                assert(E_ && "Cannot get an empty field");
                return E_;
            case b:
                assert(b_ && "Cannot get an empty field");
                return b_;
            case pol:
                assert(pol_ && "Cannot get an empty field");
                return pol_;
            case name:
                assert(name_ && "Cannot get an empty field");
                return name_;
        }
    }

};

json_value_t* parseJSONRoot(llvm::cl::Option&, llvm::StringRef);
bool parseCCFG(llvm::cl::Option&, json_value_t*, CCFG_t&);

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
    try {
        json_value_t* root = parseJSONRoot(O, Arg);
        return parseCCFG(O, root, V);
    }
    catch (std::string err){
        return O.error(err);
    }
}

json_value_t* parseJSONRoot(llvm::cl::Option &O, llvm::StringRef Arg){
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
        throw err.str();
    }
    free(cfg_buf);
    return json_root;
}


bool parseCCFG(llvm::cl::Option &O, json_value_t* json_root, CCFG_t &V){
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
                    V.setField(CCFG_t::name,
                               ((json_string_t*) val)->string,
                               ((json_string_t*) val)->string_size);
                    break;
                }
                if(!strcasecmp(name->string, "policy")){
                    V.setField(CCFG_t::pol,
                               ((json_string_t*) val)->string,
                               ((json_string_t*) val)->string_size);
                    break;
                }
                return O.error("An unexpected name/value pair of type string was encountered\n");
            case json_type_number:
                val = (void*) json_value_as_number((json_value_t*) val);
                if(!val){
                    return O.error("An unexpected error occurred while parsing\n");
                }
                if(!strcasecmp(name->string, "s")){
                    V.setField(CCFG_t::s,
                               ((json_number_t*) val)->number,
                               ((json_number_t*) val)->number_size);
                    break;
                }
                if(!strcasecmp(name->string, "b")){
                    V.setField(CCFG_t::b,
                               ((json_number_t*) val)->number,
                               ((json_number_t*) val)->number_size);
                    break;
                }
                if(!strcasecmp(name->string, "E")){
                    V.setField(CCFG_t::E,
                               ((json_number_t*) val)->number,
                               ((json_number_t*) val)->number_size);
                    break;
                }
                return O.error("An unexpected name/value pair of type number was encountered\n");
            default:
                return O.error("An invalid data type was encountered\n");
        }

        curr = curr->next;
    }

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
    OS << "Name: " << cfg.getField(CCFG_t::name) << "\n";
    OS << "Policy: " << cfg.getField(CCFG_t::pol) << "\n";
    OS << "Set Bits: " << cfg.getField(CCFG_t::s) << "\n";
    OS << "Block Bits: " << cfg.getField(CCFG_t::b) << "\n";
    OS << "Associativity: " << cfg.getField(CCFG_t::E) << "\n\n";
}

#endif //CACHEPASS_CFGPARSER_H
