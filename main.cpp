/**************************************************************************************
 * Project Name: Exploring Multiple Processes and IPC
 * Brief desc: An computer emulator that has a CPU and memory.
***************************************************************************************/

#include <stdlib.h>

#include <cstdlib>

#include <cstring>

#include <unistd.h>

#include <stdio.h>

#include <iostream>

#include <sys/types.h>

#include <time.h>

#include <cstdlib>

#include <iostream>

#include <fstream>

#include "LineInfo.h"

using namespace std;

int const READ = 0;
int const WRITE = 1;
int const PIPE_ERROR = -1;
int const FORK_ERROR = -1;
int const CHILD_PID = 0;

/**************************************************************************************
 * Function name: memory_check
 * Function args: int PC, bool kernel
 * Function return: bool value
 * Function Purpose: Check if the PC is access an an area of memory that it should not
***************************************************************************************/
bool memory_check(int PC, bool kernel) {
  if (PC >= 1000 && !kernel) {
    cout << "Memory violation: accessing system address " << PC << " in user mode " << endl;
    return false;
  } else
    return true;
}

/**************************************************************************************
 * Function name: readinputfile
 * Function args: int memory[], const char * file_name
 * Function return: void
 * Function Purpose: Load the data from the file into the memory array 
***************************************************************************************/
void readinputfile(int memory[], const char * file_name) {

  FILE * inputFile;
  int i = 0;    // keep track of the index of memory
  char input[1000];                // make a variable to catch the input

  inputFile = fopen(file_name, "r");                  // open the file

  if(inputFile == NULL)            // check if the file opened
    throw domain_error(LineInfo("file opening error", __FILE__, __LINE__));


  while (fgets(input, sizeof(input), inputFile)) {    // for loop to store in the input from the file to the input pointer array
    int num;
    if (input[0] == '.') {         // if the input starts with an .
      sscanf(input, ".%d %*s\n", & num);
      i = num;
      memory[i] = num;
    } else if (isdigit(input[0])) {// if the input starts with a digit
      sscanf(input, "%d %*s\n", & num);
      memory[i] = num;
      i++;
    }
  }

  fclose(inputFile);               // close the file

}

int main(int argc, char * argv[]) {

  try {
    if (argc != 3)                // check if the correct number of arguments are given
      throw domain_error(LineInfo("Usage: /main [program_file] [interrupt_frequency]", __FILE__, __LINE__));

    string file_name = argv[1];   // store the file_name
    int timer = atoi(argv[2]);    // store the timer

    int cpu_to_memory[2];         // create a array for the pipe
    int memory_to_cpu[2];         // create a array for the pipe
    int pid;
    int memory[2000] = { 0 };     // memory array

               // create pipes    
    if (pipe(cpu_to_memory) == PIPE_ERROR)
      throw domain_error(LineInfo("Unable to create pipe cpu_to_memory", __FILE__, __LINE__));

    if (pipe(memory_to_cpu) == PIPE_ERROR)
      throw domain_error(LineInfo("Unable to create pipe memory_to_cpu", __FILE__, __LINE__));

    cout << "Program Loading ..." << endl << endl;

    readinputfile(memory, file_name.c_str());

    cout << "Program Loaded" << endl << endl;

    pid = fork();                // fork and store the pid

    if (pid == FORK_ERROR)       // check for Fork Error
      throw domain_error(LineInfo("Fork Error", __FILE__, __LINE__));

    else if (pid != CHILD_PID) { 
      /******************-----------------Parent/CPU process---------------*****************************/

              // close the pipes that are not needed
      if (close(cpu_to_memory[READ]) == PIPE_ERROR)
        throw domain_error(LineInfo("close read side of the cpu_to_memory pipe", __FILE__, __LINE__));
      if (close(memory_to_cpu[WRITE]) == PIPE_ERROR)
        throw domain_error(LineInfo("close write side of the memory_to_cpu pipe", __FILE__, __LINE__));

              // initialize and set the registers
      int PC, IR, AC, X, Y, SP;
      int local_operand;
      SP = 1000;
      PC = 0;
      IR = 0;
      AC = 0;
      X = 0;
      Y = 0;

      bool kernel = false;       // bool variable to keep track of the mode
      int interrupt_flag = 0;    // 2: Syscall || 1: Timer Interrupt || 0: Not in an interrupt
      bool perform_timer_interrupt = false;    // keep track of if its time for an timer interrupt
      int instruction_counter = 0;  // keep track of the number of instructions passed
      int writing_to_mem = -1;   // write flag

      while (true) {

        if (perform_timer_interrupt == true && interrupt_flag == 0) {
          perform_timer_interrupt = false; 
          interrupt_flag = 1;    // 1: Timer Interrupt

          kernel = true;         // swich to kernal mode
          local_operand = SP;    // save the current SP locally
          SP = 2000;

              //Save PC onto System Stack
          SP--;
          write(cpu_to_memory[WRITE], & writing_to_mem, sizeof(writing_to_mem));
          write(cpu_to_memory[WRITE], & SP, sizeof(SP));
          write(cpu_to_memory[WRITE], & PC, sizeof(PC));

             //Save SP onto System Stack
          SP--;
          write(cpu_to_memory[WRITE], & writing_to_mem, sizeof(writing_to_mem));
          write(cpu_to_memory[WRITE], & SP, sizeof(SP));
          write(cpu_to_memory[WRITE], & local_operand, sizeof(local_operand));

          PC = 1000;             // set the PC to the system area
        }
         // Fetch the instruction from memory
        write(cpu_to_memory[WRITE], & PC, sizeof(PC));
        read(memory_to_cpu[READ], & IR, sizeof(IR));

         //execute the instruction
        switch (IR) {
          
          case 1:  // Load the value into the AC
            PC++;  // move to the next instruction to read in the operand
            if (!memory_check(PC, kernel))
              _exit(0);
            write(cpu_to_memory[WRITE], & PC, sizeof(PC));  // write to request the value at the PC location
            read(memory_to_cpu[READ], & local_operand, sizeof(local_operand));  // read back the value
            AC = local_operand;  // assign the value returned by the memory to the AC
            break;

          case 2:  // Load the value at the address into the AC
             // we read in the operand and that operand is the address of the value we need to load
            PC++;  // move to the next instruction to read in the operand
            if (!memory_check(PC, kernel))
              _exit(0);
            write(cpu_to_memory[WRITE], & PC, sizeof(PC));  // write to request the value at the PC location
            read(memory_to_cpu[READ], & local_operand, sizeof(local_operand));  // read back the value
            
            if (!memory_check(local_operand, kernel))
              _exit(0);
            write(cpu_to_memory[WRITE], & local_operand, sizeof(local_operand));  // write to request the value at the PC location
            read(memory_to_cpu[READ], & local_operand, sizeof(local_operand));  // read back the value

            AC = local_operand;
            break;

          case 3: {  //Load the value from the address found in the given address into the AC
            PC++;  // move to the next instruction to read in the operand
            if (!memory_check(PC, kernel))
              _exit(0);
            write(cpu_to_memory[WRITE], & PC, sizeof(PC));  // write to request the value at the PC location (this is the addrs that we need to access)
            read(memory_to_cpu[READ], & local_operand, sizeof(local_operand));  // read back the addr (in this case we have gotten the addr value from load addr)

            if (!memory_check(local_operand, kernel))
              _exit(0);
            write(cpu_to_memory[WRITE], & local_operand, sizeof(local_operand));  //input the found addr value into write pipe to get what is the value at the addr location
            read(memory_to_cpu[READ], & local_operand, sizeof(local_operand));  //read the value returned by memory

            if (!memory_check(local_operand, kernel))
              _exit(0);
            write(cpu_to_memory[WRITE], & local_operand, sizeof(local_operand));  // input the value of the addr (100)in the example into the pipe to get the value at that location
            read(memory_to_cpu[READ], & local_operand, sizeof(local_operand));  //read the value returned by memory
            
            AC = local_operand;  // assign to AC
            break;
          }

          case 4: {
            PC++;  // move to the next instruction to read in the operand
            if (!memory_check(PC, kernel))
              _exit(0);
            write(cpu_to_memory[WRITE], & PC, sizeof(PC));  // write to request the value at the PC location
            read(memory_to_cpu[READ], & local_operand, sizeof(local_operand));  // read back the value

             // add the X variable to the value
            local_operand = local_operand + X;

            if (!memory_check(local_operand, kernel))
              _exit(0);
            write(cpu_to_memory[WRITE], & local_operand, sizeof(local_operand));  // write to request the value at the local_operand location
            read(memory_to_cpu[READ], & local_operand, sizeof(local_operand));  // read back the value

            AC = local_operand;
            break;
          }

          case 5: {
            PC++;  // move to the next instruction to read in the operand

            if (!memory_check(PC, kernel))
              _exit(0);
            write(cpu_to_memory[WRITE], & PC, sizeof(PC));  // write to request the value at the PC location
            read(memory_to_cpu[READ], & local_operand, sizeof(local_operand));  // read back the value

             // add the X variable to the value
            local_operand = local_operand + Y;

            if (!memory_check(local_operand, kernel))
              _exit(0);
            write(cpu_to_memory[WRITE], & local_operand, sizeof(local_operand));  // write to request the value at the PC location
            read(memory_to_cpu[READ], & local_operand, sizeof(local_operand));  // read back the value

            AC = local_operand;
            break;
          }

          case 6: {  //Load from (Sp+X) into the AC
            local_operand = SP + X;

            if (!memory_check(local_operand, kernel))
              _exit(0);
            write(cpu_to_memory[WRITE], & local_operand, sizeof(local_operand));  // write to request the value at the local_operand location
            read(memory_to_cpu[READ], & local_operand, sizeof(local_operand));  // read back the value

            AC = local_operand;
            break;
          }

          case 7: {  //Store the value in the AC into the address (Store addr)
            PC++;  // move to the next instruction to read in the operand
            if (!memory_check(PC, kernel))
              _exit(0);
            write(cpu_to_memory[WRITE], & PC, sizeof(PC));  // write to request the value at the PC location
            read(memory_to_cpu[READ], & local_operand, sizeof(local_operand));  // read back the value

            if (!memory_check(local_operand, kernel))  // make sure you are writing to the user memory only
              _exit(0);
            write(cpu_to_memory[WRITE], & writing_to_mem, sizeof(writing_to_mem));
            write(cpu_to_memory[WRITE], & local_operand, sizeof(local_operand));
            write(cpu_to_memory[WRITE], & AC, sizeof(AC));
            break;
          }

          case 8: {  // Gets a random int from 1 to 100 into the AC
            AC = (rand() % (100)) + 1;
            break;
          }

          case 9: {  //Put port
             //If port=1, writes AC as an int to the screen
             //If port=2, writes AC as a char to the screen
            PC++;  // read in the operand

            if (!memory_check(PC, kernel))
              _exit(0);
            write(cpu_to_memory[WRITE], & PC, sizeof(PC));  // write to request the value at the PC location
            read(memory_to_cpu[READ], & local_operand, sizeof(local_operand));  // read in the value

            if (local_operand == 1)  // print as int
              printf("%i", AC);
            if (local_operand == 2)  // print as char
              printf("%c", AC);
            break;

          }

          case 10: {  // Add X to AC
            AC = AC + X; 
            break;
          }

          case 11: {  // Add Y to AC
            AC = AC + Y;
            break;
          }

          case 12: {  //Subtract the value in X from the AC
            AC = AC - X;
            break;
          }

          case 13: {  //Subtract the value in Y from the AC
            AC = AC - Y;
            break;
          }

          case 14: {  // Copy the value in the AC to X
            X = AC;
            break;
          }

          case 15: {  // Copy the value in X to the AC
            AC = X;
            break;
          }

          case 16: {  // Copy the value in the AC to Y
            Y = AC;
            break;
          }

          case 17: {  // Copy the value in Y to the AC
            AC = Y;
            break;
          }

          case 18: {  // Copy the value in AC to the SP 
            SP = AC;
            break;
          }

          case 19: {  //Copy the value in SP to the AC
            AC = SP;
            break;
          }

          case 20: {  //Jump addr
            PC++;  // move to the next instruction to read in the operand/addr
            if (!memory_check(PC, kernel))
              _exit(0);
            write(cpu_to_memory[WRITE], & PC, sizeof(PC));  // write to request the value at the PC location
            read(memory_to_cpu[READ], & local_operand, sizeof(local_operand));  // read back the value
            PC = local_operand - 1;

            break;
          }

          case 21: {  //if AC = 0 jump addr
            PC++;  // read in the addr
            if (!memory_check(PC, kernel))  // check for memory violation
              _exit(0);
            if (AC == 0) {
              write(cpu_to_memory[WRITE], & PC, sizeof(PC));  // write to request the value at the PC location
              read(memory_to_cpu[READ], & PC, sizeof(local_operand));  // read back the value
              PC = PC - 1;
            }
            break;
          }

          case 22: {  //Jump to the address only if the value in the AC is not zero
            PC++;
            if (!memory_check(PC, kernel))
              _exit(0);
            if (AC != 0) {
              write(cpu_to_memory[WRITE], & PC, sizeof(PC));  // write to request the value at the PC location
              read(memory_to_cpu[READ], & PC, sizeof(local_operand));  // read back the value
              PC = PC - 1;
            }
            break;
          }

          case 23: {  //Push return address onto stack, jump to the address
            PC++;  // read in the addr
            if (!memory_check(PC, kernel))
              _exit(0);
            write(cpu_to_memory[WRITE], & PC, sizeof(PC));  //ask for value at mem[operand]
            read(memory_to_cpu[READ], & local_operand, sizeof(local_operand));  //read the value returned by memory

             //Push current address onto stack
            SP--;
            write(cpu_to_memory[WRITE], & writing_to_mem, sizeof(writing_to_mem));
            write(cpu_to_memory[WRITE], & SP, sizeof(SP));
            write(cpu_to_memory[WRITE], & PC, sizeof(PC));

             // move to the new addr (-1 since we have an increment at the end of case statement)
            PC = local_operand - 1;
            break;
          }
          case 24: {
             //Pop return address from the stack, jump to the address
            write(cpu_to_memory[WRITE], & SP, sizeof(SP));  //ask for value at mem[operand]
            read(memory_to_cpu[READ], & local_operand, sizeof(local_operand));  //read the value returned by memory
            SP++;

            PC = local_operand;
            break;
          }

          case 25: {  //Increment the value in X
            X++;
            break;
          }

          case 26: {  //Decrement the value in X
            X--;
            break;
          }

          case 27: {  //Push AC onto stack
            SP--;
            write(cpu_to_memory[WRITE], & writing_to_mem, sizeof(writing_to_mem));
            write(cpu_to_memory[WRITE], & SP, sizeof(SP));
            write(cpu_to_memory[WRITE], & AC, sizeof(AC));
            break;
          }

          case 28: {  //Pop from stack into AC
            write(cpu_to_memory[WRITE], & SP, sizeof(SP));  //ask for value at mem[operand]
            read(memory_to_cpu[READ], & AC, sizeof(AC));  //read the value returned by memory
            SP++;
            break;
          }

          case 29: {
             //Perform system call

             //CHECK IF WE ARE ALREADY IN A INTERRUPT
            if (interrupt_flag != 0)
              break;
            else {  // otherwise
              interrupt_flag = 2;
              kernel = true;  // change mode

              local_operand = SP;  // store SP locally
              SP = 2000;  // set SP to system SP

              PC++;  // move to the next instuction so that we are storing the correct one

               // store PC on the system Stack
              SP--;
              write(cpu_to_memory[WRITE], & writing_to_mem, sizeof(writing_to_mem));
              write(cpu_to_memory[WRITE], & SP, sizeof(SP));
              write(cpu_to_memory[WRITE], & PC, sizeof(PC));

               // store SP on the system Stack
              SP--;
              write(cpu_to_memory[WRITE], & writing_to_mem, sizeof(writing_to_mem));
              write(cpu_to_memory[WRITE], & SP, sizeof(SP));
              write(cpu_to_memory[WRITE], & local_operand, sizeof(local_operand));

              PC = 1499;  //bc of the PC++ at the end
            }
            break;

          }

          case 30: {
             //Return from system call
            write(cpu_to_memory[WRITE], & SP, sizeof(SP));  //ask for value at mem[operand]
            read(memory_to_cpu[READ], & local_operand, sizeof(local_operand));  //read the value returned by memory
            SP++;

            write(cpu_to_memory[WRITE], & SP, sizeof(SP));  //ask for value at mem[operand]
            read(memory_to_cpu[READ], & PC, sizeof(PC));  //read the value returned by memory
            SP++;

            if (interrupt_flag == 1)  // we are returning from a timer interrupt
              perform_timer_interrupt = false;

             // RESTORE FLAGS
            interrupt_flag = 0;
            kernel = false;
            SP = local_operand;

            PC--;  // since we are going to increment at the end of the case statements
            break;
          }

          case 50: {  //End execution
            _exit(0);
          }

          default: {
            cout << "Error: The IR is not part of the instruction set" << endl;
            _exit(0);
          }

        }

        if (instruction_counter % timer == 0 && instruction_counter != 0)  // check if its time for a timer interrupt
          perform_timer_interrupt = true;

         // increment PC and counter
        instruction_counter++;
        PC++;

         //check if the new PC location able to be accessed
        if (!memory_check(PC, kernel))
          _exit(0);

      }
    } else {
      /******************-----------------Child/MEMORY process---------------*****************************/

       //close the not used pipes
      if (close(cpu_to_memory[WRITE]) == PIPE_ERROR)
        throw domain_error(LineInfo("close WRITE side of the cpu_to_memory pipe", __FILE__, __LINE__));
      if (close(memory_to_cpu[READ]) == PIPE_ERROR)
        throw domain_error(LineInfo("close READ side of the memory_to_cpu pipe", __FILE__, __LINE__));

       // local variables
      int PC;
      int write_value;
      int write_address;

      while (true) {

        read(cpu_to_memory[READ], & PC, sizeof(PC));  //read in the instruction fetch

         //-1 means that we write the data to the address 
        if (PC == -1) {
          read(cpu_to_memory[READ], & write_address, sizeof(write_address));
          read(cpu_to_memory[READ], & write_value, sizeof(write_value));
          memory[write_address] = write_value;
        } else {  //we are reading what instruction is at the PC and sending it back
          write(memory_to_cpu[WRITE], & memory[PC], sizeof(memory[PC]));
        }
      }
      _exit(0);
      return 0;
    }

  }  //try
  catch (exception & e) {
    cout << e.what() << endl;
    cout << endl << "Press the enter key once or twice to leave..." << endl;
    cin.ignore();
    cin.get();
    exit(EXIT_FAILURE);
  }  //catch

  exit(EXIT_SUCCESS);
}