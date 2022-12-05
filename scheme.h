#pragma once

#include <string>
#include <functional>
#include <map>
#include "object.h"

using Command = std::function<std::shared_ptr<Object>(std::vector<std::shared_ptr<Object>>)>;

class Scope {
    std::weak_ptr<Scope> parent_;
    std::map<std::string, std::shared_ptr<Object>> variables_;
    size_t refs_ = 0;

public:
    Scope();
    Scope(const std::weak_ptr<Scope>&);

    void AddRef();
    void DelRef();
    size_t GetRefs() const;

    std::shared_ptr<Object>* FindVariable(const std::string&); // return nullptr if no such variable
    std::shared_ptr<Object> GetVariable(const std::string&); // throws exception if no such variable
    void SetVariable(const std::string&, const std::shared_ptr<Object>&);
    void SetLocalVariable(const std::string&, const std::shared_ptr<Object>&);
};

class Interpreter {
    std::map<std::string, Command> commands_;
    std::shared_ptr<Scope> scope_;

    void InitCommands();

public:
    Interpreter();
    Interpreter(const std::shared_ptr<Scope>&);
    ~Interpreter();
    std::shared_ptr<Object> Eval(const std::shared_ptr<Object>&);
    std::string Run(const std::string&);
};
