#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#define DEBUG

// #define START 1
// #define IDLE 0
//the stage of each dir
typedef enum
{
    IDLE,
    START
} dirStage;
dirStage nn = IDLE, ns = IDLE, ne = IDLE, nw = IDLE;
// const int QMaxSize = 10; //max size of queue
#define QMaxSize 10
//queue struct
typedef struct
{
    pthread_t carThread[QMaxSize];
    int data[QMaxSize];
    int front; //front
    int rear;  //back
} Q;
//initial Q
int initQ(Q *q)
{
    q->front = 0;
    q->rear = 0;
}
//check full
int isFull(Q q)
{
    return q.front == q.rear + 1;
}
//check empty
int isEmpty(Q q)
{
    return q.front == q.rear;
}
//push element
int push(Q *q, int e)
{
    if (!isFull(*q)) //check full
    {
        q->rear++;
        q->rear %= QMaxSize;
        q->data[q->rear] = e;
        return 1;
    }
    return 0;
}
//poll element
int poll(Q *q)
{
    if (!isEmpty(*q)) //check empty
    {
        int temp = q->data[q->front++];
        q->front %= QMaxSize;
        return temp;
    }
    return -1;
}
//peek element
int peek(Q q)
{
    if (!isEmpty(q)) //check empty
    {
        return q.data[q.front];
    }
    return -1;
}

//log relevent
FILE *carLog;
int toLog(char *s)
{
    int res = fprintf(carLog, s);
    fflush(carLog);
    return res;
}

//global var
Q N, S, E, W; //queue of each dir

pthread_mutex_t a, b, c, d;                 //the source of the road
pthread_mutex_t waitN, waitS, waitE, waitW; //the waiting line of the road
pthread_mutex_t N2, E2, S2, W2;             //the mutex of conditon var
pthread_cond_t N2E, E2S, S2W, W2N;          //the condition mutex to wait right
// int sN, sS, sW, sE;                         //signal of each dir
sem_t empty;       //the deadlock signal
pthread_t car[20]; //the thread id of cars
int carSize = 0;

//cars from each dir
#define DRIVING_SEC 2
void *carFromS(void *arg)
{
    int id = *(int *)(arg); //the id of the car
    int dir = 0;            //the direction of the car
    char logBuffer[100];
    pthread_mutex_lock(&waitS); //lock follow car//is it necessary
    sprintf(logBuffer, "Car %d from %d:\t lock the wait mutex\n", id, dir);
    toLog(logBuffer);

    ns = START;//not nessary to lock S2
    pthread_mutex_lock(&a);
    sprintf(logBuffer, "Car %d from %d:\t lock the a mutex\n", id, dir);
    toLog(logBuffer);

    //print console info
    printf("car %d from South arrives crossing\n", id);
    sprintf(logBuffer, "car %d from South arrives crossing\n", id);
    toLog(logBuffer);

    sem_wait(&empty); //empty--

    //if the east dir has cars
    pthread_mutex_lock(&E2);
    if (ne == START)
    {
        pthread_cond_wait(&E2S, &E2); //wait east car go first

        sprintf(logBuffer, "Car %d from %d:\t wait the E2S mutex\n", id, dir);
        toLog(logBuffer);
    }
    pthread_mutex_unlock(&E2);

    //driving
    sleep(DRIVING_SEC);

    //the 2nd cross
    pthread_mutex_lock(&b);
    sprintf(logBuffer, "Car %d from %d:\t lock the b mutex\n", id, dir);
    toLog(logBuffer);

    //release source
    sem_post(&empty);
    pthread_mutex_unlock(&a);
    pthread_mutex_unlock(&b);
    pthread_mutex_unlock(&waitS); //next follow car
    printf("car %d from South leaving crossing\n", id);
    sprintf(logBuffer, "car %d from South leaving crossing\n", id);
    toLog(logBuffer);

    //signal west to go
    pthread_cond_signal(&S2W);
    sprintf(logBuffer, "Car %d from %d:\t release the S2W mutex\n", id, dir);
    toLog(logBuffer);

    //add next car
    poll(&S);
    if (!isEmpty(S))
    {
        pthread_create(&S.carThread[S.front], NULL, carFromS, &S.front);
        car[carSize++] = S.carThread[S.front];
    }
    else
    {
        pthread_mutex_lock(&S2);
        pthread_cond_signal(&S2W); //last signal
        ns = IDLE;
        pthread_mutex_unlock(&S2);
        sprintf(logBuffer, "Car %d from %d:\t release the S2W mutex\n", id, dir);
        toLog(logBuffer);
    }

    return 0;
}
// void *carFromN(void *arg)
// {
//     int id = *(int *)(arg);
//     int sem;
//     pthread_mutex_lock(&waitN); //lock follow car
//     sN = 1;
//     pthread_mutex_lock(&c);
//     sem_wait(&empty);
//     if (sW)
//     {
//         sem_getvalue(&empty, &sem);
//         if (sem == 0)
//         {
//             printf("DEADLOCK: car jam detexted, signalling North to go\n");
//             sem_post(&empty);
//             pthread_mutex_unlock(&c);
//             sN = 0;
//             pthread_mutex_unlock(&waitN);
//             return 0;
//         }

//         pthread_cond_wait(&W2N, &waitN); //wait west car go first
//     }

//     printf("car %d from North arrives crossing\n", id);
//     pthread_mutex_lock(&d);
//     sleep(1); //to cause deadlock
//     pthread_mutex_unlock(&d);
//     sem_post(&empty);
//     pthread_mutex_unlock(&c);

//     printf("car %d from North leaving crossing\n", id);
//     pthread_cond_signal(&N2E);
//     sN = 0;
//     pthread_mutex_unlock(&waitN); //next follow car
//     // printf("hello");
//     return 0;
// }
// void *carFromE(void *arg)
// {
//     int id = *(int *)(arg);
//     pthread_mutex_lock(&waitE); //lock follow car
//     sE = 1;
//     pthread_mutex_lock(&b);
//     sem_wait(&empty);
//     if (sN)
//     {
//         pthread_cond_wait(&N2E, &waitE); //wait north car go first
//     }

//     printf("car %d from East arrives crossing\n", id);
//     pthread_mutex_lock(&c);
//     sleep(1); //to cause deadlock
//     pthread_mutex_unlock(&c);
//     sem_post(&empty);
//     pthread_mutex_unlock(&b);

//     printf("car %d from East leaving crossing\n", id);
//     pthread_cond_signal(&E2S);
//     sE = 0;
//     pthread_mutex_unlock(&waitE); //next follow car
//     // printf("hello");
//     return 0;
// }
void *carFromW(void *arg)
{
    int id = *(int *)(arg); //the id of the car
    char dir[20] = "West";  //the direction of the car
    char logBuffer[100];
    pthread_mutex_lock(&waitW); //lock follow car
    sprintf(logBuffer, "Car %d from %s:\t lock the wait mutex\n", id, dir);
    toLog(logBuffer);

    ns = START; //not nessary to lock S2
    pthread_mutex_lock(&a);
    sprintf(logBuffer, "Car %d from %s:\t lock the a mutex\n", id, dir);
    toLog(logBuffer);

    //print console info
    printf("car %d from South arrives crossing\n", id);
    sprintf(logBuffer, "car %d from South arrives crossing\n", id);
    toLog(logBuffer);

    sem_wait(&empty); //empty--

    //if the east dir has cars
    pthread_mutex_lock(&E2);
    if (ne == START)
    {
        pthread_cond_wait(&E2S, &E2); //wait east car go first

        sprintf(logBuffer, "Car %d from %d:\t wait the E2S mutex\n", id, dir);
        toLog(logBuffer);
    }
    pthread_mutex_unlock(&E2);

    //driving
    sleep(DRIVING_SEC);

    //the 2nd cross
    pthread_mutex_lock(&b);
    sprintf(logBuffer, "Car %d from %d:\t lock the b mutex\n", id, dir);
    toLog(logBuffer);

    //release source
    sem_post(&empty);
    pthread_mutex_unlock(&a);
    pthread_mutex_unlock(&b);
    pthread_mutex_unlock(&waitS); //next follow car
    printf("car %d from South leaving crossing\n", id);
    sprintf(logBuffer, "car %d from South leaving crossing\n", id);
    toLog(logBuffer);

    //signal west to go
    pthread_cond_signal(&S2W);
    sprintf(logBuffer, "Car %d from %d:\t release the S2W mutex\n", id, dir);
    toLog(logBuffer);

    //add next car
    poll(&S);
    if (!isEmpty(S))
    {
        pthread_create(&S.carThread[S.front], NULL, carFromS, &S.front);
        car[carSize++] = S.carThread[S.front];
    }
    else
    {
        pthread_mutex_lock(&S2);
        pthread_cond_signal(&S2W); //last signal
        ns = IDLE;
        pthread_mutex_unlock(&S2);
        sprintf(logBuffer, "Car %d from %d:\t release the S2W mutex\n", id, dir);
        toLog(logBuffer);
    }

    return 0;
}
// void *carFromW(void *arg)
// {
//     int id = *(int *)(arg);
//     pthread_mutex_lock(&waitW); //lock follow car
//     sW = 1;
//     pthread_mutex_lock(&d);
//     sem_wait(&empty);
//     if (sS)
//     {
//         pthread_cond_wait(&S2W, &waitW); //wait west car go first
//     }
//     printf("car %d from West arrives crossing\n", id);
//     pthread_mutex_lock(&a);
//     sleep(1); //to cause deadlock
//     pthread_mutex_unlock(&a);
//     sem_post(&empty);
//     pthread_mutex_unlock(&d);

//     printf("car %d from West leaving crossing\n", id);
//     pthread_cond_signal(&W2N);
//     sW = 0;
//     pthread_mutex_unlock(&waitW); //next follow car
//     // printf("hello");
//     return 0;
// }

//main function
int main(void)
{
    //init mutex of road source
    pthread_mutex_init(&a, NULL);
    pthread_mutex_init(&b, NULL);
    pthread_mutex_init(&c, NULL);
    pthread_mutex_init(&d, NULL);

    //init mutex of waiting source
    pthread_mutex_init(&waitS, NULL);
    pthread_mutex_init(&waitW, NULL);
    pthread_mutex_init(&waitN, NULL);
    pthread_mutex_init(&waitE, NULL);

    //init confition mutex
    pthread_cond_init(&N2E, NULL);
    pthread_cond_init(&E2S, NULL);
    pthread_cond_init(&S2W, NULL);
    pthread_cond_init(&W2N, NULL);

    //init signal
    // sS = 0;
    // sN = 0;
    // sW = 0;
    // sE = 0;

    //semaphore init
    sem_init(&empty, 0, 4);

    //init log file
    carLog = fopen("./log", "w+");
    if (carLog == NULL)
    {
        printf("log file error\n");
    }
    fprintf(carLog, "log starts\n");
    fflush(carLog); //flush buffer
    // start input data
    char data[20];

    freopen("in", "r", stdin);
    int n;
    scanf("%d", &n);
    getchar(); //delete blank
    char c;
    int i;

    for (i = 0; i < n; i++)
    {
        scanf("%c", &(data[i]));
        c = data[i];
        switch (c) //check which dir
        {
        // case 'n':
        //     push(N, i);
        //     nn = START;

        //     pthread_create(&car[N.front], NULL, carFromN, &N.front); //north
        //     break;
        case 's':
            push(&S, i);
            if (ns == IDLE)
            {
                // ns = START;
                pthread_create(&S.carThread[S.front], NULL, carFromS, &S.front); //south
                car[carSize++] = S.carThread[S.front];
            }
            break;
        // case 'e':
        //     push(E, i);
        //     ne = START;
        //     pthread_create(&car[E.front], NULL, carFromE, &E.front); //east
        //     break;
        case 'w':
            push(&W, i);
            if (nw == IDLE)
            {
                // ns = START;
                pthread_create(&W.carThread[W.front], NULL, carFromS, &W.front); //south
                car[carSize++] = W.carThread[W.front];
            }
            break;
        default:
            break;
        }
    }
    // pthread_t threadId;
    // int arg = 0;
    // int r = pthread_create(&threadId, NULL, car2N, NULL);
    // printf("hello");
    // system("pause");
    for (int i = 0; i < n; i++)
    {
        pthread_join(car[i], NULL);
    }
    fclose(carLog);
    return 0;
}