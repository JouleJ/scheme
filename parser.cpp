#include "parser.h"
#include "error.h"
#include "scheme.h"

static void FailCompare(const std::shared_ptr<const Object>& lhs,
                        const std::shared_ptr<const Object>& rhs) {
    const std::string msg =
        std::string("Cannot compare: ") + ToString(lhs) + std::string(" and ") + ToString(rhs);
    throw NameError(msg);
}

Number::Number(int64_t value) : value_(value) {
}

int64_t Number::GetValue() const {
    return value_;
}

std::string Number::ToString() const {
    return std::to_string(value_);
}

bool Number::IsEqualTo(const std::shared_ptr<const Object>& other) const {
    const Number* number = dynamic_cast<const Number*>(other.get());
    return number != nullptr && number->value_ == value_;
}

bool Number::IsLessThan(const std::shared_ptr<const Object>& other) const {
    const Number* number = dynamic_cast<const Number*>(other.get());
    if (number == nullptr) {
        FailCompare(shared_from_this(), other);
    }
    return value_ < number->value_;
}

Symbol::Symbol(const std::string& name) : name_(name) {
}

const std::string& Symbol::GetName() const {
    return name_;
}

std::string Symbol::ToString() const {
    return name_;
}

bool Symbol::IsEqualTo(const std::shared_ptr<const Object>& other) const {
    const Symbol* symbol = dynamic_cast<const Symbol*>(other.get());
    return symbol != nullptr && symbol->name_ == name_;
}

bool Symbol::IsLessThan(const std::shared_ptr<const Object>& other) const {
    FailCompare(shared_from_this(), other);
    return false;  // never reached
}

Cell::Cell(const std::shared_ptr<Object>& first, const std::shared_ptr<Object>& second)
    : first_(first), second_(second) {
}

std::shared_ptr<Object> Cell::GetFirst() const {
    return first_;
}

std::shared_ptr<Object> Cell::GetSecond() const {
    return second_;
}

void Cell::SetFirst(const std::shared_ptr<Object>& value) {
    first_ = value;
}

void Cell::SetSecond(const std::shared_ptr<Object>& value) {
    second_ = value;
}

std::string Cell::ToString() const {
    const std::shared_ptr<const Object> shared_this = shared_from_this();
    return ListToString(shared_this);
}

bool Cell::IsEqualTo(const std::shared_ptr<const Object>& other) const {
    const Cell* cell = dynamic_cast<const Cell*>(other.get());
    return cell != nullptr && Equal(first_, cell->first_) && Equal(second_, cell->second_);
}

bool Cell::IsLessThan(const std::shared_ptr<const Object>& other) const {
    FailCompare(shared_from_this(), other);
    return false;  // never reached
}

Boolean::Boolean(bool value) : value_(value) {
}

bool Boolean::GetValue() const {
    return value_;
}

std::string Boolean::ToString() const {
    return (value_ ? "#t" : "#f");
}

bool Boolean::IsEqualTo(const std::shared_ptr<const Object>& other) const {
    const Boolean* boolean = dynamic_cast<const Boolean*>(other.get());
    return boolean != nullptr && value_ == boolean->value_;
}

bool Boolean::IsLessThan(const std::shared_ptr<const Object>& other) const {
    FailCompare(shared_from_this(), other);
    return false;  // never reached
}

std::string ToString(const std::shared_ptr<const Object>& obj) {
    if (obj == nullptr) {
        return "()";
    } else {
        return obj->ToString();
    }
}

Lambda::Lambda(const std::vector<std::string>& arg_names,
           const std::weak_ptr<Scope>& scope,
           const std::vector<std::shared_ptr<Object>>& expressions):
                arg_names_(arg_names),
                scope_(scope),
                expressions_(expressions) {
    if (!scope_.expired()) {
        scope_.lock()->AddRef();
    }
}

Lambda::~Lambda() {
    if (!scope_.expired()) {
        scope_.lock()->DelRef();
    }
}

std::shared_ptr<Object> Lambda::Call(const std::vector<std::shared_ptr<Object>>& args) {
    const size_t num_args = arg_names_.size();
    std::shared_ptr<Scope> local_scope(new Scope(scope_));
    const auto last = std::remove_if(
        local_scopes_.begin(),
        local_scopes_.end(),
        [](const std::shared_ptr<Scope>& ptr) { return ptr->GetRefs() <= 0; }
    ); 
    local_scopes_.erase(last, local_scopes_.end());
    local_scopes_.push_back(local_scope);
    if (args.size() != num_args) {
        throw RuntimeError(std::string("Invalid number of arguments in for lambda: ") + ToString());
    }
    for (size_t i = 0; i != num_args; ++i) {
        local_scope->SetLocalVariable(arg_names_[i], args[i]);
    }
    std::shared_ptr<Object> result;
    Interpreter interpreter(local_scope);
    for (const auto& expression: expressions_) {
        result = interpreter.Eval(expression);
    }
    return result;
}

std::string Lambda::ToString() const {
    std::string result = "(lambda (";
    for (const auto& name: arg_names_) {
        result += name;
        result.push_back(' ');
    }
    result.back() = ')';
    for (const auto& expression: expressions_) {
        result.push_back(' ');
        result += ::ToString(expression);
    }
    result.push_back(')');
    return result;
}

bool Lambda::IsEqualTo(const std::shared_ptr<const Object>& other) const {
    return this == other.get();
}

bool Lambda::IsLessThan(const std::shared_ptr<const Object>& other) const {
    FailCompare(shared_from_this(), other);
    return false;  // never reached
}

static bool IsClosingBracket(const Token& token) {
    const BracketToken* bracket_token = std::get_if<BracketToken>(&token);
    return bracket_token != nullptr && *bracket_token == BracketToken::CLOSE;
}

std::shared_ptr<Object> GetBooleanConstant(bool value) {
    static std::shared_ptr<Object> true_value(new Boolean(true));
    static std::shared_ptr<Object> false_value(new Boolean(false));
    return (value ? true_value : false_value);
}

bool Equal(const std::shared_ptr<const Object>& lhs, const std::shared_ptr<const Object>& rhs) {
    if (lhs == nullptr || rhs == nullptr) {
        return lhs == nullptr && rhs == nullptr;
    }
    return lhs->IsEqualTo(rhs);
}

bool Less(const std::shared_ptr<const Object>& lhs, const std::shared_ptr<const Object>& rhs) {
    if (lhs == nullptr || rhs == nullptr) {
        FailCompare(lhs, rhs);
    }
    return lhs->IsLessThan(rhs);
}

bool LessOrEqual(const std::shared_ptr<const Object>& lhs,
                 const std::shared_ptr<const Object>& rhs) {
    return Less(lhs, rhs) || Equal(lhs, rhs);
}

bool Greater(const std::shared_ptr<const Object>& lhs, const std::shared_ptr<const Object>& rhs) {
    return Less(rhs, lhs);
}

bool GreaterOrEqual(const std::shared_ptr<const Object>& lhs,
                    const std::shared_ptr<const Object>& rhs) {
    return LessOrEqual(rhs, lhs);
}

std::shared_ptr<Object> Add(const std::shared_ptr<const Object>& lhs,
                            const std::shared_ptr<const Object>& rhs) {
    const Number* left_number = dynamic_cast<const Number*>(lhs.get());
    const Number* right_number = dynamic_cast<const Number*>(rhs.get());
    if (left_number == nullptr || right_number == nullptr) {
        const std::string msg =
            std::string("Cannot add: ") + ToString(lhs) + " and " + ToString(rhs);
        throw RuntimeError(msg);
    }
    return GetNumberConstant(left_number->GetValue() + right_number->GetValue());
}

std::shared_ptr<Object> Subtract(const std::shared_ptr<const Object>& lhs,
                                 const std::shared_ptr<const Object>& rhs) {
    const Number* left_number = dynamic_cast<const Number*>(lhs.get());
    const Number* right_number = dynamic_cast<const Number*>(rhs.get());
    if (left_number == nullptr || right_number == nullptr) {
        const std::string msg =
            std::string("Cannot subtract: ") + ToString(lhs) + " and " + ToString(rhs);
        throw RuntimeError(msg);
    }
    return GetNumberConstant(left_number->GetValue() - right_number->GetValue());
}

std::shared_ptr<Object> Multiply(const std::shared_ptr<const Object>& lhs,
                                 const std::shared_ptr<const Object>& rhs) {
    const Number* left_number = dynamic_cast<const Number*>(lhs.get());
    const Number* right_number = dynamic_cast<const Number*>(rhs.get());
    if (left_number == nullptr || right_number == nullptr) {
        const std::string msg =
            std::string("Cannot multiply: ") + ToString(lhs) + " and " + ToString(rhs);
        throw RuntimeError(msg);
    }
    return GetNumberConstant(left_number->GetValue() * right_number->GetValue());
}

std::shared_ptr<Object> Divide(const std::shared_ptr<const Object>& lhs,
                               const std::shared_ptr<const Object>& rhs) {
    const Number* left_number = dynamic_cast<const Number*>(lhs.get());
    const Number* right_number = dynamic_cast<const Number*>(rhs.get());
    if (left_number == nullptr || right_number == nullptr || right_number->GetValue() == 0) {
        const std::string msg =
            std::string("Cannot divide: ") + ToString(lhs) + " and " + ToString(rhs);
        throw RuntimeError(msg);
    }
    return GetNumberConstant(left_number->GetValue() / right_number->GetValue());
}

bool AsBoolean(const std::shared_ptr<const Object>& object) {
    if (const Boolean* boolean = dynamic_cast<const Boolean*>(object.get())) {
        return boolean->GetValue();
    }
    return true;
}
std::shared_ptr<Object> Not(const std::shared_ptr<const Object>& object) {
    return GetBooleanConstant(!AsBoolean(object));
}

std::shared_ptr<Object> GetNumberConstant(int64_t value) {
    const int64_t min = -1000;
    const int64_t max = 1000;
    static std::shared_ptr<Object> table[max - min + 1];
    if (min <= value && value <= max) {
        size_t index = value - min;
        if (table[index] == nullptr) {
            table[index].reset(new Number(value));
        }
        return table[index];
    }
    return std::shared_ptr<Object>(new Number(value));
}

std::string ListToString(std::shared_ptr<const Object> object) {
    std::string result = "(";
    bool first = true;
    while (object != nullptr) {
        const Cell* cell = dynamic_cast<const Cell*>(object.get());
        if (cell == nullptr) {
            result += std::string(" . ");
            result += ToString(object);
            break;
        }
        if (first) {
            first = false;
        } else {
            result.push_back(' ');
        }
        result += ToString(cell->GetFirst());
        object = cell->GetSecond();
    }
    result.push_back(')');
    return result;
}

static std::shared_ptr<Object> ReadList(Tokenizer* tokenizer) {
    std::vector<std::shared_ptr<Object>> objects;
    std::shared_ptr<Object> result = nullptr;
    while (!tokenizer->IsEnd() && !IsClosingBracket(tokenizer->GetToken())) {
        const Token token = tokenizer->GetToken();
        if (std::get_if<DotToken>(&token) != nullptr) {
            if (objects.empty()) {
                throw SyntaxError("Read: expected expression before .");
            }
            tokenizer->Next();
            result = Read(tokenizer);
            break;
        }
        objects.push_back(Read(tokenizer));
    }
    if (tokenizer->IsEnd() || !IsClosingBracket(tokenizer->GetToken())) {
        throw SyntaxError("Read: expected ) ending list");
    }
    tokenizer->Next();  // skip ')'
    for (auto iter = objects.rbegin(); iter != objects.rend(); ++iter) {
        result = std::shared_ptr<Object>(new Cell(*iter, result));
    }
    return result;
}

std::shared_ptr<Object> Read(Tokenizer* tokenizer) {
    if (tokenizer->IsEnd()) {
        throw SyntaxError("Read: Unexpected end of input");
    }
    const Token token = tokenizer->GetToken();
    tokenizer->Next();
    if (std::get_if<QuoteToken>(&token) != nullptr) {
        static std::shared_ptr<Object> quote(new Symbol("quote"));
        std::shared_ptr<Object> expression = Read(tokenizer);
        std::shared_ptr<Object> cell1(new Cell(expression, nullptr));
        std::shared_ptr<Object> cell2(new Cell(quote, cell1));
        return cell2;
    } else if (const ConstantToken* constant_token = std::get_if<ConstantToken>(&token)) {
        return GetNumberConstant(constant_token->value);
    } else if (const SymbolToken* symbol_token = std::get_if<SymbolToken>(&token)) {
        return std::shared_ptr<Object>(new Symbol(symbol_token->name));
    } else if (const BooleanToken* boolean_token = std::get_if<BooleanToken>(&token)) {
        return GetBooleanConstant(*boolean_token == BooleanToken::TRUE);
    } else if (const BracketToken* bracket_token = std::get_if<BracketToken>(&token)) {
        if (*bracket_token == BracketToken::CLOSE) {
            throw SyntaxError("Read: Unexpcted )");
        }
        return ReadList(tokenizer);
    } else {
        throw SyntaxError("Read: Unexpected token");
    }
}
