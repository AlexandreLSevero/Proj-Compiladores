typedef enum { CAT_VAR, CAT_PROC, CAT_FUNC, CAT_PARAM } SymbolCat;
typedef enum { TYPE_INT, TYPE_VOID, TYPE_BOOL } DataType;

void ts_push_scope(const char *scope_name);
void ts_pop_scope();
void ts_insert(const char *id, SymbolCat cat, DataType type, int extra);
Symbol* ts_lookup(const char *id); // Busca do escopo atual para o global
