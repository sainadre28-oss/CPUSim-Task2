#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <queue>

using namespace std;

// ==========================================
// DATA STRUCTURES
// ==========================================
struct Task
{
    string id;
    int burst_time;
    vector<string> mem_blocks;
};

struct CacheLevel
{
    string name;
    int capacity;
    int latency;
    vector<string> blocks;

    // Returns true if found
    bool contains(string block)
    {
        for (const string &b : blocks)
        {
            if (b == block)
                return true;
        }
        return false;
    }

    // Adds a block (FIFO). Returns the name of the evicted block, or "" if none.
    string add(string block)
    {
        string evicted = "";
        if (blocks.size() >= capacity)
        {
            evicted = blocks.front();     // Get oldest
            blocks.erase(blocks.begin()); // Remove oldest
        }
        blocks.push_back(block);
        return evicted;
    }

    // Formats the blocks for printing: [M1, M2]
    string getFormattedBlocks()
    {
        string s = "[";
        for (size_t i = 0; i < blocks.size(); i++)
        {
            s += blocks[i];
            if (i < blocks.size() - 1)
                s += ", ";
        }
        s += "]";
        return s;
    }
};

// ==========================================
// GLOBAL COUNTERS FOR FINAL RESULTS
// ==========================================
int total_cycles = 0;
int ram_accesses = 0;
int tasks_completed = 0;
int step_counter = 1;

// ==========================================
// THE MEMORY MANAGER (With strict formatting)
// ==========================================
void accessMemory(CacheLevel &L1, CacheLevel &L2, CacheLevel &L3, string block, string task_id)
{
    cout << "Cycle " << step_counter++ << " - Running: " << task_id << " Requesting: " << block << " ";

    if (L1.contains(block))
    {
        cout << "L1: " << L1.getFormattedBlocks() << " -> HIT L2: " << L2.getFormattedBlocks() << " L3: " << L3.getFormattedBlocks() << "\n";
        total_cycles += L1.latency;
    }
    else
    {
        cout << "L1: " << L1.getFormattedBlocks() << " >> MISS ";
        total_cycles += L1.latency;

        if (L2.contains(block))
        {
            cout << "L2: " << L2.getFormattedBlocks() << " -> HIT L3: " << L3.getFormattedBlocks() << "\n";
            total_cycles += L2.latency;
            L1.add(block); // Promote to L1
        }
        else
        {
            cout << "L2: " << L2.getFormattedBlocks() << " >> MISS ";
            total_cycles += L2.latency;

            if (L3.contains(block))
            {
                cout << "L3: " << L3.getFormattedBlocks() << " -> HIT\n";
                total_cycles += L3.latency;
                L1.add(block); // Promote to L1
            }
            else
            {
                // Missed everything. Fetch from RAM.
                cout << "L3: " << L3.getFormattedBlocks() << " >> MISS Fetching from RAM\n";
                total_cycles += L3.latency + 200; // RAM latency is 200
                ram_accesses++;

                // Promote to L1 ONLY (As shown in the example screenshot)
                string evictedL1 = L1.add(block);

                cout << "  L1: " << L1.getFormattedBlocks();
                if (evictedL1 != "")
                    cout << " (" << evictedL1 << " evicted)";
                cout << " L2: " << L2.getFormattedBlocks() << " L3: " << L3.getFormattedBlocks() << "\n";
            }
        }
    }
}

// ==========================================
// TASK LOADER
// ==========================================
queue<Task> loadTasks(string filename)
{
    queue<Task> q;
    ifstream in(filename);
    string line;
    while (getline(in, line))
    {
        if (line.empty())
            continue;
        stringstream ss(line);
        Task t;
        string junk;
        // Parses: TASK T1 BURST 5 MEM M1 M4 M7
        ss >> junk >> t.id >> junk >> t.burst_time >> junk;
        string b;
        while (ss >> b)
            t.mem_blocks.push_back(b);
        q.push(t);
    }
    return q;
}

// ==========================================
// MAIN SIMULATOR
// ==========================================
int main()
{
    queue<Task> ready_queue = loadTasks("../input_task2.txt");

    // Initialize Caches based on project specs
    CacheLevel L1 = {"L1", 32, 4, {}};
    CacheLevel L2 = {"L2", 128, 12, {}};
    CacheLevel L3 = {"L3", 512, 40, {}};

    int quantum = 3;

    cout << "\n--- CPU SIMULATOR STARTED ---\n\n";

    while (!ready_queue.empty())
    {
        Task current = ready_queue.front();
        ready_queue.pop();

        // 1. Process all memory blocks for this task immediately
        while (!current.mem_blocks.empty())
        {
            string block = current.mem_blocks.front();
            current.mem_blocks.erase(current.mem_blocks.begin()); // Remove it so we don't request it again
            accessMemory(L1, L2, L3, block, current.id);
        }

        // 2. Process the CPU Burst Time (Round Robin)
        if (current.burst_time > quantum)
        {
            current.burst_time -= quantum;
            total_cycles += quantum;
            ready_queue.push(current); // Send back to the end of the line
        }
        else
        {
            // Task finishes completely
            total_cycles += current.burst_time;
            tasks_completed++;
        }
    }

    // ==========================================
    // FINAL GRADING OUTPUT
    // ==========================================
    cout << "\n=== Final Results ===\n";
    cout << "Total Cycles: " << total_cycles << "\n";
    cout << "Tasks Completed: " << tasks_completed << "\n";
    cout << "Scheduler: Round Robin (quantum = " << quantum << ")\n";
    cout << "RAM Accesses: " << ram_accesses << "\n\n";

    return 0;
}