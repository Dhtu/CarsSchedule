#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#define DEBUG
// const int QMaxSize = 10; //max size of queue
#define QMaxSize 10
//queue struct
typedef struct
{
    int data[10];
    int front; //front
    int back;  //back
} Q;
//check full
int isFull(Q q)
{
    return q.front == q.back;
}
//check empty
int isEmpty(Q q)
{
    return !isFull(q);
}
//push element
int push(Q q, int e)
{
    if (!isFull(q)) //check full
    {
        q.back++;
        q.back %= QMaxSize;
        q.data[q.back] = e;
        return 1;
    }
    return 0;
}
//poll element
int poll(Q q)
{
    if (!isEmpty(q)) //check empty
    {
        int temp = q.data[q.front++];
        q.front %= QMaxSize;
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
//global var
Q N, S, E, W; //queue of each dir

pthread_mutex_t a, b, c, d;                 //the source of the road
pthread_mutex_t waitN, waitS, waitE, waitW; //the waiting line of the road
pthread_cond_t N2E, E2S, S2W, W2N;          //the condition mutex to wait right
int sN, sS, sW, sE;                         //signal of each dir
sem_t empty;                                //the deadlock signal

FILE* carLog;
//car
void *carFromS(void *arg)
{
    int id = *(int *)(arg);
    pthread_mutex_lock(&waitS); //lock follow car
    sS = 1;
    pthread_mutex_lock(&a);
    sem_wait(&empty);
    if (sE)
    {
        pthread_cond_wait(&E2S, &waitS); //wait east car go first
    }

    printf("car %d from South arrives crossing\n", id);
    pthread_mutex_lock(&b);
    sleep(1);//to cause deadlock
    pthread_mutex_unlock(&b);
    sem_post(&empty);
    pthread_mutex_unlock(&a);
    printf("car %d from South leaving crossing\n", id);
    pthread_cond_signal(&S2W);
    sS = 0;
    pthread_mutex_unlock(&waitS); //next follow car
    // printf("hello");
    return 0;
}
void *carFromN(void *arg)
{
    int id = *(int *)(arg);
    int sem;
    pthread_mutex_lock(&waitN); //lock follow car
    sN = 1;
    pthread_mutex_lock(&c);
    sem_wait(&empty);
    if (sW)
    {
        sem_getvalue(&empty,&sem);
        if (sem==0)
        {
            printf("DEADLOCK: car jam detexted, signalling North to go\n");
            sem_post(&empty);
            pthread_mutex_unlock(&c);
            sN=0;
            pthread_mutex_unlock(&waitN);
            return 0;
        }
        
        pthread_cond_wait(&W2N, &waitN); //wait west car go first
    }

    printf("car %d from North arrives crossing\n", id);
    pthread_mutex_lock(&d);
    sleep(1);//to cause deadlock
    pthread_mutex_unlock(&d);
    sem_post(&empty);
    pthread_mutex_unlock(&c);

    printf("car %d from North leaving crossing\n", id);
    pthread_cond_signal(&N2E);
    sN = 0;
    pthread_mutex_unlock(&waitN); //next follow car
    // printf("hello");
    return 0;
}
void *carFromE(void *arg)
{
    int id = *(int *)(arg);
    pthread_mutex_lock(&waitE); //lock follow car
    sE = 1;
    pthread_mutex_lock(&b);
    sem_wait(&empty);
    if (sN)
    {
        pthread_cond_wait(&N2E, &waitE); //wait north car go first
    }

    printf("car %d from East arrives crossing\n", id);
    pthread_mutex_lock(&c);
    sleep(1);//to cause deadlock
    pthread_mutex_unlock(&c);
    sem_post(&empty);
    pthread_mutex_unlock(&b);

    printf("car %d from East leaving crossing\n", id);
    pthread_cond_signal(&E2S);
    sE = 0;
    pthread_mutex_unlock(&waitE); //next follow car
    // printf("hello");
    return 0;
}
void *carFromW(void *arg)
{
    int id = *(int *)(arg);
    pthread_mutex_lock(&waitW); //lock follow car
    sW = 1;
    pthread_mutex_lock(&d);
    sem_wait(&empty);
    if (sS)
    {
        pthread_cond_wait(&S2W, &waitW); //wait west car go first
    }
    printf("car %d from West arrives crossing\n", id);
    pthread_mutex_lock(&a);
    sleep(1);//to cause deadlock
    pthread_mutex_unlock(&a);
    sem_post(&empty);
    pthread_mutex_unlock(&d);

    printf("car %d from West leaving crossing\n", id);
    pthread_cond_signal(&W2N);
    sW = 0;
    pthread_mutex_unlock(&waitW); //next follow car
    // printf("hello");
    return 0;
}

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
    sS = 0;
    sN = 0;
    sW = 0;
    sE = 0;

    //semaphore init
    sem_init(&empty,0,4);

    //init log file
    carLog=fopen("./log","a+");
    if (carLog==NULL)
    {
        printf("log file error\n");
    }
    
    int logE6=fprintf(carLog,"log starts\n");
    fflush(carLog);

    char data[20];
    pthread_t car[20];
    freopen("in", "r", stdin);
    int n;
    scanf("%d", &n);
    getchar(); //delete blank
    char c;
    int i;
    int nn, ns, ne, nw;
    for (i = 0; i < 10; i++)
    {
        scanf("%c", &(data[i]));
        c = data[i];
        switch (c) //check which dir
        {
        case 'n':
            // push(N, i);
            nn = i;
            pthread_create(&car[i], NULL, carFromN, &nn); //north
            break;
        case 's':
            // push(S, i);
            ns = i;
            pthread_create(&car[i], NULL, carFromS, &ns); //south
            break;
        case 'e':
            // push(E, i);
            ne = i;
            pthread_create(&car[i], NULL, carFromE, &ne); //east
            break;
        case 'w':
            // push(W, i);
            nw = i;
            pthread_create(&car[i], NULL, carFromW, &nw); //west
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
    for (int i = 1; i < n; i++)
    {
        pthread_join(car[i], NULL);
    }
    fclose(carLog);
    return 0;
}