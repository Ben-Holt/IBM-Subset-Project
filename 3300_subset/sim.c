/**
 * @file sim.c
 * @author Ben Holt
 * @brief CPSC 3300- IBM S/360 Project
 * @version 0.1
 * @date 2022-05-02
 * @copyright Copyright (c) 2022
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/**
 * define instruction variables for RR and RX and lines per bank
 */

#define INST_LR 	0
#define INST_CR 	1
#define INST_AR 	2
#define INST_SR 	3
#define INST_LA 	4
#define INST_BCT 	5
#define INST_BC 	6
#define INST_ST 	7
#define INST_L 		8
#define INST_C 		9
#define LINES_PER_BANK 16

/**
 * universal variable declarations
 */

int verbose = 0; /* verbose flag */
int pc = 0; /* program counter register */
int halt = 0; /* halt variable */
int word_count = 0;  /* indicates how many memory words to display at end */
int bct_taken_cnt = 0; /* BCT taken count variable for percentage */
int bc_taken_cnt = 0; /* BC taken count variable for percentage */
unsigned char ram[4096]; /* main memory */
int inst_addr = 0; /* instruction address */
int eff_addr = 0; /* effective address */
int X2 = 0; /* X2 */
int B2 = 0; /* B2 */
int add = 0; /* address */
int opcode = 0; /* opcode */
int condition_code = 0; /* condition code */
int R1 = 0; /* R1 */
int R2 = 0; /* R2 */
int displacement = 0; /* displacement */
int reg[16] = {0}; /* array for registers */
int inst_fetch_cnt = 0;
int inst_cnt[10] = {0};
int jneg_taken_cnt = 0;
int mem_read_cnt = 0;  /* for this simple machine, equals lda inst cnt */
int mem_write_cnt = 0; /* for this simple machine, equals sta inst cnt */
int access; /* counter variable for accesses */
int addr_tag; /* address tag */
int addr_index; /* address index */
int bank; /* bank number */
int hits;
int misses;
int last[2][LINES_PER_BANK]; /* last used position */
int tag[2][LINES_PER_BANK]; /* tag bits */
int valid[2][LINES_PER_BANK]; /* valid bits */


//Initializes cache and sets values equal to 0
void cache_init(void){

	for (int i = 0; i < LINES_PER_BANK; i++){
	  last[0][i] = tag[0][i] = valid[0][i] = 0;
	  last[1][i] = tag[1][i] = valid[1][i] = 0;
	}
	access = addr_tag = addr_index = hits = misses = 0;
}

//Print helper function for cache
void cache_stats(void){
    printf("  cache hits          = %d\n", hits);
	printf("  cache misses        = %d\n", misses);
}

//cache access function similar to one provided in cache.c
void cache_access( unsigned int address){

	access++;

	//bitwise operators to extract index and tag
	addr_index = (address >> 3) & 0xF;
	addr_tag = address >> 7;

	bank = -1;

	//checks for hits based on valid bit and tag
	if (valid[0][addr_index] && (addr_tag == tag[0][addr_index])){
		bank = 0;
	}

	if (valid[1][addr_index] && (addr_tag == tag[1][addr_index])){
		bank = 1;
	}

	//indicates hit or miss
	if (bank != -1){
		hits++;
		last[bank][addr_index] = access;
	} else {
		misses++;

		bank = -1;

		//checks for not valid bit
		if (!valid[0][addr_index]) bank = 0;
		if (!valid[1][addr_index]) bank = 1;

		//replacement policy
		if (bank != -1){
		valid[bank][addr_index] = 1;
		tag[bank][addr_index] = addr_tag;
		last[bank][addr_index] = access;
		} else {
		bank = 0;

		if (last[1][addr_index] < last[bank][addr_index]){
			bank = 1;
		}

		valid[bank][addr_index] = 1;
		tag[bank][addr_index] = addr_tag;
		last[bank][addr_index] = access;
		}
	}
}

//Halt setter
void hlt(){
	halt = 1; /* only set once */
}

//Print current contents of the registers
void print_registers(){
	int i = 0;
	int addr = 0;

	//sets address to pc or inst_addr
	if (inst_addr > 1000)
	{
		addr = pc;
	} else {
		addr = inst_addr;
	}

	//print statements for register
	printf("instruction address = %06x, condition code = %d\n", addr, condition_code);
	for (i = 0; i <= 3; i++){
		printf("R%X = %08x, ", i, reg[i]);
		printf("R%X = %08x, ", i+4, reg[i+4]);
		printf("R%X = %08x, ", i+8, reg[i+8]);
		printf("R%X = %08x\n", i+12, reg[i+12]);
	}
}

//Load ram
void load_ram(){
	int i = 0;

	//prints statements for verbose
	if (verbose){
		printf("initial contents of memory arranged by bytes\n");
		printf("addr value\n");
	}

	//scans in stdin
	while (scanf("%2hhx", &ram[i]) != EOF){
		if (i >= 4096){
			printf("program file overflows available memory\n");
			exit(-1);
		}

		ram[i] = ram[i] & 0xfff;
		if (verbose){
			printf("%03x: %02x\n", i, ram[i]);
		}

		i++;
	}
	word_count = i;

	//intializes ram
	for (; i < 256; i++){
		ram[i] = 0;
	}

	if (verbose){
		printf("\n");
	}
}

//Decodes opcode and fetches some other relevant information
void decode(){
	inst_fetch_cnt++;
	opcode = ram[pc];
	pc++;


	//Extracts opcode and determines if it's an RR or RX instruction
	if ((opcode & 0xF0) == 0x00) return;
	if ((opcode & 0xF0) != 0x10){
		int dispadd = 0;

		R1 = ((ram[pc] >> 4) & 0x0F);
		X2 = (ram[pc]) & 0x0F;

		pc++;

		B2 = ((ram[pc] >> 4) & 0x0F);
		dispadd = (ram[pc]) & 0x0F;

		pc++;

		displacement = (dispadd << 8) | ram[pc];

		pc++;
		eff_addr = X2 + B2 + displacement;

		if (displacement >= 4095){
			inst_addr += displacement;
		} else{
			inst_addr += 4;
		}

	} else{
		R1 = ((ram[pc]) >> 4) & 0x0F;
		R2 = (ram[pc]) & 0x0F;
		eff_addr = R2;
		pc++;
		inst_addr += 2;
	}
}

//Load the contents of R2 into R1
void LR(int R1, int R2){
	reg[R1] = reg[R2];

	if (verbose){
		printf("\nLR instruction, ");
		printf("operand 1 is R%x, ", R1);
		printf("operand 2 is R%x\n", R2);
	}

	inst_cnt[INST_LR]++;
}

//Compare the contents of R2 and R1 and set condition code
void CR(int R1, int R2){
	if (reg[R1] == reg[R2]){
		condition_code = 0;
	} else if (reg[R1] < reg[R2]){
		condition_code = 1;
	} else{
		condition_code = 2;
	}

	if (verbose){
		printf("\nCR instruction, ");
		printf("operand 1 is R%x, ", R1);
		printf("operand 2 is R%x\n", R2);
	}

	inst_cnt[INST_CR]++;
}

//Add R1 and R2 and place into R1 and set condition code
void AR(int R1, int R2){
	reg[R1] = reg[R1] + reg[R2];
	if (reg[R1] == 0){
		condition_code = 0;
	} else if (reg[R1] < 0){
		condition_code = 1;
	} else{
		condition_code = 2;
	}

	if (verbose){
		printf("\nAR instruction, ");
		printf("operand 1 is R%x, ", R1);
		printf("operand 2 is R%x\n", R2);
	}

	inst_cnt[INST_AR]++;
}

//Subtract R1 and R2 and place into R1 and set condition code
void SR(int R1, int R2){
	reg[R1] = reg[R1] - reg[R2];

	if (reg[R1] == 0){
		condition_code = 0;
	} else if (reg[R1] < 0){
		condition_code = 1;
	} else{
		condition_code = 2;
	}

	if (verbose){
		printf("\nSR instruction, ");
		printf("operand 1 is R%x, ", R1);
		printf("operand 2 is R%x\n", R2);
	}

	inst_cnt[INST_SR]++;
}

//Load effective address and clamp to 24 bits
void LA(int R1, int eff_addr){
	reg[R1] = displacement;
	if (verbose){
		printf("\nLA instruction, operand 1 is R%x, ", R1);
		printf("operand 2 at address %06x\n", displacement);
	}

	inst_cnt[INST_LA]++;
}

//Branch on Count
void BCT(int R1, int eff_addr){
	if (verbose){
		printf("\nBCT instruction, operand 1 is R%x, ", R1);
		printf("branch target is address %06x\n", eff_addr);
	}

	reg[R1]--;

	if (reg[R1] != 0){
		inst_addr = eff_addr;
		bct_taken_cnt++;
		pc = eff_addr;
	}

	inst_cnt[INST_BCT]++;
}

//Branch on Condition
void BC(int R1, int eff_addr){
	if (verbose){
		if (inst_addr >= 1000)
		{
			printf("\nBC instruction, mask is %x, branch target is address %06x\n", R1, inst_addr - 3);
		} else {
			printf("\nBC instruction, mask is %x, branch target is address %06x\n", R1, eff_addr);
		}
	}

	if (inst_addr > 4095)
	{
		int i = 0;
		printf("instruction address = %06x, condition code = %d\n", inst_addr - 3, condition_code);

		for (i = 0; i <= 3; i++)
		{
			printf("R%X = %08x, ", i, reg[i]);
			printf("R%X = %08x, ", i+4, reg[i + 4]);
			printf("R%X = %08x, ", i+8, reg[i + 8]);
			printf("R%X = %08x\n", i+12, reg[i + 12]);
		}

		printf("\nout of range instruction address %x\n", inst_addr - 3);
		exit(0);
	}

	if ((R1 & condition_code) != 0){
		inst_addr = eff_addr;
		bc_taken_cnt++;
		pc = eff_addr; 
	}


	inst_cnt[INST_BC]++;
}

//Store contents of R1 into memory word at eff_addr
void ST(int R1, int X2, int B2, int displacement){	
	if (B2 != 0){
		if (X2 == 0){
			eff_addr = reg[B2] + displacement;
		} else{
			eff_addr = B2 + X2 + displacement;
		}

		cache_access(eff_addr);

		// Store contents of R1 at the effective address
		if (verbose){
			printf("\nST instruction, operand 1 is R%x, ", R1);
			printf("operand 2 at address %06x\n", eff_addr);
		}

		int i = 0;
		for (i = 3; i >= 0; i--){
			ram[reg[B2] + i] = (reg[R1] >> ((3 - i) * 8));
		}
	
	} else if (B2 == 0 && X2 == 0){

		if (verbose){
			printf("\nST instruction, operand 1 is R%x, ", R1);
			printf("operand 2 at address %06x\n", eff_addr);
		}
		cache_access(eff_addr);
		int i = 0;
		for (i = 3; i >= 0; i--){
			ram[displacement + i] = (reg[R1] >> ((3 - i) * 8));
		}
	} else if (B2 == 0 && X2 != 0){
		if (verbose){
			printf("\nST instruction, operand 1 is R%x, ", R1);
			printf("operand 2 at address %06x\n", reg[X2]);
		}
		ram[reg[X2]] = reg[X2];
		cache_access(reg[X2]);
	}

	inst_cnt[INST_ST]++;
	mem_write_cnt++;
}

//Load the contents of the memory word at the eff_addr into R1
void L(int R1, int eff_addr, int displacement){
	cache_access(eff_addr);

	printf("\nL instruction, operand 1 is R%x, ", R1);
	printf("operand 2 at address %06x\n", eff_addr);

	char *mem_word = malloc(16);
	sprintf(mem_word, "%02x%02x%02x%02x", ram[eff_addr], ram[eff_addr+1], ram[eff_addr+2], ram[eff_addr+3]);

	reg[R1] = (int) strtol(mem_word, NULL, 16);

	free(mem_word);

	inst_cnt[INST_L]++;
	mem_read_cnt++;
}

//Compare the contents of R1 with the contents of the memory word at eff_addr and set the condition code
void C(int R1, int eff_addr){
	char *mem_word = malloc(16);
	sprintf(mem_word, "%02x%02x%02x%02x", ram[eff_addr], ram[eff_addr+1], ram[eff_addr+2], ram[eff_addr+3]);

	int mem_int = (int) strtol(mem_word, NULL, 16);

	if (reg[R1] == mem_int){
		condition_code = 0;
	} else if (reg[R1] < mem_int){
		condition_code = 1;
	} else{
		condition_code = 2;
	}

	cache_access(eff_addr);

	if (verbose){
		printf("\nC instruction, ");
		printf("operand 1 is R%x, ", R1);
		printf("operand 2 at address %06x\n", eff_addr);
	}

	inst_cnt[INST_C]++;
	mem_read_cnt++;
	free(mem_word);
}

//main
int main(int argc, char *argv[]){
	int i = 0;

	if (argc > 1){ 
		if ((argv[1][0] == '-') && ((argv[1][1] == 'v') || (argv[1][1] == 'V' ))){
			verbose = 1;
		} else{
			printf("usage: either %s or %s -v with input taken from stdin\n", argv[0], argv[0]);
			exit(-1);
		}
	}

	cache_init();

	printf("\nbehavioral simulation of S/360 subset\n");
	if (verbose){
		printf("\n(memory is limited to 4096 bytes in this simulation)\n");
		printf("(addresses, register values, and memory values are shown in hexadecimal)\n\n");
	} else{
		printf("\n");
	}

	load_ram();

	if (verbose){
		printf("initial pc, condition code, and register values are all zero\n\n");

		printf("updated pc, condition code, and register values are shown after\n");
		printf(" each instruction has been executed\n");

	}

	while (!halt){
		decode();

		if (opcode == 0x41){
			LA(R1, eff_addr);
		} else if (opcode == 0x1B){
			SR(R1, R2);
		} else if (opcode == 0x1A){
			AR(R1, R2);
		} else if (opcode == 0x50){
			ST(R1, X2, B2, displacement);
		} else if (opcode == 0x46){
			BCT(R1, eff_addr);
		} else if (opcode == 0x18){
			LR(R1, R2);
		} else if (opcode == 0x19){
			CR(R1, R2);
		} else if (opcode == 0x47){
			BC(R1, eff_addr);
		} else if (opcode == 0x58){
			L(R1, eff_addr, displacement);
		} else if (opcode == 0x59){
			C(R1, eff_addr);
		} else if (opcode == 0x00){
			inst_addr += 2;
			hlt();
		}

		if (verbose && !halt) print_registers();
	}

	if (verbose){
		printf("\nHalt encountered\n"); 
		print_registers();
	}

	if (verbose){
		printf("\nfinal contents of memory arranged by words\n");
		printf("addr value\n");

		for (i = 0; i < word_count; i = i + 4){
			printf("%03x: %02x%02x%02x%02x\n", i, ram[i], ram[i+1], ram[i+2], ram[i+3]);
		}

		printf("\n");
	}

	printf("execution statistics\n");
	printf("  instruction fetches = %d\n", inst_fetch_cnt - 1);
	printf("    LR  instructions  = %d\n", inst_cnt[INST_LR]);
	printf("    CR  instructions  = %d\n", inst_cnt[INST_CR]);
	printf("    AR  instructions  = %d\n", inst_cnt[INST_AR]);
	printf("    SR  instructions  = %d\n", inst_cnt[INST_SR]);
	printf("    LA  instructions  = %d\n", inst_cnt[INST_LA]);
	printf("    BCT instructions  = %d", inst_cnt[INST_BCT]);

	if (inst_cnt[INST_BCT] > 0){
		printf(", taken = %d (%.1f%%)\n", bct_taken_cnt, 100.0*((float)bct_taken_cnt)/((float)inst_cnt[INST_BCT]));
	} else{
		printf("\n");
	}

	printf("    BC  instructions  = %d",   inst_cnt[INST_BC]);

	if (inst_cnt[INST_BC] > 0){
		printf(", taken = %d (%.1f%%)\n", bc_taken_cnt, 100.0*((float)bc_taken_cnt)/((float)inst_cnt[INST_BC]));
	} else{
		printf("\n");
	}

	printf("    ST  instructions  = %d\n", inst_cnt[INST_ST]);
	printf("    L   instructions  = %d\n", inst_cnt[INST_L]);
	printf("    C   instructions  = %d\n", inst_cnt[INST_C]);
	printf("  memory data reads   = %d\n", mem_read_cnt);
	printf("  memory data writes  = %d\n", mem_write_cnt);
	cache_stats();

	return 0;
}