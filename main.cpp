// data_compression_tool.cpp
// Corrected and enhanced version handling binary files,
// proper XOR encryption, metadata management, and saving output
// files using the original file name.
// Compile with: g++ data_compression_tool.cpp -std=c++17 -I. -o compress_tool
// Requires nlohmann/json.hpp (download the single-header version from https://github.com/nlohmann/json)

#include <iostream>
#include <fstream>
#include <map>
#include <queue>
#include <vector>
#include <string>
#include <bitset>
#include <algorithm>
#include <filesystem>
#include <sstream>
#include <cstdint>
#include <stdexcept>
#include "nlohmann/json.hpp"
using json = nlohmann::json;

using namespace std;
namespace fs = std::filesystem;

// ============================== User Authentication ==============================
map<string, string> user_db = {
    {"admin", "admin123"},
    {"user", "pass"}
};

bool authenticate(const string& username, const string& password) {
    return user_db.count(username) && user_db[username] == password;
}

// ============================== XOR Encryption ==============================
vector<uint8_t> xor_encrypt(const vector<uint8_t>& data, const string& key) {
    vector<uint8_t> result;
    size_t key_len = key.length();
    for (size_t i = 0; i < data.size(); ++i) {
        result.push_back(data[i] ^ static_cast<uint8_t>(key[i % key_len]));
    }
    return result;
}

// ============================== File Handling ==============================
vector<uint8_t> read_file(const string& filename) {
    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        throw runtime_error("Error: Could not open file: " + filename);
    }
    return vector<uint8_t>(istreambuf_iterator<char>(file), {});
}

void write_file(const string& filename, const vector<uint8_t>& data) {
    ofstream file(filename, ios::binary);
    if (!file.is_open()) {
        throw runtime_error("Error: Could not write to file: " + filename);
    }
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
}

// ============================== Bit Utility ==============================
vector<uint8_t> bits_to_bytes(const string& bitstring) {
    vector<uint8_t> bytes;
    for (size_t i = 0; i < bitstring.size(); i += 8) {
        string byteStr = bitstring.substr(i, 8);
        byteStr.resize(8, '0'); // Pad with zeros if necessary
        bytes.push_back(static_cast<uint8_t>(bitset<8>(byteStr).to_ulong()));
    }
    return bytes;
}

string bytes_to_bits(const vector<uint8_t>& bytes) {
    string bitstring;
    for (uint8_t b : bytes) {
        bitstring += bitset<8>(b).to_string();
    }
    return bitstring;
}

// ============================== Huffman Encoding/Decoding ==============================
struct HuffmanNode {
    uint8_t ch;
    int freq;
    HuffmanNode* left;
    HuffmanNode* right;
    HuffmanNode(uint8_t c, int f) : ch(c), freq(f), left(nullptr), right(nullptr) {}
};

struct CompareNode {
    bool operator()(HuffmanNode* l, HuffmanNode* r) {
        return l->freq > r->freq;
    }
};

void build_huffman_codes(HuffmanNode* root, const string& str, map<uint8_t, string>& huff_codes) {
    if (!root) return;
    if (!root->left && !root->right) {
        huff_codes[root->ch] = str;
    }
    build_huffman_codes(root->left, str + "0", huff_codes);
    build_huffman_codes(root->right, str + "1", huff_codes);
}

HuffmanNode* build_huffman_tree(const map<uint8_t, int>& freq) {
    priority_queue<HuffmanNode*, vector<HuffmanNode*>, CompareNode> pq;
    for (const auto& pair : freq) {
        pq.push(new HuffmanNode(pair.first, pair.second));
    }
    while (pq.size() > 1) {
        HuffmanNode* left = pq.top(); pq.pop();
        HuffmanNode* right = pq.top(); pq.pop();
        HuffmanNode* merged = new HuffmanNode('\0', left->freq + right->freq);
        merged->left = left;
        merged->right = right;
        pq.push(merged);
    }
    return pq.top();
}

string huffman_compress(const vector<uint8_t>& data, map<uint8_t, string>& codes, map<uint8_t, int>& freq) {
    for (uint8_t b : data) {
        freq[b]++;
    }
    HuffmanNode* root = build_huffman_tree(freq);
    build_huffman_codes(root, "", codes);
    string compressed;
    for (uint8_t b : data) {
        compressed += codes[b];
    }
    return compressed;
}

vector<uint8_t> huffman_decompress(const string& compressed, HuffmanNode* root) {
    vector<uint8_t> result;
    HuffmanNode* curr = root;
    for (char bit : compressed) {
        curr = (bit == '0') ? curr->left : curr->right;
        if (!curr->left && !curr->right) {
            result.push_back(curr->ch);
            curr = root;
        }
    }
    return result;
}

// ============================== Shannon–Fano Encoding/Decoding ==============================
struct SFNode {
    uint8_t ch;
    int freq;
    string code;
};

bool sf_sort(const SFNode &a, const SFNode &b) {
    return a.freq > b.freq;
}

void shannon_fano_rec(vector<SFNode>& symbols, int l, int r) {
    if (l >= r) return;
    int total = 0;
    for (int i = l; i <= r; ++i) total += symbols[i].freq;
    int sum = 0, split = l;
    for (int i = l; i <= r; ++i) {
        sum += symbols[i].freq;
        if (sum >= total / 2) {
            split = i;
            break;
        }
    }
    for (int i = l; i <= split; ++i) symbols[i].code += "0";
    for (int i = split + 1; i <= r; ++i) symbols[i].code += "1";
    shannon_fano_rec(symbols, l, split);
    shannon_fano_rec(symbols, split + 1, r);
}

string shannon_fano_compress(const vector<uint8_t>& data, map<uint8_t, string>& codes, map<uint8_t, int>& freq) {
    for (uint8_t b : data) freq[b]++;
    vector<SFNode> symbols;
    for (const auto& pair : freq) {
        symbols.push_back({pair.first, pair.second, ""});
    }
    sort(symbols.begin(), symbols.end(), sf_sort);
    shannon_fano_rec(symbols, 0, symbols.size() - 1);
    for (const SFNode& node : symbols) {
        codes[node.ch] = node.code;
    }
    string compressed;
    for (uint8_t b : data) {
        compressed += codes[b];
    }
    return compressed;
}

vector<uint8_t> shannon_fano_decompress(const string& compressed, const map<uint8_t, string>& codes) {
    map<string, uint8_t> reverse_codes;
    for (const auto& pair : codes) {
        reverse_codes[pair.second] = pair.first;
    }
    vector<uint8_t> result;
    string current_code;
    for (char bit : compressed) {
        current_code += bit;
        if (reverse_codes.find(current_code) != reverse_codes.end()) {
            result.push_back(reverse_codes[current_code]);
            current_code.clear();
        }
    }
    return result;
}

// ============================== Compression Ratio ==============================
double compression_ratio(size_t original_size, size_t compressed_size) {
    return 100.0 * (1.0 - (static_cast<double>(compressed_size) / original_size));
}

// ============================== Compression and Decompression ==============================
void compress_data(const string& filename, const string& method, const string& xor_key) {
    vector<uint8_t> data = read_file(filename);
    map<uint8_t, string> codes;
    map<uint8_t, int> freq;
    string compressed_bits;

    if (method == "huffman") {
        compressed_bits = huffman_compress(data, codes, freq);
    } else if (method == "shannon") {
        compressed_bits = shannon_fano_compress(data, codes, freq);
    } else {
        cerr << "Unknown compression method!" << endl;
        return;
    }

    vector<uint8_t> compressed_bytes = bits_to_bytes(compressed_bits);
    vector<uint8_t> encrypted_bytes = xor_encrypt(compressed_bytes, xor_key);

    fs::path filePath(filename);
    string base = filePath.filename().string();
    string compFileName = base + ".bin";
    string metaFileName = base + ".meta";

    write_file(compFileName, encrypted_bytes);

    json meta;
    fs::path p(filename);
    meta["method"] = method;
    meta["original_filename"] = p.filename().string();
    meta["original_extension"] = p.extension().string();
    meta["compressed_bits_length"] = compressed_bits.size();

    json freq_json;
    for (const auto& pair : freq) {
        freq_json[to_string(pair.first)] = pair.second;
    }
    meta["freq"] = freq_json;

    ofstream metafile(metaFileName);
    metafile << meta.dump(4);

    cout << "Compression completed." << endl;
    cout << "Compression Ratio: " << compression_ratio(data.size() * 8, compressed_bits.size()) << "%" << endl;
    cout << "Compressed file saved as: " << compFileName << endl;
    cout << "Metadata saved as: " << metaFileName << endl;
}
void decompress_data(const string& xor_key, const string& originalFilename) {
    string compFileName = originalFilename + ".bin";
    string metaFileName = originalFilename + ".meta";

    vector<uint8_t> encrypted_bytes = read_file(compFileName);
    vector<uint8_t> compressed_bytes = xor_encrypt(encrypted_bytes, xor_key);  // Decrypt the data
    string compressed_bits_full = bytes_to_bits(compressed_bytes);  // Convert back to bitstring

    ifstream metafile(metaFileName);
    json meta;
    metafile >> meta;
    size_t compressed_bits_length = meta["compressed_bits_length"];
    string compressed_bits = compressed_bits_full.substr(0, compressed_bits_length);  // Extract correct bitstring length

    map<uint8_t, int> freq;
    for (auto& el : meta["freq"].items()) {
        uint8_t key = static_cast<uint8_t>(stoi(el.key()));
        freq[key] = el.value();
    }

    vector<uint8_t> decompressed;

    if (meta["method"] == "huffman") {
        HuffmanNode* root = build_huffman_tree(freq);
        decompressed = huffman_decompress(compressed_bits, root);
    } else if (meta["method"] == "shannon") {
        map<uint8_t, string> codes;
        vector<SFNode> symbols;
        for (const auto& pair : freq) {
            symbols.push_back({pair.first, pair.second, ""});
        }
        sort(symbols.begin(), symbols.end(), sf_sort);
        shannon_fano_rec(symbols, 0, symbols.size() - 1);
        for (const SFNode& node : symbols)
            codes[node.ch] = node.code;
        decompressed = shannon_fano_decompress(compressed_bits, codes);
    } else {
        throw runtime_error("Unsupported method in metadata.");
    }

    write_file(originalFilename, decompressed);  // Write the decompressed data to the original file
    cout << "Decompression complete. Output written to " << originalFilename << endl;
}

// ============================== Main ==============================
int main() {
    string username, password;
    cout << "Username: "; 
    cin >> username;
    cout << "Password: "; 
    cin >> password;
    if (!authenticate(username, password)) {
        cerr << "Authentication failed!" << endl;
        return 1;
    }

    int choice;
    cout << "1. Compress\n2. Decompress\nChoose an option: ";
    cin >> choice;
    cin.ignore(); 

    string xor_key;
    cout << "Enter XOR key (string): ";
    cin.ignore();
    getline(cin, xor_key);

    if (choice == 1) {
        string filename, method;
        cout << "Enter file path to compress: ";
        getline(cin, filename);
        cout << "Enter compression method (huffman/shannon): ";
        cin >> method;
        compress_data(filename, method, xor_key);
    } else if (choice == 2) {
        string originalFilename;
        cout << "Enter original file name (e.g., Practice Sheet 2.pdf): ";
       
        getline(cin, originalFilename);
        decompress_data(xor_key, originalFilename);
    } else {
        cerr << "Invalid choice." << endl;
    }
    return 0;
}