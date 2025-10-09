#include <cctype>
#include <cstring>
#include <fstream>
#include <iostream>

const int MAX_LINE_LENGTH = 4096;
const int MAX_KEY_LENGTH = 101;
const int MAX_VALUE_LENGTH = 101;
const int MAX_PAIRS = 1024;

struct KeyValue {
    char key[MAX_KEY_LENGTH];
    char value[MAX_VALUE_LENGTH];
};

void trim(char *str) {
    if (!str) return;

    char *start = str;
    while (std::isspace(static_cast<unsigned char>(*start))) {
        start++;
    }

    if (*start == '\0') {
        str[0] = '\0';
        return;
    }

    char *end = start + std::strlen(start) - 1;
    while (end > start && std::isspace(static_cast<unsigned char>(*end))) {
        end--;
    }

    *(end + 1) = '\0';

    if (start != str) {
        std::memmove(str, start, std::strlen(start) + 1);
    }
}

bool isValidWord(const char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        char c = str[i];
        if (!((c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') ||
              (c == '_'))) {
            return false;
        }
    }
    return true;
}

int findKey(KeyValue *dataArray, int dataCount, const char *key) {
    for (int i = 0; i < dataCount; i++) {
        if (strcmp(dataArray[i].key, key) == 0) {
            return i;
        }
    }
    return -1;
}

int main(int argc, char *argv[]) {
    const char *templatePath = nullptr;
    const char *dataPath = nullptr;
    const char *outputPath = nullptr;

    for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "--template=", 11) == 0) {
            templatePath = argv[i] + 11;
        } else if (strncmp(argv[i], "--data=", 7) == 0) {
            dataPath = argv[i] + 7;
        } else if (strncmp(argv[i], "--output=", 9) == 0) {
            outputPath = argv[i] + 9;
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            templatePath = argv[++i];
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            dataPath = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            outputPath = argv[++i];
        } else {
            std::cerr << "Неизвестный аргумент: " << argv[i] << "\n";
            return 2;
        }
    }

    if (!templatePath || !dataPath) {
        std::cerr << "Ошибка: обязательные параметры не заданы.\n";
        std::cerr << "Использование:\n"
                  << "  " << argv[0] << " -t <шаблон> -d <данные> [-o <выходной_файл>]\n"
                  << "или:\n"
                  << "  " << argv[0] << " --template=шаблон --data=данные [--output=файл]\n";
        return 2;
    }

    std::ifstream dataFile(dataPath);
    if (!dataFile.is_open()) {
        std::cerr << "Не удалось открыть файл данных\n";
        return 3;
    }

    KeyValue dataArray[MAX_PAIRS];
    int dataCount = 0;

    char lineBuffer[MAX_LINE_LENGTH];

    while (dataFile.getline(lineBuffer, MAX_LINE_LENGTH)) {
        trim(lineBuffer);

        if (lineBuffer[0] == '#' || (lineBuffer[0] == '/' && lineBuffer[1] == '/')) {
            continue;
        }

        char *equalSign = strchr(lineBuffer, '=');
        if (!equalSign) {
            continue;
        }

        char *keyStr = lineBuffer;
        char *valueStr = equalSign + 1;
        *equalSign = '\0';

        trim(keyStr);
        trim(valueStr);

        if (!isValidWord(keyStr) || !isValidWord(valueStr)) {
            continue;
        }

        int pos = findKey(dataArray, dataCount, keyStr);
        if (pos >= 0) {
            strncpy(dataArray[pos].value, valueStr, MAX_VALUE_LENGTH - 1);
            dataArray[pos].value[MAX_VALUE_LENGTH - 1] = '\0';
        } else {
            if (dataCount >= MAX_PAIRS) {
                std::cerr << "Превышен лимит пар данных\n";
                return 5;
            }
            strncpy(dataArray[dataCount].key, keyStr, MAX_KEY_LENGTH - 1);
            dataArray[dataCount].key[MAX_KEY_LENGTH - 1] = '\0';

            strncpy(dataArray[dataCount].value, valueStr, MAX_VALUE_LENGTH - 1);
            dataArray[dataCount].value[MAX_VALUE_LENGTH - 1] = '\0';

            dataCount++;
        }
    }

    dataFile.close();

    std::ifstream templateFile(templatePath);
    if (!templateFile.is_open()) {
        std::cerr << "Не удалось открыть файл шаблона\n";
        return 3;
    }

    std::ostream *out = &std::cout;
    std::ofstream outputFile;
    if (outputPath) {
        outputFile.open(outputPath);
        if (!outputFile.is_open()) {
            std::cerr << "Не удалось открыть файл для записи\n";
            return 3;
        }
        out = &outputFile;
    }

    while (templateFile.getline(lineBuffer, MAX_LINE_LENGTH)) {
        char result[MAX_LINE_LENGTH] = "";
        int resultPos = 0;
        int i = 0;

        while (lineBuffer[i] != '\0' && lineBuffer[i] != '\n') {
            if (lineBuffer[i] == '{' && lineBuffer[i + 1] == '{') {
                i += 2;
                char key[MAX_KEY_LENGTH] = "";
                int keyPos = 0;

                while (lineBuffer[i] != '\0' && !(lineBuffer[i] == '}' && lineBuffer[i + 1] == '}') && keyPos < MAX_KEY_LENGTH - 1) {
                    key[keyPos++] = lineBuffer[i++];
                }
                key[keyPos] = '\0';

                trim(key);

                if (lineBuffer[i] == '}' && lineBuffer[i + 1] == '}') {
                    i += 2;
                } else {
                    std::cerr << "Ошибка: незакрытая или некорректная метка в шаблоне.\n";
                    return 4;
                }

                int pos = findKey(dataArray, dataCount, key);
                if (pos < 0) {
                    std::cerr << "В шаблоне есть метка без соответствующего ключа в данных.\n";
                    return 1;
                }

                const char *value = dataArray[pos].value;
                int j = 0;
                while (value[j] != '\0' && resultPos < (int) (sizeof(result)) - 1) {
                    result[resultPos++] = value[j++];
                }
            } else {
                if (resultPos < (int) (sizeof(result)) - 1) {
                    result[resultPos++] = lineBuffer[i++];
                } else {
                    break;
                }
            }
        }

        result[resultPos] = '\0';
        (*out) << result << '\n';
    }

    dataFile.close();
    templateFile.close();
    if (outputFile.is_open()) {
        outputFile.close();
    }

    if (outputPath) {
        std::cout << "Шаблон обработан и записан в файл " << outputPath << '\n';
    } else {
        std::cout << "Шаблон обработан и выведен на консоль\n";
    }

    return 0;
}
