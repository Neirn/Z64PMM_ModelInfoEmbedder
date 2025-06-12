#include <iostream>
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <unordered_set>

namespace fs = std::filesystem;

using json = nlohmann::json;

const char MODEL_INFO_HEADER[] = "PLAYERMODELINFO";
const size_t INTERNAL_NAME_FIELD_SIZE = 64;
const size_t DISPLAY_NAME_FIELD_SIZE = 32;
const size_t AUTHOR_NAME_FIELD_SIZE = 64;
const uint8_t CURRENT_EMBED_VERSION = 1;

const size_t MODEL_INFO_HEADER_LOCATION = 0x5500;

struct EmbeddedModelInfo {
    char header[sizeof(MODEL_INFO_HEADER) - 1];
    char embedVersion;
    char internalName[INTERNAL_NAME_FIELD_SIZE];
    char displayName[DISPLAY_NAME_FIELD_SIZE];
    char authorName[AUTHOR_NAME_FIELD_SIZE];

    EmbeddedModelInfo() {
        std::memcpy(header, MODEL_INFO_HEADER, sizeof(this->header));
        embedVersion = CURRENT_EMBED_VERSION;
        std::memset(internalName, 0, sizeof(internalName));
        std::memset(displayName, 0, sizeof(internalName));
        std::memset(authorName, 0, sizeof(internalName));
    }
};

enum ReturnTypes {
    OK,
    TOO_MANY_ARGS,
    TOO_FEW_ARGS,
    INVALID_ZOBJ,
    INVALID_JSON,
    NO_ZOBJ,
    NO_JSON,
    INVALID_FILE,
};

bool writeValidString(const json &data, const std::string &key, size_t maxSize, std::string& result) {
    auto &val = data[key];

    if (!val.is_string()) {
        std::cerr << "json field '" << key << "' is not a string!\n";
        return false;
    }

    std::string s = val;

    if (s.length() > maxSize) {
        std::cerr << "json field '" << key << "' is too big! Max size: " << maxSize << '\n';
        return false;
    }

    result = s;

    return true;
}

const uintmax_t MIN_ZOBJ_SIZE = 0x5800;

const int EXPECTED_NUM_ARGS = 2;

int main(int argc, char *argv[]) {
    if (argc == 2) {
        std::string arg(argv[0]);
        if (arg == "-h" || arg == "-H") {
            std::cout << "Usage: Pass in a path to a zobj and a path to a json.\n";
        }
    }

    if (argc > 3) {
        std::cerr << "Too many arguments passed in!\n";
        return ReturnTypes::TOO_MANY_ARGS;
    }

    if (argc < 3) {
        std::cerr << "Too few arguments passed in!\n";
        return ReturnTypes::TOO_FEW_ARGS;
    }

    fs::path a(argv[1]);
    fs::path b(argv[2]);

    fs::directory_entry aDirEntry(a);
    fs::directory_entry bDirEntry(b);

    if (!aDirEntry.exists() || aDirEntry.is_directory()) {
        std::cerr << "First path passed in is not a file!\n";
        return ReturnTypes::INVALID_FILE;
    }

    if (!bDirEntry.exists() || bDirEntry.is_directory()) {
        std::cerr << "Second path passed in is not a file!\n";
        return ReturnTypes::INVALID_FILE;
    }

    uintmax_t aSize = aDirEntry.file_size();
    uintmax_t bSize = bDirEntry.file_size();

    if (aSize < MIN_ZOBJ_SIZE && bSize < MIN_ZOBJ_SIZE) {
        std::cerr << "Did not pass in a valid zobj! (Neither file >= 0x5800 bytes in size)\n";
        return ReturnTypes::NO_ZOBJ;
    }

    if (aSize >= MIN_ZOBJ_SIZE && bSize >= MIN_ZOBJ_SIZE) {
        std::cerr << "Did not pass in a valid json! (Both files >= 0x5800 bytes in size)\n";
        return ReturnTypes::NO_JSON;
    }

    fs::directory_entry &zobjEntry = aSize > bSize ? aDirEntry : bDirEntry;
    fs::directory_entry &jsonEntry = aSize > bSize ? bDirEntry : aDirEntry;

    const size_t ML64_HEADER_LOCATION = 0x5000;
    const size_t ML64_OBJECT_POOL_SIZE = 0x800;

    std::ifstream zobjStream(zobjEntry.path(), std::ios::binary | std::ios::ate);
    std::streamsize zobjSize = zobjStream.tellg();
    zobjStream.seekg(0, std::ios::beg);

    std::vector<char> zobj(zobjSize);
    if (!zobjStream.read(zobj.data(), zobjSize)) {
        std::cerr << "Could not read zobj " << zobjEntry.path() << '\n';
        return ReturnTypes::INVALID_ZOBJ;
    }

    const char MODLOADER64[] = "MODLOADER64";
    
    for (size_t i = 0; i < sizeof(MODLOADER64) - 1; ++i) {
        if (zobj[i + ML64_HEADER_LOCATION] != MODLOADER64[i]) {
            std::cerr << "Did not find ModLoader64 header in zobj!" << '\n';
            return ReturnTypes::INVALID_ZOBJ;
        }
    }

    std::ifstream jsonStream(jsonEntry.path());
    json modelInfo;

    try {
        modelInfo = json::parse(jsonStream, nullptr, false, true);
    } catch (const json::exception &e) {
        std::cerr << e.what() << '\n';
        return ReturnTypes::INVALID_JSON;
    }

    if (!modelInfo["embed_version"].is_number_integer()) {
        std::cerr << "embed_version field is not a number!\n";
        return ReturnTypes::INVALID_JSON;
    }

    if (modelInfo["embed_version"] > 0xFF) {
        std::cerr << "'embed_version' field is not a supported version!\n(Currently supported versions: 1)\n";
        return ReturnTypes::INVALID_JSON;
    }

    if (modelInfo["embed_version"] != 1) {
        std::cerr << "'embed_version' field is not a supported version!\n(Currently supported versions: 1)\n";
        return ReturnTypes::INVALID_JSON;
    }

    const size_t MAX_INTERNAL_NAME_LENGTH = INTERNAL_NAME_FIELD_SIZE - 1;

    std::string internalName;

    if (!writeValidString(modelInfo, "internal_name", MAX_INTERNAL_NAME_LENGTH, internalName)) {
        return ReturnTypes::INVALID_JSON;
    }

    if (internalName.empty()) {
        std::cerr << "'internal_name' cannot be empty!";
        return INVALID_JSON;
    }

    const size_t MAX_DISPLAY_NAME_LENGTH = DISPLAY_NAME_FIELD_SIZE - 1;

    std::string displayName;

    if (!writeValidString(modelInfo, "display_name", MAX_DISPLAY_NAME_LENGTH, displayName)) {
        return ReturnTypes::INVALID_JSON;
    }

    if (displayName.empty()) {
        displayName = internalName;
    }

    const size_t MAX_AUTHOR_NAME_LENGTH = AUTHOR_NAME_FIELD_SIZE - 1;

    std::string author;

    if (!writeValidString(modelInfo, "author", MAX_AUTHOR_NAME_LENGTH, author)) {
        return ReturnTypes::INVALID_JSON;
    }

    if (author == "") {
        author = "N/A";
    }

    EmbeddedModelInfo info;
    strncpy(info.internalName, internalName.c_str(), internalName.length());
    strncpy(info.displayName, displayName.c_str(), displayName.length());
    strncpy(info.authorName, author.c_str(), author.length());

    const size_t MODELINFO_OFFSET_FROM_ML64 = ML64_HEADER_LOCATION + 0x500;

    #define COPY_AND_MOVE_POS(infoMember) memcpy(zobj.data() + pos, infoMember, sizeof(infoMember)); pos += sizeof(infoMember)

    size_t pos = MODELINFO_OFFSET_FROM_ML64;
    COPY_AND_MOVE_POS(info.header);
    zobj[pos++] = info.embedVersion;
    COPY_AND_MOVE_POS(info.internalName);
    COPY_AND_MOVE_POS(info.displayName);
    COPY_AND_MOVE_POS(info.authorName);

    #undef COPY_AND_MOVE_POS

    std::ofstream outZobj(zobjEntry.path(), std::ios::binary);

    outZobj.write(zobj.data(), zobjSize);

    return ReturnTypes::OK;
}
