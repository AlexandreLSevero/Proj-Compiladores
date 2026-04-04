static Token lookahead;

void parse_program() {
    lookahead = lex_next();
    parse_globals();
    parse_subroutines();
    match(sSTART);
    parse_block();
    match(sEND);
}

void match(Category expected) {
    if (lookahead.cat == expected) {
        lookahead = lex_next();
    } else {
        diag_error(expected, lookahead); // Reporta erro
    }
}
