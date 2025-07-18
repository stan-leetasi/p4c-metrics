#ifndef IR_IRUTILS_H_
#define IR_IRUTILS_H_

#include <initializer_list>
#include <vector>

#include "lib/source_file.h"

namespace P4 {

class Transform;

namespace IR {

// Forward-declare some IR classes that are used in function declarations.
class BoolLiteral;
class StringLiteral;
class Constant;
class Expression;
class BaseListExpression;
class Literal;
class Node;
class StatOrDecl;
class StructExpression;
class Type;
class Type_Bits;
template <typename T>
class IndexedVector;

/// Utility functions for generating IR nodes.
//
// Some of these are just thin wrappers around functions in the IR, but it's nice having everything
// in one place.

/* =========================================================================================
 *  Types
 * ========================================================================================= */

/// @returns a representation of bit<> that is just wide enough to fit the given value.
const Type_Bits *getBitTypeToFit(int value);

/* =========================================================================================
 *  Expressions
 * ========================================================================================= */

/// @returns a constant with the maximum big_int value that can fit into this bit width.
/// Implicitly converts boolean types to a bit vector of width one with value 1.
const IR::Constant *getMaxValueConstant(const Type *t, const Util::SourceInfo &srcInfo = {});

/// @returns the "default" value for a given type.
/// The resulting expression will have the specified srcInfo position.
/// The current mapping as defined in the P4 specification is:
/// Type_Bits           0
/// Type_Boolean        false
/// Type_InfInt         0
/// Type_Enum           first member
/// Type_SerEnum        first member
/// Type_Error          NoError
/// Type_String         ""
/// Type_Header         InvalidHeader
/// Type_HeaderUnion    InvalidHeaderUnion
/// Type_StructLike     StructExpression (fields filled with getDefaultValue)
/// Type_Fragment       recurses into getDefaultValue
/// Type_BaseList       ListExpression (fields filled with getDefaultValue)
/// Type_Array          HeaderStackExpression (fields filled with getDefaultValue)
/// Definition: https://p4.org/p4-spec/docs/P4-16-working-spec.html#sec-default-values
const IR::Expression *getDefaultValue(const Type *type, const Util::SourceInfo &srcInfo = {},
                                      bool valueRequired = false);

/// Converts a bool literal into a constant of type Type_Bits and width 1.
/// The value is 1, if the bool literal is true, 0 otherwise.
const IR::Constant *convertBoolLiteral(const IR::BoolLiteral *lit);

/// Given an StructExpression, returns a flat vector of the expressions contained in that
/// struct. Unfortunately, list and struct expressions are similar but have no common ancestors.
/// This is why we require two separate methods.
/// Note that this function will fail if the type of @param structExpr is not a Type_Name.
std::vector<const Expression *> flattenStructExpression(const StructExpression *structExpr);

/// Given an BaseListExpression, returns a flat vector of the expressions contained in that
/// list.
std::vector<const Expression *> flattenListExpression(const BaseListExpression *listExpr);

/// Given a StructExpression or BaseListExpression, returns a flat vector of the expressions
/// contained in that list.
/// Note that this function will fail if the type of any input struct expression is not a Type_Name.
std::vector<const Expression *> flattenListOrStructExpression(const Expression *listLikeExpr);

/* =========================================================================================
 *  Other helper functions
 * ========================================================================================= */

/// @returns the big int value stored in a literal.
big_int getBigIntFromLiteral(const Literal *);

/// @returns the integer value stored in a literal. We use int here.
int getIntFromLiteral(const Literal *);

/// @returns the maximum value that can fit into this type.
/// This function assumes a big int value, meaning it only supports bit vectors and booleans.
// This is 2^(t->size) - 1 for unsigned and 2^(t->size - 1) - 1 for signed.
big_int getMaxBvVal(const Type *t);

/// @returns the maximum big_int value that can fit into this bit width.
big_int getMaxBvVal(int bitWidth);

/// @returns the minimum value that can fit into this type.
// This is 0 for unsigned and -(2^(t->size - 1)) for signed.
big_int getMinBvVal(const Type *t);

/// A helper useful when returning multiple statements out of a Transform's function that should
/// return a statement. While an IR::BlockStatement can accept a vector of statements for each of
/// its child statements, some other places (namely IR::IfStatmenent, IR::SwitchStatement /
/// IR::SwitchCase) only accept a statement. In these case, the visitor will fail with a BUG if the
/// returned value is not IR::StatOrDecl. Passing the arguments through this function (or one of its
/// overloads) works around this by wrapping the statements into a BlockStatement if the parent node
/// cannot accept them directly.
///
/// This would be usually used as follows
/// const IR::Node *preorder(IR::SomeStatement *stmt) {
///      ...
///      return IR::inlineBlock(*this, {a, b, c});
/// }
///
/// @returns IR::IndexedVector<IR::StatOrDecl> * containing the values if the parent is
/// IR::BlockStatement, and IR::BlockStatement * containing the values otherwise. As a special case,
/// returns the sole statement if there is just one passed to the function.
const IR::Node *inlineBlock(const Transform &, std::initializer_list<const IR::StatOrDecl *>);
const IR::Node *inlineBlock(const Transform &, const IR::IndexedVector<IR::StatOrDecl> &);
const IR::Node *inlineBlock(const Transform &, IR::IndexedVector<IR::StatOrDecl> &&);

}  // namespace IR

}  // namespace P4

#endif /* IR_IRUTILS_H_ */
