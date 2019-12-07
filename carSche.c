#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#define DEADLOCK_DETECT //the deadlock config
#define DEBUG           //debug config
#define DRIVING_SEC 1   //the driving time

//the stage of each dir
typedef enum
{
    IDLE,
    START
} dirStage;

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
        q->data[q->rear] = e;
        q->rear++;
        q->rear %= QMaxSize;
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
dirStage nn = IDLE, ns = IDLE, ne = IDLE, nw = IDLE; //the stage of each dir

int empty;                //the deadlock signal
pthread_mutex_t emptyMtx; //the mutex to protect empty

pthread_t car[20]; //the thread id of cars

int carSize = 0; //the car counter

//cars from each dir
void *carFromS(void *arg)
{
    int id = *(int *)(arg); //the id of the car
    char dir[20] = "South"; //the direction of the car
    char logBuffer[100];
    int ddlk = 0;
    pthread_mutex_lock(&waitS); //lock follow car//is it necessary
    sprintf(logBuffer, "Car %d from %s:\t lock the wait signal\n", id, dir);
    toLog(logBuffer);

    ns = START; //not nessary to lock S2
    pthread_mutex_lock(&a);
    sprintf(logBuffer, "Car %d from %s:\t lock the a signal\n", id, dir);
    toLog(logBuffer);

    //print console info
    printf("car %d from South arrives crossing\n", id);
    sprintf(logBuffer, "car %d from South:\t arrives crossing\n", id);
    toLog(logBuffer);

#ifdef DEADLOCK_DETECT
    pthread_mutex_lock(&emptyMtx);
    empty--; //when empty==0, a deadlock happened

    if (empty == 0)
    {
        printf("DEADLOCK: car jam detected\n");
        printf("%s to go\n",dir);
        sprintf(logBuffer, "car %d from South:\t causes a deadlock\n", id);
        toLog(logBuffer);
        ddlk = 1; //ignore the right first rule
    }
    pthread_mutex_unlock(&emptyMtx);

#endif

    //driving
    sleep(DRIVING_SEC);

    //if the east dir has cars
    pthread_mutex_lock(&E2);
    if (ne == START && !ddlk)
    {
        sprintf(logBuffer, "Car %d from %s:\t wait the E2S mutex\n", id, dir);
        toLog(logBuffer);

        pthread_cond_wait(&E2S, &E2); //wait east car go first

        sprintf(logBuffer, "Car %d from %s:\t get the E2S mutex\n", id, dir);
        toLog(logBuffer);
    }
    pthread_mutex_unlock(&E2);

    //the 2nd cross
    if (!ddlk)
        pthread_mutex_lock(&b);
    sprintf(logBuffer, "Car %d from %s:\t lock the b mutex\n", id, dir);
    toLog(logBuffer);

    //driving
    sleep(DRIVING_SEC);

    //release source
    // sem_post(&empty);
    pthread_mutex_unlock(&a);
    pthread_mutex_unlock(&b);
    pthread_mutex_unlock(&waitS); //next follow car
    printf("car %d from South leaving crossing\n", id);
    sprintf(logBuffer, "car %d from South:\t leaving crossing\n", id);
    toLog(logBuffer);

    //signal west to go
    pthread_cond_signal(&S2W);
    sprintf(logBuffer, "Car %d from %s:\t release the S2W signal 1\n", id, dir);
    toLog(logBuffer);

    //add next car
    poll(&S);
    if (!isEmpty(S))
    {
        pthread_create(&S.carThread[S.front], NULL, carFromS, &S.data[S.front]);
        car[carSize++] = S.carThread[S.front];
    }
    else
    {
        pthread_mutex_lock(&S2);
        pthread_cond_signal(&S2W); //last signal
        ns = IDLE;
        pthread_mutex_unlock(&S2);
        sprintf(logBuffer, "Car %d from %s:\t release the S2W signal 2\n", id, dir);
        toLog(logBuffer);
    }

    return 0;
}

void *carFromE(void *arg)
{
    int id = *(int *)(arg); //the id of the car
    char dir[20] = "East";  //the direction of the car
    char logBuffer[100];
    int ddlk = 0;
    pthread_mutex_lock(&waitE); //lock follow car
    sprintf(logBuffer, "Car %d from %s:\t lock the wait mutex\n", id, dir);
    toLog(logBuffer);

    ne = START; //not nessary to lock S2
    pthread_mutex_lock(&b);
    sprintf(logBuffer, "Car %d from %s:\t lock the b mutex\n", id, dir);
    toLog(logBuffer);

    //print console info
    printf("car %d from %s arrives crossing\n", id, dir);
    sprintf(logBuffer, "car %d from %s:\t arrives crossing\n", id, dir);
    toLog(logBuffer);

#ifdef DEADLOCK_DETECT
    pthread_mutex_lock(&emptyMtx);
    empty--; //when empty==0, a deadlock happened

    if (empty == 0)
    {
        printf("DEADLOCK: car jam detected\n");
        printf("%s to go\n",dir);
        sprintf(logBuffer, "car %d from South:\t causes a deadlock\n", id);
        toLog(logBuffer);
        ddlk = 1; //ignore the right first rule
    }
    pthread_mutex_unlock(&emptyMtx);

#endif

    //driving
    sleep(DRIVING_SEC);

    //if the east dir has cars
    pthread_mutex_lock(&N2);
    if (nn == START && !ddlk) //dir refer!!!!!
    {
        sprintf(logBuffer, "Car %d from %s:\t wait the N2E signal\n", id, dir);
        toLog(logBuffer);

        pthread_cond_wait(&N2E, &N2); //wait east car go first

        sprintf(logBuffer, "Car %d from %s:\t get the N2E signal\n", id, dir);
        toLog(logBuffer);
    }
    pthread_mutex_unlock(&N2);

    //the 2nd cross
    if (!ddlk)
        pthread_mutex_lock(&c);
    sprintf(logBuffer, "Car %d from %s:\t lock the c mutex\n", id, dir);
    toLog(logBuffer);

    //driving
    sleep(DRIVING_SEC);

    //release source
    // sem_post(&empty);
    pthread_mutex_unlock(&b);
    pthread_mutex_unlock(&c);
    pthread_mutex_unlock(&waitE); //next follow car
    printf("car %d from %s leaving crossing\n", id, dir);
    sprintf(logBuffer, "car %d from %s:\t leaving crossing\n", id, dir);
    toLog(logBuffer);

    //signal north to go
    pthread_cond_signal(&E2S);
    sprintf(logBuffer, "Car %d from %s:\t signal the E2S signal 1\n", id, dir);
    toLog(logBuffer);

    //add next car
    poll(&E);
    if (!isEmpty(E))
    {
        pthread_create(&E.carThread[E.front], NULL, carFromE, &E.data[E.front]);
        car[carSize++] = E.carThread[E.front];
    }
    else
    {
        pthread_mutex_lock(&E2);
        pthread_cond_signal(&E2S); //last signal
        ne = IDLE;
        pthread_mutex_unlock(&E2);
        sprintf(logBuffer, "Car %d from %s:\t release the E2S signal 2\n", id, dir);
        toLog(logBuffer);
    }

    return 0;
}
void *carFromN(void *arg)
{
    int id = *(int *)(arg); //the id of the car
    char dir[20] = "North"; //the direction of the car
    char logBuffer[100];
    int ddlk = 0;
    pthread_mutex_lock(&waitN); //lock follow car
    sprintf(logBuffer, "Car %d from %s:\t lock the wait mutex\n", id, dir);
    toLog(logBuffer);

    nn = START; //not nessary to lock S2
    pthread_mutex_lock(&c);
    sprintf(logBuffer, "Car %d from %s:\t lock the c mutex\n", id, dir);
    toLog(logBuffer);

    //print console info
    printf("car %d from %s arrives crossing\n", id, dir);
    sprintf(logBuffer, "car %d from %s:\t arrives crossing\n", id, dir);
    toLog(logBuffer);

#ifdef DEADLOCK_DETECT
    pthread_mutex_lock(&emptyMtx);
    empty--; //when empty==0, a deadlock happened

    if (empty == 0)
    {
        printf("DEADLOCK: car jam detected\n");
        printf("%s to go\n",dir);
        sprintf(logBuffer, "car %d from South:\t causes a deadlock\n", id);
        toLog(logBuffer);
        ddlk = 1; //ignore the right first rule
    }
    pthread_mutex_unlock(&emptyMtx);

#endif

    //driving
    sleep(DRIVING_SEC);

    //if the east dir has cars
    pthread_mutex_lock(&W2);
    if (nw == START && !ddlk)
    {
        sprintf(logBuffer, "Car %d from %s:\t wait the W2N signal\n", id, dir);
        toLog(logBuffer);

        pthread_cond_wait(&W2N, &W2); //wait east car go first

        sprintf(logBuffer, "Car %d from %s:\t get the W2N signal\n", id, dir);
        toLog(logBuffer);
    }
    pthread_mutex_unlock(&W2);

    //the 2nd cross
    if (!ddlk)
        pthread_mutex_lock(&d);
    sprintf(logBuffer, "Car %d from %s:\t lock the d mutex\n", id, dir);
    toLog(logBuffer);

    //driving
    sleep(DRIVING_SEC);

    //release source
    // sem_post(&empty);
    pthread_mutex_unlock(&c);
    pthread_mutex_unlock(&d);
    pthread_mutex_unlock(&waitN); //next follow car
    printf("car %d from %s leaving crossing\n", id, dir);
    sprintf(logBuffer, "car %d from %s:\t leaving crossing\n", id, dir);
    toLog(logBuffer);

    //signal north to go
    pthread_cond_signal(&N2E);
    sprintf(logBuffer, "Car %d from %s:\t signal the N2E signal 1\n", id, dir);
    toLog(logBuffer);

    //add next car
    poll(&N);
    if (!isEmpty(N))
    {
        pthread_create(&N.carThread[N.front], NULL, carFromN, &N.data[N.front]);
        car[carSize++] = N.carThread[N.front];
    }
    else
    {
        pthread_mutex_lock(&N2);
        pthread_cond_signal(&N2E); //last signal
        nn = IDLE;
        pthread_mutex_unlock(&N2);
        sprintf(logBuffer, "Car %d from %s:\t release the N2E signal 2\n", id, dir);
        toLog(logBuffer);
    }

    return 0;
}

void *carFromW(void *arg)
{
    int id = *(int *)(arg); //the id of the car
    char dir[20] = "West";  //the direction of the car
    char logBuffer[100];
    int ddlk = 0;
    pthread_mutex_lock(&waitW); //lock follow car
    sprintf(logBuffer, "Car %d from %s:\t lock the wait mutex\n", id, dir);
    toLog(logBuffer);

    nw = START; //not nessary to lock S2
    pthread_mutex_lock(&d);
    sprintf(logBuffer, "Car %d from %s:\t lock the d mutex\n", id, dir);
    toLog(logBuffer);

    //print console info
    printf("car %d from %s arrives crossing\n", id, dir);
    sprintf(logBuffer, "car %d from %s:\t arrives crossing\n", id, dir);
    toLog(logBuffer);

#ifdef DEADLOCK_DETECT
    pthread_mutex_lock(&emptyMtx);
    empty--; //when empty==0, a deadlock happened

    if (empty == 0)
    {
        printf("DEADLOCK: car jam detected\n");
        printf("%s to go\n",dir);
        sprintf(logBuffer, "car %d from South:\t causes a deadlock\n", id);
        toLog(logBuffer);
        ddlk = 1; //ignore the right first rule
    }
    pthread_mutex_unlock(&emptyMtx);

#endif

    //driving
    sleep(DRIVING_SEC);

    //if the east dir has cars
    pthread_mutex_lock(&S2);
    if (ns == START && !ddlk)
    {
        sprintf(logBuffer, "Car %d from %s:\t wait the S2W signal\n", id, dir);
        toLog(logBuffer);

        pthread_cond_wait(&S2W, &S2); //wait east car go first

        sprintf(logBuffer, "Car %d from %s:\t get the S2W signal\n", id, dir);
        toLog(logBuffer);
    }
    pthread_mutex_unlock(&S2);

    //the 2nd cross
    if (!ddlk)
        pthread_mutex_lock(&a);
    sprintf(logBuffer, "Car %d from %s:\t lock the a mutex\n", id, dir);
    toLog(logBuffer);

    //driving
    sleep(DRIVING_SEC);

    //release source
    // sem_post(&empty);
    pthread_mutex_unlock(&d);
    pthread_mutex_unlock(&a);
    pthread_mutex_unlock(&waitW); //next follow car
    printf("car %d from %s leaving crossing\n", id, dir);
    sprintf(logBuffer, "car %d from %s:\t leaving crossing\n", id, dir);
    toLog(logBuffer);

    //signal north to go
    pthread_cond_signal(&W2N);
    sprintf(logBuffer, "Car %d from %s:\t signal the W2N signal 1\n", id, dir);
    toLog(logBuffer);

    //add next car
    poll(&W);
    if (!isEmpty(W))
    {
        pthread_create(&W.carThread[W.front], NULL, carFromW, &W.data[W.front]);
        car[carSize++] = W.carThread[W.front];
    }
    else
    {
        pthread_mutex_lock(&W2);
        pthread_cond_signal(&W2N); //last signal
        nw = IDLE;
        pthread_mutex_unlock(&W2);
        sprintf(logBuffer, "Car %d from %s:\t release the W2N signal 2\n", id, dir);
        toLog(logBuffer);
    }

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

    nw = IDLE;
    ne = IDLE;
    ns = IDLE;
    nn = IDLE;

    //semaphore init
    // sem_init(&empty, 0, 4);
    empty = 4;

    //init log file
    carLog = fopen("./log", "w+");
    if (carLog == NULL)
    {
        printf("log file error\n");
    }
    toLog("log starts\n");

    // start input data
    freopen("in", "r", stdin);
    int n;
    scanf("%d", &n);
    getchar(); //delete blank
    char c;
    int i;

    for (i = 0; i < n; i++)
    {
        scanf("%c", &c);
        switch (c) //check which dir
        {
        case 'n':
            push(&N, i);
            if (nn == IDLE)
            {
                nn = START;
                pthread_create(&N.carThread[N.front], NULL, carFromN, &N.data[N.front]);
                car[carSize++] = N.carThread[N.front];
            }

            break;
        case 's':
            push(&S, i);
            if (ns == IDLE)
            {
                ns = START;
                pthread_create(&S.carThread[S.front], NULL, carFromS, &S.data[S.front]); //south
                car[carSize++] = S.carThread[S.front];
                // poll(&S);
            }
            break;
        case 'e':
            push(&E, i);
            if (ne == IDLE)
            {
                ne = START;
                pthread_create(&E.carThread[E.front], NULL, carFromE, &E.data[E.front]); //east
                car[carSize++] = E.carThread[E.front];
            }
            break;
        case 'w':
            push(&W, i);
            if (nw == IDLE)
            {
                nw = START;
                pthread_create(&W.carThread[W.front], NULL, carFromW, &W.data[W.front]); //south
                car[carSize++] = W.carThread[W.front];
            }
            break;
        default:
            break;
        }
    }

    sleep(n * DRIVING_SEC);
    for (int i = 0; i < n; i++)
    {
        pthread_join(car[i], NULL);
    }

    fclose(carLog);
    return 0;
}

//ToDo: NESW queue is not protected
//ns,nw,nn,ne is not protected
//logBuffer is not protected