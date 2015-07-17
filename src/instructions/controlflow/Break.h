

#ifndef WASMINT_BREAK_H
#define WASMINT_BREAK_H


#include <instructions/Instruction.h>

class CalledBreak {};

class Break : public Instruction {
public:
    virtual std::vector<Type*> childrenTypes() {
        return {};
    }
    virtual std::string name() {
        return "break";
    }

    virtual Type* returnType() {
        return Void::instance();
    }

    virtual StepResult execute(Thread &thread) {
        //throw CalledBreak();
        return StepResult();
    }
};



#endif //WASMINT_BREAK_H
