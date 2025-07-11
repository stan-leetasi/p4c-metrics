#include "ir/irutils.h"

#include <cmath>
#include <vector>

#include "ir/indexed_vector.h"
#include "ir/ir.h"
#include "ir/vector.h"
#include "ir/visitor.h"
#include "lib/exceptions.h"

namespace P4::IR {

using namespace P4::literals;

/* =============================================================================================
 *  Types
 * ============================================================================================= */

const Type_Bits *getBitTypeToFit(int value) {
    // To represent a number N, we need ceil(log2(N + 1)) bits.
    int width = ceil(log2(value + 1));
    return Type_Bits::get(width);
}

/* =============================================================================================
 *  Expressions
 * ============================================================================================= */

const IR::Constant *getMaxValueConstant(const Type *t, const Util::SourceInfo &srcInfo) {
    if (t->is<Type_Bits>()) {
        return IR::Constant::get(t, IR::getMaxBvVal(t), srcInfo);
    }
    if (t->is<Type_Boolean>()) {
        return IR::Constant::get(IR::Type_Bits::get(1), 1, srcInfo);
    }
    P4C_UNIMPLEMENTED("Maximum value calculation for type %1% not implemented.", t);
}

const IR::Constant *convertBoolLiteral(const IR::BoolLiteral *lit) {
    return IR::Constant::get(IR::Type_Bits::get(1), lit->value ? 1 : 0, lit->getSourceInfo());
}

const IR::Expression *getDefaultValue(const IR::Type *type, const Util::SourceInfo &srcInfo,
                                      bool valueRequired) {
    if (const auto *tb = type->to<IR::Type_Bits>()) {
        return IR::Constant::get(tb, 0, srcInfo);
    }
    if (type->is<IR::Type_Boolean>()) {
        return IR::BoolLiteral::get(false, srcInfo);
    }
    if (type->is<IR::Type_InfInt>()) {
        return IR::Constant::get(type, 0, srcInfo);
    }
    if (const auto *te = type->to<IR::Type_Enum>()) {
        return new IR::Member(srcInfo, new IR::TypeNameExpression(te->name),
                              te->members.at(0)->getName());
    }
    if (const auto *te = type->to<IR::Type_SerEnum>()) {
        return new IR::Cast(srcInfo, type->getP4Type(), IR::Constant::get(te->type, 0, srcInfo));
    }
    if (const auto *te = type->to<IR::Type_Error>()) {
        return new IR::Member(srcInfo, new IR::TypeNameExpression(te->name), "NoError");
    }
    if (type->is<IR::Type_String>()) {
        return new IR::StringLiteral(srcInfo, ""_cs);
    }
    if (type->is<IR::Type_Varbits>()) {
        if (valueRequired) {
            P4C_UNIMPLEMENTED("%1%: No default value for varbit types.", srcInfo);
        }
        ::P4::error(ErrorType::ERR_UNSUPPORTED, "%1% default values for varbit types", srcInfo);
        return nullptr;
    }
    if (const auto *ht = type->to<IR::Type_Header>()) {
        return new IR::InvalidHeader(ht->getP4Type());
    }
    if (const auto *hu = type->to<IR::Type_HeaderUnion>()) {
        return new IR::InvalidHeaderUnion(hu->getP4Type());
    }
    if (const auto *st = type->to<IR::Type_StructLike>()) {
        auto *vec = new IR::IndexedVector<IR::NamedExpression>();
        for (const auto *field : st->fields) {
            const auto *value = getDefaultValue(field->type, srcInfo);
            if (value == nullptr) {
                return nullptr;
            }
            vec->push_back(new IR::NamedExpression(field->name, value));
        }
        const auto *resultType = st->getP4Type();
        return new IR::StructExpression(srcInfo, resultType, resultType, *vec);
    }
    if (const auto *tf = type->to<IR::Type_Fragment>()) {
        return getDefaultValue(tf->type, srcInfo);
    }
    if (const auto *tt = type->to<IR::Type_BaseList>()) {
        auto *vec = new IR::Vector<IR::Expression>();
        for (const auto *field : tt->components) {
            const auto *value = getDefaultValue(field, srcInfo);
            if (value == nullptr) {
                return nullptr;
            }
            vec->push_back(value);
        }
        return new IR::ListExpression(srcInfo, *vec);
    }
    if (const auto *ts = type->to<IR::Type_Array>()) {
        auto *vec = new IR::Vector<IR::Expression>();
        const auto *elementType = ts->elementType;
        for (size_t i = 0; i < ts->getSize(); i++) {
            const IR::Expression *value = getDefaultValue(elementType, srcInfo);
            if (value == nullptr) {
                return nullptr;
            }
            vec->push_back(value);
        }
        const auto *resultType = ts->getP4Type();
        return new IR::HeaderStackExpression(srcInfo, resultType, *vec, resultType);
    }
    if (valueRequired) {
        P4C_UNIMPLEMENTED("%1%: No default value for type %2% (%3%).", srcInfo, type,
                          type->node_type_name());
    }
    ::P4::error(ErrorType::ERR_INVALID, "%1%: No default value for type %2% (%3%)", srcInfo, type,
                type->node_type_name());
    return nullptr;
}

std::vector<const Expression *> flattenStructExpression(const StructExpression *structExpr) {
    std::vector<const Expression *> exprList;
    // Ensure that the underlying type is a Type_StructLike.
    const auto *structType = structExpr->type->to<IR::Type_StructLike>();
    BUG_CHECK(structType != nullptr, "%1%: expected a struct-like type, received %2%",
              structExpr->type, structExpr->node_type_name());

    // We use the underlying struct type, which will gives us the right field ordering.
    for (const auto *typeField : structType->fields) {
        const auto *listElem = structExpr->getField(typeField->name);
        if (const auto *subStructExpr = listElem->expression->to<StructExpression>()) {
            auto subList = flattenStructExpression(subStructExpr);
            exprList.insert(exprList.end(), subList.begin(), subList.end());
        } else if (const auto *subListExpr = listElem->expression->to<BaseListExpression>()) {
            auto subList = flattenListExpression(subListExpr);
            exprList.insert(exprList.end(), subList.begin(), subList.end());
        } else {
            exprList.emplace_back(listElem->expression);
        }
    }
    return exprList;
}

std::vector<const Expression *> flattenListExpression(const BaseListExpression *listExpr) {
    std::vector<const Expression *> exprList;
    for (const auto *listElem : listExpr->components) {
        if (const auto *subListExpr = listElem->to<BaseListExpression>()) {
            auto subList = flattenListExpression(subListExpr);
            exprList.insert(exprList.end(), subList.begin(), subList.end());
        } else if (const auto *subStructExpr = listElem->to<IR::StructExpression>()) {
            auto subList = flattenStructExpression(subStructExpr);
            exprList.insert(exprList.end(), subList.begin(), subList.end());
        } else {
            exprList.emplace_back(listElem);
        }
    }
    return exprList;
}

/* =============================================================================================
 *  Other helper functions
 * ============================================================================================= */

big_int getBigIntFromLiteral(const Literal *l) {
    if (const auto *c = l->to<Constant>()) {
        return c->value;
    }
    if (const auto *b = l->to<BoolLiteral>()) {
        return b->value ? 1 : 0;
    }
    P4C_UNIMPLEMENTED("Literal %1% of type %2% not supported.", l, l->node_type_name());
}

int getIntFromLiteral(const Literal *l) {
    if (const auto *c = l->to<Constant>()) {
        if (!c->fitsInt()) {
            BUG("Value %1% too large for Int.", l);
        }
        return c->asInt();
    }
    if (const auto *b = l->to<BoolLiteral>()) {
        return b->value ? 1 : 0;
    }
    P4C_UNIMPLEMENTED("Literal %1% of type %2% not supported.", l, l->node_type_name());
}

big_int getMaxBvVal(int bitWidth) { return pow(big_int(2), bitWidth) - 1; }

big_int getMaxBvVal(const Type *t) {
    if (const auto *tb = t->to<Type_Bits>()) {
        return tb->isSigned ? getMaxBvVal(tb->width_bits() - 1) : getMaxBvVal(tb->width_bits());
    }
    if (t->is<Type_Boolean>()) {
        return 1;
    }
    P4C_UNIMPLEMENTED("Maximum value calculation for type %1% not implemented.", t);
}

big_int getMinBvVal(const Type *t) {
    if (const auto *tb = t->to<Type_Bits>()) {
        return (tb->isSigned) ? -(big_int(1) << tb->width_bits() - 1) : big_int(0);
    }
    if (t->is<Type_Boolean>()) {
        return 0;
    }
    P4C_UNIMPLEMENTED("Maximum value calculation for type %1% not implemented.", t);
}

std::vector<const Expression *> flattenListOrStructExpression(const Expression *listLikeExpr) {
    if (const auto *listExpr = listLikeExpr->to<IR::BaseListExpression>()) {
        return IR::flattenListExpression(listExpr);
    }
    if (const auto *structExpr = listLikeExpr->to<IR::StructExpression>()) {
        return IR::flattenStructExpression(structExpr);
    }
    P4C_UNIMPLEMENTED("Unsupported list-like expression %1% of type %2%.", listLikeExpr,
                      listLikeExpr->node_type_name());
}

template <typename Stmts>
const IR::Node *inlineBlockImpl(const Transform &t, Stmts &&stmts) {
    if (stmts.size() == 1) {
        // it could also be a declaration, and it that case, we need to wrap it in a block anyway
        if (auto *stmt = (*stmts.begin())->template to<IR::Statement>()) {
            return stmt;
        }
    }
    if (t.getParent<IR::BlockStatement>()) {
        return new IR::IndexedVector<IR::StatOrDecl>(std::forward<Stmts>(stmts));
    }
    Util::SourceInfo srcInfo;
    if (stmts.size() > 0) {  // no .empty in initializer_list!
        srcInfo = (*stmts.begin())->srcInfo;
    }
    return new IR::BlockStatement(srcInfo,
                                  IR::IndexedVector<IR::StatOrDecl>(std::forward<Stmts>(stmts)));
}

const IR::Node *inlineBlock(const Transform &t,
                            std::initializer_list<const IR::StatOrDecl *> stmts) {
    return inlineBlockImpl(t, stmts);
}

const IR::Node *inlineBlock(const Transform &t, const IR::IndexedVector<IR::StatOrDecl> &stmts) {
    return inlineBlockImpl(t, stmts);
}

const IR::Node *inlineBlock(const Transform &t, IR::IndexedVector<IR::StatOrDecl> &&stmts) {
    return inlineBlockImpl(t, std::move(stmts));
}

}  // namespace P4::IR
