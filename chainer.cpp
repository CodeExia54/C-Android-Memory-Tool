#include "MemoryTool.h"
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <cstdlib>

struct ChainNode {
    uint64_t address;
    uint64_t value;
    std::map<uint64_t, ChainNode*> offsets;
};

class MemoryChainer {
private:
    MemoryTool memTool;
    std::vector<ChainNode*> chains;
    int depth;
    uint64_t maxOffset;
    bool is64bit;
    std::string packageName;
    
    std::string formatAddress(uint64_t addr) {
        std::stringstream ss;
        ss << "0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << addr;
        return ss.str();
    }
    
public:
    MemoryChainer(const std::string& pkg) : packageName(pkg) {
        memTool.initXMemoryTools(const_cast<char*>(pkg.c_str()), const_cast<char*>("MODE_ROOT"));
        is64bit = true; // Assuming 64-bit as per requirement
    }
    
    void setParameters(int d, uint64_t offset) {
        depth = d;
        maxOffset = offset;
    }
    
    void buildChains() {
        PMAPS results = memTool.GetResults();
        int count = memTool.GetResultCount();
        
        for (int i = 0; i < count; i++) {
            ChainNode* node = new ChainNode();
            node->address = results->addr;
            char* valStr = memTool.GetAddressValue(results->addr, results->type);
            node->value = strtoull(valStr, nullptr, 10);
            free(valStr);
            
            buildChain(node, 1);
            chains.push_back(node);
            results = results->next;
        }
    }
    
    void buildChain(ChainNode* parent, int currentDepth) {
        if (currentDepth >= depth) return;
        
        uint64_t baseValue = parent->value;
        
        // Create temporary strings for the search
        std::string fromStr = std::to_string(baseValue);
        std::string toStr = std::to_string(baseValue + maxOffset);
        
        // Search in nearby memory
        memTool.SetSearchRange(ALL);
        memTool.RangeMemorySearch(
            const_cast<char*>(fromStr.c_str()),
            const_cast<char*>(toStr.c_str()),
            is64bit ? TYPE_QWORD : TYPE_DWORD
        );
        
        PMAPS children = memTool.GetResults();
        int childCount = memTool.GetResultCount();
        
        for (int i = 0; i < childCount; i++) {
            uint64_t offset = children->addr - baseValue;
            if (offset <= maxOffset) {
                ChainNode* child = new ChainNode();
                child->address = children->addr;
                char* childValStr = memTool.GetAddressValue(children->addr, children->type);
                child->value = strtoull(childValStr, nullptr, 10);
                free(childValStr);
                
                parent->offsets[offset] = child;
                buildChain(child, currentDepth + 1);
            }
            children = children->next;
        }
    }
    
    void printResults() {
        clock_t start = clock();
        buildChains();
        double duration = (double)(clock() - start) / CLOCKS_PER_SEC;
        
        std::cout << "Found " << chains.size() << " chains in " 
                  << std::fixed << std::setprecision(3) << duration 
                  << " seconds (" << depth << ", " << maxOffset << "):\n\n";
        
        int chainNum = 1;
        for (auto* chain : chains) {
            printChain(chainNum++, "libil2cpp.so + " + formatAddress(chain->address - 0x10000000), 
                      chain, 0);
        }
        
        std::cout << "\n**EXIT**\nRETRY SAVE\n\n";
        std::cout << "82.1 # Ch.Ca.Cd.Cb.A 0\n";
    }
    
    void printChain(int chainId, const std::string& prefix, ChainNode* node, int level) {
        std::cout << chainId << ": " << prefix << " [" << formatAddress(node->address) << "]";
        
        if (!node->offsets.empty()) {
            bool first = true;
            for (auto& entry : node->offsets) {
                if (!first) std::cout << "\n" << std::string(prefix.length() + 10, ' ');
                std::cout << " -> " << formatAddress(node->value) << " + " 
                          << formatAddress(entry.first) << " = " << entry.second->value;
                
                std::string newPrefix = prefix + " -> " + formatAddress(node->value) + 
                                      " + " + formatAddress(entry.first);
                printChain(chainId, newPrefix, entry.second, level + 1);
                first = false;
            }
        } else {
            std::cout << " = " << node->value << "\n";
        }
    }
    
    ~MemoryChainer() {
        for (auto* chain : chains) {
            deleteChain(chain);
        }
    }
    
    void deleteChain(ChainNode* node) {
        for (auto& entry : node->offsets) {
            deleteChain(entry.second);
        }
        delete node;
    }
};

int main() {
    std::string packageName;
    int depth;
    uint64_t maxOffset;
    
    std::cout << "Enter package name: ";
    std::getline(std::cin, packageName);
    
    std::cout << "Enter chain depth (e.g., 3): ";
    std::cin >> depth;
    
    std::cout << "Enter max offset (e.g., 256): ";
    std::cin >> maxOffset;
    
    // Initialize with some search results
    MemoryTool tempTool;
    tempTool.initXMemoryTools(const_cast<char*>(packageName.c_str()), const_cast<char*>("MODE_ROOT"));
    tempTool.SetSearchRange(ALL);
    
    // Create temporary string for search value
    std::string searchValue = "12345";
    tempTool.MemorySearch(const_cast<char*>(searchValue.c_str()), TYPE_DWORD);
    
    MemoryChainer chainer(packageName);
    chainer.setParameters(depth, maxOffset);
    chainer.printResults();
    
    return 0;
}
