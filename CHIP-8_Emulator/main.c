#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <time.h>

#include "openGL.h"

#define DISP_SIZE 64*32
#define INSPEED 700.0 // 700 Instructions per second
//#define MEM 4096

struct chip8 {
	uint16_t pc;
	uint16_t I;

	uint8_t V[16]; // V0-VF

	// insert stack
	uint16_t stack[16];
	uint8_t SP; // Stack pointer I'll use, won't try to deal with stack overflow

	uint8_t ST; //Sound timer
	uint8_t DT; //Delay timer

	uint8_t memory[4096]; // 4KB
	uint8_t disp[DISP_SIZE];
	uint8_t keyb[16]; // 0x0-0xF

	int drawFlag;

} chip;

void unsupportedOp(uint16_t opcode) {
	printf("Unsupported op code crashing program: '%04x'\n", opcode);
	exit(1);
}

void printChip() {
	printf("Current Op: %04x\n", (chip.memory[chip.pc] << 8) | (chip.memory[chip.pc + 1]) );
	printf("PC: 0x%03x SP: %d I: %03x DT: %d ST: %d\n", chip.pc, chip.SP, chip.I, chip.DT, chip.ST);
	printf("\nV0 V1 V2 V3 V4 V5 V6 V7 V8 V9 VA VB VC VD VE VF\n");
	for (int i = 0; i < 16; i++) {
		printf("%02x ", chip.V[i]);
	}
	printf("\n\nKeyb	");
	for (int i = 0; i < 16; i++) {
		printf("%x: %d ", i, chip.keyb[i]);
	}
	printf("\n-----------------------------------------\n");
}

void initChip();
void loadROM(char* rom);
void emulateChip8();
void drawScreen();
void processInput(GLFWwindow* window);
void printChip();

clock_t DT_S;

int main(int argc, char *argv[]) {

	unsigned char test[32 * 64];

	// Space for Setting up Graphics and Input (and Sound)  ...maybe.
	GLFWwindow *window = createWindow(); // Check for error inside OpenGL.h

	// Time things
	// Alternatively could use gettimeofday() or clock(). But don't need that much precision
	DT_S = clock; // 60 Hz timer
	
	clock_t start, end;
	double elapsed;

	start = clock();

	// Init Chip ----
	//chip = (struct chip8*)malloc(sizeof(struct chip8)); // Do I even want chip as a pointer? 
	initChip();

	// More graphics setup things
	setupShaders();
	setupObjects(chip.disp);

	// Load ROM ----
	// Might be interesting to get list of available ROMS from folder and get user to choose
	//char* rom = "IBM Logo.ch8";
	char* rom = "chip8-test-suite.ch8";
	//char* rom = "bc_test.ch8";
	loadROM(rom);

	// Emulation Loop ----
	//int count = -2;
	for (;;) {
		//ITime = time(NULL);
		end = clock();
		elapsed = (double)(end - start) / (double)CLOCKS_PER_SEC;

		processInput(window); // Technically only need to check during E opcodes
		if (elapsed  > (1.0 / INSPEED)) {
			emulateChip8();
			start = clock();
		}
		
		
		if (chip.drawFlag) {

			for (int i = 0; i < 32 * 64; i++) {
				test[i] = chip.disp[i] * 0xFF;
			}
			setupObjects(test); // Confused why this can't just be once, probably something to do with updating textures
			chip.drawFlag = 0;
			//drawScreen();
			drawSquare(chip.disp);

		}
		//setupObjects(test); // NOTE: look into some roms crashing 
		drawSquare(chip.disp);
		// Render Code
		glfwSwapBuffers(window);
		glfwPollEvents();
		if (glfwWindowShouldClose(window)) {
			glfwTerminate();
		}
	}

	free(window);
	return 0;
}


void initChip() {
	// Set V registers to 0 (done by default i believe)
	// Do something with the Stack
	// Load the font into memory between 0x50 and 0x9F 

	chip.I = 0x0;
	chip.pc = 0x200;

	chip.ST = 0;
	chip.DT = 0;

	chip.drawFlag = 0;

	char fontset[] = {
		0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
		0x20, 0x60, 0x20, 0x20, 0x70, // 1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
		0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
		0x90, 0x90, 0xF0, 0x10, 0x10, // 4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
		0xF0, 0x10, 0x20, 0x40, 0x40, // 7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
		0xF0, 0x90, 0xF0, 0x90, 0x90, // A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
		0xF0, 0x80, 0x80, 0x80, 0xF0, // C
		0xE0, 0x90, 0x90, 0x90, 0xE0, // D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
		0xF0, 0x80, 0xF0, 0x80, 0x80  // F
	};

	memcpy(chip.memory + 0x50, fontset, sizeof(fontset));

	
	for (int i = 0; i < DISP_SIZE; i++) { // Not doing this probably caused crash
		chip.disp[i] = 0;
	}

	for (int i = 0; i < 16; i++) {
		chip.stack[i] = 0;
	}

	chip.SP = -1;
	
}

void loadROM(char* rom) {
	FILE* fp;
	
	if (fopen_s(&fp, rom, "rb")) {
		fprintf(stderr, "Cannot open file exiting");
		exit(1);
	}

	fread(chip.memory + 0x200, 1, 4096 - 0x200, fp); // Works under the assumption that rom is less than memory, though better implementation would look at filesize
	printf("ROM LOADED: %s\n", rom);
	fclose(fp);

	// --- Print Contents of the ROM 
	// NOTE: Could write dissassembler but probably won't
	/*unsigned char buffer[4096]; // Maybe should have better metric for this
	memcpy(buffer, chip.memory, 4096);
	for (int i = 0x200; i < 0x500; i += 2) {
		printf("%03x	 %02x%02x\n", i, (unsigned int)(unsigned char)buffer[i], (unsigned int)(unsigned char)buffer[i + 1]);
	}
	/**/
}

void drawScreen() {
	// Replace this with actual graphics eventually
	for (int i = 0; i < DISP_SIZE; i++) {
		if (chip.disp[i])
			printf("#");
		else
			printf("_");

		if (i % 64 == 0)
			printf("\n%02d ", i/64 + 1);
	}
	chip.drawFlag = 0;
	printf("\nDone Drawing\n");
}

void processInput(GLFWwindow* window) {
	// Could also have statement where escape quits the program or P prints debugging info
	// Thought about scancodes, but give me qwerty or give me death!
	chip.keyb[0x0] = (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS); // X
	chip.keyb[0x1] = (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS); // 1
	chip.keyb[0x2] = (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS); // 2
	chip.keyb[0x3] = (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS); // 3
	chip.keyb[0x4] = (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS); // Q
	chip.keyb[0x5] = (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS); // W
	chip.keyb[0x6] = (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS); // E
	chip.keyb[0x7] = (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS); // A
	chip.keyb[0x8] = (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS); // S
	chip.keyb[0x9] = (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS); // D
	chip.keyb[0xA] = (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS); // Z
	chip.keyb[0xB] = (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS); // C
	chip.keyb[0xC] = (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS); // 4
	chip.keyb[0xD] = (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS); // R
	chip.keyb[0xE] = (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS); // F
	chip.keyb[0xF] = (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS); // V
}

void emulateChip8() {
	//chip.keyb[1] = 1;
	// Fetch Instruction

		// PC contains location in memory not actual instruction, could grab first nibble and then second nibble
	// Should the pc be a number in memory or an address itself?
	uint16_t opcode = (chip.memory[chip.pc] << 8) | chip.memory[chip.pc + 1];
	
	//printChip(); Useful when using debugger

	chip.pc += 2;

	uint16_t x_cord, y_cord;

	uint16_t op, x, y, n, kk, nnn;
	op = opcode &  0xF000;
	n = opcode &   0x000F; // 4th nibble
	x = (opcode & 0x0F00) >> 8;		// 2nd nibble
	y = (opcode & 0x00F0) >> 4 ;		// 3rd nibble
	kk = opcode &  0x00FF;		// 3rd and 4th nibble
	nnn = opcode & 0x0FFF;		// 2nd, 3rd, and fourth nibble

	//Should I increment pc before or after decoding/executing?

	// Decode and Execute Instruction 
	//printf("PC: %x4", opcode); // Debug to Print Current Instruction
	switch (opcode & 0xF000) // Could have also used op since I don't use it elsewhere :')
	{
	case 0x0000:
		switch (opcode & 0xFF) // Does this mean 
		{
		case 0x00E0: //0x00E0 - CLS : clear display
			for (int i = 0; i < DISP_SIZE; i++) {
				chip.disp[i] = 0;
			}
			chip.drawFlag = 1;
			break;
		case 0x00EE: //0x00EE - RET : Return from a subroutine
			if (chip.SP != -1) {
				chip.pc = chip.stack[chip.SP]; // Do nothing if nothing in the stack i guess
				chip.SP--; // forgot to decrement stack pointer...
			}
				
			break;
		default: // 0x0nnn - SYS addr : could implement, but won't
			//unsupportedOp(opcode);
			break;
		}
		break;
	case 0x1000: // 1nnn - JP addr
		chip.pc = nnn;
		break;
	case 0x2000: // 2nnn - CALL addr 
		// Should I start SP at -1? and have program break if it tries to use ret when nothing in the stack
		chip.stack[++chip.SP] = chip.pc;
		chip.pc = nnn;
		break;
	case 0x3000: // 3xkk - SE Vx, byte : Skip next instruction if Vx = kk
		if (chip.V[x] == kk)
			chip.pc += 2;
		break;
	case 0x4000: // 4xkk - SNE Vx, byte : Skip next instruction if Vx != kk.
		if (chip.V[x] != kk)
			chip.pc += 2;
		break;
	case 0x5000: // 5xy0 - SE Vx, Vy : Skip next instruction if Vx = Vy.
		if (n != 0) unsupportedOp(opcode); // Doesn't end in 0 should I throw an error?
		if (chip.V[x] == chip.V[y])
			chip.pc += 2;
		break;
	case 0x6000: // 6xkk - LD Vx, Vy : Set Vx = kk
		chip.V[x] = kk;
		break;
	case 0x7000: // 7xkk - ADD Vx, byte
		//unsupportedOp(opcode);
		chip.V[x] += kk; 
		break;
	case 0x8000: // Arithmetic and Logic Opcodes
		switch (n)
		{
		case 0x0: // 8xy0 - LD Vx, Vy : Set Vx = Vy
			chip.V[x] = chip.V[y];
			break;
		case 0x1: // 8xy1 - OR Vx, Vy
			chip.V[x] = chip.V[x] | chip.V[y];
			break;
		case 0x2: // 8xy2 - AND Vx, Vy
			chip.V[x] = chip.V[x] & chip.V[y];
			break; 
		case 0x3: // 8xy3 - XOR Vx, Vy
			chip.V[x] = chip.V[x] ^ chip.V[y];
			break;
		case 0x4: // 8xy4 - ADD Vx, Vy
			chip.V[0xF] = chip.V[x] + chip.V[y] > 255;
			chip.V[x] = chip.V[x] + chip.V[y];
			break;
		case 0x5: // 8xy5 - SUB Vx, Vy
			chip.V[0xF] = chip.V[x] > chip.V[y];
			chip.V[x] = chip.V[x] - chip.V[y];
			break;
		case 0x6: // 8xy6 - SHR Vx {, Vy}
			// Choosing to forgo Setting Vx, to Vy for now... chip.V[x] = chip.V[y];
			chip.V[0xF] = chip.V[x] & 0x0001;
			chip.V[x] = chip.V[x] >> 1; // Could also divide by 2
			break;
		case 0x7: // 8xy7 - SUBN Vx, Vy
			chip.V[0xF] = chip.V[x] < chip.V[y];
			chip.V[x] =  chip.V[y] - chip.V[x];
			break;
		case 0xE: // 8xyE - SHL Vx {, Vy}
			// Choosing to forgo Setting Vx, to Vy for now... chip.V[x] = chip.V[y];
			chip.V[0xF] = chip.V[x] >> 7;
			chip.V[x] = chip.V[x] << 1; // Could also multiply by 2
			break;
		default:
			unsupportedOp(opcode); // Something gone horribly wrong
			break;
		}
		break;
	case 0x9000: // 9xy0 Skip next instruction if Vx != Vy
		if (chip.V[x] != chip.V[y])
			chip.pc += 2;
		break;
	case 0xA000: // Annn - LD I, addr : Set I reg to nnn 
		chip.I = nnn;
		break;
	case 0xB000: // Bnnn - JP V0, addr : The program counter is set to nnn plus the value of V0
		// Could also add the option of the BXNN version of this instruction
		chip.pc = nnn + chip.V[0];
		break;
	case 0xC000:
		chip.V[x] = (rand() % 256) & kk;
		break;
	case 0xD000: // Dxyn - DRW Vx, Vy, nibble : Also known as Pain
		x_cord = chip.V[x] % 64;
		y_cord = chip.V[y] % 32;
		chip.V[0xF] = 0;
		uint16_t pixel;

		for (int y1 = 0; y1 < n; y1++) {
			pixel = chip.memory[chip.I + y1];
			for (int x1 = 0; x1 < 8; x1++) {
				if ((pixel & (0x80 >> x1)) != 0) {

					if (chip.disp[(x1 + x_cord + ((y1 + y_cord) * 64))] == 1)
						chip.V[0xF] = 1;
					chip.disp[(x_cord + x1) + ((y_cord + y1) * 64)] ^= 1;
				}
				
				//x_cord++;
			}
			//y_cord++;
		}
		chip.drawFlag = 1;
		//unsupportedOp(opcode);
		break;
	case 0xE000:
		switch (kk)
		{
		case 0x9E: // Ex9E - SKP Vx
			if (chip.keyb[chip.V[x]])
				chip.pc += 2;
			break;
		case 0xA1: // ExA1 - SKNP Vx
			if (!chip.keyb[chip.V[x]])
				chip.pc += 2;
			break;
		default:
			unsupportedOp(opcode);
			break;
		}
		break;
	case 0xF000:
		switch (kk) {
		case 0x0007: // Fx07 - LD Vx, DT
			chip.V[x] = chip.DT;
			break;
		case 0x0015: // Fx15 - LD DT, Vx
			chip.DT = chip.V[x];
			break;
		case 0x0018: // Fx18 - LD ST, Vx
			chip.ST = chip.V[x];
			break;
		case 0x001E: // Fx1E - ADD I, Vx
			chip.I = chip.I + chip.V[x];
			// Set VF to 1 if overflows?
			break;
		case 0x000A: // Fx0A - LD Vx, K Pain
			// Could also check for keyb input here
			chip.pc -= 2;
			for (int i = 0; i < 16; i++) {// Don't press several keys at once and nothing breaks :)
				if (chip.keyb[i]) {
					chip.V[x] = i;
					chip.pc += 2; // Proceed forward if key has been pressed (COULD have done press and release)
					break;
				}
			}
			break;
		case 0x0029: // Fx29 - LD F, Vx
			chip.I = 0x50 + (5 * x); // Offset assuming font starts at 0x50 and are each 5 bytes
			break;
		case 0x0033: // Fx33 - LD B, Vx
			chip.memory[chip.I + 0] = chip.V[x] / 100;
			chip.memory[chip.I + 1] = (chip.V[x] / 10) % 10;//(chip.V[x] - ((chip.V[x] / 100) * 100)) / 10; // Feel like theres a simpler equation here
			chip.memory[chip.I + 2] = chip.V[x] % 10;
			break;
		case 0x0055: // Fx55 - LD [I], Vx
			for (int i = 0; i <= x; i++) {
				chip.memory[chip.I + i] = chip.V[i];
			}
			break;
		case 0x0065: // Fx65 - LD Vx, [I]
			for (int i = 0; i <= x; i++) { // Not gonna increment I for these instructions
				chip.V[i] = chip.memory[chip.I + i];
			}
			break;
		}
		break;
	default:
		unsupportedOp(opcode);
		break;
	}


	// Update Timers
	clock_t DT_E = clock();
	if (((double)(DT_E - DT_S) / (double)CLOCKS_PER_SEC) > (1.0 / 60.0)) {
		DT_S = clock();
		// Decrement Timer if period of 1/60 has passed
		if (chip.ST > 0) chip.ST--;
		if (chip.ST == 1) printf("Beep Noise\n"); 
		// Not gonna implement sound proper for now, though could add Beep() function to ST setting op

		if (chip.DT > 0) chip.DT--; 

	}
}