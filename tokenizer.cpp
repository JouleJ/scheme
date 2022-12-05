#include "tokenizer.h"
#include "error.h"
#include <cctype>
#include <stdexcept>

SymbolToken::SymbolToken(char chr) {
    name.push_back(chr);
}

SymbolToken::SymbolToken(const std::string& str) : name(str) {
}

bool SymbolToken::operator==(const SymbolToken& other) const {
    return name == other.name;
}

bool QuoteToken::operator==(const QuoteToken&) const {
    return true;
}

bool DotToken::operator==(const DotToken&) const {
    return true;
}

ConstantToken::ConstantToken(int x) : value(x) {
}

bool ConstantToken::operator==(const ConstantToken& other) const {
    return value == other.value;
}

// [a-zA-Z<=>*#]
static bool IsSymbolStart(unsigned char chr) {
    static bool table[256];
    if (!table[static_cast<int>('a')]) {
        for (int i = 'a'; i <= 'z'; ++i) {
            table[i] = true;
        }
        for (int i = 'A'; i <= 'Z'; ++i) {
            table[i] = true;
        }
        for (char chr : std::string("<=>*#")) {
            table[static_cast<int>(chr)] = true;
        }
    }
    return table[static_cast<int>(chr)];
}

// [a-zA-Z<=>*#0-9?!-]
static bool IsSymbol(unsigned char chr) {
    static bool table[256];
    if (!table[static_cast<int>('a')]) {
        for (int i = 'a'; i <= 'z'; ++i) {
            table[i] = true;
        }
        for (int i = 'A'; i <= 'Z'; ++i) {
            table[i] = true;
        }
        for (int i = '0'; i <= '9'; ++i) {
            table[i] = true;
        }
        for (char chr : std::string("<=>*#?!-")) {
            table[static_cast<int>(chr)] = true;
        }
    }
    return table[static_cast<int>(chr)];
}

bool Tokenizer::IsEof() {
    return in_->eof();
}

char Tokenizer::Peek() {
    return in_->peek();
}

char Tokenizer::Get() {
    return in_->get();
}

void Tokenizer::SkipWhitespace() {
    while (!IsEof() && isspace(Peek())) {
        Get();
    }
}

Tokenizer::Tokenizer(std::istream* in) : in_(in) {
    Next();
}

bool Tokenizer::IsEnd() {
    return !token_;
}

void Tokenizer::Next() {
    token_ = {};
    SkipWhitespace();
    if (IsEof()) {
        return;
    }
    char chr = Get();
    switch (chr) {
        case '(': {
            token_ = BracketToken::OPEN;
            return;
        }

        case ')': {
            token_ = BracketToken::CLOSE;
            return;
        }

        case '.': {
            token_ = DotToken();
            return;
        }

        case '\'': {
            token_ = QuoteToken();
            return;
        }

        case '/': {
            token_ = SymbolToken(chr);
            return;
        }

        case '+':
        case '-': {
            if (IsEof() || !isdigit(Peek())) {
                token_ = SymbolToken(chr);
                return;
            }
            break;
        }
    }
    if (isdigit(chr) || chr == '+' || chr == '-') {
        bool is_negative = false;
        int value = 0;
        if (chr == '+' || chr == '-') {
            is_negative = (chr == '-');
            chr = Get();
        }
        while (true) {
            value = 10 * value + (chr - '0');
            if (!IsEof() && isdigit(Peek())) {
                chr = Get();
            } else {
                break;
            }
        }
        token_ = ConstantToken(is_negative ? -value : value);
        return;
    }
    if (IsSymbolStart(chr)) {
        std::string name;
        name.push_back(chr);
        while (!IsEof() && IsSymbol(Peek())) {
            name.push_back(Get());
        }
        if (name == "#f") {
            token_ = BooleanToken::FALSE;
        } else if (name == "#t") {
            token_ = BooleanToken::TRUE;
        } else {
            token_ = SymbolToken(name);
        }
        return;
    }
    throw SyntaxError(std::string("Tokenizer::Next(): invalid character code: ") +
                      std::to_string(static_cast<int>(chr)));
}

Token Tokenizer::GetToken() {
    return token_.value();
}
