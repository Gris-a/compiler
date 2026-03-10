#include "scanner/scan.hpp"
#include "util/singleton.hpp"
#include <iostream>
#include <charconv>
#include <unordered_map>
#include <queue>

namespace Scanner {

struct Parser {
public:
    static void skip_spaces(const std::string &line, size_t &pos) {
        while (std::isspace(line[pos])) ++pos;
    }

    static Token parse(const std::string &line, size_t &pos) {
        Token token = parse_keyword(line, pos);
        if (!valid_token(token)) {
            token = parse_numeric_literal(line, pos);
        }
        if (!valid_token(token)) {
            token = parse_identifier_literal(line, pos);
        }
        return token;
    }

    static Token parse_keyword(const std::string &line, size_t &pos) {
        return Parser::get_instance().parse_keyword_impl(line, pos);
    }

    static Token parse_numeric_literal(const std::string &line, size_t &pos) {
        const char *begin = line.data() + pos;
        const char *end = line.data() + line.size();
        
        int base = 10;
        unsigned value;

        if (begin[0] == '0') {
            switch (begin[1]) {
            case 'b':
                base = 2;
                begin += 2;
                break;
            case 'x':
                base = 16;
                begin += 2;
                break;
            }
        }
        
        auto [ptr, ec] = std::from_chars(begin, end, value, base);
        if (ec != std::errc() || ptr == begin) return NoToken{};

        pos = ptr - line.data();
        return (line[pos] == 'u')
            ? (++pos, Token(Unsigned{value}))
            : Integer{static_cast<int>(value)};
    }

    static Token parse_identifier_literal(const std::string &line, size_t &pos) {
        auto check = [&line](size_t pos) -> bool {
            return (std::isalnum(line[pos]) || line[pos] == '_');
        };
        if (!check(pos)) return NoToken{};

        size_t current_pos = pos;
        while(check(current_pos)) ++current_pos;

        std::string identifier = line.substr(pos, current_pos - pos);
        pos = current_pos;

        return Identifier{std::move(identifier)};
    }

private:
    SINGLETON(Parser);

    struct Node {
    public:
        Node() = default;

        const Node *step(char ch) const {
            auto it = map_.find(ch);
            if (it == map_.end()) {
                return nullptr;
            }
            return it->second;
        }

        Node *step_alloc(char ch) {
            Node* &node = map_[ch];
            if (node == nullptr) {
                node = new Node();
            }
            return node;
        }

        Token get_token() const {
            return token_;
        }

        void set_token(Token token) {
            token_ = token;
        }

        void clear() {
            std::queue<Node *> queue;
            for (auto &[_, node] : map_) {
                queue.push(node);
            }
            map_.clear();

            while (!queue.empty()) {
                Node *delete_node = queue.front(); queue.pop();
                for (auto &[_, node] : delete_node->map_) {
                    queue.push(node);
                }
                delete delete_node;
            }
        }

    private:
        std::unordered_map<char, Node *> map_;
        Token token_;
    };

    Parser() {
        [&]<token... Ts>(TTuple<Ts...>) {
            (add_token<Ts>(), ...);
        }(Keywords{});
    }

    ~Parser() {
        start_.clear();
    }

    template <token T>
    void add_token() {
        Node *node = start();
        for (char ch : T::key) {
            node = node->step_alloc(ch);
        }
        node->set_token(T{});
    }

    Token parse_keyword_impl(const std::string &line, size_t &pos) const {
        const Node *node = start(); size_t current_pos = pos;
        for (; current_pos != line.size(); ++current_pos) {
            const Node *next = node->step(line[current_pos]);
            if (!next) break;
            node = next;
        }

        Token token = node->get_token();
        if (valid_token(token)) {
            pos = current_pos;
        }
        return token;
    }

    Node *start() {
        return &start_;
    }
    
    const Node *start() const {
        return &start_;
    }

    Node start_;
};

std::vector<TokenInfo> scan(std::ifstream &source) {
    std::vector<TokenInfo> tokens;

    for (size_t line = 0; source; ++line) {
        std::string string; std::getline(source, string);
        for (size_t pos = 0; pos != string.size();) {
            Parser::skip_spaces(string, pos);

            Position position(line, pos);
            Token token = Parser::parse(string, pos);

            if (!valid_token(token)) {
                throw std::exception();
            }
            tokens.emplace_back(position, token);
        }
    }
    tokens.emplace_back(Position(0, 0), NoToken{});

    return tokens;
}

};