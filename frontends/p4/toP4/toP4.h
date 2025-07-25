/*
Copyright 2013-present Barefoot Networks, Inc.

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

#ifndef FRONTENDS_P4_TOP4_TOP4_H_
#define FRONTENDS_P4_TOP4_TOP4_H_

#include <filesystem>
#include <iostream>
#include <optional>
#include <vector>

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/sourceCodeBuilder.h"

namespace P4 {

/**
This pass converts a P4-16 IR into a P4 source (text) program.
It can optionally emit as comments a representation of the program IR.
*/
class ToP4 : public Inspector, ResolutionContext {
 protected:
    /// precedence of current IR::Operation
    int expressionPrecedence = DBPrint::Prec_Low;
    bool isDeclaration = true;    /// current type is a declaration
    bool showIR;                  /// if true dump IR as comments
    bool withinArgument = false;  /// if true we are within a method call argument
    bool noIncludes = false;      /// If true do not generate #include statements.
                                  /// Used for debugging.

    struct VecPrint {
        cstring separator;
        cstring terminator;

        VecPrint(const char *sep, const char *term) : separator(sep), terminator(term) {}
    };

    struct ListPrint {
        cstring start;
        cstring end;

        ListPrint(const char *start, const char *end) : start(start), end(end) {}
    };

    // maintained as stacks
    std::vector<VecPrint> vectorSeparator;
    size_t vectorSeparator_init_apply_size = 0;
    std::vector<ListPrint> listTerminators;
    size_t listTerminators_init_apply_size = 0;

    void setVecSep(const char *sep, const char *term = nullptr) {
        vectorSeparator.push_back(VecPrint(sep, term));
    }
    void doneVec() {
        BUG_CHECK(!vectorSeparator.empty(), "Empty vectorSeparator");
        vectorSeparator.pop_back();
    }
    VecPrint getSep() {
        BUG_CHECK(!vectorSeparator.empty(), "Empty vectorSeparator");
        return vectorSeparator.back();
    }

    void doneList() {
        BUG_CHECK(!listTerminators.empty(), "Empty listTerminators");
        listTerminators.pop_back();
    }
    /// @returns the file that contains the node, if the node is part of a system file.
    std::optional<cstring> ifSystemFile(const IR::Node *node);
    /// dump node IR tree up to depth - in the form of a comment
    void dump(unsigned depth, const IR::Node *node = nullptr, unsigned adjDepth = 0);
    unsigned curDepth() const;
    bool printAnnotations(const IR::IAnnotated *ann);

 public:
    // Output is constructed here
    Util::SourceCodeBuilder &builder;
    /* FIXME  -- simplify this by getting rid of the 'builder' object and just emitting
     * directly to the ostream.  The SourceCodeBuilder object does not appear to add any
     * useful functionality the ostream does not already provide; it just serves to
     * obfuscate the code */
    std::ostream *outStream = nullptr;
    /** If this is set to non-empty, some declarations
        that come from libraries and models are not
        emitted. */
    std::optional<std::filesystem::path> mainFile;

    ToP4(Util::SourceCodeBuilder &builder, bool showIR) : showIR(showIR), builder(builder) {
        visitDagOnce = false;
        setName("ToP4");
    }

    ToP4(std::ostream *outStream, bool showIR) : ToP4(*new Util::SourceCodeBuilder(), showIR) {
        this->outStream = outStream;
    }

    ToP4(Util::SourceCodeBuilder &builder, bool showIR, std::filesystem::path mainFile)
        : ToP4(builder, showIR) {
        this->mainFile = mainFile;
    }
    ToP4(std::ostream *outStream, bool showIR, std::filesystem::path mainFile)
        : ToP4(outStream, showIR) {
        this->mainFile = mainFile;
    }

    ToP4() : ToP4(*new Util::SourceCodeBuilder(), false) {}

    using Inspector::preorder;

    void setnoIncludesArg(bool condition) { noIncludes = condition; }

    void setListTerm(const char *start, const char *end) {
        listTerminators.push_back(ListPrint(start, end));
    }
    Visitor::profile_t init_apply(const IR::Node *node) override;
    void end_apply(const IR::Node *node) override;

    bool process(const IR::Type_StructLike *t, const char *name);
    // types
    bool preorder(const IR::Type_Boolean *t) override;
    bool preorder(const IR::Type_Varbits *t) override;
    bool preorder(const IR::Type_Bits *t) override;
    bool preorder(const IR::Type_InfInt *t) override;
    bool preorder(const IR::Type_String *t) override;
    bool preorder(const IR::Type_Var *t) override;
    bool preorder(const IR::Type_Dontcare *t) override;
    bool preorder(const IR::Type_Void *t) override;
    bool preorder(const IR::Type_Error *t) override;
    bool preorder(const IR::Type_Struct *t) override { return process(t, "struct"); }
    bool preorder(const IR::Type_Header *t) override { return process(t, "header"); }
    bool preorder(const IR::Type_HeaderUnion *t) override { return process(t, "header_union"); }
    bool preorder(const IR::Type_Package *t) override;
    bool preorder(const IR::Type_Parser *t) override;
    bool preorder(const IR::Type_Control *t) override;
    bool preorder(const IR::Type_Name *t) override;
    bool preorder(const IR::Type_Array *t) override;
    bool preorder(const IR::Type_Specialized *t) override;
    bool preorder(const IR::Type_Enum *t) override;
    bool preorder(const IR::Type_SerEnum *t) override;
    bool preorder(const IR::Type_Typedef *t) override;
    bool preorder(const IR::Type_Newtype *t) override;
    bool preorder(const IR::Type_Extern *t) override;
    bool preorder(const IR::Type_Unknown *t) override;
    bool preorder(const IR::Type_BaseList *t) override;
    bool preorder(const IR::Type *t) override {
        builder.append(t->toString());
        return false;
    }
    bool preorder(const IR::Type_SpecializedCanonical *t) override {
        BUG("%1%: specialized canonical type in IR tree", t);
        return false;
    }

    // declarations
    bool preorder(const IR::Declaration_Constant *cst) override;
    bool preorder(const IR::Declaration_Variable *v) override;
    bool preorder(const IR::Declaration_Instance *t) override;
    bool preorder(const IR::Declaration_MatchKind *d) override;

    // expressions
    bool preorder(const IR::Dots *e) override;
    bool preorder(const IR::NamedDots *e) override;
    bool preorder(const IR::Constant *c) override;
    bool preorder(const IR::AbstractSlice *slice) override;
    bool preorder(const IR::BoolLiteral *b) override;
    bool preorder(const IR::StringLiteral *s) override;
    bool preorder(const IR::PathExpression *p) override;
    bool preorder(const IR::Cast *c) override;
    bool preorder(const IR::Operation_Binary *b) override;
    bool preorder(const IR::Operation_Unary *u) override;
    bool preorder(const IR::ArrayIndex *a) override;
    bool preorder(const IR::TypeNameExpression *e) override;
    bool preorder(const IR::Mux *a) override;
    bool preorder(const IR::ConstructorCallExpression *e) override;
    bool preorder(const IR::Member *e) override;
    bool preorder(const IR::SelectCase *e) override;
    bool preorder(const IR::SelectExpression *e) override;
    bool preorder(const IR::ListExpression *e) override;
    bool preorder(const IR::P4ListExpression *e) override;
    bool preorder(const IR::StructExpression *e) override;
    bool preorder(const IR::Invalid *e) override;
    bool preorder(const IR::InvalidHeader *e) override;
    bool preorder(const IR::InvalidHeaderUnion *e) override;
    bool preorder(const IR::HeaderStackExpression *e) override;
    bool preorder(const IR::MethodCallExpression *e) override;
    bool preorder(const IR::DefaultExpression *e) override;
    bool preorder(const IR::This *e) override;

    // vectors
    bool preorder(const IR::Vector<IR::ActionListElement> *v) override;
    bool preorder(const IR::Vector<IR::Annotation> *v) override;
    bool preorder(const IR::Vector<IR::Entry> *v) override;
    bool preorder(const IR::Vector<IR::Expression> *v) override;
    bool preorder(const IR::Vector<IR::Argument> *v) override;
    bool preorder(const IR::Vector<IR::KeyElement> *v) override;
    bool preorder(const IR::Vector<IR::Method> *v) override;
    bool preorder(const IR::Vector<IR::Node> *v) override;
    bool preorder(const IR::Vector<IR::SelectCase> *v) override;
    bool preorder(const IR::Vector<IR::SwitchCase> *v) override;
    bool preorder(const IR::Vector<IR::Type> *v) override;
    bool preorder(const IR::IndexedVector<IR::Declaration_ID> *v) override;
    bool preorder(const IR::IndexedVector<IR::Declaration> *v) override;
    bool preorder(const IR::IndexedVector<IR::Node> *v) override;
    bool preorder(const IR::IndexedVector<IR::ParserState> *v) override;
    bool preorder(const IR::IndexedVector<IR::StatOrDecl> *v) override;

    // statements
    bool preorder(const IR::AssignmentStatement *s) override;
    bool preorder(const IR::OpAssignmentStatement *s) override;
    bool preorder(const IR::BlockStatement *s) override;
    bool preorder(const IR::MethodCallStatement *s) override;
    bool preorder(const IR::EmptyStatement *s) override;
    bool preorder(const IR::ReturnStatement *s) override;
    bool preorder(const IR::BreakStatement *s) override;
    bool preorder(const IR::ContinueStatement *s) override;
    bool preorder(const IR::ExitStatement *s) override;
    bool preorder(const IR::SwitchCase *s) override;
    bool preorder(const IR::SwitchStatement *s) override;
    bool preorder(const IR::IfStatement *s) override;
    bool preorder(const IR::ForStatement *s) override;
    bool preorder(const IR::ForInStatement *s) override;

    // misc
    bool preorder(const IR::NamedExpression *ne) override;
    bool preorder(const IR::Argument *arg) override;
    bool preorder(const IR::Path *p) override;
    bool preorder(const IR::Parameter *p) override;
    bool preorder(const IR::Annotation *a) override;
    bool preorder(const IR::P4Program *program) override;
    bool preorder(const IR::P4Control *c) override;
    bool preorder(const IR::P4Action *c) override;
    bool preorder(const IR::ParserState *s) override;
    bool preorder(const IR::P4Parser *c) override;
    bool preorder(const IR::TypeParameters *p) override;
    bool preorder(const IR::ParameterList *p) override;
    bool preorder(const IR::Method *p) override;
    bool preorder(const IR::Function *function) override;

    bool preorder(const IR::ExpressionValue *v) override;
    bool preorder(const IR::ActionListElement *ale) override;
    bool preorder(const IR::ActionList *v) override;
    bool preorder(const IR::Key *v) override;
    bool preorder(const IR::Property *p) override;
    bool preorder(const IR::TableProperties *t) override;
    bool preorder(const IR::EntriesList *l) override;
    bool preorder(const IR::Entry *e) override;
    bool preorder(const IR::P4Table *c) override;
    bool preorder(const IR::P4ValueSet *c) override;

    // in case it is accidentally called on a V1Program
    bool preorder(const IR::V1Program *) override { return false; }
};

std::string toP4(const IR::INode *node);
void dumpP4(const IR::INode *node);

}  // namespace P4

#endif /* FRONTENDS_P4_TOP4_TOP4_H_ */
