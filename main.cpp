#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

#include <tbb/parallel_sort.h>
#include <tbb/tbb.h>

using namespace std;

void display(
        std::string name,
        std::chrono::time_point<std::chrono::high_resolution_clock> start,
        std::chrono::time_point<std::chrono::high_resolution_clock> end
) {
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    cerr << name << " " << elapsed << " ms" << "\n";
}

int trivial_sort(const char* fileName) {
    auto start = std::chrono::high_resolution_clock::now();
    ifstream input(fileName);
    vector<string> lines;
    copy(istream_iterator<std::string>(input),
          istream_iterator<std::string>(),
          back_inserter(lines));


    tbb::parallel_sort(lines.begin(), lines.end());
    auto end = std::chrono::high_resolution_clock::now();
    display("trivial_sort", start, end);

    for(const auto& str: lines) {
        cout<<str<<'\n';
    }

    return 0;
}

class FasterSort {
    struct Line {
        Line(const uint8_t* linePtr, size_t lineLength): begin(linePtr), length(lineLength) {
        }
        uint64_t getKey() {
            KeyType key(0);
            memcpy(&key, begin, min(length, sizeof(key)));
            if constexpr (sizeof(key) == 8) {
                return __builtin_bswap64(key);
            } else if constexpr (sizeof(key) == 4) {
                return __builtin_bswap32(key);
            }
            static_assert(sizeof(key) == 8 || sizeof(key) == 4, "unsupported key size");
        }

        void refreshKey() {
            key = getKey();
        }

        bool operator<(const Line& oth) const {
            if(oth.key != key) {
                return key<oth.key;
            } else {
                int cmp = memcmp(begin, oth.begin, min(length, oth.length));
                if(cmp==0) {
                    return length<oth.length;
                } else {
                    return cmp<0;
                }
            }
        }

        typedef uint64_t KeyType;
        const uint8_t* begin;
        size_t length;
        KeyType key;
    };

public:
    FasterSort(const FasterSort&) = delete;
    FasterSort& operator=(const FasterSort&) = delete;

    FasterSort(const char* fileName) {
        f = fopen(fileName, "rb");
        if (!f) {
            throw runtime_error("cannot open input file");
        }
    }


    void sort() {
        auto start = std::chrono::high_resolution_clock::now();

        fseek(f, 0, SEEK_END);
        buf.resize(ftell(f)+1); //buf may need extra newline character for the last line
        size_t bufSize = buf.size()-1;
        fseek(f, 0, SEEK_SET);

        //Read input
        size_t read = std::fread(&buf[0], 1, bufSize, f);
        if(read!=bufSize) {
            throw runtime_error("error read input file");
        }

        //Calculate lines offsets
        vector<Line> lines;
        size_t offset = 0;
        size_t prevOffset =0;
        while (uint8_t *newLinePos = (uint8_t *) memchr(&buf[offset], '\n', bufSize - offset)) {
            prevOffset = offset;
            offset = newLinePos - &buf[0] + 1;
            lines.emplace_back(&buf[prevOffset], offset-prevOffset-1);
        }

        //fix last line
        if(offset != bufSize) {
            buf[bufSize]='\n';
            lines.emplace_back(&buf[offset], bufSize - offset);
        }

        //Calculate line keys
        tbb::parallel_for(tbb::blocked_range<size_t>(0,lines.size()),
                          [&](const tbb::blocked_range<size_t>& r) {
                              for(size_t i=r.begin(); i!=r.end(); ++i)
                                  lines[i].refreshKey();
                          } );

        //Actual sort
        tbb::parallel_sort(lines.begin(), lines.end());

        //Write lines to stdout
        for(auto line: lines) {
            fwrite(line.begin, 1, line.length+1, stdout);
        }

        auto end = std::chrono::high_resolution_clock::now();
        display("tbb sort", start, end);
    }

    ~FasterSort() {
        if(f) {
            fclose(f);
        }
    }

    FILE* f = nullptr;
    vector<uint8_t > buf{0};
};

int main(int argc, char* argv[]) {
    if(argc!=2) {
        cerr<<"Sort ASCII strings\n"
            <<"Usage:\n\t"
            <<argv[0]<<" <input_file>\n\n"
            <<"The result will be printed to standard output\n";
        return -1;
    }
    try {
#if 0
        trivial_sort(argv[1]);
#else
        FasterSort sort(argv[1]);
        sort.sort();
#endif
    } catch (const exception& ex) {
        cerr << ex.what() << "\n";
        return -2;
    }
    return 0;
}