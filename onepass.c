#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* This linked list helps us keep track of labels we haven't seen yet */
typedef struct ForwardRef {
    int instr_addr;
    struct ForwardRef *next;
} ForwardRef;

typedef struct {
    char symbol[10];
    int addr;          /* This stays -1 until the label is defined */
    ForwardRef *list;  /* Head of the list for patching earlier instructions */
} SYMTAB;

typedef struct {
    char mnemonic[10], code[5];
} OPTAB;

OPTAB optab[50];
SYMTAB symtab[100];
int optab_cnt = 0, symtab_cnt = 0;

/* We use this buffer to simulate the program memory so we can fix it later */
char memory_buffer[10000][3]; 
int memory_filled[10000] = {0}; 
int instr_size[10000] = {0}; 

void load_optab() {
    FILE *fp = fopen("opcodes.txt", "r");
    if (!fp) exit(1);
    while (fscanf(fp, "%s %s", optab[optab_cnt].mnemonic, optab[optab_cnt].code) != EOF) optab_cnt++;
    fclose(fp);
}

int get_sym_index(char *s) {
    for (int i = 0; i < symtab_cnt; i++)
        if (strcmp(symtab[i].symbol, s) == 0) return i;
    return -1;
}

/* This function fixes instructions that were waiting for a label address */
void define_label(char *name, int address, int start_addr) {
    int idx = get_sym_index(name);
    if (idx == -1) {
        strcpy(symtab[symtab_cnt].symbol, name);
        symtab[symtab_cnt].addr = address;
        symtab[symtab_cnt].list = NULL;
        symtab_cnt++;
    } else {
        symtab[idx].addr = address;
        ForwardRef *curr = symtab[idx].list;
        while (curr != NULL) {
            int patch_loc = curr->instr_addr - start_addr + 1;
            
            /* We read the existing byte to check if the index bit was set */
            int existing_byte = (int)strtol(memory_buffer[patch_loc], NULL, 16);
            int high_byte = ((address >> 8) & 0xFF) | existing_byte;
            
            sprintf(memory_buffer[patch_loc], "%02X", high_byte);
            sprintf(memory_buffer[patch_loc + 1], "%02X", address & 0xFF);
            
            memory_filled[patch_loc] = 1;
            memory_filled[patch_loc + 1] = 1;
            ForwardRef *temp = curr;
            curr = curr->next;
            free(temp);
        }
        symtab[idx].list = NULL;
    }
}

void add_forward_ref(char *name, int loc) {
    int idx = get_sym_index(name);
    if (idx == -1) {
        strcpy(symtab[symtab_cnt].symbol, name);
        symtab[symtab_cnt].addr = -1;
        symtab[symtab_cnt].list = NULL;
        idx = symtab_cnt++;
    }
    ForwardRef *new_ref = malloc(sizeof(ForwardRef));
    new_ref->instr_addr = loc;
    new_ref->next = symtab[idx].list;
    symtab[idx].list = new_ref;
}

int main() {
    load_optab();
    FILE *input = fopen("sample_input.txt", "r");
    FILE *output = fopen("output_onepass.txt", "w");
    char line[100], label[20], opcode[20], operand[20], prog_name[20];
    int locctr, start_addr;

    /* Handle the program header first */
    while (fgets(line, sizeof(line), input)) {
        if (line[0] == '.') continue;
        if (sscanf(line, "%s %s %s", prog_name, opcode, operand) == 3) {
            if (strcmp(opcode, "START") == 0) {
                start_addr = (int)strtol(operand, NULL, 16);
                locctr = start_addr;
                break;
            }
        }
    }

    /* Main assembly loop begins here */
    while (fgets(line, sizeof(line), input)) {
        if (line[0] == '.' || line[0] == '\n' || (isspace(line[0]) && strlen(line) < 5)) continue;

        if (!isspace(line[0])) {
            int n = sscanf(line, "%s %s %s", label, opcode, operand);
            if (n == 2) strcpy(operand, "NONE");
            define_label(label, locctr, start_addr);
        } else {
            strcpy(label, "-");
            int n = sscanf(line, "%s %s", opcode, operand);
            if (n == 1) strcpy(operand, "NONE");
        }

        char *op_hex = NULL;
        for(int i=0; i<optab_cnt; i++) if(strcmp(optab[i].mnemonic, opcode) == 0) op_hex = optab[i].code;

        int m_idx = locctr - start_addr;
        if (op_hex) {
            int target = 0;
            if (strcmp(operand, "NONE") != 0) {
                int indexed = (strstr(operand, ",X") != NULL);
                if (indexed) *strstr(operand, ",X") = '\0';
                
                int s_idx = get_sym_index(operand);
                if (s_idx != -1 && symtab[s_idx].addr != -1) 
                    target = symtab[s_idx].addr + (indexed ? 0x8000 : 0);
                else {
                    add_forward_ref(operand, locctr);
                    target = (indexed ? 0x8000 : 0);
                }
            }
            strcpy(memory_buffer[m_idx], op_hex);
            sprintf(memory_buffer[m_idx+1], "%02X", (target >> 8) & 0xFF);
            sprintf(memory_buffer[m_idx+2], "%02X", target & 0xFF);
            memory_filled[m_idx] = memory_filled[m_idx+1] = memory_filled[m_idx+2] = 1;
            instr_size[m_idx] = 3; 
            locctr += 3;
        } 
        else if (strcmp(opcode, "WORD") == 0) {
            int val = atoi(operand);
            sprintf(memory_buffer[m_idx], "%02X", (val >> 16) & 0xFF);
            sprintf(memory_buffer[m_idx+1], "%02X", (val >> 8) & 0xFF);
            sprintf(memory_buffer[m_idx+2], "%02X", val & 0xFF);
            memory_filled[m_idx] = memory_filled[m_idx+1] = memory_filled[m_idx+2] = 1;
            instr_size[m_idx] = 3;
            locctr += 3;
        }
        else if (strcmp(opcode, "BYTE") == 0) {
            if (operand[0] == 'X') {
                strncpy(memory_buffer[m_idx], operand+2, 2);
                memory_filled[m_idx] = 1; instr_size[m_idx] = 1;
                locctr += 1;
            } else {
                int len = strlen(operand)-3;
                instr_size[m_idx] = len;
                for (int i = 0; i < len; i++) {
                    sprintf(memory_buffer[m_idx], "%02X", operand[i+2]);
                    memory_filled[m_idx++] = 1; locctr++;
                }
            }
        }
        else if (strcmp(opcode, "RESW") == 0) locctr += 3 * atoi(operand);
        else if (strcmp(opcode, "RESB") == 0) locctr += atoi(operand);

        if (strcmp(opcode, "END") == 0) break;
    }

    /* Finally we generate the object code file */
    int total_len = locctr - start_addr;
    fprintf(output, "H%-6s%06X%06X\n", prog_name, start_addr, total_len);

    int curr = 0;
    while (curr < total_len) {
        if (!memory_filled[curr]) { curr++; continue; }

        int rec_start = curr;
        char temp_rec[70] = "";
        int rec_len = 0;

        while (curr < total_len && memory_filled[curr]) {
            int size = instr_size[curr] > 0 ? instr_size[curr] : 1;
            if (rec_len + size > 30) break; 

            for (int i = 0; i < size; i++) {
                strcat(temp_rec, memory_buffer[curr++]);
                rec_len++;
            }
        }
        fprintf(output, "T%06X%02X%s\n", rec_start + start_addr, rec_len, temp_rec);
    }
    fprintf(output, "E%06X\n", start_addr);
    
    fclose(input); fclose(output);
    return 0;
}