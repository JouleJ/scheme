#include "scheme.h"
#include "tokenizer.h"
#include "parser.h"
#include "error.h"
#include <sstream>
#include <vector>

static std::vector<std::shared_ptr<Object>> UnfoldList(std::shared_ptr<Object> object) {
    std::vector<std::shared_ptr<Object>> result;
    while (object != nullptr) {
        Cell* cell = dynamic_cast<Cell*>(object.get());
        if (cell == nullptr) {
            throw RuntimeError(std::string("Expected list, but got: ") + ToString(object));
        }
        result.push_back(cell->GetFirst());
        object = cell->GetSecond();
    }
    return result;
}

static void FailEvaluation(const std::vector<std::shared_ptr<Object>>& args) {
    std::string msg = "Failed to evaluate: (";
    for (const auto& arg : args) {
        msg.push_back(' ');
        msg += ToString(arg);
    }
    msg.push_back(')');
    throw RuntimeError(msg);
}

static std::string SymbolToName(const std::shared_ptr<const Object>& object) {
    const Symbol* symbol = dynamic_cast<const Symbol*>(object.get());
    if (symbol == nullptr) {
        throw RuntimeError(std::string("Expected symbol, but got: ") + ToString(object));
    }
    return symbol->GetName();
}

Scope::Scope() {
}

Scope::Scope(const std::weak_ptr<Scope>& parent): parent_(parent) {
}

void Scope::AddRef() {
    ++refs_;
}

void Scope::DelRef() {
    --refs_;
}

size_t Scope::GetRefs() const {
    return refs_;
}

std::shared_ptr<Object>* Scope::FindVariable(const std::string& name) {
    const auto iter = variables_.find(name);
    if (iter != variables_.end()) {
        return &(iter->second);
    }
    if (!parent_.expired()) {
        return parent_.lock()->FindVariable(name);
    }
    return nullptr;
}

std::shared_ptr<Object> Scope::GetVariable(const std::string& name) {
    std::shared_ptr<Object>* ptr = FindVariable(name);
    if (ptr != nullptr) {
        return *ptr;
    }
    throw NameError(std::string("No such variable: ") + name);
}

void Scope::SetVariable(const std::string& name, const std::shared_ptr<Object>& value) {
    std::shared_ptr<Object>* ptr = FindVariable(name);
    if (ptr != nullptr) {
        *ptr = value;
    } else {
        SetLocalVariable(name, value);
    }
}

void Scope::SetLocalVariable(const std::string& name, const std::shared_ptr<Object>& value) {
    variables_.insert(std::make_pair(name, value));
}

template <typename T>
class CheckTypeCommand {
    Interpreter* interpreter_;

public:
    CheckTypeCommand(Interpreter* interpreter) : interpreter_(interpreter) {
    }

    std::shared_ptr<Object> operator()(std::vector<std::shared_ptr<Object>> args) const {
        const size_t n = args.size();
        if (n != 2) {
            FailEvaluation(args);
        }
        args[1] = interpreter_->Eval(args[1]);
        return GetBooleanConstant(dynamic_cast<T*>(args[1].get()) != nullptr);
    }
};

void Interpreter::InitCommands() {
    commands_["quote"] = [](std::vector<std::shared_ptr<Object>> args) {
        const size_t n = args.size();
        if (n != 2) {
            FailEvaluation(args);
        }
        return args[1];
    };
    commands_["number?"] = CheckTypeCommand<Number>(this);
    commands_["boolean?"] = CheckTypeCommand<Boolean>(this);
    commands_["pair?"] = CheckTypeCommand<Cell>(this);
    commands_["symbol?"] = CheckTypeCommand<Symbol>(this);
    commands_["="] = [this](std::vector<std::shared_ptr<Object>> args) {
        const size_t n = args.size();
        for (size_t i = 1; i < n; ++i) {
            args[i] = Eval(args[i]);
            if (dynamic_cast<Number*>(args[i].get()) == nullptr) {
                FailEvaluation(args);
            }
        }
        for (size_t i = 2; i < n; ++i) {
            if (!Equal(args[1], args[i])) {
                return GetBooleanConstant(false);
            }
        }
        return GetBooleanConstant(true);
    };
    commands_["<"] = [this](std::vector<std::shared_ptr<Object>> args) {
        const size_t n = args.size();
        for (size_t i = 1; i < n; ++i) {
            args[i] = Eval(args[i]);
            if (dynamic_cast<Number*>(args[i].get()) == nullptr) {
                FailEvaluation(args);
            }
        }
        for (size_t i = 2; i < n; ++i) {
            if (!Less(args[i - 1], args[i])) {
                return GetBooleanConstant(false);
            }
        }
        return GetBooleanConstant(true);
    };
    commands_[">"] = [this](std::vector<std::shared_ptr<Object>> args) {
        const size_t n = args.size();
        for (size_t i = 1; i < n; ++i) {
            args[i] = Eval(args[i]);
            if (dynamic_cast<Number*>(args[i].get()) == nullptr) {
                FailEvaluation(args);
            }
        }
        for (size_t i = 2; i < n; ++i) {
            if (!Greater(args[i - 1], args[i])) {
                return GetBooleanConstant(false);
            }
        }
        return GetBooleanConstant(true);
    };
    commands_["<="] = [this](std::vector<std::shared_ptr<Object>> args) {
        const size_t n = args.size();
        for (size_t i = 1; i < n; ++i) {
            args[i] = Eval(args[i]);
            if (dynamic_cast<Number*>(args[i].get()) == nullptr) {
                FailEvaluation(args);
            }
        }
        for (size_t i = 2; i < n; ++i) {
            if (!LessOrEqual(args[i - 1], args[i])) {
                return GetBooleanConstant(false);
            }
        }
        return GetBooleanConstant(true);
    };
    commands_[">="] = [this](std::vector<std::shared_ptr<Object>> args) {
        const size_t n = args.size();
        for (size_t i = 1; i < n; ++i) {
            args[i] = Eval(args[i]);
            if (dynamic_cast<Number*>(args[i].get()) == nullptr) {
                FailEvaluation(args);
            }
        }
        for (size_t i = 2; i < n; ++i) {
            if (!GreaterOrEqual(args[i - 1], args[i])) {
                return GetBooleanConstant(false);
            }
        }
        return GetBooleanConstant(true);
    };
    commands_["+"] = [this](std::vector<std::shared_ptr<Object>> args) {
        const size_t n = args.size();
        for (size_t i = 1; i < n; ++i) {
            args[i] = Eval(args[i]);
        }
        std::shared_ptr<Object> result = GetNumberConstant(0);
        for (size_t i = 1; i < n; ++i) {
            result = Add(result, args[i]);
        }
        return result;
    };
    commands_["-"] = [this](std::vector<std::shared_ptr<Object>> args) {
        const size_t n = args.size();
        for (size_t i = 1; i < n; ++i) {
            args[i] = Eval(args[i]);
        }
        if (n <= 1) {
            FailEvaluation(args);
        }
        std::shared_ptr<Object> result = args[1];
        for (size_t i = 2; i < n; ++i) {
            result = Subtract(result, args[i]);
        }
        return result;
    };
    commands_["*"] = [this](std::vector<std::shared_ptr<Object>> args) {
        const size_t n = args.size();
        for (size_t i = 1; i < n; ++i) {
            args[i] = Eval(args[i]);
        }
        std::shared_ptr<Object> result = GetNumberConstant(1);
        for (size_t i = 1; i < n; ++i) {
            result = Multiply(result, args[i]);
        }
        return result;
    };
    commands_["/"] = [this](std::vector<std::shared_ptr<Object>> args) {
        const size_t n = args.size();
        for (size_t i = 1; i < n; ++i) {
            args[i] = Eval(args[i]);
        }
        if (n <= 1) {
            FailEvaluation(args);
        }
        std::shared_ptr<Object> result = args[1];
        for (size_t i = 2; i < n; ++i) {
            result = Divide(result, args[i]);
        }
        return result;
    };
    commands_["not"] = [this](std::vector<std::shared_ptr<Object>> args) {
        const size_t n = args.size();
        for (size_t i = 1; i < n; ++i) {
            args[i] = Eval(args[i]);
        }
        if (n != 2) {
            FailEvaluation(args);
        }
        return Not(args[1]);
    };
    commands_["and"] = [this](std::vector<std::shared_ptr<Object>> args) {
        const size_t n = args.size();
        for (size_t i = 1; i < n; ++i) {
            args[i] = Eval(args[i]);
            if (!AsBoolean(args[i])) {
                return args[i];
            }
        }
        if (n > 1) {
            return args.back();
        } else {
            return GetBooleanConstant(true);
        }
    };
    commands_["or"] = [this](std::vector<std::shared_ptr<Object>> args) {
        const size_t n = args.size();
        for (size_t i = 1; i < n; ++i) {
            args[i] = Eval(args[i]);
            if (AsBoolean(args[i])) {
                return args[i];
            }
        }
        if (n > 1) {
            return args.back();
        } else {
            return GetBooleanConstant(false);
        }
    };
    commands_["min"] = [this](std::vector<std::shared_ptr<Object>> args) {
        const size_t n = args.size();
        for (size_t i = 1; i < n; ++i) {
            args[i] = Eval(args[i]);
        }
        if (n <= 1) {
            FailEvaluation(args);
        }
        for (size_t i = 1; i < n; ++i) {
            Number* number = dynamic_cast<Number*>(args[i].get());
            if (number == nullptr) {
                FailEvaluation(args);
            }
        }
        std::shared_ptr<Object> result = args[1];
        for (size_t i = 2; i < n; ++i) {
            if (Less(args[i], result)) {
                result = args[i];
            }
        }
        return result;
    };
    commands_["max"] = [this](std::vector<std::shared_ptr<Object>> args) {
        const size_t n = args.size();
        for (size_t i = 1; i < n; ++i) {
            args[i] = Eval(args[i]);
        }
        if (n <= 1) {
            FailEvaluation(args);
        }
        for (size_t i = 1; i < n; ++i) {
            Number* number = dynamic_cast<Number*>(args[i].get());
            if (number == nullptr) {
                FailEvaluation(args);
            }
        }
        std::shared_ptr<Object> result = args[1];
        for (size_t i = 2; i < n; ++i) {
            if (Greater(args[i], result)) {
                result = args[i];
            }
        }
        return result;
    };
    commands_["abs"] = [this](std::vector<std::shared_ptr<Object>> args) {
        const size_t n = args.size();
        for (size_t i = 1; i < n; ++i) {
            args[i] = Eval(args[i]);
        }
        if (n != 2) {
            FailEvaluation(args);
        }
        Number* number = dynamic_cast<Number*>(args[1].get());
        if (number == nullptr) {
            FailEvaluation(args);
        }
        if (number->GetValue() >= 0) {
            return args[1];
        } else {
            return GetNumberConstant(-number->GetValue());
        }
    };
    commands_["null?"] = [this](std::vector<std::shared_ptr<Object>> args) {
        const size_t n = args.size();
        for (size_t i = 1; i < n; ++i) {
            args[i] = Eval(args[i]);
        }
        if (n != 2) {
            FailEvaluation(args);
        }
        return GetBooleanConstant(args[1] == nullptr);
    };
    commands_["list?"] = [this](std::vector<std::shared_ptr<Object>> args) {
        const size_t n = args.size();
        for (size_t i = 1; i < n; ++i) {
            args[i] = Eval(args[i]);
        }
        if (n != 2) {
            FailEvaluation(args);
        }
        std::shared_ptr<Object> object = args[1];
        while (object != nullptr) {
            Cell* cell = dynamic_cast<Cell*>(object.get());
            if (cell == nullptr) {
                return GetBooleanConstant(false);
            }
            object = cell->GetSecond();
        }
        return GetBooleanConstant(true);
    };
    commands_["cons"] = [this](std::vector<std::shared_ptr<Object>> args) {
        const size_t n = args.size();
        for (size_t i = 1; i < n; ++i) {
            args[i] = Eval(args[i]);
        }
        if (n != 3) {
            FailEvaluation(args);
        }
        return std::shared_ptr<Object>(new Cell(args[1], args[2]));
    };
    commands_["car"] = [this](std::vector<std::shared_ptr<Object>> args) {
        const size_t n = args.size();
        for (size_t i = 1; i < n; ++i) {
            args[i] = Eval(args[i]);
        }
        if (n != 2) {
            FailEvaluation(args);
        }
        Cell* cell = dynamic_cast<Cell*>(args[1].get());
        if (cell == nullptr) {
            FailEvaluation(args);
        }
        return cell->GetFirst();
    };
    commands_["cdr"] = [this](std::vector<std::shared_ptr<Object>> args) {
        const size_t n = args.size();
        for (size_t i = 1; i < n; ++i) {
            args[i] = Eval(args[i]);
        }
        if (n != 2) {
            FailEvaluation(args);
        }
        Cell* cell = dynamic_cast<Cell*>(args[1].get());
        if (cell == nullptr) {
            FailEvaluation(args);
        }
        return cell->GetSecond();
    };
    commands_["list"] = [this](std::vector<std::shared_ptr<Object>> args) {
        const size_t n = args.size();
        for (size_t i = 1; i < n; ++i) {
            args[i] = Eval(args[i]);
        }
        std::shared_ptr<Object> result = nullptr;
        for (size_t i = n - 1; i > 0; --i) {
            result = std::shared_ptr<Object>(new Cell(args[i], result));
        }
        return result;
    };
    commands_["list-ref"] = [this](std::vector<std::shared_ptr<Object>> args) {
        const size_t n = args.size();
        for (size_t i = 1; i < n; ++i) {
            args[i] = Eval(args[i]);
        }
        if (n != 3) {
            FailEvaluation(args);
        }
        std::vector<std::shared_ptr<Object>> list;
        try {
            list = UnfoldList(args[1]);
        } catch (...) {
            FailEvaluation(args);
        }
        Number* number = dynamic_cast<Number*>(args[2].get());
        if (number == nullptr) {
            FailEvaluation(args);
        }
        size_t index = number->GetValue();
        if (index >= list.size()) {
            FailEvaluation(args);
        }
        return list[index];
    };
    commands_["list-tail"] = [this](std::vector<std::shared_ptr<Object>> args) {
        const size_t n = args.size();
        for (size_t i = 1; i < n; ++i) {
            args[i] = Eval(args[i]);
        }
        if (n != 3) {
            FailEvaluation(args);
        }
        Number* number = dynamic_cast<Number*>(args[2].get());
        if (number == nullptr) {
            FailEvaluation(args);
        }
        size_t to_drop = number->GetValue();
        std::shared_ptr<Object> object = args[1];
        for (size_t i = 0; i != to_drop; ++i) {
            Cell* cell = dynamic_cast<Cell*>(object.get());
            if (cell == nullptr) {
                FailEvaluation(args);
            }
            object = cell->GetSecond();
        }
        return object;
    };
    commands_["define"] = [this](const std::vector<std::shared_ptr<Object>>& args) {
        const size_t n = args.size();
        if (n <= 1) {
            throw SyntaxError("Invalid define");
        }
        if (Symbol* symbol = dynamic_cast<Symbol*>(args[1].get())) {
            if (n != 3) {
                throw SyntaxError("Invalid define");
            }
            scope_->SetVariable(symbol->GetName(), nullptr); // to make variable visible inside its own scope
            std::shared_ptr<Object> value = Eval(args[2]);
            scope_->SetVariable(symbol->GetName(), value);
        } else {
            if (n < 3) {
                throw SyntaxError("Invalid define");
            }
            std::string func_name;
            std::vector<std::string> arg_names;
            try {
                std::vector<std::shared_ptr<Object>> list = UnfoldList(args[1]);
                const size_t list_size = list.size();
                if (list_size == 0) {
                    throw;
                }
                func_name = SymbolToName(list.front());
                arg_names.reserve(list_size - 1);
                for (size_t i = 1; i < list_size; ++i) {
                    arg_names.push_back(SymbolToName(list[i]));
                }
            } catch (...) {
                throw SyntaxError("Invalid define");
            }
            std::vector<std::shared_ptr<Object>> expressions;
            expressions.reserve(n - 2);
            for (size_t i = 2; i < n; ++i) {
                expressions.push_back(args[i]);
            }
            scope_->SetVariable(func_name, nullptr); // to make variable visible inside its own scope
            Lambda* func = new Lambda(arg_names, scope_, expressions);
            scope_->SetVariable(func_name, std::shared_ptr<Object>(func));
        }
        return nullptr;
    };
    commands_["set!"] = [this](const std::vector<std::shared_ptr<Object>>& args) {
        const size_t n = args.size();
        if (n != 3) {
            throw SyntaxError("Invalid set!");
        }
        Symbol* symbol = dynamic_cast<Symbol*>(args[1].get());
        if (symbol == nullptr) {
            throw SyntaxError("Invalid set!");
        }
        std::shared_ptr<Object> value = Eval(args[2]);
        std::shared_ptr<Object>* ptr = scope_->FindVariable(symbol->GetName());
        if (ptr == nullptr) {
            throw NameError(std::string("Variable doesn't yet exist: ") + symbol->GetName());
        }
        *ptr = value;
        return nullptr;
    };
    commands_["set-car!"] = [this](const std::vector<std::shared_ptr<Object>>& args) {
        const size_t n = args.size();
        if (n != 3) {
            throw SyntaxError("Invalid set-car!");
        }
        Symbol* symbol = dynamic_cast<Symbol*>(args[1].get());
        if (symbol == nullptr) {
            throw SyntaxError("Invalid set-car!");
        }
        std::shared_ptr<Object> value = Eval(args[2]);
        std::shared_ptr<Object>* ptr = scope_->FindVariable(symbol->GetName());
        if (ptr == nullptr) {
            throw NameError(std::string("Variable doesn't yet exist: ") + symbol->GetName());
        }
        Cell* cell = dynamic_cast<Cell*>(ptr->get());
        if (cell == nullptr) {
            throw RuntimeError("Cannot set-car! on a non-pair");
        }
        cell->SetFirst(value);
        return nullptr;
    };
    commands_["set-cdr!"] = [this](const std::vector<std::shared_ptr<Object>>& args) {
        const size_t n = args.size();
        if (n != 3) {
            throw SyntaxError("Invalid set-cdr!");
        }
        Symbol* symbol = dynamic_cast<Symbol*>(args[1].get());
        if (symbol == nullptr) {
            throw SyntaxError("Invalid set-cdr!");
        }
        std::shared_ptr<Object> value = Eval(args[2]);
        std::shared_ptr<Object>* ptr = scope_->FindVariable(symbol->GetName());
        if (ptr == nullptr) {
            throw NameError(std::string("Variable doesn't yet exist: ") + symbol->GetName());
        }
        Cell* cell = dynamic_cast<Cell*>(ptr->get());
        if (cell == nullptr) {
            throw RuntimeError("Cannot set-car! on a non-pair");
        }
        cell->SetSecond(value);
        return nullptr;
    };
    commands_["lambda"] = [this](const std::vector<std::shared_ptr<Object>>& args) {
        const size_t n = args.size();
        if (n < 3) {
            throw SyntaxError("Invalid lambda");
        }
        std::vector<std::string> arg_names;
        try {
            std::vector<std::shared_ptr<Object>> list = UnfoldList(args[1]);
            arg_names.reserve(list.size());
            for (const auto& element: list) {
                arg_names.push_back(SymbolToName(element));
            }
        } catch (...) {
            throw SyntaxError("Invalid lambda");
        }
        std::vector<std::shared_ptr<Object>> expressions;
        expressions.reserve(n - 2);
        for (size_t i = 2; i < n; ++i) {
            expressions.push_back(args[i]);
        }
        Lambda* lambda = new Lambda(arg_names, scope_, expressions);
        return std::shared_ptr<Object>(lambda);
    };
    commands_["if"] = [this](const std::vector<std::shared_ptr<Object>>& args) -> std::shared_ptr<Object> {
        const size_t n = args.size();
        if (n != 3 && n != 4) {
            throw SyntaxError("Invalid if");
        }
        bool condition = AsBoolean(Eval(args[1]));
        if (n == 3) {
            if (condition) {
                return Eval(args[2]);
            } else {
                return nullptr;
            }
        } else {
            if (condition) {
                return Eval(args[2]);
            } else {
                return Eval(args[3]);
            }
        }
    };
}

Interpreter::Interpreter(): scope_(new Scope) {
    InitCommands();
    scope_->AddRef();
}

Interpreter::Interpreter(const std::shared_ptr<Scope>& scope): scope_(scope) {
    InitCommands();
    scope_->AddRef();
}

Interpreter::~Interpreter() {
    scope_->DelRef();
}

std::shared_ptr<Object> Interpreter::Eval(const std::shared_ptr<Object>& object) {
    if (dynamic_cast<Number*>(object.get()) != nullptr) {
        return object;
    } else if (dynamic_cast<Boolean*>(object.get()) != nullptr) {
        return object;
    } else if (Symbol* symbol = dynamic_cast<Symbol*>(object.get())) {
        return scope_->GetVariable(symbol->GetName());
    } else if (dynamic_cast<Cell*>(object.get()) != nullptr) {
        std::vector<std::shared_ptr<Object>> list = UnfoldList(object);
        const size_t n = list.size();
        if (n != 0) {
            if (Symbol* command = dynamic_cast<Symbol*>(list.front().get())) {
                const auto iter = commands_.find(command->GetName());
                if (iter != commands_.end()) {
                    return (iter->second)(list);
                }
            }
            list.front() = Eval(list.front());
            if (Lambda* lambda = dynamic_cast<Lambda*>(list.front().get())) {
                std::vector<std::shared_ptr<Object>> args = list;
                args.erase(args.begin());
                for (auto& element: args) {
                    element = Eval(element);
                }
                return lambda->Call(args);
            }
        }
    }
    throw RuntimeError(std::string("Cannot evaluate: ") + ToString(object));
}

std::string Interpreter::Run(const std::string& code) {
    std::stringstream ss;
    ss << code;
    Tokenizer tokenizer(&ss);
    std::shared_ptr<Object> object = Read(&tokenizer);
    if (!tokenizer.IsEnd()) {
        throw SyntaxError("Unexpected input");
    }
    return ToString(Eval(object));
}
