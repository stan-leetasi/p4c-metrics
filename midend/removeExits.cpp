/*
Copyright 2017 VMware, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "removeExits.h"

#include "frontends/p4/methodInstance.h"

namespace P4 {

namespace {
class CallsExit : public Inspector {
    DeclarationLookup *refMap;
    TypeMap *typeMap;
    std::set<const IR::Node *> *callers;

 public:
    bool callsExit = false;
    CallsExit(DeclarationLookup *refMap, TypeMap *typeMap, std::set<const IR::Node *> *callers)
        : refMap(refMap), typeMap(typeMap), callers(callers) {}
    void postorder(const IR::MethodCallExpression *expression) override {
        auto mi = MethodInstance::resolve(expression, refMap, typeMap);
        if (mi->isApply()) {
            auto am = mi->to<ApplyMethod>();
            CHECK_NULL(am->object);
            auto obj = am->object->getNode();
            if (callers->find(obj) != callers->end()) callsExit = true;
        } else if (mi->is<ActionCall>()) {
            auto ac = mi->to<ActionCall>();
            if (callers->find(ac->action) != callers->end()) callsExit = true;
        }
    }
    void end_apply(const IR::Node *node) override {
        LOG3(node << (callsExit ? " calls " : " does not call ") << "exit");
    }
};
}  // namespace

void DoRemoveExits::callExit(const IR::Node *node) {
    LOG3(node << " calls exit");
    callsExit.emplace(node);
}

const IR::Node *DoRemoveExits::preorder(IR::ExitStatement *statement) {
    set(TernaryBool::Yes);
    auto left = new IR::PathExpression(IR::Type::Boolean::get(), returnVar);
    auto trueVal = new IR::BoolLiteral(true);
    IR::Statement *rv = new IR::AssignmentStatement(statement->srcInfo, left, trueVal);
    if (isInContext<IR::LoopStatement>()) rv = new IR::BlockStatement({rv, new IR::BreakStatement});
    return rv;
}

const IR::Node *DoRemoveExits::preorder(IR::P4Table *table) {
    for (auto a : table->getActionList()->actionList) {
        auto path = a->getPath();
        auto decl = getDeclaration(path, true);
        BUG_CHECK(decl->is<IR::P4Action>(), "%1% is not an action", decl);
        if (callsExit.find(decl->getNode()) != callsExit.end()) {
            callExit(getOriginal());
            callExit(table);
            break;
        }
    }
    return table;
}

const IR::Node *DoRemoveExits::preorder(IR::P4Action *action) {
    LOG3("Visiting " << action);
    push();
    visit(action->body);
    auto r = hasReturned();
    if (r != TernaryBool::No) {
        callExit(action);
        callExit(getOriginal());
    }
    pop();
    prune();
    return action;
}

const IR::Node *DoRemoveExits::preorder(IR::P4Control *control) {
    HasExits he;
    he.setCalledBy(this);
    (void)control->apply(he);
    if (!he.hasExits) {
        // don't pollute the code unnecessarily
        prune();
        return control;
    }

    cstring var = nameGen.newName(variableName.string_view());
    returnVar = IR::ID(var, nullptr);
    visit(control->controlLocals, "controlLocals");

    BUG_CHECK(stack.empty(), "Non-empty stack");
    push();
    visit(control->body);
    IR::IndexedVector<IR::Declaration> stateful;
    auto decl = new IR::Declaration_Variable(returnVar, IR::Type_Boolean::get(), nullptr);
    stateful.push_back(decl);
    stateful.append(control->controlLocals);
    control->controlLocals = stateful;

    IR::IndexedVector<IR::StatOrDecl> newbody;
    auto left = new IR::PathExpression(returnVar);
    auto init = new IR::AssignmentStatement(left, new IR::BoolLiteral(false));
    newbody.push_back(init);
    newbody.append(control->body->components);
    control->body =
        new IR::BlockStatement(control->body->srcInfo, control->body->annotations, newbody);

    pop();
    BUG_CHECK(stack.empty(), "Non-empty stack");
    prune();
    return control;
}

const IR::Node *DoRemoveExits::preorder(IR::BlockStatement *statement) {
    auto block = new IR::BlockStatement(statement->srcInfo, statement->annotations);
    auto currentBlock = block;
    TernaryBool ret = TernaryBool::No;
    for (auto s : statement->components) {
        push();
        visit(s);
        currentBlock->push_back(s);
        TernaryBool r = hasReturned();
        pop();
        if (r == TernaryBool::Yes) {
            ret = r;
            break;
        } else if (r == TernaryBool::Maybe) {
            auto newBlock = new IR::BlockStatement;
            auto path = new IR::PathExpression(returnVar);
            auto condition = new IR::LNot(path);
            auto ifstat = new IR::IfStatement(condition, newBlock, nullptr);
            block->push_back(ifstat);
            currentBlock = newBlock;
            ret = r;
        }
    }
    if (!stack.empty()) set(ret);
    prune();
    return block;
}

// if (t.apply.hit()) stat1;
// becomes
// if (t.apply().hit()) if (!hasExited) stat1;
const IR::Node *DoRemoveExits::preorder(IR::IfStatement *statement) {
    push();

    CallsExit ce(this, typeMap, &callsExit);
    ce.setCalledBy(this);
    (void)statement->condition->apply(ce, getChildContext());
    auto rcond = ce.callsExit ? TernaryBool::Maybe : TernaryBool::No;

    visit(statement->ifTrue);
    if (statement->ifTrue == nullptr) statement->ifTrue = new IR::EmptyStatement();
    if (ce.callsExit) {
        auto path = new IR::PathExpression(returnVar);
        auto condition = new IR::LNot(path);
        auto newif = new IR::IfStatement(condition, statement->ifTrue, nullptr);
        statement->ifTrue = newif;
    }
    auto rt = hasReturned();
    auto rf = TernaryBool::No;
    pop();
    if (statement->ifFalse != nullptr) {
        push();
        visit(statement->ifFalse);
        if (ce.callsExit && statement->ifFalse != nullptr) {
            auto path = new IR::PathExpression(returnVar);
            auto condition = new IR::LNot(path);
            auto newif = new IR::IfStatement(condition, statement->ifFalse, nullptr);
            statement->ifFalse = newif;
        }
        rf = hasReturned();
        pop();
    }
    if (rcond == TernaryBool::Yes || (rt == TernaryBool::Yes && rf == TernaryBool::Yes))
        set(TernaryBool::Yes);
    else if (rcond == TernaryBool::No && rt == TernaryBool::No && rf == TernaryBool::No)
        set(TernaryBool::No);
    else
        set(TernaryBool::Maybe);
    prune();
    return statement;
}

const IR::Node *DoRemoveExits::preorder(IR::SwitchStatement *statement) {
    auto r = TernaryBool::No;
    CallsExit ce(this, typeMap, &callsExit);
    ce.setCalledBy(this);
    (void)statement->expression->apply(ce), getChildContext();

    /* FIXME -- alter cases in place rather than allocating a new Vector */
    IR::Vector<IR::SwitchCase> *cases = nullptr;
    if (ce.callsExit) {
        r = TernaryBool::Maybe;
        cases = new IR::Vector<IR::SwitchCase>();
    }
    for (auto &c : statement->cases) {
        push();
        visit(c);
        if (hasReturned() != TernaryBool::No)
            // this is conservative: we don't check if we cover all labels.
            r = TernaryBool::Maybe;
        if (cases != nullptr) {
            IR::Statement *stat = nullptr;
            if (c->statement != nullptr) {
                auto path = new IR::PathExpression(returnVar);
                auto condition = new IR::LNot(path);
                auto newif = new IR::IfStatement(condition, c->statement, nullptr);
                stat = new IR::BlockStatement(newif->srcInfo, {newif});
            }
            auto swcase = new IR::SwitchCase(c->srcInfo, c->label, stat);
            cases->push_back(swcase);
        }
        pop();
    }
    set(r);
    prune();
    if (cases != nullptr) statement->cases = std::move(*cases);
    return statement;
}

const IR::Node *DoRemoveExits::preorder(IR::BaseAssignmentStatement *statement) {
    CallsExit ce(this, typeMap, &callsExit);
    ce.setCalledBy(this);
    (void)statement->apply(ce, getChildContext());
    if (ce.callsExit) set(TernaryBool::Maybe);
    return statement;
}

const IR::Node *DoRemoveExits::preorder(IR::MethodCallStatement *statement) {
    CallsExit ce(this, typeMap, &callsExit);
    ce.setCalledBy(this);
    (void)statement->apply(ce, getChildContext());
    if (ce.callsExit) set(TernaryBool::Maybe);
    return statement;
}

}  // namespace P4
