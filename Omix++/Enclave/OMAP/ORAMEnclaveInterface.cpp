#ifndef ORAMENCLAVEINTERFACE_CPP
#define ORAMENCLAVEINTERFACE_CPP

#include "../Enclave.h"
#include "Enclave_t.h"
#include "OMAP.h"
#include <string>
#include "DOHEAP.hpp"

static OMAP* omap = NULL;
static DOHEAP* oheap = NULL;

void ecall_setup_oheap(int maxSize) {
    bytes<Key> tmpkey{0};
    oheap = new DOHEAP(maxSize, tmpkey, false);
    //        oheap = new OHeap(omap, maxSize);
}

void ecall_set_new_minheap_node(int newMinHeapNodeV, int newMinHeapNodeDist) {
    //    oheap->setNewMinHeapNode(newMinHeapNodeV, newMinHeapNodeDist);
    Bid id = newMinHeapNodeDist;
    array<byte_t, 16> value;
    std::fill(value.begin(), value.end(), 0);
    for (int i = 0; i < 4; i++) {
        value[i] = (byte_t) (newMinHeapNodeV >> (i * 8));
    }
    oheap->execute(id, value, 2);
}

void ecall_execute_heap_operation(int* v, int* dist, int op) {
    //        oheap->execute(*v, *dist, op);
    int d = *dist;
    Bid id = d;
    int val = *v;
    array<byte_t, 16> value;
    std::fill(value.begin(), value.end(), 0);
    for (int i = 0; i < 4; i++) {
        value[i] = (byte_t) (val >> (i * 8));
    }
    pair<Bid, array<byte_t, 16> > res = oheap->execute(id, value, op);
    int rr = res.first.getValue();
    *dist = rr;
    std::memcpy(v, res.second.data(), sizeof (int));
}

void ecall_dummy_heap_op() {
    //    oheap->dummyOperation();
}

void ecall_extract_min_id(int* id, int* dist) {
    //    oheap->extractMinID(*id, *dist);
}

void ecall_tree_preorder_keys(long long *keys, size_t len) {
    if (omap != NULL) {
        vector<long long> res = omap->treePreOrderKeys();
        if (res.size() != len) {
            printf("Error converting keys\n");
        } else {
            std::copy(res.begin(), res.end(), keys);
        }
    }
}


void ecall_print_tree() {
    if (omap != NULL) {
        omap->printTree();
    }
}

void ecall_setup_oram(int max_size) {
    bytes<Key> tmpkey{0};
    omap = new OMAP(max_size, tmpkey);
}

void ecall_setup_omap_by_client(int max_size,const char *bid, long long rootPos,const char* secretKey){
    bytes<Key> tmpkey;
    std::memcpy(tmpkey.data(), secretKey, Key);
    std::array<byte_t, ID_SIZE> id;
    std::memcpy(id.data(), bid, ID_SIZE);
    Bid rootBid(id);
    omap = new OMAP(max_size, rootBid,rootPos,tmpkey);
}

void ecall_read_node(const char *bid, char* value) {
    std::array<byte_t, ID_SIZE> id;
    std::memcpy(id.data(), bid, ID_SIZE);
    Bid inputBid(id);
    string res = omap->find(inputBid);
    std::memcpy(value, res.c_str(), 16);
}

void ecall_write_node(const char *bid, const char* value) {
    std::array<byte_t, ID_SIZE> id;
    std::memcpy(id.data(), bid, ID_SIZE);
    Bid inputBid(id);
    string val(value);
    omap->insert(inputBid, val);
}

void ecall_delete_node(const char *bid) {
    std::array<byte_t, ID_SIZE> id;
    std::memcpy(id.data(), bid, ID_SIZE);
    Bid inputBid(id);
    omap->deleteNode(inputBid);
}

double ecall_measure_oram_speed(int testSize) {
    return 0;
}

void check_memory(string text) {
    unsigned int required = 0x4f00000; // adapt to native uint
    char *mem = NULL;
    while (mem == NULL) {
        mem = (char*) malloc(required);
        if ((required -= 8) < 0xFFF) {
            if (mem) free(mem);
            printf("Cannot allocate enough memory\n");
            return;
        }
    }

    free(mem);
    mem = (char*) malloc(required);
    if (mem == NULL) {
        printf("Cannot enough allocate memory\n");
        return;
    }
    printf("%s = %d\n", text.c_str(), required);
    free(mem);
}

long long getValue(std::array< uint8_t, 16> id) {
    long long result = 0;
    result += id[0];
    result += (id[1] << 8);
    result += (id[2] << 16);
    result += (id[3] << 24);
    return result;
}

double ecall_measure_omap_speed(int testSize) {
    double time1=0, time2=0, time3=0, time4=0, total = 0, totalWrite = 0, totalRead = 0, totalDelete = 0;
    ecall_setup_oram(testSize);
    int warmup = 1000;
    printf("Warming UP DOMAP:\n");
    for (int i = 0; i < warmup; i++) {
        if (i % 100 == 0) {
            printf("%d/%d\n",i,warmup);
        }
//        total = 0;
        uint32_t randval;
        sgx_read_rand((unsigned char *) &randval, 4);
        int num = (randval % (testSize)) + 1;
        std::array< uint8_t, 16> id;
        std::fill(id.begin(), id.end(), 0);

        for (int j = 0; j < 4; j++) {
            id[3 - j] = (byte_t) (num >> (j * 8));
        }

        string str = to_string(num);
        std::array< uint8_t, 16> value;
        std::fill(value.begin(), value.end(), 0);
        std::copy(str.begin(), str.end(), value.begin());
//        ocall_start_timer(535);
        ecall_write_node((const char*) id.data(), (const char*) value.data());
//        ocall_stop_timer(&time1, 535);

        char* val = new char[16];
//        ocall_start_timer(535);
        ecall_read_node((const char*) id.data(), val);
//        ocall_stop_timer(&time2, 535);
//        total += time1 + time2;
        assert(string(val) == str);
        delete val;
    }
    printf("\n");

    printf("begin test\n");
    omap->treeHandler->logTime = true;
    omap->treeHandler->times[0].clear();
    omap->treeHandler->times[1].clear();
    omap->treeHandler->times[2].clear();
    omap->treeHandler->times[3].clear();

    int tests = 100;
    for (int i = 0; i < tests; i++) {
        total = 0;
//        totalWrite = 0;
//        totalRead = 0;
        uint32_t randval;
        sgx_read_rand((unsigned char *) &randval, 4);
        int num = (randval % (testSize)) + 1;
#if SGX_DEBUG
        printf("ORAM test %d/%d for num=%d\n", i, tests, num);
#endif
        std::array< uint8_t, 16> id;
        std::fill(id.begin(), id.end(), 0);

        for (int k = 0; k < 4; k++) {
            id[3 - k] = (byte_t) (num >> (k * 8));
        }

        string str = to_string(i);
        std::array< uint8_t, 16> value;
        std::fill(value.begin(), value.end(), 0);
        std::copy(str.begin(), str.end(), value.begin());

#if SGX_DEBUG
        printf("Write key=%lld\n", getValue(id));
#endif
        ocall_start_timer(535);
        ecall_write_node((const char*) id.data(), (const char*) value.data());
        ocall_stop_timer(&time1, 535);

//            printf("Write Time:%f\n", time1);
        char* val = new char[16];
#if SGX_DEBUG
        printf("Read key=%lld\n", getValue(id));
#endif
        ocall_start_timer(535);
        ecall_read_node((const char*) id.data(), val);
        ocall_stop_timer(&time2, 535);

//            ecall_print_tree();
#if SGX_DEBUG
        printf("Delete key=%lld\n", getValue(id));
#endif
        ocall_start_timer(535);
        ecall_delete_node((const char*) id.data());
        ocall_stop_timer(&time3, 535);

//        printf("Write key=%d\n", getValue(id));
//        ocall_start_timer(535);
//        ecall_write_node((const char*) id.data(), (const char*) value.data());
//        ocall_stop_timer(&time4, 535);
//            ecall_print_tree();
//            printf("Read Time:%f\n", time2);
        total += time1 + time2 + time3 + time4;
        totalWrite += (time1+time4);
        totalRead += time2;
        totalDelete += time3;
        //printf("expected value:%s result:%s\n",str.c_str(),string(val).c_str());
        assert(string(val) == str);
        delete[] val;
//        printf("Average OMAP Access Time: %f\n", total / 200);

    }
    printf("Average OMAP Read Time: %f\n", totalRead / 100);
    printf("Average OMAP Write Time: %f\n", totalWrite / 100);
    printf("Average OMAP Delete Time: %f\n", totalDelete / 100);

    vector<string> names;
    names.push_back("Write Balance:");
    names.push_back("Write Evict Buckets:");
    names.push_back("Read Tree Traverse:");
    names.push_back("Read Evict Buckets:");

//    for (int i = 0; i < names.size(); i++) {
//        printf("%s:\n", names[i].c_str());
//        for (int j = 0; j < omap->treeHandler->times[i].size(); j++) {
//            printf("%f\n", omap->treeHandler->times[i][j]);
//        }
//    }

    return total / (20);
}

double ecall_measure_eviction_speed(int testSize) {
    bytes<Key> tmpkey{0};
    double time1, total = 0;
    long long s = (long long) pow(2, testSize);
    ORAM* oram = new ORAM(s, tmpkey, true,true);

    for (int j = 0; j < 10; j++) {
        total = 0;
        for (int i = 1; i <= 100; i++) {
            oram->start(false);
            oram->prepareForEvictionTest();
            ocall_start_timer(535);
            oram->evict(true);
            ocall_stop_timer(&time1, 535);
            total += time1;
        }
        printf("Total Eviction Time: %f\n", total / 100);
    }
    return oram->evicttime / oram->evictcount;
}

double ecall_measure_oram_setup_speed(int testSize) {
    vector<Node*> nodes;
    int depth = (int) (ceil(log2(testSize)) - 1) + 1;
    long long maxSize = (int) (pow(2, depth));
    for (int i = 0; i < maxSize; i++) {
        Node* tmp = new Node();
        tmp->index = i + 1;
        tmp->key = i + 1;
        tmp->isDummy = false;
        string value = "test_" + to_string(i + 1);
        std::fill(tmp->value.begin(), tmp->value.end(), 0);
        std::copy(value.begin(), value.end(), tmp->value.begin());
        uint32_t randval;
        sgx_read_rand((unsigned char *) &randval, 4);
        nodes.push_back(tmp);
    }
    map<unsigned long long, unsigned long long> permutation;

    int j = 0;
    int cnt = 0;
    for (int i = 0; i < maxSize * 4; i++) {
        if (i % 1000000 == 0) {
            printf("%d/%d\n", i, maxSize * 4);
        }
        if (cnt == 4) {
            j++;
            cnt = 0;
        }
        permutation[i] = (j + 1) % maxSize;
        cnt++;
    }

    bytes<Key> tmpkey{0};
    double time2;
    ocall_start_timer(535);
    ORAM* oram = new ORAM(maxSize, tmpkey, &nodes, permutation);
    ocall_stop_timer(&time2, 535);
    //    Node* dummyNode = new Node();
    //    dummyNode->isDummy = true;
    //    Bid id = 2;
    //    unsigned long long readpos = 1;
    //    Node* res = oram->ReadWrite(id, dummyNode, readpos, readpos, true, false);
    //     assert(!res->isDummy);
    printf("Setup time is:%f\n", time2);
}

double ecall_measure_omap_setup_speed(int testSize) {
    map<Bid, string> pairs;
    int depth = (int) (ceil(log2(testSize)) - 1) + 1;
    long long maxSize = (int) (pow(2, depth));
    for (int i = 1; i < maxSize / 10; i++) {
        Bid k = i;
        pairs[k] = "test_" + to_string(i);
    }
    map<unsigned long long, unsigned long long> permutation;

    int j = 0;
    int cnt = 0;
    for (int i = 0; i < maxSize * 4; i++) {
        if (i % 1000000 == 0) {
            printf("%d/%d\n", i, maxSize * 4);
        }
        if (cnt == 4) {
            j++;
            cnt = 0;
        }
        permutation[i] = (j + 1) % maxSize;
        cnt++;
    }

    bytes<Key> tmpkey{0};
    double time2;
    ocall_start_timer(535);
    OMAP* omap = new OMAP(maxSize, tmpkey, &pairs, &permutation);
    ocall_stop_timer(&time2, 535);
    printf("Setup time is:%f\n", time2);
    Bid testKey = 3;
    string res = omap->find(testKey);
    assert(strcmp(res.c_str(), "test_3") == 0);

    //    printf("Creating AVL time is:%f\n", omap->treeHandler->times[0][0]);
    //    printf("ORAM Setup:%f\n", omap->treeHandler->times[1][0]);
}
#endif /* ORAMENCLAVEINTERFACE_CPP */

