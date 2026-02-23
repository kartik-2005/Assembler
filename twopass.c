#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct { char mnemonic[10], code[5]; } OPTAB;
typedef struct { char symbol[10]; int addr; } SYMTAB;

OPTAB optab[50];
SYMTAB symtab[100];
int optab_cnt = 0, symtab_cnt = 0;
int start_addr, prog_len;

void load_optab() {
    FILE *fp = fopen("opcodes.txt", "r");
    while (fscanf(fp, "%s %s", optab[optab_cnt].mnemonic, optab[optab_cnt].code) != EOF) optab_cnt++;
    fclose(fp);
}

char* find_opcode(char *m) {
    for (int i = 0; i < optab_cnt; i++)
        if (strcmp(optab[i].mnemonic, m) == 0) return optab[i].code;
    return NULL;
}

int find_symbol(char *s) {
    for (int i = 0; i < symtab_cnt; i++)
        if (strcmp(symtab[i].symbol, s) == 0) return symtab[i].addr;
    return -1;
}

/* Pass 1 determines the addresses of all labels */
void pass1() {
    FILE *input = fopen("sample_input.txt", "r");
    FILE *inter = fopen("intermediate.txt", "w");
    char line[100], label[20], opcode[20], operand[20];
    int locctr = 0;

    while (fgets(line, sizeof(line), input)) {
        if (line[0] == '.' || line[0] == '\n' || (isspace(line[0]) && strlen(line) < 5)) continue;

        if (!isspace(line[0])) {
            int num = sscanf(line, "%s %s %s", label, opcode, operand);
            if (num == 2) strcpy(operand, "NONE");
        } else {
            strcpy(label, "-");
            int num = sscanf(line, "%s %s", opcode, operand);
            if (num == 1) strcpy(operand, "NONE");
        }

        if (strcmp(opcode, "START") == 0) {
            start_addr = (int)strtol(operand, NULL, 16);
            locctr = start_addr;
            fprintf(inter, "-\t%s\t%s\t%s\n", label, opcode, operand);
            continue;
        }

        fprintf(inter, "%04X\t%s\t%s\t%s\n", locctr, label, opcode, operand);

        if (strcmp(label, "-") != 0) {
            strcpy(symtab[symtab_cnt].symbol, label);
            symtab[symtab_cnt++].addr = locctr;
        }

        if (find_opcode(opcode) != NULL) locctr += 3;
        else if (strcmp(opcode, "WORD") == 0) locctr += 3;
        else if (strcmp(opcode, "RESW") == 0) locctr += 3 * atoi(operand);
        else if (strcmp(opcode, "RESB") == 0) locctr += atoi(operand);
        else if (strcmp(opcode, "BYTE") == 0) {
            if (operand[0] == 'X') locctr += (strlen(operand) - 3) / 2;
            else locctr += (strlen(operand) - 3);
        }

        if (strcmp(opcode, "END") == 0) break;
    }
    prog_len = locctr - start_addr;
    fclose(input); fclose(inter);
}

/* Pass 2 generates the object code using the symbol table */
void pass2() {
    FILE *inter = fopen("intermediate.txt", "r");
    FILE *output = fopen("output_twopass.txt", "w");
    char label[20], opcode[20], operand[20], addr_str[10], obj_code[20], text_record[70] = "";
    int current_t_start = -1, t_len = 0;

    fscanf(inter, "%s %s %s %s", addr_str, label, opcode, operand);
    fprintf(output, "H%-6s%06X%06X\n", label, start_addr, prog_len);

    while (fscanf(inter, "%s %s %s %s", addr_str, label, opcode, operand) != EOF) {
        if (strcmp(opcode, "END") == 0) break;
        int curr_addr = (int)strtol(addr_str, NULL, 16);
        obj_code[0] = '\0';

        char *op = find_opcode(opcode);
        if (op) {
            int target = 0;
            if (strcmp(operand, "NONE") != 0) {
                char *comma = strchr(operand, ',');
                if (comma) { *comma = '\0'; target = find_symbol(operand) + 0x8000; }
                else target = find_symbol(operand);
            }
            sprintf(obj_code, "%s%04X", op, target);
        } else if (strcmp(opcode, "BYTE") == 0) {
            if (operand[0] == 'X') sprintf(obj_code, "%.*s", (int)strlen(operand)-3, operand+2);
            else for(int i=2; i<strlen(operand)-1; i++) {
                char h[3]; sprintf(h, "%02X", operand[i]); strcat(obj_code, h);
            }
        } else if (strcmp(opcode, "WORD") == 0) sprintf(obj_code, "%06X", atoi(operand));

        if (strlen(obj_code) > 0) {
            if (current_t_start == -1) current_t_start = curr_addr;
            if (t_len + (strlen(obj_code)/2) > 30) {
                fprintf(output, "T%06X%02X%s\n", current_t_start, t_len, text_record);
                strcpy(text_record, obj_code); t_len = strlen(obj_code)/2; current_t_start = curr_addr;
            } else {
                strcat(text_record, obj_code); t_len += strlen(obj_code)/2;
            }
        } else {
            /* If we hit a RESW/RESB we must break the text record */
            if (t_len > 0) {
                fprintf(output, "T%06X%02X%s\n", current_t_start, t_len, text_record);
                strcpy(text_record, ""); t_len = 0; current_t_start = -1;
            }
        }
    }
    if (t_len > 0) fprintf(output, "T%06X%02X%s\n", current_t_start, t_len, text_record);
    fprintf(output, "E%06X\n", start_addr);
    fclose(inter); fclose(output);
}

int main() { load_optab(); pass1(); pass2(); return 0; }