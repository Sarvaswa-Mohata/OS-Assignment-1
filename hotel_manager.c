#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include<errno.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define BUF_SIZE 100
#define EARNINGS_FILE "earnings.txt"
#define HOTEL_KEY 38748
#define SHARED_MEMORY_SIZE 512
#define MANAGER_KEY 4367826

struct shared_data_hotel_manager
{
    int termination_of_table;
};

struct shared_data_hm_waiter
{
    int total_bill_amt;
};

void write_earnings(int table_number, int total_earnings)
{
    FILE *fp = fopen(EARNINGS_FILE, "a");
    if (fp == NULL)
    {
        perror("Error opening earnings file");
        exit(EXIT_FAILURE);
    }

    fprintf(fp, "Earning from Table %d: %d INR\n", table_number, total_earnings);
    fclose(fp);
}

void calculate_and_display_totals(int total_earnings)
{
    FILE *fp = fopen(EARNINGS_FILE, "a");
    if (fp == NULL)
    {
        perror("Error opening earnings file");
        exit(EXIT_FAILURE);
    }

    float total_wage = total_earnings * 0.4;
    float total_profit = total_earnings * 0.6;

    printf("Total Earnings of Hotel: %d INR\n", total_earnings);
    fprintf(fp, "Total Earnings of Hotel: %d INR\n", total_earnings);

    printf("Total Wage of Waiters: %.2f INR\n", total_earnings * 0.4);
    fprintf(fp, "Total Wage of Waiters: %.2f INR\n", total_wage);

    printf("Total Profit: %.2f INR\n", total_profit);
    fprintf(fp, "Total Profit: %.2f INR\n", total_earnings * 0.6);

    fclose(fp);
}

void create_file(){
    FILE *file;
    char filename[] = EARNINGS_FILE;

    // Open the file in write mode (clears existing contents)
    file = fopen(filename, "w");

    // Check if the file was opened successfully
    if (file == NULL) {
        perror("Error opening earnings file");
        exit(EXIT_FAILURE);
    }

    // Close the file
    fclose(file);
}

int main()
{
    int table_count = 0;

    printf("Enter the Total Number of Tables at the Hotel: ");
    scanf("%d", &table_count);

    //create the 'earnings.txt' file:
    create_file();

    printf("hotel manager is reading...\n");
    int total_earnings = 0;

    int termination_count = table_count;

    while(termination_count!=0){
        for(int i=1;i<=10;i++){
            int waiterID = i;
            int shmid1 = shmget(HOTEL_KEY+waiterID, SHARED_MEMORY_SIZE, 0777);
            if(shmid1 == -1){
                continue;
            }

            int shmid2 = shmget(MANAGER_KEY+waiterID, SHARED_MEMORY_SIZE, 0777);
            if(shmid2 == -1){
                continue;
            }

            struct shared_data_hotel_manager *shared_data_hotel_manager = shmat(shmid1, NULL, 0);
            if (shared_data_hotel_manager == (struct shared_data_hotel_manager *)(-1))
            {
                perror("connection to table failed");
                exit(EXIT_FAILURE);
            }

            
            struct shared_data_hm_waiter *shared_data_hm_waiter = shmat(shmid2, NULL, 0);
            if (shared_data_hm_waiter == (struct shared_data_hm_waiter *)(-1))
            {
                perror("connection to waiter failed");
                exit(EXIT_FAILURE);
            }

            //wait for the table to first terminate before calculating earnings :
            while(shared_data_hotel_manager->termination_of_table!=1){
                continue;
            }
            //this table hasn't been encountered previously :
            int earnings = shared_data_hm_waiter->total_bill_amt;
            total_earnings+=earnings;

            //write to the earnings file :
            write_earnings(i,earnings);

            //decrement the termination count because calculation of earnings for 1 table process is over :
            termination_count--;

            if (shmdt(shared_data_hotel_manager) == -1)
            {
                perror("shmdt for table process failed");
                exit(EXIT_FAILURE);
            }

            if (shmdt(shared_data_hm_waiter) == -1)
            {
                perror("shmdt for waiter process failed");
                exit(EXIT_FAILURE);
            }

            if (shmctl(shmid1, IPC_RMID, NULL) == -1) {
                printf("shmctl returned -1. errno = %d\n", errno);
                perror("shmctl for table process failed");
                exit(EXIT_FAILURE);
            }

            // if (shmctl(shmid2, IPC_RMID, NULL) == -1) {
            //     printf("shmctl returned -1. errno = %d\n", errno);
            //     perror("shmctl for waiter process failed");
            //     exit(EXIT_FAILURE);
            // }
        }
    }
    
    calculate_and_display_totals(total_earnings);

    key_t key;
    if ((key = ftok("hotel_manager.c", 'A')) == -1)
    {
        perror("Error in ftok\n");
        return 1;
    }

    int shmid3;   // shared memory segment identifier
    int *shmPtr2; // Pointer to the shared memory segment
    shmid3 = shmget(key, BUF_SIZE, 0644 | IPC_CREAT);
    if (shmid3 == -1)
    {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }

    shmPtr2 = shmat(shmid3, NULL, 0);
    printf("Waiting for confirmation from admin\n");
    //this will wait till the admin does not command it to close :
    while(shmPtr2[0]!=1){
        continue;
    }

    printf("Thank you for visiting the Hotel!\n");
    if (shmdt(shmPtr2) == -1)
    {
        perror("Error in shmdt in detaching the memory segment\n");
        return 1;
    }
    if (shmctl(shmid3, IPC_RMID, 0) == -1) // don't delete before reading is done
    {
        perror("Error in shmctl\n");
        return 1;
    }
    return 0;
}