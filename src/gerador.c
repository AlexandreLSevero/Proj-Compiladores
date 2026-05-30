#include "gerador.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE *mepa_file = NULL;
static int  label_counter = 0;

/* Buffer estático para novo_rotulo */
static char label_buf[16];

bool ger_init(const char *source_path) {
    /* Troca a extensão do arquivo fonte por .mepa */
    char out_path[512];
    strncpy(out_path, source_path, sizeof(out_path) - 6);
    out_path[sizeof(out_path) - 6] = '\0';

    /* Remove extensão existente, se houver */
    char *dot = strrchr(out_path, '.');
    if (dot) *dot = '\0';

    strcat(out_path, ".mepa");

    mepa_file = fopen(out_path, "w");
    label_counter = 0;
    return (mepa_file != NULL);
}

void ger_close(void) {
    if (mepa_file) {
        fclose(mepa_file);
        mepa_file = NULL;
    }
}

void ger_emite(const char *rotulo, const char *mnemonico,
               const char *param1,  const char *param2) {
    if (!mepa_file) return;

    /* Rótulo: se presente, "LABEL: "; senão espaços para indentação */
    if (rotulo && rotulo[0] != '\0') {
        fprintf(mepa_file, "%s:", rotulo);
        /* Preenche até a coluna 6 para alinhar o mnemônico */
        int pad = 6 - (int)strlen(rotulo) - 1;
        for (int i = 0; i < pad; i++) fputc(' ', mepa_file);
    } else {
        fprintf(mepa_file, "       ");
    }

    /* Mnemônico */
    fprintf(mepa_file, " %s", mnemonico);

    /* Parâmetros — vírgula SEM espaço entre eles (requisito MEPA) */
    if (param1 && param1[0] != '\0') {
        fprintf(mepa_file, " %s", param1);
        if (param2 && param2[0] != '\0') {
            fprintf(mepa_file, ",%s", param2);
        }
    }

    fprintf(mepa_file, "\n");
}

char *ger_novo_rotulo(void) {
    label_counter++;
    snprintf(label_buf, sizeof(label_buf), "L%d", label_counter);
    return label_buf;
}

FILE *ger_get_file(void) {
    return mepa_file;
}

int ger_emite_string(const char *str) {
    if (!mepa_file || !str) return 0;

    /* O lexema chega com as aspas: "hello" → começa em str[1], termina antes do último '"' */
    int len = (int)strlen(str);
    int start = (str[0] == '"') ? 1 : 0;
    int end   = (str[len-1] == '"') ? len - 1 : len;

    char buf[8];
    int count = 0;
    for (int i = start; i < end; i++) {
        unsigned char c = (unsigned char)str[i];
        /* Trata escape simples */
        if (c == '\\' && i + 1 < end) {
            i++;
            switch (str[i]) {
                case 'n':  c = '\n'; break;
                case 't':  c = '\t'; break;
                case '\\': c = '\\'; break;
                case '"':  c = '"';  break;
                default:   c = str[i]; break;
            }
        }
        snprintf(buf, sizeof(buf), "%d", (int)c);
        ger_emite(NULL, "CRCT", buf, NULL);
        ger_emite(NULL, "IMPR", NULL, NULL);
        count++;
    }
    return count;
}
