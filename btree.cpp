#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

#define UNIT 4 // the unit is 4 bytes
using namespace std;

struct path {
    int dest;
    vector<int> p;
};

struct my_entry {
    int key;
    int value;
};

my_entry getInputs(const char *); // parsing the input

class BPlusTree {
public:
    int BlockSize, RootBID, Depth; // metadata in header
    int B; // entry number of that can fit in one block
    int numOfNode;

    fstream binFile;

    BPlusTree() {}

    ~BPlusTree() {
        binFile.close();
    }

    void getMetadata(const char *); // get metadata in header

    path getPath(int); // get path to the key, and location of the key

    void insert(my_entry); // insert entry to B+tree

    void insertEmpty(my_entry); // insert when there isn't any node

    void insertIntoOnlyOne(my_entry); // insert when there is only one node

    my_entry insertLeaf(my_entry, int); // insert into leaf node

    my_entry propagate(my_entry e, int cur); // insertion propagates up to its parent node

    int search(int key); // search by a data

    vector<my_entry> rangeSearch(int , int ); // search by range

    void print(char *fileName); // print nodes that depth is 0 or 1

};

int main(int argc, char *argv[]) {
    ofstream outFile;
    ifstream inFile;
    char cmd = argv[1][0];
    char *binFile = argv[2];
    BPlusTree *bpt = new BPlusTree();
    int initialNum = 0;
    char inputs[30];

    switch (cmd) {
        case 'c': // creation
        {
            int blockSize = stoi(argv[3]);
            outFile.open(argv[2], ios::binary);
            outFile.write(reinterpret_cast<char *>(&blockSize), UNIT);
            outFile.write(reinterpret_cast<char *>(&initialNum), UNIT);
            outFile.write(reinterpret_cast<char *>(&initialNum), UNIT);
            outFile.close();
            break;
        }
        case 'i': // insertion
        {
            bpt->getMetadata(binFile);
            inFile.open(argv[3]);
            while (inFile.getline(inputs, sizeof(inputs))) {
                my_entry e = getInputs(inputs);
                bpt->insert(e);
            }
            inFile.close();
            break;
        }

        case 's': // point search
        {
            bpt->getMetadata(binFile);
            inFile.open(argv[3]);
            outFile.open(argv[4]);

            while (inFile.getline(inputs, sizeof(inputs))) {
                int i = 0;
                int key = 0;
                int value = 0;
                while (inputs[i] != '\0' && inputs[i] != '\n') {
                    if ('0' <= inputs[i] && inputs[i] <= '9')
                        key = key * 10 + (inputs[i] - '0');
                    i++;
                }
                value = bpt->search(key);
                if (key != 0)
                    outFile << key << "|" << value << "\n";
            }
            inFile.close();
            outFile.close();
            break;
        }

        case 'r': // range search
        {
            bpt->getMetadata(binFile);
            inFile.open(argv[3]);
            outFile.open(argv[4]);
            string str;
            while (inFile.peek() != EOF) {
                int beg = 0, end = 0;
                vector<string> res;
                string buf;

                getline(inFile, str);
                istringstream iss(str);

                while (getline(iss, buf, '-')) {
                    res.emplace_back(buf);
                }
                beg = stoi(res[0]);
                end = stoi(res[1]);
                vector<my_entry> v = bpt->rangeSearch(beg, end);
                for (auto &e: v) {
                    outFile << e.key << "|" << e.value << " ";
                }
                outFile << '\n';

            }
            inFile.close();
            outFile.close();
            break;
        }
        case 'p': // print
        {
            bpt->getMetadata(binFile);
            bpt->print(argv[3]);
            break;
        }
    }
    return 0;
}

my_entry getInputs(const char *inputs) { // parsing the input
    int i = 0, key = 0, value = 0;
    while (inputs[i] != '|') {
        key = key * 10 + (inputs[i] - '0');
        i++;
    }

    i++;
    while (inputs[i] != '\0' && inputs[i] != '\n') {
        if ('0' <= inputs[i] && inputs[i] <= '9') {
            value = value * 10 + (inputs[i] - '0');
        }
        i++;
    }
    return {key, value};
}

void BPlusTree::getMetadata(const char *fileName) { // get metadata in header
    string strFileName = fileName;
    binFile.open(strFileName, ios::binary | ios::out | ios::in);

    binFile.read(reinterpret_cast<char *>(&BlockSize), UNIT);
    binFile.read(reinterpret_cast<char *>(&RootBID), UNIT);
    binFile.read(reinterpret_cast<char *>(&Depth), UNIT);

    binFile.seekg(0, ios::end);
    numOfNode = ((int) binFile.tellg() - 3 * UNIT) / BlockSize;
    B = (BlockSize - UNIT) / 8;

}

path BPlusTree::getPath(int key) { // get path to the key, and location of the key
    vector<int> path;
    int cur = RootBID;
    int d = 0;
    while (true) {
        vector<int> v;
        path.emplace_back(cur);
        if (d == this->Depth) break;
        binFile.seekg(((cur - 1) * BlockSize) + 3 * UNIT, ios::beg);
        for (int i = 0; i <= 2 * B; i++) {
            int tmp;
            binFile.read(reinterpret_cast<char *>(&tmp), UNIT);
            if (tmp == 0) break;
            v.emplace_back(tmp);
        }
        int i = 1;
        for (; i < v.size(); i += 2) {
            if (v[i] > key) break;
        }
        cur = v[i - 1];
        d++;
    }

    return {cur, path};
}

void BPlusTree::insert(my_entry e) { // insert entry into B+tree
    if (numOfNode == 0)
        insertEmpty(e); // There is no node.
    else if (numOfNode == 1) {
        insertIntoOnlyOne(e); // There is only one node.
    } else {
        vector<int> path;
        path = getPath(e.key).p;
        unsigned int i = path.size() - 1;

        my_entry tmp = insertLeaf(e, path.back()); // insert into leaf node

        while (i--) {
            if (tmp.key == -1 || tmp.value == -1) break;
            tmp = propagate(tmp, path[i]); // insertion propagates up to its parent node
        }

    }

    // update metadata in header
    binFile.seekp(UNIT, ios::beg);
    binFile.write(reinterpret_cast<char *>(&RootBID), UNIT);
    binFile.write(reinterpret_cast<char *>(&Depth), UNIT);
}

void BPlusTree::insertEmpty(my_entry e) { // insert when there isn't any node
    int dummy = 0;

    binFile.seekp(0, ios::end);

    binFile.write(reinterpret_cast<char *>(&e.key), UNIT);
    binFile.write(reinterpret_cast<char *>(&e.value), UNIT);

    for (int i = 1; i < B; i++) { // initialize the node
        binFile.write(reinterpret_cast<char *>(&dummy), UNIT);
        binFile.write(reinterpret_cast<char *>(&dummy), UNIT);
    }
    binFile.write(reinterpret_cast<char *>(&dummy), UNIT);
    numOfNode++;
    RootBID = 1;
}

void BPlusTree::insertIntoOnlyOne(my_entry e) { // insert when there is only one node

    int dummy = 0;
    binFile.seekg(12, ios::beg);

    vector<pair<int, int>> v;
    int n = B;
    while (n--) {
        my_entry tmp{};
        binFile.read(reinterpret_cast<char *>(&tmp.key), UNIT);
        binFile.read(reinterpret_cast<char *>(&tmp.value), UNIT);
        if (tmp.key == 0) break;
        v.emplace_back(tmp.key, tmp.value);
    }
    v.emplace_back(e.key, e.value);
    sort(v.begin(), v.end());

    unsigned int size = v.size();
    if (B >= size) { // don't need to split.
        binFile.seekp(12);
        for (auto &tmp: v) {
            binFile.write(reinterpret_cast<char *>(&tmp.first), UNIT);
            binFile.write(reinterpret_cast<char *>(&tmp.second), UNIT);
        }
    }
    // need to split
    else {
        int smaller = 1;
        int larger = 2;
        RootBID = 3; // rootID is ID of new node
        int mid = (B - 1) / 2 + 1;

        binFile.seekp(3 * UNIT, ios::beg);
        // smaller
        for (unsigned int i = 0; i < mid; i++) {
            binFile.write(reinterpret_cast<char *>(&v[i].first), UNIT);
            binFile.write(reinterpret_cast<char *>(&v[i].second), UNIT);
        }
        for (unsigned int i = mid; i < B; i++) {
            binFile.write(reinterpret_cast<char *>(&dummy), UNIT);
            binFile.write(reinterpret_cast<char *>(&dummy), UNIT);
        }

        binFile.write(reinterpret_cast<char *>(&larger), UNIT);
        // larger
        for (unsigned int i = mid; i < size; i++) {
            binFile.write(reinterpret_cast<char *>(&v[i].first), UNIT);
            binFile.write(reinterpret_cast<char *>(&v[i].second), UNIT);
        }
        for (unsigned int i = size; i < mid + B; i++) {
            binFile.write(reinterpret_cast<char *>(&dummy), UNIT);
            binFile.write(reinterpret_cast<char *>(&dummy), UNIT);
        }
        binFile.write(reinterpret_cast<char *>(&dummy), UNIT);

        // root
        binFile.write(reinterpret_cast<char *>(&smaller), UNIT);
        binFile.write(reinterpret_cast<char *>(&v[mid].first), UNIT);
        binFile.write(reinterpret_cast<char *>(&larger), UNIT);
        for (unsigned int i = 0; i < B - 1; i++) {
            binFile.write(reinterpret_cast<char *>(&dummy), UNIT);
            binFile.write(reinterpret_cast<char *>(&dummy), UNIT);
        }
        Depth++; // update depth
        numOfNode += 2; // update #nodes
    }

}

my_entry BPlusTree::insertLeaf(my_entry e, int cur) { // insert into leaf node
    vector<pair<int, int>> v;

    binFile.seekg((cur - 1) * BlockSize + 3 * UNIT);
    int n = B;
    while (n--) {
        int tmpKey, tmpValue;
        binFile.read(reinterpret_cast<char *>(&tmpKey), UNIT);
        binFile.read(reinterpret_cast<char *>(&tmpValue), UNIT);
        if (!tmpKey) break;
        v.emplace_back(tmpKey, tmpValue);
    }
    v.emplace_back(e.key, e.value);
    sort(v.begin(), v.end());

    unsigned int size = v.size();
    if (B >= size) { // don't need to split
        binFile.seekp((cur - 1) * BlockSize + 3 * UNIT, ios::beg);
        for (auto &tmp: v) {
            binFile.write(reinterpret_cast<char *>(&(tmp.first)), UNIT);
            binFile.write(reinterpret_cast<char *>(&(tmp.second)), UNIT);
        }

        my_entry ret = {-1, -1};
        return ret;
    } else { // leaf node split
        int dummy = 0;
        int mid = (B - 1) / 2 + 1;

        int larger;
        numOfNode += 1; // update #nodes
        binFile.read(reinterpret_cast<char *>(&larger), UNIT);
        binFile.seekp((cur - 1) * BlockSize + 3 * UNIT, ios::beg);
        vector<int> left;
        for (unsigned int i = 0; i < mid; i++) {
            binFile.write(reinterpret_cast<char *>(&v[i].first), UNIT);
            binFile.write(reinterpret_cast<char *>(&v[i].second), UNIT);
        }
        for (unsigned int i = mid; i < B; i++) {
            binFile.write(reinterpret_cast<char *>(&dummy), UNIT);
            binFile.write(reinterpret_cast<char *>(&dummy), UNIT);
        }
        int newBid = numOfNode; // new Block's ID
        binFile.write(reinterpret_cast<char *>(&newBid), UNIT);
        binFile.seekp(0, ios::end);

        for (unsigned int i = mid; i < size; i++) {
            binFile.write(reinterpret_cast<char *>(&v[i].first), UNIT);
            binFile.write(reinterpret_cast<char *>(&v[i].second), UNIT);
        }
        for (unsigned int i = size; i < mid + B; i++) {
            binFile.write(reinterpret_cast<char *>(&dummy), UNIT);
            binFile.write(reinterpret_cast<char *>(&dummy), UNIT);
        }
        binFile.write(reinterpret_cast<char *>(&larger), UNIT);

        my_entry ret = {v[mid].first, newBid};
        return ret;
    }

}

my_entry BPlusTree::propagate(my_entry e, int cur) { // insertion propagates up to its parent node

    vector<pair<int, int>> v;
    binFile.seekg((cur - 1) * BlockSize + 4 * UNIT, ios::beg);
    int n = B;
    while (n--) {
        my_entry tmp{};
        binFile.read(reinterpret_cast<char *>(&tmp.key), UNIT);
        binFile.read(reinterpret_cast<char *>(&tmp.value), UNIT);
        if (tmp.key == 0) break;
        v.emplace_back(tmp.key, tmp.value);
    }
    v.emplace_back(e.key, e.value);
    sort(v.begin(), v.end());

    unsigned int size = v.size();
    if (B >= size) { // don't need to split

        binFile.seekp((cur - 1) * BlockSize + 16, ios::beg);
        for (auto &tmp: v) {
            binFile.write(reinterpret_cast<char *>(&(tmp.first)), UNIT);
            binFile.write(reinterpret_cast<char *>(&(tmp.second)), UNIT);

        }
        my_entry ret{-1, -1};
        return ret;
    } else { // non leaf node split

        int dummy = 0;
        int mid = (B - 1) / 2 + 1;

        numOfNode += 1; // update #nodes

        binFile.seekp((cur - 1) * BlockSize + 4 * UNIT, ios::beg);
        for (unsigned int i = 0; i < mid; i++) {
            binFile.write(reinterpret_cast<char *>(&(v[i].first)), UNIT);
            binFile.write(reinterpret_cast<char *>(&(v[i].second)), UNIT);
        }
        for (unsigned int i = mid; i < B; i++) {
            binFile.write(reinterpret_cast<char *>(&dummy), UNIT);
            binFile.write(reinterpret_cast<char *>(&dummy), UNIT);
        }
        int newBid = numOfNode; // newBlock's ID

        binFile.seekp(0, ios::end);
        binFile.write(reinterpret_cast<char *>(&(v[mid].second)), UNIT);
        for (unsigned int i = mid + 1; i < size; i++) {
            binFile.write(reinterpret_cast<char *>(&(v[i].first)), UNIT);
            binFile.write(reinterpret_cast<char *>(&(v[i].second)), UNIT);
        }
        for (unsigned int i = size; i < mid + B + 1; i++) {
            binFile.write(reinterpret_cast<char *>(&dummy), UNIT);
            binFile.write(reinterpret_cast<char *>(&dummy), UNIT);
        }

        // root node split
        if (cur == RootBID) {

            binFile.write(reinterpret_cast<char *>(&cur), UNIT);
            binFile.write(reinterpret_cast<char *>(&(v[mid].first)), UNIT);
            binFile.write(reinterpret_cast<char *>(&(newBid)), UNIT);
            for (unsigned int i = 0; i < 2 * B - 2; i++) {
                binFile.write(reinterpret_cast<char *>(&dummy), UNIT);
            }

            Depth++; // update depth
            numOfNode++; // update #nodes
            RootBID = numOfNode; // root block's ID, root block is new node

            my_entry ret{-1, -1};
            return ret;
        }

        my_entry ret{v[mid].first, newBid};
        return ret;

    }

}

int BPlusTree::search(int key) { // search by a data

    int ret = -1; // find fail
    int n = B; // num of entry in block
    int loc = getPath(key).dest; // get location of the key
    binFile.seekg(((loc - 1) * BlockSize) + 3 * UNIT, ios::beg);

    while (n--) { // search all entries in a block
        my_entry tmp{};
        binFile.read(reinterpret_cast<char *>(&tmp.key), UNIT);
        binFile.read(reinterpret_cast<char *>(&tmp.value), UNIT);
        if (tmp.key == 0) break; // there's no more entry
        if (tmp.key == key) { // find success
            ret = tmp.value;
            break;
        }
    }

    return ret;

}

vector<my_entry> BPlusTree::rangeSearch(int first, int last) { // search by range

    int loc = getPath(first).dest; // get location of the key
    vector<my_entry> entries; // entries in range

    while (true) {
        binFile.seekg(((loc - 1) * BlockSize) + 3 * UNIT, ios::beg);
        bool flag = false;
        int n = B; // num of entry in a block
        while (n--) { // search all entries in a block
            my_entry tmp{};
            binFile.read(reinterpret_cast<char *>(&tmp.key), UNIT);
            binFile.read(reinterpret_cast<char *>(&tmp.value), UNIT);
            if (tmp.key != 0) { // there's more entry
                if (tmp.key >= first && tmp.key <= last) { // find success
                    entries.emplace_back(tmp);
                } else if (tmp.key > last) { // out of range
                    flag = true;
                    break;
                }
            }
        }
        if (flag) break; // out of range

        binFile.read(reinterpret_cast<char *>(&loc), UNIT);
        if (loc == 0) break;
    }
    return entries;
}

void BPlusTree::print(char *fileName) {
    binFile.seekg((RootBID - 1) * BlockSize + 3 * UNIT, ios::beg);  // 루트 방문
    vector<int> output; // write to output file
    vector<int> keys; // keys of data

    my_entry e{};
    int n = B; // num of entry in a block
    // Depth is 0
    while (n--) { // search all entries in a block
        binFile.read(reinterpret_cast<char *>(&e.key), UNIT);
        binFile.read(reinterpret_cast<char *>(&e.value), UNIT);
        keys.emplace_back(e.key);
        if (e.value == 0) break; // no more entry

        output.emplace_back(e.value);
    }

    ofstream outFile;
    outFile.open(fileName);
    // write
    outFile << "<0>\n\n";

    for (auto &o: output) {
        outFile << o;
        if (o == output.back()) outFile << "\n";
        else outFile << ",";
    }

    output.clear();
    // Depth is 1
    for (int key: keys) {
        // non-leaf node
        if (Depth <= 1) {
            binFile.seekg(((key - 1) * BlockSize) + 3 * UNIT, ios::beg);
        } else {
            binFile.seekg(((key - 1) * BlockSize) + 4 * UNIT, ios::beg);
        }
        n = B; // num of entry in a block
        while (n--) { // search all entries in a block
            binFile.read(reinterpret_cast<char *>(&e.key), UNIT);
            binFile.read(reinterpret_cast<char *>(&e.value), UNIT);
            if (e.key == 0) break; // no more entry
            output.emplace_back(e.key);
        }
    }
    //write
    outFile << "\n<1>\n\n";

    for (auto &o: output) {
        outFile << o;
        if (o == output.back()) outFile << "\n";
        else outFile << ",";
    }
    outFile.close();
}

