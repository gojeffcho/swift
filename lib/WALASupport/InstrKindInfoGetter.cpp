#include "swift/WALASupport/InstrKindInfoGetter.h"
#include "swift/Demangling/Demangle.h"
#include <string>
#include <list>

using namespace swift;
using std::string;
using std::list;

InstrKindInfoGetter::InstrKindInfoGetter(SILInstruction* instr, WALAIntegration* wala, unordered_map<void*, jobject>* allNodes, raw_ostream* outs) {
	this->instr = instr;
	this->wala = wala;
	this->allNodes = allNodes;
	this->outs = outs;
}

jobject InstrKindInfoGetter::handleApplyInst() {
	// ValueKind indentifier
	if (outs != NULL) {
		*outs << "<< ApplyInst >>" << "\n";
	}

	// Cast the instr 
	ApplyInst *castInst = cast<ApplyInst>(instr);

	string funcName = Demangle::demangleSymbolAsString(castInst->getReferencedFunction()->getName());
	if (outs != NULL) {
		*outs << "\t [funcName] " << funcName << "\n";
	}
	jobject nameNode = wala->makeConstant(funcName.c_str());

	// if (funcName indicates that castInst is an built-in operator)
	//    create a built-in operator node for it
	// else
	//    create a function calling node for it
	// there are 2 possibilities: 
	//   1. the function is a built-in operator in WALA
	//   2. the function is not a built-in operator in WALA
	list<jobject> params;
	for (unsigned i = 0; i < castInst->getNumArguments(); ++i) {
		SILValue v = castInst->getArgument(i);
		if (allNodes->find(v.getOpaqueValue()) != allNodes->end()) {
			jobject child = allNodes->at(v.getOpaqueValue());
			params.push_back(child);
		} else {
			// This should not happen in the end after we finish this class. We should have a CAst node in the map for each and every argument
		}

		if (outs != NULL) {
			*outs << "\t [ARG] #" << i << ": " << v;
			*outs << "\t [ADDR] #" << i << ":" << v.getOpaqueValue() << "\n";
		}
	}

	jobject call = (*wala)->makeNode(102, nameNode, (*wala)->makeArray(&params)); // 102 stands for CALL

	return call;
}

jobject InstrKindInfoGetter::handleStringLiteralInst() {
	// ValueKind indentifier
	
	if (outs != NULL) {
		*outs << "<< StringLiteralInst >>" << "\n";
	}

	// Cast the instr to access methods
	StringLiteralInst *castInst = cast<StringLiteralInst>(instr);

	// Value: the string data for the literal, in UTF-8.
	StringRef value = castInst->getValue();
	
	if (outs != NULL) {
		*outs << "\t [value] " << value << "\n";
	}

	// Encoding: the desired encoding of the text.
	string encoding;
	switch (castInst->getEncoding()) {
		case StringLiteralInst::Encoding::UTF8: {
			encoding = "UTF8";
			break;
		}
		case StringLiteralInst::Encoding::UTF16: {
			encoding = "UTF16";
			break;
		}
		case StringLiteralInst::Encoding::ObjCSelector: {
			encoding = "ObjCSelector";
			break;
		}
	}
	
	if (outs != NULL) {
		*outs << "\t [encoding] " << encoding << "\n";
	}

	// Count: encoding-based length of the string literal in code units.
	uint64_t codeUnitCount = castInst->getCodeUnitCount();
	
	if (outs != NULL) {
		*outs << "\t [codeUnitCount] " << codeUnitCount << "\n";
	}

	// Call WALA in Java
	jobject walaConstant = wala->makeConstant(value);
	allNodes->insert(std::make_pair(instr,walaConstant));
	return walaConstant;
}

jobject InstrKindInfoGetter::handleConstStringLiteralInst() {
	// ValueKind indentifier
	if (outs != NULL) {
		*outs << "<< handleConstStringLiteralInst >>" << "\n";
	}

	// Cast the instr to access methods
	ConstStringLiteralInst *castInst = cast<ConstStringLiteralInst>(instr);

	// Value: the string data for the literal, in UTF-8.
	StringRef value = castInst->getValue();
	if (outs != NULL) {
		*outs << "\t [value] " << value << "\n";
	}

	// Encoding: the desired encoding of the text.
	string encoding;
	switch (castInst->getEncoding()) {
		case ConstStringLiteralInst::Encoding::UTF8: {
			encoding = "UTF8";
			break;
		}
		case ConstStringLiteralInst::Encoding::UTF16: {
			encoding = "UTF16";
			break;
		}
	}
	if (outs != NULL) {
		*outs << "\t [ENCODING]: " << encoding << "\n";
	}

	// Count: encoding-based length of the string literal in code units.
	uint64_t codeUnitCount = castInst->getCodeUnitCount();
	if (outs != NULL) {
		*outs << "\t [COUNT]: " << codeUnitCount << "\n";
	}

	// Call WALA in Java
	jobject walaConstant = wala->makeConstant(value);

	return walaConstant;
}

void InstrKindInfoGetter::handleFunctionRefInst() {
	// ValueKind identifier
	if (outs != NULL) {
		*outs << "<< FunctionRefInst >>" << "\n";
	}

	// Cast the instr to access methods
	FunctionRefInst *castInst = cast<FunctionRefInst>(instr);

	// Demangled FunctionRef name
	if (outs != NULL) {
		*outs << "=== [FUNC] Ref'd: ";
	}
	
	string funcName = Demangle::demangleSymbolAsString(castInst->getReferencedFunction()->getName());
	if (outs != NULL) {
		*outs << funcName << "\n";
	}
}

ValueKind InstrKindInfoGetter::get() {
	auto instrKind = instr->getKind();
	jobject node = nullptr;

	switch (instrKind) {
	
		case ValueKind::SILPHIArgument:
		case ValueKind::SILFunctionArgument:
		case ValueKind::SILUndef: {		
			*outs << "<< Not an instruction >>" << "\n";
			break;
		}
		
		case ValueKind::AllocBoxInst: {
			*outs << "<< AllocBoxInst >>" << "\n";
			break;
		}
	
		case ValueKind::ApplyInst: {
			node = handleApplyInst();
			break;
		}
		
		case ValueKind::PartialApplyInst: {
			*outs << "<< PartialApplyInst >>" << "\n";
			break;
		}
		
		case ValueKind::IntegerLiteralInst: {
			*outs << "<< IntegerLiteralInst >>" << "\n";
			IntegerLiteralInst* castInst = cast<IntegerLiteralInst>(instr);
			APInt value = castInst->getValue();
			node = wala->makeConstant(value.getSExtValue());
			break;
		}
		
		case ValueKind::FloatLiteralInst: {
			*outs << "<< FloatLiteralInst >>" << "\n";
			break;
		}
		
		case ValueKind::StringLiteralInst: {
			node = handleStringLiteralInst();
			break;
		}
		
		case ValueKind::ConstStringLiteralInst: {
			node = handleConstStringLiteralInst();
			break;
		}
		
		case ValueKind::AllocValueBufferInst: {
			*outs << "<< AllocValueBufferInst >>" << "\n";
			break;
		}
		
		case ValueKind::ProjectValueBufferInst: {
			*outs << "<< ProjectValueBufferInst >>" << "\n";
			break;
		}
		
		case ValueKind::DeallocValueBufferInst: {
			*outs << "<< DeallocValueBufferInst >>" << "\n";
			break;
		}
		
		case ValueKind::ProjectBoxInst: {
			*outs << "<< ProjectBoxInst >>" << "\n";
			break;
		}
		
		case ValueKind::ProjectExistentialBoxInst: {
			*outs << "<< ProjectExistentialBoxInst >>" << "\n";
			break;
		}
		
		case ValueKind::FunctionRefInst: {
			handleFunctionRefInst();
			break;
		}
		
		case ValueKind::BuiltinInst: {
			*outs << "<< BuiltinInst >>" << "\n";
			break;
		}
		
		case ValueKind::OpenExistentialAddrInst:
		case ValueKind::OpenExistentialBoxInst:
		case ValueKind::OpenExistentialBoxValueInst:
		case ValueKind::OpenExistentialMetatypeInst:
		case ValueKind::OpenExistentialRefInst:
		case ValueKind::OpenExistentialValueInst: {
			*outs << "<< OpenExistential[Addr/Box/BoxValue/Metatype/Ref/Value]Inst >>" << "\n";
			break;
		}
		
		// UNARY_INSTRUCTION(ID) <see ParseSIL.cpp:2248>
		// DEFCOUNTING_INSTRUCTION(ID) <see ParseSIL.cpp:2255>
		
		case ValueKind::DebugValueInst: {
			*outs << "<< DebugValueInst >>" << "\n";
			break;
		}
		
		case ValueKind::DebugValueAddrInst: {
			*outs << "<< DebugValueAddrInst >>" << "\n";
			break;
		}
		
		case ValueKind::UncheckedOwnershipConversionInst: {
			*outs << "<< UncheckedOwnershipConversionInst >>" << "\n";
			break;
		}
		
		case ValueKind::LoadInst: {
			*outs << "<< LoadInst >>" << "\n";
			break;
		}
		
		case ValueKind::LoadBorrowInst: {
			*outs << "<< LoadBorrowInst >>" << "\n";
			break;
		}
		
		case ValueKind::BeginBorrowInst: {
			*outs << "<< BeginBorrowInst >>" << "\n";
			break;
		}
		
		case ValueKind::LoadUnownedInst: {
			*outs << "<< LoadUnownedInst >>" << "\n";
			break;
		}
		
		case ValueKind::LoadWeakInst: {
			*outs << "<< LoadWeakInst >>" << "\n";
			break;
		}
		
		case ValueKind::MarkDependenceInst: {
			*outs << "<< MarkDependenceInst >>" << "\n";
			break;
		}
		
		case ValueKind::KeyPathInst: {
			*outs << "<< KeyPathInst >>" << "\n";
			break;
		}
		
		case ValueKind::UncheckedRefCastInst:
		case ValueKind::UncheckedAddrCastInst:
		case ValueKind::UncheckedTrivialBitCastInst:
		case ValueKind::UncheckedBitwiseCastInst:
		case ValueKind::UpcastInst:
		case ValueKind::AddressToPointerInst:
		case ValueKind::BridgeObjectToRefInst:
		case ValueKind::BridgeObjectToWordInst:
		case ValueKind::RefToRawPointerInst:
		case ValueKind::RawPointerToRefInst:
		case ValueKind::RefToUnownedInst:
		case ValueKind::UnownedToRefInst:
		case ValueKind::RefToUnmanagedInst:
		case ValueKind::UnmanagedToRefInst:
		case ValueKind::ThinFunctionToPointerInst:
		case ValueKind::PointerToThinFunctionInst:
		case ValueKind::ThinToThickFunctionInst:
		case ValueKind::ThickToObjCMetatypeInst:
		case ValueKind::ObjCToThickMetatypeInst:
		case ValueKind::ConvertFunctionInst:
		case ValueKind::ObjCExistentialMetatypeToObjectInst:
		case ValueKind::ObjCMetatypeToObjectInst: {
			*outs << "<< Conversion Instruction >>" << "\n";
  			break;
  		}
  		
  		case ValueKind::PointerToAddressInst: {
			*outs << "<< PointerToAddressInst >>" << "\n";
			break;
		}
		
		case ValueKind::RefToBridgeObjectInst: {
			*outs << "<< RefToBridgeObjectInst >>" << "\n";
			break;
		}
		
		case ValueKind::UnconditionalCheckedCastAddrInst:
		case ValueKind::CheckedCastAddrBranchInst:
		case ValueKind::UncheckedRefCastAddrInst: {
			*outs << "<< Indirect checked conversion instruction >>" << "\n";
			break;
		}
		
		case ValueKind::UnconditionalCheckedCastValueInst: {
			*outs << "<< UnconditionalCheckedCastValueInst >>" << "\n";
			break;
		}
		
		case ValueKind::UnconditionalCheckedCastInst:
		case ValueKind::CheckedCastValueBranchInst:
		case ValueKind::CheckedCastBranchInst: {
			*outs << "<< Checked conversion instruction >>" << "\n";
			break;
		}
		
		case ValueKind::MarkUninitializedInst: {
			*outs << "<< MarkUninitializedInst >>" << "\n";
			break;
		}
		
		case ValueKind::MarkUninitializedBehaviorInst: {
			*outs << "<< MarkUninitializedBehaviorInst >>" << "\n";
			break;
		}
		
		case ValueKind::MarkFunctionEscapeInst: {
			*outs << "<< MarkFunctionEscapeInst >>" << "\n";
			break;
		}
		
		case ValueKind::StoreInst: {
			*outs << "<< StoreInst >>" << "\n";
			break;
		}
		
		case ValueKind::EndBorrowInst: {
			*outs << "<< EndBorrowInst >>" << "\n";
			break;
		}
		
		case ValueKind::BeginAccessInst:
		case ValueKind::BeginUnpairedAccessInst:
		case ValueKind::EndAccessInst:
		case ValueKind::EndUnpairedAccessInst: {
			*outs << "<< Access Instruction >>" << "\n";
			break;
		}
		
		case ValueKind::StoreBorrowInst:
		case ValueKind::AssignInst:
		case ValueKind::StoreUnownedInst:
		case ValueKind::StoreWeakInst: {
			*outs << "<< Access Instruction >>" << "\n";
			break;
		}

		case ValueKind::AllocStackInst: {
			*outs << "<< AllocStack Instruction >>" << "\n";
			break;
		}
		case ValueKind::MetatypeInst: {		
			*outs << "<< MetatypeInst >>" << "\n";
			break;
		}
		
		case ValueKind::AllocRefInst:
		case ValueKind::AllocRefDynamicInst: {
			*outs << "<< Alloc[Ref/RefDynamic] Instruction >>" << "\n";
			break;
		}
		
		case ValueKind::DeallocStackInst: {		
			*outs << "<< DeallocStackInst >>" << "\n";
			break;
		}
		
		case ValueKind::DeallocRefInst: {		
			*outs << "<< DeallocRefInst >>" << "\n";
			break;
		}
		
		case ValueKind::DeallocPartialRefInst: {		
			*outs << "<< DeallocPartialRefInst >>" << "\n";
			break;
		}
		
		case ValueKind::DeallocBoxInst: {		
			*outs << "<< DeallocBoxInst >>" << "\n";
			break;
		}
		
		case ValueKind::ValueMetatypeInst: 
		case ValueKind::ExistentialMetatypeInst: {		
			*outs << "<< [Value/Existential]MetatypeInst >>" << "\n";
			break;
		}
		
		case ValueKind::DeallocExistentialBoxInst: {		
			*outs << "<< DeallocExistentialBoxInst >>" << "\n";
			break;
		}
		
		case ValueKind::TupleInst: {		
			*outs << "<< TupleInst >>" << "\n";
			break;
		}
		
		case ValueKind::EnumInst: {		
			*outs << "<< EnumInst >>" << "\n";
			break;
		}
		
		case ValueKind::InitEnumDataAddrInst:
		case ValueKind::UncheckedEnumDataInst:
		case ValueKind::UncheckedTakeEnumDataAddrInst: {		
			*outs << "<< EnumData Instruction >>" << "\n";
			break;
		}
		
		case ValueKind::InjectEnumAddrInst: {		
			*outs << "<< InjectEnumAddrInst >>" << "\n";
			break;
		}
		
		case ValueKind::TupleElementAddrInst:
		case ValueKind::TupleExtractInst: {		
			*outs << "<< Tuple Instruction >>" << "\n";
			break;
		}
		
		case ValueKind::ReturnInst: {		
			*outs << "<< ReturnInst >>" << "\n";
			break;
		}
		
		case ValueKind::ThrowInst: {		
			*outs << "<< ThrowInst >>" << "\n";
			break;
		}
		
		case ValueKind::BranchInst: {		
			*outs << "<< BranchInst >>" << "\n";
			break;
		}
		
		case ValueKind::CondBranchInst: {		
			*outs << "<< CondBranchInst >>" << "\n";
			break;
		}
		
		case ValueKind::UnreachableInst: {		
			*outs << "<< UnreachableInst >>" << "\n";
			break;
		}
		
		case ValueKind::ClassMethodInst:
		case ValueKind::SuperMethodInst:
		case ValueKind::DynamicMethodInst: {		
			*outs << "<< DeallocRefInst >>" << "\n";
			break;
		}
		
		case ValueKind::WitnessMethodInst: {		
			*outs << "<< WitnessMethodInst >>" << "\n";
			break;
		}
		
		case ValueKind::CopyAddrInst: {		
			*outs << "<< CopyAddrInst >>" << "\n";
			break;
		}
		
		case ValueKind::BindMemoryInst: {		
			*outs << "<< BindMemoryInst >>" << "\n";
			break;
		}
		
		case ValueKind::StructInst: {		
			*outs << "<< StructInst >>" << "\n";
			break;
		}
		
		case ValueKind::StructElementAddrInst:
		case ValueKind::StructExtractInst: {		
			*outs << "<< Struct Instruction >>" << "\n";
			break;
		}
		
		case ValueKind::RefElementAddrInst: {		
			*outs << "<< RefElementAddrInst >>" << "\n";
			break;
		}
		
		case ValueKind::RefTailAddrInst: {		
			*outs << "<< RefTailAddrInst >>" << "\n";
			break;
		}
		
		case ValueKind::IsNonnullInst: {		
			*outs << "<< IsNonnullInst >>" << "\n";
			break;
		}
		
		case ValueKind::IndexAddrInst: {		
			*outs << "<< IndexAddrInst >>" << "\n";
			break;
		}
		
		case ValueKind::TailAddrInst: {		
			*outs << "<< TailAddrInst >>" << "\n";
			break;
		}
		
		case ValueKind::IndexRawPointerInst: {		
			*outs << "<< IndexRawPointerInst >>" << "\n";
			break;
		}
		
		case ValueKind::ObjCProtocolInst: {		
			*outs << "<< ObjCProtocolInst >>" << "\n";
			break;
		}
		
		case ValueKind::AllocGlobalInst: {		
			*outs << "<< AllocGlobalInst >>" << "\n";
			break;
		}
		
		case ValueKind::GlobalAddrInst: {		
			*outs << "<< GlobalAddrInst >>" << "\n";
			break;
		}
		
		case ValueKind::SelectEnumInst: {		
			*outs << "<< SelectEnumInst >>" << "\n";
			break;
		}
		
		case ValueKind::SelectEnumAddrInst: {		
			*outs << "<< DeallocRefInst >>" << "\n";
			break;
		}
		
		case ValueKind::SwitchEnumInst: {		
			*outs << "<< SwitchEnumInst >>" << "\n";
			break;
		}
		
		case ValueKind::SwitchEnumAddrInst: {		
			*outs << "<< SwitchEnumAddrInst >>" << "\n";
			break;
		}
		
		case ValueKind::SwitchValueInst: {		
			*outs << "<< SwitchValueInst >>" << "\n";
			break;
		}
		
		case ValueKind::SelectValueInst: {		
			*outs << "<< SelectValueInst >>" << "\n";
			break;
		}
		
		case ValueKind::DeinitExistentialAddrInst: {		
			*outs << "<< DeinitExistentialAddrInst >>" << "\n";
			break;
		}
		
		case ValueKind::DeinitExistentialValueInst: {		
			*outs << "<< DeinitExistentialValueInst >>" << "\n";
			break;
		}
		
		case ValueKind::InitExistentialAddrInst: {		
			*outs << "<< InitExistentialAddrInst >>" << "\n";
			break;
		}
		
		case ValueKind::InitExistentialValueInst: {		
			*outs << "<< InitExistentialValueInst >>" << "\n";
			break;
		}
		
		case ValueKind::AllocExistentialBoxInst: {		
			*outs << "<< AllocExistentialBoxInst >>" << "\n";
			break;
		}
		
		case ValueKind::InitExistentialRefInst: {		
			*outs << "<< InitExistentialRefInst >>" << "\n";
			break;
		}
		
		case ValueKind::InitExistentialMetatypeInst: {		
			*outs << "<< InitExistentialMetatypeInst >>" << "\n";
			break;
		}
		
		case ValueKind::DynamicMethodBranchInst: {		
			*outs << "<< DynamicMethodBranchInst >>" << "\n";
			break;
		}
		
		case ValueKind::ProjectBlockStorageInst: {		
			*outs << "<< ProjectBlockStorageInst >>" << "\n";
			break;
		}
		
		case ValueKind::InitBlockStorageHeaderInst: {		
			*outs << "<< InitBlockStorageHeaderInst >>" << "\n";
			break;
		}		
		
		default: {
// 			outfile 	<< "\t\t xxxxx Not a handled inst type \n";
			break;
		}
	}

	if (node != nullptr) {
		allNodes->insert(std::make_pair(instr, node)); // insert the node into the hash map
		wala->print(node);
	}
	return instrKind;
}