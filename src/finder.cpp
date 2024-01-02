#include <clang-c/Index.h>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <ostream>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <list>
#include <sstream>
#include <set>

// #define COLORED

#ifdef COLORED
#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */ 
#endif
#ifndef COLORED
#define RESET   ""
#define BLACK   ""      /* Black */
#define RED     ""      /* Red */
#define GREEN   ""      /* Green */
#define YELLOW  ""      /* Yellow */
#define BLUE    ""      /* Blue */
#define MAGENTA ""      /* Magenta */
#define CYAN    ""      /* Cyan */
#define WHITE   ""      /* White */ 
#endif

using namespace std;

ofstream out("./EnumerationFinder.dump", ios::app);

void printSourceLines(CXCursor cursor, unsigned contextLines = 2, bool show_marker = false);

void proxy(int argc, char* argv[]);

void analize(CXTranslationUnit tu, CXCursor rootCursor);

CXChildVisitResult findEnumsEx(CXCursor cursor, CXCursor parent, CXClientData client_data);
CXChildVisitResult collectEnumVarDecl(CXCursor cursor, CXCursor parent, CXClientData clientData);
CXChildVisitResult collectEnumVarUses(CXCursor cursor, CXCursor parent, CXClientData clientData);


struct InfoCollected{
    list<CXCursor*>* enum_vars_decl_ptr;
    list<CXCursor*>* enum_vars_uses_ptr;
    set<string>*     whitelist;
};

int main(int argc, char* argv[]){
    char* cc = getenv( "REALCC" );
    char* cxx = getenv( "REALCXX" );

    if(cc == NULL || cxx == NULL){
        out << "ENV  Variables REALCC and REALCXX must be defined!"<<endl;
        out.close();
        throw std::invalid_argument("ENV  Variables REALCC and REALCXX must be defined!");
    }

    // Needed for pass test in compilation when build libraries
    std::string programName = argv[0];
    if (programName.substr(programName.size() - 2) == "++") {
        argv[0] = const_cast<char*>(cxx);
    } else {
        argv[0] = const_cast<char*>(cc);
    }

    // call proxy function
    proxy(argc, argv);

    // Needed for pass test in compilation when build libraries
    int ret;
    try{
        ret = execvp(argv[0], argv);
    } catch (...) {
        return ret;
    }
    return ret;
}

// Implementazione della funzione proxy
void proxy(int argc, char* argv[]){
    
    CXIndex index = clang_createIndex(/*excludeDeclarations=*/1, /*displayDiagnostics=*/1);

    const char **args = new const char*[argc];
    for (int i = 0; i < argc; ++i) {
        args[i] = argv[i];
    }

    CXTranslationUnit tu = clang_parseTranslationUnit(
        index, /*source_filename=*/nullptr,
        args, argc, /*unsaved_files=*/nullptr, 0,
        CXTranslationUnit_None);

    CXCursor cursor = clang_getTranslationUnitCursor(tu);

    analize(tu, cursor);

    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);
    return;
}

CXChildVisitResult findEnumsEx(CXCursor cursor, CXCursor parent, CXClientData clientData) {
    // CXTranslationUnit tu = (CXTranslationUnit) client_data;

    struct InfoCollected* data = (struct InfoCollected*) clientData;
    list<CXCursor*>* enum_vars_decl_ptr = data->enum_vars_decl_ptr;
    list<CXCursor*>* enum_vars_uses_ptr = data->enum_vars_uses_ptr;
    set<string>* whitelist_enums_ptr    = data->whitelist;
    
    CXString kind = clang_getCursorKindSpelling(cursor.kind);
    string s_kind = clang_getCString(kind);
    clang_disposeString(kind);
    
    
    CXString pkind = clang_getCursorKindSpelling(parent.kind);
    string s_pkind = clang_getCString(pkind);
    clang_disposeString(pkind);

    
    if(s_kind != "EnumDecl")return CXChildVisit_Recurse;
        
    string type_name = "";
    CXCursor cursor_ua;
    string kind_ua = "";
    if(s_pkind == "TypedefDecl"){
        type_name =  clang_getCString(clang_getCursorSpelling(parent));
        cursor_ua = parent;
        kind_ua = s_pkind;
    }else {
        type_name =  clang_getCString(clang_getCursorSpelling(cursor));
        cursor_ua = cursor;
        kind_ua = s_kind;
    }
    
    if(type_name == "")return CXChildVisit_Recurse;
    
    bool filter_active = whitelist_enums_ptr->size() != 0;

    bool not_in_whitelist = whitelist_enums_ptr->find(type_name) == whitelist_enums_ptr->end();
    if( filter_active && not_in_whitelist ) return CXChildVisit_Recurse;

    CXType varType = clang_getCursorType(cursor);
    CXType canonicalType = clang_getCanonicalType(varType);


    // CXString varTypeCXS =  clang_getTypeSpelling(varType);
    // CXString canonicalTypeCXS =  clang_getTypeSpelling(canonicalType);

    // out <<"\t"<<RED<< "VTCS "<<clang_getCString(varTypeCXS)<< " CTCS "<< clang_getCString(canonicalTypeCXS)<<RESET<<endl;

    bool is_first_time_here = true;

    for(auto ccursor : *enum_vars_decl_ptr){



        CXType varTypeDEF = clang_getCursorType(*ccursor);
        CXType canonicalTypeDEF = clang_getCanonicalType(varTypeDEF);

        CXType pointeeTypeDEF = clang_getPointeeType(varTypeDEF);
        CXType canonicalTypePtDEF = clang_getCanonicalType(pointeeTypeDEF);

        if (!clang_equalTypes(varType,  varTypeDEF) && !clang_equalTypes(canonicalType,  canonicalTypeDEF) && !clang_equalTypes(varType,  pointeeTypeDEF) && !clang_equalTypes(canonicalType,  canonicalTypePtDEF))
            continue;
        
        if(is_first_time_here){
            // get info over the file (where the cursor is)
            unsigned int line, col;
            CXFile file;
            CXSourceLocation location = clang_getCursorLocation(cursor_ua);
            clang_getSpellingLocation(location, &file, &line, &col, NULL);
            string file_path = clang_getCString(clang_getFileName(file));
            out <<RED<< kind_ua << " with name: " << type_name <<" at line"<< line<< ":"<< col <<" in file: "<< file_path <<RESET<<"\n";

            printSourceLines(cursor, 0);
            out<<endl;
            is_first_time_here = false;
        }

        out<<CYAN<<"Variable Found"<<RESET<<endl;
        // CXString varTypeCXSDEF =  clang_getTypeSpelling(varTypeDEF);
        // CXString canonicalTypeCXSDEF =  clang_getTypeSpelling(canonicalTypeDEF);
        // CXString pointeeTypeCXSDEF =  clang_getTypeSpelling(pointeeTypeDEF);
        // CXString canonicalTypePtCXSDEF =  clang_getTypeSpelling(canonicalTypePtDEF);
        // out << clang_equalTypes(varType,  varTypeDEF) << clang_equalTypes(canonicalType,  canonicalTypeDEF) << clang_equalTypes(varType,  pointeeTypeDEF) << clang_equalTypes(canonicalType,  canonicalTypePtDEF) 
        //      <<GREEN<< " VTCS "<<clang_getCString(varTypeCXSDEF)<< " CTCS "<< clang_getCString(canonicalTypeCXSDEF)<< " PTCS "<< clang_getCString(pointeeTypeCXSDEF)<< " CTPCS "<< clang_getCString(canonicalTypePtCXSDEF)<<RESET<<endl;
        // clang_disposeString(varTypeCXSDEF);
        // clang_disposeString(canonicalTypeCXSDEF);
        // clang_disposeString(pointeeTypeCXSDEF);
        // clang_disposeString(canonicalTypePtCXSDEF);

        printSourceLines(*ccursor, 0, true);

        for(auto x:*enum_vars_uses_ptr){
            CXCursor referencedCursor = clang_getCursorReferenced(*x);
            if(!clang_equalCursors(referencedCursor, *ccursor)) continue;
            // out<<"yup\n";
            out<<YELLOW<<"Used"<<RESET<<endl; 
            printSourceLines(*x, 0, true);
        }
        out<<endl;

    } 

    return CXChildVisit_Recurse;
}

CXChildVisitResult collectEnumVarDecl(CXCursor cursor, CXCursor parent, CXClientData clientData){
    list<CXCursor*>* enum_vars_decl_ptr = static_cast<list<CXCursor*>*>(clientData);

    if (cursor.kind != CXCursor_VarDecl && 
        cursor.kind != CXCursor_MemberRefExpr) return CXChildVisit_Recurse;
    

    CXType varType = clang_getCursorType(cursor);
    CXType canonicalType = clang_getCanonicalType(varType);

    if (canonicalType.kind == CXType_Enum || canonicalType.kind == CXType_Typedef) {
        CXCursor enumDecl = clang_getTypeDeclaration(canonicalType);
        if (enumDecl.kind == CXCursor_EnumDecl) {
            // Il tipo della variabile è un'enumerazione
            enum_vars_decl_ptr->push_back(new CXCursor(cursor));
        }
    }

    if (varType.kind == CXType_Pointer) {
        CXType pointeeType = clang_getPointeeType(varType);
        CXType canonicalType = clang_getCanonicalType(pointeeType);
        if (canonicalType.kind == CXType_Enum ) {
            CXCursor enumDecl = clang_getTypeDeclaration(canonicalType);
            if (enumDecl.kind == CXCursor_EnumDecl) {
                // Il tipo della variabile è un puntatore a un'enumerazione o a una typedef ad enumerazione
                enum_vars_decl_ptr->push_back(new CXCursor(cursor));
            }
        }
    }

    return CXChildVisit_Continue;
}

CXChildVisitResult collectEnumVarUses(CXCursor cursor, CXCursor parent, CXClientData clientData){
    list<CXCursor*>* enum_vars_use_ptr = static_cast<list<CXCursor*>*>(clientData);


    // Verifica se il cursore corrente è un riferimento alla variabile
    if (cursor.kind == CXCursor_DeclRefExpr) {
        CXCursor referencedCursor = clang_getCursorReferenced(cursor);
        //CXCursorKind referencedKind = clang_getCursorKind(referencedCursor);

        if (referencedCursor.kind == CXCursor_VarDecl) {
            // Salva il cursore del riferimento
            enum_vars_use_ptr->push_back(new CXCursor(cursor));
        }
    }
  // Continua a visitare i figli del cursore corrente
  return CXChildVisit_Recurse;
}

void analize(CXTranslationUnit tu, CXCursor rootCursor) {

    char* filter_by = getenv("SHOW_ONLY");
    bool filter_active = filter_by != nullptr;
    string enums_white = filter_active ? filter_by : "";

    set<string> whitelist_enums = {};
    stringstream ssin(enums_white);

    while (ssin.good()){
        string tmp;
        ssin >> tmp;
        if(tmp == "") continue;
        whitelist_enums.insert(tmp);
    }


    
    list<CXCursor*> enum_vars_decl = {};
    list<CXCursor*> enum_vars_uses = {};
    clang_visitChildren(rootCursor, collectEnumVarDecl, (void*)&enum_vars_decl);
    clang_visitChildren(rootCursor, collectEnumVarUses, (void*)&enum_vars_uses);

    struct InfoCollected data;
    data.enum_vars_decl_ptr = &enum_vars_decl;    
    data.enum_vars_uses_ptr = &enum_vars_uses; 
    data.whitelist = &whitelist_enums;  

    clang_visitChildren(rootCursor, findEnumsEx, (void*)&data);

    for(auto c:enum_vars_decl){
        delete c;
    }

    for(auto c:enum_vars_uses){
        delete c;
    }

}

void printSourceLines(CXCursor cursor, 
                      unsigned contextLines, 
                      bool show_marker) {
    
    CXFile cx_file;
    unsigned int line, column;
    clang_getSpellingLocation(clang_getCursorLocation (cursor), &cx_file, &line, &column, nullptr);
    string filePath = clang_getCString(clang_getFileName(cx_file));

    ifstream file(filePath);
    string sourceLine;
    
    CXSourceRange range = clang_getCursorExtent(cursor);
    CXSourceLocation start_loc = clang_getRangeStart(range);
    CXSourceLocation end_loc = clang_getRangeEnd(range);
    
    CXFile _;
    unsigned int start_line, start_col;
    clang_getSpellingLocation(start_loc, &_, &start_line, &start_col, NULL);
    unsigned int end_line, end_col;
    clang_getSpellingLocation(end_loc, &_, &end_line, &end_col, NULL);

    // my_out << "at "<<start_line<<" start - end "<< end_line<<"\n";
    out<< "@ "<< filePath << endl;
    // Iterate over each line, printing lines in the specified range around the desired line
    unsigned print_from_line = start_line > contextLines ? start_line - contextLines : 1;
    unsigned print_to_line = end_line + contextLines;
    for (unsigned i = 1; i <= print_to_line && getline(file, sourceLine); i++) {
        if (i >= print_from_line) {
            out << i << ": " << sourceLine << "\n";
            if (i == line && show_marker) {
                out<< i <<":"<<string(column, ' ') << "^\n";
            }
        }
    }
}
