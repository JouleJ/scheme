#pragma once

#include <cstddef>  // int64_t
#include <string>
#include <vector>
#include <memory>

class Object : public std::enable_shared_from_this<Object> {
    size_t refs_ = 0;
public:
    virtual ~Object() = default;
    void IncreaseRefs();
    void DecreaseRefs();
    size_t GetRefs() const;
    virtual std::string ToString() const = 0;
    virtual bool IsEqualTo(const std::shared_ptr<const Object>& other) const = 0;
    virtual bool IsLessThan(const std::shared_ptr<const Object>& other) const = 0;
};

class Number : public Object {
    int64_t value_;

public:
    Number(int64_t);
    virtual ~Number() = default;
    int64_t GetValue() const;
    virtual std::string ToString() const override;
    virtual bool IsEqualTo(const std::shared_ptr<const Object>& other) const override;
    virtual bool IsLessThan(const std::shared_ptr<const Object>& other) const override;
};

class Boolean : public Object {
    bool value_;

public:
    Boolean(bool);
    virtual ~Boolean() = default;
    bool GetValue() const;
    virtual std::string ToString() const override;
    virtual bool IsEqualTo(const std::shared_ptr<const Object>& other) const override;
    virtual bool IsLessThan(const std::shared_ptr<const Object>& other) const override;
};

class Symbol : public Object {
    std::string name_;

public:
    Symbol(const std::string&);
    virtual ~Symbol() = default;
    const std::string& GetName() const;
    virtual std::string ToString() const override;
    virtual bool IsEqualTo(const std::shared_ptr<const Object>& other) const override;
    virtual bool IsLessThan(const std::shared_ptr<const Object>& other) const override;
};

class Cell : public Object {
    std::shared_ptr<Object> first_, second_;

public:
    Cell(const std::shared_ptr<Object>&, const std::shared_ptr<Object>&);
    virtual ~Cell() = default;
    std::shared_ptr<Object> GetFirst() const;
    std::shared_ptr<Object> GetSecond() const;
    void SetFirst(const std::shared_ptr<Object>&);
    void SetSecond(const std::shared_ptr<Object>&);
    virtual std::string ToString() const override;
    virtual bool IsEqualTo(const std::shared_ptr<const Object>& other) const override;
    virtual bool IsLessThan(const std::shared_ptr<const Object>& other) const override;
};

class Scope;

class Lambda : public Object {
    std::vector<std::string> arg_names_;
    std::weak_ptr<Scope> scope_;
    std::vector<std::shared_ptr<Object>> expressions_;
    std::vector<std::shared_ptr<Scope>> local_scopes_;

public:
    Lambda(const std::vector<std::string>&,
           const std::weak_ptr<Scope>&,
           const std::vector<std::shared_ptr<Object>>&);
    virtual ~Lambda();
    std::shared_ptr<Object> Call(const std::vector<std::shared_ptr<Object>>&);
    virtual std::string ToString() const override;
    virtual bool IsEqualTo(const std::shared_ptr<const Object>& other) const override;
    virtual bool IsLessThan(const std::shared_ptr<const Object>& other) const override;
};

std::string ToString(const std::shared_ptr<const Object>&);
std::string ListToString(std::shared_ptr<const Object>);

bool Equal(const std::shared_ptr<const Object>&, const std::shared_ptr<const Object>&);
bool Less(const std::shared_ptr<const Object>&, const std::shared_ptr<const Object>&);
bool LessOrEqual(const std::shared_ptr<const Object>&, const std::shared_ptr<const Object>&);
bool Greater(const std::shared_ptr<const Object>&, const std::shared_ptr<const Object>&);
bool GreaterOrEqual(const std::shared_ptr<const Object>&, const std::shared_ptr<const Object>&);

std::shared_ptr<Object> Add(const std::shared_ptr<const Object>&,
                            const std::shared_ptr<const Object>&);
std::shared_ptr<Object> Subtract(const std::shared_ptr<const Object>&,
                                 const std::shared_ptr<const Object>&);
std::shared_ptr<Object> Multiply(const std::shared_ptr<const Object>&,
                                 const std::shared_ptr<const Object>&);
std::shared_ptr<Object> Divide(const std::shared_ptr<const Object>&,
                               const std::shared_ptr<const Object>&);

bool AsBoolean(const std::shared_ptr<const Object>&);
std::shared_ptr<Object> Not(const std::shared_ptr<const Object>&);

std::shared_ptr<Object> GetNumberConstant(int64_t);
std::shared_ptr<Object> GetBooleanConstant(bool);

template <class T>
std::shared_ptr<T> As(const std::shared_ptr<Object>& obj) {
    T* ptr = dynamic_cast<T*>(obj.get());  // what if ptr == nullptr?
    return std::shared_ptr<T>(obj, ptr);
}

template <class T>
bool Is(const std::shared_ptr<Object>& obj) {
    return dynamic_cast<T*>(obj.get()) != nullptr;
}
