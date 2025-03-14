//
// Created by onioin on 14/03/25.
//

#ifndef CACHEPASS_CFGLISTPARSER_H
#define CACHEPASS_CFGLISTPARSER_H

#include "llvm/Support/CommandLine.h"

#include "json.h"

#include <string>
#include <sstream>

#include <iostream>

#include "LinkedList.h"
#include "CFGParser.h"

using osl::LinkedList;

json_value_t* parseJSONRoot(llvm::cl::Option&, llvm::StringRef);
void CCFGPrint(std::ostream &OS, CCFG_t cfg);
bool parseCCFG(llvm::cl::Option &O, json_value_t* json_root, CCFG_t &V);

bool parseCCFGList (llvm::cl::Option&, json_value_t*, LinkedList<CCFG_t*>*&);

template <> class llvm::cl::parser<LinkedList<CCFG_t*>*> : public basic_parser<LinkedList<CCFG_t*>*>{
public:
    parser(llvm::cl::Option &O) : basic_parser(O) {}

    bool parse(llvm::cl::Option &O, llvm::StringRef ArgName,
               llvm::StringRef Arg, LinkedList<CCFG_t*>* &V);

    llvm::StringRef getValueName() const override {return "Cache Config";}

    void printOptionDiff(const llvm::cl::Option &O, LinkedList<CCFG_t*>* V,
                         llvm::cl::OptionValue<LinkedList<CCFG_t*>*> Default, size_t GlobalWidth) const;

    void anchor() override {};
};

bool llvm::cl::parser<LinkedList<CCFG_t*>*>::parse(llvm::cl::Option &O, llvm::StringRef ArgName,
                                                 llvm::StringRef Arg, LinkedList<CCFG_t*>* &V){
    try{
        json_value_t* root = parseJSONRoot(O, Arg);
        bool ret = parseCCFGList(O, root, V);
        free(root);
        return ret;
    }
    catch (std::string err){
        return O.error(err);
    }
}

bool parseCCFGList(llvm::cl::Option& O, json_value_t* json_root, LinkedList<CCFG_t*>*& V){
    if(json_type_array != ((json_object_t*)json_root->payload)->start->value->type){
        return O.error("The top level JSON object must contain a list");
    }
    if(1 != ((json_object_t*)json_root->payload)->length){
        return O.error("The JSON should contain one list only");
    }
    V = new osl::LinkedList<CCFG_t*>();
    //std::cout << (char*)json_write_pretty(json_root, NULL, NULL, NULL);

    json_array_t* CFGList = (json_array_t*)(((json_object_t*)json_root->payload)->start->value->payload);

    json_array_element_t* curr = CFGList->start;
    for(size_t i = 0; i < CFGList->length; ++i){
        CCFG_t* ccfg = new CCFG_t();
        bool err = parseCCFG(O, curr->value, *ccfg);
        if(err){
            return err;
        }
        V->append(ccfg);
        curr = curr->next;
    }

    return false;
}

void llvm::cl::parser<LinkedList<CCFG_t*>*>::printOptionDiff(const llvm::cl::Option &O, LinkedList<CCFG_t*>* V,
                                                           llvm::cl::OptionValue<LinkedList<CCFG_t*>*> Default,
                                                           size_t GlobalWidth) const{
    printOptionName(O, GlobalWidth);
    std::string Str("hello world");
    //TODO: put something useful in Str

    outs() << "= " << Str << "\n";
}

void printCCFGList(std::ostream &OS, LinkedList<CCFG_t*>* LL){
    for(auto cfg : *LL){
        CCFGPrint(OS, *cfg);
    }
    std::cout << *LL;
}

#endif //CACHEPASS_CFGLISTPARSER_H
