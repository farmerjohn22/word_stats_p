#include <iostream>
#include <vector>
#include <deque>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <iomanip>

using word_id = uint64_t;
constexpr word_id NONE = 0;
#define Map_Type std::unordered_map

bool is_empty(const std::string &s) {
    for(char ch: s) {
        if (!std::isspace(ch)) {
            return false;
        }
    }
    return true;
}

bool is_letter(char ch) {
    return std::isalnum(ch);

}
bool replace_str(std::string &s, const std::string &pat, const std::string &with) {
    size_t index = 0;
    bool result = false;
    while (true) {
        index = s.find(pat, index);
        if (index == std::string::npos) return result;

        result = true;
        s.replace(index, pat.size(), with);
        index += with.size();
    }
}

class Words {
public:
    Words(size_t count): _list(count, NONE) {
    }
    bool empty() const {
        return _list.empty();
    }
    size_t size() const {
        return _list.size();
    }
    void shift(word_id id) {
        _list.push_back(id);
        _list.pop_front();
    }
    void clear() {
        for (auto &v: _list) {
            v = NONE;
        }
    }
    void push(word_id id) {
        _list.push_back(id);
    }
    void pop() {
        _list.pop_back();
    }
    word_id get(size_t n) const {
        return _list[n];
    }
    word_id last() const {
        return _list.back();
    }
private:
    std::deque<word_id> _list;
};

class Stat_File {
public:
    Stat_File(const std::string &filename): file(filename) {
    }
    template <class _C, class _F>
    void read(const _C &conv, const _F &proc) {
        std::string s;
        Words words(0);
        while (getline(file, s)) {
            char ch = s.front();
            if (ch == '-') {
                words.pop();
            }
            else {
                size_t k = s.find(' ');
                words.push(conv(s.substr(1, k - 1)));
                size_t cnt = static_cast<size_t>(std::stoi(s.substr(k + 1)));
                proc(words, cnt);
                if (ch == '=') {
                    words.pop();
                }
            }
        }
        if (!words.empty()) {
            std::cout << "Error in stat file" << std::endl;
            std::terminate();
        }
    }
protected:
    std::ifstream file;
};

class Word_Id_Map {
public:
    Word_Id_Map() {
        add_word("$");
        id_to_word.reserve(60000);
    }
    size_t size() const {
        return id_to_word.size();
    }
    word_id add_word(const std::string &s) {
        if (s.empty()) {
            std::terminate();
            return NONE;
        }
        auto it = word_to_id.find(s);
        if (it == word_to_id.end()) {
            word_id n = id_to_word.size();
            word_to_id[s] = n;
            id_to_word.push_back(s);
            return n;
        }
        else {
            return it->second;
        }
    }
    const std::string &word_by_id(word_id id) const {
        return id_to_word[id];
    }
protected:
    std::vector<std::string> id_to_word;
    Map_Type<std::string, word_id> word_to_id;
};

class Word_Ngram_Tree_Map;

class Word_Ngram_Tree {
public:
    Word_Ngram_Tree();
    Word_Ngram_Tree(const Word_Ngram_Tree &) = delete;
    Word_Ngram_Tree(const Word_Ngram_Tree &&) = delete;
    Word_Ngram_Tree &operator=(const Word_Ngram_Tree &&) = delete;
    ~Word_Ngram_Tree();
    void set(const Words &words, size_t cnt);
    void add(const Words &words, size_t cnt, size_t limit);
    void add_comma(Words &words, size_t cnt, size_t limit);
    void print(std::ostream &out, const Word_Id_Map &dict) const;
private:
    void _print(word_id id, std::ostream &out, const Word_Id_Map &dict) const;
    void _set(const Words &words, size_t n, size_t cnt);
    void _add(const Words &words, size_t n, size_t cnt, size_t limit);
    size_t  _count;
    Word_Ngram_Tree_Map *_next;
};

class Word_Ngram_Tree_Map: public Map_Type<word_id, Word_Ngram_Tree> {
};

Word_Ngram_Tree::Word_Ngram_Tree(): _count(0), _next(nullptr) {
}

Word_Ngram_Tree::~Word_Ngram_Tree() {
    delete _next;
}

void Word_Ngram_Tree::set(const Words &words, size_t cnt) {
    _set(words, 0, cnt);
}

void Word_Ngram_Tree::add(const Words &words, size_t cnt, size_t limit) {
    if (words.empty()) {
        return;
    }
    _add(words, 0, cnt, limit);
}

void Word_Ngram_Tree::add_comma(Words &words, size_t cnt, size_t limit) {
    if (words.last() != NONE) {
        words.shift(NONE);
        add(words, cnt, limit);
    }
    words.clear();
}

void Word_Ngram_Tree::print(std::ostream &out, const Word_Id_Map &dict) const {
    if (_next != nullptr) {
        for(const auto &v: *_next) {
            v.second._print(v.first, out, dict);
        }
    }
}

void Word_Ngram_Tree::_print(word_id id, std::ostream &out, const Word_Id_Map &dict) const {
    if (_next == nullptr) {
        out << "=" << dict.word_by_id(id) << " " << _count << std::endl;
    }
    else {
        out << "+" << dict.word_by_id(id) << " " << _count << std::endl;
        print(out, dict);
        out << "-" << std::endl;
    }
}

void Word_Ngram_Tree::_set(const Words &words, size_t n, size_t cnt) {
    if (n < words.size()) {
        if (_next == nullptr) {
            _next = new Word_Ngram_Tree_Map();
        }
        (*_next)[words.get(n)]._set(words, n + 1, cnt);
    }
    else {
        _count += cnt;
    }
}

void Word_Ngram_Tree::_add(const Words &words, size_t n, size_t cnt, size_t limit) {
    if (n + 1 < words.size()) {
        if (((n == 0) || (words.get(n) != NONE)) && (_next != nullptr)) {
            auto it = _next->find(words.get(n));
            if (it != _next->end()) {
                it->second._add(words, n + 1, cnt, limit);
            }
        }
    }
    else if (_count >= limit) {
        if (_next == nullptr) {
            _next = new Word_Ngram_Tree_Map();
        }
        (*_next)[words.get(n)]._count += cnt;
    }
}

void load_line(size_t, std::string &s, Word_Id_Map &dict, Word_Ngram_Tree &stats, Words &words, const std::string &also_spaces, size_t hit_limit) {
    replace_str(s, "--", "=");
    replace_str(s, " -", "=");
    replace_str(s, "- ", "=");
    replace_str(s, "_", "");
    replace_str(s, "'", "");
    replace_str(s, "&", " and ");
    replace_str(s, "mr.", "mr ");
    replace_str(s, "Mr.", "Mr ");
    replace_str(s, "MR.", "MR ");
    replace_str(s, "mrs.", "mrs ");
    replace_str(s, "Mrs.", "Mrs ");
    replace_str(s, "MRS.", "MRS ");
    replace_str(s, "st.", "st ");
    replace_str(s, "St.", "St ");
    replace_str(s, "ST.", "ST ");
    replace_str(s, "a.m.", "am ");
    replace_str(s, "A.M.", "AM ");
    replace_str(s, "p.m.", "pm ");
    replace_str(s, "P.M.", "PM ");
    std::string cur_word;
    auto it = s.begin();
    while (it != s.end()) {
        cur_word.clear();
        while ((it != s.end()) && is_letter(*it)) {
            cur_word += *it;
            ++it;
        }
        if (!cur_word.empty()) {
            words.shift(dict.add_word(cur_word));
            stats.add(words, 1, hit_limit);
        }
        if (it != s.end()) {
            char ch = *it;
            bool space = (ch == ' ');
            bool also_space = (also_spaces.find(ch) != std::string::npos);
            if (!(space || also_space)) {
                stats.add_comma(words, 1, hit_limit);
            }
            ++it;
        }
    }
}

bool load_A(const std::string &fn, size_t ngram_size, Word_Id_Map &dict, Word_Ngram_Tree &stats, const std::string &also_spaces, size_t hit_limit) {
    std::ifstream ifile(fn);
    std::string s;
    size_t ln = 1;
    while (getline(ifile, s)) {
        if (++ln % 1000 == 0) {
            std::cout << "\rLine " << ln++;
        }
        Words words(ngram_size);
        load_line(ln, s, dict, stats, words, also_spaces, hit_limit);
        stats.add_comma(words, 1, hit_limit);
    }
    return true;
}


bool load_G(const std::string &fn, size_t ngram_size, Word_Id_Map &dict, Word_Ngram_Tree &stats, const std::string &also_spaces, size_t hit_limit) {
    static std::string start1 = "*** START OF THIS PROJECT GUTENBERG";
    static std::string start2 = "***START OF THE PROJECT GUTENBERG";
    static std::string start3 = "***START OF THE PROJECT GUTENBERG EBOOK AUDIO";
    static std::string finish1 = "*** END OF THIS PROJECT GUTENBERG";
    static std::string finish2 = "***END OF THE PROJECT GUTENBERG";
    std::ifstream ifile(fn);
    std::string s;
    size_t ln = 1;
    Words words(ngram_size);
    bool good_start = false;
    while (getline(ifile, s)) {
        if (s.substr(0, start3.size()) == start3) {
            std::cout << fn << " is audio book" << std::endl;
            return false;
        }
        if (s.substr(0, start2.size()) == start2) {
            good_start = true;
            break;
        }
        if (s.substr(0, start1.size()) == start1) {
            good_start = true;
            break;
        }
    }
    if (!good_start) {
        std::cout << "Beginning of file " << fn << " not found" << std::endl;
        return false;
    }
    for (size_t i = 0; i < 12; ++i) {
        getline(ifile, s);
    }
    while (getline(ifile, s)) {
        if (++ln % 1000 == 0) {
            std::cout << "\rLine " << ln;
        }
        if (is_empty(s)) {
            stats.add_comma(words, 1, hit_limit);
        }
        if (s.substr(0, finish1.size()) == finish1) {
            stats.add_comma(words, 1, hit_limit);
            return true;
        }
        if (s.substr(0, finish2.size()) == finish2) {
            stats.add_comma(words, 1, hit_limit);
            return true;
        }
        load_line(ln, s, dict, stats, words, also_spaces, hit_limit);
    }
    std::cout << "\rEnd of file " << fn << " not found" << std::endl;
    return false;
}

int main(int argc, char* args[]) {
    size_t ngram_size = static_cast<size_t>(std::stoi(args[3]));
    size_t hit_limit = static_cast<size_t>(std::stoi(args[4]));

    std::string type = args[5];

    std::string path = args[6];
    std::vector<std::string> file_list;
    {
        std::ifstream ifile(path + args[7]);
        std::string s;
        while (getline(ifile, s)) {
            file_list.push_back(path + s);
        }
    }
    std::string also_spaces = (argc > 8) ? args[8] : "";

    Word_Id_Map dict;
    Word_Ngram_Tree stats;
    Stat_File stat_file(args[1]);
    stat_file.read([&](std::string s) -> word_id {
        word_id id = dict.add_word(s);
        return id;
    },
    [&](const auto &words, size_t cnt) {
        stats.set(words, cnt);
    });

    size_t errors = 0;
    for(size_t i = 0; i < file_list.size(); ++i) {
        std::cout << "File " << file_list[i] << ", " << (i + 1) << " out of " << file_list.size();
        std::cout << ", currently " << dict.size() << " words";
        if (errors > 0) {
            std::cout << " and " << errors << " errors";
        }
        std::cout << std::endl;
        if (type == "A") {
            if (!load_A(file_list[i], ngram_size, dict, stats, also_spaces, hit_limit)) {
                errors++;
            }
        }
        else if (type == "G") {
            if (!load_G(file_list[i], ngram_size, dict, stats, also_spaces, hit_limit)) {
                errors++;
            }
        }
        else {
            std::cout << "Unknown type " << type << std::endl;
        }
        std::cout << "\rDone             " << std::endl;
    }

    std::ofstream ofile(args[2]);
    stats.print(ofile, dict);

    std::cout << "Stats saved" << std::endl;

    return 0;
}
