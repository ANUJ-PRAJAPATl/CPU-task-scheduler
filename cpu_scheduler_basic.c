#include <stdio.h>
#include <stdlib.h>
struct Process {
    int pid, arrival, burst, priority;
    int remaining, completion, waiting, turnaround;
    struct Process *next;
};
struct Process *head = NULL;
struct Process *tail = NULL;
int pcount = 0;
/*Circular Queue*/
struct Process *cq[100];
int cfront = 0, crear = -1, csize = 0;
/*Priority Queue (min-heap, index starts from 1)*/
struct Process *heap[101];
int hsize = 0;
/*Gantt chart*/
int segPid[1000], segStart[1000], segEnd[1000];
int nseg = 0;
/*LINKED LIST FUNCTIONS*/
int pidExists(int pid){
    struct Process *p;
    for (p = head; p != NULL; p = p->next)
        if (p->pid == pid)
            return 1;
    return 0;
}
int addProcess(int pid, int arrival, int burst, int priority){
    struct Process *p;
    if (pcount >= 100) {
        printf("Process limit reached.\n");
        return 0;
    }
    if (pidExists(pid)) {
        printf("PID %d already exists.\n", pid);
        return 0;
    }
    p = (struct Process *)malloc(sizeof(struct Process));
    if (p == NULL) {
        printf("Memory not available.\n");
        return 0;
    }
    p->pid = pid;
    p->arrival = arrival;
    p->burst = burst;
    p->priority = priority;
    p->remaining = burst;
    p->completion = 0;
    p->waiting = 0;
    p->turnaround = 0;
    p->next = NULL;
    if (head == NULL) {
        head = p;
        tail = p;
    } else {
        tail->next = p;
        tail = p;
    }
    pcount++;
    return 1;
}
void clearProcesses(){
    struct Process *t;
    while (head != NULL) {
        t = head;
        head = head->next;
        free(t);
    }
    tail = NULL;
    pcount = 0;
}
void displayProcesses(){
    struct Process *p;
    if (head == NULL) {
        printf("No processes added.\n");
        return;
    }
    printf("\nPID\tArrival\tBurst\tPriority\n");
    printf("--------------------------------\n");
    for (p = head; p != NULL; p = p->next)
        printf("%d\t%d\t%d\t%d\n", p->pid, p->arrival, p->burst, p->priority);
}
/*CIRCULAR QUEUE FUNCTIONS*/
void cqEnqueue(struct Process *p){
    if (csize == 100)
        return;
    crear = (crear + 1) % 100;
    cq[crear] = p;
    csize++;
}
struct Process *cqDequeue(){
    struct Process *p;
    if (csize == 0)
        return NULL;
    p = cq[cfront];
    cfront = (cfront + 1) % 100;
    csize--;
    return p;
}
/*PRIORITY QUEUE (HEAP) FUNCTIONS*/
int better(struct Process *a, struct Process *b){
    if (a->priority != b->priority)
        return a->priority < b->priority;
    if (a->arrival != b->arrival)
        return a->arrival < b->arrival;
    return a->pid < b->pid;
}
void heapPush(struct Process *p){
    int i;
    struct Process *temp;
    if (hsize >= 100)
        return;
    hsize++;
    i = hsize;
    heap[i] = p;
    while (i > 1 && better(heap[i], heap[i / 2])) {
        temp = heap[i];
        heap[i] = heap[i / 2];
        heap[i / 2] = temp;
        i = i / 2;
    }
}
struct Process *heapPop(){
    struct Process *top, *temp;
    int i, l, r, small;
    if (hsize == 0)
        return NULL;
    top = heap[1];
    heap[1] = heap[hsize];
    hsize--;
    i = 1;
    while (1) {
        l = 2 * i;
        r = 2 * i + 1;
        small = i;
        if (l <= hsize && better(heap[l], heap[small]))
            small = l;
        if (r <= hsize && better(heap[r], heap[small]))
            small = r;
        if (small == i)
            break;
        temp = heap[i];
        heap[i] = heap[small];
        heap[small] = temp;
        i = small;
    }
    return top;
}
/*GANTT CHART*/
void addSeg(int pid, int start, int end){
    if (nseg > 0 && segPid[nseg - 1] == pid && segEnd[nseg - 1] == start) {
        segEnd[nseg - 1] = end;
        return;
    }
    if (nseg < 1000) {
        segPid[nseg] = pid;
        segStart[nseg] = start;
        segEnd[nseg] = end;
        nseg++;
    }
}
void printGantt(){
    int i;
    printf("\nExecution order: ");
    for (i = 0; i < nseg; i++) {
        if (segPid[i] != -1)
            printf("P%d ", segPid[i]);
    }
    printf("\n\nGantt Chart:\n");
    for (i = 0; i < nseg; i++) {
        if (segPid[i] == -1)
            printf("| IDLE (%d-%d) ", segStart[i], segEnd[i]);
        else
            printf("| P%d (%d-%d) ", segPid[i], segStart[i], segEnd[i]);
    }
    printf("|\n");
}
/*COMMON HELPERS*/
int prepare(struct Process *arr[]){
    struct Process *p, *temp;
    int n = 0, i, j;
    for (p = head; p != NULL; p = p->next) {
        p->remaining = p->burst;
        p->completion = 0;
        p->waiting = 0;
        p->turnaround = 0;
        arr[n] = p;
        n++;
    }
    for (i = 0; i < n - 1; i++) {
        for (j = 0; j < n - 1 - i; j++) {
            if (arr[j]->arrival > arr[j + 1]->arrival ||
               (arr[j]->arrival == arr[j + 1]->arrival && arr[j]->pid > arr[j + 1]->pid)) {
                temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
    nseg = 0;
    return n;
}
void report(struct Process *arr[], int n){
    int i;
    float totalW = 0, totalT = 0;
    printGantt();
    printf("\nPID\tArrival\tBurst\tPriority\tCompletion\tTurnaround\tWaiting\n");
    printf("-----------------------------------------------------------------------\n");
    for (i = 0; i < n; i++) {
        arr[i]->turnaround = arr[i]->completion - arr[i]->arrival;
        arr[i]->waiting = arr[i]->turnaround - arr[i]->burst;
        totalW = totalW + arr[i]->waiting;
        totalT = totalT + arr[i]->turnaround;
        printf("%d\t%d\t%d\t%d\t\t%d\t\t%d\t\t%d\n",
               arr[i]->pid, arr[i]->arrival, arr[i]->burst, arr[i]->priority,
               arr[i]->completion, arr[i]->turnaround, arr[i]->waiting);
    }
    printf("\nAverage Waiting Time    = %.2f\n", totalW / n);
    printf("Average Turnaround Time = %.2f\n", totalT / n);
}
/*ROUND ROBIN*/
void roundRobin(int quantum){
    struct Process *arr[100];
    struct Process *p;
    int n, time = 0, idx = 0, done = 0, run, next;
    n = prepare(arr);
    if (n == 0) {
        printf("No processes to schedule.\n");
        return;
    }
    cfront = 0;
    crear = -1;
    csize = 0;
    while (done < n) {
        while (idx < n && arr[idx]->arrival <= time) {
            cqEnqueue(arr[idx]);
            idx++;
        }
        if (csize == 0) {
            next = arr[idx]->arrival;
            addSeg(-1, time, next);
            time = next;
            continue;
        }
        p = cqDequeue();
        if (p->remaining < quantum)
            run = p->remaining;
        else
            run = quantum;
        addSeg(p->pid, time, time + run);
        time = time + run;
        p->remaining = p->remaining - run;
        while (idx < n && arr[idx]->arrival <= time) {
            cqEnqueue(arr[idx]);
            idx++;
        }
        if (p->remaining > 0) {
            cqEnqueue(p);
        } 
        else {
            p->completion = time;
            done++;
        }
    }
    printf("\n===== ROUND ROBIN (Quantum = %d) =====\n", quantum);
    report(arr, n);
}
/*PRIORITY SCHEDULING*/
/* preemptive = 0 : non-preemptive, preemptive = 1 : preemptive */
void priorityScheduling(int preemptive){
    struct Process *arr[100];
    struct Process *p;
    int n, time = 0, idx = 0, done = 0, run, next;
    n = prepare(arr);
    if (n == 0) {
        printf("No processes to schedule.\n");
        return;
    }
    hsize = 0;
    while (done < n) {
        while (idx < n && arr[idx]->arrival <= time) {
            heapPush(arr[idx]);
            idx++;
        }
        if (hsize == 0) {
            next = arr[idx]->arrival;
            addSeg(-1, time, next);
            time = next;
            continue;
        }
        p = heapPop();
        if (preemptive == 1)
            run = 1;                
        else
            run = p->remaining;    
        addSeg(p->pid, time, time + run);
        time = time + run;
        p->remaining = p->remaining - run;
        while (idx < n && arr[idx]->arrival <= time) {
            heapPush(arr[idx]);
            idx++;
        }
        if (p->remaining > 0) {
            heapPush(p);
        } else {
            p->completion = time;
            done++;
        }
    }
    if (preemptive == 1)
        printf("\n===== PRIORITY SCHEDULING (Preemptive) =====\n");
    else
        printf("\n===== PRIORITY SCHEDULING (Non-Preemptive) =====\n");
    report(arr, n);
}
/*INPUT+MENU*/
int readInt(char msg[], int min){
    int v, c;
    while (1) {
        printf("%s", msg);
        if (scanf("%d", &v) == 1 && v >= min)
            return v;
        printf("Please enter a number >= %d\n", min);
        c = getchar();
        while (c != '\n' && c != EOF)
            c = getchar();
        if (c == EOF)
            exit(0);
    }
}
void loadSample(){
    clearProcesses();
    addProcess(1, 0, 5, 2);
    addProcess(2, 1, 3, 1);
    addProcess(3, 2, 8, 4);
    addProcess(4, 3, 6, 3);
    printf("Sample data loaded (4 processes).\n");
}

int main()
{
    int choice, pid, at, bt, pr, q;

    while (1) {
        printf("\n===== CPU TASK SCHEDULER =====\n");
        printf("1. Add process\n");
        printf("2. Display processes\n");
        printf("3. Load sample data\n");
        printf("4. Round Robin\n");
        printf("5. Priority (Non-Preemptive)\n");
        printf("6. Priority (Preemptive)\n");
        printf("7. Clear all processes\n");
        printf("0. Exit\n");
        choice = readInt("Enter choice: ", 0);

        if (choice == 1) {
            pid = readInt("PID: ", 1);
            at = readInt("Arrival time: ", 0);
            bt = readInt("Burst time: ", 1);
            pr = readInt("Priority (lower number = higher priority): ", 0);
            if (addProcess(pid, at, bt, pr))
                printf("Process added.\n");
        } else if (choice == 2) {
            displayProcesses();
        } else if (choice == 3) {
            loadSample();
        } else if (choice == 4) {
            q = readInt("Time quantum: ", 1);
            roundRobin(q);
        } else if (choice == 5) {
            priorityScheduling(0);
        } else if (choice == 6) {
            priorityScheduling(1);
        } else if (choice == 7) {
            clearProcesses();
            printf("All processes cleared.\n");
        } else if (choice == 0) {
            clearProcesses();
            return 0;
        } else {
            printf("Invalid choice.\n");
        }
    }
}
