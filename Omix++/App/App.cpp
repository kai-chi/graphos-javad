#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <pwd.h>
#include <array>
#include <iostream>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <algorithm>

using namespace std;
#define MAX_PATH FILENAME_MAX

#include "sgx_urts.h"
#include "App.h"
#include "Enclave_u.h"
#include "OMAP/RAMStoreEnclaveInterface.h"
#include "../Common/Common.h"
#include "OMAP/Node.h"
/*
 * Copyright (C) 2011-2018 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "Enclave_u.h"
#include "AVL.h"


#ifndef SAFE_FREE
#define SAFE_FREE(ptr) {if (NULL != (ptr)) {free(ptr); (ptr) = NULL;}}
#endif

// In addition to generating and sending messages, this application
// can use pre-generated messages to verify the generation of
// messages and the information flow.

#define ENCLAVE_PATH "isv_enclave.signed.so"

/* Global EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;

typedef struct _sgx_errlist_t {
    sgx_status_t err;
    const char *msg;
    const char *sug; /* Suggestion */
} sgx_errlist_t;

/* Error code returned by sgx_create_enclave */
static sgx_errlist_t sgx_errlist[] = {
    {
        SGX_ERROR_UNEXPECTED,
        "Unexpected error occurred.",
        NULL
    },
    {
        SGX_ERROR_INVALID_PARAMETER,
        "Invalid parameter.",
        NULL
    },
    {
        SGX_ERROR_OUT_OF_MEMORY,
        "Out of memory.",
        NULL
    },
    {
        SGX_ERROR_ENCLAVE_LOST,
        "Power transition occurred.",
        "Please refer to the sample \"PowerTransition\" for details."
    },
    {
        SGX_ERROR_INVALID_ENCLAVE,
        "Invalid enclave image.",
        NULL
    },
    {
        SGX_ERROR_INVALID_ENCLAVE_ID,
        "Invalid enclave identification.",
        NULL
    },
    {
        SGX_ERROR_INVALID_SIGNATURE,
        "Invalid enclave signature.",
        NULL
    },
    {
        SGX_ERROR_OUT_OF_EPC,
        "Out of EPC memory.",
        NULL
    },
    {
        SGX_ERROR_NO_DEVICE,
        "Invalid SGX device.",
        "Please make sure SGX module is enabled in the BIOS, and install SGX driver afterwards."
    },
    {
        SGX_ERROR_MEMORY_MAP_CONFLICT,
        "Memory map conflicted.",
        NULL
    },
    {
        SGX_ERROR_INVALID_METADATA,
        "Invalid enclave metadata.",
        NULL
    },
    {
        SGX_ERROR_DEVICE_BUSY,
        "SGX device was busy.",
        NULL
    },
    {
        SGX_ERROR_INVALID_VERSION,
        "Enclave version was invalid.",
        NULL
    },
    {
        SGX_ERROR_INVALID_ATTRIBUTE,
        "Enclave was not authorized.",
        NULL
    },
    {
        SGX_ERROR_ENCLAVE_FILE_ACCESS,
        "Can't open enclave file.",
        NULL
    },
    {
        SGX_ERROR_NDEBUG_ENCLAVE,
        "The enclave is signed as product enclave, and can not be created as debuggable enclave.",
        NULL
    },
};

/* Check error conditions for loading enclave */
void print_error_message(sgx_status_t ret) {
    size_t idx = 0;
    size_t ttl = sizeof sgx_errlist / sizeof sgx_errlist[0];

    for (idx = 0; idx < ttl; idx++) {
        if (ret == sgx_errlist[idx].err) {
            if (NULL != sgx_errlist[idx].sug)
                printf("Info: %s\n", sgx_errlist[idx].sug);
            printf("Error: %s\n", sgx_errlist[idx].msg);
            break;
        }
    }

    if (idx == ttl)
        printf("Error: Unexpected error occurred.\n");
}

/* Initialize the enclave:
 *   Call sgx_create_enclave to initialize an enclave instance
 */
int initialize_enclave(void) {
    sgx_launch_token_t token = {0};
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    int updated = 0;

    /* Call sgx_create_enclave to initialize an enclave instance */
    /* Debug Support: set 2nd parameter to 1 */
    ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, &token, &updated, &global_eid, NULL);
    if (ret != SGX_SUCCESS) {
        print_error_message(ret);
        return -1;
    }

    return 0;
}

std::string exec(const char* cmd) {
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}

/* Application entry */
#define _T(x) x

void bitonicSort(vector<Node*>* nodes, int step);
void bitonic_sort(vector<Node*>* nodes, int low, int n, int dir, int step);
void bitonic_merge(vector<Node*>* nodes, int low, int n, int dir, int step);
void compare_and_swap(Node* item_i, Node* item_j, int dir, int step);
int greatest_power_of_two_less_than(int n);

void bitonicSort(vector<Node*>* nodes, int step) {
    int len = nodes->size();
    bitonic_sort(nodes, 0, len, 1, step);
}

void bitonic_sort(vector<Node*>* nodes, int low, int n, int dir, int step) {
    if (n > 1) {
        int middle = n / 2;
        bitonic_sort(nodes, low, middle, !dir, step);
        bitonic_sort(nodes, low + middle, n - middle, dir, step);
        bitonic_merge(nodes, low, n, dir, step);
    }
}

void bitonic_merge(vector<Node*>* nodes, int low, int n, int dir, int step) {
    if (n > 1) {
        int m = greatest_power_of_two_less_than(n);

        for (int i = low; i < (low + n - m); i++) {
            if (i != (i + m)) {
                compare_and_swap((*nodes)[i], (*nodes)[i + m], dir, step);
            }
        }

        bitonic_merge(nodes, low, m, dir, step);
        bitonic_merge(nodes, low + m, n - m, dir, step);
    }
}

void compare_and_swap(Node* item_i, Node* item_j, int dir, int step) {
    if (step == 1) {
        if (dir == (item_i->key > item_j->key ? 1 : 0)) {
            std::swap(*item_i, *item_j);
        }
    } else {
        if (dir == (item_i->evictionNode > item_j->evictionNode ? 1 : 0)) {
            std::swap(*item_i, *item_j);
        }
    }
}

int greatest_power_of_two_less_than(int n) {
    int k = 1;
    while (k > 0 && k < n) {
        k = k << 1;
    }
    return k >> 1;
}

int permutationIterator = 0;
unsigned long long indx = 1;
map<unsigned long long, unsigned long long> permutation;

int sortedArrayToBST(vector<Node*>* nodes, long long start, long long end, unsigned long long& pos, Bid& node) {
    if (start > end) {
        pos = -1;
        node = 0;
        return 0;
    }

    unsigned long long mid = (start + end) / 2;
    Node *root = (*nodes)[mid];

    int leftHeight = sortedArrayToBST(nodes, start, mid - 1, root->leftPos, root->leftID);

    int rightHeight = sortedArrayToBST(nodes, mid + 1, end, root->rightPos, root->rightID);
    root->pos = permutation[permutationIterator];
    permutationIterator++;
    root->height = max(leftHeight, rightHeight) + 1;
    pos = root->pos;
    node = root->key;
    return root->height;
}

void initializeORAM(long long maxSize, bytes<Key> secretkey, Bid& rootKey, unsigned long long& rootPos, map<Bid, string>* pairs, vector<long long>* indexes, vector<block>* ciphertexts, size_t& blockCount, unsigned long long& storeBlockSize) {
    int depth = (int) (ceil(log2(maxSize)) - 1) + 1;
    int maxOfRandom = (long long) (pow(2, depth));


    int j = 0;
    int cnt = 0;
    for (int i = 0; i < maxOfRandom * 4; i++) {
        if (i % 1000000 == 0) {
            printf("%d/%d\n", i, maxOfRandom * 4);
        }
        if (cnt == 4) {
            j++;
            cnt = 0;
        }
        permutation[i] = (j + 1) % maxOfRandom;
        cnt++;
    }



    vector<Node*> nodes;
    for (auto pair : (*pairs)) {
        Node* node = new Node();
        node->key = pair.first;
        node->index = indx++;
        std::fill(node->value.begin(), node->value.end(), 0);
        std::copy(pair.second.begin(), pair.second.end(), node->value.begin());
        node->leftID = 0;
        node->leftPos = -1;
        node->rightPos = -1;
        node->rightID = 0;
        node->pos = 0;
        node->isDummy = false;
        node->height = 1; // new node is initially added at leaf
        nodes.push_back(node);
    }

    bitonicSort(&nodes, 1);
    printf("Creating BST of %d Nodes\n", nodes.size());
    sortedArrayToBST(&nodes, 0, nodes.size() - 1, rootPos, rootKey);
    printf("Inserting in ORAM\n");

    int size = (int) nodes.size();
    for (int i = size; i < maxOfRandom * Z; i++) {
        Bid bid;
        bid = 9223372036854775807 + i;
        Node* node = new Node();
        node->key = bid;
        node->index = indx++;
        std::fill(node->value.begin(), node->value.end(), 0);
        std::copy(string("").begin(), string("").end(), node->value.begin());
        node->leftID = 0;
        node->leftPos = -1;
        node->rightPos = -1;
        node->rightID = 0;
        node->pos = 0;
        node->isDummy = false;
        node->height = 1; // new node is initially added at leaf
        node->isDummy = true;
        node->pos = permutation[permutationIterator];
        permutationIterator++;
        nodes.push_back(node);
    }

    //----------------------------------------------------------------
    AES::Setup();
    unsigned long long bucketCount = maxOfRandom * 2 - 1;
    unsigned long long INF = 9223372036854775807 - (bucketCount);

    printf("Number of leaves:%lld\n", maxOfRandom);
    printf("depth:%lld\n", depth);

    unsigned long long nextDummyCounter = INF;
    unsigned long long blockSize = sizeof (Node); // B  
    printf("block size is:%d\n", blockSize);
    blockCount = (size_t) (Z * bucketCount);
    storeBlockSize = (size_t) (IV + AES::GetCiphertextLength((int) (Z * (blockSize))));
    unsigned long long clen_size = AES::GetCiphertextLength((int) (blockSize) * Z);
    unsigned long long plaintext_size = (blockSize) * Z;
    unsigned long long maxHeightOfAVLTree = (int) floor(log2(blockCount)) + 1;

    unsigned long long first_leaf = bucketCount / 2;

    Bucket* bucket = new Bucket();



    int i;
    printf("Setting Nodes Eviction ID\n");
    for (i = 0; i < nodes.size(); i++) {
        nodes[i]->evictionNode = nodes[i]->pos + first_leaf;
    }

    printf("Sorting\n");
    bitonicSort(&nodes, 2);

    vector<Bucket> buckets;

    long long first_bucket_of_last_level = bucketCount / 2;
    j = 0;

    for (unsigned int i = 0; i < nodes.size(); i++) {
        if (i % 100000 == 0) {
            printf("Creating Buckets:%d/%d\n", i, nodes.size());
        }
        Node* cureNode = nodes[i];
        long long curBucketID = nodes[i]->evictionNode;

        Block &curBlock = (*bucket)[j];
        curBlock.data.resize(blockSize, 0);

        std::array<byte_t, sizeof (Node) > data;

        const byte_t* begin = reinterpret_cast<const byte_t*> (std::addressof(*cureNode));
        const byte_t* end = begin + sizeof (Node);
        std::copy(begin, end, std::begin(data));

        block tmp(data.begin(), data.end());

        if (cureNode->isDummy) {
            curBlock.id = 0;
        } else {
            curBlock.id = cureNode->index;
        }
        for (int k = 0; k < tmp.size(); k++) {
            if (cureNode->isDummy == false) {
                curBlock.data[k] = tmp[k];
            }
        }
        delete cureNode;
        j++;

        if (j == Z) {
            (*indexes).push_back(curBucketID);
            buckets.push_back((*bucket));
            delete bucket;
            bucket = new Bucket();
            j = 0;
        }
    }

    for (int i = 0; i < first_bucket_of_last_level; i++) {
        if (i % 100000 == 0) {
            printf("Adding Upper Levels Dummy Buckets:%d/%d\n", i, nodes.size());
        }
        for (int z = 0; z < Z; z++) {
            Block &curBlock = (*bucket)[z];
            curBlock.id = 0;
            curBlock.data.resize(blockSize, 0);
        }
        (*indexes).push_back(i);
        buckets.push_back((*bucket));
        delete bucket;
        bucket = new Bucket();
    }
    delete bucket;

    for (int i = 0; i < (*indexes).size(); i++) {
        block buffer;
        for (int z = 0; z < Z; z++) {
            Block b = buckets[i][z];
            buffer.insert(buffer.end(), b.data.begin(), b.data.end());
        }
        block ciphertext = AES::Encrypt(secretkey, buffer, clen_size, plaintext_size);
        (*ciphertexts).push_back(ciphertext);
    }
}

void initializeNodes(bytes<Key> secretkey, map<Bid, string>* pairs, vector<block>* ciphertexts) {
    vector<Node*> nodes;
    for (auto pair : (*pairs)) {
        Node* node = new Node();
        node->key = pair.first;
        node->index = 0;
        std::fill(node->value.begin(), node->value.end(), 0);
        std::copy(pair.second.begin(), pair.second.end(), node->value.begin());
        node->leftID = 0;
        node->leftPos = -1;
        node->rightPos = -1;
        node->rightID = 0;
        node->pos = 0;
        node->isDummy = false;
        node->height = 1; // new node is initially added at leaf
        nodes.push_back(node);
    }

    unsigned long long blockSize = sizeof (Node);
    unsigned long long clen_size = AES::GetCiphertextLength((int) (blockSize));
    unsigned long long plaintext_size = (blockSize);

    for (int i = 0; i < nodes.size(); i++) {
        std::array<byte_t, sizeof (Node) > data;

        const byte_t* begin = reinterpret_cast<const byte_t*> (std::addressof((*nodes[i])));
        const byte_t* end = begin + sizeof (Node);
        std::copy(begin, end, std::begin(data));

        block buffer(data.begin(), data.end());
        block ciphertext = AES::Encrypt(secretkey, buffer, clen_size, plaintext_size);
        (*ciphertexts).push_back(ciphertext);
    }
}

int SGX_CDECL main(int argc, char *argv[]) {
    (void) (argc);
    (void) (argv);

    bool check_remote_attestation = false;
    double t;

    /* Initialize the enclave */
    if (initialize_enclave() < 0) {
        printf("Enter a character before exit ...\n");
        getchar();
        return -1;
    } else {
        printf("Enclave id = %d\n", global_eid);
    }

    /* My Codes */
    int maxSize = 32;
    int test_case = 6;
    int experiment = 0;
    if (argc == 4) {
        maxSize = stoi(argv[1]);
        test_case = stoi(argv[2]);
        experiment = stoi(argv[3]);
    }
    else if (argc == 3) {
        maxSize = stoi(argv[1]);
        test_case = stoi(argv[2]);
    } else if (argc == 2) {
        maxSize = stoi(argv[1]);
    }
    printf("maxSize = %d\n", maxSize);


    vector<int> ids;
    vector<long long> preorder_after_deletion;
    int id;
    switch(test_case) {
        case 0:
            // delete leaf and get no rebalancing
            ids = {2,1,3};
            id = 3;
            preorder_after_deletion = {2,1};
            break;
        case 1:
            // delete leaf 5 and get left-left case
            ids = {4,2,5,1,3};
            id = 5;
            preorder_after_deletion = {2,1,4,3};
            break;
        case 2:
            // delete leaf 1 and get right-right case
            ids = {2,1,3,4,5};
            id = 1;
            preorder_after_deletion = {4,2,3,5};
            break;
        case 3:
            // delete leaf 7 and get left-right case
            ids = {5,2,6,1,4,7,3};
            id = 7;
            preorder_after_deletion = {4,2,1,3,5,6};
            break;
        case 4:
            // delete leaf 1 and get right-left case
            ids = {3,2,6,1,5,7,4};
            id = 1;
            preorder_after_deletion = {5,3,2,4,6,7};
            break;
        case 5:
            // delete leaf 19 and get worst-case rebalancing
            ids = {13,8,18,5,11,16,20,3,7,10,12,15,17,19,2,4,6,9,14,1};
            id = 19;
            preorder_after_deletion = {8,5,3,2,1,4,7,6,13,11,10,9,12,16,15,14,18,17,20};
            break;
        case 6:
            // delete 5 with only right child and get no balancing
            ids = {4,2,5,1,3,6};
            id = 5;
            preorder_after_deletion = {4,2,1,3,6};
            break;
        case 7:
            // delete 6 with only left child and get no balancing
            ids = {4,2,6,1,3,5};
            id = 6;
            preorder_after_deletion = {4,2,1,3,5};
            break;
        case 8:
            // delete 20 with only left child and get worst-case rebalancing
            ids = {13,8,18,5,11,16,20,3,7,10,12,15,17,19,2,4,6,9,14,1};
            id = 20;
            preorder_after_deletion = {8,5,3,2,1,4,7,6,13,11,10,9,12,16,15,14,18,17,19};
            break;
        case 9:
            // delete 2 with two children and get no rebalancing
            ids = {4,2,5,1,3};
            id = 2;
            preorder_after_deletion = {4,3,1,5};
            break;
        case 10:
            // delete root with no children
            ids = {1};
            id = 1;
            preorder_after_deletion = {};
            break;
        case 11:
            // delete root with one left child
            ids = {2, 1};
            id = 2;
            preorder_after_deletion = {1};
            break;
        case 12:
            // delete root with one right child
            ids = {1, 2};
            id = 1;
            preorder_after_deletion = {2};
            break;
        case 13:
            // delete root with two children
            ids = {2, 1, 3};
            id = 2;
            preorder_after_deletion = {3,1};
            break;
        case 14:
            // delete 4 (root) with two children and get left-left case
            ids = {4,2,5,1,3};
            id = 4;
            preorder_after_deletion = {2,1,5,3};
            break;
        case 15:
            // delete 10 with two children and get left-left case
            ids = {8,5,10,3,6,9,12,2,4,7,11,1};
            id = 10;
            preorder_after_deletion = {5,3,2,1,4,8,6,7,11,9,12};
            break;
        case 16:
            // delete 10 with two children and get left-left case and another delete
            ids = {8,5,10,3,6,9,12,2,4,7,13,1};
            id = 10;
            preorder_after_deletion = {5,3,2,1,4,8,6,7,12,9,13};
            break;
        case 17:
            // delete 3 with two children and get double rotation
            ids = {5,3,8,1,4,7,10,2,6,9,11,12};
            id = 3;
            preorder_after_deletion = {8,5,2,1,4,7,6,10,9,11,12};
            break;
        case 18:
            // delete 18 with two children and get worst-case rebalancing
            ids = {13,8,18,5,11,16,20,3,7,10,12,15,17,19,2,4,6,9,14,1};
            id = 18;
            preorder_after_deletion = {8,5,3,2,1,4,7,6,13,11,10,9,12,16,15,14,19,17,20};
            break;
        case 19:
            // some difficult tree
            ids = {9,4,12,2,6,10,13,1,3,5,7,11,8};
            id = 13;
            preorder_after_deletion = {6,4,2,1,3,5,9,7,8,11,10,12};
            break;
        case 20:
            ids = {10,4,26,2,8,17,30,3,5,13,20,27,19,25};
            id = 11;
            preorder_after_deletion = {10,4,2,3,8,5,26,17,13,20,19,25,30,27};
            break;
        case 21:
            ids = {8, 3, 13, 2, 5, 11, 14, 4, 7, 10};
            id = 2;
            preorder_after_deletion = {8,5,3,4,7,13,11,10,14};
        default:
            break;
    }

    if (experiment == 1) {
//        ecall_measure_omap_speed(global_eid, &t, maxSize);
        ecall_measure_btree_read_speed(global_eid, maxSize);
        sgx_destroy_enclave(global_eid);
        return 0;
    }
    else if (experiment == 2) {
//        ecall_measure_omap_speed(global_eid, &t, maxSize);
        ecall_measure_btree_read_write_speed(global_eid, maxSize);
        sgx_destroy_enclave(global_eid);
        return 0;
    }
//    ecall_measure_omap_setup_speed(global_eid, &t, maxSize);


    //******************************************************************************
    //******************************************************************************
    //*************************INITIALIZE ORAM FROM CLIENT**************************
    //******************************************************************************
    //******************************************************************************

    bytes<Key> secretkey{0};
    Bid rootKey;
    unsigned long long rootPos;
    map<Bid, string> pairs;
    vector<long long> indexes;
    vector<block> ciphertexts;
    size_t blockCount;
    unsigned long long storeBlockSize;

//    int ids[] = {32, 16, 48, 40};
    int ids_length = ids.size();

//    for (int i = 0; i < ids_length; i++) {
//        Bid k = ids[i];
//        pairs[k] = "test_" + to_string(ids[i]);
//    }

    initializeORAM(maxSize, secretkey, rootKey, rootPos, &pairs, &indexes, &ciphertexts, blockCount, storeBlockSize);
    ocall_setup_ramStore(blockCount, storeBlockSize);
    ocall_nwrite_ramStore_by_client(&indexes, &ciphertexts);
    ecall_setup_omap_by_client(global_eid, maxSize, (const char*) rootKey.id.data(), rootPos, (const char*) secretkey.data());

    AVL::Node *root = NULL;
    int it = 0;
    srand (time(0));
//    srand (12345);

    // TEST CASE
    if (false) {
        for (int i = 0; i < ids_length; i++) {
            Bid tmp = ids[i];
            string val = "test_" + to_string(ids[i]);
            ecall_write_node(global_eid, (const char *) tmp.id.data(), val.c_str());
            root = AVL::insert(root, ids[i]);
        }
        Bid del = id;
        ecall_delete_node(global_eid, (const char *) del.id.data());
        root = AVL::deleteNode(root, id);

        int res_size = preorder_after_deletion.size();
        long long *keys = new long long[res_size];
        ecall_tree_preorder_keys(global_eid, keys, res_size);
        cout << "********************" << endl;
        cout << "Preorder keys: ";
        for (int i = 0; i < res_size; i++) {
            cout << keys[i] << ", ";
        }
        cout << endl;
        for (int i = 0; i < res_size; i++) {
            assert(keys[i] == preorder_after_deletion[i]);
        }
        delete[] keys;
    }

    while(true) {
        printf("iteration %d\n", it);
        int i;
//        if (it < 1) {
        i = rand() % (maxSize/2) + 1;
        Bid k = i;
        string val = "test_" + to_string(i);
        printf("Write %d\n", i);
        ecall_write_node(global_eid, (const char*) k.id.data(), val.c_str());
        root = AVL::insert(root, i);
        printf("************ iteration %d - after Write operation ************************\n", it);
        ecall_print_tree(global_eid);
        printf("****************************************\n");
//        }
        i = rand() % (maxSize/2) + 1;
        printf("Delete %d\n", i);
        Bid l = i;
        ecall_delete_node(global_eid, (const char*) l.id.data());
        root = AVL::deleteNode(root, i);

        long long *keys = new long long[maxSize];
        ecall_tree_preorder_keys(global_eid, keys, maxSize);
        std::vector<int> avlKeys = AVL::preOrder(root);
        printf("************ iteration %d - after Delete operation ************************\n", it);
        ecall_print_tree(global_eid);
        printf("****************************************\n");
        AVL::printTree(root, 0);
        printf("****************************************\n");
        for (auto elem : avlKeys) {
            cout << elem << ",";
        }
        cout << endl;
        for (int i = 0; i < avlKeys.size(); i++) {
            if (avlKeys.at(i) != keys[i]) {
                throw runtime_error("");
            }
        }
        delete[] keys;
        it++;
    }

    if (false) {
        ids = {73,627,753,740,756,758,582,277,436,57,401,233,546,460,389,669,45,757,135,751,503,203,896,109,467,907,619,834,494,607,155,626,225,775,787,642,661,254,315,491,85,697,633,234,647,412,82,443,268,851,932,884,739,605,850,866,32,830,929,939,431,841,1022,294,17,64,236,238,72,480,950,367,292,600,951,872,519,551,930,227,317,38,995,439,587,947,572,54,815,181,337,836,1023,163,506,286,699,941,944,663,362,629,384,678,117,143,931,393,188,65,205,659,971,231,36,927,618,309,886,257,160,261,29,420,977,745,545,120,848,711,263,161,303,122,346,754,253,265,11,568,698,996,171,904,976,383,175,969,417,786,937,352,604,759,986,844,718,39,128,6,1011,191,137,746,554,335,905,55,838,441,101,560,585,528,854,433,767,703,890,534,517,52,48,639,542,12,934,115,89,110,400,207,768,440,81,186,603,514,461,793,49,500,970,928,522,77,485,272,354,90,410,426,720,914,316,313,457,423,468,305,874,902,655,645,532,79,50,861,474,891,174,434,671,578,430,364,806,13,392,133,964,849,60,347,339,805,738,157,1017,567,414,660,262,916,732,75,134,773,376,766,275,348,18,365,737,543,978,654,409,897,84,33,1001,264,496,271,445,270,473,99,100,908,247,88,811,385,325,176,398,350,363,95,688,318,991,535,250,1015,839,396,1003,510,855,78,959,25,549,727,586,214,150,547,269,266,695,784,458,80,332,824,609,465,432,646,523,525,707,812,816,832,536,244,562,428,493,783,624,829,847,621,418,216,831,566,602,198,497,674,559,518,387,989,394,563,664,878,375,51,91,888,575,278,1020,326,447,199,823,329,679,769,464,399,42,822,144,606,741,252,803,222,961,858,463,915,7,356,62,299,131,648,651,355,899,945,729,918,419,200,219,946,689,280,83,859,66,291,314,368,256,864,926,125,304,797,923,103,245,548,571,924,610,895,138,565,116,853,35,781,344,817,577,63,232,212,340,454,999,324,846,142,894,863,974,218,885,402,192,71,714,448,46,752,993,113,527,229,865,809,725,488,146,183,168,129,185,843,658,359,209,58,706,379,649,761,30,366,742,869,28,722,217,564,901,22,471,148,700,302,875,239,145,19,684,637,382,693,297,190,86,795,919,632,968,470,868,319,873,338,499,840,361,747,453,903,614,613,202,943,1024,702,255,529,139,588,819,992,673,504,508,936,828,967,223,1019,119,701,988,274,774,656,173,495,530,289,910,164,15,736,351,193,801,67,141,483,505,425,240,34,807,267,666,954,592,1021,260,403,415,672,108,667,342,920,293,388,634,511,381,391,1008,301,56,743,123,599,900,498,416,909,47,156,126,149,197,328,802,230,311,1009,285,241,765,636,1004,377,10,749,596,876,979,657,189,372,182,987,479,300,475,860,459,513,579,653,159,590,20,194,570,76,98,349,74,574,573,290,407,677,158,721,748,687,558,972,533,733,380,215,818,611,333,53,1013,792,411,760,24,597,102,808,981,162,427,676,178,23,140,550,26,184,631,593,97,877,957,776,43,437,555,764,251,358,717,778,696,492,856,922,622,625,963,237,833,424,312,307,1014,650,615,106,92,616,369,455,1,502,827,1018,798,208,152,526,942,994,906,195,933,630,694,404,670,70,644,1010,446,373,406,612,845,2,889,353,643,595,744,983,321,965,136,395,531,357,790,814,682,879,226,958,708,652,882,917,360,284,898,583,537,422,166,243,93,617,390,211,287,935,788,690,509,539,487,322,785,728,771,516,984,334,452,429,306,881,330,800,966,327,435,308,343,320,638,880,955,731,956,378,735,569,213,196,111,552,336,1016,750,456,69,892,31,235,105,985,507,5,124,224,799,893,476,482,794,925,810,789,821,118,172,561,477,478,685,952,628,709,675,397,442,665,276,295,557,104,449,296,21,4,584,331,975,591,512,9,755,887,980,870,982,541,730,288,705,837,371,242,279,1007,912,154,114,179,710,273,87,462,576,553,723,871,973,147,712,668,228,501,246,107,413,204,719,780,804,481,580,370,121,130,948,842,323,997,96,259,589,421,635,601,763,598,167,27,826,220,438,127,94,44,283,538,298,249,210,451,374,466,490,180,726,61,772,68,153,594,544,921,581,41,450,762,704,883,515,724,132,408,782,486,683,37,341,791,820,953,911,201,686,444,345,681,862,187,469,169,16,520,489,779,282,1012,258,949,777,112,913,608,691,770,998,867,170,623,857,641,3,484,835,8,556,165,221,716,1000,938,662,206,248,962,540,310,734,1002,40,177,405,151,472,825,620,521,14,524,640,386,813,1006,940,692,796,852,1005,281,59,990,713,715,680,960};
        int batch = 100;
        int windowSize = 100;
        int currentSize = 0;
        int deleteIndex = 0;
        int insertIndex = 0;
        ids_length = ids.size();
        for (int i = 0; i < ids_length; i += batch) {
            // add new batch
            printf("Insert %d/%d\n", i, ids_length);
            while (insertIndex < ids_length && insertIndex < i+batch) {
                printf("%d,", ids[insertIndex]);
                Bid k = ids[insertIndex];
                string val = "test_" + to_string(ids[insertIndex]);
                ecall_write_node(global_eid, (const char*) k.id.data(), val.c_str());
                currentSize++;
                insertIndex++;
            }
            printf("\n\n");
            // delete if exceeds
            if (currentSize > windowSize) {
                printf("delete...\n");
                for (int i = deleteIndex; i < ids_length && i < deleteIndex+batch; i++) {
                    Bid k = ids[i];
                    printf("%d,", ids[i]);
                    ecall_delete_node(global_eid, (const char*) k.id.data());
                }
                printf("\n\n");
                deleteIndex += batch;
                currentSize -= batch;
            }
            printf("currentSize=%d, insertIndex=%d, deleteIndex=%d\n", currentSize, insertIndex, deleteIndex);
        }
        sgx_destroy_enclave(global_eid);
        return 0;
    }

//    for (int i = 0; i < ids_length; i++) {
//        Bid k = ids[i];
//        string val = "test_" + to_string(ids[i]);
//        ecall_write_node(global_eid, (const char*) k.id.data(), val.c_str());
//    }

    char* val = new char[16];

//    ecall_read_node(global_eid, (const char*) kk.id.data(), val);
//    assert(strcmp(val, "test_6969") == 0);
//    cout<<"Setup was Successful: "<< val << endl;

    it = 0;
    while(0) {
        cout << "Iteration " << it << endl;
        id = rand() % (maxSize/2) + 1;
        cout<<"Delete node " << id << endl;
        Bid k = id;
        ecall_delete_node(global_eid, (const char*) k.id.data());
        cout<<"********************"<<endl;
        ecall_print_tree(global_eid);
        cout<<"********************"<<endl;
        int res_size = preorder_after_deletion.size();
        long long *keys = new long long[res_size];
        ecall_tree_preorder_keys(global_eid, keys, res_size);
        for (int i = 0; i < res_size; i++) {
            assert(keys[i] == preorder_after_deletion[i]);
        }
        id = rand() % (maxSize/2) + 1;
        cout<<"Write node " << id << endl;
        Bid kk = id;
        string val = "test_" + to_string(id);
        ecall_write_node(global_eid, (const char*) kk.id.data(), val.c_str());
        ecall_print_tree(global_eid);
        cout<<"********************"<<endl;
        it++;
    }


    //******************************************************************************
    //******************************************************************************






    /* Destroy the enclave */
    //------------------------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------
    sgx_destroy_enclave(global_eid);
    delete[] val;

    return 0;
}

