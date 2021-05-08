from pyparsing import *
import sys
from prewritten_ram import prewritten_values

#==================================================================================
# Configurations
#==================================================================================

QFTASM_RAM_AS_STDIN_BUFFER = True
QFTASM_RAM_AS_STDOUT_BUFFER = True

# QFTASM_RAMSTDIN_BUF_STARTPOSITION = 7167
# QFTASM_RAMSTDOUT_BUF_STARTPOSITION = 8191
# QFTASM_RAMSTDIN_BUF_STARTPOSITION = (4499 - 1024)
# QFTASM_RAMSTDOUT_BUF_STARTPOSITION = (4999 - 1024)

QFTASM_RAMSTDIN_BUF_STARTPOSITION = 350 #4095-1024-512-390-25
QFTASM_RAMSTDOUT_BUF_STARTPOSITION = 823 #4095-1024-(512-32)-390-25 #4095-1024

QFTASM_NEGATIVE_BUFFER_SIZE = 200

# QFTASM_RAMSTDIN_BUF_STARTPOSITION = 125-64
# QFTASM_RAMSTDOUT_BUF_STARTPOSITION = 127-64

debug_ramdump = True
debug_ramdump_verbose = False
debug_plot_memdist = False   # Requires numpy and matplotlib when set to True
use_stdio = True
stdin_from_pipe = True

QFTASM_MEMORY_WRAP = 1024

#==================================================================================
QFTASM_STDIO_OPEN = 1 << 8
QFTASM_STDIO_CLOSED = 1 << 9
QFTASM_STDIN = 1
QFTASM_STDOUT = 2

class Parser(object):
    def __init__(self):
        addressing_mode = Optional(Char("ABC")).setParseAction(
            lambda t: {"A": 1, "B": 2, "C": 3}[t[0]] if len(t) > 0 else 0
        )
        integer = (Optional("-") + Word(nums)).setParseAction(
            lambda t: int("".join(t))
        )
        operand = Group(addressing_mode + integer)
        MNZ = Literal("MNZ")
        MLZ = Literal("MLZ")
        ADD = Literal("ADD")
        SUB = Literal("SUB")
        AND = Literal("AND")
        OR = Literal("OR")
        XOR = Literal("XOR")
        ANT = Literal("ANT")
        SL = Literal("SL")
        SRL = Literal("SRL")
        SRA = Literal("SRA")
        SRU = Literal("SRU")
        SRE = Literal("SRE")
        opcode = MNZ | MLZ | ADD | SUB | AND | OR | XOR | ANT | SL | SRL | SRA | SRU | SRE
        lineno = (integer + Literal(".")).suppress()
        comment = (Literal(";") + restOfLine).suppress()
        inst = (lineno + opcode + operand + operand + operand + Optional(comment)).setParseAction(
            lambda t: [t]
        )
        self.program = ZeroOrMore(inst)

    def parse_string(self, string):
        return self.program.parseString(string)

def decode_stdin_buffer(stdin_buf):
    ret = []
    for b in stdin_buf:
        ret.append(b & 0b0000000011111111)
        ret.append(b >> 8)
    return "".join([chr(i) for i in ret])


def interpret_file(filepath):
    def wrap(x):
        return x & ((1 << 16) - 1)
    # def sub(a,b,c):
    #     if (wrap(a-b + (1 << 16)) == 0) ^ ((a^b) == 0):
    #         print("sub:", wrap(a-b + (1 << 16)), "xor:", a^b)
    #     return (True, wrap(a-b + (1 << 16)), c)
    d_inst = {
        "MNZ": lambda a, b, c: (True, b, c) if a != 0 else (False, None, None),
        "MLZ": lambda a, b, c: (True, b, c) if (wrap(a) >> 15) == 1 else (False, None, None),
        "ADD": lambda a, b, c: (True, wrap(a+b), c),
        "SUB": lambda a, b, c: (True, wrap(a-b + (1 << 16)), c),
        # "SUB": sub,
        "AND": lambda a, b, c: (True, (a & b), c),
        "OR" : lambda a, b, c: (True, (a | b), c),
        "XOR": lambda a, b, c: (True, (a ^ b), c),
        "ANT": lambda a, b, c: (True, (a & (~b)), c),
        "SL" : lambda a, b, c: (True, (a << b), c),
        "SRL": lambda a, b, c: (True, (a >> b), c),
        "SRA": lambda a, b, c: (True, (a & (1 << 7)) ^ (a & ((1 << 7) - 1) >> b), c),
        "SRU": lambda a, b, c: (True, (b >> 1), c),
        "SRE": lambda a, b, c: (True, (b >> 8), c),
    }

    parser = Parser()


    with open(filepath, "rt") as f:
        rom_lines = f.readlines()
    rom = rom_lines
    ram = []
    rom_reads = [0 for _ in range(len(rom))]
    for _ in range(1 << 16):
        ram.append([0,0])

    for addr, value in prewritten_values:
        ram[addr%QFTASM_MEMORY_WRAP][0] = value

    if use_stdio:
        stdin_counter = 0
        read_stdin_flag = False
        stdout_ready_flag = False

        if QFTASM_RAM_AS_STDIN_BUFFER:
            python_stdin = sys.stdin.read()
            python_stdin_int = [ord(c) for c in python_stdin]
            if len(python_stdin_int) % 2 == 1:
                python_stdin_int = python_stdin_int + [0]

            for i_str, i in enumerate(python_stdin_int):
                # ram[QFTASM_RAMSTDIN_BUF_STARTPOSITION - i_str][0] = ord(c)
                # ram[QFTASM_RAMSTDIN_BUF_STARTPOSITION - i_str][1] += 1
                if i_str % 2 == 0:
                    stdin_int = i
                else:
                    stdin_int += i << 8

                if i_str % 2 == 1 or i_str == len(python_stdin_int) - 1:
                    ram[QFTASM_RAMSTDIN_BUF_STARTPOSITION + i_str//2][0] = stdin_int
                    ram[QFTASM_RAMSTDIN_BUF_STARTPOSITION + i_str//2][1] += 1

        elif stdin_from_pipe:
            python_stdin = sys.stdin.read()
        else:
            python_stdin = ""

    n_steps = 0

    prev_result_write_flag = False
    prev_result_value = 0
    prev_result_dst = 0

    # Main loop
    while ram[0][0] < len(rom):
        # if n_steps > 300000:
        #     break
        # 1. Fetch instruction
        pc = ram[0][0]
        inst = rom[pc]
        rom_reads[pc] += 1
        if type(inst) == str:
            rom[pc] = parser.parse_string(inst)[0]
            inst = rom[pc]
        opcode, (mode_1, d1), (mode_2, d2), (mode_3, d3) = inst

        # 2. Write the result of the previous instruction to the RAM
        if prev_result_write_flag:
            ram[prev_result_dst % QFTASM_MEMORY_WRAP][0] = prev_result_value
            ram[prev_result_dst % QFTASM_MEMORY_WRAP][1] += 1

        QFTASM_HEAP_MEM_MAX = QFTASM_RAMSTDOUT_BUF_STARTPOSITION + 1

        # 3. Read the data for the current instruction from the RAM
        for _ in range(mode_1):
            if QFTASM_HEAP_MEM_MAX < d1 < 32768 or 32768 <= d1 < 65536 - QFTASM_NEGATIVE_BUFFER_SIZE:
                print("Address overflow at pc", pc, "with address", d1, "(d1), n_steps:", n_steps)
                # exit()
            ram[d1%QFTASM_MEMORY_WRAP][1] += 1
            d1 = ram[d1%QFTASM_MEMORY_WRAP][0]
        for _ in range(mode_2):
            if QFTASM_HEAP_MEM_MAX < d2 < 32768 or 32768 <= d2 < 65536 - QFTASM_NEGATIVE_BUFFER_SIZE:
                print("Address overflow at pc", pc, "with address", d2, "(d2), n_steps:", n_steps)
                # exit()
            ram[d2%QFTASM_MEMORY_WRAP][1] += 1
            d2 = ram[d2%QFTASM_MEMORY_WRAP][0]
        for _ in range(mode_3):
            if QFTASM_HEAP_MEM_MAX < d3 < 32768 or 32768 <= d3 < 65536 - QFTASM_NEGATIVE_BUFFER_SIZE:
                print("Address overflow at pc", pc, "with address", d3, "(d3), n_steps:", n_steps)
                # exit()
            ram[d3%QFTASM_MEMORY_WRAP][1] += 1
            d3 = ram[d3%QFTASM_MEMORY_WRAP][0]

        # 4. Compute the result
        prev_result_write_flag, prev_result_value, prev_result_dst = d_inst[opcode](d1, d2, d3)

        # 5. Increment the program counter
        ram[0][0] += 1
        ram[0][1] += 1


        # 6 (Extended): Manage the values of stdin and stdout
        if use_stdio:
            # stdin
            if not QFTASM_RAM_AS_STDIN_BUFFER:
                stdin = ram[QFTASM_STDIN][0]
                if pc > 1 and stdin == QFTASM_STDIO_OPEN:
                    if not stdin_from_pipe and stdin_counter >= len(python_stdin):
                        python_stdin += sys.stdin.readline()

                    if stdin_counter < len(python_stdin):
                        c = python_stdin[stdin_counter]
                        c_int = ord(c)
                        ram[QFTASM_STDIN][0] = c_int
                        stdin_counter += 1
                    else:
                        ram[QFTASM_STDIN][0] = 0

            if not QFTASM_RAM_AS_STDOUT_BUFFER:
                stdout = ram[QFTASM_STDOUT][0]
                # stdout
                if pc > 1 and stdout == QFTASM_STDIO_CLOSED:
                    stdout_ready_flag = True

                if pc > 1 and stdout != QFTASM_STDIO_CLOSED and stdout_ready_flag:
                    stdout_wrapped = stdout & ((1 << 8) - 1)
                    stdout_char = chr(stdout_wrapped)
                    print(stdout_char, end="")

                    stdout_ready_flag = False

        n_steps += 1

    if QFTASM_RAM_AS_STDOUT_BUFFER:
        stdout_buf = "".join([chr(ram[i_stdout][0] & ((1 << 8) - 1)) for i_stdout in reversed(range(ram[2][0]+1,QFTASM_RAMSTDOUT_BUF_STARTPOSITION+1))])
        print("".join(stdout_buf), end="")


    if debug_ramdump:
        print()
        n_nonzero_write_ram_indices = []
        n_nonzero_write_count_ram = 0
        n_nonzero_write_count_ram_maxindex = -1
        for i_index, (v, times) in enumerate(ram):
            if times > 0:
                n_nonzero_write_ram_indices.append(i_index)
                n_nonzero_write_count_ram += 1
                n_nonzero_write_count_ram_maxindex = max(n_nonzero_write_count_ram_maxindex, i_index)
        if QFTASM_RAM_AS_STDIN_BUFFER:
            stdin_buf = []
            i_stdin = QFTASM_RAMSTDIN_BUF_STARTPOSITION
            while ram[i_stdin][0]:
                # c = chr(ram[i_stdin][0] & ((1 << 8) - 1))
                stdin_buf.append(ram[i_stdin][0])
                i_stdin += 1
            # stdin_buf = "".join(stdin_buf)
            # print("stdin buffer: {}".format(stdin_buf))
            print("stdin buffer: {}".format(decode_stdin_buffer(stdin_buf)))

        print("ROM size: {}".format(len(rom_lines)))
        print("n_steps: {}".format(n_steps))
        print("Nonzero write count ram addresses: {}".format(n_nonzero_write_count_ram))
        # print(n_nonzero_write_ram_indices)
        print("Nonzero write count ram max address: {}".format(n_nonzero_write_count_ram_maxindex))

        print(ram[:20])

        print(list(reversed(ram[QFTASM_RAMSTDOUT_BUF_STARTPOSITION-20:QFTASM_RAMSTDOUT_BUF_STARTPOSITION+1])))

        # Requires numpy and matplotlib
        x = range(-QFTASM_NEGATIVE_BUFFER_SIZE,QFTASM_RAMSTDOUT_BUF_STARTPOSITION)
        import numpy as np
        import matplotlib.pyplot as plt
        a = np.array(ram[:n_nonzero_write_count_ram_maxindex+1])
        ramarray = np.array(ram)
        if debug_plot_memdist:

            plt.figure()
            # plt.plot(x, np.log(np.hstack((a[-QFTASM_NEGATIVE_BUFFER_SIZE:,1], a[:QFTASM_RAMSTDOUT_BUF_STARTPOSITION,1]))+1), "o-")
            plt.plot(np.log(ramarray[:QFTASM_MEMORY_WRAP,1]+1), "o-")
            # plt.plot(np.log(np.hstack((a[-50:,1], a[:QFTASM_RAMSTDOUT_BUF_STARTPOSITION,1]))+1), "o-")
            plt.savefig("./memdist.png")

            plt.figure()
            plt.plot(x, np.log(np.hstack((a[-QFTASM_NEGATIVE_BUFFER_SIZE:,1], a[:QFTASM_RAMSTDOUT_BUF_STARTPOSITION,1]))+1), "o-")
            # plt.plot(np.log(np.hstack((a[-50:,1], a[:QFTASM_RAMSTDOUT_BUF_STARTPOSITION,1]))+1), "o-")
            plt.savefig("./memdist_negative.png")

            plt.figure()
            plt.plot(np.log(ramarray[:,1]+1), "o-")
            # plt.plot(np.log(np.hstack((a[-50:,1], a[:QFTASM_RAMSTDOUT_BUF_STARTPOSITION,1]))+1), "o-")
            plt.savefig("./memdist2.png")

            plt.figure()
            plt.plot(np.log(ramarray[:,0]+1), "o-")
            # plt.plot(np.log(np.hstack((a[-50:,1], a[:QFTASM_RAMSTDOUT_BUF_STARTPOSITION,1]))+1), "o-")
            plt.savefig("./memvaluedist.png")

            plt.figure()
            plt.plot(np.log(a[-QFTASM_NEGATIVE_BUFFER_SIZE:,1]+1), "o-")
            # plt.plot(np.log(np.hstack((a[-50:,1], a[:QFTASM_RAMSTDOUT_BUF_STARTPOSITION,1]))+1), "o-")
            plt.savefig("./stackdist.png")

            plt.figure()
            plt.plot(np.log(np.array(rom_reads)+1), "og-")
            plt.savefig("./romdist.png")

        if debug_ramdump_verbose:
            ramvalues = ramarray[:QFTASM_MEMORY_WRAP,0]
            for i, v in enumerate(ramvalues):
                if i > QFTASM_MEMORY_WRAP-QFTASM_NEGATIVE_BUFFER_SIZE-1: #i > 10 and i not in skiplist and v != 0:
                    print("{} : {}".format(i, v))


if __name__ == "__main__":
    filepath = sys.argv[1]
    interpret_file(filepath)
