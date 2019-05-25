
Conversation opened. 1 unread message.


Skip to content
Using Gmail with screen readers
Enable desktop notifications for Gmail.
   OK  No thanks

1 of 4,254
project

Jose Sandoval
Attachments
7:59 PM (0 minutes ago)
to me


Attachments area

import queue


class Process:
    def __init__(self, process_id, action, page):
        self.process_id = process_id
        self.action = action
        self.page = page
        self.virtual = []


def readfile(file):
    memory_file = open(file, "r")
    all_processes = []
    line = memory_file.readline()
    while line:
        line = line.split()
        all_processes.append(Process(int(line[0]), str(line[1]), int(line[2])))
        line = memory_file.readline()

    # Sotre al the processes
    return all_processes


def print_address(process):
    print("PROCESS ID: ", end=" ")
    print(process.process_id)
    for i in range(0, len(process.virtual)):
        print("Virtual: ", end=" ")
        print(process.virtual[i], end="\t")
        print("Physical: ", end=" ")
        print(physical_memory[i])
        print("\n")

if __name__ == "__main__":
    processes = readfile('memory.dat')
    physical_memory = [None] * 20
    mem = queue.Queue(maxsize=20)
    virtual_index = 0
    created_processes = []

    for process in processes:
        if process.action == 'C':
            created_processes.append(process)
    for running_process in created_processes:
        for process in processes:
            if running_process.process_id == process.process_id:
                if process.action == 'A':
                    running_process.virtual.append(process.page)
                    physical_memory[virtual_index] = process.page
                    if not mem.full():
                        mem.put(process.process_id)
                    else:
                        mem.get()
                        mem.put(process.process_id)
                    virtual_index += 1
                if process.action == 'T':
                    pass
                if process.action == 'R':
                    pass
                if process.action == 'W':
                    pass
                if process.action == 'F':
                    pass

    for i in range(0, len(created_processes)):
        print_address(created_processes[i])


memoryproject.py
Displaying memoryproject.py.