**I.	Project Purpose**

The purpose of this project was to create a simple computer system consisting of a CPU and Memory using two processes that run simultaneously. In this project, we explored concepts like forks, pipes, memory management, fetch and execute behavior, stack processing, computer emulation, I/O, Interrupt handling, and much more. This project helped solidify the concepts relating to memory, CPU and low-level operating systems. The overall objective of this project was to create a computer emulator that could execute one program. Some of the deliverables included: a memory process, a CPU process, interrupt processing (timer and syscall), and memory protection.

**II.	Project Implementation**

**a.	Implementation of reading in the user program**

To read the data from the file I created a separate function called readinputfile with the arguments as the memory array and filename. In the function, I first initialize an object of the type FILE and open it in read mode. Then I set a buffer called input to read the data. Finally, I create a while loop that reads the file line by line into the buffer. In the while loop if the read in input starts with .number then the memory array is set to the number location. If the input starts with a number, that number is read in set to memory index I, then we increment i. Another other type of input is ignored. The while loop ends when the EOF is read in. Lastly, I close the FILE object. This function does not return anything since the memory array is initialized in the main program and passed to the function.

**b.	Implementation of the Memory**

The memory is implemented as the child process of the fork. We begin the child/memory process by closing the pipes that will not be used by the child. This includes the cpu_to_memory[WRITE] and memory_to_cpu[READ]. Next, we initialize local variables for PC, value, and address; these are to store the data read in from the pipe. Then we create an infinite loop that reads in the PC from the CPU, and if the PC is -1, meaning we have a write flag, then the memory reads in the address and value from the CPU and assigns the memory at the address location to the value. If the PC is not -1 (meaning no write flag), then the memory fetches the value at the PC location and writes it to the CPU process.

**c.	Implementation of the CPU**

The CPU is implemented in the parent process of the fork. We begin the CPU process by closing the pipes that will not be used. This includes the cpu_to_memory[READ] and memory_to_cpu[WRITE]. Next, we initialize registers for the CPU like SP, PC, IR, AC, X, and Y. Additionally, we also create other variables like kernel_flag, write_flag, timer_interrupt_flag, type_interrupt, and timer_counter. Then we create an infinite loop that fetches and executes instructions and interrupts.

First, we check if we need to perform a timer interrupt by checking the flag for timer_interrupt and making sure that we are not already in an interrupt. If we need to perform the interrupt then we change the status of the flags to show that we are currently in an interrupt and switch to kernel mode. Next, we switch to the system and save the current/user PC and SP on the system stack. Lastly, we set the PC to 1000, so that we can execute the interrupt handler code/instructions.

Now, we have a set location for our PC, 1000 if in an interrupt. The CPU writes the PC to the memory and the memory returns the value at that address, which the CPU reads into the IR register. Next, a switch is implemented to decode the IR instruction into specific actions. This report will only go over some instructions, and detailed implementation information for each instruction can be found via the comments in the source code. For example, if the IR is 1 then the PC is incremented to read the operand and load the operands into the AC register. Additionally, a memory check is placed to make sure that the PC is only accessing the user memory in kernel mode. Another example, If IR is 29 that means we have to perform a sys call. In this we first check if we are already in an interrupt, if not, then we change the interrupt_flag to 2 to represent being in a system call and activate kernel mode. The next actions are the same as a timer interrupt, in which we switch to the stack pointer and store the SP and PC. However, we set the PC to 1499 instead of 1000 for the syscall interrupt. Lastly, IR 30 is used to return from syscall. In this instruction, we restore the SP and PC to the user memory and set the interrupt flags and kernel back to negative. Also if we are returning from a timer interrupt we set that flag to false. Finally, we decrement the PC to override the PC increment at the end of case statements.

After the IR instruction is executed through the case statement, We check if we need to perform a timer interrupt by taking the mod of the instruction counter and timer value. Lastly, we increment the instruction_counter and PC values. This marks the end of one CPU fetch and execution. The process is repeated for all other instructions.

