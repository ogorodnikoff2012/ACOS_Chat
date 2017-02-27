#include <stdio.h>

void print_pstat(FILE *pstat) {
    int pid, ppid, uid, 
}

int main(void) {
    printf("Print info about process with pid: ");
    int pid;
    scanf("%d", &pid);
    char pstat_fname[80];
    snprintf(pstat_fname, 80, "/proc/%d/stat", pid);

    FILE *pstat = fopen(pstat_fname, "r");
    
    print_pstat(pstat);

    fclose(pstat);
    return 0;
}
