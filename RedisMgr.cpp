#include "RedisMgr.hpp"
#include "RedisUtil.hpp"
#include "Configure.hpp"
#include "RedisClient.hpp"

RedisMgr::RedisMgr(bool isNotRedisClus): notRedisClus(isNotRedisClus) {

}

RedisMgr::~RedisMgr() {

}

int RedisMgr::init() {
    return 0;
}

int RedisMgr::final() {
    isRunning = false;
    return 0;
}

int RedisMgr::start() {
    isRunning = true;
    //
    if (0 == Configure::GetInstance().mSingleThread) {
        threadNum = std::thread::hardware_concurrency();
    } else {
        threadNum = 1;
    }
    //
    for (int i = 0; i < threadNum; i++) {
        std::thread t1(std::bind(&RedisMgr::process2, this));
        t1.detach();
    }
    //
    return 0;
}

int RedisMgr::wait() {
    //
    isRunning = false;
    //
    while (threadNum > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    return 0;
}

int RedisMgr::process() {
    LOG_DEBUG("thread-" << std::this_thread::get_id() << " begined.");
    redisClusterContext *cc = redisClusterContextInit();
    assert(cc);
    //
    int status = -1;
    if (true) {
        //
        status = redisClusterSetOptionUsername(cc, Configure::GetInstance().mRedisUser.c_str());
        ASSERT_MSG(status == REDIS_OK, cc->errstr);
        status = redisClusterSetOptionPassword(cc, Configure::GetInstance().mRedisPass.c_str());
        ASSERT_MSG(status == REDIS_OK, cc->errstr);
        //
        std::set<std::string> &sNodes = Configure::GetInstance().mRedisNodes;
        for (auto iter = sNodes.begin(); iter != sNodes.end(); iter++) {
            status = redisClusterSetOptionAddNodes(cc, (*iter).c_str());
            ASSERT_MSG(status == REDIS_OK, cc->errstr);
        }
    } else {
        //
        status = redisClusterSetOptionUsername(cc, CLUSTER_USER);
        ASSERT_MSG(status == REDIS_OK, cc->errstr);
        status = redisClusterSetOptionPassword(cc, CLUSTER_PASS);
        ASSERT_MSG(status == REDIS_OK, cc->errstr);
        //
        status = redisClusterSetOptionAddNodes(cc, CLUSTER_NODE1);
        ASSERT_MSG(status == REDIS_OK, cc->errstr);
        status = redisClusterSetOptionAddNodes(cc, CLUSTER_NODE2);
        ASSERT_MSG(status == REDIS_OK, cc->errstr);
        status = redisClusterSetOptionAddNodes(cc, CLUSTER_NODE3);
        ASSERT_MSG(status == REDIS_OK, cc->errstr);
        status = redisClusterSetOptionAddNodes(cc, CLUSTER_NODE4);
        ASSERT_MSG(status == REDIS_OK, cc->errstr);
        status = redisClusterSetOptionAddNodes(cc, CLUSTER_NODE5);
        ASSERT_MSG(status == REDIS_OK, cc->errstr);
        status = redisClusterSetOptionAddNodes(cc, CLUSTER_NODE6);
        ASSERT_MSG(status == REDIS_OK, cc->errstr);
    }
    //
    status = redisClusterConnect2(cc);
    ASSERT_MSG(status == REDIS_OK, cc->errstr);
    //
    while (isRunning) {
        auto node = taskQueue.get();
        if (nullptr != node) {
            std::string infoSet = "";
            LOG_DEBUG("thread-" << std::this_thread::get_id() << " process: " << node->getInfoSet());
            // regretSum
            if (true) {
                infoSet = "regretSum@" + node->getInfoSet();
                putRedis(cc, infoSet, node->getRegretSum());
            }
            // strategySum
            if (true) {
                infoSet = "strategySum@" + node->getInfoSet();
                putRedis(cc, infoSet, node->getStrategySum());
            }
            // statTrain
            if (true) {
                infoSet = "trainSum@" + node->getInfoSet();
                std::unordered_map<string, double> kv;
                kv.insert({"regretNumber", (double)node->getRegretNumber()});
                kv.insert({"strategyNumber", (double)node->getStrategyNumber()});
                putRedis(cc, infoSet, kv);
            }
            // mValidActions
            if (true) {
                // node->mValidActions;
            }
        }
    }
    //
    redisClusterFree(cc);
    //
    LOG_DEBUG("thread-" << std::this_thread::get_id() << " exited.");
    threadNum--;
    return 0;
}

int RedisMgr::process2() {
    LOG_DEBUG("thread-" << std::this_thread::get_id() << " begined.");
    auto redisCli = new RedisClient("10.10.10.117", 6379, "aitemptest", 0);
    redisCli->openRedis(true);
    //
    while (isRunning) {
        auto node = taskQueue.get();
        if (nullptr != node) {
            std::string infoSet = "";
            LOG_DEBUG("thread-" << std::this_thread::get_id() << " process: " << node->getInfoSet());
            // regretSum
            if (true) {
                infoSet = "regretSum@" + node->getInfoSet();
                redisCli->hashSetAll(infoSet, node->getRegretSum());
            }
            // strategySum
            if (true) {
                infoSet = "strategySum@" + node->getInfoSet();
                redisCli->hashSetAll(infoSet, node->getStrategySum());
            }
            // statTrain
            if (true) {
                infoSet = "trainSum@" + node->getInfoSet();
                std::unordered_map<string, double> kv;
                kv.insert({"regretNumber", (double)node->getRegretNumber()});
                kv.insert({"strategyNumber", (double)node->getStrategyNumber()});
                redisCli->hashSetAll(infoSet, kv);
            }
        }
    }
    //
    redisCli->closeRedis();
    //
    LOG_DEBUG("thread-" << std::this_thread::get_id() << " exited.");
    threadNum--;
    return 0;
}
int RedisMgr::putTask(InfoNode *node) {
    if (nullptr != node) {
        taskQueue.put(node);
    }
    return 0;
}

int RedisMgr::putRedis(redisClusterContext *cc, std::string infoSet,
                       std::unordered_map<string, double> kv) {
    std::ostringstream os;
    for (auto iter = kv.begin(); iter != kv.end(); iter++) {
        os.str("");
        os << "HSET " << infoSet << " " << (*iter).first << " " << (*iter).second;
        if (REDIS_OK != redisClusterAppendCommand(cc, os.str().c_str())) {
            LOG_INFO("@request infoSet=" << infoSet << " failed: str=" << os.str());
        }
    }
    //
    for (auto iter = kv.begin(); iter != kv.end(); iter++) {
        LOG_INFO("@reply: infoSet=" << infoSet << " " << (*iter).first << " " << (*iter).second);
        redisReply *reply;
        redisClusterGetReply(cc, (void **)&reply);
        // CHECK_REPLY_INT(cc, reply, 0);
        freeReplyObject(reply);
        LOG_INFO(os.str());
    }
    //
    return 0;
}