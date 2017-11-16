#ifndef SWIFT_INSTRKINDINFOGETTER_H
#define SWIFT_INSTRKINDINFOGETTER_H

#include "swift/WALASupport/WALAWalker.h" // included for WALAIntegration
#include "swift/WALASupport/BasicBlockLabeller.h"
#include "swift/WALASupport/SymbolTable.h"
#include <string>

using std::string;

namespace swift {

class InstrKindInfoGetter {
public:
	// If not NULL, debugging info will be printed via outs
	InstrKindInfoGetter(SILInstruction* instr, WALAIntegration* wala, 
						unordered_map<void*, jobject>* nodeMap, list<jobject>* nodeList, 
						SymbolTable* symbolTable, BasicBlockLabeller* labeller,
						raw_ostream* outs = NULL);
	
	ValueKind get();
private:
	// member variables
	SILInstruction* instr;
	WALAIntegration* wala;
	unordered_map<void*, jobject>* nodeMap;
	list<jobject>* nodeList;
	SymbolTable* symbolTable;
	BasicBlockLabeller* labeller;
	raw_ostream* outs;

	// member functions

	// This function finds a CAst node in using the key. The node will be removed from the nodeList
	// If the key corresponds to a variable, a new VAR CAst node will be created and returned
	// nullptr will be returned if such node does not exist
	jobject findAndRemoveCAstNode(void* key);

	bool isBuiltInFunction(SILFunction* function);
	bool isUnaryOperator(SILFunction* function);
	bool isBinaryOperator(SILFunction* function);
	Identifier getBuiltInOperatorName(SILFunction* function);
	jobject getOperatorCAstType(Identifier name);

	jobject handleApplyInst();
	jobject handleStringLiteralInst();
	jobject handleConstStringLiteralInst();
	jobject handleFunctionRefInst();

	jobject handleStoreInst();
	jobject handleBranchInst();
	jobject handleCondBranchInst();
	jobject handleAssignInst();
	jobject handleIntegerLiteralInst();
};

} // end namespace swift


#endif