#include "AVLTree.h"
#include "Enclave.h"

void check_memory2(string text) {
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

AVLTree::AVLTree(long long maxSize, bytes<Key> secretkey, bool isEmptyMap) {
    oram = new ORAM(maxSize, secretkey, false, isEmptyMap);
    int depth = (int) (ceil(log2(maxSize)) - 1) + 1;
    maxOfRandom = (long long) (pow(2, depth));
    times.push_back(vector<double>());
    times.push_back(vector<double>());
    times.push_back(vector<double>());
    times.push_back(vector<double>());
}

AVLTree::~AVLTree() {
    delete oram;
}

// A utility function to get maximum of two integers

int AVLTree::max(int a, int b) {
    int res = CTcmp(a, b);
    return CTeq(res, 1) ? a : b;
}

Node* AVLTree::newNode(Bid omapKey, string value) {
    Node* node = new Node();
    node->key = omapKey;
    node->index = index++;
    std::fill(node->value.begin(), node->value.end(), 0);
    std::copy(value.begin(), value.end(), node->value.begin());
    node->leftID = 0;
    node->leftPos = -1;
    node->rightPos = -1;
    node->rightID = 0;
    node->pos = 0;
    node->isDummy = false;
    node->height = 1; // new node is initially added at leaf
    return node;
}

// A utility function to right rotate subtree rooted with leftNode
// See the diagram given above.

void AVLTree::rotate(Node* node, Node* oppositeNode, int targetHeight, bool right, bool dummyOp) {
    Node* T2 = nullptr;
    Node* tmpDummyNode = new Node();
    tmpDummyNode->isDummy = true;
    T2 = newNode(0, "");
    Node* tmp = nullptr;
    bool left = !right;
    unsigned long long newPos = RandomPath();
    unsigned long long dumyPos;
    Bid dummy;
    dummy.setValue(oram->nextDummyCounter++);

    bool isRealRotation = !dummyOp;
    // is real and valid rotation
    bool isRealValidRotation = !dummyOp && ((right && oppositeNode->rightID.isZero()) || (left && oppositeNode->leftID.isZero()));
    // is real right rotation
    bool isRealRightRotation = !dummyOp && right;
    // is real left rotation
    bool isRealLeftRotation = !dummyOp && left;


    T2->height = Node::conditional_select(0, T2->height, isRealValidRotation);

    Bid readKey = dummy;
    readKey = Bid::conditional_select(oppositeNode->rightID, readKey, !isRealValidRotation && isRealRightRotation);
    readKey = Bid::conditional_select(oppositeNode->leftID, readKey, !isRealValidRotation && !isRealRightRotation && isRealLeftRotation);

    bool isDummy = !(!isRealValidRotation && (isRealRightRotation || isRealLeftRotation));

    tmp = readWriteCacheNode(readKey, tmpDummyNode, true, isDummy); //READ

    Node::conditional_assign(T2, tmp, !isRealValidRotation && (isRealRightRotation || isRealLeftRotation));
    delete tmp;

    // Perform rotation
//    bool cond1 = !dummyOp && right;
//    bool cond2 = !dummyOp;
    oppositeNode->rightID = Bid::conditional_select(node->key, oppositeNode->rightID, isRealRightRotation);
    oppositeNode->rightPos = Bid::conditional_select(node->pos, oppositeNode->rightPos, isRealRightRotation);
    node->leftID = Bid::conditional_select(T2->key, node->leftID, isRealRightRotation);
    node->leftPos = Bid::conditional_select(T2->pos, node->leftPos, isRealRightRotation);

    oppositeNode->leftID = Bid::conditional_select(node->key, oppositeNode->leftID, !isRealRightRotation && isRealRotation);
    oppositeNode->leftPos = Bid::conditional_select(node->pos, oppositeNode->leftPos, !isRealRightRotation && isRealRotation);
    node->rightID = Bid::conditional_select(T2->key, node->rightID, !isRealRightRotation && isRealRotation);
    node->rightPos = Bid::conditional_select(T2->pos, node->rightPos, !isRealRightRotation && isRealRotation);


    // Update heights    
    int curNodeHeight = max(T2->height, targetHeight) + 1;
    node->height = Node::conditional_select(curNodeHeight, node->height, !dummyOp);

    int oppositeOppositeHeight = 0;
    unsigned long long newOppositePos = RandomPath();
    Node* oppositeOppositeNode = nullptr;

    bool cond1 = isRealRightRotation && !oppositeNode->leftID.isZero();
    bool cond2 = isRealLeftRotation && !oppositeNode->rightID.isZero();

    readKey = dummy;
    readKey = Bid::conditional_select(oppositeNode->leftID, readKey, cond1);
    readKey = Bid::conditional_select(oppositeNode->rightID, readKey, !cond1 && cond2);

    isDummy = !(cond1 || cond2);

    oppositeOppositeNode = readWriteCacheNode(readKey, tmpDummyNode, true, isDummy); //READ

    oppositeOppositeHeight = Node::conditional_select(oppositeOppositeNode->height, oppositeOppositeHeight, cond1 || cond2);
    delete oppositeOppositeNode;


    int maxValue = max(oppositeOppositeHeight, curNodeHeight) + 1;
    oppositeNode->height = Node::conditional_select(maxValue, oppositeNode->height, !dummyOp);

    delete T2;
    delete tmpDummyNode;
}

void AVLTree::rotate2(Node* node, Node* oppositeNode, int targetHeight, bool right, bool dummyOp) {
    Node* T2 = nullptr;
    Node* tmpDummyNode = new Node();
    tmpDummyNode->isDummy = true;
    T2 = newNode(0, "");
    Node* tmp = nullptr;
    bool left = !right;
    unsigned long long newPos = RandomPath();
    unsigned long long dumyPos;
    Bid dummy;
    dummy.setValue(oram->nextDummyCounter++);

    if (!dummyOp && ((right && oppositeNode->rightID == 0) || (left && oppositeNode->leftID == 0))) {
        T2->height = 0;
        readWriteCacheNode(dummy, tmpDummyNode, true, true);
    } else if (!dummyOp && right) {
        T2 = readWriteCacheNode(oppositeNode->rightID, tmpDummyNode, true, false);
    } else if (!dummyOp && left) {
        T2 = readWriteCacheNode(oppositeNode->leftID, tmpDummyNode, true, false);
    } else {
        readWriteCacheNode(dummy, tmpDummyNode, true, true);
    }
    // Perform rotation
    if (!dummyOp && right) {
        oppositeNode->rightID = node->key;
        oppositeNode->rightPos = node->pos;
        oppositeNode->leftID = oppositeNode->leftID;
        oppositeNode->leftPos = oppositeNode->leftPos;
        node->rightID = node->rightID;
        node->rightPos = node->rightPos;
        node->leftID = T2->key;
        node->leftPos = T2->pos;
    } else if (!dummyOp) {
        oppositeNode->leftID = node->key;
        oppositeNode->leftPos = node->pos;
        oppositeNode->rightID = oppositeNode->rightID;
        oppositeNode->rightPos = oppositeNode->rightPos;
        node->rightID = T2->key;
        node->rightPos = T2->pos;
        node->leftID = node->leftID;
        node->leftPos = node->leftPos;
    } else {
        oppositeNode->leftID = oppositeNode->leftID;
        oppositeNode->leftPos = oppositeNode->leftPos;
        oppositeNode->rightID = oppositeNode->rightID;
        oppositeNode->rightPos = oppositeNode->rightPos;
        node->rightID = node->rightID;
        node->rightPos = node->rightPos;
        node->leftID = node->leftID;
        node->leftPos = node->leftPos;
    }
    // Update heights    
    int curNodeHeight = max(T2->height, targetHeight) + 1;
    if (!dummyOp) {
        node->height = curNodeHeight;
    } else {
        node->height = node->height;
    }

    int oppositeOppositeHeight = 0;
    unsigned long long newOppositePos = RandomPath();
    Node* oppositeOppositeNode = nullptr;
    if (!dummyOp && right && oppositeNode->leftID != 0) {
        oppositeOppositeNode = readWriteCacheNode(oppositeNode->leftID, tmpDummyNode, true, false);
        oppositeOppositeHeight = oppositeOppositeNode->height;
        delete oppositeOppositeNode;
    } else if (!dummyOp && left && oppositeNode->rightID != 0) {
        oppositeOppositeNode = readWriteCacheNode(oppositeNode->rightID, tmpDummyNode, true, false);
        oppositeOppositeHeight = oppositeOppositeNode->height;
        delete oppositeOppositeNode;
    } else {
        oppositeOppositeNode = readWriteCacheNode(dummy, tmpDummyNode, true, true);
        oppositeOppositeHeight = oppositeOppositeHeight;
        delete oppositeOppositeNode;
    }
    int maxValue = max(oppositeOppositeHeight, curNodeHeight) + 1;
    if (!dummyOp) {
        oppositeNode->height = maxValue;
    } else {
        oppositeNode->height = oppositeNode->height;
    }
    delete T2;
    delete tmpDummyNode;
}

Bid AVLTree::insert(Bid rootKey, unsigned long long& rootPos, Bid omapKey, string value, int& height, Bid lastID, bool isDummyIns) {
    totheight++;
    unsigned long long rndPos = RandomPath();
    std::array< byte_t, 16> tmpval;
    std::fill(tmpval.begin(), tmpval.end(), 0);
    std::copy(value.begin(), value.end(), tmpval.begin());
    Node* tmpDummyNode = new Node();
    tmpDummyNode->isDummy = true;
    Bid dummy;
    Bid retKey;
    unsigned long long dumyPos;
    bool remainerIsDummy = false;
    dummy.setValue(oram->nextDummyCounter++);

    if (isDummyIns && CTeq(CTcmp(totheight, oram->depth * 1.44), 1)) {
        Node* nnode = newNode(omapKey, value);
        nnode->pos = RandomPath();
        height = Node::conditional_select(nnode->height, height, !exist);
        rootPos = Node::conditional_select(nnode->pos, rootPos, !exist);

        Node* previousNode = readWriteCacheNode(omapKey, tmpDummyNode, true, !exist);

        Node* wrtNode = Node::clone(previousNode);
        Node::conditional_assign(wrtNode, nnode, !exist);

        unsigned long long lastPos = nnode->pos;
        lastPos = Node::conditional_select(previousNode->pos, lastPos, exist);

        Node* tmp = oram->ReadWrite(omapKey, wrtNode, lastPos, nnode->pos, false, false, false);

        wrtNode->pos = nnode->pos;
        Node* tmp2 = readWriteCacheNode(omapKey, wrtNode, false, false);
        Bid retKey = lastID;
        retKey = Bid::conditional_select(nnode->key, retKey, !exist);
        delete tmp;
        delete tmp2;
        delete wrtNode;
        delete nnode;
        delete previousNode;
        delete tmpDummyNode;
        return retKey;
    }
    /* 1. Perform the normal BST rotation */
    double t;
    bool cond1, cond2, cond3, cond4, cond5, isDummy;

    // only true if insert real and rootKey not zero
    cond1 = !isDummyIns && !rootKey.isZero();
    // true if insert dummy or rootKey zero
    remainerIsDummy = !cond1;

    Node* node = nullptr;

    Bid initReadKey = dummy;
    // select either the rootKey if insert real and rooKey not zero, otherwise a dummy Bid
    initReadKey = Bid::conditional_select(rootKey, initReadKey, !remainerIsDummy);
    isDummy = remainerIsDummy;
    // Read, overwrite if rootKey = omapKey,
    node = oram->ReadWrite(initReadKey, tmpDummyNode, rootPos, rootPos, true, isDummy, tmpval, Bid::CTeq(Bid::CTcmp(rootKey, omapKey), 0), true); //READ
    // Write to cache but what does it really return?
    Node* tmp2 = readWriteCacheNode(initReadKey, node, false, isDummy);
    delete tmp2;

    int balance = -1;
    int leftHeight = -1;
    int rightHeight = -1;
    Node* leftNode = new Node();
    bool leftNodeisNull = true;
    Node* rightNode = new Node();
    bool rightNodeisNull = true;
    std::array< byte_t, 16> garbage;
    bool childDirisLeft = false;


    unsigned long long newRLPos = RandomPath();

    // true if insert real and rootKey not zero
    cond1 = !remainerIsDummy;
    // check if omapKey was found in the ORAM
    int cmpRes = Bid::CTcmp(omapKey, node->key);
    // check if omapKey smaller than the last found key
    cond2 = Node::CTeq(cmpRes, -1);
    bool cond2_1 = node->rightID.isZero();
    // check if omapKey larger than the last found key
    cond3 = Node::CTeq(cmpRes, 1);
    bool cond3_1 = node->leftID.isZero();
    // check if omapKey is equal to last found key
    cond4 = Node::CTeq(cmpRes, 0);

    Bid insRootKey = rootKey;
    // if insert real and omapKey smaller than node
    insRootKey = Bid::conditional_select(node->leftID, insRootKey, cond1 && cond2);
    // if insert real and omapKey larger than node
    insRootKey = Bid::conditional_select(node->rightID, insRootKey, cond1 && cond3);

    unsigned long long insRootPos = rootPos;
    insRootPos = Node::conditional_select(node->leftPos, insRootPos, cond1 && cond2);
    insRootPos = Node::conditional_select(node->rightPos, insRootPos, cond1 && cond3);

    int insHeight = height;
    insHeight = Node::conditional_select(leftHeight, insHeight, cond1 && cond2);
    insHeight = Node::conditional_select(rightHeight, insHeight, cond1 && cond3);

    Bid insLastID = dummy;
    // if (real and omapKey equal to node) or it's a dummy insert
    insLastID = Bid::conditional_select(node->key, insLastID, (cond1 && cond4) || (!cond1));

    // true if dummy insert or a real insert but the key was found in ORAM
    isDummy = (!cond1) || (cond1 && cond4);

    // exist = false when starting the operation
    // if real insert and omapKey found in ORAM
    exist = Node::conditional_select(true, exist, cond1 && cond4);
    Bid resValBid = insert(insRootKey, insRootPos, omapKey, value, insHeight, insLastID, isDummy);



    node->leftID = Bid::conditional_select(resValBid, node->leftID, cond1 && cond2);
    node->rightID = Bid::conditional_select(resValBid, node->rightID, cond1 && cond3);

    node->leftPos = Node::conditional_select(insRootPos, node->leftPos, cond1 && cond2);
    node->rightPos = Node::conditional_select(insRootPos, node->rightPos, cond1 && cond3);
    rootPos = Node::conditional_select(insRootPos, rootPos, (!cond1) || (cond1 && cond4));

    leftHeight = Node::conditional_select(insHeight, leftHeight, cond1 && cond2);
    rightHeight = Node::conditional_select(insHeight, rightHeight, cond1 && cond3);
    height = Node::conditional_select(insHeight, height, (!cond1) || (cond1 && cond4));

    totheight--;
    childDirisLeft = (cond1 && cond2);

    Bid readKey = dummy;
    readKey = Bid::conditional_select(node->rightID, readKey, cond1 && cond2 && (!cond2_1));
    readKey = Bid::conditional_select(node->leftID, readKey, cond1 && cond3 && (!cond3_1));

    unsigned long long tmpreadPos = newRLPos;
    tmpreadPos = Bid::conditional_select(node->rightPos, tmpreadPos, cond1 && cond2 && (!cond2_1));
    tmpreadPos = Bid::conditional_select(node->leftPos, tmpreadPos, cond1 && cond3 && (!cond3_1));

    isDummy = !((cond1 && cond2 && (!cond2_1)) || (cond1 && cond3 && (!cond3_1)));

    Node* tmp = oram->ReadWrite(readKey, tmpDummyNode, tmpreadPos, tmpreadPos, true, isDummy, true); //READ
    Node::conditional_assign(rightNode, tmp, cond1 && cond2 && (!cond2_1));
    Node::conditional_assign(leftNode, tmp, cond1 && cond3 && (!cond3_1));

    tmp2 = readWriteCacheNode(tmp->key, tmp, false, isDummy);

    delete tmp;
    delete tmp2;

    tmp = readWriteCacheNode(node->key, tmpDummyNode, true, !(cond1 && cond4));

    Node::conditional_assign(node, tmp, cond1 && cond4);

    delete tmp;

    rightNodeisNull = Node::conditional_select(false, rightNodeisNull, cond1 && cond2 && (!cond2_1));
    leftNodeisNull = Node::conditional_select(false, leftNodeisNull, cond1 && cond3 && (!cond3_1));

    rightHeight = Node::conditional_select(0, rightHeight, cond1 && cond2 && cond2_1);
    rightHeight = Node::conditional_select(rightNode->height, rightHeight, cond1 && cond2 && (!cond2_1));
    leftHeight = Node::conditional_select(0, leftHeight, cond1 && cond3 && cond3_1);
    leftHeight = Node::conditional_select(leftNode->height, leftHeight, cond1 && cond3 && (!cond3_1));

    std::fill(garbage.begin(), garbage.end(), 0);
    std::copy(value.begin(), value.end(), garbage.begin());

    for (int i = 0; i < 16; i++) {
        node->value[i] = Bid::conditional_select(garbage[i], node->value[i], cond1 && cond4);
    }

    rootPos = Node::conditional_select(node->pos, rootPos, cond1 && cond4);
    height = Node::conditional_select(node->height, height, cond1 && cond4);

    retKey = Bid::conditional_select(node->key, retKey, cond1 && cond4);
    retKey = Bid::conditional_select(resValBid, retKey, !cond1);

    remainerIsDummy = Node::conditional_select(true, remainerIsDummy, cond1 && cond4);
    remainerIsDummy = Node::conditional_select(false, remainerIsDummy, cond1 && (cond2 || cond3));

    /* 2. Update height of this ancestor node */

    int tmpHeight = max(leftHeight, rightHeight) + 1;
    node->height = Node::conditional_select(node->height, tmpHeight, remainerIsDummy);
    height = Node::conditional_select(height, node->height, remainerIsDummy);

    /* 3. Get the balance factor of this ancestor node to check whether this node became unbalanced */

    balance = Node::conditional_select(balance, leftHeight - rightHeight, remainerIsDummy);

    ocall_start_timer(totheight + 100000);
    //------------------------------------------------

    // if is real AND left subtree too high AND omapKey smaller than its ancestor - Left Left Case
    cond1 = !remainerIsDummy && CTeq(CTcmp(balance, 1), 1) && Node::CTeq(Bid::CTcmp(omapKey, node->leftID), -1);
    // if is real AND right subtree too high AND omapKey larger than its ancestor - Right Right Case
    cond2 = !remainerIsDummy && CTeq(CTcmp(balance, -1), -1) && Node::CTeq(Bid::CTcmp(omapKey, node->rightID), 1);
    // if is real AND left subtree too high AND omaKey larger than its ancestor - Left Right Case
    cond3 = !remainerIsDummy && CTeq(CTcmp(balance, 1), 1) && Node::CTeq(Bid::CTcmp(omapKey, node->leftID), 1);
    // if is real AND right tree too high AND omapKey smaller than its ancestor - Right Left Case
    cond4 = !remainerIsDummy && CTeq(CTcmp(balance, -1), -1) && Node::CTeq(Bid::CTcmp(omapKey, node->rightID), -1);
    // if not real
    cond5 = !remainerIsDummy;

    //  printf("tot:%d cond1:%d cond2:%d cond3:%d cond4:%d cond5:%d\n",totheight,cond1,cond2,cond3,cond4,cond5);

    // left tree too high 
    bool leftTreeTooHigh = (cond1 || (!cond1 && !cond2 && cond3)) && leftNodeisNull;
    
    // right tree too high AND omapKey smaller than its ancestors
    bool lrCond2 = (!cond1 & !cond2 && !cond3 && cond4);
    
    // right tree too high AND rightNode is null
    bool rightTreeTooHigh = ((!cond1 && cond2) || lrCond2) && rightNodeisNull;

    // left tree too high and key larger than ancestor
    bool lrrlCond1 = !cond1 && !cond2 && cond3;

    // right tree too high AND key smaller than ancestor -- isRightRotate
    bool lrrlCond2 = !cond1 && !cond2 && !cond3 && cond4;

    // is the tree imbalanced in any way?
    bool rotCond1 = cond1 || cond2 || cond3 || cond4;

    // right tree too high AND key larger than ancestor
    bool secondRotateCond1 = !cond1 && cond2;

    //------------------------------------------------
    // Left/Right Node - either left rotation or right rotation
    //------------------------------------------------

    Bid leftRightReadKey = dummy;

    leftRightReadKey = Bid::conditional_select(node->leftID, leftRightReadKey, leftTreeTooHigh);
    leftRightReadKey = Bid::conditional_select(node->rightID, leftRightReadKey, rightTreeTooHigh);

    // the tree is balanced - can we not simply check the balance variable?
    isDummy = !(leftTreeTooHigh || rightTreeTooHigh);

    tmp = readWriteCacheNode(leftRightReadKey, tmpDummyNode, true, isDummy); //READ
    Node::conditional_assign(leftNode, tmp, leftTreeTooHigh);
    Node::conditional_assign(rightNode, tmp, rightTreeTooHigh);
    delete tmp;

    node->leftPos = Node::conditional_select(leftNode->pos, node->leftPos, leftTreeTooHigh);
    leftNodeisNull = Node::conditional_select(false, leftNodeisNull, leftTreeTooHigh);

    node->rightPos = Node::conditional_select(rightNode->pos, node->rightPos, rightTreeTooHigh);
    rightNodeisNull = Node::conditional_select(false, rightNodeisNull, rightTreeTooHigh);
    //------------------------------------------------
    // Left-Right Right-Left Node - if we need the second rotation
    //------------------------------------------------

    Bid leftRightNodeReadKey = dummy;
    leftRightNodeReadKey = Bid::conditional_select(leftNode->rightID, leftRightNodeReadKey, lrrlCond1);
    leftRightNodeReadKey = Bid::conditional_select(rightNode->leftID, leftRightNodeReadKey, lrrlCond2);

    isDummy = !(lrrlCond1 || lrrlCond2);

    tmp = readWriteCacheNode(leftRightNodeReadKey, tmpDummyNode, true, isDummy); //READ

    Node* leftRightNode = new Node();
    Node* rightLeftNode = new Node();

    Node::conditional_assign(leftRightNode, tmp, lrrlCond1);
    Node::conditional_assign(rightLeftNode, tmp, lrrlCond2);

    delete tmp;

    int leftLeftHeight = 0;
    int rightRightHeight = 0;

    Bid leftLeftRightRightKey = dummy;
    leftLeftRightRightKey = Bid::conditional_select(leftNode->leftID, leftLeftRightRightKey, lrrlCond1 && !leftNode->leftID.isZero());
    leftLeftRightRightKey = Bid::conditional_select(rightNode->rightID, leftLeftRightRightKey, lrrlCond2 && !rightNode->rightID.isZero());

    isDummy = !(((lrrlCond1) && !leftNode->leftID.isZero()) || (lrrlCond2 && !rightNode->rightID.isZero()));

    tmp = readWriteCacheNode(leftLeftRightRightKey, tmpDummyNode, true, isDummy); //READ

    Node* leftLeftNode = new Node();
    Node* rightRightNode = new Node();

    Node::conditional_assign(leftLeftNode, tmp, lrrlCond1 && !leftNode->leftID.isZero());
    Node::conditional_assign(rightRightNode, tmp, lrrlCond2 && !rightNode->rightID.isZero());
    delete tmp;

    leftLeftHeight = Node::conditional_select(leftLeftNode->height, leftLeftHeight, lrrlCond1 && !leftNode->leftID.isZero());
    rightRightHeight = Node::conditional_select(rightRightNode->height, rightRightHeight, lrrlCond2 && !rightNode->rightID.isZero());
    delete leftLeftNode;
    delete rightRightNode;

    //------------------------------------------------
    // Rotate
    //------------------------------------------------

    Node* targetRotateNode = Node::clone(tmpDummyNode);
    Node* oppositeRotateNode = Node::clone(tmpDummyNode);

    Node::conditional_assign(targetRotateNode, leftNode, lrrlCond1);
    Node::conditional_assign(targetRotateNode, rightNode, lrrlCond2);

    Node::conditional_assign(oppositeRotateNode, leftRightNode, lrrlCond1);
    Node::conditional_assign(oppositeRotateNode, rightLeftNode, lrrlCond2);

    int rotateHeight = 0;
    rotateHeight = Node::conditional_select(leftLeftHeight, rotateHeight, lrrlCond1);
    rotateHeight = Node::conditional_select(rightRightHeight, rotateHeight, lrrlCond2);

    bool isRightRotate = lrrlCond2;
    bool isDummyRotate = !(lrrlCond1 || lrrlCond2);

    rotate(targetRotateNode, oppositeRotateNode, rotateHeight, isRightRotate, isDummyRotate);

    Node::conditional_assign(leftNode, targetRotateNode, lrrlCond1);
    Node::conditional_assign(rightNode, targetRotateNode, lrrlCond2);

    delete targetRotateNode;

    Node::conditional_assign(leftRightNode, oppositeRotateNode, lrrlCond1);
    Node::conditional_assign(rightLeftNode, oppositeRotateNode, lrrlCond2);

    delete oppositeRotateNode;


    unsigned long long newP = RandomPath();
    unsigned long long oldLeftRightPos = RandomPath();
    oldLeftRightPos = Node::conditional_select(leftNode->pos, oldLeftRightPos, lrrlCond1);
    oldLeftRightPos = Node::conditional_select(rightNode->pos, oldLeftRightPos, lrrlCond2);

    leftNode->pos = Node::conditional_select(newP, leftNode->pos, lrrlCond1);
    rightNode->pos = Node::conditional_select(newP, rightNode->pos, lrrlCond2);

    Bid firstRotateWriteKey = dummy;
    firstRotateWriteKey = Bid::conditional_select(leftNode->key, firstRotateWriteKey, lrrlCond1);
    firstRotateWriteKey = Bid::conditional_select(rightNode->key, firstRotateWriteKey, lrrlCond2);

    Node* firstRotateWriteNode = Node::clone(tmpDummyNode);
    Node::conditional_assign(firstRotateWriteNode, leftNode, lrrlCond1);
    Node::conditional_assign(firstRotateWriteNode, rightNode, lrrlCond2);

    isDummy = !(lrrlCond1 || lrrlCond2);
    tmp = readWriteCacheNode(firstRotateWriteKey, firstRotateWriteNode, false, isDummy); //WRITE
    delete tmp;
    delete firstRotateWriteNode;

    leftRightNode->leftPos = Node::conditional_select(newP, leftRightNode->leftPos, lrrlCond1);
    rightLeftNode->rightPos = Node::conditional_select(newP, rightLeftNode->rightPos, lrrlCond2);

    node->leftID = Bid::conditional_select(leftRightNode->key, node->leftID, lrrlCond1);
    node->leftPos = Node::conditional_select(leftRightNode->pos, node->leftPos, lrrlCond1);

    node->rightID = Bid::conditional_select(rightLeftNode->key, node->rightID, lrrlCond2);
    node->rightPos = Node::conditional_select(rightLeftNode->pos, node->rightPos, lrrlCond2);


    //------------------------------------------------
    // Second Rotate
    //------------------------------------------------

    targetRotateNode = Node::clone(tmpDummyNode);

    Node::conditional_assign(targetRotateNode, node, rotCond1);

    oppositeRotateNode = Node::clone(tmpDummyNode);
    Node::conditional_assign(oppositeRotateNode, leftNode, cond1);
    Node::conditional_assign(oppositeRotateNode, rightNode, secondRotateCond1);
    Node::conditional_assign(oppositeRotateNode, leftRightNode, lrrlCond1);
    Node::conditional_assign(oppositeRotateNode, rightLeftNode, lrrlCond2);

    rotateHeight = 0;
    rotateHeight = Node::conditional_select(rightHeight, rotateHeight, cond1 || lrrlCond1);
    rotateHeight = Node::conditional_select(leftHeight, rotateHeight, secondRotateCond1 || lrrlCond2);

    isRightRotate = cond1 || lrrlCond1;
    isDummyRotate = !(rotCond1);

    rotate(targetRotateNode, oppositeRotateNode, rotateHeight, isRightRotate, isDummyRotate);

    Node::conditional_assign(node, targetRotateNode, rotCond1);

    delete targetRotateNode;

    Node::conditional_assign(leftNode, oppositeRotateNode, cond1);
    Node::conditional_assign(rightNode, oppositeRotateNode, secondRotateCond1);
    Node::conditional_assign(leftRightNode, oppositeRotateNode, lrrlCond1);
    Node::conditional_assign(rightLeftNode, oppositeRotateNode, lrrlCond2);

    delete oppositeRotateNode;

    //------------------------------------------------
    // Last Two Writes
    //------------------------------------------------

    newP = RandomPath();

    bool lastTwoWriteCond1 = !cond1 && !cond2 && !cond3 && !cond4;
    bool lastTwoWriteCond2 = lastTwoWriteCond1 && doubleRotation && childDirisLeft;
    bool lastTwoWriteCond3 = lastTwoWriteCond1 && doubleRotation && !childDirisLeft;
    bool lastWriteCond1 = lastTwoWriteCond1 && cond5;

    Bid firstReadCache = dummy;
    firstReadCache = Bid::conditional_select(node->leftID, firstReadCache, lastTwoWriteCond2);
    firstReadCache = Bid::conditional_select(node->rightID, firstReadCache, lastTwoWriteCond3);

    isDummy = !(lastTwoWriteCond1 && doubleRotation);
    tmp = readWriteCacheNode(firstReadCache, tmpDummyNode, true, isDummy);

    Node::conditional_assign(leftNode, tmp, lastTwoWriteCond2);
    Node::conditional_assign(rightNode, tmp, lastTwoWriteCond3);

    delete tmp;

    Bid finalFirstWriteKey = dummy;
    finalFirstWriteKey = Bid::conditional_select(node->key, finalFirstWriteKey, cond1 || secondRotateCond1);
    finalFirstWriteKey = Bid::conditional_select(leftNode->key, finalFirstWriteKey, lrrlCond1 || lastTwoWriteCond2);
    finalFirstWriteKey = Bid::conditional_select(rightNode->key, finalFirstWriteKey, lrrlCond2 || lastTwoWriteCond3);

    Node* finalFirstWriteNode = Node::clone(tmpDummyNode);
    Node::conditional_assign(finalFirstWriteNode, node, cond1 || secondRotateCond1);
    Node::conditional_assign(finalFirstWriteNode, leftNode, lrrlCond1 || lastTwoWriteCond2);
    Node::conditional_assign(finalFirstWriteNode, rightNode, lrrlCond2 || lastTwoWriteCond3);


    unsigned long long finalFirstWritePos = dumyPos;
    finalFirstWritePos = Node::conditional_select(node->pos, finalFirstWritePos, cond1 || secondRotateCond1);
    finalFirstWritePos = Node::conditional_select(oldLeftRightPos, finalFirstWritePos, lrrlCond1 || lrrlCond2);
    finalFirstWritePos = Node::conditional_select(leftNode->pos, finalFirstWritePos, lastTwoWriteCond2);
    finalFirstWritePos = Node::conditional_select(rightNode->pos, finalFirstWritePos, lastTwoWriteCond3);

    unsigned long long finalFirstWriteNewPos = dumyPos;
    finalFirstWriteNewPos = Node::conditional_select(newP, finalFirstWriteNewPos, cond1 || secondRotateCond1 || (lastTwoWriteCond1 && doubleRotation));
    finalFirstWriteNewPos = Node::conditional_select(leftNode->pos, finalFirstWriteNewPos, lrrlCond1);
    finalFirstWriteNewPos = Node::conditional_select(rightNode->pos, finalFirstWriteNewPos, lrrlCond2);

    isDummy = lastTwoWriteCond1 && !doubleRotation;

    tmp = oram->ReadWrite(finalFirstWriteKey, finalFirstWriteNode, finalFirstWritePos, finalFirstWriteNewPos, false, isDummy, false); //WRITE        

    delete tmp;
    delete finalFirstWriteNode;


    node->pos = Node::conditional_select(newP, node->pos, cond1 || secondRotateCond1);
    leftNode->rightPos = Node::conditional_select(newP, leftNode->rightPos, cond1);
    rightNode->leftPos = Node::conditional_select(newP, rightNode->leftPos, secondRotateCond1);
    leftNode->pos = Node::conditional_select(newP, leftNode->pos, lastTwoWriteCond2);
    rightNode->pos = Node::conditional_select(newP, rightNode->pos, lastTwoWriteCond3);
    node->leftPos = Node::conditional_select(newP, node->leftPos, lastTwoWriteCond2);
    node->rightPos = Node::conditional_select(newP, node->rightPos, lastTwoWriteCond3);



    Bid finalFirstWriteCacheWriteKey = dummy;
    finalFirstWriteCacheWriteKey = Bid::conditional_select(node->key, finalFirstWriteCacheWriteKey, cond1 || secondRotateCond1);
    finalFirstWriteCacheWriteKey = Bid::conditional_select(node->leftID, finalFirstWriteCacheWriteKey, lastTwoWriteCond2);
    finalFirstWriteCacheWriteKey = Bid::conditional_select(node->rightID, finalFirstWriteCacheWriteKey, lastTwoWriteCond3);

    Node* finalFirstWriteCacheWrite = Node::clone(tmpDummyNode);
    Node::conditional_assign(finalFirstWriteCacheWrite, node, cond1 || secondRotateCond1);
    Node::conditional_assign(finalFirstWriteCacheWrite, leftNode, lastTwoWriteCond2);
    Node::conditional_assign(finalFirstWriteCacheWrite, rightNode, lastTwoWriteCond3);

    isDummy = (lrrlCond1) || (lrrlCond2) || (lastTwoWriteCond1 && !doubleRotation);

    tmp = readWriteCacheNode(finalFirstWriteCacheWriteKey, finalFirstWriteCacheWrite, false, isDummy); //WRITE

    delete tmp;
    delete finalFirstWriteCacheWrite;

    doubleRotation = Node::conditional_select(false, doubleRotation, lastTwoWriteCond1 && doubleRotation);
    doubleRotation = Node::conditional_select(true, doubleRotation, lrrlCond1 || lrrlCond2);
    //------------------------------------------------
    // Last Write
    //------------------------------------------------

    newP = RandomPath();

    Bid finalWriteKey = dummy;
    finalWriteKey = Bid::conditional_select(leftNode->key, finalWriteKey, cond1);
    finalWriteKey = Bid::conditional_select(rightNode->key, finalWriteKey, secondRotateCond1);
    finalWriteKey = Bid::conditional_select(node->key, finalWriteKey, lrrlCond1);
    finalWriteKey = Bid::conditional_select(node->key, finalWriteKey, lrrlCond2);
    finalWriteKey = Bid::conditional_select(node->key, finalWriteKey, lastWriteCond1);


    unsigned long long finalWritePos = dumyPos;
    finalWritePos = Node::conditional_select(leftNode->pos, finalWritePos, cond1);
    finalWritePos = Node::conditional_select(rightNode->pos, finalWritePos, secondRotateCond1);
    finalWritePos = Node::conditional_select(node->pos, finalWritePos, lrrlCond1);
    finalWritePos = Node::conditional_select(node->pos, finalWritePos, lrrlCond2);
    finalWritePos = Node::conditional_select(node->pos, finalWritePos, lastWriteCond1);


    unsigned long long finalWriteNewPos = dumyPos;
    finalWriteNewPos = Node::conditional_select(newP, finalWriteNewPos, cond1 || secondRotateCond1 || lrrlCond1 || lrrlCond2 || lastWriteCond1);


    isDummy = !(rotCond1 || cond5);

    Node* finalWriteNode = Node::clone(tmpDummyNode);
    Node::conditional_assign(finalWriteNode, leftNode, cond1);
    Node::conditional_assign(finalWriteNode, rightNode, secondRotateCond1);
    Node::conditional_assign(finalWriteNode, node, lrrlCond1);
    Node::conditional_assign(finalWriteNode, node, lrrlCond2);
    Node::conditional_assign(finalWriteNode, node, lastWriteCond1);

    tmp = oram->ReadWrite(finalWriteKey, finalWriteNode, finalWritePos, finalWriteNewPos, false, isDummy, false); //WRITE
    delete tmp;
    delete finalWriteNode;


    leftNode->pos = Node::conditional_select(newP, leftNode->pos, cond1);
    rightNode->pos = Node::conditional_select(newP, rightNode->pos, secondRotateCond1);
    node->pos = Node::conditional_select(newP, node->pos, lrrlCond1 || lrrlCond2 || lastWriteCond1);

    Bid finalCacheWriteKey = dummy;
    finalCacheWriteKey = Bid::conditional_select(leftNode->key, finalCacheWriteKey, cond1);
    finalCacheWriteKey = Bid::conditional_select(rightNode->key, finalCacheWriteKey, secondRotateCond1);
    finalCacheWriteKey = Bid::conditional_select(node->key, finalCacheWriteKey, lrrlCond1 || lrrlCond2 || lastWriteCond1);

    Node* finalCacheWriteNode = Node::clone(tmpDummyNode);
    Node::conditional_assign(finalCacheWriteNode, leftNode, cond1);
    Node::conditional_assign(finalCacheWriteNode, rightNode, secondRotateCond1);
    Node::conditional_assign(finalCacheWriteNode, node, lrrlCond1 || lrrlCond2 || lastWriteCond1);

    isDummy = lastTwoWriteCond1 && !cond5;

    tmp = readWriteCacheNode(finalCacheWriteKey, finalCacheWriteNode, false, isDummy); //WRITE             

    delete tmp;
    delete finalCacheWriteNode;

    leftRightNode->rightPos = Node::conditional_select(newP, leftRightNode->rightPos, lrrlCond1);
    rightLeftNode->leftPos = Node::conditional_select(newP, rightLeftNode->leftPos, lrrlCond2);


    finalCacheWriteKey = dummy;
    finalCacheWriteKey = Bid::conditional_select(leftRightNode->key, finalCacheWriteKey, lrrlCond1);
    finalCacheWriteKey = Bid::conditional_select(rightLeftNode->key, finalCacheWriteKey, lrrlCond2);

    finalCacheWriteNode = Node::clone(tmpDummyNode);
    Node::conditional_assign(finalCacheWriteNode, leftRightNode, lrrlCond1);
    Node::conditional_assign(finalCacheWriteNode, rightLeftNode, lrrlCond2);

    isDummy = !(lrrlCond1 || lrrlCond2);

    tmp = readWriteCacheNode(finalCacheWriteKey, finalCacheWriteNode, false, isDummy); //WRITE             

    delete tmp;
    delete finalCacheWriteNode;



    rootPos = Node::conditional_select(leftNode->pos, rootPos, cond1);
    rootPos = Node::conditional_select(rightNode->pos, rootPos, secondRotateCond1);
    rootPos = Node::conditional_select(leftRightNode->pos, rootPos, lrrlCond1);
    rootPos = Node::conditional_select(rightLeftNode->pos, rootPos, lrrlCond2);
    rootPos = Node::conditional_select(node->pos, rootPos, lastWriteCond1);


    retKey = Bid::conditional_select(leftNode->key, retKey, cond1);
    retKey = Bid::conditional_select(rightNode->key, retKey, secondRotateCond1);
    retKey = Bid::conditional_select(leftRightNode->key, retKey, lrrlCond1);
    retKey = Bid::conditional_select(rightLeftNode->key, retKey, lrrlCond2);
    retKey = Bid::conditional_select(node->key, retKey, lastWriteCond1);

    height = Node::conditional_select(leftNode->height, height, cond1);
    height = Node::conditional_select(rightNode->height, height, secondRotateCond1);
    height = Node::conditional_select(leftRightNode->height, height, lrrlCond1);
    height = Node::conditional_select(rightLeftNode->height, height, lrrlCond2);

    if (totheight == 1) {
        isDummy = !doubleRotation;
        unsigned long long finalPos = RandomPath();
        Node* finalNode = readWriteCacheNode(retKey, tmpDummyNode, true, isDummy);
        tmp = oram->ReadWrite(finalNode->key, finalNode, finalNode->pos, finalPos, false, isDummy, false); //WRITE
        rootPos = Node::conditional_select(finalPos, rootPos, doubleRotation);
        height = Node::conditional_select(finalNode->height, height, doubleRotation);
        delete tmp;
        delete finalNode;
    }

    if (logTime) {
        ocall_stop_timer(&t, totheight + 100000);
        times[0][times[0].size() - 1] += t;
    }

    delete tmpDummyNode;
    delete node;
    delete leftNode;
    delete rightNode;
    delete leftRightNode;
    delete rightLeftNode;
    return retKey;

}

Node * AVLTree::minValueNode(Bid rootKey, unsigned long long& rootPos, bool isDummyOp) {
    Node* tmpDummyNode = new Node();
    tmpDummyNode->isDummy = true;
    Node *n = new Node();
    n->isDummy = true;
    bool remainderIsDummy = isDummyOp;
    Bid dummy;
    dummy.setValue(oram->nextDummyCounter++);
    unsigned long long dummyPos;
    int depth = 0;

    if (!remainderIsDummy) {
        n = oram->ReadWrite(rootKey, tmpDummyNode, rootPos, rootPos, true, false, false);
    } else {
        oram->ReadWrite(dummy, tmpDummyNode, dummyPos, dummyPos, true, true, true);
    }

    // while depth < oram->depth * 1.44
    while (CTeq(CTcmp(depth, (int) ((float) oram->depth * 1.44)), -1)) {
        if (!remainderIsDummy && !n->leftID.isZero()) {
            n = oram->ReadWrite(n->leftID, tmpDummyNode, n->leftPos, n->leftPos, true, false, false);
            if (n->leftID.isZero()) {
                remainderIsDummy = true;
            } else {
                remainderIsDummy = false;
            }
        } else {
            oram->ReadWrite(dummy, tmpDummyNode, dummyPos, dummyPos, true, true, true);
            remainderIsDummy = remainderIsDummy;
        }
        depth++;
    }
    delete tmpDummyNode;
#ifdef SGX_DEBUG
    printf("minValueNode after rootKey=%d found: %d\n", rootKey.getValue(), n->key.getValue());
#endif
    return n;
}

Bid AVLTree::deleteNode2(Bid rootKey, unsigned long long& rootPos, Bid parentKey, unsigned long long &parentPos, int parentRootRelation, Bid key, int &height, Bid lastID, int &children, int &depth, bool isDummyDel) {
    depth++;
    bool remainderIsDummy = false;
    Bid dummy;
    unsigned long long dummyPos;
    dummy.setValue(oram->nextDummyCounter++);
    int leftHeight = 0, rightHeight = 0, balance = 0;
    Node *leftNode=nullptr, *rightNode=nullptr;
    Node *parentNode=nullptr;
    bool leftNodeIsNull = true;
    bool rightNodeIsNull = true;
    Node* tmpDummyNode = new Node();
    tmpDummyNode->isDummy = true;


    Node *node = nullptr;

#if SGX_DEBUG
    printf("DeleteNode2 rootKey=%d, rootPos=%lld, parentKey=%d, parentPos=%lld, parentRootRelation=%d, key=%d, height=%d, depth=%d, isDummyDel=%d\n",
           rootKey.getValue(), rootPos, parentKey.getValue(), parentPos, parentRootRelation, key.getValue(), height, depth, isDummyDel);
#endif
    unsigned long long newP;
    Bid retKey = rootKey;

    if (isDummyDel && !CTeq(CTcmp(depth, (int) ((float) oram->depth * 1.44)), -1)) {
        // TODO: write the final condition
#if SGX_DEBUG
        printf("Deleting key=%d...\n", key.getValue());
#endif
        node = oram->ReadWrite(rootKey, tmpDummyNode, rootPos, rootPos, true, false, true); //READ
        readWriteCacheNode(rootKey, node, false, false);
        if (node->leftID.isZero() || node->rightID.isZero()) {
            if (node->leftID.isZero() && node->rightID.isZero()) {
                children = 0;
#if SGX_DEBUG
                printf("No child case\n");
#endif
                parentNode = oram->ReadWrite(parentKey, tmpDummyNode, parentPos, parentPos, true, false, false);
#if SGX_DEBUG
                printf("parentNode key=%d, height=%d, leftID=%llu, rightID=%llu, parentRootRelation=%d\n",
                       parentKey.getValue(), parentNode->height, parentNode->leftID.getValue(), parentNode->rightID.getValue(), parentRootRelation);
#endif
                if (parentRootRelation == 0) {
                    node->isDummy = true;
                } else{
                    if (parentRootRelation == -1) {
                        parentNode->leftID.setToZero();
                        parentNode->leftPos = -1;
                        if (parentNode->rightID.isZero()) {
                            parentNode->height--;
                        }
                    } else if (parentRootRelation == 1) {
                        parentNode->rightID.setToZero();
                        parentNode->rightPos = -1;
                        if (parentNode->leftID.isZero()) {
                            parentNode->height--;
                        }
                    }
                    else {
                        printf("WTFFFF\n");
                    }
                    newP = RandomPath();
#if SGX_DEBUG
                    printf("Saving parentNode key=%d, height=%d, leftID=%llu, rightID=%llu\n",
                       parentKey.getValue(), parentNode->height, parentNode->leftID.getValue(), parentNode->rightID.getValue());
#endif
                    oram->ReadWrite(parentKey, parentNode, parentPos, newP, false, false, false);
                    parentNode->pos = newP;
                    parentPos = newP;
                    readWriteCacheNode(parentKey, parentNode, false, false);

                }
                rootKey = 0;
                rootPos = -1;
                retKey = rootKey;
                // TODO: what is this line doing here?
//                oram->ReadWrite(node->key, node, node->pos, node->pos, false, false, false);
            } else {
                // one child case

                children = 1;
                Bid childBid = node->rightID.isZero() ?
                               node->leftID : node->rightID;
                unsigned long long childPos = node->rightID.isZero() ?
                                              node->leftPos : node->rightPos;
#if SGX_DEBUG
                printf("One child case. childKey=%d\n", childBid.getValue());
#endif
                parentNode = oram->ReadWrite(parentKey, tmpDummyNode, parentPos, parentPos, true, false, false);
#if SGX_DEBUG
                printf("parentNode key=%d, height=%d, leftID=%llu, rightID=%llu, parentRootRelation=%d\n",
                       parentKey.getValue(), parentNode->height, parentNode->leftID.getValue(), parentNode->rightID.getValue(), parentRootRelation);
#endif
                if (parentRootRelation == 0) {
                    node->isDummy = true;
                    parentKey = parentNode->key;
                    rootKey = childBid;
                    rootPos = childPos;
                } else if (parentRootRelation == -1) {
                    parentNode->leftID.setValue(childBid.getValue());
                    parentNode->leftPos = childPos;
                } else if (parentRootRelation == 1) {
                    parentNode->rightID.setValue(childBid.getValue());
                    parentNode->rightPos = childPos;
                } else {
                    printf("WTFFFF\n");
                }
                newP = RandomPath();
#if SGX_DEBUG
                printf("Saving parentNode key=%d, height=%d, leftID=%llu, rightID=%llu\n",
                       parentKey.getValue(), parentNode->height, parentNode->leftID.getValue(), parentNode->rightID.getValue());
#endif
                oram->ReadWrite(parentKey, parentNode, parentPos, newP, false, false, false);
                parentNode->pos = newP;
                parentPos = newP;
                readWriteCacheNode(parentKey, parentNode, false, false);
                rootKey = childBid;
                rootPos = childPos;
                retKey = childBid;
                lastID = 0;
                node = oram->ReadWrite(childBid, tmpDummyNode, childPos, childPos, true, false, true);
            }
        } else {
            // node with two children
            children = 2;
            Node *successor = minValueNode(node->rightID, node->rightPos, remainderIsDummy);
#if SGX_DEBUG
            printf("node with two children. Successor key=%d, pos=%lld\n", successor->key.getValue(), successor->pos);
#endif
            if (parentRootRelation == 0) {
#if SGX_DEBUG
                printf("Removing root\n");
#endif
//                parentNode->isDummy = true;
                newP = RandomPath();
            } else {
                parentNode = oram->ReadWrite(parentKey, tmpDummyNode, parentPos, parentPos, true, false, false);
                if (parentRootRelation == -1) {
                    parentNode->leftID.setValue(successor->key.getValue());
                    parentNode->leftPos = successor->leftPos;
                } else if (parentRootRelation == 1) {
                    parentNode->rightID.setValue(successor->key.getValue());
                    parentNode->rightPos = successor->rightPos;
                }
                newP = RandomPath();
#if SGX_DEBUG
                printf("Saving parentNode key=%d, height=%d, leftID=%llu, rightID=%llu\n",
                           parentKey.getValue(), parentNode->height, parentNode->leftID.getValue(), parentNode->rightID.getValue());
#endif
                oram->ReadWrite(parentKey, parentNode, parentPos, newP, false, false, false);
                parentNode->pos = newP;
                parentPos = newP;
                readWriteCacheNode(parentKey, parentNode, false, false);
            }
            node->key.setValue(successor->key.getValue());
            node->pos = successor->pos;
            node->setValue(successor->value);
            int height2=0;
            int depth2=0;
            int children2=-1;
            node->rightID = deleteNode2(node->rightID, node->rightPos, node->key, node->pos, 1, successor->key, height2, 0, children2, depth2, false);
            if (!node->leftID.isZero()) {
                leftNode = oram->ReadWrite(node->leftID, tmpDummyNode, node->leftPos, node->leftPos, true, false, false);
                readWriteCacheNode(node->leftID, leftNode, false, false);
                leftHeight = leftNode->height;
                leftNodeIsNull = false;
            }
            node->height = max(height2, leftHeight+1);
            newP = RandomPath();

#if SGX_DEBUG
            printf("Saving updated node key=%d, height=%d, leftID=%llu, rightID=%llu\n",
                           node->key.getValue(), node->height, node->leftID.getValue(), node->rightID.getValue());
#endif
            oram->ReadWrite(node->key, node, node->pos, newP, false, false, false);
            node->pos = newP;
            readWriteCacheNode(node->key, node, false, false);
            rootKey = node->key;
            rootPos = node->pos;
            retKey = rootKey;
        }
        return retKey;
    }


    if (!isDummyDel && !rootKey.isZero()) {
        remainderIsDummy = false;
    } else {
        remainderIsDummy = true;
    }

    if(remainderIsDummy) {
        node = oram->ReadWrite(dummy, tmpDummyNode, rootPos, rootPos, true, true, true);
        readWriteCacheNode(dummy, tmpDummyNode, false, true);
    } else {
        node = oram->ReadWrite(rootKey, tmpDummyNode, rootPos, rootPos, true, false, true); //READ
        readWriteCacheNode(rootKey, node, false, false);
    }

    if (!remainderIsDummy) {
        if (key < node->key) {
            node->leftID = deleteNode2(node->leftID, node->leftPos, node->key, node->pos, -1, key, height, dummy, children, depth, false);
            depth--;
        } else if (key > node->key) {
            node->rightID = deleteNode2(node->rightID, node->rightPos, node->key, node->pos, 1, key, height, dummy, children, depth, false);
            depth--;
        } else {
            exist = true;
            Bid resValBid = deleteNode2(rootKey, rootPos, parentKey, parentPos, parentRootRelation, key, height, node->key, children, depth, true);
            depth--;
#if SGX_DEBUG
            printf("Returning after child case: %d, current node=%d\n", children, node->key.getValue());
#endif
//            rootPos = node->pos;
//            height = node->height;
            if (children == 0) {
                retKey = resValBid;
                node->isDummy = true;
#if SGX_DEBUG
                printf("Case 0: zero children, return retKey=%d, resValBid=%d\n", retKey.getValue(), resValBid.getValue());
#endif
            } else if (children == 1) {
                retKey = resValBid;
                node->isDummy = true;
#if SGX_DEBUG
                printf("Case 1: one child, return retKey=%d, resValBid=%d\n", retKey.getValue(), resValBid.getValue());
#endif
            } else if (children == 2) {
                retKey = resValBid;
                node = oram->ReadWrite(resValBid, tmpDummyNode, rootPos, rootPos, true, false, false);
                if (parentRootRelation == 0) {
                    node = oram->ReadWrite(resValBid, tmpDummyNode, rootPos, rootPos, true, false, false);
                }
#if SGX_DEBUG
                printf("Case 2: two children, return retKey=%d, resValBid=%d\n", retKey.getValue(), resValBid.getValue());
#endif
            } else {
                printf("WTF\n");
            }
        }
    } else {
        Bid resValBid = deleteNode2(rootKey, rootPos, parentKey, parentPos, parentRootRelation, key, height, node->key, children, depth, true);
        depth--;
        retKey = resValBid;
        remainderIsDummy = remainderIsDummy;
    }

    if (rootKey.isZero()) {
#if SGX_DEBUG
        printf("If the tree had only one node then return\n");
#endif
        retKey = rootKey;
        remainderIsDummy = true;
//        return rootKey;
    } else {
        retKey = retKey;
        remainderIsDummy = remainderIsDummy;
    }

    unsigned long long newRLPos = RandomPath();

    if (!remainderIsDummy) {
        if (!node->leftID.isZero()) {
            leftNode = oram->ReadWrite(node->leftID, tmpDummyNode, node->leftPos, node->leftPos, true, false, false);
            readWriteCacheNode(node->leftID, leftNode, false, false);
            leftHeight = leftNode->height;
            leftNodeIsNull = false;
        } else {
            Node* dummyLeft = oram->ReadWrite(dummy, tmpDummyNode, newRLPos, newRLPos, true, false, false);
            readWriteCacheNode(dummy, dummyLeft, false, true);
            leftHeight = leftHeight;
            leftNodeIsNull = true;
        }

        newRLPos = RandomPath();

        if (!node->rightID.isZero()) {
            rightNode = oram->ReadWrite(node->rightID, tmpDummyNode, node->rightPos, node->rightPos, true, false, false);
            readWriteCacheNode(node->rightID, rightNode, false, false);
            rightHeight = rightNode->height;
            rightNodeIsNull = false;
        } else {
            Node* dummyRight = oram->ReadWrite(dummy, tmpDummyNode, newRLPos, newRLPos, true, false, false);
            readWriteCacheNode(dummy, dummyRight, false, true);
            rightHeight = rightHeight;
            rightNodeIsNull = true;
        }
    } else {
        Node* dummyLeft = oram->ReadWrite(dummy, tmpDummyNode, newRLPos, newRLPos, true, false, false);
        readWriteCacheNode(dummy, dummyLeft, false, true);
        leftHeight = leftHeight;
        leftNodeIsNull = true;
        Node* dummyRight = oram->ReadWrite(dummy, tmpDummyNode, newRLPos, newRLPos, true, false, false);
        readWriteCacheNode(dummy, dummyRight, false, true);
        rightHeight = rightHeight;
        rightNodeIsNull = true;
    }

    if(remainderIsDummy) {
        1 + max(leftHeight, rightHeight);
        node->height = node->height;
        height = height;
    } else {
        node->height = 1 + max(leftHeight, rightHeight);
        height = node->height;
    }

    if (remainderIsDummy) {
        balance = balance;
    } else {
        balance = leftHeight - rightHeight;
    }

    int leftBalance = getBalance(node->leftID, node->leftPos, remainderIsDummy);
    int rightBalance = getBalance(node->rightID, node->rightPos, remainderIsDummy);
#if SGX_DEBUG
    printf("node=%d, height=%d, balance=%d, leftBalance=%d, rightBalance=%d, leftHeight=%d, rightHeight=%d\n",
           node->key.getValue(), node->height, balance, leftBalance, rightBalance, leftHeight, rightHeight);
#endif

    // Left Left Case
    if (balance > 1 &&
        leftBalance >= 0) {
#if SGX_DEBUG
        printf("Left-Left Case: node=%d, leftNode=%d\n", node->key.getValue(), leftNode->key.getValue());
#endif
        Node *leftLeftNode = nullptr, *leftRightNode = nullptr;
        if (!leftNode->leftID.isZero()) {
            leftLeftNode = oram->ReadWrite(leftNode->leftID, tmpDummyNode, leftNode->leftPos, leftNode->leftPos, true, false, false);
            readWriteCacheNode(leftLeftNode->key, leftLeftNode, false, false);
        }
        if (!leftNode->rightID.isZero()) {
            leftRightNode = oram->ReadWrite(leftNode->rightID, tmpDummyNode, leftNode->rightPos, leftNode->rightPos, true, false, false);
            readWriteCacheNode(leftRightNode->key, leftRightNode, false, false);
        }
        rotate2(node, leftNode, rightHeight, true);
        newP = RandomPath();
        oram->ReadWrite(node->key, node, node->pos, newP, false, false, false); //WRITE
        node->pos = newP;
        leftNode->rightPos = newP;
        readWriteCacheNode(node->key, node, false, false); //WRITE

        newP = RandomPath();
        oram->ReadWrite(leftNode->key, leftNode, leftNode->pos, newP, false, false, false); //WRITE
        leftNode->pos = newP;
        readWriteCacheNode(leftNode->key, leftNode, false, false); //WRITE

        rootPos = leftNode->pos;
        height = leftNode->height;
        retKey = leftNode->key;
        delete leftLeftNode;
        delete leftRightNode;
//        return rightRotate(root);
    }

        // Left Right Case
    else if (balance > 1 && leftBalance < 0)
    {
#if SGX_DEBUG
        printf("Left-Right Case\n");
#endif
//        if (leftNodeisNull) {
//            delete leftNode;
//            leftNode = readWriteCacheNode(node->leftID, tmpDummyNode, true, false); //READ
//            leftNodeisNull = false;
//            node->leftPos = leftNode->pos;
//        } else {
//            readWriteCacheNode(dummy, tmpDummyNode, true, true);
//        }

        Node* leftRightNode=nullptr, *leftLeftNode=nullptr, *leftRightLeftNode=nullptr, *leftRightRightNode=nullptr;
        if (!leftNode->rightID.isZero()) {
            leftRightNode = oram->ReadWrite(leftNode->rightID, tmpDummyNode, leftNode->rightPos, leftNode->rightPos, true, false, false);
            readWriteCacheNode(leftNode->rightID, leftRightNode, false, false); //READ
        }


        int leftLeftHeight = 0;
        if (!leftNode->leftID.isZero()) {
            leftLeftNode = oram->ReadWrite(leftNode->leftID, tmpDummyNode, leftNode->leftPos, leftNode->leftPos, true, false, false);
            readWriteCacheNode(leftNode->leftID, leftLeftNode, false, false); //READ
            leftLeftHeight = leftLeftNode->height;
        }

        if (!leftRightNode->leftID.isZero()) {
            leftRightLeftNode = oram->ReadWrite(leftRightNode->leftID, tmpDummyNode, leftRightNode->leftPos, leftRightNode->leftPos, true, false, false);
            readWriteCacheNode(leftRightNode->leftID, leftRightLeftNode, false, false); //READ
        }

#if SGX_DEBUG
        printf("Left Rotate node=%d, oppositeNode=%d, targetHeight=%d\n",
               leftNode->key.getValue(), leftRightNode->key.getValue(), leftLeftHeight);
#endif
        if (!leftRightNode->rightID.isZero()) {
            leftRightRightNode = oram->ReadWrite(leftRightNode->rightID, tmpDummyNode, leftRightNode->rightPos, leftRightNode->rightPos, true, false, false);
            readWriteCacheNode(leftRightNode->rightID, leftRightRightNode, false, false); //READ
        }
        rotate2(leftNode, leftRightNode, leftLeftHeight, false);

        unsigned long long newP = RandomPath();
        unsigned long long oldLeftRightPos = leftNode->pos;
        leftNode->pos = newP;
        readWriteCacheNode(leftNode->key, leftNode, false, false); //WRITE
        leftRightNode->leftPos = newP;

        node->leftID = leftRightNode->key;
        node->leftPos = leftRightNode->pos;

#if SGX_DEBUG
        printf("Right Rotate node=%d, oppositeNode=%d, targetHeight=%d\n",
               node->key.getValue(), leftRightNode->key.getValue(), rightHeight);
#endif
        rotate2(node, leftRightNode, rightHeight, true);

        oram->ReadWrite(leftNode->key, leftNode, oldLeftRightPos, leftNode->pos, false, false, false); //WRITE
        newP = RandomPath();
        oram->ReadWrite(node->key, node, node->pos, newP, false, false, false); //WRITE
        node->pos = newP;
        readWriteCacheNode(node->key, node, false, false); //WRITE
        leftRightNode->rightPos = newP;
        newP = RandomPath();
        oram->ReadWrite(leftRightNode->key, leftRightNode, leftRightNode->pos, newP, false, false, false);
        leftRightNode->pos = newP;
        readWriteCacheNode(leftRightNode->key, leftRightNode, false, false); //WRITE
        doubleRotation = true;

        rootPos = leftRightNode->pos;
        height = leftRightNode->height;
        retKey = leftRightNode->key;
        delete leftRightNode;
        delete leftRightLeftNode;
        delete leftLeftNode;
        delete leftRightRightNode;
//        root->left = leftRotate(root->left);
//        return rightRotate(root);
    }

        // Right Right Case
    else if (balance < -1 && rightBalance <= 0) {
#if SGX_DEBUG
        printf("Right-Right Case: node=%d, rightNode=%d\n", node->key.getValue(), rightNode->key.getValue());
#endif
        Node *rightLeftNode=nullptr, *rightRightNode=nullptr;
        if (!rightNode->leftID.isZero()) {
            rightLeftNode = oram->ReadWrite(rightNode->leftID, tmpDummyNode, rightNode->leftPos, rightNode->leftPos, true, false, false);
            readWriteCacheNode(rightLeftNode->key, rightLeftNode, false, false);
        }
        if (!rightNode->rightID.isZero()) {
            rightRightNode = oram->ReadWrite(rightNode->rightID, tmpDummyNode, rightNode->rightPos, rightNode->rightPos, true, false, false);
            readWriteCacheNode(rightRightNode->key, rightRightNode, false, false);
        }
        rotate2(node, rightNode, leftHeight, false);

        unsigned long long newP = RandomPath();
        oram->ReadWrite(node->key, node, node->pos, newP, false, false, false); //WRITE
        node->pos = newP;
        readWriteCacheNode(node->key, node, false, false); //WRITE

        rightNode->leftPos = newP;
        newP = RandomPath();
        oram->ReadWrite(rightNode->key, rightNode, rightNode->pos, newP, false, false, false); //WRITE
        rightNode->pos = newP;
        readWriteCacheNode(rightNode->key, rightNode, false, false); //WRITE

        rootPos = rightNode->pos;
        height = rightNode->height;
        retKey = rightNode->key;
        delete rightLeftNode;
        delete rightRightNode;
    }

        // Right Left Case
    else if (balance < -1 &&
             rightBalance > 0)
    {
#if SGX_DEBUG
        printf("Right-Left Case\n");
#endif
        Node* rightLeftNode=nullptr, *rightRightNode=nullptr, *rightLeftLeftNode=nullptr, *rightLeftRightNode=nullptr;

        if (!rightNode->leftID.isZero()) {
            rightLeftNode = oram->ReadWrite(rightNode->leftID, tmpDummyNode, rightNode->leftPos, rightNode->leftPos, true, false, false);
            readWriteCacheNode(rightNode->leftID, rightLeftNode, true, false); //READ
        }

        int rightRightHeight = 0;
        if (!rightNode->rightID.isZero()) {
            rightRightNode = oram->ReadWrite(rightNode->rightID, tmpDummyNode, rightNode->rightPos, rightNode->rightPos, true, false, false);
            readWriteCacheNode(rightRightNode->key, rightRightNode, true, false); //READ
            rightRightHeight = rightRightNode->height;
        }


#if SGX_DEBUG
        printf("Right Rotate node=%d, oppositeNode=%d, targetHeight=%d\n",
               rightNode->key.getValue(), rightLeftNode->key.getValue(), rightRightHeight);
#endif
        if(!rightLeftNode->leftID.isZero()) {
            rightLeftLeftNode = oram->ReadWrite(rightLeftNode->leftID, tmpDummyNode, rightLeftNode->leftPos, rightLeftNode->leftPos, true, false, false);
            readWriteCacheNode(rightLeftNode->leftID, rightLeftLeftNode, false, false); //READ
        }

        if(!rightLeftNode->rightID.isZero()) {
            rightLeftRightNode = oram->ReadWrite(rightLeftNode->rightID, tmpDummyNode, rightLeftNode->rightPos, rightLeftNode->rightPos, true, false, false);
            readWriteCacheNode(rightLeftNode->rightID, rightLeftRightNode, false, false); //READ
        }

        rotate2(rightNode, rightLeftNode, rightRightHeight, true);


        unsigned long long newP = RandomPath();
        unsigned long long oldLeftRightPos = rightNode->pos;
        rightNode->pos = newP;
        readWriteCacheNode(rightNode->key, rightNode, false, false); //WRITE
        rightLeftNode->rightPos = newP;

        node->rightID = rightLeftNode->key;
        node->rightPos = rightLeftNode->pos;

#if SGX_DEBUG
        printf("Left Rotate node=%d, oppositeNode=%d, targetHeight=%d\n",
               node->key.getValue(), rightLeftNode->key.getValue(), rightHeight);
#endif
        rotate2(node, rightLeftNode, leftHeight, false);

        oram->ReadWrite(rightNode->key, rightNode, oldLeftRightPos, rightNode->pos, false, false, false); //WRITE
        newP = RandomPath();
        oram->ReadWrite(node->key, node, node->pos, newP, false, false, false); //WRITE
        node->pos = newP;
        readWriteCacheNode(node->key, node, false, false); //WRITE
        rightLeftNode->leftPos = newP;
        newP = RandomPath();
        oram->ReadWrite(rightLeftNode->key, rightLeftNode, rightLeftNode->pos, newP, false, false, false);
        rightLeftNode->pos = newP;
        readWriteCacheNode(rightLeftNode->key, rightLeftNode, false, false); //WRITE
        doubleRotation = true;

        rootPos = rightLeftNode->pos;
        height = rightLeftNode->height;
        retKey = rightLeftNode->key;
        delete rightLeftNode;
        delete rightRightNode;
        delete rightLeftLeftNode;
        delete rightLeftRightNode;

//        root->right = rightRotate(root->right);
//        return leftRotate(root);
    }
    else {
#if SGX_DEBUG
        printf("No balance needed\n");
#endif
        if (!node->key.isZero() && !node->isDummy) {
            newP = RandomPath();
            oram->ReadWrite(node->key, node, node->pos, newP, false, false, false); //WRITE
            node->pos = newP;
            readWriteCacheNode(node->key, node, false, false); //WRITE
            rootPos = node->pos;
            retKey = node->key;
        }
    }
    delete node;
    delete tmpDummyNode;
    return retKey;
}


Bid AVLTree::deleteNode3(Bid rootKey, unsigned long long& rootPos, Bid parentKey, unsigned long long &parentPos, int parentRootRelation, Bid key, int &height, Bid lastID, int &children, int &depth, bool isDummyDel) {
#if SGX_DEBUG
    printf("DeleteNode3 rootKey=%d, rootPos=%lld, parentKey=%d, parentPos=%lld, parentRootRelation=%d, key=%d, height=%d, isDummyDel=%d\n",
           rootKey.getValue(), rootPos, parentKey.getValue(), parentPos, parentRootRelation, key.getValue(), height, isDummyDel);
#endif
    unsigned long long newP;
    Bid retKey = rootKey;
    Bid dummy;
    dummy.setValue(oram->nextDummyCounter++);

    if (rootKey.isZero()) {
        return rootKey;
    }
    Node* tmpDummyNode = new Node();
    tmpDummyNode->isDummy = true;
    Node *node = oram->ReadWrite(rootKey, tmpDummyNode, rootPos, rootPos, true, false, true); //READ

    if (key < node->key)
        node->leftID = deleteNode3(node->leftID, node->leftPos, node->key, node->pos, -1, key, height, dummy, children, depth, false);

    else if (key > node->key)
        node->rightID = deleteNode3(node->rightID, node->rightPos, node->key, node->pos, 1, key, height, dummy, children, depth, false);

    else {
#if SGX_DEBUG
        printf("Deleting key%d...\n", key.getValue());
#endif
        if (node->leftID.isZero() || node->rightID.isZero()) {
            if (node->leftID.isZero() && node->rightID.isZero()) {
#if SGX_DEBUG
                printf("No child case\n");
#endif
                Node *parentNode = oram->ReadWrite(parentKey, tmpDummyNode, parentPos, parentPos, true, false, false);
#if SGX_DEBUG
                printf("parentNode key=%d, height=%d, leftID=%llu, rightID=%llu, parentRootRelation=%d\n",
                       parentKey.getValue(), parentNode->height, parentNode->leftID.getValue(), parentNode->rightID.getValue(), parentRootRelation);
#endif
                if (parentRootRelation == 0) {
                    node->isDummy = true;
                } else{
                    if (parentRootRelation == -1) {
                        parentNode->leftID.setToZero();
                        parentNode->leftPos = -1;
                        if (parentNode->rightID.isZero()) {
                            parentNode->height--;
                        }
                    } else if (parentRootRelation == 1) {
                        parentNode->rightID.setToZero();
                        parentNode->rightPos = -1;
                        if (parentNode->leftID.isZero()) {
                            parentNode->height--;
                        }
                    }
                    else {
                        printf("WTFFFF\n");
                    }
                    newP = RandomPath();
#if SGX_DEBUG
                    printf("Saving parentNode key=%d, height=%d, leftID=%llu, rightID=%llu\n",
                       parentKey.getValue(), parentNode->height, parentNode->leftID.getValue(), parentNode->rightID.getValue());
#endif
                    oram->ReadWrite(parentKey, parentNode, parentPos, newP, false, false, false);
                    parentNode->pos = newP;
                    parentPos = newP;
                    readWriteCacheNode(parentKey, parentNode, false, false);

                }
                rootKey = 0;
                rootPos = -1;
                oram->ReadWrite(node->key, node, node->pos, node->pos, false, false, false);
            } else {
                // one child case
#if SGX_DEBUG
                printf("One child case\n");
#endif
                Bid childBid = node->rightID.isZero() ?
                               node->leftID : node->rightID;
                unsigned long long childPos = node->rightID.isZero() ?
                                              node->leftPos : node->rightPos;
                Node *parentNode = oram->ReadWrite(parentKey, tmpDummyNode, parentPos, parentPos, true, false, false);
#if SGX_DEBUG
                printf("parentNode key=%d, height=%d, leftID=%llu, rightID=%llu, parentRootRelation=%d\n",
                       parentKey.getValue(), parentNode->height, parentNode->leftID.getValue(), parentNode->rightID.getValue(), parentRootRelation);
#endif
                if (parentRootRelation == 0) {
                    node->isDummy = true;
                    rootKey = 0;
                    rootPos = -1;
                } else if (parentRootRelation == -1) {
                    parentNode->leftID.setValue(childBid.getValue());
                    parentNode->leftPos = childPos;
                } else if (parentRootRelation == 1) {
                    parentNode->rightID.setValue(childBid.getValue());
                    parentNode->rightPos = childPos;
                } else {
                    printf("WTFFFF\n");
                }
                newP = RandomPath();
#if SGX_DEBUG
                printf("Saving parentNode key=%d, height=%d, leftID=%llu, rightID=%llu\n",
                       parentKey.getValue(), parentNode->height, parentNode->leftID.getValue(), parentNode->rightID.getValue());
#endif
                oram->ReadWrite(parentKey, parentNode, parentPos, newP, false, false, false);
                parentNode->pos = newP;
                parentPos = newP;
                readWriteCacheNode(parentKey, parentNode, false, false);
                rootKey = childBid;
                rootPos = childPos;
                retKey = childBid;
                node = oram->ReadWrite(childBid, tmpDummyNode, childPos, childPos, true, false, true);
            }
        } else {
            // node with two children
            Node *successor = minValueNode(node->rightID, node->rightPos, false);
#if SGX_DEBUG
            printf("node with two children. Successor key=%d, pos=%lld\n", successor->key.getValue(), successor->pos);
#endif
            Node *parentNode;
            if (parentRootRelation == 0) {
#if SGX_DEBUG
                printf("Removing root\n");
#endif
//                parentNode->isDummy = true;
                newP = RandomPath();
            } else {
                parentNode = oram->ReadWrite(parentKey, tmpDummyNode, parentPos, parentPos, true, false, false);
                if (parentRootRelation == -1) {
                    parentNode->leftID.setValue(successor->key.getValue());
                    parentNode->leftPos = successor->leftPos;
                } else if (parentRootRelation == 1) {
                    parentNode->rightID.setValue(successor->key.getValue());
                    parentNode->rightPos = successor->rightPos;
                }
                    newP = RandomPath();
#if SGX_DEBUG
                    printf("Saving parentNode key=%d, height=%d, leftID=%llu, rightID=%llu\n",
                           parentKey.getValue(), parentNode->height, parentNode->leftID.getValue(), parentNode->rightID.getValue());
#endif
                    oram->ReadWrite(parentKey, parentNode, parentPos, newP, false, false, false);
                    parentNode->pos = newP;
                    parentPos = newP;
                    readWriteCacheNode(parentKey, parentNode, false, false);
            }
            node->key.setValue(successor->key.getValue());
            node->pos = successor->pos;
            node->setValue(successor->value);
            int height2;
            node->rightID = deleteNode3(node->rightID, node->rightPos, node->key, node->pos, 1, successor->key, height2, dummy, children, depth, false);
        }
    }

    if (rootKey == 0) {
#if SGX_DEBUG
        printf("If the tree had only one node then return\n");
#endif
        return rootKey;
    }

    //TODO: was -1!!!! WHY?
    int leftHeight = 0, rightHeight = 0;
    Node *leftNode, *rightNode;
    if (!node->leftID.isZero()) {
        leftNode = oram->ReadWrite(node->leftID, tmpDummyNode, node->leftPos, node->leftPos, true, false, false);
        readWriteCacheNode(node->leftID, leftNode, false, false);
        leftHeight = leftNode->height;
    }

    if (!node->rightID.isZero()) {
        rightNode = oram->ReadWrite(node->rightID, tmpDummyNode, node->rightPos, node->rightPos, true, false, false);
        readWriteCacheNode(node->rightID, rightNode, false, false);
        rightHeight = rightNode->height;
    }

    node->height = 1 + max(leftHeight, rightHeight);
    height = node->height;

    int balance = leftHeight - rightHeight;
    int leftBalance = getBalance(node->leftID, node->leftPos, false);
    int rightBalance = getBalance(node->rightID, node->rightPos, false);
#if SGX_DEBUG
    printf("node=%d, height=%d, balance=%d, leftBalance=%d, rightBalance=%d, leftHeight=%d, rightHeight=%d\n",
           node->key.getValue(), node->height, balance, leftBalance, rightBalance, leftHeight, rightHeight);
#endif


    // Left Left Case
    if (balance > 1 &&
        leftBalance >= 0) {
#if SGX_DEBUG
        printf("Left-Left Case: node=%d, leftNode=%d\n", node->key.getValue(), leftNode->key.getValue());
#endif
        Node *leftLeftNode = nullptr, *leftRightNode = nullptr;
        if (!leftNode->leftID.isZero()) {
            leftLeftNode = oram->ReadWrite(leftNode->leftID, tmpDummyNode, leftNode->leftPos, leftNode->leftPos, true, false, false);
            readWriteCacheNode(leftLeftNode->key, leftLeftNode, false, false);
        }
        if (!leftNode->rightID.isZero()) {
            leftRightNode = oram->ReadWrite(leftNode->rightID, tmpDummyNode, leftNode->rightPos, leftNode->rightPos, true, false, false);
            readWriteCacheNode(leftRightNode->key, leftRightNode, false, false);
        }
        rotate2(node, leftNode, rightHeight, true);
        newP = RandomPath();
        oram->ReadWrite(node->key, node, node->pos, newP, false, false, false); //WRITE
        node->pos = newP;
        leftNode->rightPos = newP;
        readWriteCacheNode(node->key, node, false, false); //WRITE

        newP = RandomPath();
        oram->ReadWrite(leftNode->key, leftNode, leftNode->pos, newP, false, false, false); //WRITE
        leftNode->pos = newP;
        readWriteCacheNode(leftNode->key, leftNode, false, false); //WRITE

        rootPos = leftNode->pos;
        height = leftNode->height;
        retKey = leftNode->key;
        delete leftLeftNode;
        delete leftRightNode;
//        return rightRotate(root);
    }

    // Left Right Case
    else if (balance > 1 && leftBalance < 0)
    {
#if SGX_DEBUG
        printf("Left-Right Case\n");
#endif
//        if (leftNodeisNull) {
//            delete leftNode;
//            leftNode = readWriteCacheNode(node->leftID, tmpDummyNode, true, false); //READ
//            leftNodeisNull = false;
//            node->leftPos = leftNode->pos;
//        } else {
//            readWriteCacheNode(dummy, tmpDummyNode, true, true);
//        }

        Node* leftRightNode=nullptr, *leftLeftNode=nullptr, *leftRightLeftNode=nullptr, *leftRightRightNode=nullptr;
        if (!leftNode->rightID.isZero()) {
            leftRightNode = oram->ReadWrite(leftNode->rightID, tmpDummyNode, leftNode->rightPos, leftNode->rightPos, true, false, false);
            readWriteCacheNode(leftNode->rightID, leftRightNode, false, false); //READ
        }


        int leftLeftHeight = 0;
        if (!leftNode->leftID.isZero()) {
            leftLeftNode = oram->ReadWrite(leftNode->leftID, tmpDummyNode, leftNode->leftPos, leftNode->leftPos, true, false, false);
            readWriteCacheNode(leftNode->leftID, leftLeftNode, false, false); //READ
            leftLeftHeight = leftLeftNode->height;
        }

        if (!leftRightNode->leftID.isZero()) {
            leftRightLeftNode = oram->ReadWrite(leftRightNode->leftID, tmpDummyNode, leftRightNode->leftPos, leftRightNode->leftPos, true, false, false);
            readWriteCacheNode(leftRightNode->leftID, leftRightLeftNode, false, false); //READ
        }

#if SGX_DEBUG
        printf("Left Rotate node=%d, oppositeNode=%d, targetHeight=%d\n",
               leftNode->key.getValue(), leftRightNode->key.getValue(), leftLeftHeight);
#endif
        if (!leftRightNode->rightID.isZero()) {
            leftRightRightNode = oram->ReadWrite(leftRightNode->rightID, tmpDummyNode, leftRightNode->rightPos, leftRightNode->rightPos, true, false, false);
            readWriteCacheNode(leftRightNode->rightID, leftRightRightNode, false, false); //READ
        }
        rotate2(leftNode, leftRightNode, leftLeftHeight, false);

        unsigned long long newP = RandomPath();
        unsigned long long oldLeftRightPos = leftNode->pos;
        leftNode->pos = newP;
        readWriteCacheNode(leftNode->key, leftNode, false, false); //WRITE
        leftRightNode->leftPos = newP;

        node->leftID = leftRightNode->key;
        node->leftPos = leftRightNode->pos;

#if SGX_DEBUG
        printf("Right Rotate node=%d, oppositeNode=%d, targetHeight=%d\n",
               node->key.getValue(), leftRightNode->key.getValue(), rightHeight);
#endif
        rotate2(node, leftRightNode, rightHeight, true);

        oram->ReadWrite(leftNode->key, leftNode, oldLeftRightPos, leftNode->pos, false, false, false); //WRITE
        newP = RandomPath();
        oram->ReadWrite(node->key, node, node->pos, newP, false, false, false); //WRITE
        node->pos = newP;
        readWriteCacheNode(node->key, node, false, false); //WRITE
        leftRightNode->rightPos = newP;
        newP = RandomPath();
        oram->ReadWrite(leftRightNode->key, leftRightNode, leftRightNode->pos, newP, false, false, false);
        leftRightNode->pos = newP;
        readWriteCacheNode(leftRightNode->key, leftRightNode, false, false); //WRITE
        doubleRotation = true;

        rootPos = leftRightNode->pos;
        height = leftRightNode->height;
        retKey = leftRightNode->key;
        delete leftRightNode;
        delete leftRightLeftNode;
        delete leftLeftNode;
        delete leftRightRightNode;
//        root->left = leftRotate(root->left);
//        return rightRotate(root);
    }

    // Right Right Case
    else if (balance < -1 && rightBalance <= 0) {
#if SGX_DEBUG
        printf("Right-Right Case: node=%d, rightNode=%d\n", node->key.getValue(), rightNode->key.getValue());
#endif
        Node *rightLeftNode=nullptr, *rightRightNode=nullptr;
        if (!rightNode->leftID.isZero()) {
            rightLeftNode = oram->ReadWrite(rightNode->leftID, tmpDummyNode, rightNode->leftPos, rightNode->leftPos, true, false, false);
            readWriteCacheNode(rightLeftNode->key, rightLeftNode, false, false);
        }
        if (!rightNode->rightID.isZero()) {
            rightRightNode = oram->ReadWrite(rightNode->rightID, tmpDummyNode, rightNode->rightPos, rightNode->rightPos, true, false, false);
            readWriteCacheNode(rightRightNode->key, rightRightNode, false, false);
        }
        rotate2(node, rightNode, leftHeight, false);

        unsigned long long newP = RandomPath();
        oram->ReadWrite(node->key, node, node->pos, newP, false, false, false); //WRITE
        node->pos = newP;
        readWriteCacheNode(node->key, node, false, false); //WRITE

        rightNode->leftPos = newP;
        newP = RandomPath();
        oram->ReadWrite(rightNode->key, rightNode, rightNode->pos, newP, false, false, false); //WRITE
        rightNode->pos = newP;
        readWriteCacheNode(rightNode->key, rightNode, false, false); //WRITE

        rootPos = rightNode->pos;
        height = rightNode->height;
        retKey = rightNode->key;
        delete rightLeftNode;
        delete rightRightNode;
    }

    // Right Left Case
    else if (balance < -1 &&
        rightBalance > 0)
    {
#if SGX_DEBUG
        printf("Right-Left Case\n");
#endif
        Node* rightLeftNode=nullptr, *rightRightNode=nullptr, *rightLeftLeftNode=nullptr, *rightLeftRightNode=nullptr;

        if (!rightNode->leftID.isZero()) {
            rightLeftNode = oram->ReadWrite(rightNode->leftID, tmpDummyNode, rightNode->leftPos, rightNode->leftPos, true, false, false);
            readWriteCacheNode(rightNode->leftID, rightLeftNode, true, false); //READ
        }

        int rightRightHeight = 0;
        if (!rightNode->rightID.isZero()) {
            rightRightNode = oram->ReadWrite(rightNode->rightID, tmpDummyNode, rightNode->rightPos, rightNode->rightPos, true, false, false);
            readWriteCacheNode(rightRightNode->key, rightRightNode, true, false); //READ
            rightRightHeight = rightRightNode->height;
        }


#if SGX_DEBUG
        printf("Right Rotate node=%d, oppositeNode=%d, targetHeight=%d\n",
               rightNode->key.getValue(), rightLeftNode->key.getValue(), rightRightHeight);
#endif
        if(!rightLeftNode->leftID.isZero()) {
            rightLeftLeftNode = oram->ReadWrite(rightLeftNode->leftID, tmpDummyNode, rightLeftNode->leftPos, rightLeftNode->leftPos, true, false, false);
            readWriteCacheNode(rightLeftNode->leftID, rightLeftLeftNode, false, false); //READ
        }

        if(!rightLeftNode->rightID.isZero()) {
            rightLeftRightNode = oram->ReadWrite(rightLeftNode->rightID, tmpDummyNode, rightLeftNode->rightPos, rightLeftNode->rightPos, true, false, false);
            readWriteCacheNode(rightLeftNode->rightID, rightLeftRightNode, false, false); //READ
        }

        rotate2(rightNode, rightLeftNode, rightRightHeight, true);


        unsigned long long newP = RandomPath();
        unsigned long long oldLeftRightPos = rightNode->pos;
        rightNode->pos = newP;
        readWriteCacheNode(rightNode->key, rightNode, false, false); //WRITE
        rightLeftNode->rightPos = newP;

        node->rightID = rightLeftNode->key;
        node->rightPos = rightLeftNode->pos;

#if SGX_DEBUG
        printf("Left Rotate node=%d, oppositeNode=%d, targetHeight=%d\n",
               node->key.getValue(), rightLeftNode->key.getValue(), rightHeight);
#endif
        rotate2(node, rightLeftNode, leftHeight, false);

        oram->ReadWrite(rightNode->key, rightNode, oldLeftRightPos, rightNode->pos, false, false, false); //WRITE
        newP = RandomPath();
        oram->ReadWrite(node->key, node, node->pos, newP, false, false, false); //WRITE
        node->pos = newP;
        readWriteCacheNode(node->key, node, false, false); //WRITE
        rightLeftNode->leftPos = newP;
        newP = RandomPath();
        oram->ReadWrite(rightLeftNode->key, rightLeftNode, rightLeftNode->pos, newP, false, false, false);
        rightLeftNode->pos = newP;
        readWriteCacheNode(rightLeftNode->key, rightLeftNode, false, false); //WRITE
        doubleRotation = true;

        rootPos = rightLeftNode->pos;
        height = rightLeftNode->height;
        retKey = rightLeftNode->key;
        delete rightLeftNode;
        delete rightRightNode;
        delete rightLeftLeftNode;
        delete rightLeftRightNode;

//        root->right = rightRotate(root->right);
//        return leftRotate(root);
    }
    else {
#if SGX_DEBUG
        printf("No balance needed\n");
#endif
        newP = RandomPath();
        oram->ReadWrite(node->key, node, node->pos, newP, false, false, false); //WRITE
        node->pos = newP;
        readWriteCacheNode(node->key, node, false, false); //WRITE
        rootPos = node->pos;
        retKey = node->key;
    }
    delete tmpDummyNode;
    return retKey;
}

Bid AVLTree::insert2(Bid rootKey, unsigned long long& rootPos, Bid omapKey, string value, int& height, Bid lastID, bool isDummyIns) {
    totheight++;
    unsigned long long rndPos = RandomPath();
    double t;
    std::array< byte_t, 16> tmpval;
    std::fill(tmpval.begin(), tmpval.end(), 0);
    std::copy(value.begin(), value.end(), tmpval.begin());
    Node* tmpDummyNode = new Node();
    tmpDummyNode->isDummy = true;
    Bid dummy;
    Bid retKey;
    unsigned long long dumyPos;
    bool remainerIsDummy = false;
    dummy.setValue(oram->nextDummyCounter++);

    // if is dummy and tothehight >= threshold
    if (isDummyIns && !CTeq(CTcmp(totheight, (int) ((float) oram->depth * 1.44)), -1)) {
        Bid resKey = lastID;
        if (!exist) {
            Node* nnode = newNode(omapKey, value);
            nnode->pos = RandomPath();
            height = nnode->height;
            rootPos = nnode->pos;
            oram->ReadWrite(omapKey, nnode, nnode->pos, nnode->pos, false, false, false);
            readWriteCacheNode(omapKey, nnode, false, false);
            resKey = nnode->key;
        } else {
            unsigned long long newP = RandomPath();
            Node* previousNode = readWriteCacheNode(omapKey, tmpDummyNode, true, false);
            oram->ReadWrite(omapKey, previousNode, previousNode->pos, newP, false, false, false);
            previousNode->pos = newP;
            readWriteCacheNode(omapKey, previousNode, false, false);
        }
        return resKey;
    }
    /* 1. Perform the normal BST rotation */

    if (!isDummyIns && !rootKey.isZero()) {
        remainerIsDummy = false;
    } else {
        remainerIsDummy = true;
    }

    Node* node = nullptr;
    if (remainerIsDummy) {
        node = oram->ReadWrite(dummy, tmpDummyNode, rootPos, rootPos, true, true, tmpval, Bid::CTeq(Bid::CTcmp(rootKey, omapKey), 0), true); //READ
        readWriteCacheNode(dummy, tmpDummyNode, false, true);
    } else {
        node = oram->ReadWrite(rootKey, tmpDummyNode, rootPos, rootPos, true, false, tmpval, Bid::CTeq(Bid::CTcmp(rootKey, omapKey), 0), true); //READ
        readWriteCacheNode(rootKey, node, false, false);
    }


    int balance = -1;
    int leftHeight = -1;
    int rightHeight = -1;
    Node* leftNode = new Node();
    bool leftNodeisNull = true;
    Node* rightNode = new Node();
    bool rightNodeisNull = true;
    std::array< byte_t, 16> garbage;
    bool childDirisLeft = false;


    unsigned long long newRLPos = RandomPath();

    if (!remainerIsDummy) {
        if (omapKey < node->key) {
            node->leftID = insert(node->leftID, node->leftPos, omapKey, value, leftHeight, dummy, false);
            if (!node->rightID.isZero()) {
                Node* rightNode = oram->ReadWrite(node->rightID, tmpDummyNode, node->rightPos, node->rightPos, true, false, true); //READ
                readWriteCacheNode(node->rightID, rightNode, false, false);
                rightNodeisNull = false;
                rightHeight = rightNode->height;
                std::fill(garbage.begin(), garbage.end(), 0);
                std::copy(value.begin(), value.end(), garbage.begin());
                remainerIsDummy = false;
            } else {
                Node* dummyright = oram->ReadWrite(dummy, tmpDummyNode, newRLPos, newRLPos, true, true, true);
                readWriteCacheNode(dummy, dummyright, false, true);
                rightHeight = 0;
                std::fill(garbage.begin(), garbage.end(), 0);
                std::copy(value.begin(), value.end(), garbage.begin());
                remainerIsDummy = false;
            }
            childDirisLeft = true;
            totheight--;

        } else if (omapKey > node->key) {
            node->rightID = insert(node->rightID, node->rightPos, omapKey, value, rightHeight, dummy, false);
            if (!node->leftID.isZero()) {
                Node* leftNode = oram->ReadWrite(node->leftID, tmpDummyNode, node->leftPos, node->leftPos, true, false, true); //READ
                readWriteCacheNode(node->leftID, leftNode, false, false);
                leftNodeisNull = false;
                leftHeight = leftNode->height;
                std::fill(garbage.begin(), garbage.end(), 0);
                std::copy(value.begin(), value.end(), garbage.begin());
                remainerIsDummy = false;
            } else {
                Node* dummyleft = oram->ReadWrite(dummy, tmpDummyNode, newRLPos, newRLPos, true, true, true);
                readWriteCacheNode(dummy, dummyleft, false, true);
                leftHeight = 0;
                std::fill(garbage.begin(), garbage.end(), 0);
                std::copy(value.begin(), value.end(), garbage.begin());
                remainerIsDummy = false;
            }
            childDirisLeft = false;
            totheight--;

        } else {
            exist = true;
            Bid resValBid = insert(rootKey, rootPos, omapKey, value, height, node->key, true);
            Node* dummyleft = oram->ReadWrite(dummy, tmpDummyNode, newRLPos, newRLPos, true, true, true);
            totheight--;
            node = readWriteCacheNode(node->key, tmpDummyNode, true, false);
            rootPos = node->pos;
            height = node->height;
            retKey = node->key;
            remainerIsDummy = true;
        }
    } else {
        Bid resValBid = insert(rootKey, rootPos, omapKey, value, height, node->key, true);
        Node* dummyleft = oram->ReadWrite(dummy, tmpDummyNode, newRLPos, newRLPos, true, true, true);
        totheight--;
        readWriteCacheNode(dummy, tmpDummyNode, true, true);
        std::fill(garbage.begin(), garbage.end(), 0);
        std::copy(value.begin(), value.end(), garbage.begin());
        retKey = resValBid;
        remainerIsDummy = remainerIsDummy;
    }
    /* 2. Update height of this ancestor node */
    if (remainerIsDummy) {
        max(leftHeight, rightHeight) + 1;
        node->height = node->height;
        height = height;
    } else {
        node->height = max(leftHeight, rightHeight) + 1;
        height = node->height;
    }
    /* 3. Get the balance factor of this ancestor node to check whether this node became unbalanced */
    if (remainerIsDummy) {
        balance = balance;
    } else {
        balance = leftHeight - rightHeight;
    }

    ocall_start_timer(totheight + 100000);

    if (remainerIsDummy == false && CTeq(CTcmp(balance, 1), 1) && omapKey < node->leftID) {
        //                printf("log1\n");
        // Left Left Case
        if (leftNodeisNull) {
            delete leftNode;
            leftNode = readWriteCacheNode(node->leftID, tmpDummyNode, true, false); //READ
            leftNodeisNull = false;
            node->leftPos = leftNode->pos;
        } else {
            readWriteCacheNode(dummy, tmpDummyNode, true, true); //WRITE            
        }

        //------------------------DUMMY---------------------------
        Node* leftRightNode = readWriteCacheNode(dummy, tmpDummyNode, true, true);
        leftRightNode->rightPos = leftRightNode->rightPos;

        int leftLeftHeight = 0;
        if (leftNode->leftID != 0) {
            Node* leftLeftNode = readWriteCacheNode(dummy, tmpDummyNode, true, true);
            leftLeftHeight = leftLeftHeight;
            delete leftLeftNode;
        } else {
            Node* leftLeftNode = readWriteCacheNode(dummy, tmpDummyNode, true, true);
            leftLeftHeight = leftLeftHeight;
            delete leftLeftNode;
        }
        rotate(tmpDummyNode, tmpDummyNode, leftLeftHeight, false, true);

        RandomPath();
        readWriteCacheNode(dummy, tmpDummyNode, false, true); //WRITE

        node->rightID = node->rightID;
        node->rightPos = node->rightPos;
        node->leftID = node->leftID;
        node->leftPos = node->leftPos;
        //---------------------------------------------------

        rotate(node, leftNode, rightHeight, true);


        unsigned long long newP = RandomPath();
        oram->ReadWrite(node->key, node, node->pos, newP, false, false, false); //WRITE
        node->pos = newP;
        leftNode->rightPos = newP;
        readWriteCacheNode(node->key, node, false, false); //WRITE

        newP = RandomPath();
        oram->ReadWrite(leftNode->key, leftNode, leftNode->pos, newP, false, false, false); //WRITE
        leftNode->pos = newP;
        readWriteCacheNode(leftNode->key, leftNode, false, false); //WRITE             
        readWriteCacheNode(dummy, tmpDummyNode, true, true);

        rootPos = leftNode->pos;
        height = leftNode->height;
        retKey = leftNode->key;
    } else if (remainerIsDummy == false && CTeq(CTcmp(balance, -1), -1) && omapKey > node->rightID) {
        //                printf("log2\n");
        // Right Right Case
        if (rightNodeisNull) {
            delete rightNode;
            rightNode = readWriteCacheNode(node->rightID, tmpDummyNode, true, false); //READ
            rightNodeisNull = false;
            node->rightPos = rightNode->pos;
        } else {
            readWriteCacheNode(dummy, tmpDummyNode, true, true);
        }

        //------------------------DUMMY---------------------------
        Node* leftRightNode = readWriteCacheNode(dummy, tmpDummyNode, true, true);

        int leftLeftHeight = 0;
        if (rightNode->leftID != 0) {
            Node* leftLeftNode = readWriteCacheNode(dummy, tmpDummyNode, true, true);
            leftLeftHeight = leftLeftHeight;
            delete leftLeftNode;
        } else {
            Node* leftLeftNode = readWriteCacheNode(dummy, tmpDummyNode, true, true);
            leftLeftHeight = leftLeftHeight;
            delete leftLeftNode;
        }
        rotate(tmpDummyNode, tmpDummyNode, leftLeftHeight, false, true);

        RandomPath();
        readWriteCacheNode(dummy, tmpDummyNode, false, true); //WRITE

        node->rightID = node->rightID;
        node->rightPos = node->rightPos;
        node->leftID = node->leftID;
        node->leftPos = node->leftPos;
        //---------------------------------------------------

        rotate(node, rightNode, leftHeight, false);

        unsigned long long newP = RandomPath();
        oram->ReadWrite(node->key, node, node->pos, newP, false, false, false); //WRITE
        node->pos = newP;
        readWriteCacheNode(node->key, node, false, false); //WRITE

        rightNode->leftPos = newP;
        newP = RandomPath();
        oram->ReadWrite(rightNode->key, rightNode, rightNode->pos, newP, false, false, false); //WRITE
        rightNode->pos = newP;
        readWriteCacheNode(rightNode->key, rightNode, false, false); //WRITE
        readWriteCacheNode(dummy, tmpDummyNode, true, true);

        rootPos = rightNode->pos;
        height = rightNode->height;
        retKey = rightNode->key;
    } else if (remainerIsDummy == false && CTeq(CTcmp(balance, 1), 1) && omapKey > node->leftID) {
        //                printf("log3\n");
        // Left Right Case        
        if (leftNodeisNull) {
            delete leftNode;
            leftNode = readWriteCacheNode(node->leftID, tmpDummyNode, true, false); //READ
            leftNodeisNull = false;
            node->leftPos = leftNode->pos;
        } else {
            readWriteCacheNode(dummy, tmpDummyNode, true, true);
        }


        Node* leftRightNode = readWriteCacheNode(leftNode->rightID, tmpDummyNode, true, false); //READ

        int leftLeftHeight = 0;
        if (leftNode->leftID != 0) {
            Node* leftLeftNode = readWriteCacheNode(leftNode->leftID, tmpDummyNode, true, false); //READ
            leftLeftHeight = leftLeftNode->height;
            delete leftLeftNode;
        } else {
            Node* leftLeftNode = readWriteCacheNode(dummy, tmpDummyNode, true, true);
            leftLeftHeight = leftLeftHeight;
            delete leftLeftNode;
        }

        rotate(leftNode, leftRightNode, leftLeftHeight, false);

        unsigned long long newP = RandomPath();
        unsigned long long oldLeftRightPos = leftNode->pos;
        leftNode->pos = newP;
        readWriteCacheNode(leftNode->key, leftNode, false, false); //WRITE
        leftRightNode->leftPos = newP;

        node->leftID = leftRightNode->key;
        node->leftPos = leftRightNode->pos;

        rotate(node, leftRightNode, rightHeight, true);

        oram->ReadWrite(leftNode->key, leftNode, oldLeftRightPos, leftNode->pos, false, false, false); //WRITE
        newP = RandomPath();
        oram->ReadWrite(node->key, node, node->pos, newP, false, false, false); //WRITE
        node->pos = newP;
        readWriteCacheNode(node->key, node, false, false); //WRITE
        leftRightNode->rightPos = newP;
        readWriteCacheNode(leftRightNode->key, leftRightNode, false, false); //WRITE
        doubleRotation = true;
        readWriteCacheNode(dummy, tmpDummyNode, true, true);

        rootPos = leftRightNode->pos;
        height = leftRightNode->height;
        retKey = leftRightNode->key;
    } else if (remainerIsDummy == false && CTeq(CTcmp(balance, -1), -1) && omapKey < node->rightID) {
        //                printf("log4\n");
        //  Right-Left Case
        if (rightNodeisNull) {
            delete rightNode;
            rightNode = readWriteCacheNode(node->rightID, tmpDummyNode, true, false); //READ
            rightNodeisNull = false;
            node->rightPos = rightNode->pos;
        } else {
            readWriteCacheNode(dummy, tmpDummyNode, true, true);
            node->rightPos = node->rightPos;
        }

        Node* rightLeftNode = readWriteCacheNode(rightNode->leftID, tmpDummyNode, true, false); //READ

        int rightRightHeight = 0;
        if (rightNode->rightID != 0) {
            Node* rightRightNode = readWriteCacheNode(rightNode->rightID, tmpDummyNode, true, false); //READ
            rightRightHeight = rightRightNode->height;
            delete rightRightNode;
        } else {
            Node* rightRightNode = readWriteCacheNode(dummy, tmpDummyNode, true, true);
            rightRightHeight = rightRightHeight;
            delete rightRightNode;
        }

        rotate(rightNode, rightLeftNode, rightRightHeight, true);

        unsigned long long newP = RandomPath();
        unsigned long long oldLeftRightPos = rightNode->pos;
        rightNode->pos = newP;
        readWriteCacheNode(rightNode->key, rightNode, false, false); //WRITE
        rightLeftNode->rightPos = newP;

        node->rightID = rightLeftNode->key;
        node->rightPos = rightLeftNode->pos;

        rotate(node, rightLeftNode, leftHeight, false);

        oram->ReadWrite(rightNode->key, rightNode, oldLeftRightPos, rightNode->pos, false, false, false); //WRITE        
        newP = RandomPath();
        oram->ReadWrite(node->key, node, node->pos, newP, false, false, false); //WRITE
        node->pos = newP;
        readWriteCacheNode(node->key, node, false, false); //WRITE
        rightLeftNode->leftPos = newP;
        readWriteCacheNode(rightLeftNode->key, rightLeftNode, false, false); //WRITE
        doubleRotation = true;
        readWriteCacheNode(dummy, tmpDummyNode, true, true);

        rootPos = rightLeftNode->pos;
        height = rightLeftNode->height;
        retKey = rightLeftNode->key;
    } else if (remainerIsDummy == false) {
        //                printf("log5\n");
        //------------------------DUMMY---------------------------
        if (node == NULL) {
            readWriteCacheNode(dummy, tmpDummyNode, true, true);
            node->rightPos = node->rightPos;
        } else {
            readWriteCacheNode(dummy, tmpDummyNode, true, true);
            node->rightPos = node->rightPos;
        }

        readWriteCacheNode(dummy, tmpDummyNode, true, true);
        node->rightPos = node->rightPos;

        int leftLeftHeight = 0;
        if (node->leftID != 0) {
            Node* leftLeftNode = readWriteCacheNode(dummy, tmpDummyNode, true, true);
            node->leftPos = node->leftPos;
            leftLeftHeight = leftLeftHeight;
            delete leftLeftNode;
        } else {
            Node* leftLeftNode = readWriteCacheNode(dummy, tmpDummyNode, true, true);
            node->leftPos = node->leftPos;
            leftLeftHeight = leftLeftHeight;
            delete leftLeftNode;
        }

        rotate(tmpDummyNode, tmpDummyNode, 0, false, true);

        RandomPath();
        readWriteCacheNode(dummy, tmpDummyNode, false, true); //WRITE

        node->rightID = node->rightID;
        node->rightPos = node->rightPos;
        node->leftID = node->leftID;
        node->leftPos = node->leftPos;

        rotate(tmpDummyNode, tmpDummyNode, 0, false, true);

        unsigned long long newP = RandomPath();
        if (doubleRotation) {
            doubleRotation = false;
            if (childDirisLeft) {
                leftNode = readWriteCacheNode(node->leftID, tmpDummyNode, true, false);
                oram->ReadWrite(leftNode->key, leftNode, leftNode->pos, newP, false, false, false); //WRITE
                leftNode->pos = newP;
                node->leftPos = newP;
                Node* t = readWriteCacheNode(node->leftID, leftNode, false, false);
                delete t;
            } else {
                rightNode = readWriteCacheNode(node->rightID, tmpDummyNode, true, false);
                oram->ReadWrite(rightNode->key, rightNode, rightNode->pos, newP, false, false, false); //WRITE
                rightNode->pos = newP;
                node->rightPos = newP;
                Node* t = readWriteCacheNode(node->rightID, rightNode, false, false);
                delete t;
            }
        } else {
            readWriteCacheNode(dummy, tmpDummyNode, true, true);
            oram->ReadWrite(dummy, tmpDummyNode, rightNode->pos, rightNode->pos, false, true, false); //WRITE
            Node* t = readWriteCacheNode(dummy, tmpDummyNode, true, true);
            delete t;
        }
        //----------------------------------------------------------------------------------------

        newP = RandomPath();
        oram->ReadWrite(node->key, node, node->pos, newP, false, false, false); //WRITE
        node->pos = newP;
        readWriteCacheNode(node->key, node, false, false); //WRITE
        readWriteCacheNode(dummy, tmpDummyNode, true, true);

        rootPos = node->pos;
        height = height;
        retKey = node->key;
    } else {
        //printf("log6\n");
        //------------------------DUMMY---------------------------
        if (node == NULL) {
            readWriteCacheNode(dummy, tmpDummyNode, true, true);
            node->rightPos = node->rightPos;
        } else {
            readWriteCacheNode(dummy, tmpDummyNode, true, true);
            node->rightPos = node->rightPos;
        }

        readWriteCacheNode(dummy, tmpDummyNode, true, true);
        int leftLeftHeight = 0;
        if (node->leftID != 0) {
            Node* leftLeftNode = readWriteCacheNode(dummy, tmpDummyNode, true, true);
            node->leftPos = node->leftPos;
            leftLeftHeight = leftLeftHeight;
            delete leftLeftNode;
        } else {
            Node* leftLeftNode = readWriteCacheNode(dummy, tmpDummyNode, true, true);
            node->leftPos = node->leftPos;
            leftLeftHeight = leftLeftHeight;
            delete leftLeftNode;
        }
        rotate(tmpDummyNode, tmpDummyNode, 0, false, true);

        RandomPath();
        readWriteCacheNode(dummy, tmpDummyNode, false, true); //WRITE

        node->rightID = node->rightID;
        node->rightPos = node->rightPos;
        node->leftID = node->leftID;
        node->leftPos = node->leftPos;

        rotate(tmpDummyNode, tmpDummyNode, 0, false, true);

        unsigned long long newP = RandomPath();
        if (doubleRotation) {
            doubleRotation = false;
            if (childDirisLeft) {
                leftNode = readWriteCacheNode(node->leftID, tmpDummyNode, true, false);
                oram->ReadWrite(leftNode->key, leftNode, leftNode->pos, newP, false, false, false); //WRITE
                leftNode->pos = newP;
                node->leftPos = newP;
                Node* t = readWriteCacheNode(node->leftID, leftNode, false, false);
                delete t;
            } else {
                rightNode = readWriteCacheNode(node->rightID, tmpDummyNode, true, false);
                oram->ReadWrite(rightNode->key, rightNode, rightNode->pos, newP, false, false, false); //WRITE
                rightNode->pos = newP;
                node->rightPos = newP;
                Node* t = readWriteCacheNode(node->rightID, rightNode, false, false);
                delete t;
            }
        } else {
            readWriteCacheNode(dummy, tmpDummyNode, true, true);
            oram->ReadWrite(dummy, tmpDummyNode, rightNode->pos, rightNode->pos, false, true, false); //WRITE
            Node* t = readWriteCacheNode(dummy, tmpDummyNode, true, true);
            delete t;
        }

        oram->ReadWrite(dummy, tmpDummyNode, dumyPos, dumyPos, false, true, false); //WRITE
        readWriteCacheNode(dummy, tmpDummyNode, true, true);
        readWriteCacheNode(dummy, tmpDummyNode, true, true);

        rootPos = rootPos;
        height = height;
        retKey = retKey;
        //----------------------------------------------------------------------------------------
    }
    ocall_stop_timer(&t, totheight + 100000);
    //    times[0][times[0].size() - 1] += t;
    delete tmpDummyNode;
    return retKey;
}

string AVLTree::search(Node* rootNode, Bid omapKey) {
    Bid curKey = rootNode->key;
    unsigned long long lastPos = rootNode->pos;
    unsigned long long newPos = RandomPath();
    rootNode->pos = newPos;
    string res(16, '\0');
    Bid dumyID = oram->nextDummyCounter;
    Node* tmpDummyNode = new Node();
    tmpDummyNode->isDummy = true;
    std::array< byte_t, 16> resVec;
    Node* head;
    int dummyState = 0;
    int upperBound = (int) (1.44 * oram->depth);
    bool found = false;
    unsigned long long dumyPos;

    do {
        unsigned long long rnd = RandomPath();
        unsigned long long rnd2 = RandomPath();
        bool isDummyAction = Node::CTeq(Node::CTcmp(dummyState, 1), 0);
        head = oram->ReadWrite(curKey, lastPos, newPos, isDummyAction, rnd2, omapKey);

        bool cond1 = Node::CTeq(Node::CTcmp(dummyState, 1), 0);
        bool cond2 = Node::CTeq(Bid::CTcmp(head->key, omapKey), 1);
        bool cond3 = Node::CTeq(Bid::CTcmp(head->key, omapKey), -1);
        bool cond4 = Node::CTeq(Bid::CTcmp(head->key, omapKey), 0);

        lastPos = Node::conditional_select(rnd, lastPos, cond1);
        lastPos = Node::conditional_select(head->leftPos, lastPos, !cond1 && cond2);
        lastPos = Node::conditional_select(head->rightPos, lastPos, !cond1 && !cond2 && cond3);

        curKey = dumyID;

        bool leftIsZero = head->leftID.isZero();
        bool rightIsZero = head->rightID.isZero();
        for (int k = 0; k < curKey.id.size(); k++) {
            curKey.id[k] = Node::conditional_select(head->leftID.id[k], curKey.id[k], !cond1 && cond2 && !leftIsZero);
            curKey.id[k] = Node::conditional_select(head->rightID.id[k], curKey.id[k], !cond1 && !cond2 && cond3 && !rightIsZero);
        }

        newPos = Node::conditional_select(rnd, newPos, cond1);
        newPos = Node::conditional_select(rnd2, newPos, !cond1 && cond2);
        newPos = Node::conditional_select(rnd2, newPos, !cond1 && !cond2 && cond3);

        for (int i = 0; i < 16; i++) {
            resVec[i] = Bid::conditional_select(head->value[i], resVec[i], !cond1);
        }        

        dummyState = Node::conditional_select(1, dummyState, !cond1 && !cond2 && !cond3 && cond4);               
        dummyState = Node::conditional_select(1, dummyState, !cond4 && ((!cond1 && cond2 && leftIsZero)|| (!cond1 && !cond2 && cond3 && rightIsZero) ));        
        found = Node::conditional_select(true, found, !cond1 && !cond2 && !cond3 && cond4);
        delete head;
    } while (oram->readCnt <= upperBound);
    delete tmpDummyNode;
    for (int i = 0; i < 16; i++) {
        res[i] = Bid::conditional_select(resVec[i], (byte_t) 0, found);
    }
//    res.assign(resVec.begin(), resVec.end());
    return res;
}

int AVLTree::getBalance(Bid rootKey, unsigned long long& rootPos, bool isDummyOp)
{
    int balance = 0;
    if (rootKey.isZero())
        return 0;
    Node *N = nullptr, *lNode = nullptr, *rNode = nullptr;
    Node *tmpDummyNode = new Node();
    int lHeight = 0;
    tmpDummyNode->isDummy = true;
    int rHeight = 0;
    Bid dummy;
    unsigned long long dummyPos = RandomPath();
    dummy.setValue(oram->nextDummyCounter++);


    if (!isDummyOp && !rootKey.isZero()) {
        N = oram->ReadWrite(rootKey, tmpDummyNode, rootPos, rootPos, true, false, true);
        if (!N->leftID.isZero()) {
            lNode = oram->ReadWrite(N->leftID, tmpDummyNode, N->leftPos, N->leftPos, true, false, true);
            lHeight = lNode->height;
        } else {
            lNode = oram->ReadWrite(dummy, tmpDummyNode, dummyPos, dummyPos, true, false, false);
            lHeight = lHeight;
        }
        if (!N->rightID.isZero()) {
            rNode = oram->ReadWrite(N->rightID, tmpDummyNode, N->rightPos, N->rightPos, true, false, true);
            rHeight = rNode->height;
        } else {
            rNode = oram->ReadWrite(dummy, tmpDummyNode, dummyPos, dummyPos, true, false, false);
            rHeight = rHeight;
        }
    } else {
        N = oram->ReadWrite(dummy, tmpDummyNode, dummyPos, dummyPos, true, false, false);
        lNode = oram->ReadWrite(dummy, tmpDummyNode, dummyPos, dummyPos, true, false, false);
        lHeight = lHeight;
        rNode = oram->ReadWrite(dummy, tmpDummyNode, dummyPos, dummyPos, true, false, false);
        rHeight = rHeight;
    }

    balance = lHeight - rHeight;
    delete N;
    delete lNode;
    delete rNode;
    delete tmpDummyNode;

    return balance;
}

//string AVLTree::search(Node* rootNode, Bid omapKey) {
//    Bid curKey = rootNode->key;
//    unsigned long long lastPos = rootNode->pos;
//    unsigned long long newPos = RandomPath();
//    rootNode->pos = newPos;
//    string res = "";
//    Bid dumyID = oram->nextDummyCounter;
//    Node* tmpDummyNode = new Node();
//    tmpDummyNode->isDummy = true;
//    Node* head;
//    int dummyState = 0;
//    int upperBound = (int) (1.44 * oram->depth);
//    bool found = false;
//    unsigned long long dumyPos;
//    do {
//        head = oram->ReadWrite(curKey, tmpDummyNode, lastPos, newPos, true, dummyState > 1 ? true : false);
//        //        head = oram->ReadNode(curKey, lastPos, newPos, dummyState > 1 ? true : false);
//        unsigned long long rnd = RandomPath();
//        if (dummyState > 1) {
//            lastPos = rnd;
//            head->rightPos = head->rightPos;
//            head->leftPos = head->leftPos;
//            curKey = dumyID;
//            newPos = rnd;
//            res.assign(res.begin(), res.end());
//            dummyState = dummyState;
//            head->key = dumyID;
//            found = found;
//        } else if (head->key > omapKey) {
//            lastPos = head->leftPos;
//            head->rightPos = head->rightPos;
//            head->leftPos = rnd;
//            curKey = head->leftID;
//            newPos = head->leftPos;
//            res.assign(head->value.begin(), head->value.end());
//            dummyState = dummyState;
//            head->key = head->key;
//            found = found;
//        } else if (head->key < omapKey) {
//            lastPos = head->rightPos;
//            head->rightPos = rnd;
//            head->leftPos = head->leftPos;
//            curKey = head->rightID;
//            newPos = head->rightPos;
//            res.assign(head->value.begin(), head->value.end());
//            dummyState = dummyState;
//            head->key = head->key;
//            found = found;
//        } else {
//            lastPos = lastPos;
//            head->rightPos = head->rightPos;
//            head->leftPos = head->leftPos;
//            curKey = dumyID;
//            newPos = newPos;
//            res.assign(head->value.begin(), head->value.end());
//            dummyState = dummyState;
//            dummyState++;
//            head->key = head->key;
//            found = true;
//        }
//        oram->ReadWrite(head->key, head, head->pos, head->pos, false, dummyState <= 1 ? false : true);
//        //        oram->WriteNode(head->key, head, oram->evictBuckets, dummyState <= 1 ? false : true);
//        dummyState == 1 ? dummyState++ : dummyState;
//    } while (!found || oram->readCnt < upperBound);
//    
//    return res;
//}

/**
 * a recursive search function which traverse binary tree to find the target node
 */
void AVLTree::batchSearch(Node* head, vector<Bid> keys, vector<Node*>* results) {
    //    if (head == NULL || head->key == 0) {
    //        return;
    //    }
    //    head = oram->ReadNode(head->key, head->pos, head->pos);
    //    bool getLeft = false, getRight = false;
    //    vector<Bid> leftkeys, rightkeys;
    //    for (Bid bid : keys) {
    //        if (head->key > bid) {
    //            getLeft = true;
    //            leftkeys.push_back(bid);
    //        }
    //        if (head->key < bid) {
    //            getRight = true;
    //            rightkeys.push_back(bid);
    //        }
    //        if (head->key == bid) {
    //            results->push_back(head);
    //        }
    //    }
    //    if (getLeft) {
    //        batchSearch(oram->ReadNode(head->leftID, head->leftPos, head->leftPos), leftkeys, results);
    //    }
    //    if (getRight) {
    //        batchSearch(oram->ReadNode(head->rightID, head->rightPos, head->rightPos), rightkeys, results);
    //    }
}

void AVLTree::preOrderKeys(Node *rt, vector<long long> &res) {
    if (rt != nullptr && !rt->key.isZero()) {
        Node* tmpDummyNode = new Node();
        tmpDummyNode->isDummy = true;
        Node* root = oram->ReadWriteTest(rt->key, tmpDummyNode, rt->pos, rt->pos, true, false, true);
        res.push_back(root->key.getValue());
        Node *l = nullptr, *r = nullptr;
        if (!root->leftID.isZero()) {
            l = oram->ReadWriteTest(root->leftID, tmpDummyNode, root->leftPos, root->leftPos, true, false, true);
            preOrderKeys(l, res);
        }
        if (!root->rightID.isZero()) {
            l = oram->ReadWriteTest(root->rightID, tmpDummyNode, root->rightPos, root->rightPos, true, false, true);
            preOrderKeys(l, res);
        }
        delete tmpDummyNode;
        delete root;
        delete l;
        delete r;
    }
}

void AVLTree::printTree(Node* rt, int indent) {
    if (rt != 0 && rt->key != 0) {
        Node* tmpDummyNode = new Node();
        tmpDummyNode->isDummy = true;
        Node* root = oram->ReadWriteTest(rt->key, tmpDummyNode, rt->pos, rt->pos, true, false, true);

        if (root->leftID != 0)
            printTree(oram->ReadWriteTest(root->leftID, tmpDummyNode, root->leftPos, root->leftPos, true, false, true), indent + 4);
        if (indent > 0) {
            for (int i = 0; i < indent; i++) {
                printf(" ");
            }
        }

        string value;
        value.assign(root->value.begin(), root->value.end());
        printf("Key: %lld Height:%lld Pos:%lld LeftID:%lld LeftPos:%lld RightID:%lld RightPos:%lld\n", root->key.getValue(), root->height, root->pos, root->leftID.getValue(), root->leftPos, root->rightID.getValue(), root->rightPos);
        if (root->rightID != 0)
            printTree(oram->ReadWriteTest(root->rightID, tmpDummyNode, root->rightPos, root->rightPos, true, false, true), indent + 4);
        delete tmpDummyNode;
        delete rt;
        delete root;
    }
}

/*
 * before executing each operation, this function should be called with proper arguments
 */
void AVLTree::startOperation(bool batchWrite) {
    oram->start(batchWrite);
    totheight = 0;
    exist = false;
    doubleRotation = false;
}

/*
 * after executing each operation, this function should be called with proper arguments
 */
void AVLTree::finishOperation() {
    for (auto item : avlCache) {
        delete item;
    }
    avlCache.clear();
    oram->finilize();
}

unsigned long long AVLTree::RandomPath() {
    uint32_t val;
    sgx_read_rand((unsigned char *) &val, 4);
    return val % (maxOfRandom);
}

AVLTree::AVLTree(long long maxSize, bytes<Key> secretkey, Bid& rootKey, unsigned long long& rootPos, map<Bid, string>* pairs, map<unsigned long long, unsigned long long>* permutation) {
    int depth = (int) (ceil(log2(maxSize)) - 1) + 1;
    maxOfRandom = (long long) (pow(2, depth));
    times.push_back(vector<double>());
    times.push_back(vector<double>());
    times.push_back(vector<double>());
    times.push_back(vector<double>());
    times.push_back(vector<double>());

    vector<Node*> nodes;
    for (auto pair : (*pairs)) {
        Node* node = newNode(pair.first, pair.second);
        nodes.push_back(node);
    }

    int nextPower2 = (int) pow(2, ceil(log2(nodes.size())));
    for (int i = (int) nodes.size(); i < nextPower2; i++) {
        Bid bid = INF + i;
        Node* node = newNode(bid, "");
        node->isDummy = false;
        nodes.push_back(node);
    }

    bitonicSort(&nodes);
    double t;
    printf("Creating BST of %d Nodes\n", nodes.size());
    ocall_start_timer(53);
    sortedArrayToBST(&nodes, 0, nodes.size() - 2, rootPos, rootKey, permutation);
    ocall_stop_timer(&t, 53);
    times[0].push_back(t);
    printf("Inserting in ORAM\n");

    double gp;
    int size = (int) nodes.size();
    for (int i = size; i < maxOfRandom * Z; i++) {
        Bid bid;
        bid = INF + i;
        Node* node = newNode(bid, "");
        node->isDummy = true;
        node->pos = (*permutation)[permutationIterator];
        permutationIterator++;
        nodes.push_back(node);
    }
    ocall_start_timer(53);
    oram = new ORAM(maxSize, secretkey, &nodes);
    ocall_stop_timer(&t, 53);
    times[1].push_back(t);
}

int AVLTree::sortedArrayToBST(vector<Node*>* nodes, long long start, long long end, unsigned long long& pos, Bid& node, map<unsigned long long, unsigned long long>* permutation) {
    if (start > end) {
        pos = -1;
        node = 0;
        return 0;
    }

    unsigned long long mid = (start + end) / 2;
    Node *root = (*nodes)[mid];

    int leftHeight = sortedArrayToBST(nodes, start, mid - 1, root->leftPos, root->leftID, permutation);

    int rightHeight = sortedArrayToBST(nodes, mid + 1, end, root->rightPos, root->rightID, permutation);
    root->pos = (*permutation)[permutationIterator];
    permutationIterator++;
    root->height = max(leftHeight, rightHeight) + 1;
    pos = root->pos;
    node = root->key;
    return root->height;
}

Node* AVLTree::readWriteCacheNode(Bid bid, Node* inputnode, bool isRead, bool isDummy) {
    Node* tmpWrite = Node::clone(inputnode);

    Node* res = new Node();
    res->isDummy = true;
    res->index = oram->nextDummyCounter++;
    res->key = oram->nextDummyCounter++;
    bool write = !isRead;

    for (Node* node : avlCache) {
        bool match = Node::CTeq(Bid::CTcmp(node->key, bid), 0) && !node->isDummy;

        node->isDummy = Node::conditional_select(true, node->isDummy, !isDummy && match && write);
        bool choice = !isDummy && match && isRead && !node->isDummy;
        res->index = Node::conditional_select((long long) node->index, (long long) res->index, choice);
        res->isDummy = Node::conditional_select(node->isDummy, res->isDummy, choice);
        res->pos = Node::conditional_select((long long) node->pos, (long long) res->pos, choice);
        for (int k = 0; k < res->value.size(); k++) {
            res->value[k] = Node::conditional_select(node->value[k], res->value[k], choice);
        }
        for (int k = 0; k < res->dum.size(); k++) {
            res->dum[k] = Node::conditional_select(node->dum[k], res->dum[k], choice);
        }
        res->evictionNode = Node::conditional_select(node->evictionNode, res->evictionNode, choice);
        res->modified = Node::conditional_select(true, res->modified, choice);
        res->height = Node::conditional_select(node->height, res->height, choice);
        res->leftPos = Node::conditional_select(node->leftPos, res->leftPos, choice);
        res->rightPos = Node::conditional_select(node->rightPos, res->rightPos, choice);
        for (int k = 0; k < res->key.id.size(); k++) {
            res->key.id[k] = Node::conditional_select(node->key.id[k], res->key.id[k], choice);
        }
        for (int k = 0; k < res->leftID.id.size(); k++) {
            res->leftID.id[k] = Node::conditional_select(node->leftID.id[k], res->leftID.id[k], choice);
        }
        for (int k = 0; k < res->rightID.id.size(); k++) {
            res->rightID.id[k] = Node::conditional_select(node->rightID.id[k], res->rightID.id[k], choice);
        }
    }

    avlCache.push_back(tmpWrite);
    return res;
}

void AVLTree::bitonicSort(vector<Node*>* nodes) {
    int len = nodes->size();
    bitonic_sort(nodes, 0, len, 1);
}

void AVLTree::bitonic_sort(vector<Node*>* nodes, int low, int n, int dir) {
    if (n > 1) {
        int middle = n / 2;
        bitonic_sort(nodes, low, middle, !dir);
        bitonic_sort(nodes, low + middle, n - middle, dir);
        bitonic_merge(nodes, low, n, dir);
    }
}

void AVLTree::bitonic_merge(vector<Node*>* nodes, int low, int n, int dir) {
    if (n > 1) {
        int m = greatest_power_of_two_less_than(n);

        for (int i = low; i < (low + n - m); i++) {
            if (i != (i + m)) {
                compare_and_swap((*nodes)[i], (*nodes)[i + m], dir);
            }
        }

        bitonic_merge(nodes, low, m, dir);
        bitonic_merge(nodes, low + m, n - m, dir);
    }
}

void AVLTree::compare_and_swap(Node* item_i, Node* item_j, int dir) {
    int res = Bid::CTcmp(item_i->key, item_j->key);
    int cmp = Node::CTeq(res, 1);
    Node::conditional_swap(item_i, item_j, Node::CTeq(cmp, dir));
}

int AVLTree::greatest_power_of_two_less_than(int n) {
    int k = 1;
    while (k > 0 && k < n) {
        k = k << 1;
    }
    return k >> 1;
}
