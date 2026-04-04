typedef enum {
    sIDENTIF, sCTEINT, sIF, sELSE, sSTART, sEND, 
    sATRIB, sSOMA, sPONTO_VIRGULA, sEOF
} Category;

typedef struct {
    Category cat;
    char lexema[128];
    int linha;
} Token;
