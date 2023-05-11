#include "codec.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include<pthread.h>
#include<unistd.h>


typedef struct _node {
    int key;
    char flag[2];
    char* data;
    int curr_data_frame;
    struct _node* next;
} node, *node_ptr;

typedef struct _queue {
    node_ptr front;
    node_ptr rear;
} queue, *queue_ptr;

//global varibels
pthread_mutex_t Lock_q;
pthread_cond_t CV_q;
pthread_mutex_t Lock_stdout;
pthread_cond_t CV_stdout;
pthread_mutex_t Lock_stop;
pthread_cond_t CV_stop;
queue_ptr q;
int number_of_data_frames=0;
int numuber_of_last_data_frame_writen=0;
int main_finish =0;
int second_finish =0;


void enqueue(int key, char flag[2], char* data, int curr_data_frame) {
    node_ptr new_node = (node_ptr)malloc(sizeof(node));
    new_node->key = key;
    strcpy(new_node->flag, flag);
    new_node->data = strdup(data);
    new_node->curr_data_frame = curr_data_frame;
    new_node->next = NULL;
    if (q->front == NULL) {
        q->front = new_node;
    } else {
        q->rear->next = new_node;
    }
    q->rear = new_node;
}

void dequeue(int* key, char flag[2], char** data, int* curr_data_frame) {
    if (q->front == NULL) {
        return;
    }
    node_ptr temp = q->front;
    *key = temp->key;
    strcpy(flag, temp->flag);
    *data = strdup(temp->data);
    *curr_data_frame = temp->curr_data_frame;
    q->front = q->front->next;
    if (q->front == NULL) {
        q->rear = NULL;
    }
    free(temp->data);
    free(temp);
}


void* thread_function(void *arg)
{
    while(1){
        char* data;
        int curr_frame_number;
        int key;
        char fun[2];

        // get the next part to work on
        pthread_mutex_lock(&Lock_q);

        while (q->front == NULL) {
            pthread_cond_wait(&CV_q, &Lock_q);
        }

        dequeue(&key, fun, &data, &curr_frame_number);

        pthread_cond_signal(&CV_q);

        pthread_mutex_unlock(&Lock_q);

        // do the encrypt and decrypt
        if (strcmp(fun, "-e") == 0)
        {
            encrypt(data,key);
        }
        else
        {
            decrypt(data,key);

        }
        pthread_mutex_lock(&Lock_stdout);

        while (curr_frame_number - 1 != numuber_of_last_data_frame_writen ) {
            pthread_cond_signal(&CV_stdout);
            pthread_cond_wait(&CV_stdout, &Lock_stdout);
        }
        printf("%s",data);
        numuber_of_last_data_frame_writen++;
        if(main_finish == 1)
        {

            if(numuber_of_last_data_frame_writen == number_of_data_frames)
            {
                second_finish =1;
                pthread_cond_signal(&CV_stop);
            }
        }
        pthread_cond_signal(&CV_stdout);
        pthread_mutex_unlock(&Lock_stdout);
    }

}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("usage: key < file \n");
        printf("!! data more than 1024 char will be ignored !!\n");
        return 0;
    }

//    printf("start2\n");
    q = (queue_ptr) (malloc(sizeof(queue)));
    q->front = NULL;
    q->rear = NULL;



    int key = atoi(argv[1]);

    char fun[2];
    strcpy(fun,argv[2]);

    if ((strcmp(fun, "-e") != 0) && (strcmp(fun, "-d") != 0))
    {
        printf("bad function/flag. \n");
        return 0;
    }
    int numCPU = sysconf(_SC_NPROCESSORS_ONLN);

    // init all the cond and mutex

    if (pthread_mutex_init(&Lock_q, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }

    if (pthread_mutex_init(&Lock_stdout, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }

    if (pthread_mutex_init(&Lock_stop, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }

    if (pthread_cond_init(&CV_q, NULL) != 0)
    {
        printf("\n cond init failed\n");
        return 1;
    }

    if (pthread_cond_init(&CV_stdout, NULL) != 0)
    {
        printf("\n cond init failed\n");
        return 1;
    }

    if (pthread_cond_init(&CV_stop, NULL) != 0)
    {
        printf("\n cond init failed\n");
        return 1;
    }

//    start threads
    pthread_t tid[numCPU];
    int numbers[4]={0,1,2,3};
    for(int i=0;i<numCPU;i++)
    {
        pthread_create(&(tid[i]), NULL, &thread_function, NULL);
    }


    char c;
    int counter = 0;
    int dest_size = 1024;
    char data[dest_size+1];

    while ((c = getchar()) != EOF)
    {
        data[counter] = c;
        counter++;
        if (counter == dest_size){
            data[dest_size] = '\0';
            number_of_data_frames++;
            pthread_mutex_lock(&Lock_q);

            if(q->front == NULL)
            {

                enqueue(key, fun, data, number_of_data_frames);
                pthread_cond_signal(&CV_q);
            }
            else
            {
                enqueue(key, fun, data, number_of_data_frames);
                pthread_cond_signal(&CV_q);

            }
            pthread_mutex_unlock(&Lock_q);
            counter=0;
        }
    }

    if (counter > 0)
    {
        data[counter] = '\0';
        number_of_data_frames++;
        pthread_mutex_lock(&Lock_q);
        if(q->front == NULL)
        {
            enqueue(key, fun, data, number_of_data_frames);
            pthread_cond_signal(&CV_q);
        }
        else
        {
            enqueue(key, fun, data, number_of_data_frames);
            pthread_cond_signal(&CV_q);

        }
        pthread_mutex_unlock(&Lock_q);
        counter=0;
    }

    main_finish = 1;


    pthread_mutex_lock(&Lock_stop);
    while (second_finish == 0) {
        pthread_cond_wait(&CV_stop, &Lock_stop);
    }
    pthread_mutex_unlock(&Lock_stop);
    for (int i = 0; i < numCPU; i++) {
        pthread_cancel(tid[i]);
    }
    // destroy all the cond and mutex

    pthread_mutex_destroy(&Lock_q);
    pthread_mutex_destroy(&Lock_stdout);
    pthread_mutex_destroy(&Lock_stop);
    pthread_cond_destroy(&CV_q);
    pthread_cond_destroy(&CV_stdout);
    pthread_cond_destroy(&CV_stop);


    return 0;
}
