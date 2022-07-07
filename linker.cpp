#include <iostream>
#include <string>
#include <cstring>
#include <map>
#include <set>
#include <vector>

using namespace std;

int linenum = 0;
int lineoffset = 0;
FILE *file;
string errstring;
static int tokenlength = 0;
const char* DELIM = " \t\n\r\v\f";
char* tok;
char line[1024];
bool eofFlag=false;
map<string, vector<int> > symtable;
set<string> def_used;
set<string> used_throughout;
int memory_map_len=0;
bool validSym(char *c);
bool validInstruction(char *c);
void Pass2();

void __parseerror(int errcode) {
    static char* errstr[] = {
            "NUM_EXPECTED", // Number expect, anything >= 2^30 is not a number either
            "SYM_EXPECTED", // Symbol Expected
            "ADDR_EXPECTED", // Addressing Expected which is A/E/I/R
            "SYM_TOO_LONG", // Symbol Name is too long
            "TOO_MANY_DEF_IN_MODULE", // > 16
            "TOO_MANY_USE_IN_MODULE", // > 16
            "TOO_MANY_INSTR", // total num_instr exceeds memory size (512)
    };
    printf("Parse Error line %d offset %d: %s\n", linenum,lineoffset,errstr[errcode]);
    exit(-1);
}

char * getToken()
{
    while(!eofFlag) {
        if (tok==NULL) {
            if (fgets(line, 4096, file) == NULL) {
                lineoffset = lineoffset + tokenlength;
                eofFlag = true;
                return NULL;
            }
            if((strcmp(line,"\r\n") == 0)||(strcmp(line,"\0") == 0)||(strcmp(line,"\n") == 0)){
                tokenlength = 0;
                linenum ++;
                lineoffset = 1;
                continue;
            }
            linenum++;
            tok = strtok(line, DELIM);
            if (tok == NULL) // no tokens in line
                continue; // we try with next line
            tokenlength = strlen(tok);
            lineoffset = tok - line + 1;
            return tok;
        }
        tok = strtok(NULL, DELIM);
        if (tok != NULL){
            lineoffset = tok - line + 1;
            tokenlength = strlen(tok);// found a token
           return tok;
        }
    }
}
bool isNum(char* s){
    char * t;
    if(s==NULL) {
        return false;
    }
    for (int i=0; i<(strlen(s)); i++){
        if(!isdigit(s[i])) return false;
    }
    return true;
}

int readInt() {
    char *c = getToken();
    if(c==NULL && eofFlag) return -1;
    try {
    if (isNum(c)) {
        int n = atoi(c);
        return n;
    } else if(eofFlag){
    }
        __parseerror(0);
    }
    catch (exception e){
        __parseerror(0);
    }
}

char* readSym(){
    char* c = getToken();
    if (validSym(c)){
      return c;
    }
    else {
        __parseerror(1);
    }
}

bool validSym(char *s) {
   // char * t = s;
    if(s==NULL) {
        return false;
    }
    if(!isalpha(s[0])) {
        return false;
    }
   // int size =1;
    for (int i=1; i<(strlen(s)); i++){
        if(!isalnum(s[i])) return false;
       // t++;
      //  size++;
    }
    if(strlen(s)>16) {
        __parseerror(3);
        return false;
    }

    return true;
}

char readIAER(){
    char* c = getToken();
    if(validInstruction(c)){
       return *c;
    }
    else {
        __parseerror(2);
    }
}

bool validInstruction(char *c) {
    char * t = c;
    if(c==NULL) {
        return false;
    }
    int size =0;
    if(*t=='\0') {
        return false;
    }
    while(*t != '\0')  {
        if(!isalpha(*t)) return false;
        t++;
        size++;
    }

    if(size>1 || !(*c=='R'|| *c=='E'|| *c=='I'|| *c=='A')) {
        return false;
    }
   return true;
}

void Pass1() {
    int baseAdd;
    int moduleNu=0;
    int prevMAdd=0, prevML=0;
    while (!eofFlag) {
        moduleNu++;
        baseAdd = prevMAdd+prevML;
        int defcount = readInt();
        if(defcount>16) {
            __parseerror(4);
        }
        if(defcount==-1 && eofFlag) break;
        if (eofFlag) {break;}
        vector<pair<string,int> > symbolDecla;
        for (int i = 0; i < defcount; i++) {
            char* sym = readSym();
            int val = readInt();
            if (symtable.count(sym)==1){
               def_used.insert(sym); //symbol declared before
            }
            else{
                symbolDecla.push_back(pair<string, int> (sym,val));
            }
        }
        int usecount = readInt();
        if(usecount>16) {
            __parseerror(5);
        }
        for (int i = 0; i < usecount; i++) {
            char* sym = readSym();
        }
        int instcount = readInt();
        if(instcount + baseAdd>=512){
            __parseerror(6);
        }
        for (int i = 0; i < instcount; i++) {
            char addressmode = readIAER();
            int operand = readInt();

        }
        prevMAdd = baseAdd;
        prevML = instcount;
        vector< pair<string, int> >::iterator it;
        for (it = symbolDecla.begin(); it != symbolDecla.end(); it++) { //Rule 5
            int val=0;
            if(it->second<instcount){
                val = it->second;
            }
            else{
                printf("Warning: Module %d: %s too big %d (max=%d) assume zero relative\n",
                       moduleNu,it->first.c_str(),it->second,instcount-1);
            }
            vector<int> add_mod;
            add_mod.push_back(moduleNu);
            add_mod.push_back(baseAdd+val);
            symtable.insert(pair<string, vector<int> >(it->first, add_mod));
        }
    }
    map<string, vector<int> >::iterator it;
    printf("Symbol Table"); //No line after symbol table
    //bool err=false;
    for (it = symtable.begin(); it != symtable.end(); it++) {
        if(def_used.count(it->first)){
            errstring="Error: This variable is multiple times defined; first value used\n";
            def_used.erase(it->first);
        }
        else{
            errstring="";
        }
        printf("\n%s=%d",it->first.c_str(),it->second[1]);
        if(!errstring.empty())
            printf(" %s",errstring.c_str());
    }
}

void Pass2() {
    errstring="";
    int baseAdd=0;
    int moduleNu=0;
    int prevMAdd=0, prevML=0;
    printf("\n\nMemory Map");
    while (!eofFlag) {
        moduleNu++;
        baseAdd = prevMAdd+prevML;
        int defcount = readInt();
        if(defcount==-1 && eofFlag) break;
       if (eofFlag) break;
        for (int i = 0; i < defcount; i++) {
            char* sym = readSym();
            int val = readInt();
        }
        vector<string> uselist_symbols;
        int usecount = readInt();
        for (int i = 0; i < usecount; i++) {
            char* sym = readSym();
            uselist_symbols.push_back(sym);
        }
        int instcount = readInt();

        for (int i = 0; i < instcount; i++) {
            char addressmode = readIAER();
            int oper = readInt();
            int opcode = oper/1000;
            int operand = oper%1000;
            errstring="";
            int absolute_add;
            if(opcode>9){
                if(addressmode!='I')
                errstring="Error: Illegal opcode; treated as 9999";
                else {
                    errstring = "Error: Illegal immediate value; treated as 9999";
                }
                absolute_add=9999;
            }
            else if(addressmode=='R'){ //Relative Adderess+BaseAdd
                if(operand>=instcount){
                    errstring="Error: Relative address exceeds module size; zero used";
                    operand=0;
                }
                if(operand+baseAdd>=512){
                    errstring="Error: Absolute address exceeds machine size; zero used";
                    absolute_add=0;
                }
                else{
               absolute_add = opcode*1000 + operand + baseAdd;
                }
            }
            else if(addressmode=='E'){
                string sym;
                if(operand>=uselist_symbols.size()){
                    errstring = "Error: External address exceeds length of uselist; treated as immediate";
                    absolute_add = oper;
                }
                else {
                    sym = uselist_symbols.at(operand);
                    if (symtable.empty() || !symtable.count(sym)) {
                        sym = uselist_symbols.at(operand);
                        absolute_add = opcode * 1000;
                        std::string someString(sym);
                        errstring = "Error: " + someString + " is not defined; zero used";
                    } else {
                       operand = symtable[sym][1];
                        absolute_add = opcode * 1000 + operand;
                        used_throughout.insert(sym);
                    }
                    def_used.insert(sym);
                }
            }
            else if(addressmode=='A'){
                absolute_add = opcode*1000+operand;
                if(operand>=512){
                    errstring="Error: Absolute address exceeds machine size; zero used";
                    absolute_add=opcode*1000;
                }
            }
            else if(addressmode=='I'){
                absolute_add = oper;
            }
            printf("\n%03d: %04d",memory_map_len,absolute_add,errstring.c_str());
            if(!errstring.empty())
                printf(" %s",errstring.c_str());
            memory_map_len++;
            errstring="";
        }
        prevMAdd = baseAdd;
        prevML = instcount;
        for(string i : uselist_symbols){
            if(def_used.empty() || !def_used.count(i)){
                printf("\nWarning: Module %d: %s appeared in the uselist but was not actually used",moduleNu,i.c_str());
            }
        }
        uselist_symbols.clear();
        def_used.clear();
    }
    cout<<'\n';
    map<string,vector<int> >::iterator it;
    if(!symtable.empty()) { //rule 4
        for (it = symtable.begin(); it != symtable.end(); it++) {
            if (!used_throughout.count(it->first)) {
                used_throughout.erase(it->first);
                printf("\nWarning: Module %d: %s was defined but never used", it->second[0], it->first.c_str());
            }
        }
    }
    cout<<"\n\n";
    }

int main(int argc, char **argv) {
    const string inputfile = string(argv[1]);
    file = fopen(inputfile.c_str(), "r");
    Pass1();
    eofFlag = false;
    fseek(file, 0, SEEK_SET);
    Pass2();
    fclose(file);
}
