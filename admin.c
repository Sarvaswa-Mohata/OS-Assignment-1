#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <signal.h>

#define BUFFER_SIZE 100

int main()
{

    key_t key;
    if ((key = ftok("hotel_manager.c", 'A')) == -1)
    {
        perror("Error in ftok\n");
        return 1;
    }
    int shmid;   // shared memory segment identifier
    int *shmPtr; // Pointer to the shared memory segment
    shmid = shmget(key, BUFFER_SIZE, 0644);

    if (shmid == -1)
    {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }

    shmPtr = shmat(shmid, NULL, 0);
    int close_hotel = 0;
    char closing_signal;

    while (1)
    {
        /* code */
        printf("Do you want to close the hotel? Enter Y for Yes and N for No.");
        scanf(" %c", &closing_signal);

        if (closing_signal == 'Y' || closing_signal == 'y')
        {
            // Inform hotel manager to close the hotel
            shmPtr[0] = 1;
            if (shmdt(shmPtr) == -1)
            {
                perror("Error in shmdt in detaching the memory segment\n");
                return 1;
            }
            break;
        }
        else if(closing_signal =='N' || closing_signal == 'n')
        {
            continue;
        }
        else{
            printf("Invalid input. Please enter Y or N.\n");
        }
    }

}