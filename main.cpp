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
map<string, vector<int>> symtable;
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
    //cout  << errstr[errcode] << endl;
    printf("Parse Error line %d offset %d: %s\n", linenum,lineoffset,errstr[errcode]);
    exit(-1);
}

char * getToken()
{
    while(!eofFlag) {
        if (tok==NULL) {

            if (fgets(line, 1024, file) == NULL) {
                //cout << "end of file" << endl;
                lineoffset = lineoffset + tokenlength;
                eofFlag = true;
                return NULL;
            } // EOF reached
//            if((strcmp(line,"\n") == 0)||(strcmp(line,"\r\n") == 0)||(strcmp(line,"\0") == 0)){
//                //cout << "blank line" << endl;
//                lineoffset = 1;
//                linenum ++;
//                tokenlength = 0;
//                //lineoffset = 1;
//                continue;
//                return NULL;
//            } // if blank line go to next line;
            linenum++;
            tok = strtok(line, DELIM);
            if (tok == NULL) // no tokens in line
                continue; // we try with next line
           // newLine = false;
            tokenlength = strlen(tok);
            lineoffset = tok - line + 1;
           // cout<<"Token is in first check: "<<tok<<"\n";
            return tok;
        }
         tok = strtok(NULL, DELIM);

        //linelen = strlen(tok);
        if (tok != NULL){
           // lineoffset = tok - line + 1;
            tokenlength = strlen(tok);// found a token
           // cout<<"Token is: "<<tok;
            return tok;
        }

    }

}
bool isNum(char* s){
    char * t; // first copy the pointer to not change the original
    if(*t=='\0') {
        return false;
    }
    for (t = s; *t != '\0'; t++) {

        if(!isdigit(*t)) return false;
    }
    return true;
}

int readInt() {
    char *c = getToken();
    if(c==NULL && eofFlag) return -1;
    try {
    if (isNum(c)) {
        int n = atoi(c);
        //cout<<"Number is: "<<n<<"\n";
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
      //  printf("symbol got is: %s \n",c);
        return c;
    }
    else {
        __parseerror(1);
    }
}

bool validSym(char *s) {
    char * t = s;
    if(*t=='\0') {
        return false;
    }
    if(!isalpha(*t)) {
        return false;
    }
    int size =1;
    while(*t != '\0')  {
        if(!isalnum(*t)) return false;
        t++;
        size++;
    }
    if(size>16) {
        __parseerror(3);
        return false;
    }

    return true;
}

char readIAER(){
    char* c = getToken();
    if(validInstruction(c)){
       // printf("Instr got is: %s \n",c);
        return *c;
    }
    else {
        __parseerror(2);
    }
}

bool validInstruction(char *c) {

  //  printf("Instr got is: %s \n",c);
    char * t = c;
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
   // printf("Size is %d", size);
    return true;
}

void Pass1() {
    int baseAdd=0;
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
        //printf("debug result %d::  %d\n", moduleNu,baseAdd);
        if (eofFlag) break;
        for (int i = 0; i < defcount; i++) {
            char* sym = readSym();
            int val = readInt();
            if (symtable.count(sym)==1){
               // cout<<'\t'<<"symnol declared before";
                def_used.insert(sym); //symbol declared before
            }
            else{
                vector<int> add_mod;
                add_mod.push_back(moduleNu);
                add_mod.push_back(baseAdd+val);
                symtable.insert(pair<string, vector<int>>(sym, add_mod));
            }
        }
        int usecount = readInt();
        if(usecount>16) {
            __parseerror(5);
        }
        for (int i = 0; i < usecount; i++) {
            char* sym = readSym();
           // printf("Here: %s \n",sym);// we don’t do anything here   this would change in pass2
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
    }
    map<string, vector<int>>::iterator it;

    printf("Symbol Table\n");
    //bool err=false;
    for (it = symtable.begin(); it != symtable.end(); it++) {
        if(def_used.count(it->first)){
            errstring="Error: This variable is multiple times defined; first value used\n";
            def_used.erase(it->first);
        }
        else{
            errstring="";
        }
        printf("%s=%d %s\n",it->first.c_str(),it->second[1],errstring.c_str());
    }

}

void Pass2() {
    errstring="";
    int baseAdd;
    int moduleNu=0;
    int prevMAdd=0, prevML=0;
    printf("Memory Map\n");
    while (!eofFlag) {
        moduleNu++;
        baseAdd = prevMAdd+prevML;
        int defcount = readInt();
        if(defcount==-1 && eofFlag) break;
       // printf("debug result %d::  %d\n", moduleNu,baseAdd);
        if (eofFlag) break;
        for (int i = 0; i < defcount; i++) {
            char* sym = readSym();
            int val = readInt();
//            if (symtable.count(sym)==1){
////                cout<<'\t'<<"symbol declared before";
////            }
////            else{
////                symtable[sym] = val;
////            }
        }
        vector<string> uselist_symbols;
        int usecount = readInt();
        for (int i = 0; i < usecount; i++) {
            char* sym = readSym();
         //   printf("Here: %s \n",sym);// we don’t do anything here   this would change in pass2
            uselist_symbols.push_back(sym);
        }
        int instcount = readInt();

        for (int i = 0; i < instcount; i++) {
            char addressmode = readIAER();
            int oper = readInt();
            int opcode = oper/1000;
            int operand = oper%1000;
          //  char* errString;
          errstring="";
            int absolute_add;
            if(opcode>9){
                if(addressmode!='R')
                errstring="Error: Illegal opcode; treated as 9999";
                else {
                    errstring = "Error: Illegal immediate value; treated as 9999";
                }
                printf("%d %d %s\n",memory_map_len,9999,errstring.c_str());
                memory_map_len++;
                errstring="";
            }
            else if(addressmode=='R'){ //Relative Adderess+BaseAdd
                if(operand>instcount){
                    errstring="Error: Relative address exceeds module size; zero used";
                    operand=0;
                }
               absolute_add = opcode*1000+operand+baseAdd;
                if(absolute_add>=512){
                    errstring="Error: Absolute address exceeds machine size; zero used";
                    absolute_add=0;
                }
                printf("%d %d %s\n",memory_map_len,absolute_add,errstring.c_str());
                memory_map_len++;
                errstring="";
            }
            else if(addressmode=='E'){
                string sym;
                if(operand>uselist_symbols.size()){
                    errstring = "Error: External address exceeds length of uselist; treated as immediate";
                }
                  sym = uselist_symbols.at(operand);
                if (!symtable.count(sym)){
                    absolute_add = opcode*1000;
                    std::string someString(sym);
                    errstring = "Error: "+someString+" is not defined; zero used";
                }
                else {
                    operand = symtable[sym][1];
                    absolute_add = opcode * 1000 + operand;
                    def_used.insert(sym);
                    used_throughout.insert(sym);
                }
                    printf("%d %d %s\n",memory_map_len,absolute_add,errstring.c_str());
                    memory_map_len++;
                errstring="";
            }
            else if(addressmode=='A'){
                absolute_add = opcode*1000+operand;
                if(absolute_add>=512){
                    errstring="Error: Absolute address exceeds machine size; zero used";
                    absolute_add=0;
                }
                printf("%d %d %s\n",memory_map_len,absolute_add,errstring.c_str());
                memory_map_len++;
                errstring="";
            }
            else if(addressmode='I'){
                absolute_add = oper;
                printf("%d %d %s\n",memory_map_len,absolute_add,errstring.c_str());
                memory_map_len++;
                errstring="";
            }

        }
        prevMAdd = baseAdd;
        prevML = instcount;
        for(string i : uselist_symbols){
            if(!def_used.count(i)){
                printf("Warning: Module %d: %s appeared in the uselist but was not actually used\n",moduleNu,i.c_str());
            }
        }
        uselist_symbols.clear();
        def_used.clear();
    }
    map<string,vector<int>>::iterator it;
    for (it = symtable.begin(); it != symtable.end(); it++) {
        if(!used_throughout.count(it->first)){
            used_throughout.erase(it->first);
            printf("Warning: Module %d: %s was defined but never used\n",it->second[0],it->first.c_str());
        }
    }
    }

int main() {

    const char *inputfile = "/Users/sejaldivekar/CLionProjects/Linker1/filename.txt";
    file = fopen(inputfile, "r");
    Pass1();
    eofFlag = false;
    //fclose (file);
    fseek(file, 0, SEEK_SET);
    Pass2();
}
