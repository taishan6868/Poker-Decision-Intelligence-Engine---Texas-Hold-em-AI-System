#ifndef __REDIS_MGR_H__
#define __REDIS_MGR_H__

#include<iostream>
#include<thread>
#include<mutex>
#include<atomic>
#include<condition_variable>
#include<queue>
#include<functional>
#include<sstream>
#include<map>
#include <unordered_map>
//
#include<assert.h>
#include<stdbool.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
//
#include"adapters/libevent.h"
#include"hircluster.h"
//
#include "InfoNode.hpp"
#include "Log.hpp"
//
#define CLUSTER_USER ""
#define CLUSTER_PASS ""
#define CLUSTER_NODE1  "10.10.10.117:8001"
#define CLUSTER_NODE2  "10.10.10.183:8003"
#define CLUSTER_NODE3  "10.10.10.184:8005"
#define CLUSTER_NODE4  "10.10.10.117:8002"
#define CLUSTER_NODE5  "10.10.10.183:8004"
#define CLUSTER_NODE6  "10.10.10.184:8006"

using namespace std;

//
class InfoNode;
//
class noncopyable {
protected:
    // 默认的构造函数和析构函数是 protected，
    // 不允许创建noncopyable实例，但允许子类创建实例（即允许派生类构造和析构）。
    noncopyable() = default;
    ~noncopyable() = default;

private:
    // 使用 delete关键字禁止编译器自动产生复制构造函数和复制赋值操作符。
    noncopyable(const noncopyable &) = delete;
    const noncopyable &operator=(const noncopyable &) = delete;
};

//
class ConcurrencyQueue : protected noncopyable {
public:
    //
    void put(InfoNode *val) {
        std::unique_lock<std::mutex> lck(mtx);
        while (!que.empty()) {
            cv.wait(lck);
        }
        //
        que.push(val);
        cv.notify_all();
    }
    //
    InfoNode *get() {
        std::unique_lock<std::mutex> lck(mtx);
        while (que.empty()) {
            cv.wait(lck);
        }
        //
        InfoNode *val = que.front();
        que.pop();
        cv.notify_all();
        return val;
    }

private:
    //
    std::queue<InfoNode *> que;
    //
    std::mutex mtx;
    //
    std::condition_variable cv;
};

class RedisMgr {
public:
    //
    RedisMgr(bool isNotRedisClus = true);
    //
    ~RedisMgr();
    //
    int init();
    //
    int final();
    //
    int start();
    //
    int wait();
    //
    int process();
    //
    int process2();
    //
    int putTask(InfoNode *node);
    //
    int putRedis(redisClusterContext *cc, std::string infoSet, std::unordered_map<string, double> kv);

private:
    //
    ConcurrencyQueue taskQueue;
    //
    std::atomic<bool> isRunning;
    //
    std::atomic<int> threadNum;
    //
    bool notRedisClus;
};

#endif
