#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INST_LR 	0 /* LOAD REGISTER */
#define INST_CR 	1 /* COMPARE REGISTER */
#define INST_AR 	2 /* ADD REGISTER */
#define INST_SR 	3 /* SUBTRACT REGISTER */
#define INST_LA 	4 /* LOAD ADDRESS */
#define INST_BCT 	5 /* BRANCH ON COUNT */
#define INST_BC 	6 /* BRANCH ON CONDITION */
#define INST_ST 	7 /* STORE */
#define INST_L 		8 /* LOAD */
#define INST_C 		9 /* COMPARE */

int halt     		    = 0, /* halt flag */
    pc       		    = 0, /* program counter register */
    inst_addr		    = 0, /* instruction address */
    R1			        = 0, /* stores R1 for RR/RX operations */
    R2			        = 0, /* stores R2 for RR operations */
    X2			        = 0, /* stores X2 for RX operations */
    B2			        = 0, /* stores B2 for RX operations */
    opcode		      = 0, /* stores current opcode */
    condition_code	= 0, /* stores current condition code */
    displacement  	= 0, /* stores displacement value for RX operations */
    word_count		  = 0, /* indicates how many memory words to display at end */
    bct_taken_cnt 	= 0, /* counter for how many times the BCT funciton branches */
    bc_taken_cnt  	= 0, /* counter for how many times the BC function branches */
    mem_read_cnt  	= 0, /* counter for how many times functions read from main memory */
    mem_write_cnt 	= 0, /* counter for how many times functions write to main memory */
    inst_fetch_cnt	= 0, /* counter for how many total instructions were fetched */
    eff_addr	     	= 0, /* stores the effective address calculation */
    verbose		      = 0; /* flag to control printing of cycle-by-cycle results */

int reg[16] = {0}; /* array for registers */

unsigned char ram[4096]; /* main memory for values that are read in */

int inst_cnt[10] = {0}; /* stores counters for the number of times each function is called */

#define LINES_PER_BANK 16

int last[2][LINES_PER_BANK],
    valid[2][LINES_PER_BANK],
    tag[2][LINES_PER_BANK],
    access,
    addr_tag,
    hits,
    misses,
    bank,
    addr_index;

int second = 0;

void cache_stats(void)
{
	printf("  cache hits          = %d\n", hits);
	printf("  cache misses        = %d\n", misses);
}

void cache_init(void)
{
	hits = misses = access = addr_tag = addr_index = 0;

	int i;
	for (i = 0; i < LINES_PER_BANK; i++)
	{
	  last[0][i] = valid[0][i] = tag[0][i] = 0;
	  last[1][i] = valid[1][i] = tag[1][i] = 0;
	}
}

void cache_access(int address)
{
  access++;

  addr_index = (address >> 3) & 0xF;
  addr_tag = address >> 7;

  // Check for hit
  bank = -1;

  if (valid[0][addr_index] && (addr_tag == tag[0][addr_index]))
  {
    bank = 0;
  }

  if (valid[1][addr_index] && (addr_tag == tag[1][addr_index]))
  {
    bank = 1;
  }

  if (bank != -1)
  {
    hits++;
    last[bank][addr_index] = access;
  } else {
    misses++;

    bank = -1;

    if (!valid[0][addr_index]) bank = 0;
    if (!valid[1][addr_index]) bank = 1;

    if (bank != -1)
    {
      valid[bank][addr_index] = 1;
      tag[bank][addr_index] = addr_tag;
      last[bank][addr_index] = access;
    } else {
      bank = 0;

      if (last[1][addr_index] < last[bank][addr_index])
      {
        bank = 1;
      }

      valid[bank][addr_index] = 1;
      tag[bank][addr_index] = addr_tag;
      last[bank][addr_index] = access;
    }
  }
}

void hlt()
{
	// Set halt flag
	halt = 1;
}

void print_registers()
{
	// Print current contents of the registers
	int i = 0;
	int addr = 0;

	if (inst_addr > 1000)
	{
		addr = pc;
	} else {
		addr = inst_addr;
	}

	printf("instruction address = %06x, condition code = %d\n", addr, condition_code);

	for (i = 0; i <= 3; i++)
	{
		printf("R%X = %08x, ", i, reg[i]);
		printf("R%X = %08x, ", i+4, reg[i+4]);
		printf("R%X = %08x, ", i+8, reg[i+8]);
		printf("R%X = %08x\n", i+12, reg[i+12]);
	}
}

void load_ram()
{
	// Load stdin into memory
	int i = 0;

	if (verbose)
	{
		printf("initial contents of memory arranged by bytes\n");
		printf("addr value\n");
	}

	while (scanf("%2hhx", &ram[i]) != EOF)
	{
		if (i >= 4096)
		{
			printf("program file overflows available memory\n");
			exit(-1);
		}

		ram[i] = ram[i] & 0xfff;
		if (verbose)
		{
			printf("%03x: %02x\n", i, ram[i]);
		}

		i++;
	}
	word_count = i;

	for (; i < 256; i++)
	{
		ram[i] = 0;
	}

	if (verbose)
	{
		printf("\n");
	}
}

void decode()
{
	// Increment the fetch counter
	inst_fetch_cnt++;

	// Get opcode and increment program counter
	opcode = ram[pc];
	pc++;

	// Determine if opcode dictates an RR or RX instruction
	if ((opcode & 0xF0) == 0x00) return;
	if ((opcode & 0xF0) != 0x10)
	{
		// RX instruction
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

		if (displacement >= 4095)
		{
			inst_addr += displacement;
		} else {
			inst_addr += 4;
		}
	} else {
		// RR instruction
		R1 = ((ram[pc]) >> 4) & 0x0F;
		R2 = (ram[pc]) & 0x0F;

		eff_addr = R2;

		pc++;

		inst_addr += 2;
	}
}

void LR(int R1, int R2)
{
	// Load the contents of R2 into R1
	reg[R1] = reg[R2];

	if (verbose)
	{
		printf("\nLR instruction, ");
		printf("operand 1 is R%x, ", R1);
		printf("operand 2 is R%x\n", R2);
	}

	inst_cnt[INST_LR]++;
}

void CR(int R1, int R2)
{
	// Compare the contents of R2 and R1 and set condition code
	if (reg[R1] == reg[R2])
	{
		condition_code = 0;
	} else if (reg[R1] < reg[R2])
	{
		condition_code = 1;
	} else {
		condition_code = 2;
	}

	if (verbose)
	{
		printf("\nCR instruction, ");
		printf("operand 1 is R%x, ", R1);
		printf("operand 2 is R%x\n", R2);
	}

	inst_cnt[INST_CR]++;
}

void AR(int R1, int R2)
{
	reg[R1] = reg[R1] + reg[R2];
	if (reg[R1] == 0)
	{
		condition_code = 0;
	} else if (reg[R1] < 0)
	{
		condition_code = 1;
	} else {
		condition_code = 2;
	}

	if (verbose)
	{
		printf("\nAR instruction, ");
		printf("operand 1 is R%x, ", R1);
		printf("operand 2 is R%x\n", R2);
	}

	inst_cnt[INST_AR]++;
}

void SR(int R1, int R2)
{
	reg[R1] = reg[R1] - reg[R2];

	if (reg[R1] == 0)
	{
		condition_code = 0;
	} else if (reg[R1] < 0)
	{
		condition_code = 1;
	} else {
		condition_code = 2;
	}

	if (verbose)
	{
		printf("\nSR instruction, ");
		printf("operand 1 is R%x, ", R1);
		printf("operand 2 is R%x\n", R2);
	}

	inst_cnt[INST_SR]++;
}

void LA(int R1, int eff_addr)
{
//  printf("LA eff_addr: %06x\n", eff_addr);
  //if (eff_addr == 0x000320) second = 1;
	reg[R1] = displacement;

	if (verbose)
	{
		printf("\nLA instruction, operand 1 is R%x, ", R1);
		printf("operand 2 at address %06x\n", displacement);
	}

	inst_cnt[INST_LA]++;
}

void BCT(int R1, int eff_addr)
{
	// //Check for alignment error
	// if ((eff_addr & 0xFFFFFF) != 0)
	// {
	// 	printf("instruction fetch alignment error at %d to %d\n", R1, eff_addr);
	// 	exit(0);
	// }

  // eff_addr = eff_addr & 0xFFFFFF;

	if (verbose)
	{
		printf("\nBCT instruction, operand 1 is R%x, ", R1);
		printf("branch target is address %06x\n", eff_addr);
	}

	reg[R1]--;

	if (reg[R1] != 0)
	{
		inst_addr = eff_addr;
		bct_taken_cnt++;

		// Branch to correct pc based on instruction
		pc = eff_addr;
	}

	inst_cnt[INST_BCT]++;
}

void BC(int R1, int eff_addr)
{
	if (verbose)
	{
		if (inst_addr >= 1000)
		{
			printf("\nBC instruction, mask is %x, branch target is address %06x\n", R1, inst_addr - 3);
		} else {
			printf("\nBC instruction, mask is %x, branch target is address %06x\n", R1, eff_addr);
		}
	}

	// Check for out of range instruction address
	if (inst_addr > 4095)
	{
		int i = 0;
		printf("instruction address = %06x, condition code = %d\n", inst_addr - 3, condition_code);

		for (i = 0; i <= 3; i++)
		{
			printf("R%X = %08x, ", i, reg[i]);
			printf("R%X = %08x, ", i+4, reg[i+4]);
			printf("R%X = %08x, ", i+8, reg[i+8]);
			printf("R%X = %08x\n", i+12, reg[i+12]);
		}

		printf("\nout of range instruction address %x\n", inst_addr - 3);
		exit(0);
	}

	// Check if condition code matches
	if ((R1 & condition_code) != 0)
	{
		inst_addr = eff_addr;
		bc_taken_cnt++;

		// Branch to correct pc based on instruction
		pc = eff_addr;
	}

	inst_cnt[INST_BC]++;
}

void ST(int R1, int X2, int B2, int displacement)
{
	if (B2 != 0)
	{


    if (X2 == 0)
    {
      eff_addr = reg[B2] + displacement;
    } else {
      eff_addr = B2 + X2 + displacement;
    }

    //eff_addr = reg[B2] + displacement;
    //eff_addr = (X2 + displacement + 0x6);
    //eff_addr = reg[B2] + displacement;

		cache_access(eff_addr);

		// Store contents of R1 at the effective address
		if (verbose)
		{
			printf("\nST instruction, operand 1 is R%x, ", R1);
			printf("operand 2 at address %06x\n", eff_addr);
		}

		int i = 0;
		for (i = 3; i >= 0; i--)
		{
			ram[reg[B2] + i] = (reg[R1] >> ((3 - i) * 8));
		}
	} else if (B2 == 0 && X2 == 0)
	{
		// Store R1 into memory location (displacement)
		if (verbose)
		{
			printf("\nST instruction, operand 1 is R%x, ", R1);
			printf("operand 2 at address %06x\n", eff_addr);
		}

		cache_access(eff_addr);

		int i = 0;
		for (i = 3; i >= 0; i--)
		{
			ram[displacement + i] = (reg[R1] >> ((3 - i) * 8));
		}
	} else if (B2 == 0 && X2 != 0) {
    //printf("third store if\n");
    // Store the contents of R1 into the memory word at eff_addr
    // printf("last if in store\n");
		// int mem_loc = ((4 * abs(X2 - R1) * reg[R1]) + displacement);
    //
		// if (verbose)
		// {
		// 	printf("\nST instruction, operand 1 is R%x, ", R1);
		// 	//printf("operand 2 at address %06x\n", mem_loc);
    //   printf("operand 2 at address %06x\n", eff_addr);
		// }
    //
		// ram[mem_loc + 3] = reg[R1];

		//cache_access(mem_loc);
    //cache_access(eff_addr);
    if (verbose)
    {
      printf("\nST instruction, operand 1 is R%x, ", R1);
      printf("operand 2 at address %06x\n", reg[X2]);
    }

    ram[reg[X2]] = reg[X2];
    cache_access(reg[X2]);
	}

	inst_cnt[INST_ST]++;
	mem_write_cnt++;
}

void L(int R1, int eff_addr, int displacement)
{
	cache_access(eff_addr);
	// // Check for data access alignment error
	// if ((eff_addr & 3) != 0)
	// {
	// 	printf("data access alignment error at %d to %d\n", R1, eff_addr);
	// 	exit(0);
	// }

	// Load the contents of the memory word at eff_addr into R1
	if (verbose)
	{
		printf("\nL instruction, operand 1 is R%x, ", R1);
		printf("operand 2 at address %06x\n", eff_addr);
	}

	// Create a string to represent the memory word
	char *mem_word = malloc(16);
	sprintf(mem_word, "%02x%02x%02x%02x", ram[eff_addr], ram[eff_addr+1], ram[eff_addr+2], ram[eff_addr+3]);

	// Set the register
	reg[R1] = (int) strtol(mem_word, NULL, 16);

	free(mem_word);

	inst_cnt[INST_L]++;
	mem_read_cnt++;
}

void C(int R1, int eff_addr)
{
	// Compare the contents of R1 with the contents of the memory word at
	// eff_addr and set the condition code

	// Create a string to represent the memory word
	char *mem_word = malloc(16);
	sprintf(mem_word, "%02x%02x%02x%02x", ram[eff_addr], ram[eff_addr+1], ram[eff_addr+2], ram[eff_addr+3]);

	int mem_int = (int) strtol(mem_word, NULL, 16);

	if (reg[R1] == mem_int)
	{
		condition_code = 0;
	} else if (reg[R1] < mem_int)
	{
		condition_code = 1;
	} else {
		condition_code = 2;
	}

	if (verbose)
	{
		printf("\nC instruction, ");
		printf("operand 1 is R%x, ", R1);
		printf("operand 2 at address %06x\n", eff_addr);
	}

	cache_access(eff_addr);

	inst_cnt[INST_C]++;
	mem_read_cnt++;
	free(mem_word);
}

int main(int argc, char *argv[])
{
	int i = 0;

	// Check for arguments and correct usage
	if (argc > 1)
	{
		if ((argv[1][0] == '-') && ((argv[1][1] == 'v') || (argv[1][1] == 'V')))
		{
			verbose = 1;
		} else {
			printf("usage: either %s or %s -v with input taken from stdin\n", argv[0], argv[0]);
			exit(-1);
		}
	}

	// Initialize the cache
	cache_init();

	printf("\nbehavioral simulation of S/360 subset\n");
	if (verbose)
	{
		printf("\n(memory is limited to 4096 bytes in this simulation)\n");
		printf("(addresses, register values, and memory values are shown in hexadecimal)\n\n");
	} else {
		printf("\n");
	}

	// Read input from stdin into memory
	load_ram();

	if (verbose)
	{
		printf("initial pc, condition code, and register values are all zero\n\n");

		printf("updated pc, condition code, and register values are shown after\n");
		printf(" each instruction has been executed\n");

	}

	while (!halt)
	{
		// Call decode() to get opcodes and set other variables
		decode();

		// Check the opcode and call the appropriate function
		switch (opcode)
		{
			case 0x41:
				LA(R1, eff_addr);
				break;

			case 0x1B:
				SR(R1, R2);
				break;

			case 0x1A:
				AR(R1, R2);
				break;

			case 0x50:
				ST(R1, X2, B2, displacement);
				break;

			case 0x46:
				BCT(R1, eff_addr);
				break;

			case 0x18:
				LR(R1, R2);
				break;

			case 0x19:
				CR(R1, R2);
				break;

			case 0x47:
				BC(R1, eff_addr);
				break;

			case 0x58:
				L(R1, eff_addr, displacement);
				break;

			case 0x59:
				C(R1, eff_addr);
				break;

			case 0x00:
				inst_addr += 2;
				hlt();
				break;
		}

		if (verbose && !halt) print_registers();
	}

	if (verbose)
	{
		printf("\nHalt encountered\n");
		print_registers();
	}

	if (verbose)
	{
		printf("\nfinal contents of memory arranged by words\n");
		printf("addr value\n");

		for (i = 0; i < word_count; i = i + 4)
		{
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

	if (inst_cnt[INST_BCT] > 0)
	{
		printf(", taken = %d (%.1f%%)\n", bct_taken_cnt, 100.0*((float)bct_taken_cnt)/((float)inst_cnt[INST_BCT]));
	} else {
		printf("\n");
	}

	printf("    BC  instructions  = %d", inst_cnt[INST_BC]);

	if (inst_cnt[INST_BC] > 0)
	{
		printf(", taken = %d (%.1f%%)\n", bc_taken_cnt, 100.0*((float)bc_taken_cnt)/((float)inst_cnt[INST_BC]));
	} else {
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