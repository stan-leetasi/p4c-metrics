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

#ifndef FRONTENDS_P4_14_FROMV1_0_PROGRAMSTRUCTURE_H_
#define FRONTENDS_P4_14_FROMV1_0_PROGRAMSTRUCTURE_H_

#include <set>
#include <vector>

#include "frontends/p4/callGraph.h"
#include "frontends/p4/coreLibrary.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/map.h"
#include "v1model.h"

namespace P4::P4V1 {

class ConversionContext {
 public:
    virtual ~ConversionContext() = default;
    const IR::Expression *header = nullptr;
    const IR::Expression *userMetadata = nullptr;
    const IR::Expression *standardMetadata = nullptr;
    virtual void clear() {
        header = nullptr;
        userMetadata = nullptr;
        standardMetadata = nullptr;
    }
};

/// Information about the structure of a P4-14 program, used to convert it to a P4-16 program.
class ProgramStructure {
    // In P4-14 one can have multiple objects with different types with the same name
    // In P4-16 this is not possible, so we may need to rename some objects.
    // We will preserve the original name using an @name("") annotation.
    template <typename T>
    class NamedObjectInfo {
        // If allNames is nullptr we don't check for duplicate names.
        // Key is a name, value represents how many times this name was used as a base
        // for newly generated unique names.
        std::unordered_map<cstring, int> *allNames;
        std::map<cstring, T> nameToObject;
        std::map<T, cstring> objectToNewName;

        // Iterate in order of name, but return pair<T, newname>
        class iterator {
            friend class NamedObjectInfo;

         private:
            typename std::map<cstring, T>::iterator it;
            typename std::map<T, cstring> &objToName;
            iterator(typename std::map<cstring, T>::iterator it,
                     typename std::map<T, cstring> &objToName)
                : it(it), objToName(objToName) {}

         public:
            const iterator &operator++() {
                ++it;
                return *this;
            }
            bool operator!=(const iterator &other) const { return it != other.it; }
            std::pair<T, cstring> operator*() const {
                return std::pair<T, cstring>(it->second, objToName[it->second]);
            }
        };

     public:
        explicit NamedObjectInfo(std::unordered_map<cstring, int> *allNames) : allNames(allNames) {}
        void emplace(T obj) {
            if (objectToNewName.find(obj) != objectToNewName.end()) {
                // Already done
                LOG3(" already emplaced obj " << obj);
                return;
            }

            nameToObject.emplace(obj->name, obj);
            cstring newName;

            if (allNames == nullptr || (allNames->find(obj->name) == allNames->end())) {
                newName = obj->name;
            } else {
                newName = cstring::make_unique(*allNames, obj->name, allNames->at(obj->name), '_');
            }
            if (allNames != nullptr) allNames->insert({newName, 0});
            LOG3("Discovered " << obj << " named " << newName);
            objectToNewName.emplace(obj, newName);
        }
        /// Lookup using the original name
        T get(cstring name) const { return ::P4::get(nameToObject, name); }
        /// Get the new name
        cstring get(T object) const {
            return ::P4::get(objectToNewName, object, object->name.name);
        }
        /// Get the new name from the old name
        cstring newname(cstring name) const { return get(get(name)); }
        bool contains(cstring name) const { return nameToObject.find(name) != nameToObject.end(); }
        iterator begin() { return iterator(nameToObject.begin(), objectToNewName); }
        iterator end() { return iterator(nameToObject.end(), objectToNewName); }
        void erase(cstring name) {
            allNames->erase(name);
            auto obj = get(name);
            objectToNewName.erase(obj);
            nameToObject.erase(name);
        }
    };

    std::set<cstring> included_files;

 public:
    ProgramStructure();
    virtual ~ProgramStructure() = default;

    P4V1::V1Model &v1model;
    P4::P4CoreLibrary &p4lib;

    std::unordered_map<cstring, int> allNames;
    NamedObjectInfo<const IR::Type_StructLike *> types;
    NamedObjectInfo<const IR::HeaderOrMetadata *> metadata;
    NamedObjectInfo<const IR::Header *> headers;
    NamedObjectInfo<const IR::HeaderStack *> stacks;
    NamedObjectInfo<const IR::V1Control *> controls;
    NamedObjectInfo<const IR::V1Parser *> parserStates;
    NamedObjectInfo<const IR::V1Table *> tables;
    NamedObjectInfo<const IR::ActionFunction *> actions;
    NamedObjectInfo<const IR::Counter *> counters;
    NamedObjectInfo<const IR::Register *> registers;
    NamedObjectInfo<const IR::Meter *> meters;
    NamedObjectInfo<const IR::ActionProfile *> action_profiles;
    NamedObjectInfo<const IR::FieldList *> field_lists;
    NamedObjectInfo<const IR::FieldListCalculation *> field_list_calculations;
    NamedObjectInfo<const IR::ActionSelector *> action_selectors;
    NamedObjectInfo<const IR::Type_Extern *> extern_types;
    std::map<const IR::Type_Extern *, const IR::Type_Extern *> extern_remap;
    NamedObjectInfo<const IR::Declaration_Instance *> externs;
    NamedObjectInfo<const IR::ParserValueSet *> value_sets;
    std::set<cstring> value_sets_implemented;
    std::vector<const IR::CalculatedField *> calculated_fields;
    std::map<const IR::Node *, const IR::Declaration_Instance *> globalInstances;
    P4::CallGraph<cstring> calledActions;
    P4::CallGraph<cstring> calledControls;
    P4::CallGraph<cstring> calledCounters;
    P4::CallGraph<cstring> calledMeters;
    P4::CallGraph<cstring> calledRegisters;
    P4::CallGraph<cstring> calledExterns;
    P4::CallGraph<cstring> parsers;
    std::map<cstring, IR::Vector<IR::Expression>> extracts;  // for each parser
    std::map<cstring, cstring> directCounters;               /// Maps table to direct counter.
    /// Maps table name to direct meter.
    std::map<cstring, const IR::Meter *> directMeters;
    std::map<const IR::Meter *, const IR::Declaration_Instance *> meterMap;
    std::map<cstring, const IR::Declaration_Instance *> counterMap;
    /// Field lists that appear in the program.
    ordered_set<const IR::FieldList *> allFieldLists;

    std::map<const IR::V1Table *, const IR::V1Control *> tableMapping;
    std::map<const IR::V1Table *, const IR::Apply *> tableInvocation;
    /// Some types are transformed during conversion; this maps the
    /// original P4-14 header type name to the final P4-16
    /// Type_Header.  We can't use the P4-14 type object itself as a
    /// key, because it keeps changing.
    std::map<cstring, const IR::Type *> finalHeaderType;
    /// For registers whose layout is a header, this map contains the mapping
    /// from the original layout type name to the final layout type name.
    std::map<cstring, cstring> registerLayoutType;

    /// Maps each inserted extract statement to the type of the header
    /// type that is being extracted.  The extracts will need another
    /// pass to cope with varbit fields.
    std::map<const IR::MethodCallExpression *, const IR::Type_Header *> extractsSynthesized;

    std::map<cstring, const IR::ParserState *> parserEntryPoints;
    /// Name of the serializable enum that holds one id for each field list.
    cstring fieldListsEnum;

    // P4-14 struct/header type can be converted to three types
    // of struct/header in P4-16.
    // 1) as part of the 'hdr' struct
    // 2) as part of the 'meta' struct
    // 3) as the parameters of a parser/control block
    // In case 1 and 2, the converter needs to fix the path
    // by prepending 'hdr.' or 'meta.' to the ConcreteHeaderRef.
    // In case 3. the converter only needs to convert headerRef to PathExpression
    std::set<cstring> headerTypes;
    std::set<cstring> metadataTypes;
    std::set<cstring> parameterTypes;
    std::set<cstring> metadataInstances;
    std::set<cstring> headerInstances;

    /// extra local instances to control created by primitive translation
    std::vector<const IR::Declaration *> localInstances;

    ConversionContext *conversionContext = nullptr;

    IR::Vector<IR::Type> *emptyTypeArguments = nullptr;
    const IR::Parameter *parserPacketIn = nullptr;
    const IR::Parameter *parserHeadersOut = nullptr;

    // output is constructed here
    IR::Vector<IR::Node> *declarations;

 protected:
    virtual const IR::Statement *convertPrimitive(const IR::Primitive *primitive);
    virtual void checkHeaderType(const IR::Type_StructLike *hrd, bool toStruct);

    /**
     * Like addNameAnnotation(), but prefixes a "." to make the name global. You
     * should generally prefer this method; @see addNameAnnotation() for more
     * discussion.
     */
    static IR::Vector<IR::Annotation> addGlobalNameAnnotation(
        cstring name, const IR::Vector<IR::Annotation> &annos);

    virtual const IR::ParserState *convertParser(const IR::V1Parser *,
                                                 IR::IndexedVector<IR::Declaration> *);
    virtual const IR::Statement *convertParserStatement(const IR::Expression *expr);
    virtual const IR::P4Control *convertControl(const IR::V1Control *control, cstring newName);
    virtual const IR::Declaration_Instance *convertDirectMeter(const IR::Meter *m, cstring newName);
    virtual const IR::Declaration_Instance *convertDirectCounter(const IR::Counter *c,
                                                                 cstring newName);
    virtual const IR::Declaration_Instance *convert(const IR::CounterOrMeter *cm, cstring newName);
    virtual const IR::Declaration_Instance *convertActionProfile(const IR::ActionProfile *,
                                                                 cstring newName);
    virtual const IR::P4Table *convertTable(const IR::V1Table *table, cstring newName,
                                            IR::IndexedVector<IR::Declaration> &stateful,
                                            std::map<cstring, cstring> &);
    virtual const IR::P4Action *convertAction(const IR::ActionFunction *action, cstring newName,
                                              const IR::Meter *meterToAccess,
                                              cstring counterToAccess);
    virtual const IR::Statement *convertMeterCall(const IR::Meter *meterToAccess);
    virtual const IR::Statement *convertCounterCall(cstring counterToAccess);
    virtual const IR::Type_Control *controlType(IR::ID name);
    const IR::PathExpression *getState(IR::ID dest);
    virtual const IR::Expression *counterType(const IR::CounterOrMeter *cm);
    virtual void createChecksumVerifications();
    virtual void createChecksumUpdates();
    virtual void createStructures();
    virtual cstring createType(const IR::Type_StructLike *type, bool header,
                               std::unordered_set<const IR::Type *> *converted);
    virtual void createParser();
    virtual void createControls();
    void createDeparserInternal(IR::ID deparserId, IR::Parameter *packetOut, IR::Parameter *headers,
                                std::vector<IR::Parameter *>,
                                IR::IndexedVector<IR::Declaration> controlLocals,
                                std::function<IR::BlockStatement *(IR::BlockStatement *)>);
    virtual void createDeparser();
    virtual void createMain();

 public:
    void include(cstring filename, cstring ppoptions = cstring());
    /// This inserts the names of the identifiers used in the output P4-16 programs
    /// into allNames, forcing P4-14 names that clash to be renamed.
    void populateOutputNames();
    const IR::AssignmentStatement *assign(Util::SourceInfo srcInfo, const IR::Expression *left,
                                          const IR::Expression *right, const IR::Type *type);
    virtual const IR::Expression *convertFieldList(const IR::Expression *expression);
    virtual const IR::Expression *convertHashAlgorithm(Util::SourceInfo srcInfo, IR::ID algorithm);
    virtual const IR::Expression *convertHashAlgorithms(const IR::NameList *algorithm);
    virtual const IR::Declaration_Instance *convert(const IR::Register *reg, cstring newName,
                                                    const IR::Type *regElementType = nullptr);
    virtual const IR::Type_Struct *createFieldListType(const IR::Expression *expression);
    virtual const IR::FieldListCalculation *getFieldListCalculation(const IR::Expression *);
    virtual const IR::FieldList *getFieldLists(const IR::FieldListCalculation *flc);
    virtual const IR::Expression *paramReference(const IR::Parameter *param);
    const IR::Statement *sliceAssign(const IR::Primitive *prim, const IR::Expression *left,
                                     const IR::Expression *right, const IR::Expression *mask);
    void tablesReferred(const IR::V1Control *control, std::vector<const IR::V1Table *> &out);
    bool isHeader(const IR::ConcreteHeaderRef *nhr) const;
    cstring makeUniqueName(cstring base);
    bool isFieldInList(cstring type, cstring field, const IR::FieldList *fl) const;
    /// A vector with indexes of the field lists that contain this field.
    /// Returns nullptr if the field does not appear in any list.
    virtual const IR::Vector<IR::Expression> *listIndexes(cstring type, cstring field) const;
    /// Given an expression which is supposed to be a field list
    /// return a constant representing its value in the fieldListsEnum.
    const IR::Expression *listIndex(const IR::Expression *fl) const;

    const IR::Type *explodeType(const std::vector<const IR::Type::Bits *> &fieldTypes);
    const IR::Expression *explodeLabel(const IR::Constant *value, const IR::Constant *mask,
                                       const std::vector<const IR::Type::Bits *> &fieldTypes);

    virtual IR::Vector<IR::Argument> *createApplyArguments(cstring n);

    const IR::V1Control *ingress;
    IR::ID ingressReference;

    const IR::P4Control *verifyChecksums;
    const IR::P4Control *updateChecksums;
    const IR::P4Control *deparser;
    /// Represents 'latest' P4-14 construct.
    const IR::Expression *latest;
    const int defaultRegisterWidth = 32;

    virtual void loadModel();
    void createExterns();
    void createTypes();
    virtual const IR::P4Program *create(Util::SourceInfo info);
};

}  // namespace P4::P4V1

#endif /* FRONTENDS_P4_14_FROMV1_0_PROGRAMSTRUCTURE_H_ */
