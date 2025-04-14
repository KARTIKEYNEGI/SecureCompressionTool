# Secure Compression Tool v1.0

A C++ CLI-based compression tool with built-in encryption, metadata handling, and support for binary files.

## 📦 Requirements

- ✅ C++17 compiler (e.g., g++ or clang++)
- ✅ nlohmann files
  - 👉 Place the file in the same directory as this tool (\nlohmann)

**Optional (for large files):**
- ✅ 16GB+ RAM recommended
- ✅ 64-bit OS

## 🔧 Compilation Instructions

Compile using g++:
```bash
g++ main.cpp -std=c++17 -o main
```

## 🚀 Usage Instructions

### Step 1️⃣ - Run the program:
```bash
./main
```

### Step 2️⃣ - Authenticate
- 🔐 Username: admin
- 🔐 Password: admin123

### Step 3️⃣ - Choose Operation
- [1] Compress
- [2] Decompress

### Step 4️⃣ - Enter XOR Key (e.g. "123x456x")
- 💡 This is used for encryption and decryption. Use the same key to decompress later.

## 📁 Compression Flow

- 👉 Input: any file (TXT, CSV, JSON, BIN, PDF, etc.)
- 👉 You'll be asked:
  - File path
  - Compression method: "huffman" or "shannon"
- ✅ Output:
  - Compressed binary: `<filename>.bin`
  - Metadata: `<filename>.meta`

## 📂 Decompression Flow

- 👉 Required:
  - Same XOR key used during compression
  - Original file name (e.g., "sample.csv")
- ✅ Output:
  - Restored file written as original (e.g., "sample.csv")

## ⚠️ Notes

- DO NOT delete the .meta file — it's required for decompression.
- The XOR key must be remembered — there's no recovery mechanism.
- Large files (1GB+) may take time depending on system resources.

## 🛠️ Support / Troubleshooting

- Make sure you're using the correct XOR key.
- Ensure the JSON metadata and binary file are not corrupted or renamed.
- Run in a terminal/console that supports UTF-8 characters.
