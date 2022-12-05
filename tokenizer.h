#pragma once

#include <variant>
#include <optional>
#include <istream>

struct SymbolToken {
    std::string name;

    SymbolToken(char);
    SymbolToken(const std::string&);

    bool operator==(const SymbolToken& other) const;
};

struct QuoteToken {
    bool operator==(const QuoteToken&) const;
};

struct DotToken {
    bool operator==(const DotToken&) const;
};

enum class BracketToken { OPEN, CLOSE };
enum class BooleanToken { FALSE, TRUE };

struct ConstantToken {
    int value;

    ConstantToken(int);

    bool operator==(const ConstantToken& other) const;
};

using Token =
    std::variant<ConstantToken, BracketToken, BooleanToken, SymbolToken, QuoteToken, DotToken>;

class Tokenizer {
    std::optional<Token> token_;
    std::istream* in_;

    bool IsEof();
    char Peek();
    char Get();

    void SkipWhitespace();

public:
    Tokenizer(std::istream* in);

    bool IsEnd();

    void Next();

    Token GetToken();
};
