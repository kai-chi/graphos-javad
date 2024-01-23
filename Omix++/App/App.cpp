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
    if (argc == 3) {
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
        default:
            break;
    }

    if (true) {
        ecall_measure_omap_speed(global_eid, &t, maxSize);
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

    for (int i = 0; i < ids_length; i++) {
        Bid k = ids[i];
        string val = "test_" + to_string(ids[i]);
        ecall_write_node(global_eid, (const char*) k.id.data(), val.c_str());
    }

    char* val = new char[16];

//    ecall_read_node(global_eid, (const char*) kk.id.data(), val);
//    assert(strcmp(val, "test_6969") == 0);
//    cout<<"Setup was Successful: "<< val << endl;
    cout<<"********************"<<endl;
    ecall_print_tree(global_eid);
    cout<<"********************"<<endl;
    Bid k = id;
    ecall_read_node(global_eid, (const char*) k.id.data(), val);
    cout <<"Read " << id << ": " << val << endl;
    cout<<"Delete node " << id << endl;
    ecall_delete_node(global_eid, (const char*) k.id.data());
    cout<<"********************"<<endl;
    ecall_print_tree(global_eid);
    cout<<"********************"<<endl;
    ecall_read_node(global_eid, (const char*) k.id.data(), val);
    cout <<"Read " << id << ": " << val << endl;

    long long *keys = new long long[ids_length-1];
    ecall_tree_preorder_keys(global_eid, keys, ids_length-1);
    cout<<"********************"<<endl;
    cout << "Preorder keys: ";
    for (int i = 0; i < ids_length - 1; i++) {
        cout << keys[i] << ", ";
    }
    cout << endl;
    for (int i = 0; i < ids_length - 1; i++) {
        assert(keys[i] == preorder_after_deletion[i]);
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
    delete val;
    delete keys;




    return 0;
}

