//
//  main.c
//  port_scan
//
//  Created by Addison on 14-6-29.
//  Copyright (c) 2014年 Addison. All rights reserved.
//

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>

int conn_nonb(char *ip,int port,int nsec);
void scan_r(char *ip,int s_port,int e_port,int *result);
void thread_run(void *arg);
void mulite_thread_run(char *ip,int s_port,int e_port,int thread_count);
pthread_mutex_t mut;

int **res;

struct argument
{
    char *ip;
    int s_port;
    int e_port;
    int i;
};

int main(int argc, const char * argv[])
{
    
    // insert code here...
    if (argc < 4) {
        printf("参数错误，正确命令：扫描的IP地址 开始端口 结束端口 [线程数]\n");
        return 0;
    }
    printf("端口扫描开始\n");
    
    //int fd[65535];
    char *ip = argv[1];
    int s_port = atoi(argv[2]);
    int e_port = atoi(argv[3]);
    int open[65535];
    int count = 0;
    
    if(argc == 5)
    {
        //使用多线程方式
        int ts = atoi(argv[4]);
        res = (int **)malloc(sizeof(int*)*ts);
        mulite_thread_run(ip, s_port, e_port,ts);
    }else{
        //使用单线程
        for (int i = s_port; i <= e_port; i++) {
            printf("开始扫描%d号端口\n",i);
            int r = -1;
            if ((r=conn_nonb(ip, i, 100)) == 0) {
                open[count] = i;
                count++;
            }
        }
    }
    
    
    return 0;
}

void mulite_thread_run(char *ip,int s_port,int e_port,int thread_count)
{
    int all = e_port - s_port + 1;
    int ts = thread_count;
    int c = all / ts +1;
    
    pthread_mutex_init(&mut,NULL);
    pthread_t *thread;
    thread = (pthread_t*)malloc(sizeof(pthread_t)*thread_count);
    for (int i = 0; i < ts; i++) {
        struct argument *arg1;
        arg1 = (struct argument *)malloc(sizeof(struct argument));
        arg1->ip = ip;
        arg1->s_port = s_port+i * c;
        arg1->e_port = s_port+(i+1)*c;
        arg1->i=i;
        pthread_create(&thread[i], NULL, thread_run,(void *)arg1);
    }
    
    
    for (int j = 0; j < ts; j++) {
        void *thread_return1;
        int ret=pthread_join(thread[j],&thread_return1);/*等待第一个线程退出，并接收它的返回值*/
        if(ret!=0)
            printf("调用pthread_join获取线程1返回值出现错误!\n");
        else
            printf("pthread_join调用成功!线程1退出后带回的值是%d\n",(int)thread_return1);
    }
    
    printf("扫描结果:\n------------------\n");
    for (int k = 0 ; k < ts; k++) {
        int count = res[k][0];
        for(int i = 1 ; i <= count;i++)
        {
            printf("%d 开放\n",res[k][i]);
        }
    }
    
    printf("扫描结束\n------------------\n");
}

void thread_run(void *arg)
{
    struct argument *arg_thread1;/*这里定义了一个指向argument类型结构体的指针arg_thread1，用它来接收传过来的参数的地址*/
    
    arg_thread1=(struct argument *)arg;
    int size = arg_thread1->e_port - arg_thread1->s_port + 2;
    int *result = (int *)malloc(sizeof(int)*size);
    scan_r(arg_thread1->ip, arg_thread1->s_port, arg_thread1->e_port, result);
    res[arg_thread1->i] = result;
    pthread_exit(NULL);
}

void scan_r(char *ip,int s_port,int e_port,int *result)
{
    int count = 0;
    for (int i = s_port; i <= e_port; i++) {
        printf("开始扫描%d号端口\n",i);
        
        int r = -1;
        if ((r=conn_nonb(ip, i, 100)) == 0) {
            count++;
            result[count]= i;
        }
        
    }
    result[0] = count;
   
}

int conn_nonb(char *ip,int port,int nsec)
{
    int flags, n, error;
    //socklen_t len;
    fd_set rset,wset;
    struct timeval tval;
    
    FD_ZERO(&wset);
    FD_ZERO(&rset);
    tval.tv_sec = 1;
    tval.tv_usec = 0;
    //struct servent *sent;
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);
    
    error = 0;
    if((n=connect(sockfd,(struct sockaddr *)&address,sizeof(address)))<0){
        if(errno!=EINPROGRESS)
        {
            printf("Connecting 1 error!\n");
            return -1;
        }
        else if(n==0)
        { //This case may be happen on localhost
            printf("Connecting 1 success! \n");
            return 0;
        }
    }
    FD_SET(sockfd,&rset);
    wset=rset;
    //usleep(10);
    
    /* Do whatever we want while the connect is taking place */
    
    int rst=select(sockfd+1, &rset,&wset,NULL,&tval);
    
    switch (rst) {
        case -1:
            perror("Select error"); exit(-1);
        case 0:
            close(sockfd);
            printf("Timed Out!\n");
            break;
        default:
            if (FD_ISSET(sockfd,&rset)||FD_ISSET(sockfd,&wset)) {
                int error;
                socklen_t len = sizeof (error);
                if(getsockopt(sockfd,SOL_SOCKET,SO_ERROR,&error,&len) < 0)
                {
                    printf ("getsockopt fail,connected fail\n");
                    return -1;
                }
                if(error==0)
                {
                    printf ("%d开放\n",port);
                    return 0;
                }
            }
            close(sockfd);
    }
    return -1;
}

