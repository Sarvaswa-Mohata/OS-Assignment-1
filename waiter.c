#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <signal.h>

#define BUFFER_SIZE 100
#define SHARED_MEMORY_SIZE 512
#define MANAGER_KEY 4367826
#define TEST 29837

int totalDishes = 0;

struct shared_data
{
    int termination_of_waiter;
    int shared_array[BUFFER_SIZE];
    int billing_amt;
    int valid;
    int total_orders;
};

struct shared_data_hm_waiter
{
    int total_bill_amount;
};

int *createPricesArray(int size)
{
    FILE *file;
    file = fopen("menu.txt", "r");

    if (file == NULL)
    {
        printf("Error opening the file.\n");
        return NULL;
    }

    int *prices = (int *)malloc(size * sizeof(int));
    int count = 0;
    while (count < size && fscanf(file, "%*d. %*[^0-9]%d INR\n", &prices[count]) != EOF)
    {
        count++;
    }

    fclose(file);

    return prices;
}

int read_menu()
{
    FILE *file;
    char line[1000];
    file = fopen("menu.txt", "r");

    if (file == NULL)
    {
        perror("Error opening the file");
        exit(EXIT_FAILURE);
    }

    while (fgets(line, sizeof(line), file))
    {
        totalDishes++;
    }

    fclose(file);
    return totalDishes;
}

int main()
{
    int waiterId;
    // Prompt user for Waiter ID
    printf("Enter Waiter ID: ");
    scanf("%d", &waiterId);

    int menuSize = read_menu();
    int *arrPtr = createPricesArray(menuSize);

    // Create shared memory for total bill amount
    // key_t key = ftok("waiter", waiterId); // key for shared memory
    int shmid = shmget(TEST + waiterId, SHARED_MEMORY_SIZE, 0777);
    if (shmid == -1)
    {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }
    struct shared_data *shared_data = (struct shared_data *)shmat(shmid, NULL, 0);

    // Check if shmat was successful
    if (shared_data == (struct shared_data *)(-1))
    {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }

    // Sending totalbill amount to manager
    int shmid1 = shmget(waiterId + MANAGER_KEY, SHARED_MEMORY_SIZE, 0777 | IPC_CREAT);
    if (shmid1 == -1)
    {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }
    struct shared_data_hm_waiter *shared_data_hm_waiter = (struct shared_data_hm_waiter *)shmat(shmid1, NULL, 0);

    // Check if shmat was successful
    if (shared_data_hm_waiter == (struct shared_data_hm_waiter *)(-1))
    {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }

    while(1){
        //wait till the waiter is called :

        while(shared_data->valid!=2){
            continue;
        }
        if(shared_data->termination_of_waiter==1){
            break;
        }
        int orders_size = shared_data->total_orders;
        
        //validate the orders :
        for (int i = 0; i < orders_size; i++)
        {
            if (shared_data->shared_array[i] > totalDishes)
            {
                shared_data->valid = 0;
                break;
            }
        }
        if(shared_data->valid==0){
            printf("Invalid order recd.\n");
            continue;
        }
        
        // only proceed with below if the order is valid :
        shared_data->billing_amt = 0;
        for (int i = 0; i < orders_size; i++)
        {
            shared_data->billing_amt += *(arrPtr + shared_data->shared_array[i] - 1);
        }

        shared_data_hm_waiter->total_bill_amount += shared_data->billing_amt;
        printf("{");
        for (int i = 0; i < orders_size; i++)
        {
            printf("%d", shared_data->shared_array[i]);
            if (i < orders_size - 1)
            {
                printf(", ");
            }
        }
        printf("}\n");
        shared_data->valid=1;
    }

    // Detach shared memory
    if (shmdt(shared_data) == -1)
    {
        perror("shmdt failed");
        exit(EXIT_FAILURE);
    }

    // Cleanup the shared memory segment change : 1
    if (shmctl(shmid1, IPC_RMID, NULL) == -1) {
        perror("shmctl failed");
        exit(EXIT_FAILURE);
    }

    return 0;
}